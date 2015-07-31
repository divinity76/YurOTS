
//#include "preheaders.h"
#include "creature.h"
#include "monster.h"
#include "player.h"
#include "luascript.h"
#include "magic.h"
#include <sstream>

extern Game g_game;
extern LuaScript g_config;

MagicEffectClass::MagicEffectClass()
{
    animationColor = 0;
    animationEffect = 0;
    hitEffect = 0xFF;
    damageEffect = 0xFF;
    minDamage = 0;
    maxDamage = 0;
    offensive = false;
    drawblood = false;
    manaCost = 0;
    attackType = ATTACK_NONE;
}

bool MagicEffectClass::isIndirect() const
{
    return false;
}

bool MagicEffectClass::causeExhaustion(bool hasTarget) const
{
    return hasTarget;
}

int32_t MagicEffectClass::getDamage(Creature *target, const Creature *attacker /*= NULL*/) const
{

    if((attackType != ATTACK_NONE) && (target->getImmunities() & attackType) == attackType)
        return 0;

    if((!offensive || (target != attacker)) && target->access < g_config.ACCESS_PROTECT)
    {
        int32_t damage = (int32_t)random_range(minDamage, maxDamage);

        if(!offensive)
        {
            const Monster* targetMonster = dynamic_cast<const Monster*>(target);

            if (target != attacker && targetMonster
#ifdef TR_SUMMONS
                    && !targetMonster->isPlayersSummon()
#endif //TR_SUMMONS
               )
                damage = 0;
            else
                damage = -damage;
        }
        else
        {
            if(attacker && attacker->access >= g_config.ACCESS_PROTECT)
                return damage;

            const Monster *monster = dynamic_cast<const Monster*>(attacker);
            if(monster)
            {
                //no reduction of damage if attack came from a monster
                return damage;
            }

            const Player *targetPlayer = dynamic_cast<const Player*>(target);

            if(targetPlayer)
            {
                damage = (int32_t)floor(damage / 2.0);
            }
        }

        return damage;
    }

    return 0;
}

void MagicEffectClass::getMagicEffect(Player* spectator, const Creature* attacker, const Creature* target,
                                      const Position& pos, /*bool hasTarget,*/ int32_t damage, bool isPz, bool isBlocking) const
{
    if(!isBlocking && target != NULL /*hasTarget*/)
    {
        if(spectator->CanSee(pos.x, pos.y, pos.z))
        {
            if(damageEffect != 0xFF)
            {
                if(!offensive || !(g_game.getWorldType() == WORLD_TYPE_NO_PVP && dynamic_cast<const Player*>(attacker) &&
                                   dynamic_cast<const Player*>(target) && target->access < g_config.ACCESS_PROTECT && attacker->access < g_config.ACCESS_PROTECT) || target->access != 0)
                {
                    if(offensive && (target->getImmunities() & attackType) == attackType)
                    {
                        spectator->sendMagicEffect(pos, target->bloodeffect);
                    }
                    else
                    {
                        spectator->sendMagicEffect(pos, damageEffect);
                    }
                }
            }

            if(hitEffect != 0xFF)
                spectator->sendMagicEffect(pos, hitEffect);
        }
    }
}

void MagicEffectClass::getDistanceShoot(Player* spectator, const Creature* attacker, const Position& to,
                                        bool hasTarget) const
{
    if(animationEffect > 0)
    {
        if(spectator->CanSee(attacker->pos.x, attacker->pos.y, attacker->pos.z) || spectator->CanSee(to.x, to.y, to.z))
        {
            spectator->sendDistanceShoot(attacker->pos, to, animationEffect);
        }
    }
}

void MagicEffectClass::getArea(const Position& rcenterpos, MagicAreaVec& list) const
{
    list.push_back(rcenterpos);
}

MagicEffectItem* MagicEffectClass::getMagicItem(const Creature* attacker, bool isPz, bool isBlocking) const
{
    return NULL;
}

bool MagicEffectClass::canCast(bool isBlocking, bool hasCreature) const
{
    return !isBlocking;
}

void MagicEffectClass::FailedToCast(Player* spectator, const Creature* attacker,
                                    bool isBlocking, bool hasTarget) const
{
    if(!hasTarget && attacker)
    {
        if(attacker == spectator)
        {
            spectator->sendTextMessage(MSG_SMALLINFO, "Tej runy mozesz uzyc tylko na potworach.");
        }
        spectator->sendMagicEffect(attacker->pos, NM_ME_PUFF);
    }
}

//Need a target
MagicEffectTargetClass::MagicEffectTargetClass()
{
    //
}
void MagicEffectTargetClass::getMagicEffect(Player* spectator, const Creature* attacker, const Creature* target,
        const Position& pos, int32_t damage, bool isPz, bool isBlocking) const
{
    if(target != NULL)
    {
        //default
        MagicEffectClass::getMagicEffect(spectator, attacker, target, pos, /*hasTarget,*/ damage, isPz, isBlocking);
    }
    else
    {
        if(attacker)
        {
            if(spectator->CanSee(attacker->pos.x, attacker->pos.y, attacker->pos.z))
            {
                spectator->sendMagicEffect(attacker->pos, NM_ME_PUFF);
            }
        }
    }
}

void MagicEffectTargetClass::getDistanceShoot(Player* spectator, const Creature* attacker, const Position& to,
        bool hasTarget) const
{
    if(animationEffect > 0 && hasTarget)
    {
        if(spectator->CanSee(attacker->pos.x, attacker->pos.y, attacker->pos.z) || spectator->CanSee(to.x, to.y, to.z))
        {
            spectator->sendDistanceShoot(attacker->pos, to, animationEffect);
        }
    }
}

MagicEffectTargetExClass::MagicEffectTargetExClass(const ConditionVec& dmglist) :
    condition(dmglist)
{

}

int32_t MagicEffectTargetExClass::getDamage(Creature *target, const Creature *attacker /*= NULL*/) const
{
    if((attackType != ATTACK_NONE) && (target->getImmunities() & attackType) == attackType)
        return 0;

    if((!offensive || (target != attacker)) && target->access < g_config.ACCESS_PROTECT)
    {

        //target->addMagicDamage(dmgContainer, true);

        bool refresh = true;
        for(ConditionVec::const_iterator condIt = condition.begin(); condIt != condition.end(); ++condIt)
        {
            /*if(condIt == condition.begin()) //skip first
            	continue;*/

            if((condIt->getCondition()->attackType != ATTACK_NONE) &&
                    (target->getImmunities() & condIt->getCondition()->attackType) != condIt->getCondition()->attackType)
            {
                target->addCondition(*condIt, refresh);
                refresh = false; //only set refresh flag on first "new event"
            }
        }

        int32_t damage = (int32_t)random_range(minDamage, maxDamage);

        if(!offensive)
            damage = -damage;

        return damage;
    }

    return 0;
}

//Burning/poisoned/energized
MagicEffectTargetCreatureCondition::MagicEffectTargetCreatureCondition(const uint32_t creatureid)
    : ownerid(creatureid)
{
    //
}

int32_t MagicEffectTargetCreatureCondition::getDamage(Creature *target, const Creature *attacker /*= NULL*/) const
{
    if((attackType != ATTACK_NONE) && (target->getImmunities() & attackType) == attackType)
        return 0;

    if(target->access < g_config.ACCESS_PROTECT)
    {
        int32_t damage = (int32_t)random_range(minDamage, maxDamage);

        if(!offensive)
            damage = -damage;

        return damage;
    }

    return 0;
}

void MagicEffectTargetCreatureCondition::getMagicEffect(Player* spectator, const Creature* attacker, const Creature* target,
        const Position& pos, int32_t damage, bool isPz, bool isBlocking) const
{
    if(target != NULL)
    {
        //default
        MagicEffectClass::getMagicEffect(spectator, attacker, target, pos, /*hasTarget,*/ damage, isPz, isBlocking);
    }
    else
    {
        if(spectator->CanSee(pos.x, pos.y, pos.z))
        {
            spectator->sendMagicEffect(pos, NM_ME_PUFF);
        }
    }

    //MagicEffectTargetClass::getMagicEffect(spectator, attacker, pos, hasTarget, damage, isPz, isBlocking, msg);
}


//magic wall, wild growth
MagicEffectTargetGroundClass::MagicEffectTargetGroundClass(MagicEffectItem* item)
{
    magicItem = item;
}

MagicEffectTargetGroundClass::~MagicEffectTargetGroundClass()
{
    if(magicItem)
    {
        magicItem->releaseThing();
        magicItem = NULL;
    }
}

MagicEffectItem* MagicEffectTargetGroundClass::getMagicItem(const Creature* attacker, bool isPz, bool isBlocking) const
{
    if(!isBlocking && (!isPz || attacker->access >= g_config.ACCESS_PROTECT))
    {
        return magicItem;
    }
    else
        return NULL;
}

bool MagicEffectTargetGroundClass::canCast(bool isBlocking, bool hasCreature) const
{
    if(magicItem)
    {
        if(magicItem->isBlocking() && (isBlocking || hasCreature))
        {
            return false;
        }
    }

    return true;
}

void MagicEffectTargetGroundClass::FailedToCast(Player* spectator, const Creature* attacker,
        bool isBlocking, bool hasTarget) const
{
    const Player* player = dynamic_cast<const Player*>(attacker);

    if(isBlocking || hasTarget)
    {
        if(hasTarget)
        {
            if(player && player == spectator)
            {
                spectator->sendTextMessage(MSG_SMALLINFO, "Przykro mi, nie ma miejsca.");
            }
            spectator->sendMagicEffect(player->pos, NM_ME_PUFF);
        }
        else if(player && player == spectator)
        {
            spectator->sendTextMessage(MSG_SMALLINFO, "Nie mozesz tam rzucic.");
        }
    }
}

void MagicEffectTargetGroundClass::getMagicEffect(Player* spectator, const Creature* attacker, const Creature* target,
        const Position& pos, int32_t damage, bool isPz, bool isBlocking) const
{
    //Default: nothing
}

void MagicEffectTargetGroundClass::getDistanceShoot(Player* spectator, const Creature* attacker, const Position& to,
        bool hasTarget) const
{
    if(!hasTarget && animationEffect > 0)
    {
        if(spectator->CanSee(attacker->pos.x, attacker->pos.y, attacker->pos.z) || spectator->CanSee(to.x, to.y, to.z))
        {
            spectator->sendDistanceShoot(attacker->pos, to, animationEffect);
        }
    }
}

MagicEffectAreaClass::MagicEffectAreaClass()
{
    direction = 0;
    areaEffect = 0xFF;
}

void MagicEffectAreaClass::getMagicEffect(Player* spectator, const Creature* attacker, const Creature* target,
        const Position& pos, int32_t damage, bool isPz, bool isBlocking) const
{
    if(target != NULL && target->access < g_config.ACCESS_PROTECT)
    {
        //default
        MagicEffectClass::getMagicEffect(spectator, attacker, target, pos, /*hasTarget,*/ damage, isPz, isBlocking);
    }
    else
    {
        if(!isBlocking && areaEffect != 0xFF && (attacker->access >= g_config.ACCESS_PROTECT || !isPz))
        {
            if(spectator->CanSee(pos.x, pos.y, pos.z))
            {
                spectator->sendMagicEffect(pos, areaEffect);
            }
        }
    }
}

void MagicEffectAreaClass::getArea(const Position& rcenterpos, MagicAreaVec& list) const
{
    int32_t rows = (int32_t)areaVec.size();
    int32_t cols = (int32_t)(rows > 0 ? areaVec[0].size() : 0);

    if(rows < 3 || cols < 3)
        return;

    Position tpos = rcenterpos;
    tpos.x -= (cols - 1) / 2; //8;
    tpos.y -= (rows - 1) / 2; //6;

    for(int32_t y = 0; y < rows /*14*/; y++)
    {
        for(int32_t x = 0; x < cols /*18*/; x++)
        {
            //if(area[y][x] == direction) {
            if(areaVec[y][x] == direction)
            {

                list.push_back(tpos);
            }
            tpos.x += 1;
        }

        tpos.x -= cols /*18*/;
        tpos.y += 1;
    }
}

MagicEffectAreaExClass::MagicEffectAreaExClass(const ConditionVec& dmglist) :
    condition(dmglist)
{
    //
}

int32_t MagicEffectAreaExClass::getDamage(Creature *target, const Creature *attacker /*= NULL*/) const
{
    if((attackType != ATTACK_NONE) && (target->getImmunities() & attackType) == attackType)
        return 0;

    if(target->access < g_config.ACCESS_PROTECT)
    {
        //target->addMagicDamage(dmgContainer, true);

        bool refresh = true;
        for(ConditionVec::const_iterator condIt = condition.begin(); condIt != condition.end(); ++condIt)
        {
            if(condIt == condition.begin()) //skip first
                continue;

            if((condIt->getCondition()->attackType != ATTACK_NONE) &&
                    (target->getImmunities() & condIt->getCondition()->attackType) != condIt->getCondition()->attackType)
            {
                target->addCondition(*condIt, refresh);
                refresh = false; //only set refresh flag on first "new event"
            }
        }

        int32_t damage = (int32_t)random_range(minDamage, maxDamage);

        if(!offensive)
            damage = -damage;

        return damage;
    }

    return 0;
}

MagicEffectAreaGroundClass::MagicEffectAreaGroundClass(MagicEffectItem* item)
{
    magicItem = item;
}

MagicEffectAreaGroundClass::~MagicEffectAreaGroundClass()
{
    if(magicItem)
    {
        //delete magicItem;
        magicItem->releaseThing();
        magicItem = NULL;
    }
}

int32_t MagicEffectAreaGroundClass::getDamage(Creature *target, const Creature *attacker /*= NULL*/) const
{
    if((attackType != ATTACK_NONE) && (target->getImmunities() & attackType) == attackType)
        return 0;

    if(target->access < g_config.ACCESS_PROTECT)
    {
        if(magicItem)
        {
            return magicItem->getDamage(target, attacker);
        }
        else
            return 0;
    }

    return 0;
}

MagicEffectItem* MagicEffectAreaGroundClass::getMagicItem(const Creature* attacker, bool isPz, bool isBlocking) const
{
    if(!isBlocking && (!isPz || attacker->access >= g_config.ACCESS_PROTECT))
    {
        return magicItem;
    }
    else
        return NULL;
}

//Constructor for solid objects.
MagicEffectItem::MagicEffectItem(const TransformMap& transformMap)
{
    this->transformMap = transformMap;
    useCount = 0;
    uint16_t type = 0;
    TransformMap::const_iterator dm = transformMap.begin();
    if(dm != transformMap.end())
    {
        type = dm->first;
    }
    setID(type);
    buildCondition();
}

bool MagicEffectItem::transform(const MagicEffectItem *rhs)
{
    this->transformMap = rhs->transformMap;
    setID(rhs->getID());
    buildCondition();
    return true;
}

int32_t MagicEffectItem::getDecayTime()
{
    TransformMap::iterator dm = transformMap.find(getID());

    if(dm != transformMap.end())
    {
        return dm->second.first;
    }

    return 0;
}

Item* MagicEffectItem::decay()
{
    TransformMap::iterator dm = transformMap.find(getID());
    if(dm != transformMap.end())
    {

        //get next id to transform to
        ++dm;

        if(dm != transformMap.end())
        {
            setID(dm->first);
            buildCondition();
            return this;
        }
    }

    return NULL;
}

void MagicEffectItem::buildCondition()
{
    condition.clear();

    TransformMap::iterator dm = transformMap.find(getID());
    if(dm != transformMap.end())
    {
        while(dm != transformMap.end())
        {
            for(ConditionVec::iterator di = dm->second.second.begin(); di != dm->second.second.end(); ++di)
            {

                condition.push_back(*di);
            }

            ++dm;
        }
    }
}

int32_t MagicEffectItem::getDamage(Creature *target, const Creature *attacker /*= NULL*/) const
{

    if(target->access < g_config.ACCESS_PROTECT)
    {

        bool refresh = true;
        for(ConditionVec::const_iterator condIt = condition.begin(); condIt != condition.end(); ++condIt)
        {
            if(condIt == condition.begin()) //skip first
                continue;

            if((condIt->getCondition()->attackType != ATTACK_NONE) &&
                    (target->getImmunities() & condIt->getCondition()->attackType) != condIt->getCondition()->attackType)
            {
                target->addCondition(*condIt, refresh);
                refresh = false; //only set refresh flag on first "new event"
            }
        }

        const MagicEffectTargetCreatureCondition *magicTargetCondition = getCondition();

        if(magicTargetCondition)
            return magicTargetCondition->getDamage(target, attacker);
        else
            return 0;
    }

    return 0;
}

const MagicEffectTargetCreatureCondition* MagicEffectItem::getCondition() const
{
    if(!condition.empty())
    {
        return condition[0].getCondition();
    }

    return NULL;
}

