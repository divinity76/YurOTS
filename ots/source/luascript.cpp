#include <string>
#include <iostream>

#include "luascript.h"
#include "player.h"

LuaScript::LuaScript()
{
    luaState = NULL;
}


LuaScript::~LuaScript()
{
    if (luaState)
        lua_close(luaState);
}


int32_t LuaScript::OpenFile(const char *filename)
{
    luaState = lua_open();

    if (lua_dofile(luaState, filename))
        return false;
    //liczby
    PORT = getGlobalNumber("port", 7171);
    MAX_PLAYERS = getGlobalNumber("maxplayers", 100);
    MAX_SUMMONS = (size_t)getGlobalNumber("maxsummons",2);
    MAX_DEATH_ENTRIES = getGlobalNumber("maxdeathentries",10);

    DROP_RATE = getGlobalNumber("droprate",1);

    EXP_MUL = getGlobalNumber("expmul",1);
    EXP_MUL_PVP = getGlobalNumber("expmulpvp",1);
    EXP_ROOK = getGlobalNumber("exprook",1);

    HEALTH_TICK_MUL = getGlobalNumber("healthtickmul",1);
    MANA_TICK_MUL = getGlobalNumber("manatickmul",1);
    HEALTH_TICK_MUL_ROOK = getGlobalNumber("healthtickmulrook",1);
    MANA_TICK_MUL_ROOK = getGlobalNumber("manatickmulrook",1);
    SOFTMANA = getGlobalNumber("softMana",1);
    SOFTHEALTH = getGlobalNumber("softHealth",1);
    LIFE_RING_ZYCIE = getGlobalNumber("liferingzycie", 1);
    LIFE_RING_MANA = getGlobalNumber("liferingmana", 1);
    ROH_ZYCIE = getGlobalNumber("ringofhealingzycie", 1);
    ROH_MANA = getGlobalNumber("ringofhealingmana", 1);
#ifdef __MIZIAK_SUPERMANAS__
    MANAS_EXHAUSTED = getGlobalNumber("manas_exh",1);
#endif //__MIZIAK_SUPERMANAS__
    SANDALY = getGlobalNumber("sandaly",7);
    BLUE_ROBE = getGlobalNumber("bluerobe",20);
    MYSTIC_TURBAN = getGlobalNumber("mysticturban",20);
    TRIBAL_SET_INCR = getGlobalNumber("tribal_set_incr", 5);
    TRIBAL_SET_DECR = getGlobalNumber("tribal_set_desc", 5);
    SHIELD = getGlobalNumber("blood_shield", 10);
    ASTEL_SHIELD = getGlobalNumber("astel_shield", 10);
    FEATHER_LEGS = getGlobalNumber("feather_legs", 5);
    DIAMOND_RING = getGlobalNumber("diamond_ring", 5);
    DIAMOND_RING_MANA = getGlobalNumber("diamond_ringHp", 5);
    DIAMOND_RING_HP = getGlobalNumber("diamond_ringMp", 5);
    PANDEMKA = getGlobalNumber("pandemka", 10);
#ifdef HUCZU_AMULET
    TYMERIA_AMULET_INCR = getGlobalNumber("amulet_incr", 10);
    TYMERIA_AMULET_DESC = getGlobalNumber("amulet_desc", 5);
#endif
    MAGIC_BACKPACK = getGlobalNumber("MagicBpIncr", 10);
    MAGIC_BACKPACK_HP = getGlobalNumber("MagicBpHp", 1);
    MAGIC_BACKPACK_MP = getGlobalNumber("MagicBpMp", 1);

    GOLD_DMG_LVL = atof(getGlobalStringField("goldboltdmg", 1, "2.0").c_str());
    GOLD_DMG_MLVL = atof(getGlobalStringField("goldboltdmg", 2, "3.0").c_str());
    GOLD_DMG_LO = atof(getGlobalStringField("goldboltdmg", 3, "0.24").c_str());
    GOLD_DMG_HI = atof(getGlobalStringField("goldboltdmg", 4, "0.55").c_str());

    SILVER_DMG_LVL = atof(getGlobalStringField("silverwanddmg", 1, "2.0").c_str());
    SILVER_DMG_MLVL = atof(getGlobalStringField("silverwanddmg", 2, "3.0").c_str());
    SILVER_DMG_LO = atof(getGlobalStringField("silverwanddmg", 3, "0.24").c_str());
    SILVER_DMG_HI = atof(getGlobalStringField("silverwanddmg", 4, "0.55").c_str());

    MANAS_MIN_LVL = atof(getGlobalStringField("manasMinRate", 1, "1.0").c_str());
    MANAS_MIN_MLVL = atof(getGlobalStringField("manasMinRate", 2, "1.0").c_str());
    MANAS_MIN_LO = atof(getGlobalStringField("manasMinRate", 3, "0.24").c_str());

    MANAS_MAX_LVL = atof(getGlobalStringField("manasMaxRate", 1, "1.0").c_str());
    MANAS_MAX_MLVL = atof(getGlobalStringField("manasMaxRate", 2, "1.0").c_str());
    MANAS_MAX_HI = atof(getGlobalStringField("manasMaxRate", 3, "1.55").c_str());

    ACCESS_PROTECT = getGlobalNumber("accessprotect",3);
    ACCESS_REMOTE = getGlobalNumber("accessremote",3);
    ACCESS_ENTER = getGlobalNumber("accessenter",3);
    ACCESS_LOOK = getGlobalNumber("accesslook",3);

#ifdef TLM_HOUSE_SYSTEM
    ACCESS_HOUSE = getGlobalNumber("accesshouse",3);
    MAX_HOUSE_TILE_ITEMS = getGlobalNumber("maxhousetileitems",10);
#endif //TLM_HOUSE_SYSTEM
    MAX_HOUSES = getGlobalNumber("maxhouses", 0);
    HOUSE_LVL_ROOK = getGlobalNumber("buyhouselvlrook",1);
    HOUSE_LVL = getGlobalNumber("buyhouselvl",2);
    PRICE_FOR_SQM = getGlobalNumber("priceforsqm", 100*100*100);
    EXHAUSTED = getGlobalNumber("exhausted",0);
    EXHAUSTED_ADD = getGlobalNumber("exhaustedadd",0);
    EXHAUSTED_HEAL = getGlobalNumber("exhaustedheal",0);

#ifdef HUCZU_SKULLS
    HIT_TIME = getGlobalNumber("hittime",1)*60000;
    WHITE_TIME = getGlobalNumber("whitetime",15)*60000;
    RED_TIME = getGlobalNumber("redtime",5*60)*60000;
    FRAG_TIME = getGlobalNumber("fragtime",3*60)*60000;
    RED_UNJUST = getGlobalNumber("redunjust",3);
    BAN_UNJUST = getGlobalNumber("banunjust",6);
#endif //HUCZU_SKULLS
    PZ_LOCKED = getGlobalNumber("pzlocked",0);
    KICK_TIME = getGlobalNumber("kicktime",15)*60000;

    AUTO_SAVE = getGlobalNumber("autosave", 1)*60000;
    AUTO_CLEAN = getGlobalNumber("autoclean", 2)*60000;
    AUTO_RESTART = getGlobalNumber("autorestart", 2)*60000;
    OWNER_TIME = getGlobalNumber("ownerTime", 10);

    SUSPEND_TIME_MAX = getGlobalNumber("suspend_time_max", 15);
    SUSPEND_IP_TRIES = getGlobalNumber("suspend_ip_tries", 4);

    MAX_DEPOTITEMS_PREMMY = getGlobalNumber("maxdepotpremmy",2000);
    MAX_DEPOTITEMS_FREE = getGlobalNumber("maxdepotfree",1000);

    ODLEGLOSC_OD_DEPO = getGlobalNumber("odlegloscoddepo", 1);
    PK_BANDAYS = getGlobalNumber("pkbandays",3)*3600;
    SPEAR_LOSE_CHANCE = getGlobalNumber("spearlosechance",50);
    FIRST_ATTACK = getGlobalNumber("first_attack", 1000);

    SQL_PORT = getGlobalNumber("sqlPort", 3306);
    MYSQL_READ_TIMEOUT = getGlobalNumber("mysqlReadTimeout", 10);
    MYSQL_WRITE_TIMEOUT = getGlobalNumber("mysqlWriteTimeout", 10);
    SQL_KEEPALIVE = getGlobalNumber("sqlKeepAlive", 0);

    GUILD_FORM_LEVEL = getGlobalNumber("guild_form_level", 1000);

    //stringi
    DATA_DIR = getGlobalString("datadir");
    MAP_PATH = getGlobalString("map");
    IP = getGlobalString("ip", "127.0.0.1");
    WORLD_NAME = getGlobalString("worldname","KentanaOTS");
    WORLD_TYPE = getGlobalString("worldtype");
    MOTD = getGlobalString("motd");
    SUSPEND_MSG = getGlobalString("suspendmsg");
    PRIORYTET = getGlobalString("priorytet");
    SERVER_NAME = getGlobalString("servername");
    LOGIN_MSG = getGlobalString("loginmsg", "Witamy.");
    SQL_HOST = getGlobalString("sqlHost", "localhost");
    SQL_DB = getGlobalString("sqlDatabase", "kentana");
    SQL_USER = getGlobalString("sqlUser", "root");
    SQL_PASS = getGlobalString("sqlPass", "");
    LOAD_NPC = getGlobalString("loadNpc", "all");
    //bool
    CAP_SYSTEM = getGlobalBool("capsystem");
#ifdef YUR_PREMIUM_PROMOTION
    FREE_PREMMY = getGlobalBool("freepremmy");
    QUEUE_PREMMY = getGlobalBool("queuepremmy");
#endif //YUR_PREMIUM_PROMOTION
    GM_MSG = getGlobalBool("gmmsg", false);
    ENDING_AMMO = getGlobalBool("ending_ammo", true);
    SUMMONS_ALL_VOC = getGlobalBool("summonsallvoc", false);
    BANMSG = getGlobalBool("banmsg", false);
    OPTIMIZE_DB_AT_STARTUP = getGlobalBool("optimizeDatabaseAtStartup", true);
    STAGE_EXP = getGlobalBool("enableStageExp", true);
#ifdef HUCZU_PAY_SYSTEM
    PAY_SYSTEM = getGlobalBool("enablePaySystem", false);
    HOMEPAY_USR_ID = getGlobalNumber("homePayUserId", 123);
#endif //HUCZU_PAY_SYSTEM
    //tabele
    NO_VOCATION_SPEED = atof(getGlobalStringField("szybkoscataku", 1, "2.0").c_str());
    SORCERER_SPEED = atof(getGlobalStringField("szybkoscataku", 2, "2.0").c_str());
    DRUID_SPEED = atof(getGlobalStringField("szybkoscataku", 3, "2.0").c_str());
    PALADIN_SPEED = atof(getGlobalStringField("szybkoscataku", 4, "2.0").c_str());
    KNIGHT_SPEED = atof(getGlobalStringField("szybkoscataku", 5, "2.0").c_str());

    CAP_GAIN[VOCATION_NONE] = atoi(getGlobalStringField("capgain", VOCATION_NONE + 1, "10").c_str());
    CAP_GAIN[VOCATION_SORCERER] = atoi(getGlobalStringField("capgain", VOCATION_SORCERER + 1, "10").c_str());
    CAP_GAIN[VOCATION_DRUID] = atoi(getGlobalStringField("capgain", VOCATION_DRUID + 1, "10").c_str());
    CAP_GAIN[VOCATION_PALADIN] = atoi(getGlobalStringField("capgain", VOCATION_PALADIN + 1, "20").c_str());
    CAP_GAIN[VOCATION_KNIGHT] = atoi(getGlobalStringField("capgain", VOCATION_KNIGHT + 1, "25").c_str());

    MANA_GAIN[VOCATION_NONE] = atoi(getGlobalStringField("managain", VOCATION_NONE + 1, "5").c_str());
    MANA_GAIN[VOCATION_SORCERER] = atoi(getGlobalStringField("managain", VOCATION_SORCERER + 1, "30").c_str());
    MANA_GAIN[VOCATION_DRUID] = atoi(getGlobalStringField("managain", VOCATION_DRUID + 1, "30").c_str());
    MANA_GAIN[VOCATION_PALADIN] = atoi(getGlobalStringField("managain", VOCATION_PALADIN + 1, "15").c_str());
    MANA_GAIN[VOCATION_KNIGHT] = atoi(getGlobalStringField("managain", VOCATION_KNIGHT + 1, "5").c_str());

    HP_GAIN[VOCATION_NONE] = atoi(getGlobalStringField("hpgain", VOCATION_NONE + 1, "5").c_str());
    HP_GAIN[VOCATION_SORCERER] = atoi(getGlobalStringField("hpgain", VOCATION_SORCERER + 1, "5").c_str());
    HP_GAIN[VOCATION_DRUID] = atoi(getGlobalStringField("hpgain", VOCATION_DRUID + 1, "5").c_str());
    HP_GAIN[VOCATION_PALADIN] = atoi(getGlobalStringField("hpgain", VOCATION_PALADIN + 1, "10").c_str());
    HP_GAIN[VOCATION_KNIGHT] = atoi(getGlobalStringField("hpgain", VOCATION_KNIGHT + 1, "15").c_str());

    WEAPON_MUL[VOCATION_NONE] = atoi(getGlobalStringField("weaponmul", VOCATION_NONE+1, "1").c_str());
    WEAPON_MUL[VOCATION_SORCERER] = atoi(getGlobalStringField("weaponmul", VOCATION_SORCERER+1, "1").c_str());
    WEAPON_MUL[VOCATION_DRUID] = atoi(getGlobalStringField("weaponmul", VOCATION_DRUID+1, "1").c_str());
    WEAPON_MUL[VOCATION_PALADIN] = atoi(getGlobalStringField("weaponmul", VOCATION_PALADIN+1, "1").c_str());
    WEAPON_MUL[VOCATION_KNIGHT] = atoi(getGlobalStringField("weaponmul", VOCATION_KNIGHT+1, "1").c_str());

    DIST_MUL[VOCATION_NONE] = atoi(getGlobalStringField("distmul", VOCATION_NONE+1, "1").c_str());
    DIST_MUL[VOCATION_SORCERER] = atoi(getGlobalStringField("distmul", VOCATION_SORCERER+1, "1").c_str());
    DIST_MUL[VOCATION_DRUID] = atoi(getGlobalStringField("distmul", VOCATION_DRUID+1, "1").c_str());
    DIST_MUL[VOCATION_PALADIN] = atoi(getGlobalStringField("distmul", VOCATION_PALADIN+1, "1").c_str());
    DIST_MUL[VOCATION_KNIGHT] = atoi(getGlobalStringField("distmul", VOCATION_KNIGHT+1, "1").c_str());

    SHIELD_MUL[VOCATION_NONE] = atoi(getGlobalStringField("shieldmul", VOCATION_NONE+1, "1").c_str());
    SHIELD_MUL[VOCATION_SORCERER] = atoi(getGlobalStringField("shieldmul", VOCATION_SORCERER+1, "1").c_str());
    SHIELD_MUL[VOCATION_DRUID] = atoi(getGlobalStringField("shieldmul", VOCATION_DRUID+1, "1").c_str());
    SHIELD_MUL[VOCATION_PALADIN] = atoi(getGlobalStringField("shieldmul", VOCATION_PALADIN+1, "1").c_str());
    SHIELD_MUL[VOCATION_KNIGHT] = atoi(getGlobalStringField("shieldmul", VOCATION_KNIGHT+1, "1").c_str());

    MANA_MUL[VOCATION_NONE] = atoi(getGlobalStringField("manamul", VOCATION_NONE+1, "1").c_str());
    MANA_MUL[VOCATION_SORCERER] = atoi(getGlobalStringField("manamul", VOCATION_SORCERER+1, "1").c_str());
    MANA_MUL[VOCATION_DRUID] = atoi(getGlobalStringField("manamul", VOCATION_DRUID+1, "1").c_str());
    MANA_MUL[VOCATION_PALADIN] = atoi(getGlobalStringField("manamul", VOCATION_PALADIN+1, "1").c_str());
    MANA_MUL[VOCATION_KNIGHT] = atoi(getGlobalStringField("manamul", VOCATION_KNIGHT+1, "1").c_str());

    BURST_DMG_LVL = atof(getGlobalStringField("burstarrowdmg", 1, "2.0").c_str());
    BURST_DMG_MLVL = atof(getGlobalStringField("burstarrowdmg", 2, "3.0").c_str());
    BURST_DMG_LO = atof(getGlobalStringField("burstarrowdmg", 3, "0.24").c_str());
    BURST_DMG_HI = atof(getGlobalStringField("burstarrowdmg", 4, "0.55").c_str());

#ifdef YUR_CVS_MODS
    VOCATIONS[VOCATION_KNIGHT] = getGlobalStringField("vocations",VOCATION_KNIGHT);
    VOCATIONS[VOCATION_PALADIN] = getGlobalStringField("vocations",VOCATION_PALADIN);
    VOCATIONS[VOCATION_SORCERER] = getGlobalStringField("vocations",VOCATION_SORCERER);
    VOCATIONS[VOCATION_DRUID] = getGlobalStringField("vocations",VOCATION_DRUID);

    PROMOTED_VOCATIONS[VOCATION_NONE] = getGlobalStringField("promoted_vocations",VOCATION_NONE);
    PROMOTED_VOCATIONS[VOCATION_KNIGHT] = getGlobalStringField("promoted_vocations",VOCATION_KNIGHT);
    PROMOTED_VOCATIONS[VOCATION_PALADIN] = getGlobalStringField("promoted_vocations",VOCATION_PALADIN);
    PROMOTED_VOCATIONS[VOCATION_SORCERER] = getGlobalStringField("promoted_vocations",VOCATION_SORCERER);
    PROMOTED_VOCATIONS[VOCATION_DRUID] = getGlobalStringField("promoted_vocations",VOCATION_DRUID);

    DIE_PERCENT_EXP = atoi(getGlobalStringField("diepercent",1,"7").c_str());
    DIE_PERCENT_MANA = atoi(getGlobalStringField("diepercent",2,"7").c_str());
    DIE_PERCENT_SKILL = atoi(getGlobalStringField("diepercent",3,"7").c_str());
    DIE_PERCENT_EQ = atoi(getGlobalStringField("diepercent",4,"7").c_str());
    DIE_PERCENT_BP = atoi(getGlobalStringField("diepercent",5,"100").c_str());
#endif //YUR_CVS_MODS

    DIE_PERCENT_SL = getGlobalDouble("diepercent_sl",0.5);

    MANA_SNAKEBITE = atoi(getGlobalStringField("rodmana", 1, "2").c_str());
    MANA_MOONLIGHT = atoi(getGlobalStringField("rodmana", 2, "3").c_str());
    MANA_VOLCANIC = atoi(getGlobalStringField("rodmana", 3, "5").c_str());
    MANA_QUAGMIRE = atoi(getGlobalStringField("rodmana", 4, "8").c_str());
    MANA_TEMPEST = atoi(getGlobalStringField("rodmana", 5, "13").c_str());

    MANA_VORTEX = atoi(getGlobalStringField("wandmana", 1, "2").c_str());
    MANA_DRAGONBREATH = atoi(getGlobalStringField("wandmana", 2, "3").c_str());
    MANA_PLAGUE = atoi(getGlobalStringField("wandmana", 3, "5").c_str());
    MANA_COSMIC = atoi(getGlobalStringField("wandmana", 4, "8").c_str());
    MANA_INFERNO = atoi(getGlobalStringField("wandmana", 5, "13").c_str());

    RANGE_SNAKEBITE = atoi(getGlobalStringField("rodrange", 1, "4").c_str());
    RANGE_MOONLIGHT = atoi(getGlobalStringField("rodrange", 2, "3").c_str());
    RANGE_VOLCANIC = atoi(getGlobalStringField("rodrange", 3, "2").c_str());
    RANGE_QUAGMIRE = atoi(getGlobalStringField("rodrange", 4, "1").c_str());
    RANGE_TEMPEST = atoi(getGlobalStringField("rodrange", 5, "3").c_str());

    RANGE_VORTEX = atoi(getGlobalStringField("wandrange", 1, "4").c_str());
    RANGE_DRAGONBREATH = atoi(getGlobalStringField("wandrange", 2, "3").c_str());
    RANGE_PLAGUE = atoi(getGlobalStringField("wandrange", 3, "2").c_str());
    RANGE_COSMIC = atoi(getGlobalStringField("wandrange", 4, "1").c_str());
    RANGE_INFERNO = atoi(getGlobalStringField("wandrange", 5, "3").c_str());

    return true;
}

bool LuaScript::getGlobalBool(std::string var, bool _default /*= false*/)
{
    lua_getglobal(luaState, var.c_str());

    if(!lua_isboolean(luaState, -1))
    {
        std::cout << "Uwaga! Brak wartosci dla : " << var << " . Ustawiono domyslna wartosc: " << _default << std::endl;
        return _default;
    }

    bool val = lua_toboolean(luaState, -1);
    lua_pop(luaState,1);
    return val;
}

std::string LuaScript::getGlobalString(std::string var, const std::string &defString)
{
    lua_getglobal(luaState, var.c_str());

    if(!lua_isstring(luaState, -1))
    {
        std::cout << "Uwaga! Brak wartosci dla : " << var << " . Ustawiono domyslna wartosc: " << defString << std::endl;
        return defString;
    }

    int32_t len = (int32_t)lua_strlen(luaState, -1);
    std::string ret(lua_tostring(luaState, -1), len);
    lua_pop(luaState,1);

    return ret;
}

int32_t LuaScript::getGlobalNumber(std::string var, const int32_t defNum)
{
    lua_getglobal(luaState, var.c_str());

    if(!lua_isnumber(luaState, -1))
    {
        std::cout << "Uwaga! Brak wartosci dla : " << var << " . Ustawiono domyslna wartosc: " << defNum << std::endl;
        return defNum;
    }

    int32_t val = (int32_t)lua_tonumber(luaState, -1);
    lua_pop(luaState,1);

    return val;
}

double LuaScript::getGlobalDouble(std::string var, const double defNum)
{
    lua_getglobal(luaState, var.c_str());

    if(!lua_isnumber(luaState, -1))
    {
        std::cout << "Uwaga! Brak wartosci dla : " << var << " . Ustawiono domyslna wartosc: " << defNum << std::endl;
        return defNum;
    }

    double val = lua_tonumber(luaState, -1);
    lua_pop(luaState,1);

    return val;
}


int32_t LuaScript::setGlobalString(std::string var, std::string val)
{
    return false;
}

int32_t LuaScript::setGlobalNumber(std::string var, int32_t val)
{
    lua_pushnumber(luaState, val);
    lua_setglobal(luaState, var.c_str());
    return true;
}

std::string LuaScript::getGlobalStringField (std::string var, const int32_t key, const std::string &defString)
{
    lua_getglobal(luaState, var.c_str());

    lua_pushnumber(luaState, key);
    lua_gettable(luaState, -2);  /* get table[key] */
    if(!lua_isstring(luaState, -1))
        return defString;
    std::string result = lua_tostring(luaState, -1);
    lua_pop(luaState, 2);  /* remove number and key*/
    return result;
}

int32_t LuaScript::getField (const char *key)
{
    int32_t result;
    lua_pushstring(luaState, key);
    lua_gettable(luaState, -2);  /* get table[key] */
    result = (int32_t)lua_tonumber(luaState, -1);
    lua_pop(luaState, 1);  /* remove number and key*/
    return result;
}

void LuaScript::setField (const char *index, int32_t val)
{
    lua_pushstring(luaState, index);
    lua_pushnumber(luaState, (double)val);
    lua_settable(luaState, -3);
}


int32_t LuaScript::getField (lua_State *L , const char *key)
{
    int32_t result;
    lua_pushstring(L, key);
    lua_gettable(L, -2);  /* get table[key] */
    result = (int32_t)lua_tonumber(L, -1);
    lua_pop(L, 1);  /* remove number and key*/
    return result;
}

void LuaScript::setField (lua_State *L, const char *index, int32_t val)
{
    lua_pushstring(L, index);
    lua_pushnumber(L, (double)val);
    lua_settable(L, -3);
}
