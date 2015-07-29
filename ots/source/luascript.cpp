//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// class which takes care of all data which must get saved in the player
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


int LuaScript::OpenFile(const char *filename)
{
	luaState = lua_open();

	if (lua_dofile(luaState, filename))
		return false;

#ifdef YUR_MULTIPLIERS
	EXP_MUL = getGlobalNumber("expmul",1);
	EXP_MUL_PVP = getGlobalNumber("expmulpvp",1);
	HEALTH_TICK_MUL = getGlobalNumber("healthtickmul",1);
	MANA_TICK_MUL = getGlobalNumber("manatickmul",1);

	CAP_GAIN[VOCATION_NONE] = atoi(getGlobalStringField("capgain", VOCATION_NONE + 1, "10").c_str());
	CAP_GAIN[VOCATION_SORCERER] = atoi(getGlobalStringField("capgain", VOCATION_SORCERER + 1, "10").c_str());
	CAP_GAIN[VOCATION_DRUID] = atoi(getGlobalStringField("capgain", VOCATION_DRUID + 1, "10").c_str());
	CAP_GAIN[VOCATION_PALADIN] = atoi(getGlobalStringField("capgain", VOCATION_PALADIN + 1, "20").c_str());
	CAP_GAIN[VOCATION_KNIGHT] = atoi(getGlobalStringField("capgain", VOCATION_KNIGHT + 1, "25").c_str());

	MANA_GAIN[VOCATION_NONE] = atoll(getGlobalStringField("managain", VOCATION_NONE + 1, "5").c_str());
	MANA_GAIN[VOCATION_SORCERER] = atoll(getGlobalStringField("managain", VOCATION_SORCERER + 1, "30").c_str());
	MANA_GAIN[VOCATION_DRUID] = atoll(getGlobalStringField("managain", VOCATION_DRUID + 1, "30").c_str());
	MANA_GAIN[VOCATION_PALADIN] = atoll(getGlobalStringField("managain", VOCATION_PALADIN + 1, "15").c_str());
	MANA_GAIN[VOCATION_KNIGHT] = atoll(getGlobalStringField("managain", VOCATION_KNIGHT + 1, "5").c_str());

	HP_GAIN[VOCATION_NONE] = atoll(getGlobalStringField("hpgain", VOCATION_NONE + 1, "5").c_str());
	HP_GAIN[VOCATION_SORCERER] = atoll(getGlobalStringField("hpgain", VOCATION_SORCERER + 1, "5").c_str());
	HP_GAIN[VOCATION_DRUID] = atoll(getGlobalStringField("hpgain", VOCATION_DRUID + 1, "5").c_str());
	HP_GAIN[VOCATION_PALADIN] = atoll(getGlobalStringField("hpgain", VOCATION_PALADIN + 1, "10").c_str());
	HP_GAIN[VOCATION_KNIGHT] = atoll(getGlobalStringField("hpgain", VOCATION_KNIGHT + 1, "15").c_str());

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

	MANA_MUL[VOCATION_NONE] = atoll(getGlobalStringField("manamul", VOCATION_NONE+1, "1").c_str());
	MANA_MUL[VOCATION_SORCERER] = atoll(getGlobalStringField("manamul", VOCATION_SORCERER+1, "1").c_str());
	MANA_MUL[VOCATION_DRUID] = atoll(getGlobalStringField("manamul", VOCATION_DRUID+1, "1").c_str());
	MANA_MUL[VOCATION_PALADIN] = atoll(getGlobalStringField("manamul", VOCATION_PALADIN+1, "1").c_str());
	MANA_MUL[VOCATION_KNIGHT] = atoll(getGlobalStringField("manamul", VOCATION_KNIGHT+1, "1").c_str());
#endif //YUR_MULTIPLIERS

#ifdef TR_ANTI_AFK
	KICK_TIME = getGlobalNumber("kicktime",15)*60000;
#endif //TR_ANTI_AFK

#ifdef YUR_LEARN_SPELLS
	LEARN_SPELLS = getGlobalString("learnspells") == "yes";
#endif //YUR_LEARN_SPELLS

#ifdef TLM_HOUSE_SYSTEM
	ACCESS_HOUSE = getGlobalNumber("accesshouse",3);
	MAX_HOUSE_TILE_ITEMS = getGlobalNumber("maxhousetileitems",10);
#endif //TLM_HOUSE_SYSTEM

#ifdef TR_SUMMONS
	SUMMONS_ALL_VOC = getGlobalString("summonsallvoc") == "yes";
	MAX_SUMMONS = (size_t)getGlobalNumber("maxsummons",2);
#endif //TR_SUMMONS

#ifdef TLM_SKULLS_PARTY
	HIT_TIME = getGlobalNumber("hittime",1)*60000;
	WHITE_TIME = getGlobalNumber("whitetime",15)*60000;
	RED_TIME = getGlobalNumber("redtime",5*60)*60000;
	FRAG_TIME = getGlobalNumber("fragtime",10*60)*60000;
	RED_UNJUST = getGlobalNumber("redunjust",3);
	BAN_UNJUST = getGlobalNumber("banunjust",6);
#endif //TLM_SKULLS_PARTY

#ifdef SD_BURST_ARROW
	BURST_DMG_LVL = atof(getGlobalStringField("burstarrowdmg", 1, "2.0").c_str());
	BURST_DMG_MLVL = atof(getGlobalStringField("burstarrowdmg", 2, "3.0").c_str());
	BURST_DMG_LO = atof(getGlobalStringField("burstarrowdmg", 3, "0.24").c_str());
	BURST_DMG_HI = atof(getGlobalStringField("burstarrowdmg", 4, "0.55").c_str());
#endif //SD_BURST_ARROW

#ifdef YUR_CONFIG_CAP
	CAP_SYSTEM = getGlobalString("capsystem") == "yes";
#endif //YUR_CONFIG_CAP

#ifdef BDB_REPLACE_SPEARS
	SPEAR_LOSE_CHANCE = getGlobalNumber("spearlosechance",50);
#endif //BDB_REPLACE_SPEARS

#ifdef YUR_PREMIUM_PROMOTION
	FREE_PREMMY = getGlobalString("freepremmy") == "yes";
	QUEUE_PREMMY = getGlobalString("queuepremmy") == "yes";
#endif //YUR_PREMIUM_PROMOTION

#ifdef YUR_CVS_MODS
	VOCATIONS[VOCATION_KNIGHT] = getGlobalStringField("vocations",VOCATION_KNIGHT);
	VOCATIONS[VOCATION_PALADIN] = getGlobalStringField("vocations",VOCATION_PALADIN);
	VOCATIONS[VOCATION_SORCERER] = getGlobalStringField("vocations",VOCATION_SORCERER);
	VOCATIONS[VOCATION_DRUID] = getGlobalStringField("vocations",VOCATION_DRUID);

	PROMOTED_VOCATIONS[VOCATION_KNIGHT] = getGlobalStringField("promoted_vocations",VOCATION_KNIGHT);
	PROMOTED_VOCATIONS[VOCATION_PALADIN] = getGlobalStringField("promoted_vocations",VOCATION_PALADIN);
	PROMOTED_VOCATIONS[VOCATION_SORCERER] = getGlobalStringField("promoted_vocations",VOCATION_SORCERER);
	PROMOTED_VOCATIONS[VOCATION_DRUID] = getGlobalStringField("promoted_vocations",VOCATION_DRUID);

	DIE_PERCENT_EXP = atoi(getGlobalStringField("diepercent",1,"7").c_str());
	DIE_PERCENT_MANA = atoll(getGlobalStringField("diepercent",2,"7").c_str());
	DIE_PERCENT_SKILL = atoi(getGlobalStringField("diepercent",3,"7").c_str());
	DIE_PERCENT_EQ = atoi(getGlobalStringField("diepercent",4,"7").c_str());
	DIE_PERCENT_BP = atoi(getGlobalStringField("diepercent",5,"100").c_str());

	ACCESS_PROTECT = getGlobalNumber("accessprotect",3);
	ACCESS_REMOTE = getGlobalNumber("accessremote",3);
	ACCESS_TALK = getGlobalNumber("accesstalk",3);
	ACCESS_ENTER = getGlobalNumber("accessenter",3);
	ACCESS_LOOK = getGlobalNumber("accesslook",3);

	EXHAUSTED = getGlobalNumber("exhausted",0);
	EXHAUSTED_ADD = getGlobalNumber("exhaustedadd",0);
	EXHAUSTED_HEAL = getGlobalNumber("exhaustedheal",0);

	PZ_LOCKED = getGlobalNumber("pzlocked",0);
	MAX_DEPOT_ITEMS = getGlobalNumber("maxdepotitem",1000);
	DATA_DIR = getGlobalString("datadir");
#endif //YUR_CVS_MODS

#ifdef JD_DEATH_LIST
	MAX_DEATH_ENTRIES = getGlobalNumber("maxdeathentries",10);
#endif //JD_DEATH_LIST

#ifdef JD_WANDS
	MANA_SNAKEBITE = atoll(getGlobalStringField("rodmana", 1, "2").c_str());
	MANA_MOONLIGHT = atoll(getGlobalStringField("rodmana", 2, "3").c_str());
	MANA_VOLCANIC = atoll(getGlobalStringField("rodmana", 3, "5").c_str());
	MANA_QUAGMIRE = atoll(getGlobalStringField("rodmana", 4, "8").c_str());
	MANA_TEMPEST = atoll(getGlobalStringField("rodmana", 5, "13").c_str());

	MANA_VORTEX = atoll(getGlobalStringField("wandmana", 1, "2").c_str());
	MANA_DRAGONBREATH = atoll(getGlobalStringField("wandmana", 2, "3").c_str());
	MANA_PLAGUE = atoll(getGlobalStringField("wandmana", 3, "5").c_str());
	MANA_COSMIC = atoll(getGlobalStringField("wandmana", 4, "8").c_str());
	MANA_INFERNO = atoll(getGlobalStringField("wandmana", 5, "13").c_str());

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
#endif //JD_WANDS

#ifdef YUR_BUILTIN_AAC
	ACCMAKER = getGlobalString("accmaker") != "none";
	ACCMAKER_ROOK = getGlobalString("accmaker") == "rook";
#endif //YUR_BUILTIN_AAC

	return true;
}


std::string LuaScript::getGlobalString(std::string var, const std::string &defString)
{
	lua_getglobal(luaState, var.c_str());

  if(!lua_isstring(luaState, -1))
  	  return defString;

	int len = (int)lua_strlen(luaState, -1);
	std::string ret(lua_tostring(luaState, -1), len);
	lua_pop(luaState,1);

	return ret;
}

int LuaScript::getGlobalNumber(std::string var, const int defNum)
{
	lua_getglobal(luaState, var.c_str());

  if(!lua_isnumber(luaState, -1))
  	  return defNum;

	int val = (int)lua_tonumber(luaState, -1);
	lua_pop(luaState,1);

	return val;
}


int LuaScript::setGlobalString(std::string var, std::string val)
{
	return false;
}

int LuaScript::setGlobalNumber(std::string var, int val){
	lua_pushnumber(luaState, val);
	lua_setglobal(luaState, var.c_str());
	return true;
}

std::string LuaScript::getGlobalStringField (std::string var, const int key, const std::string &defString) {
      lua_getglobal(luaState, var.c_str());

      lua_pushnumber(luaState, key);
      lua_gettable(luaState, -2);  /* get table[key] */
      if(!lua_isstring(luaState, -1))
  	  return defString;
      std::string result = lua_tostring(luaState, -1);
      lua_pop(luaState, 2);  /* remove number and key*/
      return result;
}

int LuaScript::getField (const char *key) {
      int result;
      lua_pushstring(luaState, key);
      lua_gettable(luaState, -2);  /* get table[key] */
      result = (int)lua_tonumber(luaState, -1);
      lua_pop(luaState, 1);  /* remove number and key*/
      return result;
}

void LuaScript::setField (const char *index, int val) {
      lua_pushstring(luaState, index);
      lua_pushnumber(luaState, (double)val);
      lua_settable(luaState, -3);
    }


int LuaScript::getField (lua_State *L , const char *key) {
      int result;
      lua_pushstring(L, key);
      lua_gettable(L, -2);  /* get table[key] */
      result = (int)lua_tonumber(L, -1);
      lua_pop(L, 1);  /* remove number and key*/
      return result;
}

void LuaScript::setField (lua_State *L, const char *index, int val) {
      lua_pushstring(L, index);
      lua_pushnumber(L, (double)val);
      lua_settable(L, -3);
    }
