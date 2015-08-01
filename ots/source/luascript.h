#ifndef __LUASCRIPT_H__
#define __LUASCRIPT_H__

#include <string>
extern "C"
{
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}

extern "C" struct lua_State;

#ifdef YUR_HIGH_LEVELS
typedef int64_t exp_t;
#else
typedef uint32_t exp_t;
#endif //YUR_HIGH_LEVELS

class LuaScript
{
public:
    LuaScript();
    ~LuaScript();

#ifdef __MIZIAK_SUPERMANAS__
    int64_t MANAS_EXHAUSTED;
    int64_t MANAS_MIN_MANA;
    int64_t MANAS_MAX_MANA;
#endif //__MIZIAK_SUPERMANAS__
    int32_t MAX_PLAYERS;
    bool GM_MSG;


    exp_t EXP_ROOK;
    exp_t EXP_MUL;
    exp_t EXP_MUL_PVP;

    int64_t HEALTH_TICK_MUL;
    int64_t MANA_TICK_MUL;
    int64_t HEALTH_TICK_MUL_ROOK;
    int64_t MANA_TICK_MUL_ROOK;
    int32_t CAP_GAIN[5];
    int64_t MANA_GAIN[5];
    int64_t HP_GAIN[5];
    int32_t WEAPON_MUL[5];
    int32_t SHIELD_MUL[5];
    int32_t DIST_MUL[5];
    int64_t MANA_MUL[5];
    int64_t SOFTMANA;
    int64_t SOFTHEALTH;
    int32_t KICK_TIME;
    int32_t DROP_RATE;
#ifdef TLM_HOUSE_SYSTEM
    int32_t ACCESS_HOUSE;
    int32_t MAX_HOUSE_TILE_ITEMS;
#endif //TLM_HOUSE_SYSTEM

#ifdef HUCZU_SKULLS
    int32_t HIT_TIME;
    int32_t WHITE_TIME;
    int32_t RED_TIME;
    int32_t FRAG_TIME;
    int32_t RED_UNJUST;
    int32_t BAN_UNJUST;
#endif //HUCZU_SKULLS
    int32_t SQL_PORT;
    int32_t MYSQL_READ_TIMEOUT;
    int32_t MYSQL_WRITE_TIMEOUT;
    int32_t SQL_KEEPALIVE;
    int32_t GUILD_FORM_LEVEL;

    double BURST_DMG_LVL;
    double BURST_DMG_MLVL;
    double BURST_DMG_LO;
    double BURST_DMG_HI;

    double GOLD_DMG_LVL;
    double GOLD_DMG_MLVL;
    double GOLD_DMG_LO;
    double GOLD_DMG_HI;

    double SILVER_DMG_LVL;
    double SILVER_DMG_MLVL;
    double SILVER_DMG_LO;
    double SILVER_DMG_HI;

    double NO_VOCATION_SPEED;
    double SORCERER_SPEED;
    double DRUID_SPEED;
    double PALADIN_SPEED;
    double KNIGHT_SPEED;

    bool CAP_SYSTEM;
    bool ENDING_AMMO;
    int32_t SPEAR_LOSE_CHANCE;

#ifdef YUR_PREMIUM_PROMOTION
    bool FREE_PREMMY;
    bool QUEUE_PREMMY;
#endif //YUR_PREMIUM_PROMOTION

#ifdef YUR_CVS_MODS
    std::string VOCATIONS[5];
    std::string PROMOTED_VOCATIONS[6];
    int32_t DIE_PERCENT_EXP;
    int64_t DIE_PERCENT_MANA;
    int32_t DIE_PERCENT_SKILL;
    int32_t DIE_PERCENT_EQ;
    int32_t DIE_PERCENT_BP;
    int32_t PZ_LOCKED;
    int32_t EXHAUSTED;
    int32_t EXHAUSTED_ADD;
    int32_t EXHAUSTED_HEAL;
    int32_t ACCESS_PROTECT;
    int32_t ACCESS_REMOTE;
    int32_t ACCESS_TALK;
    int32_t ACCESS_ENTER;
    int32_t ACCESS_LOOK;
    std::string DATA_DIR;
#endif //YUR_CVS_MODS

#ifdef TR_SUMMONS
    bool SUMMONS_ALL_VOC;
    size_t MAX_SUMMONS;
#endif //TR_SUMMONS

    size_t MAX_DEATH_ENTRIES;

    int64_t MANA_SNAKEBITE;
    int64_t MANA_MOONLIGHT;
    int64_t MANA_VOLCANIC;
    int64_t MANA_QUAGMIRE;
    int64_t MANA_TEMPEST;

    int64_t MANA_VORTEX;
    int64_t MANA_DRAGONBREATH;
    int64_t MANA_PLAGUE;
    int64_t MANA_COSMIC;
    int64_t MANA_INFERNO;

    int32_t SANDALY;
    int32_t BLUE_ROBE;
    int32_t MYSTIC_TURBAN;
    int32_t TRIBAL_SET_INCR;
    int32_t TRIBAL_SET_DECR;
    int32_t SHIELD;
    int32_t ASTEL_SHIELD;
    int32_t FEATHER_LEGS;
    int32_t DIAMOND_RING;
    int64_t DIAMOND_RING_MANA;
    int64_t DIAMOND_RING_HP;
    int32_t PANDEMKA;
#ifdef HUCZU_AMULET
    int32_t TYMERIA_AMULET_INCR;
    int32_t TYMERIA_AMULET_DESC;
#endif
    int64_t MANAS_MIN_LVL;
    int64_t MANAS_MIN_MLVL;
    int64_t MANAS_MIN_LO;

    int64_t MANAS_MAX_LVL;
    int64_t MANAS_MAX_MLVL;
    int64_t MANAS_MAX_HI;

    int64_t MAGIC_BACKPACK_HP;
    int64_t MAGIC_BACKPACK_MP;
    int32_t MAGIC_BACKPACK;

    int32_t RANGE_SNAKEBITE;
    int32_t RANGE_MOONLIGHT;
    int32_t RANGE_VOLCANIC;
    int32_t RANGE_QUAGMIRE;
    int32_t RANGE_TEMPEST;

    int32_t RANGE_VORTEX;
    int32_t RANGE_DRAGONBREATH;
    int32_t RANGE_PLAGUE;
    int32_t RANGE_COSMIC;
    int32_t RANGE_INFERNO;
    int64_t LIFE_RING_ZYCIE;
    int64_t LIFE_RING_MANA;
    int64_t ROH_ZYCIE;
    int64_t ROH_MANA;
    int32_t MAX_HOUSES;
    int32_t HOUSE_LVL_ROOK;
    int32_t HOUSE_LVL;
    int32_t PRICE_FOR_SQM;
    int32_t AUTO_SAVE;
    int32_t AUTO_CLEAN;
    int32_t AUTO_RESTART;
    int32_t PK_BANDAYS;
    int32_t SUSPEND_TIME_MAX;
    int32_t SUSPEND_IP_TRIES;
    int32_t PORT;
    int32_t MAX_DEPOTITEMS_PREMMY;
    int32_t MAX_DEPOTITEMS_FREE;
    int32_t ODLEGLOSC_OD_DEPO;
    int32_t FIRST_ATTACK;
    int32_t OWNER_TIME;

    std::string SUSPEND_MSG;
    std::string MOTD;
    std::string WORLD_NAME;
    std::string PRIORYTET;
    std::string WORLD_TYPE;
    std::string MAP_PATH;
    std::string IP;
    std::string SERVER_NAME;
    std::string LOGIN_MSG;
    std::string SQL_DB;
    std::string SQL_HOST;
    std::string SQL_USER;
    std::string SQL_PASS;
    std::string LOAD_NPC;

    bool BANMSG;
    bool OPTIMIZE_DB_AT_STARTUP;
    bool STAGE_EXP;
#ifdef HUCZU_PAY_SYSTEM
    bool PAY_SYSTEM;
    int32_t HOMEPAY_USR_ID;
#endif
    double DIE_PERCENT_SL;

    int32_t OpenFile(const char* file);
    int32_t getField (const char *key);
    void setField (const char *index, int32_t value);
    //static version
    static int32_t getField (lua_State *L , const char *key);
    static void setField (lua_State *L, const char *index, int32_t val);
    // get a global string
    bool getGlobalBool(std::string var, bool _default = false);
    double getGlobalDouble(std::string var, const double defNum = 0);
    std::string getGlobalString(std::string var, const std::string &defString = "");
    int32_t getGlobalNumber(std::string var, const int32_t defNum = 0);
    std::string getGlobalStringField (std::string var, const int32_t key, const std::string &defString = "");
    // set a var to a val
    int32_t setGlobalString(std::string var, std::string val);
    int32_t setGlobalNumber(std::string var, int32_t val);

protected:
    std::string luaFile;   // the file we represent
    lua_State*  luaState;  // our lua state
};
#endif  // #ifndef __LUASCRIPT_H__
