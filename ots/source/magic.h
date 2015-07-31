
#ifndef __MAGIC_H__
#define __MAGIC_H__

#include "position.h"
#include "item.h"

#include <map>
#include "tools.h"
#include "const76.h"

class Creature;
class Player;
class Item;
class Position;

enum attacktype_t
{
    ATTACK_NONE = 0,
    ATTACK_ENERGY = 1,
    ATTACK_BURST = 2,
    ATTACK_FIRE = 8,
    ATTACK_PHYSICAL = 16,
    ATTACK_POISON = 32,
    ATTACK_PARALYZE = 64,
    ATTACK_DRUNKNESS = 128,
    ATTACK_LIFEDRAIN = 256,
    ATTACK_MANADRAIN = 512,
    ATTACK_INVISIBLE = 1024
};

/*
MagicEffectClass
|
|
|----->	MagicEffectTargetClass : public MagicEffectClass
|       |
|       |-----> MagicEffectTargetEx : public MagicEffectTargetClass //ie. soul fire
|       |
|       |-----> MagicEffectTargetCreatureCondition : public MagicEffectTarget //ie. burning, energized etc.
|       |
|       |-----> MagicEffectTargetGroundClass //ie. m-wall, wild growth
|								(Holds a MagicEffectItem*)
|
|-----> MagicEffectAreaClass : public MagicEffectClass //ie. gfb
|       |
|       |-----> MagicEffectAreaExClass : public MagicEffectAreaClass //ie. poison storm
|       |
|	      |-----> MagicEffectAreaGroundClass : public MagicEffectArea //ie. fire bomb
|                 (Holds a MagicEffectItem*)
|
|----->	MagicEffectItem : public Item, public MagicEffectClass
*/
//------------------------------------------------------------------------------

class MagicEffectItem;

typedef std::vector<Position> MagicAreaVec;

class MagicEffectClass
{
public:
    MagicEffectClass();
    virtual ~MagicEffectClass() {};

    virtual bool isIndirect() const;
    virtual bool causeExhaustion(bool hasTarget) const;

    virtual int32_t getDamage(Creature* target, const Creature* attacker = NULL) const;

    virtual void getMagicEffect(Player* spectator, const Creature* attacker, const Creature* target,
                                const Position& pos, int32_t damage, bool isPz, bool isBlocking) const;

    virtual void getDistanceShoot(Player* spectator, const Creature* c, const Position& to,
                                  bool hasTarget) const;

    virtual void getArea(const Position& rcenterpos, MagicAreaVec& list) const;

    virtual MagicEffectItem* getMagicItem(const Creature* attacker, bool isPz, bool isBlocking) const;

    virtual bool canCast(bool isBlocking, bool hasCreature) const;

    virtual void FailedToCast(Player* spectator, const Creature* attacker,
                              bool isBlocking, bool hasTarget) const;

    int32_t minDamage;
    int32_t maxDamage;
    bool offensive;
    bool drawblood; //causes blood splashes
    int32_t manaCost;

    attacktype_t attackType;

    unsigned char animationColor;
    unsigned char animationEffect;
    unsigned char hitEffect;
    unsigned char damageEffect;
};

//Need a target. Example sudden death
class MagicEffectTargetClass : public MagicEffectClass
{
public:
    MagicEffectTargetClass();
    virtual ~MagicEffectTargetClass() {};

    virtual void getMagicEffect(Player* spectator, const Creature* attacker, const Creature* target,
                                const Position& pos, int32_t damage, bool isPz, bool isBlocking) const;

    virtual void getDistanceShoot(Player* spectator, const Creature* attacker, const Position& to,
                                  bool hasTarget) const;

    virtual bool canCast(bool isBlocking, bool hasCreature) const
    {
        return !isBlocking && hasCreature;
    }
};

//Is created indirectly, need a target and make magic damage (burning/poisoned/energized)
class MagicEffectTargetCreatureCondition : public MagicEffectTargetClass
{
public:
    MagicEffectTargetCreatureCondition() {};
    MagicEffectTargetCreatureCondition(uint32_t creatureid);
    virtual ~MagicEffectTargetCreatureCondition() {};

    virtual bool isIndirect() const
    {
        return true;
    }

    virtual bool causeExhaustion(bool hasTarget) const
    {
        return false;
    }

    virtual int32_t getDamage(Creature *target, const Creature *attacker = NULL) const;

    virtual void getMagicEffect(Player* spectator, const Creature* attacker, const Creature* target,
                                const Position& pos, int32_t damage, bool isPz, bool isBlocking) const;

    virtual void getDistanceShoot(const Creature* c, const Position& to,
                                  bool hasTarget) const
    {
        //this class shouldn't have any distance shoots, just return.
    }

    const uint32_t getOwnerID() const
    {
        return ownerid;
    }
    void setOwnerID(uint32_t owner)
    {
        ownerid=owner;
    }


protected:
    uint32_t ownerid;
};


class CreatureCondition
{
public:
    CreatureCondition(int32_t ticks, int32_t count, const MagicEffectTargetCreatureCondition& magicTargetCondition)
    {
        this->delayTicks = ticks;
        this->count = count;
        this->magicTargetCondition = magicTargetCondition;
        this->internalTicks = delayTicks;
    }

    const MagicEffectTargetCreatureCondition* getCondition() const
    {
        return &magicTargetCondition;
    }

    bool onTick(int32_t tick)
    {
        internalTicks -= tick;
        if(internalTicks <= 0 && count > 0)
        {
            internalTicks = delayTicks;
            --count;
            return true;
        }

        return false;
    }

    int32_t getCount() const
    {
        return count;
    };

private:
    int32_t delayTicks;
    int32_t count;
    int32_t internalTicks;
    MagicEffectTargetCreatureCondition magicTargetCondition;
};

//<<delayTicks, conditionCount>, MagicEffectTargetCreatureCondition>
typedef std::vector<CreatureCondition> ConditionVec;

//<duration, ConditionVec>
typedef std::pair<int32_t, ConditionVec> TransformItem;;

//<type, <duration, <<delayTicks, conditionCount>, MagicEffectTargetCreatureCondition>> >
typedef std::map<uint16_t, TransformItem> TransformMap;

//Needs target, holds a damage list. Example: Soul fire.
class MagicEffectTargetExClass : public MagicEffectTargetClass
{
public:
    MagicEffectTargetExClass(const ConditionVec& condition);
    virtual ~MagicEffectTargetExClass() {};

    virtual int32_t getDamage(Creature *target, const Creature *attacker = NULL) const;

protected:
    ConditionVec condition;
};

//magic field (Fire/Energy/Poison) and solid objects (Magic-wall/Wild growth)
class MagicEffectItem : public Item, public MagicEffectClass
{
public:
    MagicEffectItem(const TransformMap& transformMap);

    virtual void useThing()
    {
        //std::cout << "Magic: useThing() " << this << std::endl;
        useCount++;
    };

    virtual void releaseThing()
    {
        //std::cout << "Magic: releaseThing() " << this << std::endl;
        useCount--;
        //if (useCount == 0)
        if (useCount <= 0)
            delete this;
    };

    const MagicEffectTargetCreatureCondition* getCondition() const;

    virtual bool causeExhaustion(bool hasTarget) const
    {
        return false;
    }

    virtual int32_t getDamage(Creature *target, const Creature *attacker = NULL) const;

    virtual Item* decay();
    bool transform(const MagicEffectItem *rhs);
    int32_t getDecayTime();

protected:
    int32_t useCount;
    void buildCondition();
    TransformMap transformMap;
    ConditionVec condition;
};

//Create a solid object. Example: Magic wall, Wild growth
class MagicEffectTargetGroundClass : public MagicEffectTargetClass
{
public:
    MagicEffectTargetGroundClass(MagicEffectItem* fieldItem);
    virtual ~MagicEffectTargetGroundClass();

    virtual bool causeExhaustion(bool hasTarget) const
    {
        return true;
    }

    virtual int32_t getDamage(Creature *target, const Creature *attacker = NULL) const
    {
        return 0;
    }

    virtual void getMagicEffect(Player* spectator, const Creature* attacker, const Creature* target,
                                const Position& pos, int32_t damage, bool isPz, bool isBlocking) const;

    virtual void getDistanceShoot(Player* spectator, const Creature* attacker, const Position& to,
                                  bool hasTarget) const;

    virtual MagicEffectItem* getMagicItem(const Creature* attacker, bool isPz, bool isBlocking) const;

    virtual bool canCast(bool isBlocking, bool hasCreature) const;

    virtual void FailedToCast(Player* spectator, const Creature* attacker,
                              bool isBlocking, bool hasTarget) const;

protected:
    MagicEffectItem* magicItem;
};

//Don't need a target. Example: GFB
class MagicEffectAreaClass : public MagicEffectClass
{
public:
    MagicEffectAreaClass();
    virtual ~MagicEffectAreaClass() {};

    virtual bool causeExhaustion(bool hasTarget) const
    {
        return true;
    }

    virtual void getMagicEffect(Player* spectator, const Creature* attacker, const Creature* target,
                                const Position& pos, int32_t damage, bool isPz, bool isBlocking) const;

    virtual void getArea(const Position& rcenterpos, MagicAreaVec& list) const;

    unsigned char direction;
    unsigned char areaEffect;

    std::vector< std::vector<unsigned char> > areaVec;
};

//Dont need target. Example: Poison storm
class MagicEffectAreaExClass : public MagicEffectAreaClass
{
public:
    MagicEffectAreaExClass(const ConditionVec& dmglist);
    virtual ~MagicEffectAreaExClass() {};

    virtual int32_t getDamage(Creature *target, const Creature *attacker = NULL) const;

protected:
    ConditionVec condition;
};

//Don't need a target. Example: Fire bomb
class MagicEffectAreaGroundClass : public MagicEffectAreaClass
{
public:
    MagicEffectAreaGroundClass(MagicEffectItem* fieldItem);
    virtual ~MagicEffectAreaGroundClass();

    virtual int32_t getDamage(Creature *target, const Creature *attacker = NULL) const;

    virtual MagicEffectItem* getMagicItem(const Creature* attacker, bool isPz, bool isBlocking) const;

protected:

    MagicEffectItem* magicItem;
};
#include "creature.h"
#endif //__MAGIC_H__

