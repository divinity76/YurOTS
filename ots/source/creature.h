#ifndef __creature_h
#define __creature_h

#include "thing.h"
#include "position.h"
#include "container.h"
#include "magic.h"

#include <vector>

#include "templates.h"
#include "luascript.h"
extern LuaScript g_config;

#ifdef YUR_HIGH_LEVELS
typedef int64_t exp_t;
#else
typedef uint32_t exp_t;
#endif //YUR_HIGH_LEVELS


typedef std::vector<Creature*> CreatureVector;

enum slots_t
{
    SLOT_WHEREEVER=0,
    SLOT_HEAD=1,
    SLOT_NECKLACE=2,
    SLOT_BACKPACK=3,
    SLOT_ARMOR=4,
    SLOT_RIGHT=5,
    SLOT_LEFT=6,
    SLOT_LEGS=7,
    SLOT_FEET=8,
    SLOT_RING=9,
    SLOT_AMMO=10,
    SLOT_DEPOT=11
};

enum fight_t
{
    FIGHT_MELEE,
    FIGHT_DIST,
    FIGHT_MAGICDIST
};

// Macros
#define CREATURE_SET_OUTFIT(c, type, head, body, legs, feet) c->looktype = type; \
	c->lookhead = head; \
	c->lookbody = body; \
	c->looklegs = legs; \
c->lookfeet = feet;

enum playerLooks
{
    PLAYER_MALE_1=0x80,
    PLAYER_MALE_2=0x81,
    PLAYER_MALE_3=0x82,
    PLAYER_MALE_4=0x83,
    PLAYER_MALE_5=0x84,
    PLAYER_MALE_6=0x85,
    PLAYER_MALE_7=0x86,
    PLAYER_FEMALE_1=0x88,
    PLAYER_FEMALE_2=0x89,
    PLAYER_FEMALE_3=0x8A,
    PLAYER_FEMALE_4=0x8B,
    PLAYER_FEMALE_5=0x8C,
    PLAYER_FEMALE_6=0x8D,
    PLAYER_FEMALE_7=0x8E,
    PLAYER_DWARF=0xA0,
    PLAYER_NIMFA=0x90,
};


class Map;

class Item;

class Thing;
class Player;
class Monster;

class Conditions : public std::map<attacktype_t, ConditionVec>
{
public:
    bool hasCondition(attacktype_t type)
    {
        Conditions::iterator condIt = this->find(type);
        if(condIt != this->end() && !condIt->second.empty())
        {
            return true;
        }

        return false;
    }
};


//////////////////////////////////////////////////////////////////////
// Defines the Base class for all creatures and base functions which
// every creature has

class Creature : public AutoID, public Thing
{
public:
    //Creature(const std::string& name);
    Creature();
    virtual ~Creature();

    virtual const std::string& getName() const = 0;

    void setID()
    {
        this->id = auto_id | this->idRange();
    }
    virtual uint32_t idRange() = 0;
    uint32_t getID() const
    {
        return id;
    }
    virtual void removeList() = 0;
    virtual void addList() = 0;

    exp_t getExpForLv(const int32_t& lv) const
    {
#ifdef YUR_HIGH_LEVELS
        exp_t x = lv;
        return ((50*x/3 - 100)*x + 850/3)*x - 200;
#else
        return (int32_t)((50*lv*lv*lv)/3 - 100 * lv * lv + (850*lv) / 3 - 200);
#endif //YUR_HIGH_LEVELS
    }

    Direction getDirection() const
    {
        return direction;
    }
    void setDirection(Direction dir)
    {
        direction = dir;
    }

#ifdef HUCZU_FOLLOW
    uint32_t followCreature;
    std::list<Position> pathList;
    uint32_t eventCheckFollow;
#endif

    virtual fight_t getFightType()
    {
        return FIGHT_MELEE;
    };
    virtual subfight_t getSubFightType()
    {
        return DIST_NONE;
    }
    virtual Item* getDistItem()
    {
        return NULL;
    };
    virtual void removeDistItem()
    {
        return;
    }
    virtual int32_t getImmunities() const
    {
        if(access >= g_config.ACCESS_PROTECT)
            return  ATTACK_ENERGY | ATTACK_BURST | ATTACK_FIRE |
                    ATTACK_PHYSICAL | ATTACK_POISON | ATTACK_PARALYZE | ATTACK_DRUNKNESS;
        else
            return immunities;
    };

#ifdef YUR_PVP_ARENA
    virtual void drainHealth(int64_t, CreatureVector* arenaLosers);
#else
    virtual void drainHealth(int64_t);
#endif //YUR_PVP_ARENA

    virtual void drainMana(int64_t);
    virtual void die() {};
    virtual std::string getDescription(bool self = false) const;
    virtual void setAttackedCreature(const Creature* creature);
    //virtual void setAttackedCreature(uint32_t id);

    virtual void setMaster(Creature* creature);
    virtual Creature* getMaster()
    {
        return master;
    }

    virtual void addSummon(Creature *creature);
    virtual void removeSummon(Creature *creature);

    virtual int64_t getWeaponDamage() const
    {
        return 1+(int64_t)(10.0*rand()/(RAND_MAX+1.0));
    }
    virtual int64_t getArmor() const
    {
        return 0;
    }
    virtual int64_t getDefense() const
    {
        return 0;
    }

    uint32_t attackedCreature;

    virtual bool isAttackable() const
    {
        return true;
    };
    virtual bool isPushable() const
    {
        return true;
    }
    virtual void dropLoot(Container *corpse)
    {
        return;
    };
    virtual int32_t getLookCorpse()
    {
        return lookcorpse;
    };

    //  virtual int32_t sendInventory(){return 0;};
    virtual int32_t addItemInventory(Item* item, int32_t pos)
    {
        return 0;
    };
    virtual Item* getItem(int32_t pos)
    {
        return NULL;
    }
    virtual Direction getDirection()
    {
        return direction;
    }
    void addCondition(const CreatureCondition& condition, bool refresh);
    Conditions& getConditions()
    {
        return conditions;
    };
    void removeCondition(attacktype_t attackType);

    int32_t lookhead, lookbody, looklegs, lookfeet, looktype, lookcorpse, lookmaster;
    int64_t mana, manamax, manaspent;
    bool pzLocked;

    int32_t inFightTicks, exhaustedTicks;
    int32_t manaShieldTicks, hasteTicks, paralyzeTicks;
    int32_t dwarvenTicks;
    int32_t immunities;

    //uint32_t experience;
    Position masterPos;

    int64_t health, healthmax;
    uint64_t lastmove;

    int32_t bloodcolor;
    unsigned char bloodeffect;
    unsigned char bloodsplash;

    int64_t getSleepTicks() const;
    int32_t getStepDuration() const;

    int32_t getSpeed() const
    {
        return speed;
    };

    void setNormalSpeed()
    {
        if(access >= g_config.ACCESS_PROTECT)
        {
            speed = 1000;
            return;
        }
        speed = getNormalSpeed();
    }

    int32_t getNormalSpeed()
    {
        if(access >= g_config.ACCESS_PROTECT)
        {
            return 1000;
        }
#ifdef YUR_BOH
#ifdef YUR_RINGS_AMULETS
        return std::min(900, 220 + (2* (level + 30*boh + 30*timeRing - 1)));
#else //YUR_RINGS_AMULETS
        return std::min(900, 220 + (2* (level + 30*boh - 1)));
#endif //YUR_RINGS_AMULETS
#else //YUR_BOH
#ifdef YUR_RINGS_AMULETS
        return std::min(900, 220 + (2* (level + 30*timeRing - 1)));
#else //YUR_RINGS_AMULETS
        return std::min(900, 220 + (2* (level - 1)));
#endif //YUR_RINGS_AMULETS
#endif //YUR_BOH
    }

    int32_t access;		//access level
    int64_t maglevel;	// magic level
    int32_t level;		// level
    int32_t speed;
    int32_t hasteSpeed;
    bool oneSiteAttack;
#ifdef HUCZU_EXHAUSTED
    int32_t mmo;
    int32_t lookex;
    int32_t antyrainbow;
    int32_t antyrainbow2;
#endif //HUCZU_EXHAUSTED
    Direction direction;

    virtual bool canMovedTo(const Tile *tile) const;

    virtual void sendCancel(const char *msg) const { };
    //virtual void sendCancelWalk(const char *msg) const { };

    virtual void addInflictedDamage(Creature* attacker, int64_t damage);
    virtual exp_t getGainedExperience(Creature* attacker);
    virtual std::vector<int32_t> getInflicatedDamageCreatureList();//->first is creature ID...
    virtual exp_t getLostExperience();
    virtual int64_t getInflicatedDamage(Creature* attacker);
    virtual int64_t getTotalInflictedDamage();
    virtual int64_t getInflicatedDamage(uint32_t id);
#ifdef TR_SUMMONS
    size_t getSummonCount() const
    {
        return summons.size();
    }
    bool isPlayersSummon() const;
#endif //TR_SUMMONS

    skull_t skullType;

    bool isInvisible() const
    {
        return invisibleTicks > 0;
    }
    void setInvisible(int32_t ticks)
    {
        invisibleTicks = ticks;
    }
    bool checkInvisible(int32_t thinkTicks);

protected:

#ifdef YUR_BOH
    bool boh;
#endif //YUR_BOH
#ifdef YUR_RINGS_AMULETS
    bool timeRing;
#endif //YUR_RINGS_AMULETS
    int32_t invisibleTicks;

    uint32_t eventCheck;
    uint32_t eventCheckAttacking;

    Creature *master;
    std::list<Creature*> summons;

    Conditions conditions;
    typedef std::vector< std::pair<uint64_t, int64_t> > DamageList;//->second is damage.. ->first ? wish i knew...
    typedef std::map<int32_t, DamageList > TotalDamageList; //->first is creature id..
    TotalDamageList totaldamagelist;

protected:
    virtual int32_t onThink(int32_t& newThinkTicks)
    {
        newThinkTicks = 300;
        return 300;
    };
    virtual void onThingMove(const Creature *player, const Thing *thing, const Position *oldPos,
                             unsigned char oldstackpos, unsigned char oldcount, unsigned char count) { };

    virtual void onCreatureAppear(const Creature *creature) { };
    virtual void onCreatureDisappear(const Creature *creature, unsigned char stackPos, bool tele = false) { };
    virtual void onThingDisappear(const Thing* thing, unsigned char stackPos) = 0;
    virtual void onThingTransform(const Thing* thing,int32_t stackpos) = 0;
    virtual void onThingAppear(const Thing* thing) = 0;
    virtual void onCreatureTurn(const Creature *creature, unsigned char stackPos) { };
    virtual void onCreatureSay(const Creature *creature, SpeakClasses type, const std::string &text) { };

    virtual void onCreatureChangeOutfit(const Creature* creature) { };
    virtual void onTileUpdated(const Position &pos) { };

    virtual void onTeleport(const Creature *creature, const Position *oldPos, unsigned char oldstackpos) { };

    //container to container
    virtual void onThingMove(const Creature *creature, const Container *fromContainer, uint16_t from_slotid,
                             const Item* fromItem, int32_t oldFromCount, Container *toContainer, uint16_t to_slotid,
                             const Item *toItem, int32_t oldToCount, int32_t count) {};

    //inventory to container
    virtual void onThingMove(const Creature *creature, slots_t fromSlot, const Item* fromItem,
                             int32_t oldFromCount, const Container *toContainer, uint16_t to_slotid, const Item *toItem, int32_t oldToCount, int32_t count) {};

    //inventory to inventory
    virtual void onThingMove(const Creature *creature, slots_t fromSlot, const Item* fromItem,
                             int32_t oldFromCount, slots_t toSlot, const Item* toItem, int32_t oldToCount, int32_t count) {};

    //container to inventory
    virtual void onThingMove(const Creature *creature, const Container *fromContainer, uint16_t from_slotid,
                             const Item* fromItem, int32_t oldFromCount, slots_t toSlot, const Item *toItem, int32_t oldToCount, int32_t count) {};

    //container to ground
    virtual void onThingMove(const Creature *creature, const Container *fromContainer, unsigned char from_slotid,
                             const Item* fromItem, int32_t oldFromCount, const Position &toPos, const Item *toItem, int32_t oldToCount, int32_t count) {};

    //inventory to ground
    virtual void onThingMove(const Creature *creature, slots_t fromSlot,
                             const Item* fromItem, int32_t oldFromCount, const Position &toPos, const Item *toItem, int32_t oldToCount, int32_t count) {};

    //ground to container
    virtual void onThingMove(const Creature *creature, const Position &fromPos, int32_t stackpos, const Item* fromItem,
                             int32_t oldFromCount, const Container *toContainer, unsigned char to_slotid, const Item *toItem, int32_t oldToCount, int32_t count) {};

    //ground to inventory
    virtual void onThingMove(const Creature *creature, const Position &fromPos, int32_t stackpos, const Item* fromItem,
                             int32_t oldFromCount, slots_t toSlot, const Item *toItem, int32_t oldToCount, int32_t count) {};

    friend class Game;
    friend class Map;
    friend class Commands;
    friend class GameState;

    uint32_t id;
    //std::string name;
};


#endif // __creature_h
