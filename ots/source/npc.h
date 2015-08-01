#ifndef __npc_h_
#define __npc_h_

#include "creature.h"
#include "game.h"
#include "luascript.h"
#include "templates.h"

extern "C"
{
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}


//////////////////////////////////////////////////////////////////////
// Defines an NPC...
class Npc;
class NpcScript : protected LuaScript
{
public:
    NpcScript(std::string name, Npc* npc);
    virtual ~NpcScript() {}
    //	virtual void onThingMove(const Player *player, const Thing *thing, const Position *oldPos,
    //	unsigned char oldstackpos, unsigned char oldcount, unsigned char count);
    virtual void onCreatureAppear(uint32_t cid);
    virtual void onCreatureDisappear(int32_t cid);
    //	virtual void onCreatureTurn(const Creature *creature, unsigned char stackpos);
    virtual void onCreatureSay(int32_t cid, SpeakClasses, const std::string &text);
    virtual void onThink();
    //	virtual void onCreatureChangeOutfit(const Creature* creature);
    static Npc* getNpc(lua_State *L);
    static int32_t luaActionSay(lua_State *L);
    static int32_t luaActionMove(lua_State *L);
    static int32_t luaActionMoveTo(lua_State *L);
    static int32_t luaCreatureGetName(lua_State *L);
    static int32_t luaCreatureGetName2(lua_State *L);
    static int32_t luaActionAttackCreature(lua_State *L);
    static int32_t luaCreatureGetPos(lua_State *L);
    static int32_t luaSelfGetPos(lua_State *L);
    static int32_t luaBuyItem(lua_State *L);
    static int32_t luaBuyActionId(lua_State *L);
    static int32_t luaSellItem(lua_State *L);
    static int32_t luaPayMoney(lua_State *L);
    static int32_t luaTradeItem(lua_State *L);
    static int32_t luaBuyCont(lua_State *L);
    static int32_t luaGetPlayerStorageValue(lua_State *L);
    static int32_t luaSetPlayerStorageValue(lua_State *L);
    static int32_t luaPlayerRemoveItem(lua_State *L);
    static int32_t luaGetPlayerLevel(lua_State *L);
    static int32_t luaSetPlayerMasterPos(lua_State* L);
#ifdef YUR_PREMIUM_PROMOTION
    static int32_t luaIsPremium(lua_State* L);
    static int32_t luaIsPromoted(lua_State* L);
#endif //YUR_PREMIUM_PROMOTION

    static int32_t luaSetPlayerVocation(lua_State* L);
    static int32_t luaGetPlayerVocation(lua_State *L);
    static int32_t luaActionLook(lua_State *L);
    static int32_t luaItemToStorage(lua_State* L);
#ifdef HUCZU_PAY_SYSTEM
    static int32_t luaAddPlayerPoints(lua_State* L);
    static int32_t luaGetPlayerPoints(lua_State* L);
    static int32_t luaRemovePlayerPoints(lua_State* L);
#endif
    static int32_t luaSetPlayerOutfit(lua_State* L);
    bool isLoaded()
    {
        return loaded;
    }

protected:
    int32_t registerFunctions();
    Npc* npc;
    bool loaded;
};

class Npc : public Creature
{
public:
    Npc(const std::string& name, Game* game);
    virtual ~Npc();
    virtual void useThing()
    {
        //std::cout << "Npc: useThing() " << this << std::endl;
        useCount++;
    };

    virtual void releaseThing()
    {
        //std::cout << "Npc: releaseThing() " << this << std::endl;
        useCount--;
        if (useCount == 0)
            delete this;
    };

    virtual uint32_t idRange()
    {
        return 0x30000000;
    }
    static AutoList<Npc> listNpc;
    void removeList()
    {
        listNpc.removeList(getID());
    }
    void addList()
    {
        listNpc.addList(this);
    }

    void speak(const std::string &text) {};
    const std::string& getName() const
    {
        return name;
    };
    fight_t getFightType()
    {
        return fighttype;
    };

    int64_t mana, manamax;
    int32_t eventTalk;

    //damage per hit
    int64_t damage;

    fight_t fighttype;

    Game* game;

    void doSay(std::string msg);
    void doMove(int32_t dir);
    void doMoveTo(Position pos);
    void doAttack(int32_t id);
    void doLook(const Creature *creature);
    bool isLoaded()
    {
        return loaded;
    }

protected:
    int32_t useCount;
    virtual void onThingMove(const Player *player, const Thing *thing, const Position *oldPos,
                             unsigned char oldstackpos, unsigned char oldcount, unsigned char count);
    virtual void onCreatureAppear(const Creature *creature);
    virtual void onCreatureDisappear(const Creature *creature, unsigned char stackPos, bool tele);
    virtual void onThingDisappear(const Thing* thing, unsigned char stackPos);
    virtual void onThingTransform(const Thing* thing,int32_t stackpos) {};
    virtual void onThingAppear(const Thing* thing);
    virtual void onCreatureTurn(const Creature *creature, unsigned char stackpos);
    virtual void onCreatureSay(const Creature *creature, SpeakClasses type, const std::string &text);
    virtual void onCreatureChangeOutfit(const Creature* creature);
    virtual int32_t onThink(int32_t& newThinkTicks);
    //virtual void setAttackedCreature(uint32_t id);
    virtual std::string getDescription(bool self = false) const;

    virtual bool isAttackable() const
    {
        return false;
    };

#ifdef YUR_CVS_MODS
    virtual bool isPushable() const
    {
        return false;
    };
#else
    virtual bool isPushable() const
    {
        return true;
    };
#endif //YUR_CVS_MODS

    std::string name;
    std::string scriptname;
    NpcScript* script;
    std::list<Position> route;
    bool loaded;
};
#endif // __npc_h_
