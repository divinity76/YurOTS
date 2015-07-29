//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// class representing the gamestate
//////////////////////////////////////////////////////////////////////
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software Foundation,
// Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//////////////////////////////////////////////////////////////////////


#include "definitions.h"

#include <string>
#include <sstream>

#include <map>
//#include <algorithm>

#ifdef __DEBUG_CRITICALSECTION__
#include <iostream>
#include <fstream>
#endif

#include <boost/config.hpp>
#include <boost/bind.hpp>

using namespace std;

#include <stdio.h>
#include "otsystem.h"
#include "items.h"
#include "commands.h"
#include "creature.h"
#include "player.h"
#include "monster.h"
#include "npc.h"
#include "game.h"
#include "tile.h"

#include "spells.h"
#include "actions.h"
#include "ioplayer.h"
#include "chat.h"
#include "status.h"

#include "luascript.h"
#include "templates.h"
#include "houses.h"
#include "summons.h"
#include "pvparena.h"
#include <ctype.h>

#if defined __EXCEPTION_TRACER__
#include "exception.h"
extern OTSYS_THREAD_LOCKVAR maploadlock;
#endif

#define EVENT_CHECKCREATURE          123
#define EVENT_CHECKCREATUREATTACKING 124

extern LuaScript g_config;
extern Spells spells;
extern Actions actions;
extern Commands commands;
extern Chat g_chat;

extern std::vector< std::pair<unsigned long, unsigned long> > bannedIPs;

GameState::GameState(Game *game, const Range &range)
{
	this->game = game;
	game->getSpectators(range, spectatorlist);
}

#ifdef YUR_PVP_ARENA
bool GameState::isPvpArena(Creature* c)
{
	if (!c)
		return false;
	Tile *tile = game->map->getTile(c->pos);
	return tile && tile->isPvpArena();
}
#endif //YUR_PVP_ARENA

#ifdef YUR_RINGS_AMULETS
int GameState::applyAmulets(Player* player, int damage, attacktype_t atype)
{
	if (!player || atype == ATTACK_NONE)
		return damage;

	double newDamage = (double)damage;
	Item* necklace = player->getItem(SLOT_NECKLACE);
	Item* ring = player->getItem(SLOT_RING);

	if (necklace && necklace->getCharges() > 0)
	{
		if (necklace->getID() == ITEM_STONE_SKIN_AMULET)
		{
			newDamage *= 0.05;
			necklace->useCharge();
		}
		else if (necklace->getID() == ITEM_PROTECTION_AMULET)
		{
			newDamage *= 0.95;
			necklace->useCharge();
		}
		else if ((necklace->getID() == ITEM_DRAGON_NECKLACE && (atype & ATTACK_FIRE)) ||
			(necklace->getID() == ITEM_SILVER_AMULET && (atype & ATTACK_POISON)) ||
			(necklace->getID() == ITEM_STRANGE_TALISMAN && (atype & ATTACK_ENERGY)) ||
			(necklace->getID() == ITEM_ELVEN_AMULET))
		{
			newDamage *= 0.9;
			necklace->useCharge();
		}
 #ifdef YUR_DRAINS
		else if (necklace->getID() == ITEM_BRONZE_AMULET && (atype & ATTACK_MANADRAIN))
		{
			newDamage *= 0.5;
			necklace->useCharge();
		}
		else if (necklace->getID() == ITEM_GARLIC_NECKLACE && (atype & ATTACK_LIFEDRAIN))
		{
			newDamage = 0.05;
			necklace->useCharge();
		}
 #endif //YUR_DRAINS

		if (necklace->getCharges() <= 0)
			player->removeItemInventory(SLOT_NECKLACE);
	}

	if (ring && ring->getCharges() > 0)
	{
		if (ring->getID() == ITEM_MIGHT_RING)
		{
			newDamage *= 0.9;
			ring->useCharge();
		}

		if (ring->getCharges() <= 0)
			player->removeItemInventory(SLOT_RING);
	}

	return (int)newDamage;
}
#endif //YUR_RINGS_AMULETS

void GameState::onAttack(Creature* attacker, const Position& pos, const MagicEffectClass* me)
{
	Tile *tile = game->map->getTile(pos);

	if(!tile)
		return;

#ifdef YUR_PVP_ARENA
	CreatureVector arenaLosers;
#endif //YUR_PVP_ARENA

	CreatureVector::iterator cit;
	Player* attackPlayer = dynamic_cast<Player*>(attacker);
	Creature *targetCreature = NULL;
	Player *targetPlayer = NULL;
	for(cit = tile->creatures.begin(); cit != tile->creatures.end(); ++cit) {
		targetCreature = (*cit);
		targetPlayer = dynamic_cast<Player*>(targetCreature);
		bool pvpArena = false;

#ifdef TR_SUMMONS
		bool targetIsSummon = (targetCreature && targetCreature->isPlayersSummon());
		bool summonVsPlayer = (attacker && attacker->isPlayersSummon() && targetPlayer);
#endif //TR_SUMMONS

		int64_t damage = me->getDamage(targetCreature, attacker);
		int64_t manaDamage = 0;

#ifdef YUR_RINGS_AMULETS
		damage = applyAmulets(targetPlayer, damage, me->attackType);
#endif //YUR_RINGS_AMULETS

		if (damage > 0) {
			if(attackPlayer && attackPlayer->access < g_config.ACCESS_PROTECT) {
				if(targetPlayer && targetPlayer != attackPlayer && game->getWorldType() != WORLD_TYPE_NO_PVP)
					attackPlayer->pzLocked = true;
			}

			if(targetCreature->access < g_config.ACCESS_PROTECT && targetPlayer && game->getWorldType() != WORLD_TYPE_NO_PVP)
			{
#ifdef YUR_CVS_MODS
				targetPlayer->inFightTicks = std::max(g_config.PZ_LOCKED, targetPlayer->inFightTicks);
#else
				targetPlayer->inFightTicks = (long)g_config.getGlobalNumber("pzlocked", 0);
#endif //YUR_CVS_MODS
				targetPlayer->sendIcons();
			}

#ifdef YUR_PVP_ARENA
			pvpArena = isPvpArena(attacker) && isPvpArena(targetCreature);
#endif //YUR_PVP_ARENA

#ifdef TR_SUMMONS
			if ((game->getWorldType() == WORLD_TYPE_NO_PVP && !pvpArena && summonVsPlayer) ||
				(game->getWorldType() == WORLD_TYPE_NO_PVP && !pvpArena && attackPlayer && (targetPlayer || targetIsSummon) && attackPlayer->access < g_config.ACCESS_PROTECT)) {
#else
			if(game->getWorldType() == WORLD_TYPE_NO_PVP && !pvpArena && attackPlayer && targetPlayer && attackPlayer->access < ACCESS_PROTECT){
#endif //TR_SUMMONS
				damage = 0;
			}
		}

		if (damage != 0)
		{
#ifdef YUR_DRAINS
			if (me->attackType & ATTACK_MANADRAIN)
			{
				manaDamage = std::min(damage, targetCreature->mana);
				targetCreature->drainMana(manaDamage);
				damage = 0;
			}
			else
#endif //YUR_DRAINS
			{
				game->creatureApplyDamage(targetCreature, damage, damage, manaDamage
#ifdef YUR_PVP_ARENA
					, (pvpArena? &arenaLosers : NULL)
#endif //YUR_PVP_ARENA
					);
			}

#ifdef YUR_DRAINS
			if (me->attackType & ATTACK_LIFEDRAIN)
			{
				attacker->health = std::min(attacker->healthmax, attacker->health + damage);
				addCreatureState(tile, attacker, 0, 0, false);	// update attacker health
			}
#endif //YUR_DRAINS

#ifdef YUR_INVISIBLE
			if (targetCreature && !targetPlayer)
			{
				targetCreature->setInvisible(0);
				game->creatureChangeOutfit(targetCreature);
			}
#endif //YUR_INVISIBLE
		}

#ifdef TLM_SKULLS_PARTY
		if (me->offensive && game->getWorldType() == WORLD_TYPE_PVP)
			game->onPvP(attacker, targetCreature, targetCreature->health <= 0);
#endif //TLM_SKULLS_PARTY

		addCreatureState(tile, targetCreature, damage, manaDamage, me->drawblood);
	}

	//Solid ground items/Magic items (fire/poison/energy)
	MagicEffectItem *newmagicItem = me->getMagicItem(attacker, tile->isPz(),
		(tile->isBlocking(BLOCK_SOLID, true) != RET_NOERROR));

	if(newmagicItem) {

		MagicEffectItem *magicItem = tile->getFieldItem();

		if(magicItem) {
			//Replace existing magic field
			magicItem->transform(newmagicItem);

			int stackpos = tile->getThingStackPos(magicItem);
			if(tile->removeThing(magicItem)) {

				SpectatorVec list;
				SpectatorVec::iterator it;

				game->getSpectators(Range(pos, true), list);

				//players
				for(it = list.begin(); it != list.end(); ++it) {
					if(dynamic_cast<Player*>(*it)) {
						(*it)->onThingDisappear(magicItem, stackpos);
					}
				}

				//none-players
				for(it = list.begin(); it != list.end(); ++it) {
					if(!dynamic_cast<Player*>(*it)) {
						(*it)->onThingDisappear(magicItem, stackpos);
					}
				}

				tile->addThing(magicItem);

				//players
				for(it = list.begin(); it != list.end(); ++it) {
					if(dynamic_cast<Player*>(*it)) {
						(*it)->onThingAppear(magicItem);
					}
				}

				//none-players
				for(it = list.begin(); it != list.end(); ++it) {
					if(!dynamic_cast<Player*>(*it)) {
						(*it)->onThingAppear(magicItem);
					}
				}
			}
		}
		else {
			magicItem = new MagicEffectItem(*newmagicItem);
			magicItem->useThing();
			magicItem->pos = pos;

			tile->addThing(magicItem);

			SpectatorVec list;
			SpectatorVec::iterator it;

			game->getSpectators(Range(pos, true), list);

			//players
			for(it = list.begin(); it != list.end(); ++it) {
				if(dynamic_cast<Player*>(*it)) {
					(*it)->onThingAppear(magicItem);
				}
			}

			//none-players
			for(it = list.begin(); it != list.end(); ++it) {
				if(!dynamic_cast<Player*>(*it)) {
					(*it)->onThingAppear(magicItem);
				}
			}

			magicItem->isRemoved = false;
			game->startDecay(magicItem);
		}
	}

	//Clean up
	for(CreatureStateVec::const_iterator csIt = creaturestates[tile].begin(); csIt != creaturestates[tile].end(); ++csIt) {
		onAttackedCreature(tile, attacker, csIt->first, csIt->second.damage, csIt->second.drawBlood);
	}

	if(attackPlayer && attackPlayer->access < g_config.ACCESS_PROTECT) {
		//Add exhaustion
		if(me->causeExhaustion(true) /*!areaTargetVec.empty())*/)
		{
#ifdef YUR_HEAL_EXHAUST
			if (!me->offensive && me->minDamage != 0)	// healing
				attackPlayer->exhaustedTicks = g_config.EXHAUSTED_HEAL;
			else
#endif //YUR_HEAL_EXHAUST
				attackPlayer->exhaustedTicks = g_config.EXHAUSTED;
		}

		//Fight symbol
		if(me->offensive /*&& !areaTargetVec.empty()*/)
		{
#ifdef YUR_CVS_MODS
			attackPlayer->inFightTicks = std::max(g_config.PZ_LOCKED, attackPlayer->inFightTicks);
#else
			attackPlayer->inFightTicks = (long)g_config.getGlobalNumber("pzlocked", 0);
#endif //YUR_CVS_MODS
		}
	}

#ifdef YUR_PVP_ARENA
	for (CreatureVector::iterator it = arenaLosers.begin(); it != arenaLosers.end(); ++it)
	{
		Tile* tile = game->getTile((*it)->pos);
		if (tile)
			game->teleport(*it, tile->getPvpArenaExit());	// kick losers
	}
#endif //YUR_PVP_ARENA
}

void GameState::onAttack(Creature* attacker, const Position& pos, Creature* attackedCreature)
{
	bool pvpArena = false;
#ifdef YUR_PVP_ARENA
	CreatureVector arenaLosers;
	pvpArena = isPvpArena(attacker) && isPvpArena(attackedCreature);
#endif //YUR_PVP_ARENA

	//TODO: Decent formulas and such...
	int64_t damage = attacker->getWeaponDamage();
	int armor = attackedCreature->getArmor();
	int defense = attackedCreature->getDefense();

	Player* attackPlayer = dynamic_cast<Player*>(attacker);
	Player* attackedPlayer = dynamic_cast<Player*>(attackedCreature);

	if(attackedPlayer)
		attackedPlayer->addSkillShieldTry(1);

	int probability = rand() % 10000;

#ifdef YUR_CVS_MODS
	if (probability * damage < defense * 1000)
		damage = 0;
	else
		damage -= (int)(damage*(armor/100.0)*(rand()/(RAND_MAX+1.0))) + armor;
		//damage -= (int)((armor)*(rand()/(RAND_MAX+1.0))) + armor;	// wik's
#else
	if(probability * damage < defense * 10000)
		damage = 0;
	else
	{
		damage -= (armor * (10000 + rand() % 10000)) / 10000;
	}
#endif //YUR_CVS_MODS

	int64_t manaDamage = 0;

	if(attackPlayer && attackedPlayer){
		damage -= damage / 2;
	}

	if (attacker->access >= g_config.ACCESS_PROTECT)
		damage += 1337;

	if(damage < 0 || attackedCreature->access >= g_config.ACCESS_PROTECT)
		damage = 0;

	Tile* tile = game->map->getTile(pos);
	bool blood;
	if(damage != 0)
	{
#ifdef YUR_ICE_RAPIER
		if (attackPlayer)
			for (int slot = SLOT_RIGHT; slot <= SLOT_LEFT; slot++)
				if(attackPlayer->getItem(slot) && attackPlayer->getItem(slot)->getID() == ITEM_ICE_RAPIER)
					attackPlayer->removeItemInventory(slot);
#endif //YUR_ICE_RAPIER

#ifdef YUR_RINGS_AMULETS
		damage = applyAmulets(attackedPlayer, damage, ATTACK_PHYSICAL);
#endif //YUR_RINGS_AMULETS

		game->creatureApplyDamage(attackedCreature, damage, damage, manaDamage
#ifdef YUR_PVP_ARENA
			, (pvpArena? &arenaLosers : NULL)
#endif //YUR_PVP_ARENA
			);

#ifdef TLM_SKULLS_PARTY
		if (game->getWorldType() == WORLD_TYPE_PVP)
			game->onPvP(attacker, attackedCreature, attackedCreature->health <= 0);
#endif //TLM_SKULLS_PARTY

		blood = true;
	}
	else{//no draw blood
		blood = false;
	}

	addCreatureState(tile, attackedCreature, damage, manaDamage, blood);
	onAttackedCreature(tile, attacker, attackedCreature, damage,  true);

#ifdef BDB_REPLACE_SPEARS
	if (attackPlayer && attackPlayer->isUsingSpears() && random_range(1,100000) > g_config.SPEAR_LOSE_CHANCE)
	{
		Item* spear = Item::CreateItem(ITEM_SPEAR, 1);
		spear->pos = attackedCreature->pos;
		game->addThing(attackPlayer, spear->pos, spear);
	}
#endif //BDB_REPLACE_SPEARS

#ifdef YUR_PVP_ARENA
	for (CreatureVector::iterator it = arenaLosers.begin(); it != arenaLosers.end(); ++it)
	{
		Tile* tile = game->getTile((*it)->pos);
		if (tile)
			game->teleport(*it, tile->getPvpArenaExit());
	}
#endif //YUR_PVP_ARENA
}

void GameState::addCreatureState(Tile* tile, Creature* attackedCreature, int64_t damage, int64_t manaDamage, bool drawBlood)
{
	CreatureState cs;
	cs.damage = damage;
	cs.manaDamage = manaDamage;
	cs.drawBlood = drawBlood;

	creaturestates[tile].push_back( make_pair(attackedCreature, cs) );
}

void GameState::onAttackedCreature(Tile* tile, Creature *attacker, Creature* attackedCreature, int64_t damage, bool drawBlood)
{
	Player *attackedplayer = dynamic_cast<Player*>(attackedCreature);
	Position CreaturePos = attackedCreature->pos;

#ifdef TJ_MONSTER_BLOOD
	bool dead = false;
#endif //TJ_MONSTER_BLOOD

#ifdef TR_SUMMONS
	Creature* attackerMaster = attacker? attacker->getMaster() : NULL;
	if (attackerMaster && dynamic_cast<Player*>(attackerMaster))	// attacker is players summon
	{
		attackedCreature->addInflictedDamage(attacker, damage/2);
		attackedCreature->addInflictedDamage(attackerMaster, damage/2);
	}
	else
		attackedCreature->addInflictedDamage(attacker, damage);
#else
	attackedCreature->addInflictedDamage(attacker, damage);
#endif //TR_SUMMONS

	if(attackedplayer){
		attackedplayer->sendStats();
	}
	//Remove player?
	if(attackedCreature->health <= 0 && attackedCreature->isRemoved == false)
	{
#ifdef TJ_MONSTER_BLOOD
		dead = true;
#endif //TJ_MONSTER_BLOOD
#ifdef JD_DEATH_LIST
		if (attackedplayer && attacker)
			attackedplayer->addDeath(attacker->getName(), attackedplayer->level, time(0));
#endif //JD_DEATH_LIST
		unsigned char stackpos = tile->getThingStackPos(attackedCreature);

		//Prepare body
		Item *corpseitem = Item::CreateItem(attackedCreature->getLookCorpse());
		corpseitem->pos = CreaturePos;
		tile->addThing(corpseitem);

		//Add eventual loot
		Container *lootcontainer = dynamic_cast<Container*>(corpseitem);
		if(lootcontainer) {
			attackedCreature->dropLoot(lootcontainer);
		}

		if(attackedplayer){
			attackedplayer->onThingDisappear(attackedplayer,stackpos);
			attackedplayer->die();        //handles exp/skills/maglevel loss
		}
		//remove creature
		game->removeCreature(attackedCreature);
		// Update attackedCreature pos because contains
		//  temple position for players
		attackedCreature->pos = CreaturePos;

		//add body
		game->sendAddThing(NULL,corpseitem->pos,corpseitem);

		if(attackedplayer){
			std::stringstream ss;
			ss << corpseitem->getDescription(false);

			ss << "You recognize " << attackedplayer->getName() << ". ";
			if(attacker){
				ss << (attackedplayer->getSex() == PLAYERSEX_FEMALE ? "She" : "He") << " was killed by ";

				Player *attackerplayer = dynamic_cast<Player*>(attacker);
				if(attackerplayer) {
					ss << attacker->getName();
				}
				else {
					std::string creaturename = attacker->getName();
					std::transform(creaturename.begin(), creaturename.end(), creaturename.begin(), (int(*)(int))tolower);
					ss << article(creaturename);
				}
			}

			//set body special description
			corpseitem->setSpecialDescription(ss.str());
			//send corpse to the dead player. It is not in spectator list
			// because was removed
			attackedplayer->onThingAppear(corpseitem);
		}
		game->startDecay(corpseitem);

		//Get all creatures that will gain xp from this kill..
		CreatureState* attackedCreatureState = NULL;
		std::vector<long> creaturelist;
		if(!(dynamic_cast<Player*>(attackedCreature) && game->getWorldType() != WORLD_TYPE_PVP_ENFORCED)){
			creaturelist = attackedCreature->getInflicatedDamageCreatureList();
			CreatureStateVec& creatureStateVec = creaturestates[tile];
			for(CreatureStateVec::iterator csIt = creatureStateVec.begin(); csIt != creatureStateVec.end(); ++csIt) {
				if(csIt->first == attackedCreature) {
					attackedCreatureState = &csIt->second;
					break;
				}
			}
		}

		if(attackedCreatureState) { //should never be NULL..
			//Add experience
			for(std::vector<long>::const_iterator iit = creaturelist.begin(); iit != creaturelist.end(); ++iit) {
				Creature* gainExpCreature = game->getCreatureByID(*iit);
				if(gainExpCreature) {
					exp_t gainedExperience = attackedCreature->getGainedExperience(gainExpCreature);
					if(gainedExperience <= 0)
						continue;

					Player *gainExpPlayer = dynamic_cast<Player*>(gainExpCreature);

					if(gainExpPlayer) {
						gainExpPlayer->addExp(gainedExperience);
					}

					//Need to add this creature and all that can see it to spectators, unless they already added
					SpectatorVec creaturelist;
					game->getSpectators(Range(gainExpCreature->pos, true), creaturelist);

					for(SpectatorVec::const_iterator cit = creaturelist.begin(); cit != creaturelist.end(); ++cit) {
						if(std::find(spectatorlist.begin(), spectatorlist.end(), *cit) == spectatorlist.end()) {
							spectatorlist.push_back(*cit);
						}
					}

					//Add creature to attackerlist
					attackedCreatureState->attackerlist.push_back(gainExpCreature);
				}
			}
		}

		Player *player = dynamic_cast<Player*>(attacker);
		if(player){
			player->sendStats();
		}

		if(attackedCreature && attackedCreature->getMaster() != NULL) {
			attackedCreature->getMaster()->removeSummon(attackedCreature);
		}
	}


	//Add blood?
#ifdef TJ_MONSTER_BLOOD
	if((drawBlood || attackedCreature->health <= 0) && damage > 0 && attackedCreature->bloodsplash != 255) {
		Item* splash = Item::CreateItem(dead? ITEM_POOL : ITEM_SPLASH, attackedCreature->bloodsplash);
		game->addThing(NULL, CreaturePos, splash);
		game->startDecay(splash);
	}
#else
	if((drawBlood || attackedCreature->health <= 0) && damage > 0) {
		Item* splash = Item::CreateItem(ITEM_SPLASH, FLUID_BLOOD);
		game->addThing(NULL, CreaturePos, splash);
		game->startDecay(splash);
	}
#endif //TJ_MONSTER_BLOOD
}


Game::Game()
{
	eventIdCount = 1000;
	this->game_state = GAME_STATE_NORMAL;
	this->map = NULL;
	this->worldType = WORLD_TYPE_PVP;
	OTSYS_THREAD_LOCKVARINIT(gameLock);
	OTSYS_THREAD_LOCKVARINIT(eventLock);
	OTSYS_THREAD_LOCKVARINIT(AutoID::autoIDLock);
#if defined __EXCEPTION_TRACER__
	OTSYS_THREAD_LOCKVARINIT(maploadlock);
#endif
	OTSYS_THREAD_SIGNALVARINIT(eventSignal);
	BufferedPlayers.clear();
	OTSYS_CREATE_THREAD(eventThread, this);

#ifdef __DEBUG_CRITICALSECTION__
	OTSYS_CREATE_THREAD(monitorThread, this);
#endif

	addEvent(makeTask(DECAY_INTERVAL, boost::bind(&Game::checkDecay,this,DECAY_INTERVAL)));

#ifdef CVS_DAY_CYCLE
	int daycycle = 3600;
	light_hour_delta = 1440*10/daycycle;
	light_hour = 0;
	lightlevel = LIGHT_LEVEL_NIGHT;
	light_state = LIGHT_STATE_NIGHT;
	addEvent(makeTask(10000, boost::bind(&Game::checkLight, this, 10000)));
#endif //CVS_DAY_CYCLE
}


Game::~Game()
{
	if(map) {
		delete map;
	}
}

void Game::setWorldType(enum_world_type type)
{
	this->worldType = type;
}

enum_game_state Game::getGameState()
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::getGameState()");
	return game_state;
}

int Game::loadMap(std::string filename, std::string filekind) {
	if(!map)
		map = new Map;

	max_players = atoi(g_config.getGlobalString("maxplayers").c_str());
	return map->loadMap(filename, filekind);
}



/*****************************************************************************/

#ifdef __DEBUG_CRITICALSECTION__

OTSYS_THREAD_RETURN Game::monitorThread(void *p)
{
  Game* _this = (Game*)p;

	while (true) {
		OTSYS_SLEEP(6000);

		int ret = OTSYS_THREAD_LOCKEX(_this->gameLock, 60 * 2 * 1000);
		if(ret != OTSYS_THREAD_TIMEOUT) {
			OTSYS_THREAD_UNLOCK(_this->gameLock, NULL);
			continue;
		}

		bool file = false;
		std::ostream *outdriver;
		std::cout << "Error: generating critical section file..." <<std::endl;
		std::ofstream output("deadlock.txt",std::ios_base::app);
		if(output.fail()){
			outdriver = &std::cout;
			file = false;
		}
		else{
			file = true;
			outdriver = &output;
		}

		time_t rawtime;
		time(&rawtime);
		*outdriver << "*****************************************************" << std::endl;
		*outdriver << "Error report - " << std::ctime(&rawtime) << std::endl;

		OTSYS_THREAD_LOCK_CLASS::LogList::iterator it;
		for(it = OTSYS_THREAD_LOCK_CLASS::loglist.begin(); it != OTSYS_THREAD_LOCK_CLASS::loglist.end(); ++it) {
			*outdriver << (it->lock ? "lock - " : "unlock - ") << it->str
				<< " threadid: " << it->threadid
				<< " time: " << it->time
				<< " ptr: " << it->mutexaddr
				<< std::endl;
		}

		*outdriver << "*****************************************************" << std::endl;
		if(file)
			((std::ofstream*)outdriver)->close();

		std::cout << "Error report generated. Killing server." <<std::endl;
		exit(1); //force exit
	}
}
#endif

OTSYS_THREAD_RETURN Game::eventThread(void *p)
{
#if defined __EXCEPTION_TRACER__
	ExceptionHandler eventExceptionHandler;
	eventExceptionHandler.InstallHandler();
#endif

  Game* _this = (Game*)p;

  // basically what we do is, look at the first scheduled item,
  // and then sleep until it's due (or if there is none, sleep until we get an event)
  // of course this means we need to get a notification if there are new events added
  while (true)
  {
#ifdef __DEBUG__EVENTSCHEDULER__
    std::cout << "schedulercycle start..." << std::endl;
#endif

    SchedulerTask* task = NULL;
		bool runtask = false;

    // check if there are events waiting...
    OTSYS_THREAD_LOCK(_this->eventLock, "eventThread()")

		int ret;
    if (_this->eventList.size() == 0) {
      // unlock mutex and wait for signal
      ret = OTSYS_THREAD_WAITSIGNAL(_this->eventSignal, _this->eventLock);
    } else {
      // unlock mutex and wait for signal or timeout
      ret = OTSYS_THREAD_WAITSIGNAL_TIMED(_this->eventSignal, _this->eventLock, _this->eventList.top()->getCycle());
    }
    // the mutex is locked again now...
    if (ret == OTSYS_THREAD_TIMEOUT) {
      // ok we had a timeout, so there has to be an event we have to execute...
#ifdef __DEBUG__EVENTSCHEDULER__
      std::cout << "event found at " << OTSYS_TIME() << " which is to be scheduled at: " << _this->eventList.top()->getCycle() << std::endl;
#endif
      task = _this->eventList.top();
      _this->eventList.pop();
		}

		if(task) {
			std::map<unsigned long, SchedulerTask*>::iterator it = _this->eventIdMap.find(task->getEventId());
			if(it != _this->eventIdMap.end()) {
				_this->eventIdMap.erase(it);
				runtask = true;
			}
		}

		OTSYS_THREAD_UNLOCK(_this->eventLock, "eventThread()");
    if (task) {
			if(runtask) {
				(*task)(_this);
			}
			delete task;
    }
  }
#if defined __EXCEPTION_TRACER__
	eventExceptionHandler.RemoveHandler();
#endif

}

unsigned long Game::addEvent(SchedulerTask* event) {
  bool do_signal = false;
  OTSYS_THREAD_LOCK(eventLock, "addEvent()")

	if(event->getEventId() == 0) {
		++eventIdCount;
		event->setEventId(eventIdCount);
	}

#ifdef __DEBUG__EVENTSCHEDULER__
		std::cout << "addEvent - " << event->getEventId() << std::endl;
#endif

	eventIdMap[event->getEventId()] = event;

	/*
	if (eventList.empty() ||  *event < *eventList.top())
    do_signal = true;
	*/

	bool isEmpty = eventList.empty();
	eventList.push(event);

	if(isEmpty || *event < *eventList.top())
		do_signal = true;

	/*
	if (eventList.empty() ||  *event < *eventList.top())
    do_signal = true;
	*/

  OTSYS_THREAD_UNLOCK(eventLock, "addEvent()")

	if (do_signal)
		OTSYS_THREAD_SIGNAL_SEND(eventSignal);

	return event->getEventId();
}

bool Game::stopEvent(unsigned long eventid) {
	if(eventid == 0)
		return false;

  OTSYS_THREAD_LOCK(eventLock, "stopEvent()")

	std::map<unsigned long, SchedulerTask*>::iterator it = eventIdMap.find(eventid);
	if(it != eventIdMap.end()) {

#ifdef __DEBUG__EVENTSCHEDULER__
		std::cout << "stopEvent - eventid: " << eventid << "/" << it->second->getEventId() << std::endl;
#endif

		//it->second->setEventId(0); //invalidate the event
		eventIdMap.erase(it);

	  OTSYS_THREAD_UNLOCK(eventLock, "stopEvent()")
		return true;
	}

  OTSYS_THREAD_UNLOCK(eventLock, "stopEvent()")
	return false;
}

/*****************************************************************************/

uint32_t Game::getPlayersOnline() {return (uint32_t)Player::listPlayer.list.size();};
uint32_t Game::getMonstersOnline() {return (uint32_t)Monster::listMonster.list.size();};
uint32_t Game::getNpcsOnline() {return (uint32_t)Npc::listNpc.list.size();};
uint32_t Game::getCreaturesOnline() {return (uint32_t)listCreature.list.size();};

Tile* Game::getTile(unsigned short _x, unsigned short _y, unsigned char _z)
{
	return map->getTile(_x, _y, _z);
}

Tile* Game::getTile(const Position& pos)
{
	return map->getTile(pos);
}

void Game::setTile(unsigned short _x, unsigned short _y, unsigned char _z, unsigned short groundId)
{
	map->setTile(_x, _y, _z, groundId);
}

Creature* Game::getCreatureByID(unsigned long id)
{
	if(id == 0)
		return NULL;

	AutoList<Creature>::listiterator it = listCreature.list.find(id);
	if(it != listCreature.list.end()) {
		return (*it).second;
	}

	return NULL; //just in case the player doesnt exist
}

Player* Game::getPlayerByID(unsigned long id)
{
	if(id == 0)
		return NULL;

	AutoList<Player>::listiterator it = Player::listPlayer.list.find(id);
	if(it != Player::listPlayer.list.end()) {
		return (*it).second;
	}

	return NULL; //just in case the player doesnt exist
}

Creature* Game::getCreatureByName(const std::string &s)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::getCreatureByName()");

	std::string txt1 = s;
	std::transform(txt1.begin(), txt1.end(), txt1.begin(), upchar);
	for(AutoList<Creature>::listiterator it = listCreature.list.begin(); it != listCreature.list.end(); ++it){
		std::string txt2 = (*it).second->getName();
		std::transform(txt2.begin(), txt2.end(), txt2.begin(), upchar);
		if(txt1 == txt2)
			return it->second;
	}

	return NULL; //just in case the creature doesnt exist
}

Player* Game::getPlayerByName(const std::string &s)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::getPlayerByName()");

	std::string txt1 = s;
	std::transform(txt1.begin(), txt1.end(), txt1.begin(), upchar);
	for(AutoList<Player>::listiterator it = Player::listPlayer.list.begin(); it != Player::listPlayer.list.end(); ++it){
		std::string txt2 = (*it).second->getName();
		std::transform(txt2.begin(), txt2.end(), txt2.begin(), upchar);
		if(txt1 == txt2)
			return it->second;
	}

	return NULL; //just in case the player doesnt exist
}

bool Game::placeCreature(Position &pos, Creature* c
#ifdef YUR_LOGIN_QUEUE
						 , int* placeInQueue
#endif //YUR_LOGIN_QUEUE
						 )
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::placeCreature()");
	bool success = false;
	Player *p = dynamic_cast<Player*>(c);

#ifdef YUR_LOGIN_QUEUE
	if (!p || c->access >= g_config.ACCESS_ENTER ||
 #ifdef YUR_PREMIUM_PROMOTION
		(p->isPremium() && !g_config.QUEUE_PREMMY) ||
 #endif //YUR_PREMIUM_PROMOTION
		loginQueue.login(p->accountNumber, getPlayersOnline(), max_players, placeInQueue)) {
#else //YUR_LOGIN_QUEUE
	if (!p || c->access >= g_config.ACCESS_ENTER || getPlayersOnline() < max_players) {
#endif //YUR_LOGIN_QUEUE

		success = map->placeCreature(pos, c);
		if(success) {
			c->useThing();

			c->setID();
			//std::cout << "place: " << c << " " << c->getID() << std::endl;
			listCreature.addList(c);
			c->addList();
			c->isRemoved = false;

			sendAddThing(NULL,c->pos,c);

			if(p)
			{
#ifdef __DEBUG_PLAYERS__
				std::cout << (uint32_t)getPlayersOnline() << " players online." << std::endl;
#endif
#ifdef YUR_GUILD_SYSTEM
				Guilds::ReloadGuildInfo(p);
#endif //YUR_GUILD_SYSTEM
#ifdef ELEM_VIP_LIST
				vipLogin(p);
#endif //ELEM_VIP_LIST
			}

			if(p){
				c->eventCheck = addEvent(makeTask(1000, std::bind2nd(std::mem_fun(&Game::checkCreature), c->getID())));
			}
			else{
				c->eventCheck = addEvent(makeTask(500, std::bind2nd(std::mem_fun(&Game::checkCreature), c->getID())));
			}

			//c->eventCheckAttacking = addEvent(makeTask(2000, std::bind2nd(std::mem_fun(&Game::checkCreatureAttacking), c->getID())));
		}
	}
	else {
		//we cant add the player, server is full
		success = false;
	}

  return success;
}

bool Game::removeCreature(Creature* c)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::removeCreature()");
	if(c->isRemoved == true)
		return false;
#ifdef __DEBUG__
	std::cout << "removing creature "<< std::endl;
#endif

	if(!removeThing(NULL,c->pos,c))
		return false;

	//std::cout << "remove: " << c << " " << c->getID() << std::endl;
	listCreature.removeList(c->getID());
	c->removeList();
	c->isRemoved = true;

	for(std::list<Creature*>::iterator cit = c->summons.begin(); cit != c->summons.end(); ++cit) {
		removeCreature(*cit);
	}

	stopEvent(c->eventCheck);
	stopEvent(c->eventCheckAttacking);

	Player* player = dynamic_cast<Player*>(c);
	if(player){
#ifdef TLM_SKULLS_PARTY
		if(player->party != 0)
			LeaveParty(player);
#endif //TLM_SKULLS_PARTY

		if(player->tradePartner != 0) {
			playerCloseTrade(player);
		}
		if(player->eventAutoWalk)
			stopEvent(player->eventAutoWalk);

		g_chat.removeUserFromAllChannels(player);
		if(!IOPlayer::instance()->savePlayer(player)){
			std::cout << "Error while saving player: " << player->getName() << std::endl;
		}
		#ifdef __DEBUG_PLAYERS__
		std::cout << (uint32_t)getPlayersOnline() << " players online." << std::endl;
		#endif

#ifdef ELEM_VIP_LIST
		vipLogout(c->getName());
#endif //ELEM_VIP_LIST
	}

	this->FreeThing(c);
	return true;
}

void Game::thingMove(Creature *creature, Thing *thing,
	unsigned short to_x, unsigned short to_y, unsigned char to_z, unsigned char count)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::thingMove() - 1");

	Tile *fromTile = map->getTile(thing->pos);

	if (fromTile)
	{
		int oldstackpos = fromTile->getThingStackPos(thing);
		thingMoveInternal(creature, thing->pos.x, thing->pos.y, thing->pos.z, oldstackpos, 0, to_x, to_y, to_z, count);
	}
}


void Game::thingMove(Creature *creature, unsigned short from_x, unsigned short from_y, unsigned char from_z,
	unsigned char stackPos, unsigned short itemid, unsigned short to_x, unsigned short to_y, unsigned char to_z, unsigned char count)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::thingMove() - 2");

	Tile *fromTile = getTile(from_x, from_y, from_z);
	if(!fromTile)
		return;

	Thing* thing = fromTile->getThingByStackPos(stackPos);
	if(!thing)
		return;

	Item* item = dynamic_cast<Item*>(thing);

	if(item && (item->getID() != itemid || item != fromTile->getTopDownItem()))
		return;

	thingMoveInternal(creature, from_x, from_y, from_z, stackPos, itemid, to_x, to_y, to_z, count);
}

//container/inventory to container/inventory
void Game::thingMove(Player *player,
	unsigned char from_cid, unsigned char from_slotid, unsigned short itemid, bool fromInventory,
	unsigned char to_cid, unsigned char to_slotid, bool toInventory,
	unsigned char count)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::thingMove() - 3");

	thingMoveInternal(player, from_cid, from_slotid, itemid, fromInventory,
		to_cid, to_slotid, toInventory, count);
}

//container/inventory to ground
void Game::thingMove(Player *player,
	unsigned char from_cid, unsigned char from_slotid, unsigned short itemid, bool fromInventory,
	const Position& toPos, unsigned char count)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::thingMove() - 4");

	thingMoveInternal(player, from_cid, from_slotid, itemid, fromInventory, toPos, count);
}

//ground to container/inventory
void Game::thingMove(Player *player,
	const Position& fromPos, unsigned char stackPos, unsigned short itemid,
	unsigned char to_cid, unsigned char to_slotid, bool toInventory,
	unsigned char count)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::thingMove() - 5");
	thingMoveInternal(player, fromPos, stackPos, itemid, to_cid, to_slotid, toInventory, count);
}

/*ground -> ground*/
bool Game::onPrepareMoveThing(Creature* player, const Thing* thing,
	const Position& fromPos, const Position& toPos, int count)
{
	if ((player->access < g_config.ACCESS_REMOTE || dynamic_cast<const Player*>(thing)) &&
		((abs(player->pos.x - fromPos.x) > 1) || (abs(player->pos.y - fromPos.y) > 1) || (player->pos.z != fromPos.z)))
	{
		player->sendCancel("Too far away...");
		return false;
	}
	else if ((player->access < g_config.ACCESS_REMOTE || dynamic_cast<const Player*>(thing)) &&
		((abs(fromPos.x - toPos.x) > thing->throwRange) || (abs(fromPos.y - toPos.y) > thing->throwRange)
		|| (fromPos.z != toPos.z) /*TODO: Make it possible to throw items to different floors*/ ))
	{
		player->sendCancel("Destination is out of reach.");
		return false;
	}
	else {
		const Item* item = dynamic_cast<const Item*>(thing);
		if(item) {
			int blockstate = 0;
			if(item->isBlocking())
				blockstate |= BLOCK_SOLID;

			if(item->isPickupable() || !item->isNotMoveable())
				blockstate |= BLOCK_PICKUPABLE;

			if(blockstate != 0) {
				switch(map->canThrowObjectTo(fromPos, toPos, blockstate)) {
					case RET_NOERROR:
						return true;
						break;

					case RET_CANNOTTHROW:
						player->sendCancel("You cannot throw there.");
						return false;
						break;

					case RET_CREATUREBLOCK:
					case RET_NOTENOUGHROOM:
						player->sendCancel("There is not enough room.");
						return false;
						break;

					default:
						player->sendCancel("Sorry not possible.");
						return false;
						break;
				}
			}
		}
	}

	return true;
}

/*ground -> ground*/
bool Game::onPrepareMoveThing(Creature* creature, const Thing* thing,
	const Tile* fromTile, const Tile *toTile, int count)
{
	const Player* player = dynamic_cast<const Player*>(creature);

	const Item *item = dynamic_cast<const Item*>(thing);
	const Creature* movingCreature = dynamic_cast<const Creature*>(thing);
	const Player* movingPlayer = dynamic_cast<const Player*>(thing);

	if(item && !item->canMovedTo(toTile)) {
	 	creature->sendCancel("Sorry, not possible.");
		return false;
	}
	else if(movingCreature && !movingCreature->canMovedTo(toTile)) {
    if(player) {
		  player->sendTextMessage(MSG_SMALLINFO, "Sorry, not possible.");
		  player->sendCancelWalk();
    }

		return false;
	}
	else if(!movingPlayer && toTile && toTile->floorChange()) {
		creature->sendCancel("Sorry, not possible.");
		return false;
	}
  else if(movingCreature && toTile && !toTile->ground) {
    if(player) {
      player->sendTextMessage(MSG_SMALLINFO, "Sorry, not possible.");
      player->sendCancelWalk();
    }

		return false;
	}

	if (fromTile && fromTile->splash == thing && fromTile->splash->isNotMoveable()) {
			creature->sendCancel("You cannot move this object.");
#ifdef __DEBUG__
		cout << creature->getName() << " is trying to move a splash item!" << std::endl;
#endif
		return false;
	}
	else if (item && item->isNotMoveable()) {
			creature->sendCancel("You cannot move this object.");
#ifdef __DEBUG__
		cout << creature->getName() << " is trying to move an unmoveable item!" << std::endl;
#endif
		return false;
	}

#ifdef TLM_HOUSE_SYSTEM
	if (item && toTile && toTile->isHouse())
	{
		const Container* container = dynamic_cast<const Container*>(item);
		int moving = container? container->getItemHoldingCount() : 1;

		if (moving + toTile->getItemHoldingCount() > g_config.MAX_HOUSE_TILE_ITEMS)
		{
			creature->sendCancel("You cannot put more items on a house tile.");
			return false;
		}
	}
#endif //TLM_HOUSE_SYSTEM

	return true; //return thing->canMovedTo(toTile);
}

/*inventory -> container*/
bool Game::onPrepareMoveThing(Player* player, const Item* fromItem, slots_t fromSlot,
	const Container* toContainer, const Item* toItem, int count)
{
	if(!fromItem->isPickupable()) {
		player->sendCancel("Sorry, not possible.");
		return false;
	}
	else {
		const Container *itemContainer = dynamic_cast<const Container*>(fromItem);
		if(itemContainer) {
			if(itemContainer->isHoldingItem(toContainer) || (toContainer == itemContainer)) {
				player->sendCancel("This is impossible.");
				return false;
			}
		}

		if((!fromItem->isStackable() || !toItem || fromItem->getID() != toItem->getID() || toItem->getItemCountOrSubtype() >= 100) && toContainer->size() + 1 > toContainer->capacity()) {
			player->sendCancel("Sorry, not enough room.");
			return false;
		}

		Container const *topContainer = toContainer->getTopParent();
		int itemsToadd;
		if(!topContainer)
			topContainer = toContainer;

		Container const *fromContainer = dynamic_cast<const Container*>(fromItem);
		if(fromContainer)
			itemsToadd = fromContainer->getItemHoldingCount() + 1;
		else
			itemsToadd = 1;

		if(topContainer->depot != 0 && player->max_depot_items != 0 && topContainer->getItemHoldingCount() + itemsToadd >= player->max_depot_items){
			player->sendCancel("You can not put more items in this depot.");
			return false;
		}
	}

	return true;
}

/*container -> container*/
/*ground -> container*/
bool Game::onPrepareMoveThing(Player* player,
	const Position& fromPos, const Container* fromContainer, const Item* fromItem,
	const Position& toPos, const Container* toContainer, const Item* toItem, int count)
{
	if (player->access < g_config.ACCESS_REMOTE &&
		((abs(player->pos.x - fromPos.x) > 1) || (abs(player->pos.y - fromPos.y) > 1) || (player->pos.z != fromPos.z)))
	{
		player->sendCancel("Too far away...");
		return false;
	}
	else if (player->access < g_config.ACCESS_REMOTE &&
		((abs(fromPos.x - toPos.x) > fromItem->throwRange) || (abs(fromPos.y - toPos.y) > fromItem->throwRange)
		|| (fromPos.z != toPos.z)))
	{
		player->sendCancel("Destination is out of reach.");
		return false;
	}

	if(!fromItem->isPickupable()) {
		player->sendCancel("You cannot move this object.");
		return false;
	}
	else {
		if((!fromItem->isStackable() || !toItem || fromItem->getID() != toItem->getID() || toItem->getItemCountOrSubtype() >= 100) && toContainer->size() + 1 > toContainer->capacity()) {
			player->sendCancel("Sorry, not enough room.");
			return false;
		}

		const Container *itemContainer = dynamic_cast<const Container*>(fromItem);
		if(itemContainer) {
			if(itemContainer->isHoldingItem(toContainer) || (toContainer == itemContainer) || (fromContainer && fromContainer == itemContainer)) {
				player->sendCancel("This is impossible.");
				return false;
			}
		}

		double itemWeight = (fromItem->isStackable() ? Item::items[fromItem->getID()].weight * std::max(1, count) : fromItem->getWeight());
		if((!fromContainer || !player->isHoldingContainer(fromContainer)) && player->isHoldingContainer(toContainer)) {
			if(player->access < g_config.ACCESS_PROTECT && player->getFreeCapacity() < itemWeight) {
				player->sendCancel("This object is too heavy.");
				return false;
			}
		}

		Container const *topContainer = toContainer->getTopParent();
		int itemsToadd;
		if(!topContainer)
			topContainer = toContainer;

		Container const *fromContainer = dynamic_cast<const Container*>(fromItem);
		if(fromContainer)
			itemsToadd = fromContainer->getItemHoldingCount() + 1;
		else
			itemsToadd = 1;

		if(topContainer->depot != 0 && player->max_depot_items != 0 && topContainer->getItemHoldingCount() >= player->max_depot_items){
			player->sendCancel("You can not put more items in this depot.");
			return false;
		}
	}

	return true;
}

/*ground -> ground*/
bool Game::onPrepareMoveCreature(Creature *creature, const Creature* creatureMoving,
	const Tile *fromTile, const Tile *toTile)
{
	const Player* playerMoving = dynamic_cast<const Player*>(creatureMoving);
	Player* player = dynamic_cast<Player*>(creature);

	if (creature->access < g_config.ACCESS_PROTECT && creature != creatureMoving && !creatureMoving->isPushable()) {
		creature->sendCancel("Sorry, not possible.");
    return false;
  }
	if(!toTile){
		if(creature == creatureMoving && player) {
			player->sendTextMessage(MSG_SMALLINFO, "Sorry, not possible.");
			player->sendCancelWalk();
		}

		return false;
	}
  else if (playerMoving && toTile->isPz() && playerMoving->pzLocked) {
		if (creature == creatureMoving && creature->pzLocked) {

			if(player) {
				player->sendTextMessage(MSG_SMALLINFO, "You can not enter a protection zone after attacking another player.");
				player->sendCancelWalk();
			}

			//player->sendCancelWalk("You can not enter a protection zone after attacking another player.");
			return false;
		}
		else if (playerMoving->pzLocked) {
			creature->sendCancel("Sorry, not possible.");
			return false;
		}
  }
  else if (playerMoving && fromTile->isPz() && !toTile->isPz() && creature != creatureMoving) {
		creature->sendCancel("Sorry, not possible.");
		return false;
  }
	else if(creature != creatureMoving && toTile->floorChange()){
		creature->sendCancel("Sorry, not possible.");
		return false;
	}
  else if(creature != creatureMoving && toTile->getTeleportItem()){
		creature->sendCancel("Sorry, not possible.");
		return false;
  }

	return true;
}

/*ground -> inventory*/
bool Game::onPrepareMoveThing(Player *player, const Position& fromPos, const Item *item,
	slots_t toSlot, int count)
{
	if (player->access < g_config.ACCESS_REMOTE &&
		((abs(player->pos.x - fromPos.x) > 1) || (abs(player->pos.y - fromPos.y) > 1) || (player->pos.z != fromPos.z)))
	{
		player->sendCancel("Too far away...");
		return false;
	}
	else if(!item->isPickupable()) {
		player->sendCancel("You cannot move this object.");
		return false;
	}

	double itemWeight = (item->isStackable() ? Item::items[item->getID()].weight * std::max(1, count) : item->getWeight());
	if(player->access < g_config.ACCESS_PROTECT && player->getFreeCapacity() < itemWeight) {
		player->sendCancel("This object is too heavy.");
		return false;
	}

	return true;
}

/*inventory -> inventory*/
bool Game::onPrepareMoveThing(Player *player, slots_t fromSlot, const Item *fromItem,
	slots_t toSlot, const Item *toItem, int count)
{
	if(toItem && (!toItem->isStackable() || toItem->getID() != fromItem->getID())) {
		player->sendCancel("Sorry, not enough room.");
		return false;
	}

	return true;
}

/*container -> inventory*/
bool Game::onPrepareMoveThing(Player *player, const Container *fromContainer, const Item *fromItem,
	slots_t toSlot, const Item *toItem, int count)
{
	if(toItem && (!toItem->isStackable() || toItem->getID() != fromItem->getID())) {
		player->sendCancel("Sorry, not enough room.");
		return false;
	}

	double itemWeight = (fromItem->isStackable() ? Item::items[fromItem->getID()].weight * std::max(1, count) : fromItem->getWeight());
	if(player->access < g_config.ACCESS_PROTECT && !player->isHoldingContainer(fromContainer) &&
		player->getFreeCapacity() < itemWeight) {
		player->sendCancel("This object is too heavy.");
		return false;
	}

	return true;
}

/*->inventory*/
bool Game::onPrepareMoveThing(Player *player, const Item *item,
	slots_t toSlot, int count)
{
	switch(toSlot)
	{
	case SLOT_HEAD:
		if(item->getSlotPosition() & SLOTP_HEAD)
			return true;
		break;
	case SLOT_NECKLACE:
		if(item->getSlotPosition() & SLOTP_NECKLACE)
			return true;
		break;
	case SLOT_BACKPACK:
		if(item->getSlotPosition() & SLOTP_BACKPACK)
			return true;
		break;
	case SLOT_ARMOR:
		if(item->getSlotPosition() & SLOTP_ARMOR)
			return true;
		break;
	case SLOT_RIGHT:
		if(item->getSlotPosition() & SLOTP_RIGHT){
			if(item->getSlotPosition() & SLOTP_TWO_HAND){
				if(player->items[SLOT_LEFT] != NULL){
					player->sendCancel("First remove the two-handed item.");
					return false;
				}
				return true	;
			}
			else{
				if(player->items[SLOT_LEFT]){
					if(player->items[SLOT_LEFT]->getSlotPosition() & SLOTP_TWO_HAND){
						player->sendCancel("First remove the two-handed item.");
						return false;
					}
					return true;
				}
				return true;
			}
		}
		break;
	case SLOT_LEFT:
		if(item->getSlotPosition() & SLOTP_LEFT){
			if(item->getSlotPosition() & SLOTP_TWO_HAND){
				if(player->items[SLOT_RIGHT] != NULL){
					player->sendCancel("First remove the two-handed item.");
					return false;
				}
				return true	;
			}
			else{
				if(player->items[SLOT_RIGHT]){
					if(player->items[SLOT_RIGHT]->getSlotPosition() & SLOTP_TWO_HAND){
						player->sendCancel("First remove the two-handed item.");
						return false;
					}
					return true;
				}
				return true;
			}
		}
		break;
	case SLOT_LEGS:
		if(item->getSlotPosition() & SLOTP_LEGS)
			return true;
		break;
	case SLOT_FEET:
		if(item->getSlotPosition() & SLOTP_FEET)
			return true;
		break;
	case SLOT_RING:
		if(item->getSlotPosition() & SLOTP_RING)
			return true;
		break;
	case SLOT_AMMO:
		if(item->getSlotPosition() & SLOTP_AMMO)
			return true;
		break;
	}

	player->sendCancel("You cannot put that object in that place.");
	return false;
}

//container/inventory to container/inventory
void Game::thingMoveInternal(Player *player,
	unsigned char from_cid, unsigned char from_slotid, unsigned short itemid,
	bool fromInventory,unsigned char to_cid, unsigned char to_slotid, bool toInventory,
	unsigned char count)
{
	Container *fromContainer = NULL;
	Container *toContainer = NULL;
	Item *fromItem = NULL;
	Item *toItem = NULL;

	if(fromInventory) {
		fromItem = player->getItem(from_cid);
		fromContainer = dynamic_cast<Container *>(fromItem);
	}
	else {
		fromContainer = player->getContainer(from_cid);

		if(fromContainer) {
			if(from_slotid >= fromContainer->size())
				return;

			fromItem = fromContainer->getItem(from_slotid);
		}
	}

	if(toInventory) {
		toItem = player->getItem(to_cid);
		toContainer = dynamic_cast<Container *>(toItem);
	}
	else {
		toContainer = player->getContainer(to_cid);

		if(toContainer) {
			if(to_slotid >= toContainer->capacity())
				return;

			toItem = toContainer->getItem(to_slotid);
			Container *toSlotContainer = dynamic_cast<Container*>(toItem);
			if(toSlotContainer) {
				toContainer = toSlotContainer;
				toItem = NULL;
			}
		}
	}

	if(!fromItem || (toItem == fromItem) || (fromItem->isStackable() && count > fromItem->getItemCountOrSubtype()) || fromItem->getID() != itemid)
		return;

	//Container to container
	if(!fromInventory && fromContainer && toContainer) {
		Position fromPos = (fromContainer->pos.x == 0xFFFF ? player->pos : fromContainer->pos);
		Position toPos = (toContainer->pos.x == 0xFFFF ? player->pos : toContainer->pos);

		if(onPrepareMoveThing(player, fromPos, fromContainer, fromItem, toPos, toContainer, toItem, count)) {

			autoCloseTrade(fromItem, true);
			int oldFromCount = fromItem->getItemCountOrSubtype();
			int oldToCount = 0;

			//move around an item in a container
			if(fromItem->isStackable()) {
				if(toItem && toItem != fromItem && toItem->getID() == fromItem->getID())
				{
					oldToCount = toItem->getItemCountOrSubtype();
					int newToCount = std::min(100, oldToCount + count);
					toItem->setItemCountOrSubtype(newToCount);

					if(oldToCount != newToCount) {
						autoCloseTrade(toItem);
					}

					int subcount = oldFromCount - count;
					fromItem->setItemCountOrSubtype(subcount);

					int surplusCount = oldToCount + count  - 100;
					if(surplusCount > 0) {
						Item *surplusItem = Item::CreateItem(fromItem->getID(), surplusCount);
						if(onPrepareMoveThing(player, fromPos, fromContainer, surplusItem, toPos, toContainer, NULL, count)) {
							autoCloseTrade(toContainer);
							toContainer->addItem(surplusItem);
						}
						else {
							delete surplusItem;
							count -= surplusCount; //re-define the actual amount we move.
							fromItem->setItemCountOrSubtype(fromItem->getItemCountOrSubtype() + surplusCount);
						}
					}
				}
				else if(count < oldFromCount) {
					int subcount = oldFromCount - count;
					fromItem->setItemCountOrSubtype(subcount);

					Item* moveItem = Item::CreateItem(fromItem->getID(), count);
					toContainer->addItem(moveItem);
					autoCloseTrade(toContainer);
				}
				else {
					if(fromContainer == toContainer) {
						fromContainer->moveItem(from_slotid, 0);
					}
					else if(fromContainer->removeItem(fromItem)) {
						autoCloseTrade(toContainer);
						toContainer->addItem(fromItem);
					}
				}

				if(fromItem->getItemCountOrSubtype() == 0) {
					fromContainer->removeItem(fromItem);
					this->FreeThing(fromItem);
				}
			}
			else {
				if(fromContainer == toContainer) {
					fromContainer->moveItem(from_slotid, 0);
				}
				else if(fromContainer->removeItem(fromItem)) {
					autoCloseTrade(toContainer);
					toContainer->addItem(fromItem);
				}
			}

			if(player->isHoldingContainer(fromContainer) != player->isHoldingContainer(toContainer)) {
				player->updateInventoryWeigth();
			}

			SpectatorVec list;
			SpectatorVec::iterator it;

			if(fromPos == toPos) {
				getSpectators(Range(fromPos, false), list);
			}
			else {
				getSpectators(Range(fromPos, toPos), list);
			}

			if(!list.empty()) {
				//players
				for(it = list.begin(); it != list.end(); ++it) {
					if(dynamic_cast<Player*>(*it)) {
						(*it)->onThingMove(player, fromContainer, from_slotid, fromItem, oldFromCount, toContainer, to_slotid, toItem, oldToCount, count);
					}
				}

				//none-players
				for(it = list.begin(); it != list.end(); ++it) {
					if(!dynamic_cast<Player*>(*it)) {
						(*it)->onThingMove(player, fromContainer, from_slotid, fromItem, oldFromCount, toContainer, to_slotid, toItem, oldToCount, count);
					}
				}
			}
			else
				player->onThingMove(player, fromContainer, from_slotid, fromItem, oldFromCount, toContainer, to_slotid, toItem, oldToCount, count);
		}
	}
	else {
		//inventory to inventory
		if(fromInventory && toInventory && !toContainer) {
			Position fromPos = player->pos;
			Position toPos = player->pos;
			if(onPrepareMoveThing(player, fromItem, (slots_t)to_cid, count) && onPrepareMoveThing(player, (slots_t)from_cid, fromItem, (slots_t)to_cid, toItem, count)) {

				autoCloseTrade(fromItem, true);
				int oldFromCount = fromItem->getItemCountOrSubtype();
				int oldToCount = 0;

				if(fromItem->isStackable()) {
					if(toItem && toItem != fromItem && toItem->getID() == fromItem->getID())
					{
						oldToCount = toItem->getItemCountOrSubtype();
						int newToCount = std::min(100, oldToCount + count);
						toItem->setItemCountOrSubtype(newToCount);

						if(oldToCount != newToCount) {
							autoCloseTrade(toItem);
						}

						int subcount = oldFromCount - count;
						fromItem->setItemCountOrSubtype(subcount);

						int surplusCount = oldToCount + count  - 100;
						if(surplusCount > 0) {
							fromItem->setItemCountOrSubtype(fromItem->getItemCountOrSubtype() + surplusCount);
							player->sendCancel("Sorry not enough room.");
						}

						if(fromItem->getItemCountOrSubtype() == 0) {
							player->removeItemInventory(from_cid, true);
							this->FreeThing(fromItem);
						}
					}
					else if(count < oldFromCount) {
						int subcount = oldFromCount - count;
						fromItem->setItemCountOrSubtype(subcount);

						autoCloseTrade(toItem);
						player->removeItemInventory(to_cid, true);
						player->addItemInventory(Item::CreateItem(fromItem->getID(), count), to_cid, true);

						if(fromItem->getItemCountOrSubtype() == 0) {
							player->removeItemInventory(from_cid, true);
							this->FreeThing(fromItem);
						}
					}
					else {
						if(player->removeItemInventory(from_cid, true)) {
							player->removeItemInventory(to_cid, true);
							player->addItemInventory(fromItem, to_cid, true);
						}
					}
				}
				else if(player->removeItemInventory(from_cid, true)) {
					player->removeItemInventory(to_cid, true);
					player->addItemInventory(fromItem, to_cid, true);
				}

				player->onThingMove(player, (slots_t)from_cid, fromItem, oldFromCount, (slots_t)to_cid, toItem, oldToCount, count);
			}
		}
		//container to inventory
		else if(!fromInventory && fromContainer && toInventory) {
			if(onPrepareMoveThing(player, fromItem, (slots_t)to_cid, count) && onPrepareMoveThing(player, fromContainer, fromItem, (slots_t)to_cid, toItem, count)) {
				autoCloseTrade(fromItem, true);
				int oldFromCount = fromItem->getItemCountOrSubtype();
				int oldToCount = 0;

				if(fromItem->isStackable()) {
					if(toItem && toItem != fromItem && toItem->getID() == fromItem->getID())
					{
						oldToCount = toItem->getItemCountOrSubtype();
						int newToCount = std::min(100, oldToCount + count);
						toItem->setItemCountOrSubtype(newToCount);

						if(oldToCount != newToCount) {
							autoCloseTrade(toItem);
						}

						int subcount = oldFromCount - count;
						fromItem->setItemCountOrSubtype(subcount);

						int surplusCount = oldToCount + count  - 100;
						if(surplusCount > 0) {
							fromItem->setItemCountOrSubtype(fromItem->getItemCountOrSubtype() + surplusCount);
							player->sendCancel("Sorry not enough room.");
						}

						if(fromItem->getItemCountOrSubtype() == 0) {
							fromContainer->removeItem(fromItem);
							this->FreeThing(fromItem);
						}
					}
					else if(count < oldFromCount) {
						int subcount = oldFromCount - count;
						fromItem->setItemCountOrSubtype(subcount);

						player->removeItemInventory(to_cid, true);
						player->addItemInventory(Item::CreateItem(fromItem->getID(), count), to_cid, true);

						if(toItem) {
							fromContainer->addItem(toItem);
						}

						if(fromItem->getItemCountOrSubtype() == 0) {
							fromContainer->removeItem(fromItem);
							this->FreeThing(fromItem);
						}
					}
					else {
						if(fromContainer->removeItem(fromItem)) {
							player->removeItemInventory(to_cid, true);
							player->addItemInventory(fromItem, to_cid, true);

							if(toItem) {
								fromContainer->addItem(toItem);
							}
						}
					}
				}
				else if(fromContainer->removeItem(fromItem)) {
					player->removeItemInventory(to_cid, true);
					player->addItemInventory(fromItem, to_cid, true);

					if(toItem) {
						fromContainer->addItem(toItem);
					}
				}

				if(!player->isHoldingContainer(fromContainer)) {
					player->updateInventoryWeigth();
				}

				//if(fromContainer->pos.x != 0xFFFF) {
				if(fromContainer->getTopParent()->pos.x != 0xFFFF) {
					SpectatorVec list;
					SpectatorVec::iterator it;

					getSpectators(Range(fromContainer->getTopParent()->pos, false), list);

					//players
					for(it = list.begin(); it != list.end(); ++it) {
						if(dynamic_cast<Player*>(*it)) {
							(*it)->onThingMove(player, fromContainer, from_slotid, fromItem, oldFromCount, (slots_t)to_cid, toItem, oldToCount, count);
						}
					}

					//none-players
					for(it = list.begin(); it != list.end(); ++it) {
						if(!dynamic_cast<Player*>(*it)) {
							(*it)->onThingMove(player, fromContainer, from_slotid, fromItem, oldFromCount, (slots_t)to_cid, toItem, oldToCount, count);
						}
					}
				}
				else
					player->onThingMove(player, fromContainer, from_slotid, fromItem, oldFromCount, (slots_t)to_cid, toItem, oldToCount, count);
			}
		}
		//inventory to container
		else if(fromInventory && toContainer) {
			Position fromPos = player->pos;
			Position toPos = (toContainer->pos.x == 0xFFFF ? player->pos : toContainer->pos);

			int oldFromCount = fromItem->getItemCountOrSubtype();
			int oldToCount = 0;

			if(onPrepareMoveThing(player, fromItem, (slots_t)from_cid, toContainer, toItem, count)) {
				autoCloseTrade(fromItem, true);
				if(fromItem->isStackable()) {
					if(toItem && toItem != fromItem && toItem->getID() == fromItem->getID())
					{
						oldToCount = toItem->getItemCountOrSubtype();
						int newToCount = std::min(100, oldToCount + count);
						toItem->setItemCountOrSubtype(newToCount);

						if(oldToCount != newToCount) {
							autoCloseTrade(toItem);
						}

						int subcount = oldFromCount - count;
						fromItem->setItemCountOrSubtype(subcount);

						int surplusCount = oldToCount + count  - 100;
						if(surplusCount > 0) {
							Item *surplusItem = Item::CreateItem(fromItem->getID(), surplusCount);

							if(onPrepareMoveThing(player, fromPos, NULL, surplusItem, toPos, toContainer, NULL, count)) {
								autoCloseTrade(toContainer);
								toContainer->addItem(surplusItem);
							}
							else {
								delete surplusItem;
								count -= surplusCount; //re-define the actual amount we move.
								fromItem->setItemCountOrSubtype(fromItem->getItemCountOrSubtype() + surplusCount);
							}
						}
					}
					else if(count < oldFromCount) {
						int subcount = oldFromCount - count;
						fromItem->setItemCountOrSubtype(subcount);
						autoCloseTrade(toContainer);
						toContainer->addItem(Item::CreateItem(fromItem->getID(), count));
					}
					else {
						if(player->removeItemInventory((slots_t)from_cid, true)) {
							autoCloseTrade(toContainer);
							toContainer->addItem(fromItem);

							if(player->isHoldingContainer(toContainer)) {
								player->updateInventoryWeigth();
							}
						}
					}

					if(fromItem->getItemCountOrSubtype() == 0) {
						player->removeItemInventory(from_cid, true);
						this->FreeThing(fromItem);
					}
				}
				else if(player->removeItemInventory(from_cid, true)) {
					autoCloseTrade(toContainer);
					toContainer->addItem(fromItem);

					if(player->isHoldingContainer(toContainer)) {
						player->updateInventoryWeigth();
					}
				}

				if(!player->isHoldingContainer(toContainer)) {
					player->updateInventoryWeigth();
				}

				if(toContainer->getTopParent()->pos.x != 0xFFFF) {
					SpectatorVec list;
					SpectatorVec::iterator it;

					getSpectators(Range(toContainer->getTopParent()->pos, false), list);

					//players
					for(it = list.begin(); it != list.end(); ++it) {
						if(dynamic_cast<Player*>(*it)) {
							(*it)->onThingMove(player, (slots_t)from_cid, fromItem, oldFromCount, toContainer, to_slotid, toItem, oldToCount, count);
						}
					}

					//none-players
					for(it = list.begin(); it != list.end(); ++it) {
						if(!dynamic_cast<Player*>(*it)) {
							(*it)->onThingMove(player, (slots_t)from_cid, fromItem, oldFromCount, toContainer, to_slotid, toItem, oldToCount, count);
						}
					}
				}
				else
					player->onThingMove(player, (slots_t)from_cid, fromItem, oldFromCount, toContainer, to_slotid, toItem, oldToCount, count);
			}
		}
	}
}

//container/inventory to ground
void Game::thingMoveInternal(Player *player,
	unsigned char from_cid, unsigned char from_slotid, unsigned short itemid, bool fromInventory,
	const Position& toPos, unsigned char count)
{
	Container *fromContainer = NULL;
	Tile *toTile = map->getTile(toPos);
	if(!toTile)
		return;

	/*container to ground*/
	if(!fromInventory) {
		fromContainer = player->getContainer(from_cid);
		if(!fromContainer)
			return;

		Position fromPos = (fromContainer->pos.x == 0xFFFF ? player->pos : fromContainer->pos);
		Item *fromItem = dynamic_cast<Item*>(fromContainer->getItem(from_slotid));
		Item* toItem = dynamic_cast<Item*>(toTile->getTopDownItem());

		if(!fromItem || (toItem == fromItem) || (fromItem->isStackable() && count > fromItem->getItemCountOrSubtype()) || fromItem->getID() != itemid)
			return;

		if(onPrepareMoveThing(player, fromItem, fromPos, toPos, count) && onPrepareMoveThing(player, fromItem, NULL, toTile, count)) {
			autoCloseTrade(fromItem, true);
			int oldFromCount = fromItem->getItemCountOrSubtype();
			int oldToCount = 0;

			//Do action...
#ifdef TP_TRASH_BINS
			if (toItem && toItem->isDeleter())
			{
				fromContainer->removeItem(fromItem);
				FreeThing(fromItem);
			}
			else if(fromItem->isStackable()) {
#else
			if(fromItem->isStackable()) {
#endif //TP_TRASH_BINS
				if(toItem && toItem->getID() == fromItem->getID())
				{
					oldToCount = toItem->getItemCountOrSubtype();
					int newToCount = std::min(100, oldToCount + count);
					toItem->setItemCountOrSubtype(newToCount);

					if(oldToCount != newToCount) {
						autoCloseTrade(toItem);
					}

					int subcount = oldFromCount - count;
					fromItem->setItemCountOrSubtype(subcount);

					int surplusCount = oldToCount + count  - 100;
					if(surplusCount > 0) {
						Item *surplusItem = Item::CreateItem(fromItem->getID(), surplusCount);
						surplusItem->pos = toPos;

						toTile->addThing(surplusItem);
					}

					if(fromItem->getItemCountOrSubtype() == 0) {
						fromContainer->removeItem(fromItem);
						this->FreeThing(fromItem);
					}
				}
				else if(count < oldFromCount) {
					int subcount = oldFromCount - count;
					fromItem->setItemCountOrSubtype(subcount);

					Item *moveItem = Item::CreateItem(fromItem->getID(), count);
					moveItem->pos = toPos;
					toTile->addThing(moveItem);

					if(fromItem->getItemCountOrSubtype() == 0) {
						fromContainer->removeItem(fromItem);
						this->FreeThing(fromItem);
					}
				}
				else if(fromContainer->removeItem(fromItem)) {
					fromItem->pos = toPos;
					toTile->addThing(fromItem);
				}
			}
			else if(fromContainer->removeItem(fromItem)) {
				fromItem->pos = toPos;
				toTile->addThing(fromItem);
			}

			if(player->isHoldingContainer(fromContainer)) {
				player->updateInventoryWeigth();
			}

			SpectatorVec list;
			SpectatorVec::iterator it;

			getSpectators(Range(fromPos, false), list);

			SpectatorVec tolist;
			getSpectators(Range(toPos, true), tolist);

			for(it = tolist.begin(); it != tolist.end(); ++it) {
				if(std::find(list.begin(), list.end(), *it) == list.end()) {
					list.push_back(*it);
				}
			}

			//players
			for(it = list.begin(); it != list.end(); ++it) {
				if(dynamic_cast<Player*>(*it)) {
					(*it)->onThingMove(player, fromContainer, from_slotid, fromItem, oldFromCount, toPos, toItem, oldToCount, count);
				}
			}

			//none-players
			for(it = list.begin(); it != list.end(); ++it) {
				if(!dynamic_cast<Player*>(*it)) {
					(*it)->onThingMove(player, fromContainer, from_slotid, fromItem, oldFromCount, toPos, toItem, oldToCount, count);
				}
			}
		}
	}
	else /*inventory to ground*/{
		Item *fromItem = player->getItem(from_cid);
		if(!fromItem || (fromItem->isStackable() && count > fromItem->getItemCountOrSubtype()) || fromItem->getID() != itemid)
			return;

		if(onPrepareMoveThing(player, fromItem, player->pos, toPos, count) && onPrepareMoveThing(player, fromItem, NULL, toTile, count)) {
			autoCloseTrade(fromItem, true);
			Item* toItem = dynamic_cast<Item*>(toTile->getTopDownItem());
			int oldFromCount = fromItem->getItemCountOrSubtype();
			int oldToCount = 0;

			//Do action...
#ifdef TP_TRASH_BINS
			if(toItem && toItem->isDeleter())
			{
				player->removeItemInventory(from_cid, true);
				FreeThing(fromItem);
			}
			else if(fromItem->isStackable()) {
#else
			if(fromItem->isStackable()) {
#endif //TP_TRASH_BINS
				if(toItem && toItem->getID() == fromItem->getID())
				{
					oldToCount = toItem->getItemCountOrSubtype();
					int newToCount = std::min(100, oldToCount + count);
					toItem->setItemCountOrSubtype(newToCount);

					if(oldToCount != newToCount) {
						autoCloseTrade(toItem);
					}

					int subcount = oldFromCount - count;
					fromItem->setItemCountOrSubtype(subcount);

					int surplusCount = oldToCount + count  - 100;
					if(surplusCount > 0) {
						Item *surplusItem = Item::CreateItem(fromItem->getID(), surplusCount);
						surplusItem->pos = toPos;

						toTile->addThing(surplusItem);
					}

					if(fromItem->getItemCountOrSubtype() == 0) {
						player->removeItemInventory(from_cid, true);
						this->FreeThing(fromItem);
					}
				}
				else if(count < oldFromCount) {
					int subcount = oldFromCount - count;
					fromItem->setItemCountOrSubtype(subcount);

					Item *moveItem = Item::CreateItem(fromItem->getID(), count);
					moveItem->pos = toPos;
					toTile->addThing(moveItem);

					if(fromItem->getItemCountOrSubtype() == 0) {
						player->removeItemInventory(from_cid, true);
						this->FreeThing(fromItem);
					}
				}
				else if(player->removeItemInventory(from_cid, true)) {
					fromItem->pos = toPos;
					toTile->addThing(fromItem);
				}
			}
			else if(player->removeItemInventory(from_cid, true)) {
				fromItem->pos = toPos;
				toTile->addThing(fromItem);
			}

			player->updateInventoryWeigth();

			SpectatorVec list;
			SpectatorVec::iterator it;

			getSpectators(Range(player->pos, false), list);

			SpectatorVec tolist;
			getSpectators(Range(toPos, true), tolist);

			for(it = tolist.begin(); it != tolist.end(); ++it) {
				if(std::find(list.begin(), list.end(), *it) == list.end()) {
					list.push_back(*it);
				}
			}

			//players
			for(it = list.begin(); it != list.end(); ++it) {
				if(dynamic_cast<Player*>(*it)) {
					(*it)->onThingMove(player, (slots_t)from_cid, fromItem, oldFromCount, toPos, toItem, oldToCount, count);
				}
			}

			//none-players
			for(it = list.begin(); it != list.end(); ++it) {
				if(!dynamic_cast<Player*>(*it)) {
					(*it)->onThingMove(player, (slots_t)from_cid, fromItem, oldFromCount, toPos, toItem, oldToCount, count);
				}
			}
		}
	}

#ifdef TP_TRASH_BINS
	//creatureBroadcastTileUpdated(toPos);
	//sendAddThing
#endif //TP_TRASH_BINS
}

//ground to container/inventory
void Game::thingMoveInternal(Player *player, const Position& fromPos, unsigned char stackPos,
	unsigned short itemid, unsigned char to_cid, unsigned char to_slotid, bool toInventory, unsigned char count)
{
	Tile *fromTile = map->getTile(fromPos);
	if(!fromTile)
		return;

	Container *toContainer = NULL;

	Item* fromItem = dynamic_cast<Item*>(fromTile->getTopDownItem());
	Item *toItem = NULL;

	if(!fromItem || (fromItem->getID() != itemid) || (fromItem != fromTile->getTopDownItem()))
		return;

	if(toInventory) {
		toItem = player->getItem(to_cid);
		toContainer = dynamic_cast<Container*>(toItem);
	}
	else {
		toContainer = player->getContainer(to_cid);
		if(!toContainer)
			return;

		toItem = toContainer->getItem(to_slotid);
		Container *toSlotContainer = dynamic_cast<Container*>(toItem);

		if(toSlotContainer) {
			toContainer = toSlotContainer;
			toItem = NULL;
		}
	}

	if((toItem == fromItem) || (fromItem->isStackable() && count > fromItem->getItemCountOrSubtype()))
		return;

	/*ground to container*/
	if(toContainer) {
		/*if(onPrepareMoveThing(player, fromItem, fromPos, player->pos, count) &&
				onPrepareMoveThing(player, fromItem, NULL, toContainer, toItem, count))*/

		Position toPos = (toContainer->pos.x == 0xFFFF ? player->pos : toContainer->pos);
		if(onPrepareMoveThing(player, fromPos, NULL, fromItem, toPos, toContainer, toItem, count)) {
			autoCloseTrade(fromItem, true);
			int oldFromCount = fromItem->getItemCountOrSubtype();
			int oldToCount = 0;
			int stackpos = fromTile->getThingStackPos(fromItem);

			if(fromItem->isStackable()) {
				if(toItem && toItem->getID() == fromItem->getID()) {
					oldToCount = toItem->getItemCountOrSubtype();
					int newToCount = std::min(100, oldToCount + count);
					toItem->setItemCountOrSubtype(newToCount);

					if(oldToCount != newToCount) {
						autoCloseTrade(toItem);
					}

					int subcount = oldFromCount - count;
					fromItem->setItemCountOrSubtype(subcount);

					int surplusCount = oldToCount + count  - 100;
					if(surplusCount > 0) {
						Item *surplusItem = Item::CreateItem(fromItem->getID(), surplusCount);

						if(onPrepareMoveThing(player, fromPos, NULL, surplusItem, toPos, toContainer, NULL, count)) {
							autoCloseTrade(toContainer);
							toContainer->addItem(surplusItem);
						}
						else {
							delete surplusItem;
							count -= surplusCount; //re-define the actual amount we move.
							fromItem->setItemCountOrSubtype(fromItem->getItemCountOrSubtype() + surplusCount);
						}
					}

					if(fromItem->getItemCountOrSubtype() == 0) {
						if(fromTile->removeThing(fromItem)){
							this->FreeThing(fromItem);
						}
					}
				}
				else if(count < oldFromCount) {
					int subcount = oldFromCount - count;
					fromItem->setItemCountOrSubtype(subcount);

					autoCloseTrade(toContainer);
					toContainer->addItem(Item::CreateItem(fromItem->getID(), count));

					if(fromItem->getItemCountOrSubtype() == 0) {
						if(fromTile->removeThing(fromItem)){
							this->FreeThing(fromItem);
						}
					}
				}
				else if(fromTile->removeThing(fromItem)) {
					autoCloseTrade(toContainer);
					toContainer->addItem(fromItem);
				}
			}
			else {
				if(fromTile->removeThing(fromItem)) {
					autoCloseTrade(toContainer);
					toContainer->addItem(fromItem);
				}
			}

			if(player->isHoldingContainer(toContainer)) {
				player->updateInventoryWeigth();
			}

			SpectatorVec list;
			SpectatorVec::iterator it;

			getSpectators(Range(fromPos, true), list);

			//players
			for(it = list.begin(); it != list.end(); ++it) {
				if(dynamic_cast<Player*>(*it)) {
					(*it)->onThingMove(player, fromPos, stackpos, fromItem, oldFromCount, toContainer, to_slotid, toItem, oldToCount, count);
				}
			}

			//none-players
			for(it = list.begin(); it != list.end(); ++it) {
				if(!dynamic_cast<Player*>(*it)) {
					(*it)->onThingMove(player, fromPos, stackpos, fromItem, oldFromCount, toContainer, to_slotid, toItem, oldToCount, count);
				}
			}
		}
	}
	//ground to inventory
	else if(toInventory) {
		if(onPrepareMoveThing(player, fromPos, fromItem, (slots_t)to_cid, count) && onPrepareMoveThing(player, fromItem, (slots_t)to_cid, count)) {
			autoCloseTrade(fromItem, true);
			int oldFromCount = fromItem->getItemCountOrSubtype();
			int oldToCount = 0;
			int stackpos = fromTile->getThingStackPos(fromItem);

			if(fromItem->isStackable()) {
				if(toItem && toItem->getID() == fromItem->getID()) {
					oldToCount = toItem->getItemCountOrSubtype();
					int newToCount = std::min(100, oldToCount + count);
					toItem->setItemCountOrSubtype(newToCount);

					if(oldToCount != newToCount) {
						autoCloseTrade(toItem);
					}

					int subcount = oldFromCount - count;
					fromItem->setItemCountOrSubtype(subcount);

					int surplusCount = oldToCount + count  - 100;
					if(surplusCount > 0) {
						fromItem->setItemCountOrSubtype(fromItem->getItemCountOrSubtype() + surplusCount);
						player->sendCancel("Sorry not enough room.");
					}

					if(fromItem->getItemCountOrSubtype() == 0) {
						if(fromTile->removeThing(fromItem)){
							this->FreeThing(fromItem);
						}
					}
				}
				else if(count < oldFromCount) {
					int subcount = oldFromCount - count;
					fromItem->setItemCountOrSubtype(subcount);

					player->removeItemInventory(to_cid, true);
					player->addItemInventory(Item::CreateItem(fromItem->getID(), count), to_cid, true);

					if(toItem) {
						autoCloseTrade(toItem);
						fromTile->addThing(toItem);
						toItem->pos = fromPos;
					}

					if(fromItem->getItemCountOrSubtype() == 0) {
						if(fromTile->removeThing(fromItem)){
							this->FreeThing(fromItem);
						}
					}
				}
				else {
					if(fromTile->removeThing(fromItem)) {
						player->removeItemInventory(to_cid, true);
						player->addItemInventory(fromItem, to_cid, true);

						if(toItem) {
							autoCloseTrade(toItem);
							fromTile->addThing(toItem);
							toItem->pos = fromPos;
						}
					}
				}
			}
			else {
				if(fromTile->removeThing(fromItem)) {
					player->removeItemInventory(to_cid, true);
					player->addItemInventory(fromItem, to_cid, true);

					if(toItem) {
						autoCloseTrade(toItem);
						fromTile->addThing(toItem);
						toItem->pos = fromPos;
					}
				}
			}

			player->updateInventoryWeigth();

			SpectatorVec list;
			SpectatorVec::iterator it;

			getSpectators(Range(fromPos, true), list);

			//players
			for(it = list.begin(); it != list.end(); ++it) {
				if(dynamic_cast<Player*>(*it)) {
					(*it)->onThingMove(player, fromPos, stackpos, fromItem, oldFromCount, (slots_t)to_cid, toItem, oldToCount, count);
				}
			}

			//none-players
			for(it = list.begin(); it != list.end(); ++it) {
				if(!dynamic_cast<Player*>(*it)) {
					(*it)->onThingMove(player, fromPos, stackpos, fromItem, oldFromCount, (slots_t)to_cid, toItem, oldToCount, count);
				}
			}
		}
	}
}

//ground to ground
void Game::thingMoveInternal(Creature *creature, unsigned short from_x, unsigned short from_y, unsigned char from_z,
	unsigned char stackPos, unsigned short itemid, unsigned short to_x, unsigned short to_y, unsigned char to_z, unsigned char count)
{
	Tile *fromTile = getTile(from_x, from_y, from_z);
	if(!fromTile)
		return;

	Tile *toTile   = getTile(to_x, to_y, to_z);
	/*
	if(!toTile){
		if(dynamic_cast<Player*>(player))
			dynamic_cast<Player*>(player)->sendCancelWalk("Sorry, not possible...");
		return;
	}
	*/

	Thing *thing = fromTile->getThingByStackPos(stackPos);

#ifdef __DEBUG__
				//				std::cout << "moving"
				/*
				<< ": from_x: "<< (int)from_x << ", from_y: "<< (int)from_y << ", from_z: "<< (int)from_z
				<< ", stackpos: "<< (int)stackPos
				<< ", to_x: "<< (int)to_x << ", to_y: "<< (int)to_y << ", to_z: "<< (int)to_z
				*/
				//<< std::endl;
#endif

	if (!thing)
		return;

	Item* item = dynamic_cast<Item*>(thing);
	Creature* creatureMoving = dynamic_cast<Creature*>(thing);
	Player* playerMoving = dynamic_cast<Player*>(creatureMoving);
	Player* player = dynamic_cast<Player*>(creature);

	Position oldPos;
	oldPos.x = from_x;
	oldPos.y = from_y;
	oldPos.z = from_z;

#ifdef TP_TRASH_BINS
	if(toTile)
	{
		Thing *tothing = toTile->getThingByStackPos(stackPos);
		Item *toItem = dynamic_cast<Item*>(tothing);

		if(item && toItem && !playerMoving && !creature && toItem->isDeleter())
		{
			fromTile->removeThing(item);
			this->FreeThing(item);
			//creatureBroadcastTileUpdated(oldPos);
			sendRemoveThing(player, item->pos, item, stackPos);
			return;
		}
    }
#endif //TP_TRASH_BINS

	// *** Creature moving itself to a non-tile
	if(!toTile && creatureMoving && creatureMoving == creature){
		//change level begin
		Tile* downTile = getTile(to_x, to_y, to_z+1);
		//diagonal begin
		if(!downTile)
		{

			if(player) {
				player->sendTextMessage(MSG_SMALLINFO, "Sorry, not possible.");
				player->sendCancelWalk();
			}

			return;
		}

		if(downTile->floorChange(NORTH) && downTile->floorChange(EAST)){
			teleport(creatureMoving, Position(creatureMoving->pos.x-2, creatureMoving->pos.y+2, creatureMoving->pos.z+1));
		}
		else if(downTile->floorChange(NORTH) && downTile->floorChange(WEST)){
			teleport(creatureMoving, Position(creatureMoving->pos.x+2, creatureMoving->pos.y+2, creatureMoving->pos.z+1));
		}
		else if(downTile->floorChange(SOUTH) && downTile->floorChange(EAST)){
			teleport(creatureMoving, Position(creatureMoving->pos.x-2, creatureMoving->pos.y-2, creatureMoving->pos.z+1));
		}
		else if(downTile->floorChange(SOUTH) && downTile->floorChange(WEST)){
			teleport(creatureMoving, Position(creatureMoving->pos.x+2, creatureMoving->pos.y-2, creatureMoving->pos.z+1));
		}
		//diagonal end
		else if(downTile->floorChange(NORTH)){
			teleport(creatureMoving, Position(to_x, to_y + 1, creatureMoving->pos.z+1));
		}
		else if(downTile->floorChange(SOUTH)){
			teleport(creatureMoving, Position(to_x, to_y - 1, creatureMoving->pos.z+1));
		}
		else if(downTile->floorChange(EAST)){
			teleport(creatureMoving, Position(to_x - 1, to_y, creatureMoving->pos.z+1));
		}
		else if(downTile->floorChange(WEST)){
			teleport(creatureMoving, Position(to_x + 1, to_y, creatureMoving->pos.z+1));
		}
		else
			if(player) {
				player->sendTextMessage(MSG_SMALLINFO, "Sorry, not possible.");
				player->sendCancelWalk();
			}

		//change level end
		return;
	}


	// *** Checking if the thing can be moved around

	if(!toTile)
		return;

	if(!onPrepareMoveThing(creature, thing, oldPos, Position(to_x, to_y, to_z), count))
		return;

	if(creatureMoving && !onPrepareMoveCreature(creature, creatureMoving, fromTile, toTile))
		return;

	if(!onPrepareMoveThing(creature, thing, fromTile, toTile, count))
		return;

	/*
	if(item && (item->getID() != itemid || item != fromTile->getTopDownItem()))
		return;
	*/

	// *** If the destiny is a teleport item, teleport the thing

	const Teleport *teleportitem = toTile->getTeleportItem();
	if(teleportitem) {
		teleport(thing, teleportitem->getDestPos());
		return;
	}

#ifdef TLM_HOUSE_SYSTEM
	if (playerMoving && toTile->getHouse() &&
		(fromTile->getHouse() != toTile->getHouse() || playerMoving->houseRightsChanged))
	{
		if (playerMoving->access < g_config.ACCESS_HOUSE &&
			toTile->getHouse()->getPlayerRights(playerMoving->getName()) == HOUSE_NONE)
		{
			playerMoving->sendTextMessage(MSG_SMALLINFO, "You are not invited.");
			playerMoving->sendCancelWalk();
			return;
		}
		else
			playerMoving->houseRightsChanged = false;	// if we are invited stop checking rights
	}
#endif //TLM_HOUSE_SYSTEM

	// *** Normal moving

	if(creatureMoving)
	{
		// we need to update the direction the player is facing to...
		// otherwise we are facing some problems in turning into the
		// direction we were facing before the movement
		// check y first cuz after a diagonal move we lock to east or west
		if (to_y < oldPos.y) creatureMoving->direction = NORTH;
		if (to_y > oldPos.y) creatureMoving->direction = SOUTH;
		if (to_x > oldPos.x) creatureMoving->direction = EAST;
		if (to_x < oldPos.x) creatureMoving->direction = WEST;
	}

	int oldstackpos = fromTile->getThingStackPos(thing);
	if (fromTile->removeThing(thing))
	{
		toTile->addThing(thing);

		thing->pos.x = to_x;
		thing->pos.y = to_y;
		thing->pos.z = to_z;

		SpectatorVec list;
		SpectatorVec::iterator it;

		getSpectators(Range(oldPos, Position(to_x, to_y, to_z)), list);

#ifdef TRS_GM_INVISIBLE
		if(playerMoving && playerMoving->gmInvisible)
		{
			for(it = list.begin(); it != list.end(); ++it)
			{
				if(playerMoving == (*it) || (*it)->access >= playerMoving->access)
				{
					if(Player* p = dynamic_cast<Player*>(*it))
					{
						if(p->attackedCreature == creature->getID())
						{
							autoCloseAttack(p, creature);
						}
						(*it)->onThingMove(creature, thing, &oldPos, oldstackpos, 1, 1);
					}
				}
			}
		}
		else if(playerMoving && !playerMoving->gmInvisible)
		{
			for(it = list.begin(); it != list.end(); ++it)
			{
				if(Player* p = dynamic_cast<Player*>(*it))
				{
					if(p->attackedCreature == creature->getID())
					{
						autoCloseAttack(p, creature);
					}
					(*it)->onThingMove(creature, thing, &oldPos, oldstackpos, 1, 1);
				}
			}
		}
		else
		{
			for(it = list.begin(); it != list.end(); ++it)
			{
				if(Player* p = dynamic_cast<Player*>(*it))
				{
					if(p->attackedCreature == creature->getID())
					{
						autoCloseAttack(p, creature);
					}
					(*it)->onThingMove(creature, thing, &oldPos, oldstackpos, 1, 1);
				}
			}
		}
#else //TRS_GM_INVISIBLE
		//players
		for(it = list.begin(); it != list.end(); ++it)
		{
			if(Player* p = dynamic_cast<Player*>(*it)) {
        if(p->attackedCreature == creature->getID()) {
          autoCloseAttack(p, creature);
        }
				(*it)->onThingMove(creature, thing, &oldPos, oldstackpos, 1, 1);
			}
		}
#endif //TRS_GM_INVISIBLE

		//none-players
		for(it = list.begin(); it != list.end(); ++it)
		{
			if(!dynamic_cast<Player*>(*it)) {
				(*it)->onThingMove(creature, thing, &oldPos, oldstackpos, 1, 1);
			}
		}

		autoCloseTrade(item, true);

		if (playerMoving)
		{
			if(playerMoving->attackedCreature != 0) {
				Creature* attackedCreature = getCreatureByID(creatureMoving->attackedCreature);
				if(attackedCreature){
          autoCloseAttack(playerMoving, attackedCreature);
				}
			}

			if(playerMoving->tradePartner != 0) {
				Player* tradePartner = getPlayerByID(playerMoving->tradePartner);
				if(tradePartner) {
					if((std::abs(playerMoving->pos.x - tradePartner->pos.x) > 2) ||
					(std::abs(playerMoving->pos.y - tradePartner->pos.y) > 2) || (playerMoving->pos.z != tradePartner->pos.z)){
						playerCloseTrade(playerMoving);
					}
				}
			}

			//change level begin
			if(toTile->floorChangeDown())
			{
				Tile* downTile = getTile(to_x, to_y, to_z+1);
				if(downTile){
					//diagonal begin
					if(downTile->floorChange(NORTH) && downTile->floorChange(EAST)){
						teleport(playerMoving, Position(playerMoving->pos.x-1, playerMoving->pos.y+1, playerMoving->pos.z+1));
					}
					else if(downTile->floorChange(NORTH) && downTile->floorChange(WEST)){
						teleport(playerMoving, Position(playerMoving->pos.x+1, playerMoving->pos.y+1, playerMoving->pos.z+1));
					}
					else if(downTile->floorChange(SOUTH) && downTile->floorChange(EAST)){
						teleport(playerMoving, Position(playerMoving->pos.x-1, playerMoving->pos.y-1, playerMoving->pos.z+1));
					}
					else if(downTile->floorChange(SOUTH) && downTile->floorChange(WEST)){
						teleport(playerMoving, Position(playerMoving->pos.x+1, playerMoving->pos.y-1, playerMoving->pos.z+1));
					}
					//diagonal end
					else if(downTile->floorChange(NORTH)){
						teleport(playerMoving, Position(playerMoving->pos.x, playerMoving->pos.y+1, playerMoving->pos.z+1));
					}
					else if(downTile->floorChange(SOUTH)){
						teleport(playerMoving, Position(playerMoving->pos.x, playerMoving->pos.y-1, playerMoving->pos.z+1));
					}
					else if(downTile->floorChange(EAST)){
						teleport(playerMoving, Position(playerMoving->pos.x-1, playerMoving->pos.y, playerMoving->pos.z+1));
					}
					else if(downTile->floorChange(WEST)){
						teleport(playerMoving, Position(playerMoving->pos.x+1, playerMoving->pos.y, playerMoving->pos.z+1));
					}
					else { //allow just real tiles to be hole'like
                        // TODO: later can be changed to check for piled items like chairs, boxes
						teleport(playerMoving, Position(playerMoving->pos.x, playerMoving->pos.y, playerMoving->pos.z+1));
					}
				}
			}
			//diagonal begin
			else if(toTile->floorChange(NORTH) && toTile->floorChange(EAST)){
				teleport(playerMoving, Position(playerMoving->pos.x+1, playerMoving->pos.y-1, playerMoving->pos.z-1));
			}
			else if(toTile->floorChange(NORTH) && toTile->floorChange(WEST)){
				teleport(playerMoving, Position(playerMoving->pos.x-1, playerMoving->pos.y-1, playerMoving->pos.z-1));
			}
			else if(toTile->floorChange(SOUTH) && toTile->floorChange(EAST)){
				teleport(playerMoving, Position(playerMoving->pos.x+1, playerMoving->pos.y+1, playerMoving->pos.z-1));
			}
			else if(toTile->floorChange(SOUTH) && toTile->floorChange(WEST)){
				teleport(playerMoving, Position(playerMoving->pos.x-1, playerMoving->pos.y+1, playerMoving->pos.z-1));
			}
			//diagonal end
			else if(toTile->floorChange(NORTH)){
				teleport(playerMoving, Position(playerMoving->pos.x, playerMoving->pos.y-1, playerMoving->pos.z-1));
			}
			else if(toTile->floorChange(SOUTH)){
				teleport(playerMoving, Position(playerMoving->pos.x, playerMoving->pos.y+1, playerMoving->pos.z-1));
			}
			else if(toTile->floorChange(EAST)){
				teleport(playerMoving, Position(playerMoving->pos.x+1, playerMoving->pos.y, playerMoving->pos.z-1));
			}
			else if(toTile->floorChange(WEST)){
				teleport(playerMoving, Position(playerMoving->pos.x-1, playerMoving->pos.y, playerMoving->pos.z-1));
			}
			//change level end
		}

		// Magic Field in destiny field
		if(creatureMoving)
		{
			const MagicEffectItem* fieldItem = toTile->getFieldItem();

			if(fieldItem) {
				const MagicEffectTargetCreatureCondition *magicTargetCondition = fieldItem->getCondition();

				if(!(getWorldType() == WORLD_TYPE_NO_PVP && playerMoving && magicTargetCondition && magicTargetCondition->getOwnerID() != 0)) {
					fieldItem->getDamage(creatureMoving);
				}

				if(magicTargetCondition && ((magicTargetCondition->attackType == ATTACK_FIRE) ||
						(magicTargetCondition->attackType == ATTACK_POISON) ||
						(magicTargetCondition->attackType == ATTACK_ENERGY))) {
					Creature *c = getCreatureByID(magicTargetCondition->getOwnerID());
					creatureMakeMagic(c, thing->pos, magicTargetCondition);
				}
			}
		}
	}
}

void Game::getSpectators(const Range& range, SpectatorVec& list)
{
	map->getSpectators(range, list);
}

void Game::creatureTurn(Creature *creature, Direction dir)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::creatureTurn()");

	if (creature->direction != dir) {
		creature->direction = dir;

		int stackpos = map->getTile(creature->pos)->getThingStackPos(creature);

		SpectatorVec list;
		SpectatorVec::iterator it;

		map->getSpectators(Range(creature->pos, true), list);

		//players
		for(it = list.begin(); it != list.end(); ++it) {
			if(dynamic_cast<Player*>(*it)) {
				(*it)->onCreatureTurn(creature, stackpos);
			}
		}

		//none-players
		for(it = list.begin(); it != list.end(); ++it) {
			if(!dynamic_cast<Player*>(*it)) {
				(*it)->onCreatureTurn(creature, stackpos);
			}
		}
	}
}

void Game::addCommandTag(std::string tag){
	bool found = false;
	for(size_t i=0;i< commandTags.size() ;i++){
		if(commandTags[i] == tag){
			found = true;
			break;
		}
	}
	if(!found){
		commandTags.push_back(tag);
	}
}

void Game::resetCommandTag(){
	commandTags.clear();
}

void Game::creatureSay(Creature *creature, SpeakClasses type, const std::string &text)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::creatureSay()");

	bool GMcommand = false;
	// First, check if this was a GM command
	for(size_t i=0;i< commandTags.size() ;i++){
		if(commandTags[i] == text.substr(0,1)){
			if(commands.exeCommand(creature,text)){
				GMcommand = true;
			}
			break;
		}
	}
	if(!GMcommand){
		Player* player = dynamic_cast<Player*>(creature);
		if (player)
			checkSpell(player, type, text);

		// It was no command, or it was just a player
		SpectatorVec list;
		SpectatorVec::iterator it;

		getSpectators(Range(creature->pos), list);

		//players
		for(it = list.begin(); it != list.end(); ++it) {
			if(dynamic_cast<Player*>(*it)) {
				(*it)->onCreatureSay(creature, type, text);
			}
		}

		//none-players
		for(it = list.begin(); it != list.end(); ++it) {
			if(!dynamic_cast<Player*>(*it)) {
				(*it)->onCreatureSay(creature, type, text);
			}
		}
	}
}

void Game::teleport(Thing *thing, const Position& newPos) {

	if(newPos == thing->pos)
		return;

	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::teleport()");

	//Tile *toTile = getTile( newPos.x, newPos.y, newPos.z );
	Tile *toTile = map->getTile(newPos);
	if(toTile){
		Creature *creature = dynamic_cast<Creature*>(thing);
		if(creature){
			//Tile *fromTile = getTile( thing->pos.x, thing->pos.y, thing->pos.z );
			Tile *fromTile = map->getTile(thing->pos);
			if(!fromTile)
				return;

			int osp = fromTile->getThingStackPos(thing);
			if(!fromTile->removeThing(thing))
				return;

			toTile->addThing(thing);
			Position oldPos = thing->pos;

			SpectatorVec list;
			SpectatorVec::iterator it;

			getSpectators(Range(thing->pos, true), list);

			//players
			for(it = list.begin(); it != list.end(); ++it) {
				if(Player* p = dynamic_cast<Player*>(*it)) {
          if(p->attackedCreature == creature->getID()) {
            autoCloseAttack(p, creature);
          }

					(*it)->onCreatureDisappear(creature, osp, true);
				}
			}

			//none-players
			for(it = list.begin(); it != list.end(); ++it) {
				if(!dynamic_cast<Player*>(*it)) {
					(*it)->onCreatureDisappear(creature, osp, true);
				}
			}

			if(newPos.y < oldPos.y)
				creature->direction = NORTH;
			if(newPos.y > oldPos.y)
				creature->direction = SOUTH;
			if(newPos.x > oldPos.x && (std::abs(newPos.x - oldPos.x) >= std::abs(newPos.y - oldPos.y)) )
				creature->direction = EAST;
			if(newPos.x < oldPos.x && (std::abs(newPos.x - oldPos.x) >= std::abs(newPos.y - oldPos.y)))
				creature->direction = WEST;

			thing->pos = newPos;

			Player *player = dynamic_cast<Player*>(creature);
			if(player && player->attackedCreature != 0){
				Creature* attackedCreature = getCreatureByID(player->attackedCreature);
				if(attackedCreature){
          autoCloseAttack(player, attackedCreature);
				}
			}

			list.clear();
			getSpectators(Range(thing->pos, true), list);

#ifdef TRS_GM_INVISIBLE
			//players
			for(it = list.begin(); it != list.end(); ++it)
			{
				if(player)
				{
					if (player->gmInvisible && player == (*it))
					{
						if(Player* p = dynamic_cast<Player*>(*it))
						{
							if(p->attackedCreature == creature->getID())
							{
								autoCloseAttack(p, creature);
							}
							(*it)->onTeleport(creature, &oldPos, osp);
						}
					}
					else if (player->gmInvisible && player != (*it) && (*it)->access < player->access)
					{
						// Nothing Because he is invisible...
					}
					else
					{
						if(Player* p = dynamic_cast<Player*>(*it))
						{
							if(p->attackedCreature == creature->getID())
							{
								autoCloseAttack(p, creature);
							}
							(*it)->onTeleport(creature, &oldPos, osp);
						}
					}
				}
				else
				creatureBroadcastTileUpdated(newPos);
			}
#else //TRS_GM_INVISIBLE
			//players
			for(it = list.begin(); it != list.end(); ++it)
			{
				if(Player* p = dynamic_cast<Player*>(*it)) {
					if(p->attackedCreature == creature->getID()) {
						autoCloseAttack(p, creature);
					}
					(*it)->onTeleport(creature, &oldPos, osp);
				}
			}
#endif //TRS_GM_INVISIBLE

			//none-players
			for(it = list.begin(); it != list.end(); ++it)
			{
				if(!dynamic_cast<Player*>(*it)) {
					(*it)->onTeleport(creature, &oldPos, osp);
				}
			}
		}
		else{
			if(removeThing(NULL, thing->pos, thing, false)){
				addThing(NULL,newPos,thing);
			}
		}
	}//if(toTile)

}


void Game::creatureChangeOutfit(Creature *creature)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::creatureChangeOutfit()");

	SpectatorVec list;
	SpectatorVec::iterator it;

	getSpectators(Range(creature->pos, true), list);

	//players
	for(it = list.begin(); it != list.end(); ++it) {
		if(dynamic_cast<Player*>(*it)) {
			(*it)->onCreatureChangeOutfit(creature);
		}
	}

	//none-players
	for(it = list.begin(); it != list.end(); ++it) {
		if(!dynamic_cast<Player*>(*it)) {
			(*it)->onCreatureChangeOutfit(creature);
		}
	}
}

void Game::creatureWhisper(Creature *creature, const std::string &text)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::creatureWhisper()");

	SpectatorVec list;
	SpectatorVec::iterator it;

	getSpectators(Range(creature->pos), list);

	//players
	for(it = list.begin(); it != list.end(); ++it) {
		if(dynamic_cast<Player*>(*it)) {
			if(abs(creature->pos.x - (*it)->pos.x) > 1 || abs(creature->pos.y - (*it)->pos.y) > 1)
				(*it)->onCreatureSay(creature, SPEAK_WHISPER, std::string("pspsps"));
			else
				(*it)->onCreatureSay(creature, SPEAK_WHISPER, text);
		}
	}

	//none-players
	for(it = list.begin(); it != list.end(); ++it) {
		if(!dynamic_cast<Player*>(*it)) {
			if(abs(creature->pos.x - (*it)->pos.x) > 1 || abs(creature->pos.y - (*it)->pos.y) > 1)
				(*it)->onCreatureSay(creature, SPEAK_WHISPER, std::string("pspsps"));
			else
				(*it)->onCreatureSay(creature, SPEAK_WHISPER, text);
		}
	}
}

void Game::creatureYell(Creature *creature, std::string &text)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::creatureYell()");
	Player* player = dynamic_cast<Player*>(creature);

	if (player && player->access < g_config.ACCESS_PROTECT && player->exhaustedTicks >= 1000)
	{
		player->exhaustedTicks += g_config.EXHAUSTED_ADD;
		player->sendTextMessage(MSG_SMALLINFO, "You are exhausted.");
	}
	else {
		creature->exhaustedTicks = g_config.EXHAUSTED;
		std::transform(text.begin(), text.end(), text.begin(), upchar);

		SpectatorVec list;
		SpectatorVec::iterator it;

		getSpectators(Range(creature->pos, 18, 18, 14, 14), list);

		//players
		for(it = list.begin(); it != list.end(); ++it) {
			if(dynamic_cast<Player*>(*it)) {
				(*it)->onCreatureSay(creature, SPEAK_YELL, text);
			}
		}

		//none-players
		for(it = list.begin(); it != list.end(); ++it) {
			if(!dynamic_cast<Player*>(*it)) {
				(*it)->onCreatureSay(creature, SPEAK_YELL, text);
			}
		}
	}
}

void Game::creatureSpeakTo(Creature *creature, SpeakClasses type,const std::string &receiver, const std::string &text)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::creatureSpeakTo");

	Player* player = dynamic_cast<Player*>(creature);
	if(!player)
		return;

	Player* toPlayer = getPlayerByName(receiver);
	if(!toPlayer) {
		player->sendTextMessage(MSG_SMALLINFO, "A player with this name is not online.");
		return;
	}

	if(creature->access < g_config.ACCESS_TALK){
		type = SPEAK_PRIVATE;
	}

	toPlayer->onCreatureSay(creature, type, text);

	std::stringstream ss;
	ss << "Message sent to " << toPlayer->getName() << ".";
	player->sendTextMessage(MSG_SMALLINFO, ss.str().c_str());
}

void Game::creatureTalkToChannel(Player *player, SpeakClasses type, std::string &text, unsigned short channelId)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::creatureTalkToChannel");

	if(player->access < g_config.ACCESS_TALK){
		type = SPEAK_CHANNEL_Y;
	}

	g_chat.talkToChannel(player, type, text, channelId);
}

void Game::creatureMonsterYell(Monster* monster, const std::string& text)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::creatureMonsterYell()");

	SpectatorVec list;
	SpectatorVec::iterator it;

	map->getSpectators(Range(monster->pos, 18, 18, 14, 14), list);

	//players
	for(it = list.begin(); it != list.end(); ++it) {
		if(dynamic_cast<Player*>(*it)) {
			(*it)->onCreatureSay(monster, SPEAK_MONSTER1, text);
		}
	}
}

void Game::creatureBroadcastMessage(Creature *creature, const std::string &text)
{
	if(creature->access < g_config.ACCESS_TALK)
		return;

	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::creatureBroadcastMessage()");

	for(AutoList<Player>::listiterator it = Player::listPlayer.list.begin(); it != Player::listPlayer.list.end(); ++it)
	{
		(*it).second->onCreatureSay(creature, SPEAK_BROADCAST, text);
	}
}

/** \todo Someone _PLEASE_ clean up this mess */
bool Game::creatureMakeMagic(Creature *creature, const Position& centerpos, const MagicEffectClass* me)
{

	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::creatureMakeMagic()");

#ifdef __DEBUG__
	cout << "creatureMakeMagic: " << (creature ? creature->getName() : "No name") << ", x: " << centerpos.x << ", y: " << centerpos.y << ", z: " << centerpos.z << std::endl;
#endif

	Position frompos;

	if(creature) {
		frompos = creature->pos;

		if(!creatureOnPrepareMagicAttack(creature, centerpos, me))
		{

			return false;
		}
	}
	else {
		frompos = centerpos;
	}

	MagicAreaVec tmpMagicAreaVec;
	me->getArea(centerpos, tmpMagicAreaVec);

	std::vector<Position> poslist;

	Position topLeft(0xFFFF, 0xFFFF, frompos.z), bottomRight(0, 0, frompos.z);

	//Filter out the tiles we actually can work on
	for(MagicAreaVec::iterator maIt = tmpMagicAreaVec.begin(); maIt != tmpMagicAreaVec.end(); ++maIt) {
		Tile *t = map->getTile(maIt->x, maIt->y, maIt->z);
		if(t && (!creature || (creature->access >= g_config.ACCESS_PROTECT || !me->offensive || !t->isPz()) ) ) {
			if((t->isBlocking(BLOCK_PROJECTILE) == RET_NOERROR) && (me->isIndirect() ||
				//(map->canThrowItemTo(frompos, (*maIt), false, true) && !t->floorChange()))) {
				((map->canThrowObjectTo(centerpos, (*maIt), BLOCK_PROJECTILE) == RET_NOERROR) && !t->floorChange()))) {

				if(maIt->x < topLeft.x)
					topLeft.x = maIt->x;

				if(maIt->y < topLeft.y)
					topLeft.y = maIt->y;

				if(maIt->x > bottomRight.x)
					bottomRight.x = maIt->x;

				if(maIt->y > bottomRight.y)
					bottomRight.y = maIt->y;

				poslist.push_back(*maIt);
			}
		}
	}

	topLeft.z = frompos.z;
	bottomRight.z = frompos.z;

	if(topLeft.x == 0xFFFF || topLeft.y == 0xFFFF || bottomRight.x == 0 || bottomRight.y == 0){

    return false;
	}

#ifdef __DEBUG__
	printf("top left %d %d %d\n", topLeft.x, topLeft.y, topLeft.z);
	printf("bottom right %d %d %d\n", bottomRight.x, bottomRight.y, bottomRight.z);
#endif

	//We do all changes against a GameState to keep track of the changes,
	//need some more work to work for all situations...
	GameState gamestate(this, Range(topLeft, bottomRight));

	//Tile *targettile = getTile(centerpos.x, centerpos.y, centerpos.z);
	Tile *targettile = map->getTile(centerpos);
	bool bSuccess = false;
	bool hasTarget = false;
	bool isBlocking = true;
	if(targettile){
		hasTarget = !targettile->creatures.empty();
		isBlocking = (targettile->isBlocking(BLOCK_SOLID, true) != RET_NOERROR);
	}

	if(targettile && me->canCast(isBlocking, !targettile->creatures.empty())) {
		bSuccess = true;

		//Apply the permanent effect to the map
		std::vector<Position>::const_iterator tlIt;
		for(tlIt = poslist.begin(); tlIt != poslist.end(); ++tlIt) {
			gamestate.onAttack(creature, Position(*tlIt), me);
		}
	}

	SpectatorVec spectatorlist = gamestate.getSpectators();
	SpectatorVec::iterator it;

	for(it = spectatorlist.begin(); it != spectatorlist.end(); ++it) {
		Player* spectator = dynamic_cast<Player*>(*it);

		if(!spectator)
			continue;

		if(bSuccess) {
			me->getDistanceShoot(spectator, creature, centerpos, hasTarget);

			std::vector<Position>::const_iterator tlIt;
			for(tlIt = poslist.begin(); tlIt != poslist.end(); ++tlIt) {
				Position pos = *tlIt;
				//Tile *tile = getTile(pos.x, pos.y, pos.z);
				Tile *tile = map->getTile(pos);
				const CreatureStateVec& creatureStateVec = gamestate.getCreatureStateList(tile);

				if(creatureStateVec.empty()) { //no targets
					me->getMagicEffect(spectator, creature, NULL, pos, 0, targettile->isPz(), isBlocking);
				}
				else {
					for(CreatureStateVec::const_iterator csIt = creatureStateVec.begin(); csIt != creatureStateVec.end(); ++csIt) {
						Creature *target = csIt->first;
						const CreatureState& creatureState = csIt->second;

						me->getMagicEffect(spectator, creature, target, target->pos, creatureState.damage, tile->isPz(), false);

						//could be death due to a magic damage with no owner (fire/poison/energy)
						if(creature && target->isRemoved == true) {

							for(std::vector<Creature*>::const_iterator cit = creatureState.attackerlist.begin(); cit != creatureState.attackerlist.end(); ++cit) {
								Creature* gainExpCreature = *cit;
								if(dynamic_cast<Player*>(gainExpCreature))
									dynamic_cast<Player*>(gainExpCreature)->sendStats();

								if(spectator->CanSee(gainExpCreature->pos.x, gainExpCreature->pos.y, gainExpCreature->pos.z)) {
									std::stringstream exp;
									exp << target->getGainedExperience(gainExpCreature);
									spectator->sendAnimatedText(gainExpCreature->pos, 0xD7, exp.str());
								}
							}

						}

						if(spectator->CanSee(target->pos.x, target->pos.y, target->pos.z))
						{
							if(creatureState.damage != 0) {
								std::stringstream dmg;
								dmg << std::abs(creatureState.damage);
#ifdef TJ_MONSTER_BLOOD
								if (me->attackType & ATTACK_PHYSICAL)
								    spectator->sendAnimatedText(target->pos, target->bloodcolor, dmg.str());
								else
#endif //TJ_MONSTER_BLOOD
									spectator->sendAnimatedText(target->pos, me->animationColor, dmg.str());
							}

							if(creatureState.manaDamage > 0){
								spectator->sendMagicEffect(target->pos, NM_ME_LOOSE_ENERGY);
								std::stringstream manaDmg;
								manaDmg << std::abs(creatureState.manaDamage);
								spectator->sendAnimatedText(target->pos, 2, manaDmg.str());
							}

							if (target->health > 0)
								spectator->sendCreatureHealth(target);

							if (spectator == target){
								CreateManaDamageUpdate(target, creature, creatureState.manaDamage);
								CreateDamageUpdate(target, creature, creatureState.damage);
							}
						}
					}
				}
			}
		}
		else {
			me->FailedToCast(spectator, creature, isBlocking, hasTarget);
		}

	}

	return bSuccess;
}

void Game::creatureApplyDamage(Creature *creature, int64_t damage, int64_t &outDamage, int64_t &outManaDamage
#ifdef YUR_PVP_ARENA
							   , CreatureVector* arenaLosers
#endif //YUR_PVP_ARENA
							   )
{
	outDamage = damage;
	outManaDamage = 0;

	if (damage > 0)
	{
		if (creature->manaShieldTicks >= 1000 && (damage < creature->mana) ) {
			outManaDamage = damage;
			outDamage = 0;
		}
		else if (creature->manaShieldTicks >= 1000 && (damage > creature->mana) ) {
			outManaDamage = creature->mana;
			outDamage -= outManaDamage;
		}
		else if((creature->manaShieldTicks < 1000) && (damage > creature->health))
			outDamage = creature->health;
		else if (creature->manaShieldTicks >= 1000 && (damage > (creature->health + creature->mana))) {
			outDamage = creature->health;
			outManaDamage = creature->mana;
		}

		if(creature->manaShieldTicks < 1000 || (creature->mana == 0))
#ifdef YUR_PVP_ARENA
			creature->drainHealth(outDamage, arenaLosers);
#else
			creature->drainHealth(outDamage);
#endif //YUR_PVP_ARENA
		else if(outManaDamage > 0)
		{
#ifdef YUR_PVP_ARENA
			creature->drainHealth(outDamage, arenaLosers);
#else
			creature->drainHealth(outDamage);
#endif //YUR_PVP_ARENA
			creature->drainMana(outManaDamage);
		}
		else
			creature->drainMana(outDamage);
	}
	else {
		int newhealth = creature->health - damage;
		if(newhealth > creature->healthmax)
			newhealth = creature->healthmax;

		creature->health = newhealth;

		outDamage = creature->health - newhealth;
		outManaDamage = 0;
	}
}

bool Game::creatureCastSpell(Creature *creature, const Position& centerpos, const MagicEffectClass& me) {

	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::creatureCastSpell()");

	return creatureMakeMagic(creature, centerpos, &me);
}

bool Game::creatureThrowRune(Creature *creature, const Position& centerpos, const MagicEffectClass& me) {

	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::creatureThrowRune()");

	bool ret = false;
	if(creature->pos.z != centerpos.z) {
		creature->sendCancel("You need to be on the same floor.");
	}
	//else if(!map->canThrowItemTo(creature->pos, centerpos, false, true)) {
	else if(map->canThrowObjectTo(creature->pos, centerpos, BLOCK_PROJECTILE) != RET_NOERROR) {
		creature->sendCancel("You cannot throw there.");
	}
	else
		ret = creatureMakeMagic(creature, centerpos, &me);



	return ret;
}

bool Game::creatureOnPrepareAttack(Creature *creature, Position pos)
{
  if(creature){
		Player* player = dynamic_cast<Player*>(creature);

		//Tile* tile = (Tile*)getTile(creature->pos.x, creature->pos.y, creature->pos.z);
		Tile* tile = map->getTile(creature->pos);
		//Tile* targettile = getTile(pos.x, pos.y, pos.z);
		Tile* targettile = map->getTile(pos);

		if(creature->access < g_config.ACCESS_PROTECT) {
			if(tile && tile->isPz()) {
				if(player) {
					player->sendTextMessage(MSG_SMALLINFO, "You may not attack a person while you are in a protection zone.");
					playerSetAttackedCreature(player, 0);
				}

				return false;
			}
			else if(targettile && targettile->isPz()) {
				if(player) {
					player->sendTextMessage(MSG_SMALLINFO, "You may not attack a person in a protection zone.");
					playerSetAttackedCreature(player, 0);
				}

				return false;
			}
		}

		return true;
	}

	return false;
}

bool Game::creatureOnPrepareMagicAttack(Creature *creature, Position pos, const MagicEffectClass* me)
{
	if(!me->offensive || me->isIndirect() || creatureOnPrepareAttack(creature, pos)) {
		/*
			if(creature->access < ACCESS_PROTECT) {
				if(!((std::abs(creature->pos.x-centerpos.x) <= 8) && (std::abs(creature->pos.y-centerpos.y) <= 6) &&
					(creature->pos.z == centerpos.z)))
					return false;
			}
		*/

		Player* player = dynamic_cast<Player*>(creature);
		if(player) {
			if(player->access < g_config.ACCESS_PROTECT) {
				if(player->exhaustedTicks >= 1000 && me->causeExhaustion(true)) {
					if(me->offensive) {
						player->sendTextMessage(MSG_SMALLINFO, "You are exhausted.",player->pos, NM_ME_PUFF);
						player->exhaustedTicks += g_config.EXHAUSTED_ADD;
					}

					return false;
				}
				else if(player->mana < me->manaCost) {
					player->sendTextMessage(MSG_SMALLINFO, "You do not have enough mana.",player->pos, NM_ME_PUFF);
					return false;
				}
				else
					player->mana -= (int64_t)me->manaCost;
					//player->manaspent += me->manaCost;
					player->addManaSpent((int64_t)me->manaCost);
			}
		}

		return true;
	}

	return false;
}

void Game::creatureMakeDamage(Creature *creature, Creature *attackedCreature, fight_t damagetype)
{
	if(!creatureOnPrepareAttack(creature, attackedCreature->pos))
		return;


	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::creatureMakeDamage()");

	Player* player = dynamic_cast<Player*>(creature);
	Player* attackedPlayer = dynamic_cast<Player*>(attackedCreature);

	//Tile* targettile = getTile(attackedCreature->pos.x, attackedCreature->pos.y, attackedCreature->pos.z);
	Tile* targettile = map->getTile(attackedCreature->pos);

	//can the attacker reach the attacked?
	bool inReach = false;

	switch(damagetype){
		case FIGHT_MELEE:
			if((std::abs(creature->pos.x-attackedCreature->pos.x) <= 1) &&
				(std::abs(creature->pos.y-attackedCreature->pos.y) <= 1) &&
				(creature->pos.z == attackedCreature->pos.z))
					inReach = true;
		break;
		case FIGHT_DIST:
			if((std::abs(creature->pos.x-attackedCreature->pos.x) <= 8) &&
				(std::abs(creature->pos.y-attackedCreature->pos.y) <= 5) &&
				(creature->pos.z == attackedCreature->pos.z)) {

					//if(map->canThrowItemTo(creature->pos, attackedCreature->pos, false, true))
					if(map->canThrowObjectTo(creature->pos, attackedCreature->pos, BLOCK_PROJECTILE) == RET_NOERROR)
						inReach = true;
				}
		break;
		case FIGHT_MAGICDIST:
			if((std::abs(creature->pos.x-attackedCreature->pos.x) <= 8) &&
				(std::abs(creature->pos.y-attackedCreature->pos.y) <= 5) &&
				(creature->pos.z == attackedCreature->pos.z)) {

					//if(map->canThrowItemTo(creature->pos, attackedCreature->pos, false, true))
					if(map->canThrowObjectTo(creature->pos, attackedCreature->pos, BLOCK_PROJECTILE) == RET_NOERROR)
						inReach = true;
				}
		break;

	}

	if (player && player->access < g_config.ACCESS_PROTECT)
	{
#ifdef YUR_CVS_MODS
		player->inFightTicks = std::max(g_config.PZ_LOCKED, player->inFightTicks);
#else
		player->inFightTicks = (long)g_config.getGlobalNumber("pzlocked", 0);
#endif //YUR_CVS_MODS

		player->sendIcons();
		if(attackedPlayer)
			player->pzLocked = true;
	}

	if(attackedPlayer && attackedPlayer->access < g_config.ACCESS_PROTECT)
	{
#ifdef YUR_CVS_MODS
		attackedPlayer->inFightTicks = std::max(g_config.PZ_LOCKED, attackedPlayer->inFightTicks);
#else
		attackedPlayer->inFightTicks = (long)g_config.getGlobalNumber("pzlocked", 0);
#endif //YUR_CVS_MODS
	 attackedPlayer->sendIcons();
  }

	if(!inReach){
		return;
	}

	//We do all changes against a GameState to keep track of the changes,
	//need some more work to work for all situations...
	GameState gamestate(this, Range(creature->pos, attackedCreature->pos));

	gamestate.onAttack(creature, attackedCreature->pos, attackedCreature);

	const CreatureStateVec& creatureStateVec = gamestate.getCreatureStateList(targettile);
	const CreatureState& creatureState = creatureStateVec[0].second;

	if(player && (creatureState.damage > 0 || creatureState.manaDamage > 0)) {
		player->addSkillTry(1);
	}
	else if(player)
		player->addSkillTry(1);


	SpectatorVec spectatorlist = gamestate.getSpectators();
	SpectatorVec::iterator it;

	for(it = spectatorlist.begin(); it != spectatorlist.end(); ++it) {
		Player* spectator = dynamic_cast<Player*>(*it);
		if(!spectator)
			continue;

		if(damagetype != FIGHT_MELEE){
			spectator->sendDistanceShoot(creature->pos, attackedCreature->pos, creature->getSubFightType());
		}

		if (attackedCreature->manaShieldTicks < 1000 && (creatureState.damage == 0) &&
			(spectator->CanSee(attackedCreature->pos.x, attackedCreature->pos.y, attackedCreature->pos.z))) {
				spectator->sendMagicEffect(attackedCreature->pos, NM_ME_PUFF);
		}
		else if (attackedCreature->manaShieldTicks < 1000 && (creatureState.damage < 0) &&
			(spectator->CanSee(attackedCreature->pos.x, attackedCreature->pos.y, attackedCreature->pos.z))) {
				spectator->sendMagicEffect(attackedCreature->pos, NM_ME_BLOCKHIT);
		}
		else {
			for(std::vector<Creature*>::const_iterator cit = creatureState.attackerlist.begin(); cit != creatureState.attackerlist.end(); ++cit) {
				Creature* gainexpCreature = *cit;
				if(dynamic_cast<Player*>(gainexpCreature))
					dynamic_cast<Player*>(gainexpCreature)->sendStats();

				if(spectator->CanSee(gainexpCreature->pos.x, gainexpCreature->pos.y, gainexpCreature->pos.z)) {
					spectator->sendAnimatedText(gainexpCreature->pos, 0xD7,
						str(attackedCreature->getGainedExperience(gainexpCreature)));
				}
			}

			if (spectator->CanSee(attackedCreature->pos.x, attackedCreature->pos.y, attackedCreature->pos.z))
			{
				if(creatureState.damage > 0) {
					std::stringstream dmg;
					dmg << std::abs(creatureState.damage);
#ifdef TJ_MONSTER_BLOOD
					spectator->sendAnimatedText(attackedCreature->pos, attackedCreature->bloodcolor, dmg.str());
					spectator->sendMagicEffect(attackedCreature->pos, attackedCreature->bloodeffect);
#else
					spectator->sendAnimatedText(attackedCreature->pos, 0xB4, dmg.str());
					spectator->sendMagicEffect(attackedCreature->pos, NM_ME_DRAW_BLOOD);
#endif //TJ_MONSTER_BLOOD
				}

				if(creatureState.manaDamage >0) {
					std::stringstream manaDmg;
					manaDmg << std::abs(creatureState.manaDamage);
					spectator->sendMagicEffect(attackedCreature->pos, NM_ME_LOOSE_ENERGY);
					spectator->sendAnimatedText(attackedCreature->pos, 2, manaDmg.str());
				}

				if (attackedCreature->health > 0)
					spectator->sendCreatureHealth(attackedCreature);

				if (spectator == attackedCreature) {
					CreateManaDamageUpdate(attackedCreature, creature, creatureState.manaDamage);
					CreateDamageUpdate(attackedCreature, creature, creatureState.damage);
				}
			}
		}
	}

	if(damagetype != FIGHT_MELEE && player) {
		player->removeDistItem();
	}


}

std::list<Position> Game::getPathTo(Creature *creature, Position start, Position to, bool creaturesBlock){
	return map->getPathTo(creature, start, to, creaturesBlock);
}

void Game::checkPlayerWalk(unsigned long id)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::checkPlayerWalk");

	Player *player = getPlayerByID(id);

	if(!player)
		return;

	Position pos = player->pos;
	Direction dir = player->pathlist.front();
	player->pathlist.pop_front();

	switch (dir) {
		case NORTH:
			pos.y--;
			break;
		case EAST:
			pos.x++;
			break;
		case SOUTH:
			pos.y++;
			break;
		case WEST:
			pos.x--;
			break;
		case NORTHEAST:
			pos.x++;
			pos.y--;
			break;
		case NORTHWEST:
			pos.x--;
			pos.y--;
			break;
		case SOUTHWEST:
			pos.x--;
			pos.y++;
			break;
		case SOUTHEAST:
			pos.x++;
			pos.y++;
			break;
	}

/*
#ifdef __DEBUG__
	std::cout << "move to: " << dir << std::endl;
#endif
*/

	player->lastmove = OTSYS_TIME();
	this->thingMove(player, player, pos.x, pos.y, pos.z, 1);
	flushSendBuffers();

	if(!player->pathlist.empty()) {
		int ticks = (int)player->getSleepTicks();
/*
#ifdef __DEBUG__
		std::cout << "checkPlayerWalk - " << ticks << std::endl;
#endif
*/
		player->eventAutoWalk = addEvent(makeTask(ticks, std::bind2nd(std::mem_fun(&Game::checkPlayerWalk), id)));
	}
	else
		player->eventAutoWalk = 0;
}

void Game::checkCreature(unsigned long id)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::checkCreature()");

	Creature *creature = getCreatureByID(id);

	if (creature && creature->isRemoved == false)
	{
		int thinkTicks = 0;
		int oldThinkTicks = creature->onThink(thinkTicks);

		if(thinkTicks > 0) {
			creature->eventCheck = addEvent(makeTask(thinkTicks, std::bind2nd(std::mem_fun(&Game::checkCreature), id)));
		}
		else
			creature->eventCheck = 0;

		Player* player = dynamic_cast<Player*>(creature);
		if(player)
		{
			//Tile *tile = getTile(player->pos.x, player->pos.y, player->pos.z);
			Tile *tile = map->getTile(player->pos);
			if(tile == NULL){
				std::cout << "CheckPlayer NULL tile: " << player->getName() << std::endl;
				return;
			}

#ifdef CVS_DAY_CYCLE
			player->sendWorldLightLevel(lightlevel, 0xD7);
#endif //CVS_DAY_CYCLE
#ifdef TR_ANTI_AFK
			player->checkAfk(thinkTicks);
#endif //TR_ANTI_AF
#ifdef YUR_BOH
			player->checkBoh();
#endif //YUR_BOH
#ifdef YUR_RINGS_AMULETS
			player->checkRing(thinkTicks);
#endif //YUR_RINGS_AMULETS
#ifdef YUR_LIGHT_ITEM
			player->checkLightItem(thinkTicks);
#endif //YUR_LIGHT_ITEM
#ifdef TLM_SKULLS_PARTY
			if (player->checkSkull(thinkTicks))
				Skull(player);
#endif //TLM_SKULLS_PARTY
#ifdef YUR_PREMIUM_PROMOTION
			player->checkPremium(thinkTicks);
#endif //YUR_PREMIUM_PROMOTION
#ifdef YUR_INVISIBLE
			if (player->checkInvisible(thinkTicks))
				creatureChangeOutfit(player);
#endif //YUR_INVISIBLE

			if(!tile->isPz()){
				if(player->food > 1000){
					//player->mana += min(5, player->manamax - player->mana);
					player->gainManaTick();
					player->food -= thinkTicks;
					if(player->healthmax - player->health > 0){
						//player->health += min(5, player->healthmax - player->health);
						if(player->gainHealthTick()){
							SpectatorVec list;
							SpectatorVec::iterator it;
							getSpectators(Range(creature->pos), list);
							for(it = list.begin(); it != list.end(); ++it) {
								Player* p = dynamic_cast<Player*>(*it);
								if(p)
									p->sendCreatureHealth(player);
							}
						}
					}
				}
			}

			//send stast only if have changed
			if(player->NeedUpdateStats()){
				player->sendStats();
			}

			player->sendPing();

			if(player->inFightTicks >= 1000) {
				player->inFightTicks -= thinkTicks;

				if(player->inFightTicks < 1000)
					player->pzLocked = false;
					player->sendIcons();
			}

			if(player->exhaustedTicks >= 1000){
				player->exhaustedTicks -= thinkTicks;

				if(player->exhaustedTicks < 0)
					player->exhaustedTicks = 0;
			}

			if(player->manaShieldTicks >=1000){
				player->manaShieldTicks -= thinkTicks;

				if(player->manaShieldTicks  < 1000)
					player->sendIcons();
			}

			if(player->hasteTicks >=1000){
				player->hasteTicks -= thinkTicks;
			}
		}
		else {
			if(creature->manaShieldTicks >=1000){
				creature->manaShieldTicks -= thinkTicks;
			}

			if(creature->hasteTicks >=1000){
				creature->hasteTicks -= thinkTicks;
			}

#ifdef YUR_INVISIBLE
			if (creature->checkInvisible(thinkTicks))
				creatureChangeOutfit(creature);
#endif //YUR_INVISIBLE
		}

		Conditions& conditions = creature->getConditions();
		for(Conditions::iterator condIt = conditions.begin(); condIt != conditions.end(); ++condIt) {
			if(condIt->first == ATTACK_FIRE || condIt->first == ATTACK_ENERGY || condIt->first == ATTACK_POISON) {
				ConditionVec &condVec = condIt->second;

				if(condVec.empty())
					continue;

				CreatureCondition& condition = condVec[0];

				if(condition.onTick(oldThinkTicks)) {
					const MagicEffectTargetCreatureCondition* magicTargetCondition =  condition.getCondition();
					Creature* c = getCreatureByID(magicTargetCondition->getOwnerID());
					creatureMakeMagic(c, creature->pos, magicTargetCondition);

					if(condition.getCount() <= 0) {
						condVec.erase(condVec.begin());
					}
				}
			}
		}
		flushSendBuffers();
	}
}

void Game::changeOutfit(unsigned long id, int looktype){

	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::changeOutfit()");

	Creature *creature = getCreatureByID(id);
	if(creature){
		creature->looktype = looktype;
		creatureChangeOutfit(creature);
	}
}

void Game::changeOutfitAfter(unsigned long id, int looktype, long time)
{
	addEvent(makeTask(time, boost::bind(&Game::changeOutfit, this,id, looktype)));
}

void Game::changeSpeed(unsigned long id, unsigned short speed)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::changeSpeed()");

	Creature *creature = getCreatureByID(id);
	if(creature && creature->hasteTicks < 1000 && creature->speed != speed)
	{
		creature->speed = speed;
		Player* player = dynamic_cast<Player*>(creature);
		if(player){
			player->sendChangeSpeed(creature);
			player->sendIcons();
		}

		SpectatorVec list;
		SpectatorVec::iterator it;

		getSpectators(Range(creature->pos), list);

		//for(unsigned int i = 0; i < list.size(); i++)
		for(it = list.begin(); it != list.end(); ++it) {
			Player* p = dynamic_cast<Player*>(*it);
			if(p)
				p->sendChangeSpeed(creature);
		}
	}
}

void Game::checkCreatureAttacking(unsigned long id)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::checkCreatureAttacking()");

	Creature *creature = getCreatureByID(id);
	if (creature != NULL && creature->isRemoved == false)
	{
		creature->eventCheckAttacking = 0;
		Monster *monster = dynamic_cast<Monster*>(creature);
		if (monster) {
			monster->onAttack();
		}
		else {
			if (creature->attackedCreature != 0)
			{
				Creature *attackedCreature = getCreatureByID(creature->attackedCreature);
				if (attackedCreature)
				{
					//Tile* fromtile = getTile(creature->pos.x, creature->pos.y, creature->pos.z);
					Tile* fromtile = map->getTile(creature->pos);
					if(fromtile == NULL)
					{
						std::cout << "checkCreatureAttacking NULL tile: " << creature->getName() << std::endl;
						//return;
					}
					if (!attackedCreature->isAttackable() == 0 && fromtile && fromtile->isPz() && creature->access < g_config.ACCESS_PROTECT)
					{
						Player* player = dynamic_cast<Player*>(creature);
						if (player) {
							player->sendTextMessage(MSG_SMALLINFO, "You may not attack a person in a protection zone.");
							//player->sendCancelAttacking();
							playerSetAttackedCreature(player, 0);
							return;
						}
					}
					else
					{
#ifdef YUR_INVISIBLE
						if (attackedCreature->isInvisible())
						{
							Player* player = dynamic_cast<Player*>(creature);
							Player* attackedPlayer = dynamic_cast<Player*>(attackedCreature);

							if (player && !attackedPlayer) {
								player->sendTextMessage(MSG_SMALLINFO, "Target lost.");
								playerSetAttackedCreature(player, 0);
								return;
							}
						}
#endif //YUR_INVISIBLE
						if (!attackedCreature->isRemoved)
						{
							Player* player = dynamic_cast<Player*>(creature);
							if (player)
							{
#ifdef SD_BURST_ARROW
								if (player->isUsingBurstArrows())
									burstArrow(player, attackedCreature->pos);
#endif //SD_BURST_ARROW
#ifdef JD_WANDS
								int wandid = player->getWandId();
								if (wandid > 0)
									useWand(player, attackedCreature, wandid);
#endif //JD_WANDS
							}
							this->creatureMakeDamage(creature, attackedCreature, creature->getFightType());
						}
					}

					creature->eventCheckAttacking = addEvent(makeTask(2000, std::bind2nd(std::mem_fun(&Game::checkCreatureAttacking), id)));
				}
			}
		}
		flushSendBuffers();
	}


}

void Game::checkDecay(int t)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::checkDecay()");

	addEvent(makeTask(DECAY_INTERVAL, boost::bind(&Game::checkDecay,this,DECAY_INTERVAL)));

	list<decayBlock*>::iterator it;
	for(it = decayVector.begin();it != decayVector.end();){
		(*it)->decayTime -= t;
		if((*it)->decayTime <= 0){
			list<Item*>::iterator it2;
			for(it2 = (*it)->decayItems.begin(); it2 != (*it)->decayItems.end(); it2++){
				/*todo: Decaying item could be in a  container carried by a player,
				should all items have a pointer to their parent (like containers)?*/
				Item* item = *it2;
				item->isDecaying = false;
				if(item->canDecay()){
					if(item->pos.x != 0xFFFF){
						Tile *tile = map->getTile(item->pos);
						if(tile){
							Position pos = item->pos;
							Item* newitem = item->decay();

							if(newitem){
								int stackpos = tile->getThingStackPos(item);
								if(newitem == item){
									sendUpdateThing(NULL,pos,newitem,stackpos);
								}
								else{
									if(tile->removeThing(item)){
										//autoclose containers
										if(dynamic_cast<Container*>(item)){
											SpectatorVec list;
											SpectatorVec::iterator it;

											getSpectators(Range(pos, true), list);

											for(it = list.begin(); it != list.end(); ++it) {
												Player* spectator = dynamic_cast<Player*>(*it);
												if(spectator)
													spectator->onThingRemove(item);
											}
										}

										tile->insertThing(newitem, stackpos);
										sendUpdateThing(NULL,pos,newitem,stackpos);
										FreeThing(item);
									}
								}
								startDecay(newitem);
							}
							else{
								if(removeThing(NULL,pos,item)){
									FreeThing(item);
								}
							}//newitem
						}//tile
					}//pos != 0xFFFF
				}//item->canDecay()
				FreeThing(item);
			}//for it2
			delete *it;
			it = decayVector.erase(it);
		}//(*it)->decayTime <= 0
		else{
			it++;
		}
	}//for it

	flushSendBuffers();
}

void Game::startDecay(Item* item){
	if(item->isDecaying)
		return;//dont add 2 times the same item
	//get decay time
	item->isDecaying = true;
	unsigned long dtime = item->getDecayTime();
	if(dtime == 0)
		return;
	//round time
	if(dtime < DECAY_INTERVAL)
		dtime = DECAY_INTERVAL;
	dtime = (dtime/DECAY_INTERVAL)*DECAY_INTERVAL;
	item->useThing();
	//search if there are any block with this time
	list<decayBlock*>::iterator it;
	for(it = decayVector.begin();it != decayVector.end();it++){
		if((*it)->decayTime == dtime){
			(*it)->decayItems.push_back(item);
			return;
		}
	}
	//we need a new decayBlock
	decayBlock* db = new decayBlock;
	db->decayTime = dtime;
	db->decayItems.clear();
	db->decayItems.push_back(item);
	decayVector.push_back(db);
}

void Game::checkSpawns(int t)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::checkSpawns()");

	SpawnManager::instance()->checkSpawns(t);
	this->addEvent(makeTask(t, std::bind2nd(std::mem_fun(&Game::checkSpawns), t)));
}

void Game::CreateDamageUpdate(Creature* creature, Creature* attackCreature, int64_t damage)
{
	Player* player = dynamic_cast<Player*>(creature);
	Player* attackPlayer = dynamic_cast<Player*>(attackCreature);
	if(!player)
		return;
	//player->sendStats();
	//msg.AddPlayerStats(player);
	if (damage > 0) {
		std::stringstream dmgmesg;

		if(damage == 1) {
			dmgmesg << "You lose 1 hitpoint";
		}
		else
			dmgmesg << "You lose " << damage << " hitpoints";

		if(attackPlayer) {
			dmgmesg << " due to an attack by " << attackCreature->getName();
		}
		else if(attackCreature) {
			std::string strname = attackCreature->getName();
			std::transform(strname.begin(), strname.end(), strname.begin(), (int(*)(int))tolower);
			dmgmesg << " due to an attack by a " << strname;
		}
		dmgmesg <<".";

		player->sendTextMessage(MSG_EVENT, dmgmesg.str().c_str());
		//msg.AddTextMessage(MSG_EVENT, dmgmesg.str().c_str());
	}
	if (player->isRemoved == true){
		player->sendTextMessage(MSG_ADVANCE, "You are dead.");
	}
}

void Game::CreateManaDamageUpdate(Creature* creature, Creature* attackCreature, int64_t damage)
{
	Player* player = dynamic_cast<Player*>(creature);
	if(!player)
		return;
	//player->sendStats();
	//msg.AddPlayerStats(player);
	if (damage > 0) {
		std::stringstream dmgmesg;
		dmgmesg << "You lose " << damage << " mana";
		if(attackCreature) {
			dmgmesg << " blocking an attack by " << attackCreature->getName();
		}
		dmgmesg <<".";

		player->sendTextMessage(MSG_EVENT, dmgmesg.str().c_str());
		//msg.AddTextMessage(MSG_EVENT, dmgmesg.str().c_str());
	}
}

bool Game::creatureSaySpell(Creature *creature, const std::string &text)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::creatureSaySpell()");

	bool ret = false;

	Player* player = dynamic_cast<Player*>(creature);
	std::string temp, var;
	unsigned int loc = (uint32_t)text.find( "\"", 0 );
	if( loc != string::npos && loc >= 0){
		temp = std::string(text, 0, loc-1);
		var = std::string(text, (loc+1), text.size()-loc-1);
	}
	else {
		temp = text;
		var = std::string("");
	}

	std::transform(temp.begin(), temp.end(), temp.begin(), (int(*)(int))tolower);

	if(creature->access >= g_config.ACCESS_PROTECT || !player){
		std::map<std::string, Spell*>::iterator sit = spells.getAllSpells()->find(temp);
		if( sit != spells.getAllSpells()->end() ) {
			sit->second->getSpellScript()->castSpell(creature, creature->pos, var);
			ret = true;
		}
	}
	else if(player){
		std::map<std::string, Spell*>* tmp = spells.getVocSpells(player->vocation);
		if(tmp){
			std::map<std::string, Spell*>::iterator sit = tmp->find(temp);
			if( sit != tmp->end() ) {
				if(player->maglevel >= sit->second->getMagLv()){
#ifdef YUR_LEARN_SPELLS
					if (g_config.LEARN_SPELLS && !player->knowsSpell(temp))
						ret = false;
					else
#endif //YUR_LEARN_SPELLS
					{
						sit->second->getSpellScript()->castSpell(creature, creature->pos, var);
						ret = true;
					}
				}
			}
		}
	}


	return ret;
}

void Game::playerAutoWalk(Player* player, std::list<Direction>& path)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::playerAutoWalk()");

	stopEvent(player->eventAutoWalk);

	if(player->isRemoved)
		return;

	player->pathlist = path;
	int ticks = (int)player->getSleepTicks();
/*
#ifdef __DEBUG__
	std::cout << "playerAutoWalk - " << ticks << std::endl;
#endif
*/

	player->eventAutoWalk = addEvent(makeTask(ticks, std::bind2nd(std::mem_fun(&Game::checkPlayerWalk), player->getID())));

	// then we schedule the movement...
  // the interval seems to depend on the speed of the char?
	//player->eventAutoWalk = addEvent(makeTask<Direction>(0, MovePlayer(player->getID()), path, 400, StopMovePlayer(player->getID())));
	//player->pathlist = path;
}

bool Game::playerUseItemEx(Player *player, const Position& posFrom,const unsigned char  stack_from,
		const Position &posTo,const unsigned char stack_to, const unsigned short itemid)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::playerUseItemEx()");

	if(player->isRemoved)
		return false;

	bool ret = false;

	Position thingpos = getThingMapPos(player, posFrom);
	Item *item = dynamic_cast<Item*>(getThing(posFrom, stack_from, player));

	if(item) {
		//Runes
		std::map<unsigned short, Spell*>::iterator sit = spells.getAllRuneSpells()->find(item->getID());
		if(sit != spells.getAllRuneSpells()->end()) {
			if( (abs(thingpos.x - player->pos.x) > 1) || (abs(thingpos.y - player->pos.y) > 1) ) {
				player->sendCancel("To far away...");
				ret = false;
			}
			else {
				std::string var = std::string("");
				if(player->access >= g_config.ACCESS_PROTECT || sit->second->getMagLv() <= player->maglevel)
				{
					bool success = sit->second->getSpellScript()->castSpell(player, posTo, var);
					ret = success;
					if(success) {
						autoCloseTrade(item);
						item->setItemCharge(std::max((int)item->getItemCharge() - 1, 0) );
						if(item->getItemCharge() == 0) {
							if(removeThing(player,posFrom,item)){
								FreeThing(item);
							}
						}
					}
				}
				else
				{
					player->sendCancel("You don't have the required magic level to use that rune.");
				}
			}
		}
		else{
			actions.UseItemEx(player,posFrom,stack_from,posTo,stack_to,itemid);
			ret = true;
		}
	}


	return ret;
}


bool Game::playerUseItem(Player *player, const Position& pos, const unsigned char stackpos, const unsigned short itemid, unsigned char index)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::playerUseItem()");

	if(player->isRemoved)
		return false;
	actions.UseItem(player,pos,stackpos,itemid,index);
	return true;
}

bool Game::playerUseBattleWindow(Player *player, Position &posFrom, unsigned char stackpos, unsigned short itemid, unsigned long creatureid)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::playerUseBattleWindow");

	if(player->isRemoved)
		return false;

	Creature *creature = getCreatureByID(creatureid);
	if(!creature || dynamic_cast<Player*>(creature))
		return false;

	if(std::abs(creature->pos.x - player->pos.x) > 7 || std::abs(creature->pos.y - player->pos.y) > 5 || creature->pos.z != player->pos.z)
		return false;

	bool ret = false;

	Position thingpos = getThingMapPos(player, posFrom);
	Item *item = dynamic_cast<Item*>(getThing(posFrom, stackpos, player));
	if(item) {
		//Runes
		std::map<unsigned short, Spell*>::iterator sit = spells.getAllRuneSpells()->find(item->getID());
		if(sit != spells.getAllRuneSpells()->end()) {
			if( (abs(thingpos.x - player->pos.x) > 1) || (abs(thingpos.y - player->pos.y) > 1) ) {
				player->sendCancel("To far away...");
			}
			else {
				std::string var = std::string("");
				if(player->access >= g_config.ACCESS_PROTECT || sit->second->getMagLv() <= player->maglevel)
				{
					bool success = sit->second->getSpellScript()->castSpell(player, creature->pos, var);
					ret = success;
					if(success){
						autoCloseTrade(item);
						item->setItemCharge(std::max((int)item->getItemCharge() - 1, 0) );
						if(item->getItemCharge() == 0){
							if(removeThing(player,posFrom,item)){
								FreeThing(item);
							}
						}
					}
				}
				else
				{
					player->sendCancel("You don't have the required magic level to use that rune.");
				}
			}
		}
	}
	return ret;
}

bool Game::playerRotateItem(Player *player, const Position& pos, const unsigned char stackpos, const unsigned short itemid)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::playerRotateItem()");

	if(player->isRemoved)
		return false;

	if(std::abs(player->pos.x - pos.x) > 1 || std::abs(player->pos.y - pos.y) > 1 || player->pos.z != pos.z){
		player->sendCancel("Too far away.");
		return false;
	}

	Item *item = dynamic_cast<Item*>(getThing(pos, stackpos, player));
	if(item && item->rotate()){
		sendUpdateThing(player, pos, item, stackpos);
	}

	return false;
}

void Game::playerRequestTrade(Player* player, const Position& pos,
	const unsigned char stackpos, const unsigned short itemid, unsigned long playerid)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::playerRequestTrade()");

	if(player->isRemoved)
		return;

	Player *tradePartner = getPlayerByID(playerid);
	if(!tradePartner || tradePartner == player) {
		player->sendTextMessage(MSG_INFO, "Sorry, not possible.");
		return;
	}

	if(player->tradeState != TRADE_NONE && !(player->tradeState == TRADE_ACKNOWLEDGE && player->tradePartner == playerid)) {
		player->sendCancel("You are already trading.");
		return;
	}
	else if(tradePartner->tradeState != TRADE_NONE && tradePartner->tradePartner != player->getID()) {
		player->sendCancel("This player is already trading.");
		return;
	}

	Item *tradeItem = dynamic_cast<Item*>(getThing(pos, stackpos, player));
	if(!tradeItem || tradeItem->getID() != itemid || !tradeItem->isPickupable()) {
		player->sendCancel("Sorry, not possible.");
		return;
	}

	if(!player->removeItem(tradeItem, true)) {
		/*if( (abs(player->pos.x - pos.x) > 1) || (abs(player->pos.y - pos.y) > 1) ) {
			player->sendCancel("To far away...");
			return;
		}*/
		player->sendCancel("Sorry, not possible.");
		return;
	}

	std::map<Item*, unsigned long>::const_iterator it;
	const Container* container = NULL;
	for(it = tradeItems.begin(); it != tradeItems.end(); it++) {
		if(tradeItem == it->first ||
			((container = dynamic_cast<const Container*>(tradeItem)) && container->isHoldingItem(it->first)) ||
			((container = dynamic_cast<const Container*>(it->first)) && container->isHoldingItem(tradeItem)))
		{
			player->sendTextMessage(MSG_INFO, "This item is already beeing traded.");
			return;
		}
	}

	Container* tradeContainer = dynamic_cast<Container*>(tradeItem);
	if(tradeContainer && tradeContainer->getItemHoldingCount() + 1 > 100){
		player->sendTextMessage(MSG_INFO, "You can not trade more than 100 items.");
		return;
	}

	player->tradePartner = playerid;
	player->tradeItem = tradeItem;
	player->tradeState = TRADE_INITIATED;
	tradeItem->useThing();
	tradeItems[tradeItem] = player->getID();

	player->sendTradeItemRequest(player, tradeItem, true);

	if(tradePartner->tradeState == TRADE_NONE){
		std::stringstream trademsg;
		trademsg << player->getName() <<" wants to trade with you.";
		tradePartner->sendTextMessage(MSG_INFO, trademsg.str().c_str());
		tradePartner->tradeState = TRADE_ACKNOWLEDGE;
		tradePartner->tradePartner = player->getID();
	}
	else {
		Item* counterOfferItem = tradePartner->tradeItem;
		player->sendTradeItemRequest(tradePartner, counterOfferItem, false);
		tradePartner->sendTradeItemRequest(player, tradeItem, false);
	}
}

void Game::playerAcceptTrade(Player* player)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::playerAcceptTrade()");

	if(player->isRemoved)
		return;

	player->setAcceptTrade(true);
	Player *tradePartner = getPlayerByID(player->tradePartner);
	if(tradePartner && tradePartner->getAcceptTrade()) {
		Item *tradeItem1 = player->tradeItem;
		Item *tradeItem2 = tradePartner->tradeItem;

		player->sendCloseTrade();
		tradePartner->sendCloseTrade();

		if(player->addItem(tradeItem2, true) && tradePartner->addItem(tradeItem1, true) &&
			player->removeItem(tradeItem1, true) && tradePartner->removeItem(tradeItem2, true)){

			player->removeItem(tradeItem1);
			tradePartner->removeItem(tradeItem2);

			player->onThingRemove(tradeItem1);
			tradePartner->onThingRemove(tradeItem2);

			player->addItem(tradeItem2);
			tradePartner->addItem(tradeItem1);
		}
		else{
			player->sendTextMessage(MSG_SMALLINFO, "Sorry not possible.");
			tradePartner->sendTextMessage(MSG_SMALLINFO, "Sorry not possible.");
		}

		std::map<Item*, unsigned long>::iterator it;

		it = tradeItems.find(tradeItem1);
		if(it != tradeItems.end()) {
			FreeThing(it->first);
			tradeItems.erase(it);
		}

		it = tradeItems.find(tradeItem2);
		if(it != tradeItems.end()) {
			FreeThing(it->first);
			tradeItems.erase(it);
		}

		player->setAcceptTrade(false);
		tradePartner->setAcceptTrade(false);
	}
}

void Game::playerLookInTrade(Player* player, bool lookAtCounterOffer, int index)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::playerLookInTrade()");

	Player *tradePartner = getPlayerByID(player->tradePartner);
	if(!tradePartner)
		return;

	Item *tradeItem = NULL;

	if(lookAtCounterOffer)
		tradeItem = tradePartner->getTradeItem();
	else
		tradeItem = player->getTradeItem();

	if(!tradeItem)
		return;

	if(index == 0) {
		stringstream ss;
		ss << "You see " << tradeItem->getDescription(true);
		player->sendTextMessage(MSG_INFO, ss.str().c_str());
		return;
	}

	Container *tradeContainer = dynamic_cast<Container*>(tradeItem);
	if(!tradeContainer || index > tradeContainer->getItemHoldingCount())
		return;

	bool foundItem = false;
	std::list<const Container*> stack;
	stack.push_back(tradeContainer);

	ContainerList::const_iterator it;

	while(!foundItem && stack.size() > 0) {
		const Container *container = stack.front();
		stack.pop_front();

		for (it = container->getItems(); it != container->getEnd(); ++it) {
			Container *container = dynamic_cast<Container*>(*it);
			if(container) {
				stack.push_back(container);
			}

			--index;
			if(index == 0) {
				tradeItem = *it;
				foundItem = true;
				break;
			}
		}
	}

	if(foundItem) {
		stringstream ss;
		ss << "You see " << tradeItem->getDescription(true);
		player->sendTextMessage(MSG_INFO, ss.str().c_str());
	}
}

void Game::playerCloseTrade(Player* player)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::playerCloseTrade()");

	Player* tradePartner = getPlayerByID(player->tradePartner);

	std::vector<Item*>::iterator it;
	if(player->getTradeItem()) {
		std::map<Item*, unsigned long>::iterator it = tradeItems.find(player->getTradeItem());
		if(it != tradeItems.end()) {
			FreeThing(it->first);
			tradeItems.erase(it);
		}
	}

	player->setAcceptTrade(false);
	player->sendTextMessage(MSG_SMALLINFO, "Trade cancelled.");
	player->sendCloseTrade();

	if(tradePartner) {
		if(tradePartner->getTradeItem()) {
			std::map<Item*, unsigned long>::iterator it = tradeItems.find(tradePartner->getTradeItem());
			if(it != tradeItems.end()) {
				FreeThing(it->first);
				tradeItems.erase(it);
			}
		}

		tradePartner->setAcceptTrade(false);
		tradePartner->sendTextMessage(MSG_SMALLINFO, "Trade cancelled.");
		tradePartner->sendCloseTrade();
	}
}

void Game::autoCloseTrade(const Item* item, bool itemMoved /*= false*/)
{
	if(!item)
		return;

	std::map<Item*, unsigned long>::const_iterator it;
	const Container* container = NULL;
	for(it = tradeItems.begin(); it != tradeItems.end(); it++) {
		if(item == it->first ||
			(itemMoved && (container = dynamic_cast<const Container*>(item)) && container->isHoldingItem(it->first)) ||
			((container = dynamic_cast<const Container*>(it->first)) && container->isHoldingItem(item)))
		{
			Player* player = getPlayerByID(it->second);
			if(player){
				playerCloseTrade(player);
			}

			break;
		}
	}
}

void Game::autoCloseAttack(Player* player, Creature* target)
{
  if((std::abs(player->pos.x - target->pos.x) > 7) ||
  (std::abs(player->pos.y - target->pos.y) > 5) || (player->pos.z != target->pos.z)){
	  player->sendTextMessage(MSG_SMALLINFO, "Target lost.");
	  playerSetAttackedCreature(player, 0);
  }
}

void Game::playerSetAttackedCreature(Player* player, unsigned long creatureid)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::playerSetAttackedCreature()");

	if(player->isRemoved)
		return;

	if(player->attackedCreature != 0 && creatureid == 0) {
		player->sendCancelAttacking();
	}


	Creature* attackedCreature = NULL;
	if(creatureid != 0) {
		attackedCreature = getCreatureByID(creatureid);
	}

	Player* attackedPlayer = dynamic_cast<Player*>(attackedCreature);
	bool pvpArena = false, rook = false, attackedIsSummon = false;

#ifdef YUR_PVP_ARENA
	if (player && attackedCreature)
	{
		Tile *t1 = map->getTile(player->pos), *t2 = map->getTile(attackedCreature->pos);
		pvpArena = t1 && t2 && t1->isPvpArena() && t2->isPvpArena();
	}
#endif //YUR_PVP_ARENA

#ifdef YUR_ROOKGARD
	rook = player && player->isRookie() && attackedPlayer && attackedPlayer->isRookie();
#endif //YUR_ROOKGARD

#ifdef TR_SUMMONS
	attackedIsSummon = (attackedCreature && attackedCreature->isPlayersSummon() && attackedCreature->getMaster() != player);
#endif //TR_SUMMONS

	if(!attackedCreature || (attackedCreature->access >= g_config.ACCESS_PROTECT || ((getWorldType() == WORLD_TYPE_NO_PVP || rook) &&
		!pvpArena && player->access < g_config.ACCESS_PROTECT && (dynamic_cast<Player*>(attackedCreature) || attackedIsSummon)))) {
	if(attackedCreature) {
		  player->sendTextMessage(MSG_SMALLINFO, "You may not attack this player.");
    }
		player->sendCancelAttacking();
		player->setAttackedCreature(NULL);
		stopEvent(player->eventCheckAttacking);
		player->eventCheckAttacking = 0;
	}
	else if(attackedCreature) {
		player->setAttackedCreature(attackedCreature);
		stopEvent(player->eventCheckAttacking);
		player->eventCheckAttacking = addEvent(makeTask(2000, std::bind2nd(std::mem_fun(&Game::checkCreatureAttacking), player->getID())));
	}

}

bool Game::requestAddVip(Player* player, const std::string &vip_name)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::requestAddVip");
	std::string real_name;
	real_name = vip_name;
	unsigned long guid;
	unsigned long access_lvl;

	if(!IOPlayer::instance()->getGuidByName(guid, access_lvl, real_name)){
		player->sendTextMessage(MSG_SMALLINFO, "A player with that name doesn't exist.");
		return false;
	}
	if(access_lvl > (unsigned long)player->access){
		player->sendTextMessage(MSG_SMALLINFO, "You can not add this player.");
		return false;
	}
	bool online = (getPlayerByName(real_name) != NULL);
	return player->addVIP(guid, real_name, online);
}

void Game::flushSendBuffers()
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::flushSendBuffers()");

	for(std::vector<Player*>::iterator it = BufferedPlayers.begin(); it != BufferedPlayers.end(); ++it) {
		(*it)->flushMsg();
		(*it)->SendBuffer = false;
		(*it)->releaseThing();
/*
#ifdef __DEBUG__
		std::cout << "flushSendBuffers() - releaseThing()" << std::endl;
#endif
*/
		}
	BufferedPlayers.clear();

	//free memory
	for(std::vector<Thing*>::iterator it = ToReleaseThings.begin(); it != ToReleaseThings.end(); ++it){
		(*it)->releaseThing();
	}
	ToReleaseThings.clear();


	return;
}

void Game::addPlayerBuffer(Player* p)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::addPlayerBuffer()");

/*
#ifdef __DEBUG__
	std::cout << "addPlayerBuffer() - useThing()" << std::endl;
#endif
*/
	if(p->SendBuffer == false){
		p->useThing();
		BufferedPlayers.push_back(p);
		p->SendBuffer = true;
	}

	return;
}

void Game::FreeThing(Thing* thing){

	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::FreeThing()");
	//std::cout << "freeThing() " << thing <<std::endl;
	ToReleaseThings.push_back(thing);

	return;
}
/*
ADD
container(player,pos-cid,thing)
inventory(player,pos-i,[ignored])
ground([ignored],postion,thing)

REMOVE
container(player,pos-cid,thing,autoclose?)
inventory(player,pos-i,thing,autoclose?)
ground([ignored],postion,thing,autoclose?,stackpos)

UPDATE
container(player,pos-cid,thing)
inventory(player,pos-i,[ignored])
ground([ignored],postion,thing,stackpos)
*/
void Game::sendAddThing(Player* player,const Position &pos,const Thing* thing){
	if(pos.x == 0xFFFF) {
		if(!player)
			return;
		if(pos.y & 0x40) { //container
			if(!thing)
				return;

			const Item *item = dynamic_cast<const Item*>(thing);
			if(!item)
				return;

			unsigned char containerid = pos.y & 0x0F;
			Container* container = player->getContainer(containerid);
			if(!container)
				return;

			SpectatorVec list;
			SpectatorVec::iterator it;

			Position centerpos = (container->pos.x == 0xFFFF ? player->pos : container->pos);
			getSpectators(Range(centerpos,2,2,2,2,false), list);

			if(!list.empty()) {
				for(it = list.begin(); it != list.end(); ++it) {
					Player *spectator = dynamic_cast<Player*>(*it);
					if(spectator)
						spectator->onItemAddContainer(container,item);
				}
			}
			else
				player->onItemAddContainer(container,item);

		}
		else //inventory
		{
			player->sendInventory(pos.y);
		}
	}
	else //ground
	{
		if(!thing)
			return;

		SpectatorVec list;
		SpectatorVec::iterator it;

		getSpectators(Range(pos,true), list);

		//players
		for(it = list.begin(); it != list.end(); ++it) {
			if(dynamic_cast<Player*>(*it)) {
				(*it)->onThingAppear(thing);
			}
		}

		//none-players
		for(it = list.begin(); it != list.end(); ++it) {
			if(!dynamic_cast<Player*>(*it)) {
				(*it)->onThingAppear(thing);
			}
		}
	}
}

void Game::sendRemoveThing(Player* player,const Position &pos,const Thing* thing,const unsigned char stackpos /*=1*/ ,const bool autoclose/* =false*/){
	if(!thing)
		return;

	const Item *item = dynamic_cast<const Item*>(thing);
	bool perform_autoclose = false;
	if(autoclose && item){
		const Container *container = dynamic_cast<const Container*>(item);
		if(container)
			perform_autoclose = true;
	}

	if(pos.x == 0xFFFF) {
		if(!player)
			return;
		if(pos.y & 0x40) { //container
			if(!item)
				return;

			unsigned char containerid = pos.y & 0x0F;
			Container* container = player->getContainer(containerid);
			if(!container)
				return;

			//check that item is in the container
			unsigned char slot = container->getSlotNumberByItem(item);

			SpectatorVec list;
			SpectatorVec::iterator it;

			Position centerpos = (container->pos.x == 0xFFFF ? player->pos : container->pos);
			getSpectators(Range(centerpos,2,2,2,2,false), list);

			if(!list.empty()) {
				for(it = list.begin(); it != list.end(); ++it) {
					Player *spectator = dynamic_cast<Player*>(*it);
					if(spectator){
						spectator->onItemRemoveContainer(container,slot);
						if(perform_autoclose){
							spectator->onThingRemove(thing);
						}
					}
				}
			}
			else{
				player->onItemRemoveContainer(container,slot);
				if(perform_autoclose){
					player->onThingRemove(thing);
				}
			}

		}
		else //inventory
		{
			player->removeItemInventory(pos.y);
			if(perform_autoclose){
				player->onThingRemove(thing);
			}
		}
	}
	else //ground
	{
		SpectatorVec list;
		SpectatorVec::iterator it;

		getSpectators(Range(pos,true), list);

		//players
		for(it = list.begin(); it != list.end(); ++it) {
			Player *spectator = dynamic_cast<Player*>(*it);
			if(spectator) {
				spectator->onThingDisappear(thing,stackpos);

				if(perform_autoclose){
					spectator->onThingRemove(thing);
				}
			}
		}

		//none-players
		for(it = list.begin(); it != list.end(); ++it) {
			if(!dynamic_cast<Player*>(*it)) {
				(*it)->onThingDisappear(thing,stackpos);
			}
		}
	}
}

void Game::sendUpdateThing(Player* player,const Position &pos,const Thing* thing,const unsigned char stackpos/*=1*/){

	if(pos.x == 0xFFFF) {
		if(!player)
			return;
		if(pos.y & 0x40) { //container
			if(!thing)
				return;

			const Item *item = dynamic_cast<const Item*>(thing);
			if(!item)
				return;

			unsigned char containerid = pos.y & 0x0F;
			Container* container = player->getContainer(containerid);
			if(!container)
				return;
			//check that item is in the container
			unsigned char slot = container->getSlotNumberByItem(item);

			SpectatorVec list;
			SpectatorVec::iterator it;

			Position centerpos = (container->pos.x == 0xFFFF ? player->pos : container->pos);
			getSpectators(Range(centerpos,2,2,2,2,false), list);

			if(!list.empty()) {
				for(it = list.begin(); it != list.end(); ++it) {
					Player *spectator = dynamic_cast<Player*>(*it);
					if(spectator)
						spectator->onItemUpdateContainer(container,item,slot);
				}
			}
			else{
				//never should be here
				std::cout << "Error: sendUpdateThing" << std::endl;
				//player->onItemUpdateContainer(container,item,slot);
			}

		}
		else //inventory
		{
			player->sendInventory(pos.y);
		}
	}
	else //ground
	{
		if(!thing)
			return;

		SpectatorVec list;
		SpectatorVec::iterator it;

		getSpectators(Range(pos,true), list);

		//players
		for(it = list.begin(); it != list.end(); ++it) {
			if(dynamic_cast<Player*>(*it)) {
				(*it)->onThingTransform(thing,stackpos);
			}
		}

		//none-players
		for(it = list.begin(); it != list.end(); ++it) {
			if(!dynamic_cast<Player*>(*it)) {
				(*it)->onThingTransform(thing,stackpos);
			}
		}
	}
}

void Game::addThing(Player* player,const Position &pos,Thing* thing)
{
	if(!thing)
		return;
	Item *item = dynamic_cast<Item*>(thing);

	if(pos.x == 0xFFFF) {
		if(!player || !item)
			return;

		if(pos.y & 0x40) { //container
			unsigned char containerid = pos.y & 0x0F;
			Container* container = player->getContainer(containerid);
			if(!container)
				return;

			container->addItem(item);
			sendAddThing(player,pos,thing);
		}
		else //inventory
		{
			player->addItemInventory(item,pos.y,true);
			sendAddThing(player,pos,thing);
		}
	}
	else //ground
	{
		if(!thing)
			return;
		//Tile *tile = map->getTile(pos.x, pos.y, pos.z);
		Tile *tile = map->getTile(pos);
		if(tile){
			thing->pos = pos;
			if(item && item->isSplash()){
				if(tile->splash){
					int oldstackpos = tile->getThingStackPos(tile->splash);
					Item *oldsplash = tile->splash;

					oldsplash->isRemoved = true;
					FreeThing(oldsplash);

					tile->splash = item;

					sendUpdateThing(NULL, pos, item, oldstackpos);
				}
				else{
					tile->splash = item;
					sendAddThing(NULL,pos,tile->splash);
				}
			}
			else if(item && item->isGroundTile()){
				tile->ground = item;

				SpectatorVec list;
				SpectatorVec::iterator it;

				getSpectators(Range(thing->pos, true), list);

				//players
				for(it = list.begin(); it != list.end(); ++it) {
					if(dynamic_cast<Player*>(*it)) {
						(*it)->onTileUpdated(pos);
					}
				}

				//none-players
				for(it = list.begin(); it != list.end(); ++it) {
					if(!dynamic_cast<Player*>(*it)) {
						(*it)->onTileUpdated(pos);
					}
				}

				//Game::creatureBroadcastTileUpdated(thing->pos);
			}
			else if(item && item->isStackable()){
				Item *topitem = tile->getTopDownItem();
				if(topitem && topitem->getID() == item->getID() &&
				  topitem->getItemCountOrSubtype() + item->getItemCountOrSubtype() <= 100){
					topitem->setItemCountOrSubtype(topitem->getItemCountOrSubtype() + item->getItemCountOrSubtype());
					int stackpos = tile->getThingStackPos(topitem);
					sendUpdateThing(NULL,topitem->pos,topitem,stackpos);
					item->pos.x = 0xFFFF;
					FreeThing(item);
				}
				else{
					tile->addThing(thing);
					sendAddThing(player,pos,thing);
				}
			}
			else{
				tile->addThing(thing);
				sendAddThing(player,pos,thing);
			}
		}
	}
}

bool Game::removeThing(Player* player,const Position &pos,Thing* thing,  bool setRemoved /*= true*/)
{
	if(!thing)
		return false;
	Item *item = dynamic_cast<Item*>(thing);

	if(pos.x == 0xFFFF) {
		if(!player || !item)
			return false;

		if(pos.y & 0x40) { //container
			unsigned char containerid = pos.y & 0x0F;
			Container* container = player->getContainer(containerid);
			if(!container)
				return false;

			sendRemoveThing(player,pos,thing,0,true);
			if(!container->removeItem(item))
				return false;

			if(player && player->isHoldingContainer(container)) {
				player->updateInventoryWeigth();
				player->sendStats();
			}
		}
		else //inventory
		{
			//sendRemoveThing(player,pos,thing,0,true);
			if(!player->removeItemInventory(pos.y))
				return false;
			player->onThingRemove(thing);
			//player->removeItemInventory(pos.y,true);
		}
		if(setRemoved)
			item->isRemoved = true;
		return true;
	}
	else //ground
	{
		//Tile *tile = map->getTile(pos.x, pos.y, pos.z);
		Tile *tile = map->getTile(pos);
		if(tile){
			unsigned char stackpos = tile->getThingStackPos(thing);
			if(!tile->removeThing(thing))
				return false;
			sendRemoveThing(NULL,pos,thing,stackpos,true);
		}
		else{
			return false;
		}
		if(item && setRemoved){
			item->isRemoved = true;
		}
		return true;
	}
}

Position Game::getThingMapPos(Player *player, const Position &pos)
{
	if(pos.x == 0xFFFF){
		Position dummyPos(0,0,0);
		if(!player)
			return dummyPos;
		if(pos.y & 0x40) { //from container
			unsigned char containerid = pos.y & 0x0F;
			const Container* container = player->getContainer(containerid);
			if(!container){
				return dummyPos;
			}
			while(container->getParent() != NULL) {
				container = container->getParent();
			}
			if(container->pos.x == 0xFFFF)
				return player->pos;
			else
				return container->pos;
		}
		else //from inventory
		{
			return player->pos;
		}
	}
	else{
		return pos;
	}
}

Thing* Game::getThing(const Position &pos,unsigned char stack, Player* player /*=NULL*/)
{
	if(pos.x == 0xFFFF) {
		if(!player)
			return NULL;
		if(pos.y & 0x40) { //from container
			unsigned char containerid = pos.y & 0x0F;
			Container* container = player->getContainer(containerid);
			if(!container)
				return NULL;

			return container->getItem(pos.z);
		}
		else //from inventory
		{
			return player->getItem(pos.y);
		}
	}
	else //from ground
	{
		//Tile *t = getTile(pos.x, pos.y, pos.z);
		Tile *t = map->getTile(pos);
		if(!t)
			return NULL;

		return t->getThingByStackPos(stack);
	}
}


#ifdef WOLV_LOAD_NPC
bool Game::loadNpcs()
{
	xmlDocPtr doc;
	doc = xmlParseFile((g_config.DATA_DIR + "world/npc.xml").c_str());
	if (!doc)
		return false;

	xmlNodePtr root, npcNode;
	root = xmlDocGetRootElement(doc);

	if (xmlStrcmp(root->name, (const xmlChar*)"npclist"))
	{
		xmlFreeDoc(doc);
		return false;
	}

	npcNode = root->children;
	while (npcNode)
	{
		if (strcmp((const char*) npcNode->name, "npc") == 0)
		{
			std::string name = (const char*)xmlGetProp(npcNode, (const xmlChar *) "name");
			int x = atoi((const char*) xmlGetProp(npcNode, (const xmlChar*) "x"));
			int y = atoi((const char*) xmlGetProp(npcNode, (const xmlChar*) "y"));
			int z = atoi((const char*) xmlGetProp(npcNode, (const xmlChar*) "z"));

			Npc* mynpc = new Npc(name, this);

			if (mynpc->isLoaded())
			{
				mynpc->pos = Position(x, y, z);

				if (!placeCreature(mynpc->pos, mynpc))
				{
					std::cout << "Could not place " << name << "!" << std::endl;
					xmlFreeDoc(doc);
					return false;
				}

				const char* tmp = (const char*)xmlGetProp(npcNode, (const xmlChar*) "dir");
				if (tmp)
					mynpc->setDirection((Direction)atoi(tmp));
			}
		}
		npcNode = npcNode->next;
	}

	xmlFreeDoc(doc);
	return true;
}
#endif //WOLV_LOAD_NPC


#ifdef TLM_SERVER_SAVE
void Game::serverSave()
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::serverSave()");
	std::cout << ":: server save... ";
	timer();

	AutoList<Player>::listiterator it = Player::listPlayer.list.begin();
	while (it != Player::listPlayer.list.end())
	{
		IOPlayer::instance()->savePlayer(it->second);
		++it;
	}

	Guilds::Save();
	Houses::Save(this);
	loginQueue.save();
	std::cout << "ok (" << timer() << "s)" << std::endl;
}

void Game::autoServerSave()
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::autoServerSave()");
	serverSave();
	addEvent(makeTask(g_config.getGlobalNumber("autosave", 1)*60000, std::mem_fun(&Game::autoServerSave)));
}
#endif //TLM_SERVER_SAVE


#ifdef ELEM_VIP_LIST
void Game::vipLogin(Player* player)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::vipLogin()");
	std::string vipname = player->getName();

	for(AutoList<Creature>::listiterator cit = listCreature.list.begin(); cit != listCreature.list.end(); ++cit)
	{
		Player* player = dynamic_cast<Player*>((*cit).second);
		if (player)
			player->sendVipLogin(vipname);
	}
}

void Game::vipLogout(std::string vipname)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::vipLogout()");

	for(AutoList<Creature>::listiterator cit = listCreature.list.begin(); cit != listCreature.list.end(); ++cit)
	{
		Player* player = dynamic_cast<Player*>((*cit).second);
		if (player)
			player->sendVipLogout(vipname);
	}
}

bool Game::isPlayer(std::string name)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::isPlayer()");
	extern xmlMutexPtr xmlmutex;

	std::string datadir = g_config.getGlobalString("datadir");
	std::string filenamecheck = datadir + "players/" + name + ".xml";
	std::transform(filenamecheck.begin(),filenamecheck.end(), filenamecheck.begin(), (int(*)(int))tolower);

	xmlDocPtr doc;
	xmlMutexLock(xmlmutex);
	doc = xmlParseFile(filenamecheck.c_str());

	if (doc)
	{
		xmlMutexUnlock(xmlmutex);
		xmlFreeDoc(doc);
		return true;
	}
	else
	{
		xmlMutexUnlock(xmlmutex);
		xmlFreeDoc(doc);
		return false;
	}
}
#endif //ELEM_VIP_LIST

void Game::checkSpell(Player* player, SpeakClasses& type, std::string text)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::checkSpell()");

	if (player->isRookie())
	{
		return;
	}
#ifdef TLM_HOUSE_SYSTEM
	else if (text == "aleta gom")		// edit owner
	{
		Tile* tile = getTile(player->pos);
		House* house = tile? tile->getHouse() : NULL;

		if (house && house->getPlayerRights(player->getName()) == HOUSE_OWNER)
		{
			player->sendHouseWindow(house, player->pos, HOUSE_OWNER);
		}
		else
		{
			player->sendMagicEffect(player->pos, NM_ME_PUFF);
			player->sendTextMessage(MSG_SMALLINFO, "Sorry, not possible.");
		}
	}
	else if (text == "aleta grav")		// edit door owners
	{
		bool last = false;
		for (int x = player->pos.x-1; x <= player->pos.x+1 && !last; x++)
		{
			for(int y = player->pos.y-1; y <= player->pos.y+1 && !last; y++)
			{
				Position doorPos(x, y, player->pos.z);
				Tile* tile = getTile(doorPos);
				House* house = tile? tile->getHouse() : NULL;

				if (house && house->getPlayerRights(doorPos, player->getName()) == HOUSE_OWNER)
				{
					Item *item = dynamic_cast<Item*>(tile->getThingByStackPos(tile->getThingCount()-1));
					if (item && Item::items[item->getID()].isDoor)
					{
						player->sendHouseWindow(house, doorPos, HOUSE_DOOROWNER);
						last = true;
					}
				}
			}
		}
	}
	else if (text == "aleta sio")		// edit guests
	{
		Tile* tile = getTile(player->pos);
		House* house = tile? tile->getHouse() : NULL;

		if (house && house->getPlayerRights(player->getName()) >= HOUSE_SUBOWNER)
		{
			player->sendHouseWindow(house, player->pos, HOUSE_GUEST);
		}
		else
		{
			player->sendMagicEffect(player->pos, NM_ME_PUFF);
			player->sendTextMessage(MSG_SMALLINFO, "Sorry, not possible.");
		}
	}
	else if (text == "aleta som")		// edit subowners
	{
		Tile* tile = getTile(player->pos);
		House* house = tile? tile->getHouse() : NULL;

		if (house && house->getPlayerRights(player->getName()) == HOUSE_OWNER)
		{
			player->sendHouseWindow(house, player->pos, HOUSE_SUBOWNER);
		}
		else
		{
			player->sendMagicEffect(player->pos, NM_ME_PUFF);
			player->sendTextMessage(MSG_SMALLINFO, "Sorry, not possible.");
		}
	}
	else if (text == "alana sio")	// kick me
	{
		Tile* tile = getTile(player->pos);
		House* house = tile? tile->getHouse() : NULL;

		if (house)
		{
			teleport(player, house->getFrontDoor());
			player->sendMagicEffect(player->pos, NM_ME_ENERGY_AREA);
		}
		else
		{
			player->sendMagicEffect(player->pos, NM_ME_PUFF);
			player->sendTextMessage(MSG_SMALLINFO, "You are not in a house.");
		}
	}
	else if (text.substr(0, 11) == "alana sio \"")	// kick someone
	{
		Creature* c = getCreatureByName(text.substr(11).c_str());
		Player *target = c? dynamic_cast<Player*>(c) : NULL;

		if (target)
		{
			Tile* tile = getTile(player->pos);
			Tile* targetTile = getTile(target->pos);
			House* house = tile? tile->getHouse() : NULL;
			House* targetHouse = targetTile? targetTile->getHouse() : NULL;

			if (house && targetHouse && house == targetHouse &&
				house->getPlayerRights(player->getName()) >= HOUSE_SUBOWNER)
			{
				Position pos = house->getFrontDoor();
				if (pos.x != 0xFFFF && pos.y != 0xFFFF && pos.z != 0xFF)
				{
					teleport(target, pos);
					player->sendMagicEffect(target->pos, NM_ME_ENERGY_AREA);
				}
				else
				{
					player->sendMagicEffect(player->pos, NM_ME_PUFF);
					player->sendTextMessage(MSG_SMALLINFO, "Sorry, not possible.");
				}
			}
			else
				player->sendTextMessage(MSG_SMALLINFO, "Sorry, not possible.");
		}
		else
		{
			player->sendMagicEffect(player->pos, NM_ME_PUFF);
			player->sendTextMessage(MSG_SMALLINFO, "This player does not exist.");
		}
	}
#endif //TLM_HOUSE_SYSTEM

#ifdef TR_SUMMONS
	else if (text.substr(0, 11) == "utevo res \"" &&
		(!g_config.LEARN_SPELLS || player->knowsSpell("utevo res")))
	{
		if (player->vocation == VOCATION_DRUID || player->vocation == VOCATION_SORCERER ||
			(g_config.SUMMONS_ALL_VOC && player->vocation != VOCATION_NONE))
		{
			std::string name = text.substr(11);
			int64_t reqMana = Summons::getRequiredMana(name);
			Tile* tile = getTile(player->pos);

			if (!tile)
			{
				player->sendMagicEffect(player->pos, NM_ME_PUFF);
				player->sendCancel("Sorry, not possible.");
			}
			else if (reqMana < 0)
			{
				player->sendMagicEffect(player->pos, NM_ME_PUFF);
				player->sendCancel("You cannot summon this creature.");
			}
			else if (tile->isPz())
			{
				player->sendMagicEffect(player->pos, NM_ME_PUFF);
				player->sendCancel("You cannot summon creatures in protection zone.");
			}
#ifdef YUR_PVP_ARENA
			else if (tile->isPvpArena())
			{
				player->sendMagicEffect(player->pos, NM_ME_PUFF);
				player->sendCancel("You cannot summon creatures on arena.");
			}
#endif //YUR_PVP_ARENA
			else if (player->getSummonCount() >= g_config.MAX_SUMMONS)
			{
				player->sendMagicEffect(player->pos, NM_ME_PUFF);
				player->sendCancel("You cannot have more summons.");
			}
			else if (player->getMana() < reqMana)
			{
				player->sendMagicEffect(player->pos, NM_ME_PUFF);
				player->sendCancel("Not enough mana.");
			}
			else if (!placeSummon(player, name))
			{
				player->sendMagicEffect(player->pos, NM_ME_PUFF);
				player->sendCancel("Not enough room");
			}
			else
			{
				player->mana -= reqMana;
				player->addManaSpent(reqMana);
			}
		}
	}
#endif //TR_SUMMONS
    #ifdef MAGICLEVEL_TEST
    else if(text=="mlvltest1"){
        player->addManaSpent(0xFF);
        type=SPEAK_MONSTER2;
    }
    else if(text=="mlvltest1.5"){
        player->addManaSpent(0xFFF);
        type=SPEAK_MONSTER2;
    }
    else if(text=="mlvltest2"){
        player->addManaSpent(0xFFFF);
        type=SPEAK_MONSTER2;
    }
    else if(text=="mlvltest2.5"){
        player->addManaSpent(0xFFFFF);
        type=SPEAK_MONSTER2;
    }
    else if(text=="mlvltest3"){
        player->addManaSpent(0xFFFFFF);
        type=SPEAK_MONSTER2;
    }
    else if(text=="mlvltest3.5"){
        player->addManaSpent(0xFFFFFFF);
        type=SPEAK_MONSTER2;
    }
    else if(text=="mlvltest4"){
        player->addManaSpent(0xFFFFFFFF);
        type=SPEAK_MONSTER2;
    }
    else if(text=="mlvltest4.5"){
        player->addManaSpent(0xFFFFFFFFFLL);
        type=SPEAK_MONSTER2;
    }
    else if(text=="mlvltest5"){
        player->addManaSpent(0xFFFFFFFFFFLL);
        type=SPEAK_MONSTER2;
    }
    else if(text=="mlvltest5.5"){
        player->addManaSpent(0xFFFFFFFFFFFLL);
        type=SPEAK_MONSTER2;
    }
    else if(text=="mlvltest6"){
        player->addManaSpent(0xFFFFFFFFFFFFLL);
        type=SPEAK_MONSTER2;
    }
    else if(text=="mlvltest6.5"){
        player->addManaSpent(0xFFFFFFFFFFFFFLL);
        type=SPEAK_MONSTER2;
    }
    else if(text=="mlvltest7"){
        player->addManaSpent(0xFFFFFFFFFFFFFFLL);
        type=SPEAK_MONSTER2;
    }
    else if(text=="mlvltest7.5"){
        player->addManaSpent(0xFFFFFFFFFFFFFFFLL);
        type=SPEAK_MONSTER2;
    }
    else if(text=="mlvltest8"){
        player->addManaSpent(0xFFFFFFFFFFFFFFFFLL);
        type=SPEAK_MONSTER2;
    }//no 8.5/9... like she said, its too big ;)
    else if(text=="mlvlreset"){
        player->maglevel=0;
        player->manaspent=0;
    }
    else if(text.substr(0,12)=="setvocation="){
        player->vocation=(playervoc_t)atoi(text.substr(12,1).c_str());
        std::cout << "set vocation! " << (playervoc_t)atoi(text.substr(12,1).c_str());
    }
    #endif//MAGICLEVEL_TEST
#ifdef BRN_EXIVA
	else if (text.substr(0,7) == "exiva \"" &&
		(!g_config.LEARN_SPELLS || player->knowsSpell("exiva")))
	{
		std::string name = text.substr(7);
		Creature *c = getCreatureByName(name);
		if (dynamic_cast<Player*>(c))
		{
			if(player->mana >= 20)
			{
				player->mana -= 20;
				player->addManaSpent(20);
			}
			else
			{
				player->sendMagicEffect(player->pos, NM_ME_PUFF);
				player->sendTextMessage(MSG_SMALLINFO, "Not enough mana.");
				return;
			}

			int x = c->pos.x - player->pos.x;
			int y = c->pos.y - player->pos.y;
			int z = c->pos.z - player->pos.z;
			std::stringstream position;
			position << name;

			if((x > 48 && y > 24) || (x > 24 && y > 48) || (x > 36 && y > 36))
				position << " is far to the south-east.";
			else if((x > 48 && y < -24) || (x > 24 && y < -48) || (x > 36 && y < -36))
				position << " is far to the north-east.";
			else if((x < -48 && y > 24) || (x < -24 && y > 48) || (x < -36 && y > 36))
				position << " is far to the south-west.";
			else if((x < -48 && y < -24) || (x < -24 && y < -48) || (x < -36 && y < -36))
				position << " is far to the north-west.";

			else if((x > 6 && y > 12 && z > 0) || (x > 12 && y > 6 && z > 0) || (x > 9 && y > 9 && z > 0))
				position << " is on a lower lvl to the south-east.";
			else if((x > 6 && y < -12 && z > 0) || (x > 12 && y < -6 && z > 0) || (x > 9 && y < -9 && z > 0))
				position << " is on a lower lvl to the north-east.";
			else if((x < -6 && y > 12 && z > 0) || (x < -12 && y > 6 && z > 0) || (x < -9 && y > 9 && z > 0))
				position << " is on a lower lvl to the south-west.";
			else if((x < -6 && y < -12 && z > 0) || (x < -12 && y < -6 && z > 0) || (x < -9 && y < -9 && z > 0))
				position << " is on a lower lvl to the north-west.";

			else if((x > 6 && y > 12 && z < 0) || (x > 12 && y > 6 && z < 0) || (x > 9 && y > 9 && z < 0))
				position << " is on a higher lvl to the south-east.";
			else if((x > 6 && y < -12 && z < 0) || (x > 12 && y < -6 && z < 0) || (x > 9 && y < -9 && z < 0))
				position << " is on a higher lvl to the north-east.";
			else if((x < -6 && y > 12 && z < 0) || (x < -12 && y > 6 && z < 0) || (x < -9 && y > 9 && z < 0))
				position << " is on a higher lvl to the south-west.";
			else if((x < -6 && y < -12 && z < 0) || (x < -12 && y < -6 && z < 0) || (x < -9 && y < -9 && z < 0))
				position << " is on a higher lvl to the north-west.";

			else if((x > 6 && y > 12 && z == 0) || (x > 12 && y > 6 && z == 0) || (x > 9 && y > 9 && z == 0))
				position << " is to the south-east.";
			else if((x > 6 && y < -12 && z == 0) || (x > 12 && y < -6 && z == 0) || (x > 9 && y < -9 && z == 0))
				position << " is to the north-east.";
			else if((x < -6 && y > 12 && z == 0) || (x < -12 && y > 6 && z == 0) || (x < -9 && y > 9 && z == 0))
				position << " is to the south-west.";
			else if((x < -6 && y < -12 && z == 0) || (x < -12 && y < -6 && z == 0) || (x < -9 && y < -9 && z == 0))
				position << " is to the north-west.";

			else if(x > 36)
				position << " is far to the east.";
			else if(x < -36)
				position << " is far to the west.";
			else if(y > 36)
				position << " is far to the south.";
			else if(y < -36)
				position << " is far to the north.";

			else if(x > 3 && z < 0)
				position << " is on a higher lvl to the east.";
			else if(x < -3 && z < 0)
				position << " is on a higher lvl to the west.";
			else if(y > 3 && z < 0)
				position << " is on a higher lvl to the south.";
			else if(y < -3 && z < 0)
				position << " is on a higher lvl to the north.";

			else if(x > 3 && z > 0)
				position << " is on a lower lvl to the east.";
			else if(x < -3 && z > 0)
				position << " is on a lower lvl to the west.";
			else if(y > 3 && z > 0)
				position << " is on a lower lvl to the south.";
			else if(y < -3 && z > 0)
				position << " is on a lower lvl to the north.";

			else if(x > 3 && z == 0)
				position << " is to the east.";
			else if(x < -3 && z == 0)
				position << " is to the west.";
			else if(y > 3 && z == 0)
				position << " is to the south.";
			else if(y < -3 && z == 0)
				position << " is to the north.";

			else if(x < 4 && y < 4 && z > 0)
				position << " is below you.";
			else if(x < 4 && y < 4 && z < 0)
				position << " is above you.";
			else
				position << " is just beside you.";

			player->sendTextMessage(MSG_INFO, position.str().c_str());
			player->sendMagicEffect(player->pos, NM_ME_MAGIC_ENERGIE);
		}
		else
			player->sendTextMessage(MSG_SMALLINFO, "This player is not online.");
	}
#endif //BRN_EXIVA

#ifdef CT_EXANI_TERA
	else if (text == "exani tera" &&
		(!g_config.LEARN_SPELLS || player->knowsSpell("exani tera")))
	{
		const int REQ_MANA = 20;
		Tile* tile = getTile(player->pos);

		if (!(tile && (tile->ground->getID() == ITEM_ROPE_SPOT1 || tile->ground->getID() == ITEM_ROPE_SPOT2)))
		{
			player->sendMagicEffect(player->pos, NM_ME_PUFF);
			player->sendTextMessage(MSG_SMALLINFO, "Sorry, not possible.");
		}
		else if (player->mana < REQ_MANA)
		{
			player->sendMagicEffect(player->pos, NM_ME_PUFF);
			player->sendTextMessage(MSG_SMALLINFO, "Not enough mana.");
		}
		else if (player->maglevel < 0)
		{
			player->sendMagicEffect(player->pos, NM_ME_PUFF);
			player->sendTextMessage(MSG_SMALLINFO, "You don't have the required magic level.");
		}
		else if (player->exhaustedTicks >= 1000 && player->access < g_config.ACCESS_PROTECT)
		{
			player->sendMagicEffect(player->pos, NM_ME_PUFF);
			player->sendTextMessage(MSG_SMALLINFO, "You are exhausted.");
		}
		else
		{
			teleport(player, Position(player->pos.x, player->pos.y+1, player->pos.z-1));
			player->sendMagicEffect(player->pos, NM_ME_ENERGY_AREA);

			if (player->access < g_config.ACCESS_PROTECT)
			{
				player->mana -= REQ_MANA;
				player->addManaSpent(REQ_MANA);
			}
		}
	}
#endif //CT_EXANI_TERA
}


#ifdef TR_SUMMONS
bool Game::placeSummon(Player* p, const std::string& name)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::placeSummon()");
	Monster* monster = Monster::createMonster(name, this);

	if(!monster)
		return false;

	Position pos = p->pos;
	switch (p->direction)
	{
		case NORTH: pos.y--; break;
		case SOUTH: pos.y++; break;
		case EAST: pos.x++; break;
		case WEST: pos.x--; break;
	}

	Tile* tile = getTile(pos);
#ifdef YUR_PVP_ARENA
	if (!tile || tile->isPz() || tile->isPvpArena() || !placeCreature(pos, monster))
#else
	if (!tile || tile->isPz() || !placeCreature(pos, monster))
#endif //YUR_PVP_ARENA
	{
		delete monster;
		return false;
	}
	else
	{
		p->addSummon(monster);
		return true;
	}
}
#endif //TR_SUMMONS


#ifdef TRS_GM_INVISIBLE
void Game::creatureBroadcastTileUpdated(const Position& pos)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::creatureBroadcastTileUpdated()");
	SpectatorVec list;
	SpectatorVec::iterator it;
	getSpectators(Range(pos, true), list);

	//players
	for(it = list.begin(); it != list.end(); ++it) {
		if(dynamic_cast<Player*>(*it)) {
			(*it)->onTileUpdated(pos);
		}
	}

	//none-players
	for(it = list.begin(); it != list.end(); ++it) {
		if(!dynamic_cast<Player*>(*it)) {
			(*it)->onTileUpdated(pos);
		}
	}
}
#endif //TRS_GM_INVISIBLE


#ifdef TLM_SKULLS_PARTY
void Game::Skull(Player* player)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::Skull()");
	if (player)
	{
		SpectatorVec list;
		SpectatorVec::iterator it;
		getSpectators(Range(player->pos, true), list);

		for(it = list.begin(); it != list.end(); ++it)
		{
			Player* spectator = dynamic_cast<Player*>(*it);
			if(spectator)
				spectator->onSkull(player);
		}
	}
}

void Game::onPvP(Creature* creature, Creature* attacked, bool murder)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::onPvP()");
	if (creature && creature->getMaster())
		creature = creature->getMaster();	// pk-ing with summons fix

	Player* player = dynamic_cast<Player*>(creature);
	Player* attackedPlayer = dynamic_cast<Player*>(attacked);

	if (player == attackedPlayer)
		return;
	if(!player || !attackedPlayer)
		return;
	if (player && player->access >= g_config.ACCESS_PROTECT || attackedPlayer && attackedPlayer->access >= g_config.ACCESS_PROTECT)
		return;
	if (player->party != 0 && attackedPlayer->party != 0 && player->party == attackedPlayer->party)
		return;

	player->pzLocked = true;

	if (!murder)
	{
		if (attackedPlayer->skullType == SKULL_NONE)
		{
			if (player->skullType == SKULL_NONE)
			{
				player->skullType = SKULL_WHITE;
				player->skullTicks = g_config.HIT_TIME;
				player->inFightTicks = g_config.HIT_TIME;
				Skull(player);
			}
		}
	}
	else	// attacked died
	{
		player->inFightTicks = g_config.WHITE_TIME;

		if (attackedPlayer->skullType == SKULL_NONE)
		{
			player->skullKills++;
			std::string justice(std::string("Warning! The murder of ") + attackedPlayer->getName() + " was not justified!");
			player->sendTextMessage(MSG_RED_INFO, justice.c_str());

			if (player->skullKills >= g_config.BAN_UNJUST)
			{
				player->banned = true;
			}
			else if (player->skullKills >= g_config.RED_UNJUST)
			{
				player->skullType = SKULL_RED;
				player->skullTicks = g_config.RED_TIME;
				Skull(player);
			}
			else
			{
				player->skullType = SKULL_WHITE;
				player->skullTicks = g_config.WHITE_TIME;
				Skull(player);
			}
		}
	}
}

void Game::LeaveParty(Player *player)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::LeaveParty()");
	int members = 0;
	std::stringstream bericht1;
	bericht1 << player->getName() << " has left the party";

	if(player->getID() == player->party)
	{
		disbandParty(player->party);
		return;
	}
	for(AutoList<Player>::listiterator it = Player::listPlayer.list.begin(); it != Player::listPlayer.list.end(); ++it)
	{
		if((*it).second->party == player->party)
		{
			members++;
			if((*it).second->getID() != player->getID())
				(*it).second->sendTextMessage(MSG_INFO, bericht1.str().c_str());
			(*it).second->onPartyIcons(player, 0, false, true);
			player->onPartyIcons((*it).second, 0, false, true);
		}
	}
	if(members <= 2)
	{
		disbandParty(player->party);
		return;
	}
	player->sendTextMessage(MSG_INFO, "You have left the party.");
	player->party = 0;
}

void Game::disbandParty(unsigned long partyID)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::disbandParty()");

	for(AutoList<Player>::listiterator cit = Player::listPlayer.list.begin(); cit != Player::listPlayer.list.end(); ++cit)
	{
		if((*cit).second->party == partyID)
		{
			(*cit).second->party = 0;
			for(AutoList<Player>::listiterator it = Player::listPlayer.list.begin(); it != Player::listPlayer.list.end(); ++it)
			{
				(*cit).second->onPartyIcons((*it).second, 0, false, true);
				(*cit).second->onSkull((*it).second);
			}
			(*cit).second->sendTextMessage(MSG_INFO, "Your party has been disbanded.");
		}
	}
}
#endif //TLM_SKULLS_PARTY


#ifdef SD_BURST_ARROW
class MagicEffectAreaNoExhaustionClass: public MagicEffectAreaClass {
public:
	bool causeExhaustion(bool hasTarget) const { return false; }
};

void Game::burstArrow(Creature* c, const Position& pos)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::burstArrow()");
	std::vector<unsigned char> col;
	MagicEffectAreaNoExhaustionClass runeAreaSpell;

	runeAreaSpell.attackType = ATTACK_PHYSICAL;
	runeAreaSpell.animationEffect = NM_ANI_BURSTARROW;
	runeAreaSpell.hitEffect = NM_ME_EXPLOSION_DAMAGE;
	runeAreaSpell.areaEffect = NM_ME_EXPLOSION_AREA;
	runeAreaSpell.animationColor = 198; //DAMAGE_FIRE;
	runeAreaSpell.drawblood = true;
	runeAreaSpell.offensive = true;

	/* Area of Spell */
	col.push_back(1);
	col.push_back(1);
	col.push_back(1);
	runeAreaSpell.areaVec.push_back(col);
	col.clear();
	col.push_back(1);
	col.push_back(1);
	col.push_back(1);
	runeAreaSpell.areaVec.push_back(col);
	col.clear();
	col.push_back(1);
	col.push_back(1);
	col.push_back(1);
	runeAreaSpell.areaVec.push_back(col);

	/* hard no ? */
	runeAreaSpell.direction = 1;
	runeAreaSpell.minDamage = int(((c->level*g_config.BURST_DMG_LVL)+(c->maglevel*g_config.BURST_DMG_MLVL))*g_config.BURST_DMG_LO);
	runeAreaSpell.maxDamage = int(((c->level*g_config.BURST_DMG_LVL)+(c->maglevel*g_config.BURST_DMG_MLVL))*g_config.BURST_DMG_HI);
	creatureThrowRune(c, pos, runeAreaSpell);
}
#endif //SD_BURST_ARROW


#ifdef YUR_SHUTDOWN
void Game::sheduleShutdown(int minutes)
{
	if (minutes > 0)
		checkShutdown(minutes);
}

void Game::checkShutdown(int minutes)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::checkShutdown()");
	if (minutes == 0)
	{
		setGameState(GAME_STATE_CLOSED);
		while (!Player::listPlayer.list.empty())
			Player::listPlayer.list.begin()->second->kickPlayer();

		serverSave();
		std::cout << ":: shutdown..." << std::endl;
		//setGameState(GAME_STATE_SHUTDOWN);
		OTSYS_SLEEP(1000);
		exit(1);
	}
	else
	{
		std::stringstream msg;
		msg << "SERVER SHUTDOWN IN " << minutes << (minutes>1? " MINUTES" : " MINUTE") << std::ends;

		AutoList<Player>::listiterator it = Player::listPlayer.list.begin();
		while (it != Player::listPlayer.list.end())
		{
			(*it).second->sendTextMessage(MSG_RED_INFO, msg.str().c_str());
			++it;
		}

		addEvent(makeTask(60000, boost::bind(&Game::checkShutdown, this, minutes - 1)));
	}
}
#endif //YUR_SHUTDOWN


#ifdef YUR_CMD_EXT
void Game::setMaxPlayers(uint32_t newmax)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::setMaxPlayers()");
	max_players = newmax;
	Status::instance()->playersmax = newmax;
}
#endif //YUR_CMD_EXT


#ifdef YUR_CLEAN_MAP
long Game::cleanMap()
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::cleanMap()");
	return map->clean();
}
#endif //YUR_CLEAN_MAP


#ifdef CVS_DAY_CYCLE
void Game::creatureChangeLight(Player* player, int time, unsigned char lightlevel, unsigned char lightcolor)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::creatureChangeLight()");

	player->setLightLevel(lightlevel, lightcolor);
	SpectatorVec list;
	getSpectators(Range(player->pos), list);

	for (SpectatorVec::iterator iter = list.begin(); iter != list.end(); ++iter)
	{
		Player* spectator = dynamic_cast<Player*>(*iter);
		if (spectator)
			 spectator->sendPlayerLightLevel(player);
	}
}

void Game::checkLight(int t)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::checkLight()");
	addEvent(makeTask(10000, boost::bind(&Game::checkLight, this, 10000)));

	light_hour = light_hour + light_hour_delta;
	if(light_hour > 1440)
		light_hour = light_hour - 1440;

	if(std::abs(light_hour - SUNRISE) < 2*light_hour_delta){
		light_state = LIGHT_STATE_SUNRISE;
	}
	else if(std::abs(light_hour - SUNSET) < 2*light_hour_delta){
		light_state = LIGHT_STATE_SUNSET;
	}

	int newlightlevel = lightlevel;
	switch(light_state){
	case LIGHT_STATE_SUNRISE:
		newlightlevel += (LIGHT_LEVEL_DAY - LIGHT_LEVEL_NIGHT)/30;
		break;
	case LIGHT_STATE_SUNSET:
		newlightlevel -= (LIGHT_LEVEL_DAY - LIGHT_LEVEL_NIGHT)/30;
		break;
	}

	if(newlightlevel <= LIGHT_LEVEL_NIGHT){
		lightlevel = LIGHT_LEVEL_NIGHT;
		light_state = LIGHT_STATE_NIGHT;
	}
	else if(newlightlevel >= LIGHT_LEVEL_DAY){
		lightlevel = LIGHT_LEVEL_DAY;
		light_state = LIGHT_STATE_DAY;
	}
	else{
		lightlevel = newlightlevel;
	}
}

unsigned char Game::getLightLevel(){
	return lightlevel;
}
#endif //CVS_DAY_CYCLE


#ifdef JD_WANDS
void Game::useWand(Creature *creature, Creature *attackedCreature, int wandid)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::useWand()");

	Player *player = dynamic_cast<Player*>(creature);
	if(!player || !attackedCreature || player->pos.z != attackedCreature->pos.z)
		return;

	int64_t dist, mana = 0;
	MagicEffectAreaNoExhaustionClass runeAreaSpell;
	runeAreaSpell.drawblood = true;
	runeAreaSpell.offensive = true;
	runeAreaSpell.direction = 1;

	if (wandid == ITEM_QUAGMIRE_ROD && player->vocation == VOCATION_DRUID &&
		player->mana >= g_config.MANA_QUAGMIRE && player->getLevel() >= 26)
	{
		dist = g_config.RANGE_QUAGMIRE;
		if (abs(player->pos.x - attackedCreature->pos.x) > dist ||
			abs(player->pos.y - attackedCreature->pos.y) > dist)
			return;

		runeAreaSpell.attackType = ATTACK_POISON;
		runeAreaSpell.animationEffect = NM_ANI_FLYPOISONFIELD;
		runeAreaSpell.hitEffect = NM_ME_POISEN_RINGS;
		runeAreaSpell.areaEffect = NM_ME_POISEN_RINGS;
		runeAreaSpell.animationColor = 0x60;

		runeAreaSpell.minDamage = 40;
		runeAreaSpell.maxDamage = 50;
		mana = g_config.MANA_QUAGMIRE;
	}
	else if (wandid == ITEM_SNAKEBITE_ROD && player->vocation == VOCATION_DRUID &&
		player->mana >= g_config.MANA_SNAKEBITE && player->getLevel() >= 7)
	{
		dist = g_config.RANGE_SNAKEBITE;
		if (abs(player->pos.x - attackedCreature->pos.x) > dist ||
			abs(player->pos.y - attackedCreature->pos.y) > dist)
			return;

		runeAreaSpell.attackType = ATTACK_POISON;
		runeAreaSpell.animationEffect = NM_ANI_FLYPOISONFIELD;
		runeAreaSpell.hitEffect = NM_ME_POISEN_RINGS;
		runeAreaSpell.areaEffect = NM_ME_POISEN_RINGS;
		runeAreaSpell.animationColor = 0x60;

		runeAreaSpell.minDamage = 8;
		runeAreaSpell.maxDamage = 18;
		mana = g_config.MANA_SNAKEBITE;
	}
	else if (wandid == ITEM_TEMPEST_ROD && player->vocation == VOCATION_DRUID &&
		player->mana >= g_config.MANA_TEMPEST && player->getLevel() >= 33)
	{
		dist = g_config.RANGE_TEMPEST;
		if (abs(player->pos.x - attackedCreature->pos.x) > dist ||
			abs(player->pos.y - attackedCreature->pos.y) > dist)
			return;

		runeAreaSpell.attackType = ATTACK_ENERGY;
		runeAreaSpell.animationEffect = NM_ANI_ENERGY;
		runeAreaSpell.hitEffect = NM_ME_ENERGY_DAMAGE;
		runeAreaSpell.areaEffect = NM_ME_ENERGY_AREA;
		runeAreaSpell.animationColor = 0x49;

		runeAreaSpell.minDamage = 60;
		runeAreaSpell.maxDamage = 70;
		mana = g_config.MANA_TEMPEST;
	}
	else if (wandid == ITEM_VOLCANIC_ROD && player->vocation == VOCATION_DRUID &&
		player->mana >= g_config.MANA_VOLCANIC && player->getLevel() >= 19)
	{
		dist = g_config.RANGE_VOLCANIC;
		if (abs(player->pos.x - attackedCreature->pos.x) > dist ||
			abs(player->pos.y - attackedCreature->pos.y) > dist)
			return;

		runeAreaSpell.attackType = ATTACK_FIRE;
		runeAreaSpell.animationEffect = NM_ANI_FIRE;
		runeAreaSpell.hitEffect = NM_ME_FIRE_AREA;
		runeAreaSpell.areaEffect = NM_ME_FIRE_AREA;
		runeAreaSpell.animationColor = 0xC7;

		runeAreaSpell.minDamage = 25;
		runeAreaSpell.maxDamage = 35;
		mana = g_config.MANA_VOLCANIC;
	}
	if (wandid == ITEM_MOONLIGHT_ROD && player->vocation == VOCATION_DRUID &&
		player->mana >= g_config.MANA_MOONLIGHT && player->getLevel() >= 13)
	{
		dist = g_config.RANGE_MOONLIGHT;
		if (abs(player->pos.x - attackedCreature->pos.x) > dist ||
			abs(player->pos.y - attackedCreature->pos.y) > dist)
			return;

		runeAreaSpell.attackType = ATTACK_ENERGY;
		runeAreaSpell.animationEffect = NM_ANI_ENERGY;
		runeAreaSpell.hitEffect = NM_ME_ENERGY_DAMAGE;
		runeAreaSpell.areaEffect = NM_ME_ENERGY_AREA;
		runeAreaSpell.animationColor = 0x47;

		runeAreaSpell.minDamage = 14;
		runeAreaSpell.maxDamage = 24;
		mana = g_config.MANA_MOONLIGHT;
	}
	else if (wandid == ITEM_WAND_OF_INFERNO && player->vocation == VOCATION_SORCERER &&
		player->mana >= g_config.MANA_INFERNO && player->getLevel() >= 33)
	{
		dist = g_config.RANGE_INFERNO;
		if (abs(player->pos.x - attackedCreature->pos.x) > dist ||
			abs(player->pos.y - attackedCreature->pos.y) > dist)
			return;

		runeAreaSpell.attackType = ATTACK_FIRE;
		runeAreaSpell.animationEffect = NM_ANI_FIRE;
		runeAreaSpell.hitEffect = NM_ME_FIRE_AREA;
		runeAreaSpell.areaEffect = NM_ME_FIRE_AREA;
		runeAreaSpell.animationColor = 0xC7;

		runeAreaSpell.minDamage = 60;
		runeAreaSpell.maxDamage = 70;
		mana = g_config.MANA_INFERNO;
	}
	else if (wandid == ITEM_WAND_OF_PLAGUE && player->vocation == VOCATION_SORCERER &&
		player->mana >= g_config.MANA_PLAGUE && player->getLevel() >= 19)
	{
		dist = g_config.RANGE_PLAGUE;
		if (abs(player->pos.x - attackedCreature->pos.x) > dist ||
			abs(player->pos.y - attackedCreature->pos.y) > dist)
			return;

		runeAreaSpell.attackType = ATTACK_POISON;
		runeAreaSpell.animationEffect = NM_ANI_FLYPOISONFIELD;
		runeAreaSpell.hitEffect = NM_ME_POISEN_RINGS;
		runeAreaSpell.areaEffect = NM_ME_POISEN_RINGS;
		runeAreaSpell.animationColor = 0x60;

		runeAreaSpell.minDamage = 25;
		runeAreaSpell.maxDamage = 35;
		mana = g_config.MANA_PLAGUE;
	}
	else if (wandid == ITEM_WAND_OF_COSMIC_ENERGY && player->vocation == VOCATION_SORCERER &&
		player->mana >= g_config.MANA_COSMIC && player->getLevel() >= 26)
	{
		dist = g_config.RANGE_COSMIC;
		if (abs(player->pos.x - attackedCreature->pos.x) > dist ||
			abs(player->pos.y - attackedCreature->pos.y) > dist)
			return;

		runeAreaSpell.attackType = ATTACK_ENERGY;
		runeAreaSpell.animationEffect = NM_ANI_ENERGY;
		runeAreaSpell.hitEffect = NM_ME_ENERGY_DAMAGE;
		runeAreaSpell.areaEffect = NM_ME_ENERGY_AREA;
		runeAreaSpell.animationColor = 0x47;

		runeAreaSpell.minDamage = 40;
		runeAreaSpell.maxDamage = 50;
		mana = g_config.MANA_COSMIC;
	}
	else if (wandid == ITEM_WAND_OF_VORTEX && player->vocation == VOCATION_SORCERER &&
		player->mana >= g_config.MANA_VORTEX && player->getLevel() >= 7)
	{
		dist = g_config.RANGE_VORTEX;
		if (abs(player->pos.x - attackedCreature->pos.x) > dist ||
			abs(player->pos.y - attackedCreature->pos.y) > dist)
			return;

		runeAreaSpell.attackType = ATTACK_ENERGY;
		runeAreaSpell.animationEffect = NM_ANI_ENERGY;
		runeAreaSpell.hitEffect = NM_ME_ENERGY_DAMAGE;
		runeAreaSpell.areaEffect = NM_ME_ENERGY_AREA;
		runeAreaSpell.animationColor = 0x47;

		runeAreaSpell.minDamage = 8;
		runeAreaSpell.maxDamage = 18;
		mana = g_config.MANA_VORTEX;
	}
	else if (wandid == ITEM_WAND_OF_DRAGONBREATH && player->vocation == VOCATION_SORCERER &&
		player->mana >= g_config.MANA_DRAGONBREATH && player->getLevel() >= 13)
	{
		dist = g_config.RANGE_DRAGONBREATH;
		if (abs(player->pos.x - attackedCreature->pos.x) > dist ||
			abs(player->pos.y - attackedCreature->pos.y) > dist)
			return;

		runeAreaSpell.attackType = ATTACK_FIRE;
		runeAreaSpell.animationEffect = NM_ANI_FIRE;
		runeAreaSpell.hitEffect = NM_ME_FIRE_AREA;
		runeAreaSpell.areaEffect = NM_ME_FIRE_AREA;
		runeAreaSpell.animationColor = 0xC7;

		runeAreaSpell.minDamage = 14;
		runeAreaSpell.maxDamage = 24;
		mana = g_config.MANA_DRAGONBREATH;
	}

	if (mana > 0)
	{
		std::vector<unsigned char> col;

		col.push_back(0);
		col.push_back(0);
		col.push_back(0);
		runeAreaSpell.areaVec.push_back(col);
		col.clear();
		col.push_back(0);
		col.push_back(1);
		col.push_back(0);
		runeAreaSpell.areaVec.push_back(col);
		col.clear();
		col.push_back(0);
		col.push_back(0);
		col.push_back(0);
		runeAreaSpell.areaVec.push_back(col);

		creatureThrowRune(player, attackedCreature->pos, runeAreaSpell);
		player->addManaSpent(mana);
		player->mana -= mana;
	}
}
#endif //JD_WANDS
