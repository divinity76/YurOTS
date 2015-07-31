#ifndef __spells_h_
#define __spells_h_

#include "game.h"
#include "luascript.h"
#include "player.h"

extern "C"
{
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}

class Spell;
class SpellScript;

class Spells
{
public:
    Spells(Game* game);
    bool loadFromXml(const std::string&);
    virtual ~Spells();

    Game* game;
    bool isLoaded()
    {
        return loaded;
    }
    std::map<std::string, Spell*>* getVocSpells(playervoc_t voc)
    {
        if((int32_t)voc > maxVoc || voc < 0)
        {
            return 0;
        }

        return &(vocationSpells.at(voc));
    }

    std::map<std::string, Spell*>* getAllSpells()
    {
        return &allSpells;
    }

    std::map<uint16_t, Spell*>* getVocRuneSpells(int32_t voc)
    {
        if(voc>maxVoc || voc<0)
        {
            return 0;
        }

        return &(vocationRuneSpells.at(voc));
    }

    std::map<uint16_t, Spell*>* getAllRuneSpells()
    {
        return &allRuneSpells;
    }
protected:
    std::map<std::string, Spell*> allSpells;
    std::vector<std::map<std::string, Spell*> > vocationSpells;

    std::map<uint16_t, Spell*> allRuneSpells;
    std::vector<std::map<uint16_t, Spell*> > vocationRuneSpells;
    bool loaded;
    int32_t maxVoc;
};


class Spell
{
public:
    Spell(std::string name, int32_t magLv, int32_t mana, Game* game);
    virtual ~Spell();

    Game* game;
    bool isLoaded()
    {
        return loaded;
    }
    SpellScript* getSpellScript()
    {
        return script;
    };
    std::string getName() const
    {
        return name;
    };
    int32_t getMana()
    {
        return mana;
    };
    int32_t getMagLv()
    {
        return magLv;
    };

protected:
    std::string name;
    int32_t magLv, mana;
    bool loaded;
    SpellScript* script;
};

class InstantSpell : public Spell
{
public:
    InstantSpell(const std::string &, std::string name, std::string words, int32_t magLv, int32_t mana, Game* game);
    std::string getWords()
    {
        return words;
    };

protected:
    std::string words;
};

class RuneSpell : public Spell
{
public:
    RuneSpell(const std::string& ,std::string name, uint16_t id, uint16_t charges, int32_t magLv, int32_t mana, Game* game);

protected:
    uint16_t id;
    uint16_t charges;
};

class SpellScript : protected LuaScript
{
public:
    SpellScript(const std::string&, std::string scriptname, Spell* spell);
    virtual ~SpellScript() {}
    bool castSpell(Creature* creature, const Position& pos, std::string var);
    bool isLoaded()
    {
        return loaded;
    }
    static Spell* getSpell(lua_State *L);

    static int32_t luaActionDoTargetSpell(lua_State *L);
    static int32_t luaActionDoTargetExSpell(lua_State *L);
    static int32_t luaActionDoTargetGroundSpell(lua_State *L);
    static int32_t luaActionDoAreaSpell(lua_State *L);
    static int32_t luaActionDoAreaExSpell(lua_State *L);
    static int32_t luaActionDoAreaGroundSpell(lua_State *L);
    static int32_t luaActionRemoveCondition(lua_State *L);

    //static int32_t luaActionDoSpell(lua_State *L);
    static int32_t luaActionGetPos(lua_State *L);
    static int32_t luaActionChangeOutfit(lua_State *L);
    static int32_t luaActionManaShield(lua_State *L);
    static int32_t luaActionChangeSpeed(lua_State *L);
    static int32_t luaActionParalyze(lua_State *L);
    static int32_t luaChallenge(lua_State *L);
    static int32_t luaActionChangeSpeedMonster(lua_State *L);
    static int32_t luaActionGetSpeed(lua_State *L);
    static int32_t luaActionMakeRune(lua_State *L);
    static int32_t luaActionMakeArrows(lua_State *L);
    static int32_t luaActionMakeFood(lua_State *L);
    static int32_t luaSetPlayerLightLevel(lua_State *L);
    static int32_t luaActionInvisible(lua_State *L);
    static int32_t luaActionDrunk(lua_State *L);

protected:
    static void internalGetArea(lua_State *L, MagicEffectAreaClass &magicArea);
    static void internalGetPosition(lua_State *L, Position& pos);
    static void internalGetMagicEffect(lua_State *L, MagicEffectClass &me);
    static void internalLoadDamageVec(lua_State *L, ConditionVec& condvec);
    static void internalLoadTransformVec(lua_State *L, TransformMap& transformMap);
    static int32_t  internalMakeRune(Player *p,uint16_t sl_id,Spell *S,uint16_t id, unsigned char charges);
    int32_t registerFunctions();
    Spell* spell;
    bool loaded;
};
#endif // __spells_h_
