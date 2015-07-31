
#ifndef __actions_h_
#define __actions_h_

////#include "preheaders.h"
#include "position.h"

#include "luascript.h"

#include <map>

extern "C"
{
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}

class Player;
class Npc;
class Monster;
class Item;
class Game;
class ActionScript;
class Action;

enum tCanUseRet
{
    CAN_USE,
    TOO_FAR,
    CAN_NOT_THTOW,
};

class Actions
{
public:
    Actions() {};
    Actions(Game* igame);
    bool loadFromXml(const std::string &datadir);
    virtual ~Actions();
    void clear();

    bool UseItem(Player* player, const Position &pos,const unsigned char stack,
                 const uint16_t itemid, const unsigned char index);
    bool UseItemEx(Player* player, const Position &from_pos,
                   const unsigned char from_stack,const Position &to_pos,
                   const unsigned char to_stack,const uint16_t itemid);
#ifdef __MIZIAK_TALKACTIONS__
    bool SayTalk(Player*, std::string);
#endif //__MIZIAK_TALKACTIONS__
#ifdef __MIZIAK_CREATURESCRIPTS__
    bool creatureEvent(std::string, Player*, Creature*, Item*, int32_t[]);
#endif //__MIZIAK_CREATURESCRIPTS__
    bool luaWalk(Player* player, const Position &pos, //CHANGE onWalk
                 const uint16_t itemid, const uint32_t itemuid, const uint32_t itemaid);
    bool luaWalkOff(Player* player, const Position &pos, //CHANGE onWalk
                    const uint16_t itemid, const uint32_t itemuid, const uint32_t itemaid);

    bool openContainer(Player *player,Container *container, const unsigned char index);

    Game* game;
    bool loaded;

    bool isLoaded()
    {
        return loaded;
    }
    bool reload();

protected:
    std::string datadir;
    typedef std::map<uint16_t, Action*> ActionUseMap;
    ActionUseMap useItemMap;
    ActionUseMap uniqueItemMap;
    ActionUseMap actionItemMap;
#ifdef __MIZIAK_TALKACTIONS__
    typedef std::map<std::string, Action*> ActionTalkMap;
    ActionTalkMap actionTalkMap;
    Action *getActionTalk(std::string);
#endif //__MIZIAK_TALKACTIONS__
#ifdef __MIZIAK_CREATURESCRIPTS__
    typedef std::map<std::string, Action*> ActionCreatureMap;
    ActionCreatureMap creatureScriptMap;
#endif //__MIZIAK_CREATURESCRIPTS__
    int32_t canUse(const Player *player,const Position &pos) const;
    int32_t canUseFar(const Player *player,const Position &to_pos, const bool blockWalls) const;
    Action *getAction(const Item *item);
    Action *loadAction(xmlNodePtr xmlaction);
};

enum tThingType
{
    thingTypeItem,
    thingTypePlayer,
    thingTypeMonster,
    thingTypeNpc,
    thingTypeUnknown,
};

enum ePlayerInfo
{
    PlayerInfoFood,
    PlayerInfoAccess,
    PlayerInfoLevel,
    PlayerInfoMagLevel,
    PlayerInfoMana,
    PlayerInfoHealth,
    PlayerInfoName,
    PlayerInfoPosition,
    PlayerInfoVocation,
    PlayerInfoMasterPos,
    PlayerInfoGuildId,
};

struct KnownThing
{
    Thing *thing;
    tThingType type;
    PositionEx pos;
};

class Action
{
public:
    Action(Game* igame,const std::string &datadir, const std::string &scriptname);
    virtual ~Action();
    bool isLoaded() const
    {
        return loaded;
    }
    bool allowFarUse() const
    {
        return allowfaruse;
    };
    bool blockWalls() const
    {
        return blockwalls;
    };
    void setAllowFarUse(bool v)
    {
        allowfaruse = v;
    };
    void setBlockWalls(bool v)
    {
        blockwalls = v;
    };
#ifdef __MIZIAK_TALKACTIONS__
    void setAccess(int32_t v)
    {
        access = v;
    };
    int32_t getAccess()
    {
        return access;
    };
    bool executeTalk(Player *, std::string, std::string);
#endif //__MIZIAK_TALKACTIONS__
#ifdef __MIZIAK_CREATURESCRIPTS__
    bool executeLogin(Player *);
    bool executeLogout(Player *);
    bool executeDeath(Player *, Creature *, Item *);
    bool executeAdvance(Player *, int32_t, int32_t, int32_t);
    bool executeKill(Player *, Creature *, Item *);
#endif //__MIZIAK_CREATURESCRIPTS__
    bool executeUse(Player *player,Item* item, PositionEx &posFrom, PositionEx &posTo);
    bool executeWalk(Player *player, const uint16_t item, const uint32_t itemUID, const uint32_t itemaid, const Position &posTo); //CHANGE onWalk
    bool executeWalkOff(Player *player, const uint16_t item, const uint32_t itemUID, const uint32_t itemaid, const Position &posTo); //CHANGE onWalk

protected:
    ActionScript *script;
    bool loaded;
    bool allowfaruse;
    bool blockwalls;
#ifdef __MIZIAK_TALKACTIONS__
    int32_t access;
#endif //__MIZIAK_TALKACTIONS__
};

class ActionScript : protected LuaScript
{
public:
    ActionScript(Game* igame,const std::string &datadir, const std::string &scriptname);
    virtual ~ActionScript();
    bool isLoaded()const
    {
        return loaded;
    }

    lua_State* getLuaState()
    {
        return luaState;
    }

    void ClearMap();
    static void AddThingToMapUnique(Thing *thing);
    void UpdateThingPos(int32_t uid, PositionEx &pos);
    uint32_t AddThingToMap(Thing *thing,PositionEx &pos);
    const KnownThing* GetThingByUID(int32_t uid);
    const KnownThing* GetItemByUID(int32_t uid);
    const KnownThing* GetCreatureByUID(int32_t uid);
    const KnownThing* GetPlayerByUID(int32_t uid);
    const KnownThing* GetMonsterByUID(int32_t uid);

    //lua functions
    static int32_t luaActionDoRemoveItem(lua_State *L);
    static int32_t luaActionDoFeedPlayer(lua_State *L);
    static int32_t luaActionDoSendCancel(lua_State *L);
    static int32_t luaActionDoTeleportThing(lua_State *L);
    static int32_t luaActionDoTransformItem(lua_State *L);
    static int32_t luaActionDoPlayerSay(lua_State *L);
    static int32_t luaActionDoSendMagicEffect(lua_State *L);
    static int32_t luaActionDoChangeTypeItem(lua_State *L);
    static int32_t luaActionDoSendAnimatedText(lua_State *L);
    static int32_t luaActionDoPlayerAddSkillTry(lua_State *L);
    static int32_t luaActionDoPlayerAddHealth(lua_State *L);
    static int32_t luaActionDoPlayerAddMana(lua_State *L);
    static int32_t luaActionDoPlayerAddItem(lua_State *L);
    static int32_t luaActionDoPlayerSendTextMessage(lua_State *L);
    static int32_t luaActionDoShowTextWindow(lua_State *L);
    static int32_t luaActionDoDecayItem(lua_State *L);
    static int32_t luaActionDoCreateItem(lua_State *L);
    static int32_t luaActionDoSummonCreature(lua_State *L);
    static int32_t luaActionDoPlayerRemoveMoney(lua_State *L);
    static int32_t luaActionDoPlayerSetMasterPos(lua_State *L);
    static int32_t luaActionDoPlayerSetVocation(lua_State *L);
    static int32_t luaActionDoPlayerRemoveItem(lua_State *L);
    static int32_t luaActionCreateCondition(lua_State *L);
    static int32_t luaActionDoCreateCondition(lua_State *L);
    static int32_t luaActionDoPlayerSetDrunk(lua_State *L);
    //get item info
    static int32_t luaActionGetItemRWInfo(lua_State *L);
    static int32_t luaActionGetThingfromPos(lua_State *L);
    //set item
    static int32_t luaActionDoSetItemActionId(lua_State *L);
    static int32_t luaActionDoSetItemText(lua_State *L);
    static int32_t luaActionDoSetItemSpecialDescription(lua_State *L);

    //get tile info
    static int32_t luaActionGetTilePzInfo(lua_State *L);

    //get player info functions
    static int32_t luaActionGetPlayerFood(lua_State *L);
    static int32_t luaActionGetPlayerAccess(lua_State *L);
    static int32_t luaActionGetPlayerLevel(lua_State *L);
    static int32_t luaActionGetPlayerMagLevel(lua_State *L);
    static int32_t luaActionGetPlayerMana(lua_State *L);
    static int32_t luaActionGetPlayerHealth(lua_State *L);
    static int32_t luaActionGetPlayerName(lua_State *L);
    static int32_t luaActionGetPlayerPosition(lua_State *L);
    static int32_t luaActionGetPlayerSkill(lua_State *L);
    static int32_t luaActionGetPlayerVocation(lua_State *L);
    static int32_t luaActionGetPlayerMasterPos(lua_State *L);
    static int32_t luaActionGetPlayerGuildId(lua_State *L);
    static int32_t luaActionGetPlayerStorageValue(lua_State *L);
    static int32_t luaActionSetPlayerStorageValue(lua_State *L);
    static int32_t luaActionGetItemName(lua_State *L);
    static int32_t luaActionDoAddTeleport(lua_State *L);
    static int32_t luaActionDoRemoveTeleport(lua_State *L);
    static int32_t luaActionGetPlayerSlotItem(lua_State *L);
    static int32_t luaActionGetTownIDByName(lua_State *L);
    static int32_t luaActionGetTownNameByID(lua_State *L);
    static int32_t luaActionIsMonster(lua_State *L);
    static int32_t luaActionIsPlayer(lua_State *L);
    static int32_t luaActionDoSetItemUniqueID(lua_State *L);
    static int32_t luaActionGetCreatureName(lua_State *L);
protected:

    Game *game;
    Player *_player;
    uint32_t lastuid;

    friend class Action;

    std::map<uint32_t,KnownThing*> ThingMap;
    static std::map<uint32_t,KnownThing*> uniqueIdMap;

    //lua related functions
    int32_t registerFunctions();
    bool loaded;
    //lua interface helpers
    static ActionScript* getActionScript(lua_State *L);
    static void internalAddPositionEx(lua_State *L, const PositionEx& pos);
    static void internalGetPositionEx(lua_State *L, PositionEx& pos);
    static uint32_t internalGetNumber(lua_State *L);
    static const char* internalGetString(lua_State *L);
    static void internalAddThing(lua_State *L, const Thing *thing, const uint32_t thingid);

    static Position internalGetRealPosition(ActionScript *action, Player *player, const Position &pos);
    static int32_t internalGetPlayerInfo(lua_State *L, ePlayerInfo info);

};

#endif // __actions_h_
