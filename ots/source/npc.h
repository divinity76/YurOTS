//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software Foundation,
// Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//////////////////////////////////////////////////////////////////////


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
class NpcScript : protected LuaScript{
public:
	NpcScript(std::string name, Npc* npc);
	virtual ~NpcScript(){}
	//	virtual void onThingMove(const Player *player, const Thing *thing, const Position *oldPos,
	//	unsigned char oldstackpos, unsigned char oldcount, unsigned char count);
	virtual void onCreatureAppear(unsigned long cid);
	virtual void onCreatureDisappear(int cid);
	//	virtual void onCreatureTurn(const Creature *creature, unsigned char stackpos);
	virtual void onCreatureSay(int cid, SpeakClasses, const std::string &text);
	virtual void onThink();
	//	virtual void onCreatureChangeOutfit(const Creature* creature);
	static Npc* getNpc(lua_State *L);
	static int luaActionSay(lua_State *L);
	static int luaActionMove(lua_State *L);
	static int luaActionMoveTo(lua_State *L);
	static int luaCreatureGetName(lua_State *L);
	static int luaCreatureGetName2(lua_State *L);
	static int luaActionAttackCreature(lua_State *L);
	static int luaCreatureGetPos(lua_State *L);
	static int luaSelfGetPos(lua_State *L);

#ifdef TLM_BUY_SELL
	static int luaBuyItem(lua_State *L);
	static int luaSellItem(lua_State *L);
	static int luaPayMoney(lua_State *L);
#endif

#ifdef YUR_NPC_EXT
	static int luaGetPlayerStorageValue(lua_State *L);
	static int luaSetPlayerStorageValue(lua_State *L);
	static int luaPlayerRemoveItem(lua_State *L);
	static int luaGetPlayerLevel(lua_State *L);
	static int luaSetPlayerMasterPos(lua_State* L);
#endif //YUR_NPC_EXT

#ifdef YUR_GUILD_SYSTEM
	static int luaFoundNewGuild(lua_State* L);
	static int luaGetPlayerGuildStatus(lua_State* L);
	static int luaSetPlayerGuildStatus(lua_State* L);
	static int luaGetPlayerGuildName(lua_State* L);
	static int luaSetPlayerGuild(lua_State* L);
	static int luaClearPlayerGuild(lua_State* L);
	static int luaSetPlayerGuildNick(lua_State* L);
#endif //YUR_GUILD_SYSTEM

#ifdef YUR_PREMIUM_PROMOTION
	static int luaIsPremium(lua_State* L);
	static int luaIsPromoted(lua_State* L);
#endif //YUR_PREMIUM_PROMOTION

#ifdef YUR_ROOKGARD
	static int luaSetPlayerVocation(lua_State* L);
#endif //YUR_ROOKGARD

#ifdef YUR_LEARN_SPELLS
	static int luaGetPlayerVocation(lua_State *L);
	static int luaLearnSpell(lua_State* L);
#endif //YUR_LEARN_SPELLS

	bool isLoaded(){return loaded;}

protected:
	int registerFunctions();
	Npc* npc;
	bool loaded;
};

class Npc : public Creature
{
public:
	Npc(const std::string& name, Game* game);
	virtual ~Npc();
	virtual void useThing() {
		//std::cout << "Npc: useThing() " << this << std::endl;
		useCount++;
	};

	virtual void releaseThing() {
		//std::cout << "Npc: releaseThing() " << this << std::endl;
		useCount--;
		if (useCount == 0)
			delete this;
	};

	virtual unsigned long idRange(){ return 0x30000000;}
	static AutoList<Npc> listNpc;
	void removeList() {listNpc.removeList(getID());}
	void addList() {listNpc.addList(this);}

	void speak(const std::string &text){};
	const std::string& getName() const {return name;};
	fight_t getFightType(){return fighttype;};

	int64_t mana, manamax;

	//damage per hit
	int damage;

	fight_t fighttype;

	Game* game;

	void doSay(std::string msg);
	void doMove(int dir);
	void doMoveTo(Position pos);
	void doAttack(int id);
	bool isLoaded(){return loaded;}

protected:
	int useCount;
	virtual void onThingMove(const Player *player, const Thing *thing, const Position *oldPos,
		unsigned char oldstackpos, unsigned char oldcount, unsigned char count);
	virtual void onCreatureAppear(const Creature *creature);
	virtual void onCreatureDisappear(const Creature *creature, unsigned char stackPos, bool tele);
	virtual void onThingDisappear(const Thing* thing, unsigned char stackPos);
	virtual void onThingTransform(const Thing* thing,int stackpos){};
	virtual void onThingAppear(const Thing* thing);
	virtual void onCreatureTurn(const Creature *creature, unsigned char stackpos);
	virtual void onCreatureSay(const Creature *creature, SpeakClasses type, const std::string &text);
	virtual void onCreatureChangeOutfit(const Creature* creature);
	virtual int onThink(int& newThinkTicks);
	//virtual void setAttackedCreature(unsigned long id);
	virtual std::string getDescription(bool self = false) const;

	virtual bool isAttackable() const { return false; };

#ifdef YUR_CVS_MODS
	virtual bool isPushable() const { return false; };
#else
	virtual bool isPushable() const { return true; };
#endif //YUR_CVS_MODS

	std::string name;
	std::string scriptname;
	NpcScript* script;
	std::list<Position> route;
	bool loaded;
};

#endif // __npc_h_
