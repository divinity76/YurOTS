#ifndef __monsters_h_
#define __monsters_h_

#include <string>

#include "creature.h"

class TimeProbabilityClass
{
public:
    TimeProbabilityClass()
    {
        setDefault();
    }

    TimeProbabilityClass(int32_t _cycleTicks, int32_t _probability, int32_t _exhaustionticks)
    {
        setDefault();
        init(_cycleTicks, _probability, _exhaustionticks);
    };

    ~TimeProbabilityClass() {};

    bool onTick(int32_t ticks)
    {
        ticksleft -= ticks;

        if(ticksleft <= 0)
        {
            ticksleft = cycleTicks;
            bool ret = (random_range(1, 100) <= probability);
            return ret;
        }

        return false;
    }

    void init(int32_t _cycleTicks, int32_t _probability, int32_t _exhaustionticks)
    {
        if(_cycleTicks >= 0)
        {
            this->ticksleft = _cycleTicks;
            this->cycleTicks = _cycleTicks;
        }

        if(_probability >= 0)
            probability = std::min(100, _probability);

        if(_exhaustionticks >= 0)
            exhaustionTicks = _exhaustionticks;
    }

    int32_t getExhaustion() const
    {
        return exhaustionTicks;
    }

private:
    void setDefault()
    {
        cycleTicks = 2000;
        ticksleft = cycleTicks;
        probability = 80;
        exhaustionTicks = 0;
    }

    int32_t ticksleft;
    int32_t cycleTicks;
    int32_t probability;
    int32_t exhaustionTicks;
};

class PhysicalAttackClass
{
public:
    PhysicalAttackClass()
    {
        disttype = DIST_NONE;
        minWeapondamage = 0;
        maxWeapondamage = 1;
    };

    ~PhysicalAttackClass() {};

    fight_t fighttype;
    subfight_t disttype;

    int32_t minWeapondamage;
    int32_t maxWeapondamage;
};

#define CHANCE_MAX  100000
struct LootBlock
{
    uint16_t id;
    uint16_t countmax;
    uint32_t chance1;
    uint32_t chancemax;
    typedef std::list<LootBlock> LootItems;
    LootItems childLoot;
    LootBlock()
    {
        id = 0;
        countmax = 0;
        chance1 = 0;
        chancemax = 0;
    }
};

struct summonBlock
{
    std::string name;
    uint32_t summonChance;
};

typedef std::list<LootBlock> LootItems;
typedef std::vector<TimeProbabilityClass> TimeProbabilityClassVec;
typedef std::map<std::string, TimeProbabilityClassVec> InstantAttackSpells;
typedef std::map<uint16_t, TimeProbabilityClassVec> RuneAttackSpells;
typedef std::map<PhysicalAttackClass*, TimeProbabilityClass> PhysicalAttacks;
typedef std::vector<std::pair<std::string, TimeProbabilityClass> > YellingSentences;
typedef std::list<summonBlock> SummonSpells;

class MonsterType
{
public:
    MonsterType();
    ~MonsterType();

    void reset();

    std::string name;
    exp_t experience;
    int32_t armor;
    int32_t defense;
    bool hasDistanceAttack;
    bool canPushItems;
    uint32_t staticLook;
    uint32_t staticAttack;
    uint16_t changeTargetChance;
    int32_t maxSummons;
    int32_t targetDistance;
    int32_t runAwayHealth;
    bool pushable;
    int32_t base_speed;
    int32_t level;
    int32_t maglevel;
    int32_t health;
    int32_t health_max;
    int32_t lookhead, lookbody, looklegs, lookfeet, looktype, lookcorpse, lookmaster;
    int32_t immunities;

    int32_t bloodcolor;
    unsigned char bloodeffect;
    unsigned char bloodsplash;

    InstantAttackSpells instantSpells;
    RuneAttackSpells runeSpells;
    PhysicalAttacks physicalAttacks;
    YellingSentences yellingSentences;
    SummonSpells summonSpells;

    LootItems lootItems;

    void createLoot(Container* corpse);
    void createLootContainer(Container* parent, const LootBlock& lootblock);
    Item* createLootItem(const LootBlock& lootblock);
};

class Monsters
{
public:
    Monsters();
    ~Monsters();

    bool loadFromXml(const std::string &_datadir, bool reloading = false);
    bool isLoaded()
    {
        return loaded;
    }
    bool reload();

    MonsterType* getMonsterType(uint32_t mid);
    uint32_t getIdByName(const std::string& name);

private:
    MonsterType* loadMonster(const std::string& file, const std::string& monster_name, bool reloading = false);
    bool loadLootContainer(xmlNodePtr, LootBlock&);
    bool loadLootItem(xmlNodePtr, LootBlock&);
    bool isCreatureReachable(const  Creature* creature);

    typedef std::map<std::string, uint32_t> MonsterNameMap;
    MonsterNameMap monsterNames;

    typedef std::map<uint32_t, MonsterType*> MonsterMap;
    MonsterMap monsters;

    bool loaded;
    std::string datadir;

};

#endif
