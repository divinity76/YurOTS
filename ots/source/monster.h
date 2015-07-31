
#ifndef __monster_h_
#define __monster_h_

#include "creature.h"
#include "game.h"
#include "tile.h"
#include "templates.h"
#include "monsters.h"

class Creature;

enum monsterstate_t
{
    STATE_IDLE,
    STATE_IDLESUMMON,
    STATE_TARGETNOTREACHABLE,
    STATE_ATTACKING,
    STATE_FLEEING,
};

enum monstermode_t
{
    MODE_NORMAL,
    MODE_AGGRESSIVE
};

class Monster : public Creature
{
private:
    Monster(MonsterType *mtype, Game* game);
    bool isCreatureReachable(const  Creature* creature);
public:
    static Monster* createMonster(const std::string& name, Game* game);
    virtual void setAttackedCreature(const Creature* creature);
    Creature* findTarget(int32_t range, bool &canReach, const Creature *ignoreCreature = NULL);
    void selectTarget(const Creature* creature, bool canReach /* = true*/);
    void changeTarget(const Creature* target);
    Monster(const std::string& name, Game* game);
    virtual ~Monster();
    //const Monster& operator=(const Monster& rhs);
    virtual uint32_t idRange()
    {
        return 0x40000000;
    }
    static AutoList<Monster> listMonster;
    void removeList()
    {
        listMonster.removeList(getID());
    }
    void addList()
    {
        listMonster.addList(this);
    }

    virtual void useThing()
    {
        //std::cout << "Monster: useThing() " << this << std::endl;
        useCount++;
    };

    virtual void releaseThing()
    {
        //std::cout << "Monster: releaseThing() " << this << std::endl;
        useCount--;
        if (useCount == 0)
            delete this;
    };

    virtual int32_t getArmor() const;
    virtual int32_t getDefense() const;
    virtual const std::string& getName() const;

    virtual void setMaster(Creature* creature);
    bool isSummon()
    {
        return (getMaster() != NULL);
    }
    virtual void onAttack();
    static uint32_t getRandom();
    virtual void onThingAppear(const Thing* thing);

private:
    Game* game;
    std::list<Position> route;
    monsterstate_t state;
    bool updateMovePos;
    int32_t oldThinkTicks;
    Position targetPos;
    Position moveToPos;
    bool hasLostMaster;
    MonsterType *mType;

    void doMoveTo(int32_t dx, int32_t dy);
    int32_t getCurrentDistanceToTarget(const Position &target);
    int32_t getTargetDistance();
    void setUpdateMovePos();
    bool calcMovePosition();
    void updateLookDirection();

    bool getRandomPosition(const Position &target, Position &dest);
    bool getDistancePosition(const Position &target, const int32_t& maxTryDist, bool fullPathSearch, Position &dest);
    bool getCloseCombatPosition(const Position &target, Position &dest);
    bool canMoveTo(uint16_t x, uint16_t y, unsigned char z);
    bool isInRange(const Position &pos);

    void stopAttack();
    void startThink();
    void stopThink();
    void reThink(bool updateOnlyState = true);


protected:
    int32_t useCount;
    PhysicalAttackClass	*curPhysicalAttack;

    bool doAttacks(Creature* attackedCreature, monstermode_t mode = MODE_NORMAL);

    virtual fight_t getFightType()
    {
        return curPhysicalAttack->fighttype;
    };
    virtual subfight_t getSubFightType()
    {
        return curPhysicalAttack->disttype;
    }
    virtual int32_t getWeaponDamage() const;

    void onCreatureEnter(const Creature *creature, bool canReach = true);
    void onCreatureLeave(const Creature *creature);
    void onCreatureMove(const Creature *creature, const Position *oldPos);

    bool validateDistanceAttack(const Creature *creature);
    bool validateDistanceAttack(const Position &pos);
    bool monsterMoveItem(Item* item, int32_t radius);
    bool isCreatureAttackable(const Creature* creature);

    virtual exp_t getLostExperience();

    virtual void dropLoot(Container *corpse);

    virtual void onThingMove(const Creature *creature, const Thing *thing, const Position *oldPos,
                             unsigned char oldstackpos, unsigned char oldcount, unsigned char count);

    virtual void onCreatureAppear(const Creature *creature);
    virtual void onCreatureDisappear(const Creature *creature, unsigned char stackPos, bool tele);
    virtual void onThingDisappear(const Thing* thing, unsigned char stackPos);
    virtual void onThingTransform(const Thing* thing,int32_t stackpos);
    virtual void onTeleport(const Creature *creature, const Position *oldPos, unsigned char oldstackpos);

    virtual bool isAttackable() const
    {
        return true;
    };
    virtual bool isPushable() const;

    virtual int32_t onThink(int32_t& newThinkTicks);

    std::string getDescription(bool self) const;

protected:
    //nothing ;(
};

#endif // __monster_h_
