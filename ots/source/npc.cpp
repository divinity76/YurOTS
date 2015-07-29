//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
//
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

#include <algorithm>
#include <functional>
#include <string>
#include <sstream>
#include <fstream>

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

#include "npc.h"
#include "luascript.h"
#include "player.h"

extern LuaScript g_config;

AutoList<Npc> Npc::listNpc;

Npc::Npc(const std::string& name, Game* game) :
 Creature()
{
	char *tmp;
	useCount = 0;
	this->loaded = false;
	this->name = name;
	std::string datadir = g_config.getGlobalString("datadir");
	std::string filename = datadir + "npc/" + std::string(name) + ".xml";
	std::transform(filename.begin(), filename.end(), filename.begin(), tolower);
	xmlDocPtr doc = xmlParseFile(filename.c_str());
	if(doc){
		this->loaded=true;
		xmlNodePtr root, p;
		root = xmlDocGetRootElement(doc);

		if (xmlStrcmp(root->name,(const xmlChar*) "npc")){
		//TODO: use exceptions here
		std::cerr << "Malformed XML" << std::endl;
		}

		p = root->children;

		tmp = (char*)xmlGetProp(root, (const xmlChar *)"script");
		if(tmp){
			this->scriptname = tmp;
			xmlFreeOTSERV(tmp);
		}
		else{
			this->scriptname = "";
		}

		if(tmp = (char*)xmlGetProp(root, (const xmlChar *)"name")) {
			this->name = tmp;
			xmlFreeOTSERV(tmp);
		}
		else{
			this->name = "";
		}

		if(tmp = (char*)xmlGetProp(root, (const xmlChar *)"access")) {
			access = atoi(tmp);
			xmlFreeOTSERV(tmp);
		}
		else{
			access = 0;
		}

		if(tmp = (char*)xmlGetProp(root, (const xmlChar *)"level")) {
			level = atoi(tmp);
			xmlFreeOTSERV(tmp);
			setNormalSpeed();
			//std::cout << level << std::endl;
		}
		else{
			level = 1;
		}

		if(tmp = (char*)xmlGetProp(root, (const xmlChar *)"maglevel")) {
			maglevel = atoi(tmp);
			xmlFreeOTSERV(tmp);
			//std::cout << maglevel << std::endl;
		}
		else{
			maglevel = 1;
		}

		while (p)
		{
			const char* str = (char*)p->name;
			if(strcmp(str, "mana") == 0){
				if(tmp = (char*)xmlGetProp(p, (const xmlChar *)"now")) {
					this->mana = atoll(tmp);
					xmlFreeOTSERV(tmp);
				}
				else{
					this->mana = 100;
				}
				if(tmp = (char*)xmlGetProp(p, (const xmlChar *)"max")) {
					this->manamax = atoll(tmp);
					xmlFreeOTSERV(tmp);
				}
				else{
					this->manamax = 100;
				}
			}
			if(strcmp(str, "health") == 0){
				if(tmp = (char*)xmlGetProp(p, (const xmlChar *)"now")) {
					this->health = atoi(tmp);
					xmlFreeOTSERV(tmp);
				}
				else{
					this->health = 100;
				}
				if(tmp = (char*)xmlGetProp(p, (const xmlChar *)"max")) {
					this->healthmax = atoi(tmp);
					xmlFreeOTSERV(tmp);
				}
				else{
					this->healthmax = 100;
				}
			}
			if(strcmp(str, "look") == 0){
				if(tmp = (char*)xmlGetProp(p, (const xmlChar *)"type")) {
					this->looktype = atoi(tmp);
					xmlFreeOTSERV(tmp);
				}
				else{
					this->looktype = 20;
				}
				this->lookmaster = this->looktype;
				if(tmp = (char*)xmlGetProp(p, (const xmlChar *)"head")) {
					this->lookhead = atoi(tmp);
					xmlFreeOTSERV(tmp);
				}
				else{
					this->lookhead = 10;
				}

				if(tmp = (char*)xmlGetProp(p, (const xmlChar *)"body")) {
					this->lookbody = atoi(tmp);
					xmlFreeOTSERV(tmp);
				}
				else{
					this->lookbody = 20;
				}

				if(tmp = (char*)xmlGetProp(p, (const xmlChar *)"legs")) {
					this->looklegs = atoi(tmp);
					xmlFreeOTSERV(tmp);
				}
				else{
					this->looklegs = 30;
				}

				if(tmp = (char*)xmlGetProp(p, (const xmlChar *)"feet")) {
					this->lookfeet = atoi(tmp);
					xmlFreeOTSERV(tmp);
				}
				else{
					this->lookfeet = 40;
				}
				if(tmp = (char*)xmlGetProp(p, (const xmlChar *)"corpse")) {
					this->lookcorpse = atoi(tmp);
					xmlFreeOTSERV(tmp);
				}
				else{
					this->lookcorpse = 100;
				}

			}
			if(strcmp(str, "attack") == 0){
				if(tmp = (char*)xmlGetProp(p, (const xmlChar *)"type")){
					std::string attacktype = tmp;
					xmlFreeOTSERV(tmp);
					if(attacktype == "melee")
						this->fighttype = FIGHT_MELEE;
					tmp = (char*)xmlGetProp(p, (const xmlChar *)"damage");
					if(tmp){
						this->damage = atoi(tmp);
						xmlFreeOTSERV(tmp);
					}
					else{
						this->damage = 5;
					}
				}
				else{
					this->fighttype = FIGHT_MELEE;
					this->damage = 0;
				}
			}
			if(strcmp(str, "loot") == 0){
				//TODO implement loot
			}

			p = p->next;
		}

		xmlFreeDoc(doc);
	}
	//now try to load the script
	this->script = new NpcScript(this->scriptname, this);
	if(!this->script->isLoaded())
		this->loaded=false;
	this->game=game;
}


Npc::~Npc()
{
	delete this->script;
}

std::string Npc::getDescription(bool self) const
{
	std::stringstream s;
	std::string str;
	s << name << ".";
	str = s.str();
	return str;
}

void Npc::onThingMove(const Player *player, const Thing *thing, const Position *oldPos,
	unsigned char oldstackpos, unsigned char oldcount, unsigned char count){
	//not yet implemented
}

void Npc::onCreatureAppear(const Creature *creature){
	this->script->onCreatureAppear(creature->getID());
}

void Npc::onCreatureDisappear(const Creature *creature, unsigned char stackPos, bool tele){
	this->script->onCreatureDisappear(creature->getID());
}

void Npc::onThingDisappear(const Thing* thing, unsigned char stackPos){
	const Creature *creature = dynamic_cast<const Creature*>(thing);
	if(creature)
		this->script->onCreatureDisappear(creature->getID());
}
void Npc::onThingAppear(const Thing* thing){
	const Creature *creature = dynamic_cast<const Creature*>(thing);
	if(creature)
		this->script->onCreatureAppear(creature->getID());
}

void Npc::onCreatureTurn(const Creature *creature, unsigned char stackpos){
	//not implemented yet, do we need it?
}

/*
void Npc::setAttackedCreature(unsigned long id){
	//not implemented yet
}
*/

void Npc::onCreatureSay(const Creature *creature, SpeakClasses type, const std::string &text){
	if(creature->getID() == this->getID())
		return;
	this->script->onCreatureSay(creature->getID(), type, text);
}

void Npc::onCreatureChangeOutfit(const Creature* creature){
	#ifdef __DEBUG_NPC__
		std::cout << "Npc::onCreatureChangeOutfit" << std::endl;
	#endif
	//we dont care about filthy player changing his ugly clothes
}

int Npc::onThink(int& newThinkTicks){
	this->script->onThink();
	return Creature::onThink(newThinkTicks);
}


void Npc::doSay(std::string msg){
	if(!game->creatureSaySpell(this, msg))
		this->game->creatureSay(this, SPEAK_SAY, msg);
}

void Npc::doAttack(int id){
	attackedCreature = id;
}

void Npc::doMove(int direction){
	switch(direction){
		case 0:
			this->game->thingMove(this, this,this->pos.x, this->pos.y+1, this->pos.z, 1);
		break;
		case 1:
			this->game->thingMove(this, this,this->pos.x+1, this->pos.y, this->pos.z, 1);
		break;
		case 2:
			this->game->thingMove(this, this,this->pos.x, this->pos.y-1, this->pos.z, 1);
		break;
		case 3:
			this->game->thingMove(this, this,this->pos.x-1, this->pos.y, this->pos.z, 1);
		break;
	}
}

void Npc::doMoveTo(Position target){
	if(route.size() == 0 || route.back() != target || route.front() != this->pos){
		route = this->game->getPathTo(this, this->pos, target);
	}
	if(route.size()==0){
		//still no route, means there is none
		return;
	}
	else route.pop_front();
	Position nextStep=route.front();
	route.pop_front();
	int dx = nextStep.x - this->pos.x;
	int dy = nextStep.y - this->pos.y;
	this->game->thingMove(this, this,this->pos.x + dx, this->pos.y + dy, this->pos.z, 1);
}

NpcScript::NpcScript(std::string scriptname, Npc* npc){
	this->npc = NULL;
	this->loaded = false;
	if(scriptname == "")
		return;
	luaState = lua_open();
	luaopen_loadlib(luaState);
	luaopen_base(luaState);
	luaopen_math(luaState);
	luaopen_string(luaState);
	luaopen_io(luaState);

	std::string datadir = g_config.getGlobalString("datadir");
    lua_dofile(luaState, std::string(datadir + "npc/scripts/lib/npc.lua").c_str());

#ifdef USING_VISUAL_2005
	FILE* in = NULL;
	fopen_s(&in, scriptname.c_str(), "r");
#else
	FILE* in=fopen(scriptname.c_str(), "r");
#endif //USING_VISUAL_2005

	if(!in)
		return;
	else
		fclose(in);
	lua_dofile(luaState, scriptname.c_str());
	this->loaded=true;
	this->npc=npc;
	this->setGlobalNumber("addressOfNpc", (int)npc);
	this->registerFunctions();
}

void NpcScript::onThink(){
	lua_pushstring(luaState, "onThink");
	lua_gettable(luaState, LUA_GLOBALSINDEX);
	if(lua_pcall(luaState, 0, 0, 0)){
		std::cerr << "NpcScript: onThink: lua error: " << lua_tostring(luaState, -1) << std::endl;
		lua_pop(luaState,1);
		std::cerr << "Backtrace: " << std::endl;
		lua_Debug* d = NULL;
		int i = 0;
		while(lua_getstack(luaState, i++, d)){
			std::cerr << "    " << d->name << " @ " << d->currentline << std::endl;
		}
	}
}


void NpcScript::onCreatureAppear(unsigned long cid){
	if(npc->getID() != cid){
		lua_pushstring(luaState, "onCreatureAppear");
		lua_gettable(luaState, LUA_GLOBALSINDEX);
		lua_pushnumber(luaState, cid);
		if(lua_pcall(luaState, 1, 0, 0)){
			std::cerr << "NpcScript: onCreatureAppear: lua error: " << lua_tostring(luaState, -1) << std::endl;
			lua_pop(luaState,1);
			std::cerr << "Backtrace: " << std::endl;
			lua_Debug* d = NULL;
			int i = 0;
			while(lua_getstack(luaState, i++, d)){
				std::cerr << "    " << d->name << " @ " << d->currentline << std::endl;
			}
		}
	}
}

void NpcScript::onCreatureDisappear(int cid){
	lua_pushstring(luaState, "onCreatureDisappear");
	lua_gettable(luaState, LUA_GLOBALSINDEX);
	lua_pushnumber(luaState, cid);
	if(lua_pcall(luaState, 1, 0, 0)){
		std::cerr << "NpcScript: onCreatureDisappear: lua error: " << lua_tostring(luaState, -1) << std::endl;
		lua_pop(luaState,1);
		std::cerr << "Backtrace: " << std::endl;
		lua_Debug* d = NULL;
		int i = 0;
		while(lua_getstack(luaState, i++, d)){
			std::cerr << "    " << d->name << " @ " << d->currentline << std::endl;
		}
	}
}

void NpcScript::onCreatureSay(int cid, SpeakClasses type, const std::string &text)
{
	if (!npc->game->getPlayerByID(cid))		// Tibia Rules' fix
		return;

	//now we need to call the function
	lua_pushstring(luaState, "onCreatureSay");
	lua_gettable(luaState, LUA_GLOBALSINDEX);
	lua_pushnumber(luaState, cid);
	lua_pushnumber(luaState, type);
	lua_pushstring(luaState, text.c_str());
	if(lua_pcall(luaState, 3, 0, 0)){
		std::cerr << "NpcScript: onCreatureSay: lua error: " << lua_tostring(luaState, -1) << std::endl;
		lua_pop(luaState,1);
		std::cerr << "Backtrace: " << std::endl;
		lua_Debug* d = NULL;
		int i = 0;
		while(lua_getstack(luaState, i++, d)){
			std::cerr << "    " << d->name << " @ " << d->currentline << std::endl;
		}
	}
}

int NpcScript::registerFunctions()
{
	lua_register(luaState, "selfSay", NpcScript::luaActionSay);
	lua_register(luaState, "selfMove", NpcScript::luaActionMove);
	lua_register(luaState, "selfMoveTo", NpcScript::luaActionMoveTo);
	lua_register(luaState, "selfGetPosition", NpcScript::luaSelfGetPos);
	lua_register(luaState, "selfAttackCreature", NpcScript::luaActionAttackCreature);
	lua_register(luaState, "creatureGetName", NpcScript::luaCreatureGetName);
	lua_register(luaState, "creatureGetName2", NpcScript::luaCreatureGetName2);
	lua_register(luaState, "creatureGetPosition", NpcScript::luaCreatureGetPos);
	lua_register(luaState, "selfGetPosition", NpcScript::luaSelfGetPos);

#ifdef TLM_BUY_SELL
	lua_register(luaState, "buy", NpcScript::luaBuyItem);
	lua_register(luaState, "sell", NpcScript::luaSellItem);
	lua_register(luaState, "pay", NpcScript::luaPayMoney);
#endif

#ifdef YUR_NPC_EXT
	lua_register(luaState, "getPlayerStorageValue", NpcScript::luaGetPlayerStorageValue);
	lua_register(luaState, "setPlayerStorageValue", NpcScript::luaSetPlayerStorageValue);
	lua_register(luaState, "doPlayerRemoveItem", NpcScript::luaPlayerRemoveItem);
	lua_register(luaState, "getPlayerLevel", NpcScript::luaGetPlayerLevel);
	lua_register(luaState, "getPlayerVocation", NpcScript::luaGetPlayerVocation);
	lua_register(luaState, "setPlayerMasterPos", NpcScript::luaSetPlayerMasterPos);
#endif //YUR_NPC_EXT

#ifdef YUR_GUILD_SYSTEM
	lua_register(luaState, "foundNewGuild", NpcScript::luaFoundNewGuild);
	lua_register(luaState, "getPlayerGuildStatus", NpcScript::luaGetPlayerGuildStatus);
	lua_register(luaState, "setPlayerGuildStatus", NpcScript::luaSetPlayerGuildStatus);
	lua_register(luaState, "getPlayerGuildName", NpcScript::luaGetPlayerGuildName);
	lua_register(luaState, "setPlayerGuild", NpcScript::luaSetPlayerGuild);
	lua_register(luaState, "clearPlayerGuild", NpcScript::luaClearPlayerGuild);
	lua_register(luaState, "setPlayerGuildNick", NpcScript::luaSetPlayerGuildNick);
	lua_register(luaState, "setPlayerGuildTitle", NpcScript::luaSetPlayerGuildNick);	// old
#endif //YUR_GUILD_SYSTEM

#ifdef YUR_PREMIUM_PROMOTION
	lua_register(luaState, "isPremium", NpcScript::luaIsPremium);
	lua_register(luaState, "isPromoted", NpcScript::luaIsPromoted);
#endif //YUR_PREMIUM_PROMOTION

#ifdef YUR_ROOKGARD
	lua_register(luaState, "setPlayerVocation", NpcScript::luaSetPlayerVocation);
#endif //YUR_ROOKGARD

#ifdef YUR_LEARN_SPELLS
	lua_register(luaState, "learnSpell", NpcScript::luaLearnSpell);
#endif //YUR_LEARN_SPELLS

	return true;
}

Npc* NpcScript::getNpc(lua_State *L){
	lua_getglobal(L, "addressOfNpc");
	int val = (int)lua_tonumber(L, -1);
	lua_pop(L,1);
	Npc* mynpc = (Npc*)val;

	if(!mynpc){
		return 0;
	}
	return mynpc;
}

int NpcScript::luaCreatureGetName2(lua_State *L){
	const char* s = lua_tostring(L, -1);
	lua_pop(L,1);
	Npc* mynpc = getNpc(L);
	Creature *c = mynpc->game->getCreatureByName(std::string(s));

	if(c && c->access < g_config.ACCESS_PROTECT) {
		lua_pushnumber(L, c->getID());
	}
	else
		lua_pushnumber(L, 0);

	return 1;
}

int NpcScript::luaCreatureGetName(lua_State *L){
	int id = (int)lua_tonumber(L, -1);
	lua_pop(L,1);
	Npc* mynpc = getNpc(L);
	lua_pushstring(L, mynpc->game->getCreatureByID(id)->getName().c_str());
	return 1;
}

int NpcScript::luaCreatureGetPos(lua_State *L){
	int id = (int)lua_tonumber(L, -1);
	lua_pop(L,1);
	Npc* mynpc = getNpc(L);
	Creature* c = mynpc->game->getCreatureByID(id);

	if(!c){
		lua_pushnil(L);
		lua_pushnil(L);
		lua_pushnil(L);
	}
	else{
		lua_pushnumber(L, c->pos.x);
		lua_pushnumber(L, c->pos.y);
		lua_pushnumber(L, c->pos.z);
	}
	return 3;
}

int NpcScript::luaSelfGetPos(lua_State *L){
	lua_pop(L,1);
	Npc* mynpc = getNpc(L);
	lua_pushnumber(L, mynpc->pos.x);
	lua_pushnumber(L, mynpc->pos.y);
	lua_pushnumber(L, mynpc->pos.z);
	return 3;
}

int NpcScript::luaActionSay(lua_State* L){
	int len = (uint32_t)lua_strlen(L, -1);
	std::string msg(lua_tostring(L, -1), len);
	lua_pop(L,1);
	//now, we got the message, we now have to find out
	//what npc this belongs to

	Npc* mynpc=getNpc(L);
	if(mynpc)
		mynpc->doSay(msg);
	return 0;
}

int NpcScript::luaActionMove(lua_State* L){
	int dir=(int)lua_tonumber(L, -1);
	lua_pop(L,1);
	Npc* mynpc=getNpc(L);
	if(mynpc)
		mynpc->doMove(dir);
	return 0;
}

int NpcScript::luaActionMoveTo(lua_State* L){
	Position target;
	target.z=(int)lua_tonumber(L, -1);
	lua_pop(L,1);
	target.y=(int)lua_tonumber(L, -1);
	lua_pop(L,1);
	target.x=(int)lua_tonumber(L, -1);
	lua_pop(L,1);
	Npc* mynpc=getNpc(L);
	if(mynpc)
		mynpc->doMoveTo(target);
	return 0;
}



int NpcScript::luaActionAttackCreature(lua_State *L){
	int id=(int)lua_tonumber(L, -1);
	lua_pop(L,1);
	Npc* mynpc=getNpc(L);
	if(mynpc)
		mynpc->doAttack(id);
	return 0;
}


#ifdef TLM_BUY_SELL
int NpcScript::luaBuyItem(lua_State *L)
{
	int cost = (int)lua_tonumber(L, -1);
	int count = (int)lua_tonumber(L, -2);
	int itemid = (int)lua_tonumber(L, -3);
	int cid = (int)lua_tonumber(L, -4);
	lua_pop(L,4);

	Npc* mynpc = getNpc(L);
	Creature* creature = mynpc->game->getCreatureByID(cid);
	Player* player = creature? dynamic_cast<Player*>(creature) : NULL;

	if (player)
	{
		if (player->getCoins(cost))
		{
			if (player->removeCoins(cost)) // double check
			{
				player->TLMaddItem(itemid, count);
				mynpc->doSay("Here you are.");
			}
			else
				mynpc->doSay("Sorry, you do not have enough money.");
		}
		else
			mynpc->doSay("Sorry, you do not have enough money.");
	}

	return 0;
}

int NpcScript::luaSellItem(lua_State *L)
{
   int cost = (int)lua_tonumber(L, -1);
   int count = (int)lua_tonumber(L, -2);
   int itemid = (int)lua_tonumber(L, -3);
   int cid = (int)lua_tonumber(L, -4);
   lua_pop(L,4);

   Npc* mynpc = getNpc(L);
   Creature* creature = mynpc->game->getCreatureByID(cid);
   Player* player = creature? dynamic_cast<Player*>(creature) : NULL;

	if (player)
	{
		if (player->getItem(itemid, count))
		{
			if (player->removeItem(itemid, count)) // double check
			{
				player->payBack(cost);
				mynpc->doSay("Thanks for this item!");
			}
			else
				mynpc->doSay("Sorry, you do not have that item.");
		}
		else
			mynpc->doSay("Sorry, you do not have that item.");
	}

	return 0;
}

int NpcScript::luaPayMoney(lua_State *L)
{
   int cost = (int)lua_tonumber(L, -1);
   int cid = (int)lua_tonumber(L, -2);
   lua_pop(L,2);

   Npc* mynpc = getNpc(L);
   Creature* creature = mynpc->game->getCreatureByID(cid);
   Player* player = creature? dynamic_cast<Player*>(creature) : NULL;

	if (player)
	{
		if (player->getCoins(cost))
		{
			if (player->removeCoins(cost)) // double check
				lua_pushboolean(L, true);
			else
				lua_pushboolean(L, false);
		}
		else
			lua_pushboolean(L, false);
	}
	else
		lua_pushboolean(L, false);

	return 1;
}
#endif //TLM_BUY_SELL


#ifdef YUR_NPC_EXT
int NpcScript::luaGetPlayerStorageValue(lua_State* L)
{
	int id = (int)lua_tonumber(L, -2);
	unsigned long key = (unsigned long)lua_tonumber(L, -1);
	lua_pop(L, 2);

	Npc* mynpc = getNpc(L);
	Creature* creature = mynpc->game->getCreatureByID(id);
	Player* player = creature? dynamic_cast<Player*>(creature) : NULL;

	long value;
	if (player && player->getStorageValue(key, value))
		lua_pushnumber(L, value);
	else
		lua_pushnumber(L, -1);

	return 1;
}

int NpcScript::luaSetPlayerStorageValue(lua_State* L)
{
	int id = (int)lua_tonumber(L, -3);
	unsigned long key = (unsigned long)lua_tonumber(L, -2);
	long value = (long)lua_tonumber(L, -1);
	lua_pop(L, 3);

	Npc* mynpc = getNpc(L);
	Creature* creature = mynpc->game->getCreatureByID(id);
	Player* player = creature? dynamic_cast<Player*>(creature) : NULL;

	if (player)
		player->addStorageValue(key, value);

	return 0;
}

int NpcScript::luaPlayerRemoveItem(lua_State* L)
{
	int id = (int)lua_tonumber(L, -2);
	unsigned long item_id = (unsigned long)lua_tonumber(L, -1);
	lua_pop(L, 2);

	Npc* mynpc = getNpc(L);
	Creature* creature = mynpc->game->getCreatureByID(id);
	Player* player = creature? dynamic_cast<Player*>(creature) : NULL;

	if (player && player->removeItem(item_id, 1))
		lua_pushnumber(L, 0);
	else
		lua_pushnumber(L, -1);

	return 1;
}

int NpcScript::luaGetPlayerLevel(lua_State* L)
{
	const char* name = lua_tostring(L, -1);
	lua_pop(L, 1);

	Npc* mynpc = getNpc(L);
	Creature* creature = mynpc->game->getCreatureByName(name);
	Player* player = creature? dynamic_cast<Player*>(creature) : NULL;

	if (player)
		lua_pushnumber(L, player->getLevel());
	else
		lua_pushnumber(L, -1);

	return 1;
}

int NpcScript::luaSetPlayerMasterPos(lua_State* L)
{
	int id = (int)lua_tonumber(L, -4);
	int x = (int)lua_tonumber(L, -3);
	int y = (int)lua_tonumber(L, -2);
	int z = (int)lua_tonumber(L, -1);
	lua_pop(L, 4);

	Npc* mynpc = getNpc(L);
	Creature* creature = mynpc->game->getCreatureByID(id);
	Player* player = creature? dynamic_cast<Player*>(creature) : NULL;

	if (player)
	{
		Position pos(x,y,z);
		Tile* tile = mynpc->game->getTile(pos);

		if (tile)
			player->masterPos = pos;
		else
			std::cout << "NpcScript: luaSetPlayerMasterPos: given position is invalid!" << std::endl;
	}

	return 0;
}
#endif //YUR_NPC_EXT


#ifdef YUR_GUILD_SYSTEM
int NpcScript::luaFoundNewGuild(lua_State* L)
{
	const char* gname = lua_tostring(L, -1);
	lua_pop(L, 1);

	if (gname)
		lua_pushnumber(L, Guilds::AddNewGuild(gname));
	else
		lua_pushnumber(L, -1);

	return 1;
}

int NpcScript::luaGetPlayerGuildStatus(lua_State* L)
{
	const char* name = lua_tostring(L, -1);
	lua_pop(L, 1);
	lua_pushnumber(L, Guilds::GetGuildStatus(name));
	return 1;
}

int NpcScript::luaSetPlayerGuildStatus(lua_State* L)
{
	const char* name = lua_tostring(L, -2);
	int gstat = (int)lua_tonumber(L, -1);
	lua_pop(L, 2);

	Guilds::SetGuildStatus(std::string(name), (gstat_t)gstat);

	Npc* mynpc = getNpc(L);
	Creature* creature = mynpc->game->getCreatureByName(name);
	Player* player = creature? dynamic_cast<Player*>(creature) : NULL;

	if (player)
		Guilds::ReloadGuildInfo(player);

	return 0;
}

int NpcScript::luaGetPlayerGuildName(lua_State* L)
{
	const char* name = lua_tostring(L, -1);
	lua_pop(L, 1);
	lua_pushstring(L, Guilds::GetGuildName(name).c_str());
	return 1;
}

int NpcScript::luaSetPlayerGuild(lua_State* L)
{
	const char* name = lua_tostring(L, -4);
	int gstat = (int)lua_tonumber(L, -3);
	const char* grank = lua_tostring(L, -2);
	const char* gname = lua_tostring(L, -1);
	lua_pop(L, 4);

	Guilds::SetGuildInfo(name, (gstat_t)gstat, grank, gname);

	Npc* mynpc = getNpc(L);
	Creature* creature = mynpc->game->getCreatureByName(name);
	Player* player = creature? dynamic_cast<Player*>(creature) : NULL;

	if (player)
		Guilds::ReloadGuildInfo(player);

	return 0;
}

int NpcScript::luaClearPlayerGuild(lua_State* L)
{
	const char* name = lua_tostring(L, -1);
	lua_pop(L, 1);

	Guilds::ClearGuildInfo(name);

	Npc* mynpc = getNpc(L);
	Creature* creature = mynpc->game->getCreatureByName(name);
	Player* player = creature? dynamic_cast<Player*>(creature) : NULL;

	if (player)
		Guilds::ReloadGuildInfo(player);

	return 0;
}

int NpcScript::luaSetPlayerGuildNick(lua_State* L)
{
	const char* name = lua_tostring(L, -2);
	const char* nick = lua_tostring(L, -1);
	lua_pop(L, 2);

	Guilds::SetGuildNick(name, nick);

	Npc* mynpc = getNpc(L);
	Creature* creature = mynpc->game->getCreatureByName(name);
	Player* player = creature? dynamic_cast<Player*>(creature) : NULL;

	if (player)
		Guilds::ReloadGuildInfo(player);

	return 0;
}
#endif //YUR_GUILD_SYSTEM


#ifdef YUR_ROOKGARD
int NpcScript::luaSetPlayerVocation(lua_State* L)
{
	int id = (int)lua_tonumber(L, -2);
	int voc = (int)lua_tonumber(L, -1);
	lua_pop(L, 2);

	Npc* mynpc = getNpc(L);
	Creature* creature = mynpc->game->getCreatureByID(id);
	Player* player = creature? dynamic_cast<Player*>(creature) : NULL;

	if (player)
		player->setVocation((playervoc_t)voc);

	return 0;
}
#endif //YUR_ROOKGARD


#ifdef YUR_LEARN_SPELLS
int NpcScript::luaGetPlayerVocation(lua_State* L)
{
	int id = (int)lua_tonumber(L, -1);
	lua_pop(L, 1);

	Npc* mynpc = getNpc(L);
	Creature* creature = mynpc->game->getCreatureByID(id);
	Player* player = creature? dynamic_cast<Player*>(creature) : NULL;

	if (player)
		lua_pushnumber(L, player->getVocation());
	else
		lua_pushnumber(L, -1);

	return 1;
}

int NpcScript::luaLearnSpell(lua_State *L)
{
	int cid = (int)lua_tonumber(L, -3);
	const char* words = lua_tostring(L, -2);
	int cost = (int)lua_tonumber(L, -1);
	lua_pop(L,3);

	Npc* mynpc = getNpc(L);
	Creature* creature = mynpc->game->getCreatureByID(cid);
	Player *player = creature? dynamic_cast<Player*>(creature) : NULL;

	if (player)
	{
		if (player->knowsSpell(words))
		{
			mynpc->doSay("You already know this spell.");
		}
		else if (player->getCoins(cost))
		{
			if (player->removeCoins(cost)) // double check
			{
				player->learnSpell(words);
				player->sendMagicEffect(player->pos, NM_ME_MAGIC_ENERGIE);
				mynpc->doSay((std::string("To use it say: ") + std::string(words) + ".").c_str());
			}
			else
				mynpc->doSay("Sorry, you do not have enough money.");
		}
		else
			mynpc->doSay("Sorry, you do not have enough money.");
	}

	return 0;
}
#endif //YUR_LEARN_SPELLS


#ifdef YUR_PREMIUM_PROMOTION
int NpcScript::luaIsPromoted(lua_State* L)
{
	int id = (int)lua_tonumber(L, -1);
	lua_pop(L, 1);

	Npc* mynpc = getNpc(L);
	Creature* creature = mynpc->game->getCreatureByID(id);
	Player* player = creature? dynamic_cast<Player*>(creature) : NULL;

	if (player)
		lua_pushboolean(L, player->isPromoted());
	else
		lua_pushboolean(L, false);

	return 1;
}

int NpcScript::luaIsPremium(lua_State* L)
{
	int id = (int)lua_tonumber(L, -1);
	lua_pop(L, 1);

	Npc* mynpc = getNpc(L);
	Creature* creature = mynpc->game->getCreatureByID(id);
	Player* player = dynamic_cast<Player*>(creature);

	if (player)
		lua_pushboolean(L, player->isPremium());
	else
		lua_pushboolean(L, false);

	return 1;
}
#endif //YUR_PREMIUM_PROMOTION
