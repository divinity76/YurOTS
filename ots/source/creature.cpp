
#include "definitions.h"

#include <string>
#include <sstream>
#include <algorithm>

#include "game.h"
#include "creature.h"
#include "tile.h"
#include "otsystem.h"
#include "player.h"
#include "luascript.h"

extern LuaScript g_config;

using namespace std;

OTSYS_THREAD_LOCKVAR AutoID::autoIDLock;
uint32_t AutoID::count = 1000;
AutoID::list_type AutoID::list;

extern Game g_game;

//Creature::Creature(const std::string& name) :
Creature::Creature() :
    access(0)
{
    direction  = NORTH;
    master = NULL;

    //this->name = name;

    lookhead   = 0;
    lookbody   = 0;
    looklegs   = 0;
    lookfeet   = 0;
    lookmaster = 0;
    looktype   = PLAYER_MALE_1;
    pzLocked = false;

    lookcorpse = ITEM_HUMAN_CORPSE;

#ifdef HUCZU_EXHAUSTED
    mmo = 0; //do komend
    lookex= 0; // do look
    antyrainbow = 0; // do outfitow
    antyrainbow2 = 0; // do outfitow
#endif //HUCZU_EXHAUSTED
    oneSiteAttack = false;
    health     = 1000;//150;
    healthmax  = 1000;//150;
    //experience = 100000;
    lastmove=0;

    inFightTicks = 0;
    inFightTicks = 0;
    manaShieldTicks = 0;
    hasteTicks = 0;
    paralyzeTicks = 0;
    exhaustedTicks  = 0;
    pzLocked = false;
    immunities = 0;
    eventCheck = 0;
    eventCheckAttacking = 0;
#ifdef HUCZU_FOLLOW
    eventCheckFollow = 0;
    followCreature = 0;
#endif
    attackedCreature = 0;
    speed = 220;
    dwarvenTicks = 0;
    //drunkTicks = 0;
    hasteSpeed = 0;
#ifdef YUR_BOH
    boh = false;
#endif //YUR_BOH

#ifdef YUR_RINGS_AMULETS
    timeRing = false;
#endif //YUR_RINGS_AMULETS

    skullType = SKULL_NONE;

    invisibleTicks = 0;
    bloodcolor = COLOR_RED; //the damage string
    bloodeffect = EFFECT_RED; //the hiteffect
    bloodsplash = SPLASH_RED; //splash on ground
}

Creature::~Creature()
{
    std::list<Creature*>::iterator cit;
    for(cit = summons.begin(); cit != summons.end(); ++cit)
    {
        (*cit)->setAttackedCreature(NULL);
        (*cit)->setMaster(NULL);
        (*cit)->releaseThing();
    }

    summons.clear();
}

#ifdef YUR_PVP_ARENA
void Creature::drainHealth(int32_t damage, CreatureVector* arenaLosers)
{
    if (arenaLosers && damage >= health)
    {
        health = healthmax;
        arenaLosers->push_back(this);
    }
    else
        health -= min(health, damage);
}
#else
void Creature::drainHealth(int32_t damage)
{
    health -= min(health, damage);
}
#endif //YUR_PVP_ARENA

void Creature::drainMana(int32_t damage)
{
    mana -= min(mana, damage);
}

//void Creature::setAttackedCreature(uint32_t id)
void Creature::setAttackedCreature(const Creature* creature)
{
    std::list<Creature*>::iterator cit;
    for(cit = summons.begin(); cit != summons.end(); ++cit)
    {
        (*cit)->setAttackedCreature(creature);
    }

    if(creature)
    {
        attackedCreature = creature->getID();
    }
    else
        attackedCreature = 0;
}

#ifdef TR_SUMMONS
bool Creature::isPlayersSummon() const
{
    return master && dynamic_cast<Player*>(master);
}
#endif //TR_SUMMONS

void Creature::setMaster(Creature* creature)
{
    //std::cout << "setMaster: " << this << " master=" << creature << std::endl;
    master = creature;
}

void Creature::addSummon(Creature *creature)
{
    //std::cout << "addSummon: " << this << " summon=" << creature << std::endl;
    creature->setMaster(this);
    creature->useThing();
    summons.push_back(creature);

}

void Creature::removeSummon(Creature *creature)
{
    //std::cout << "removeSummon: " << this << " summon=" << creature << std::endl;
    std::list<Creature*>::iterator cit = std::find(summons.begin(), summons.end(), creature);
    if(cit != summons.end())
    {
        (*cit)->setMaster(NULL);
        (*cit)->releaseThing();
        summons.erase(cit);
    }
}

void Creature::addCondition(const CreatureCondition& condition, bool refresh)
{
    if(condition.getCondition()->attackType == ATTACK_NONE)
        return;

    ConditionVec &condVec = conditions[condition.getCondition()->attackType];

    if(refresh)
    {
        condVec.clear();
    }

    condVec.push_back(condition);
}

void Creature::addInflictedDamage(Creature* attacker, int32_t damage)
{
    if(damage <= 0)
        return;

    uint32_t id = 0;
    if(attacker)
    {
        id = attacker->getID();
    }

    totaldamagelist[id].push_back(make_pair(OTSYS_TIME(), damage));
}

exp_t Creature::getLostExperience()
{
    //return (int32_t)std::floor(((double)experience * 0.1));
    return 0;
}

int32_t Creature::getInflicatedDamage(uint32_t id)
{
    int32_t ret = 0;
    std::map<int32_t, DamageList >::const_iterator tdIt = totaldamagelist.find(id);
    if(tdIt != totaldamagelist.end())
    {
        for(DamageList::const_iterator dlIt = tdIt->second.begin(); dlIt != tdIt->second.end(); ++dlIt)
        {
            ret += dlIt->second;
        }
    }

    return ret;
}

int32_t Creature::getInflicatedDamage(Creature* attacker)
{
    uint32_t id = 0;
    if(attacker)
    {
        id = attacker->getID();
    }

    return getInflicatedDamage(id);
}

int32_t Creature::getTotalInflictedDamage()
{
    int32_t ret = 0;
    std::map<int32_t, DamageList >::const_iterator tdIt;
    for(tdIt = totaldamagelist.begin(); tdIt != totaldamagelist.end(); ++tdIt)
    {
        ret += getInflicatedDamage(tdIt->first);
    }

    return ret;
}

exp_t Creature::getGainedExperience(Creature* attacker)
{
    int32_t totaldamage = getTotalInflictedDamage();
    int32_t attackerdamage = getInflicatedDamage(attacker);
    exp_t lostexperience = getLostExperience();
    exp_t gainexperience = 0;
    Player* player = dynamic_cast<Player*>(attacker);

    if(attackerdamage > 0 && totaldamage > 0)
    {
        //gainexperience = (int32_t)std::floor(((double)attackerdamage / totaldamage) * lostexperience);
        gainexperience = (exp_t)(attackerdamage * lostexperience / totaldamage);
    }

#ifdef HUCZU_STAGE_EXP
    if(g_config.STAGE_EXP)
    {
        int32_t multipiler = 0;
        if(player)
        {
            if(!dynamic_cast<Player*>(this))
                multipiler = g_game.getStageExp(player->getLevel(), false); //brak enfo
            else
                multipiler = g_game.getStageExp(player->getLevel(), true); //enfo

            if(multipiler > 0)
                return gainexperience * multipiler;
        }
    }
    else
    {
        if (dynamic_cast<Player*>(this))
            return gainexperience * g_config.EXP_MUL_PVP;
        else if(player && player->getVocation() == VOCATION_NONE)
            return gainexperience * g_config.EXP_ROOK;
        else
            return gainexperience * g_config.EXP_MUL;
    }
#endif
    return 0; // dla bezpieczenstwa
}

std::vector<int32_t> Creature::getInflicatedDamageCreatureList()
{
    std::vector<int32_t> damagelist;
    std::map<int32_t, DamageList >::const_iterator tdIt;
    for(tdIt = totaldamagelist.begin(); tdIt != totaldamagelist.end(); ++tdIt)
    {
        damagelist.push_back(tdIt->first);
    }

    return damagelist;
}

bool Creature::canMovedTo(const Tile *tile) const
{
    if(tile)
    {
        return (tile->isBlocking(BLOCK_SOLID) == RET_NOERROR);
    }

    return false;
}

std::string Creature::getDescription(bool self) const
{
    std::stringstream s;
    std::string str;
    s << "a creature.";
    str = s.str();
    return str;
}

int32_t Creature::getStepDuration() const
{
    int32_t duration = 500;
    Tile *tile = g_game.getTile(pos.x, pos.y, pos.z);
    if(tile && tile->ground)
    {
        int32_t groundid = tile->ground->getID();
        uint16_t stepspeed = Item::items[groundid].speed;
        if(stepspeed != 0)
        {
            duration =  (1000 * stepspeed) / (getSpeed() != 0 ? getSpeed() : 220);
        }
    }
    return duration;
};

int64_t Creature::getSleepTicks() const
{
    int64_t delay = 0;
    int32_t stepDuration = getStepDuration();

    if(lastmove != 0)
    {
        delay = (((int64_t)(lastmove)) + ((int64_t)(stepDuration))) - ((int64_t)(OTSYS_TIME()));
    }

    return delay;
}

bool Creature::checkInvisible(int32_t thinkTicks)
{
    if (invisibleTicks > 0)
    {
        invisibleTicks -= thinkTicks;

        if (invisibleTicks <= 0)
        {
            invisibleTicks = 0;
            return true;	// needs change outfit
        }
    }

    return false;	// outfit stays the same
}

void Creature::removeCondition(attacktype_t attackType)
{
    if(attackType == ATTACK_NONE)
        return;
    ConditionVec &condVec = conditions[attackType];
    condVec.clear();
}
