#ifndef __OTSERV_GAME_H
#define __OTSERV_GAME_H

#include "definitions.h"
#include <queue>
#include <vector>
#include <set>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

#include "spawn.h"
#include "position.h"
#include "item.h"
#include "container.h"
#include "magic.h"
#include "map.h"
#include "templates.h"
#include "loginqueue.h"

class Creature;   // see creature.h
class Player;
class Monster;
class Npc;
class Commands;
class SchedulerTask;
class lessSchedTask;

#define MAP_WIDTH    512
#define MAP_HEIGHT   512
#define MAP_LAYER     16

#ifdef CVS_DAY_CYCLE
enum eLightState
{
    LIGHT_STATE_DAY,
    LIGHT_STATE_NIGHT,
    LIGHT_STATE_SUNSET,
    LIGHT_STATE_SUNRISE,
};
#endif //CVS_DAY_CYCLE

/** State of a creature at a given time
  * Keeps track of all the changes to be able to send to the client
	*/

class CreatureState
{
public:
    CreatureState() {};
    ~CreatureState() {};

    int32_t damage;
    int32_t manaDamage;
    bool drawBlood;
    std::vector<Creature*> attackerlist;
};

typedef std::vector< std::pair<Creature*, CreatureState> > CreatureStateVec;
typedef std::map<Tile*, CreatureStateVec> CreatureStates;
#ifdef HUCZU_STAGE_EXP
typedef std::map<int32_t, uint32_t> StageMap; //<lvl,multipler>
typedef std::map<int32_t, uint32_t> StageMapEnfo; //<lvl,multipler>
#endif
#ifdef HUCZU_NAPISY
struct napisow
{
    std::string text;
    Position pos;
    uint16_t kolor;
};
typedef std::map<uint32_t, struct napisow> NapisyMap;
#endif
/** State of the game at a given time
  * Keeps track of all the changes to be able to send to the client
	*/

class Game;

class GameState
{
public:
    GameState(Game *game, const Range &range);
    ~GameState() {};

    void onAttack(Creature* attacker, const Position& pos, const MagicEffectClass* me);
    void onAttack(Creature* attacker, const Position& pos, Creature* attackedCreature);
    const CreatureStateVec& getCreatureStateList(Tile* tile)
    {
        return creaturestates[tile];
    };
    const SpectatorVec& getSpectators()
    {
        return spectatorlist;
    }

protected:
    void addCreatureState(Tile* tile, Creature* attackedCreature, int32_t damage, int32_t manaDamage, bool drawBlood);
    void onAttackedCreature(Tile* tile, Creature* attacker, Creature* attackedCreature, int32_t damage, bool drawBlood);
    Game *game;

#ifdef YUR_PVP_ARENA
    bool isPvpArena(Creature* c);
#endif //YUR_PVP_ARENA
#ifdef YUR_RINGS_AMULETS
    int32_t applyAmulets(Player* player, int32_t damage, attacktype_t atype);
#endif //YUR_RINGS_AMULETS

    SpectatorVec spectatorlist;
    CreatureStates creaturestates;
};

enum enum_world_type
{
    WORLD_TYPE_NO_PVP,
    WORLD_TYPE_PVP,
    WORLD_TYPE_PVP_ENFORCED
};

enum enum_game_state
{
    GAME_STATE_NORMAL,
    GAME_STATE_CLOSED,
    GAME_STATE_SHUTDOWN
};

/**
  * Main Game class.
  * This class is responsible to controll everything that happens
  */

class Game
{
public:
    Game();
    ~Game();

    int32_t loadMap(std::string filename, std::string filekind);
    void checkShopItems(Player* player);
    bool useHotkey(Player* player, int32_t itemid, int32_t count);
#ifdef HUCZU_NAPISY
    bool loadNapisy();
    void checkNapisy();
#endif
#ifdef HUCZU_PAY_SYSTEM
    bool checkHomepay(Player* player, std::string kod, std::string usluga);
    void logPunktow(Player* player, std::string kod, std::string usluga);
#endif
#ifdef HUCZU_STAGE_EXP
    uint32_t getStageExp(int32_t level, bool enfo);
    bool loadStageExp();
#endif
#ifdef HUCZU_BAN_SYSTEM
    void banPlayer(Player *player, std::string reason, std::string action, std::string comment, unsigned char IPban);
#endif //HUCZU_BAN_SYSTEM
#ifdef HUCZU_PARCEL_SYSTEM
    bool sendParcel(Player* player, Item* parcel, Position pos);
#endif
#ifdef HUCZU_FOLLOW
    void checkCreatureFollow(uint32_t id);
    void playerFollow(Player* player, Creature *followCreature);
    void playerSetFollowCreature(Player* player, uint32_t creatureid);
    void autoCloseFollow(Player* player, Creature* target);
#endif //HUCZU_FOLLOW
#ifdef RAID
    bool loadRaid(std::string name);
    bool placeRaidMonster(std::string name, int32_t x, int32_t y, int32_t z);
#endif
#ifdef HUCZU_RECORD
    void checkRecord();
    uint32_t record;
#endif
    //void addBan(Player* player, Player* bannedPlayer, std::string reason, std::string action, std::string comment, unsigned char IPBan);
#ifdef __MIZIAK_SUPERMANAS__
    void fullManas(Item* item, Player* player, const Position& posFrom, const unsigned char stack_from);
    void sendMagicEffectToSpectors(Position& pos, int32_t efekt);
#endif //__MIZIAK_SUPERMANAS__
#ifdef HUCZU_RRV
    std::vector<Player*> openViolations;
#endif
    bool loadNpcs();
    int32_t checkOwner();
    int32_t checkHouseOwners(uint32_t days);
    void updateTile(const Position& pos);
    bool loadReadables();
    Map* map;
    void changeOutfit(uint32_t id, int32_t looktype);
    void getSpectators(const Range& range, SpectatorVec& list);
    /**
      * Get the map size - info purpose only
      * \param a the referenced witdh var
      * \param b the referenced height var
      */
    void getMapDimensions(int32_t& a, int32_t& b)
    {
        a = map->mapwidth;
        b = map->mapheight;
        return;
    }

    void setWorldType(enum_world_type type);
    enum_world_type getWorldType() const
    {
        return worldType;
    }
    const std::string& getSpawnFile()
    {
        return map->spawnfile;
    }

    /**
      * Get a single tile of the map.
      * \returns A Pointer to the tile */
    Tile* getTile(uint16_t _x, uint16_t _y, unsigned char _z);
    Tile* getTile(const Position& pos);

    /**
      * Set a Tile to a specific ground id
      * \param groundId ID of the ground to set
      */
    void setTile(uint16_t _x, uint16_t _y, unsigned char _z, uint16_t groundId);

    /**
      * Place Creature on the map.
      * Adds the Creature to playersOnline and to the map
      * \param c Creature to add
      */
#ifdef YUR_LOGIN_QUEUE
    bool placeCreature(Position &pos, Creature* c, int32_t* placeInQueue = NULL);
#else
    bool placeCreature(Position &pos, Creature* c);
#endif //YUR_LOGIN_QUEUE

    /**
    	* Remove Creature from the map.
    	* Removes the Creature the map
    	* \param c Creature to remove
    	*/
    bool removeCreature(Creature* c);

    uint32_t getPlayersOnline();
    uint32_t getMonstersOnline();
    uint32_t getNpcsOnline();
    uint32_t getCreaturesOnline();

    void thingMove(Creature *creature, Thing *thing,
                   uint16_t to_x, uint16_t to_y, unsigned char to_z, unsigned char count);

    //container/inventory to container/inventory
    void thingMove(Player *player,
                   uint16_t from_cid, uint16_t from_slotid, uint16_t itemid,bool fromInventory,
                   uint16_t to_cid, uint16_t to_slotid, bool toInventory,
                   uint16_t count);

    //container/inventory to ground
    void thingMove(Player *player,
                   unsigned char from_cid, unsigned char from_slotid, uint16_t itemid, bool fromInventory,
                   const Position& toPos, unsigned char count);

    //ground to container/inventory
    void thingMove(Player *player,
                   const Position& fromPos, unsigned char stackPos, uint16_t itemid,
                   unsigned char to_cid, unsigned char to_slotid,
                   bool isInventory, unsigned char count);

    //ground to ground
    void thingMove(Creature *creature,
                   uint16_t from_x, uint16_t from_y, unsigned char from_z,
                   unsigned char stackPos,uint16_t itemid,
                   uint16_t to_x, uint16_t to_y, unsigned char to_z, unsigned char count);

    /**
    	* Creature wants to turn.
    	* \param creature Creature pointer
    	* \param dir Direction to turn to
    	*/
    void creatureTurn(Creature *creature, Direction dir);
    static double NO_VOCATION_SPEED, SORCERER_SPEED, DRUID_SPEED, PALADIN_SPEED, KNIGHT_SPEED;
    /**
      * Creature wants to say something.
      * \param creature Creature pointer
      * \param type Type of message
      * \todo document types
      * \param text The text to say
      */
    void creatureSay(Creature *creature, SpeakClasses type, const std::string &text);

    void creatureWhisper(Creature *creature, const std::string &text);
    int32_t getDepot(Container* c, int32_t e);
    void creatureYell(Creature *creature, std::string &text);
    void creatureSpeakTo(Creature *creature, SpeakClasses type, const std::string &receiver, const std::string &text);
    void creatureBroadcastMessage(Creature *creature, const std::string &text);
    void creatureTalkToChannel(Player *player, SpeakClasses type, std::string &text, uint16_t channelId);
    void creatureSendToSpecialChannel(Player *player, SpeakClasses type, const std::string &text, uint16_t channelId, const std::string &info, bool alone = true);
    void creatureMonsterYell(Monster* monster, const std::string& text);
    void creatureChangeOutfit(Creature *creature);

    bool creatureThrowRune(Creature *creature, const Position& centerpos, const MagicEffectClass& me);
    bool creatureCastSpell(Creature *creature, const Position& centerpos, const MagicEffectClass& me);
    bool creatureSaySpell(Creature *creature, const std::string &text);

    void playerAutoWalk(Player* player, std::list<Direction>& path);
    bool playerUseItemEx(Player *player, const Position& posFrom,const unsigned char  stack_from,
                         const Position &posTo,const unsigned char stack_to, const uint16_t itemid);
    bool playerUseItem(Player *player, const Position& pos, const unsigned char stackpos, const uint16_t itemid, const unsigned char index);
    bool playerUseBattleWindow(Player *player, Position &posFrom, unsigned char stackpos, uint16_t itemid, uint32_t creatureid);
    bool playerRotateItem(Player *player, const Position& pos, const unsigned char stackpos, const uint16_t itemid);
    void playerRequestTrade(Player *player, const Position& pos,
                            const unsigned char stackpos, const uint16_t itemid, uint32_t playerid);
    void playerAcceptTrade(Player* player);
    void playerLookInTrade(Player* player, bool lookAtCounterOffer, int32_t index);
    void playerCloseTrade(Player* player);
    void autoCloseTrade(const Item* item, bool itemMoved = false);
    void autoCloseAttack(Player* player, Creature* target);

    void playerSetAttackedCreature(Player* player, uint32_t creatureid);

    void changeOutfitAfter(uint32_t id, int32_t looktype, int32_t time);
    void changeSpeed(uint32_t id, uint16_t speed);
    uint32_t addEvent(SchedulerTask*);
    bool stopEvent(uint32_t eventid);

    //void creatureBroadcastTileUpdated(const Position& pos);
    void teleport(Thing *thing, const Position& newPos);

    std::vector<Player*> BufferedPlayers;
    void flushSendBuffers();
    void addPlayerBuffer(Player* p);

    std::vector<Thing*> ToReleaseThings;
    void FreeThing(Thing* thing);

    Thing* getThing(const Position &pos,unsigned char stack,Player* player = NULL);
    void addThing(Player* player,const Position &pos,Thing* thing);
    bool removeThing(Player* player,const Position &pos,Thing* thing, bool setRemoved = true);
    Position getThingMapPos(Player *player, const Position &pos);

    void sendAddThing(Player* player,const Position &pos,const Thing* thing);
    void sendRemoveThing(Player* player,const Position &pos,const Thing* thing,const unsigned char stackpos = 1,const bool autoclose = false);
    void sendUpdateThing(Player* player,const Position &pos,const Thing* thing,const unsigned char stackpos = 1);

    Creature* getCreatureByID(uint32_t id);
    Player* getPlayerByID(uint32_t id);

    Creature* getCreatureByName(const std::string &s);
    Player* getPlayerByName(const std::string &s);


    std::list<Position> getPathTo(Creature *creature, Position start, Position to, bool creaturesBlock=true);
    std::list<Position> getPathToEx(Creature *creature, Position start, Position to, bool creaturesBlock=true);
    enum_game_state getGameState();
    void setGameState(enum_game_state newstate)
    {
        game_state = newstate;
    }


    /** Lockvar for Game. */
    OTSYS_THREAD_LOCKVAR gameLock;

#ifdef CVS_DAY_CYCLE
    void creatureChangeLight(Player* player, int32_t time, unsigned char lightlevel, unsigned char lightcolor);
    unsigned char getLightLevel();
#endif //CVS_DAY_CYCLE

    void serverSave();
    void autoServerSave();

#ifdef YUR_LOGIN_QUEUE
    LoginQueue loginQueue;
#endif //YUR_LOGIN_QUEUE

#ifdef TR_SUMMONS
    bool placeSummon(Player* p, const std::string& name);
#endif //TR_SUMMONS

#ifdef TRS_GM_INVISIBLE
    void creatureBroadcastTileUpdated(const Position& pos);
#endif //TRS_GM_INVISIBLE

    void vipLogin(Player* player);
    void vipLogout(std::string vipname);
    bool requestAddVip(Player* player, const std::string &vip_name);

#ifdef HUCZU_SKULLS
    void onPvP(Creature* creature, Creature* attacked, bool murder = false);
    void Skull(Player* player);
    void disbandParty(uint32_t partyID);
    void LeaveParty(Player *player);
    void checkSkullTime(Player* player);
#endif //HUCZU_SKULLS

    void burstArrow(Creature* c, const Position& pos);
    static double BURST_DMG_LVL, BURST_DMG_MLVL, BURST_DMG_LO, BURST_DMG_HI;

    void goldBolt(Creature* c, const Position& pos);
    static double GOLD_DMG_LVL, GOLD_DMG_MLVL, GOLD_DMG_LO, GOLD_DMG_HI;

    void silverWand(Creature* c, const Position& pos);
    static double SILVER_DMG_LVL, SILVER_DMG_MLVL, SILVER_DMG_LO, SILVER_DMG_HI;

    void beforeRestart();
    void sheduleShutdown(int32_t minutes);
    void checkShutdown(int32_t minutes);
    void setMaxPlayers(uint32_t newmax);
    int32_t cleanMap();
    void autocleanMap(int32_t seconds);
    void beforeClean();
    void CreateCondition(Creature* creature, Creature* target, unsigned char animationColor, unsigned char damageEffect, unsigned char hitEffect, attacktype_t attackType, bool offensive, int32_t maxDamage, int32_t minDamage, int32_t ticks, int32_t count);
    void doFieldDamage(Creature* creature, unsigned char animationColor, unsigned char damageEffect,  unsigned char hitEffect, attacktype_t attackType, bool offensive, int32_t damage);
    Creature* getCreatureByPosition(int32_t x, int32_t y, int32_t z);
    void useWand(Creature *creature, Creature *attackedCreature, int32_t wandid);
    void checkSpell(Player* player, SpeakClasses type, std::string text);

protected:
    std::map<Item*, uint32_t> tradeItems; //list of items that are in trading state, mapped to the player

    AutoList<Creature> listCreature;

#ifdef HUCZU_STAGE_EXP
    StageMap stages;
    StageMapEnfo stages_enfo;
    int32_t lastStageLevel, lastStageLevelEnfo;
#endif

#ifdef HUCZU_NAPISY
    NapisyMap napisy;
#endif

    /*ground -> ground*/
    bool onPrepareMoveThing(Creature *player, /*const*/ Thing* thing,
                            const Position& fromPos, const Position& toPos, int32_t count);

    /*ground -> ground*/
    bool onPrepareMoveThing(Creature *creature, const Thing* thing,
                            const Tile *fromTile, const Tile *toTile, int32_t count);

    /*inventory -> container*/
    bool onPrepareMoveThing(Player *player, const Item* fromItem, slots_t fromSlot,
                            const Container *toContainer, const Item *toItem, int32_t count);

    /*container -> container*/
    bool onPrepareMoveThing(Player* player, const Position& fromPos, const Container* fromContainer,
                            const Item* fromItem, const Position& toPos, const Container* toContainer, const Item* toItem,
                            int32_t count);

    /*ground -> ground*/
    bool onPrepareMoveCreature(Creature *creature, const Creature* creatureMoving,
                               const Tile *fromTile, const Tile *toTile);

    /*ground -> inventory*/
    bool onPrepareMoveThing(Player *player, const Position& fromPos, const Item *item,
                            slots_t toSlot, int32_t count);

    /*inventory -> inventory*/
    bool onPrepareMoveThing(Player *player, slots_t fromSlot, const Item *fromItem,
                            slots_t toSlot, const Item *toItem, int32_t count);

    /*container -> inventory*/
    bool onPrepareMoveThing(Player *player, const Container *fromContainer, const Item *fromItem,
                            slots_t toSlot, const Item *toItem, int32_t count);

    /*->inventory*/
    bool onPrepareMoveThing(Player *player, const Item *item, slots_t toSlot, int32_t count);

    //container/inventory to container/inventory
    void thingMoveInternal(Player *player,
                           uint16_t from_cid, uint16_t from_slotid, uint16_t itemid,
                           bool fromInventory,uint16_t to_cid, uint16_t to_slotid,
                           bool toInventory,uint16_t count);

    //container/inventory to ground
    void thingMoveInternal(Player *player,
                           unsigned char from_cid, unsigned char from_slotid, uint16_t itemid,
                           bool fromInventory,const Position& toPos, unsigned char count);

    //ground to container/inventory
    void thingMoveInternal(Player *player,
                           const Position& fromPos, unsigned char stackPos,uint16_t itemid,
                           unsigned char to_cid, unsigned char to_slotid,
                           bool toInventory, unsigned char count);

    // use this internal function to move things around to avoid the need of
    // recursive locks
    void thingMoveInternal(Creature *player,
                           uint16_t from_x, uint16_t from_y, unsigned char from_z,
                           unsigned char stackPos,uint16_t itemid,
                           uint16_t to_x, uint16_t to_y, unsigned char to_z, unsigned char count);


    bool creatureOnPrepareAttack(Creature *creature, Position pos);
    void creatureMakeDamage(Creature *creature, Creature *attackedCreature, fight_t damagetype);

    bool creatureMakeMagic(Creature *creature, const Position& centerpos, const MagicEffectClass* me);
    bool creatureOnPrepareMagicAttack(Creature *creature, Position pos, const MagicEffectClass* me);

    /**
    	* Change the players hitpoints
    	* Return: the mana damage and the actual hitpoint loss
    	*/
    void creatureApplyDamage(Creature *creature, int32_t damage, int32_t &outDamage, int32_t &outManaDamage
#ifdef YUR_PVP_ARENA
                             , CreatureVector*
#endif //YUR_PVP_ARENA
                            );

    void CreateDamageUpdate(Creature* player, Creature* attackCreature, int32_t damage);
    void CreateManaDamageUpdate(Creature* player, Creature* attackCreature, int32_t damage);


    OTSYS_THREAD_LOCKVAR eventLock;
    OTSYS_THREAD_SIGNALVAR eventSignal;

    static OTSYS_THREAD_RETURN eventThread(void *p);

#ifdef __DEBUG_CRITICALSECTION__
    static OTSYS_THREAD_RETURN monitorThread(void *p);
#endif

    struct GameEvent
    {
        __int64  tick;
        int32_t      type;
        void*    data;
    };
    void checkPlayerWalk(uint32_t id);
    void checkCreature(uint32_t id);
    void checkCreatureAttacking(uint32_t id);
    void checkDecay(int32_t t);

#define DECAY_INTERVAL  10000
    void startDecay(Item* item);
    struct decayBlock
    {
        int32_t decayTime;
        std::list<Item*> decayItems;
    };
    std::list<decayBlock*> decayVector;

    void startOwner(Item* item, Player* player);
    struct ownerBlock
    {
        int32_t ownerTime;
        std::list<Item*> ownerItems;
    };
    std::list<ownerBlock*> ownerVector;
#ifdef CVS_DAY_CYCLE
    static const unsigned char LIGHT_LEVEL_DAY = 220;
    static const unsigned char LIGHT_LEVEL_NIGHT = 25;
    static const int32_t SUNSET = 1305;
    static const int32_t SUNRISE = 430;
    unsigned char lightlevel;
    eLightState light_state;
    int32_t light_hour;
    int32_t light_hour_delta;
    //int32_t lightdelta;
    void checkLight(int32_t delta);
#endif //CVS_DAY_CYCLE

    void checkSpawns(int32_t t);
    std::priority_queue<SchedulerTask*, std::vector<SchedulerTask*>, lessSchedTask > eventList;
    std::map<uint32_t, SchedulerTask*> eventIdMap;
    uint32_t eventIdCount;

    uint32_t max_players;
    enum_world_type worldType;



    std::vector<std::string> commandTags;
    void addCommandTag(std::string tag);
    void resetCommandTag();

    enum_game_state game_state;

    friend class Commands;
    friend class Monster;
    friend class GameState;
    friend class Spawn;
    friend class SpawnManager;
    friend class ActionScript;
    friend class Actions;
    friend class IOPlayerSQL;
};

template<class ArgType>
class TCallList : public SchedulerTask
{
public:
    TCallList(boost::function<int32_t(Game*, ArgType)> f1, boost::function<bool(Game*)> f2, std::list<ArgType>& call_list, __int64 interval) :
        _f1(f1), _f2(f2), _list(call_list), _interval(interval)
    {
    }

    void operator()(Game* arg)
    {
        if(_eventid != 0 && !_f2(arg))
        {
            int32_t ret = _f1(arg, _list.front());
            _list.pop_front();
            if (ret && !_list.empty())
            {
                SchedulerTask* newTask = new TCallList(_f1, _f2, _list, _interval);
                newTask->setTicks(_interval);
                newTask->setEventId(this->getEventId());
                arg->addEvent(newTask);
            }
        }

        return;
    }

private:
    boost::function<int32_t(Game*, ArgType)> _f1;
    boost::function<bool(Game*)>_f2;
    std::list<ArgType> _list;
    __int64 _interval;
};

template<class ArgType>
SchedulerTask* makeTask(__int64 ticks, boost::function<int32_t(Game*, ArgType)> f1, std::list<ArgType>& call_list, __int64 interval, boost::function<bool(Game*)> f2)
{
    TCallList<ArgType> *t = new TCallList<ArgType>(f1, f2, call_list, interval);
    t->setTicks(ticks);
    return t;
}
#endif
