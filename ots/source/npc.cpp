//#include "preheaders.h"

#include <algorithm>
#include <functional>
#include <string>
#include <sstream>
#include <fstream>

#include "npc.h"
#include "luascript.h"
#include "player.h"
#include "chat.h"

extern LuaScript g_config;
extern Chat g_chat;

AutoList<Npc> Npc::listNpc;

Npc::Npc(const std::string& name , Game* game) :
    Creature()
{
    char *tmp;
    useCount = 0;
    this->loaded = false;
    this->name = name;
    std::string datadir = g_config.DATA_DIR;
    std::string filename = datadir + "npc/" + std::string(name) + ".xml";
    //std::transform(filename.begin(), filename.end(), filename.begin(), tolower);
    toLowerCaseString(filename);
    xmlDocPtr doc = xmlParseFile(filename.c_str());
    if(doc)
    {
        this->loaded=true;
        xmlNodePtr root, p;
        root = xmlDocGetRootElement(doc);

        if (xmlStrcmp(root->name,(const xmlChar*) "npc"))
        {
            //TODO: use exceptions here
            std::cerr << "Malformed XML" << std::endl;
        }

        p = root->children;

        tmp = (char*)xmlGetProp(root, (const xmlChar *)"script");
        if(tmp)
        {
            this->scriptname = tmp;
            xmlFreeOTSERV(tmp);
        }
        else
        {
            this->scriptname = "";
        }

        if(tmp = (char*)xmlGetProp(root, (const xmlChar *)"name"))
        {
            this->name = tmp;
            xmlFreeOTSERV(tmp);
        }
        else
        {
            this->name = "";
        }

        if(tmp = (char*)xmlGetProp(root, (const xmlChar *)"access"))
        {
            access = atoi(tmp);
            xmlFreeOTSERV(tmp);
        }
        else
        {
            access = 0;
        }

        if(tmp = (char*)xmlGetProp(root, (const xmlChar *)"level"))
        {
            level = atoi(tmp);
            xmlFreeOTSERV(tmp);
            setNormalSpeed();
            //std::cout << level << std::endl;
        }
        else
        {
            level = 1;
        }

        if(tmp = (char*)xmlGetProp(root, (const xmlChar *)"maglevel"))
        {
            maglevel = atoll(tmp);
            xmlFreeOTSERV(tmp);
            //std::cout << maglevel << std::endl;
        }
        else
        {
            maglevel = 1;
        }

        while (p)
        {
            const char* str = (char*)p->name;
            if(strcmp(str, "mana") == 0)
            {
                if(tmp = (char*)xmlGetProp(p, (const xmlChar *)"now"))
                {
                    this->mana = atoll(tmp);
                    xmlFreeOTSERV(tmp);
                }
                else
                {
                    this->mana = 100;
                }
                if(tmp = (char*)xmlGetProp(p, (const xmlChar *)"max"))
                {
                    this->manamax = atoll(tmp);
                    xmlFreeOTSERV(tmp);
                }
                else
                {
                    this->manamax = 100;
                }
            }
            if(strcmp(str, "health") == 0)
            {
                if(tmp = (char*)xmlGetProp(p, (const xmlChar *)"now"))
                {
                    this->health = atoll(tmp);
                    xmlFreeOTSERV(tmp);
                }
                else
                {
                    this->health = 100;
                }
                if(tmp = (char*)xmlGetProp(p, (const xmlChar *)"max"))
                {
                    this->healthmax = atoll(tmp);
                    xmlFreeOTSERV(tmp);
                }
                else
                {
                    this->healthmax = 100;
                }
            }
            if(strcmp(str, "look") == 0)
            {
                if(tmp = (char*)xmlGetProp(p, (const xmlChar *)"type"))
                {
                    this->looktype = atoi(tmp);
                    xmlFreeOTSERV(tmp);
                }
                else
                {
                    this->looktype = 20;
                }
                this->lookmaster = this->looktype;
                if(tmp = (char*)xmlGetProp(p, (const xmlChar *)"head"))
                {
                    this->lookhead = atoi(tmp);
                    xmlFreeOTSERV(tmp);
                }
                else
                {
                    this->lookhead = 10;
                }

                if(tmp = (char*)xmlGetProp(p, (const xmlChar *)"body"))
                {
                    this->lookbody = atoi(tmp);
                    xmlFreeOTSERV(tmp);
                }
                else
                {
                    this->lookbody = 20;
                }

                if(tmp = (char*)xmlGetProp(p, (const xmlChar *)"legs"))
                {
                    this->looklegs = atoi(tmp);
                    xmlFreeOTSERV(tmp);
                }
                else
                {
                    this->looklegs = 30;
                }

                if(tmp = (char*)xmlGetProp(p, (const xmlChar *)"feet"))
                {
                    this->lookfeet = atoi(tmp);
                    xmlFreeOTSERV(tmp);
                }
                else
                {
                    this->lookfeet = 40;
                }
                if(tmp = (char*)xmlGetProp(p, (const xmlChar *)"corpse"))
                {
                    this->lookcorpse = atoi(tmp);
                    xmlFreeOTSERV(tmp);
                }
                else
                {
                    this->lookcorpse = 100;
                }

            }
            if(strcmp(str, "attack") == 0)
            {
                if(tmp = (char*)xmlGetProp(p, (const xmlChar *)"type"))
                {
                    std::string attacktype = tmp;
                    xmlFreeOTSERV(tmp);
                    if(attacktype == "melee")
                        this->fighttype = FIGHT_MELEE;
                    tmp = (char*)xmlGetProp(p, (const xmlChar *)"damage");
                    if(tmp)
                    {
                        this->damage = atoll(tmp);
                        xmlFreeOTSERV(tmp);
                    }
                    else
                    {
                        this->damage = 5;
                    }
                }
                else
                {
                    this->fighttype = FIGHT_MELEE;
                    this->damage = 0;
                }
            }
            if(strcmp(str, "loot") == 0)
            {
                //TODO implement loot
            }

            p = p->next;
        }

        xmlFreeDoc(doc);
    }
    //now try to load the script
    this->script = new NpcScript(this->scriptname, this);
    if(!this->script->isLoaded())
        this->loaded=false;
    this->game=game;
}


Npc::~Npc()
{
    delete this->script;
}

std::string Npc::getDescription(bool self) const
{
    std::stringstream s;
    std::string str;
    s << name << ".";
    str = s.str();
    return str;
}

void Npc::onThingMove(const Player *player, const Thing *thing, const Position *oldPos,
                      unsigned char oldstackpos, unsigned char oldcount, unsigned char count)
{
    //not yet implemented
}

void Npc::onCreatureAppear(const Creature *creature)
{
    this->script->onCreatureAppear(creature->getID());
}

void Npc::onCreatureDisappear(const Creature *creature, unsigned char stackPos, bool tele)
{
    this->script->onCreatureDisappear(creature->getID());
}

void Npc::onThingDisappear(const Thing* thing, unsigned char stackPos)
{
    const Creature *creature = dynamic_cast<const Creature*>(thing);
    if(creature)
        this->script->onCreatureDisappear(creature->getID());
}
void Npc::onThingAppear(const Thing* thing)
{
    const Creature *creature = dynamic_cast<const Creature*>(thing);
    if(creature)
        this->script->onCreatureAppear(creature->getID());
}

void Npc::onCreatureTurn(const Creature *creature, unsigned char stackpos)
{
    //not implemented yet, do we need it?
}

/*
void Npc::setAttackedCreature(uint32_t id){
	//not implemented yet
}
*/

void Npc::onCreatureSay(const Creature *creature, SpeakClasses type, const std::string &text)
{
    if(creature->getID() == this->getID())
        return;
    this->script->onCreatureSay(creature->getID(), type, text);
}

void Npc::onCreatureChangeOutfit(const Creature* creature)
{
#ifdef __DEBUG_NPC__
    std::cout << "Npc::onCreatureChangeOutfit" << std::endl;
#endif
    //we dont care about filthy player changing his ugly clothes
}

int32_t Npc::onThink(int32_t& newThinkTicks)
{
    this->script->onThink();
    return Creature::onThink(newThinkTicks);
}


void Npc::doSay(std::string msg)
{
    if(!game->creatureSaySpell(this, msg))
    {
        //game->stopEvent(eventTalk);
        eventTalk = game->addEvent(makeTask(800, boost::bind(&Game::creatureSay, game, this, SPEAK_SAY, msg)));
    }
}

void Npc::doAttack(int32_t id)
{
    attackedCreature = id;
}

void Npc::doMove(int32_t direction)
{
    switch(direction)
    {
    case 0:
        this->game->thingMove(this, this,this->pos.x, this->pos.y+1, this->pos.z, 1);
        break;
    case 1:
        this->game->thingMove(this, this,this->pos.x+1, this->pos.y, this->pos.z, 1);
        break;
    case 2:
        this->game->thingMove(this, this,this->pos.x, this->pos.y-1, this->pos.z, 1);
        break;
    case 3:
        this->game->thingMove(this, this,this->pos.x-1, this->pos.y, this->pos.z, 1);
        break;
    }
}

void Npc::doMoveTo(Position target)
{
    if(route.empty() || route.back() != target || route.front() != this->pos)
    {
        route = this->game->getPathTo(this, this->pos, target);
    }
    if(route.empty())
    {
        //still no route, means there is none
        return;
    }
    else route.pop_front();
    Position nextStep=route.front();
    route.pop_front();
    int32_t dx = nextStep.x - this->pos.x;
    int32_t dy = nextStep.y - this->pos.y;
    this->game->thingMove(this, this,this->pos.x + dx, this->pos.y + dy, this->pos.z, 1);
}

NpcScript::NpcScript(std::string scriptname, Npc* npc)
{
    this->npc = NULL;
    this->loaded = false;
    if(scriptname == "")
        return;
    luaState = lua_open();
    luaopen_loadlib(luaState);
    luaopen_base(luaState);
    luaopen_math(luaState);
    luaopen_string(luaState);
    luaopen_io(luaState);

    std::string datadir = g_config.DATA_DIR;
    lua_dofile(luaState, std::string(datadir + "npc/scripts/lib/npc.lua").c_str());

#ifdef USING_VISUAL_2005
    FILE* in = NULL;
    fopen_s(&in, scriptname.c_str(), "r");
#else
    FILE* in=fopen(scriptname.c_str(), "r");
#endif //USING_VISUAL_2005

    if(!in)
        return;
    else
        fclose(in);
    lua_dofile(luaState, scriptname.c_str());
    this->loaded=true;
    this->npc=npc;
    this->setGlobalNumber("addressOfNpc", (int32_t)npc); //VISUAL
    this->registerFunctions();
}

void NpcScript::onThink()
{
    lua_pushstring(luaState, "onThink");
    lua_gettable(luaState, LUA_GLOBALSINDEX);
    if(lua_pcall(luaState, 0, 0, 0))
    {
        std::cerr << "NpcScript: onThink: lua error: " << lua_tostring(luaState, -1) << std::endl;
        lua_pop(luaState,1);
        std::cerr << "Backtrace: " << std::endl;
        lua_Debug* d = NULL;
        int32_t i = 0;
        while(lua_getstack(luaState, i++, d))
        {
            std::cerr << "    " << d->name << " @ " << d->currentline << std::endl;
        }
    }
}


void NpcScript::onCreatureAppear(uint32_t cid)
{
    if(npc->getID() != cid)
    {
        lua_pushstring(luaState, "onCreatureAppear");
        lua_gettable(luaState, LUA_GLOBALSINDEX);
        lua_pushnumber(luaState, cid);
        if(lua_pcall(luaState, 1, 0, 0))
        {
            std::cerr << "NpcScript: onCreatureAppear: lua error: " << lua_tostring(luaState, -1) << std::endl;
            lua_pop(luaState,1);
            std::cerr << "Backtrace: " << std::endl;
            lua_Debug* d = NULL;
            int32_t i = 0;
            while(lua_getstack(luaState, i++, d))
            {
                std::cerr << "    " << d->name << " @ " << d->currentline << std::endl;
            }
        }
    }
}

void NpcScript::onCreatureDisappear(int32_t cid)
{
    lua_pushstring(luaState, "onCreatureDisappear");
    lua_gettable(luaState, LUA_GLOBALSINDEX);
    lua_pushnumber(luaState, cid);
    if(lua_pcall(luaState, 1, 0, 0))
    {
        std::cerr << "NpcScript: onCreatureDisappear: lua error: " << lua_tostring(luaState, -1) << std::endl;
        lua_pop(luaState,1);
        std::cerr << "Backtrace: " << std::endl;
        lua_Debug* d = NULL;
        int32_t i = 0;
        while(lua_getstack(luaState, i++, d))
        {
            std::cerr << "    " << d->name << " @ " << d->currentline << std::endl;
        }
    }
}

void Npc::doLook(const Creature *creature)
{

    int32_t deltax = creature->pos.x - this->pos.x;
    int32_t deltay = creature->pos.y - this->pos.y;

    if(!(deltax == 0 && deltay == 0))
    {
        Direction newdir = this->getDirection();

        //SE
        if(deltax < 0 && deltay < 0)
        {
            if(std::abs(deltax) > std::abs(deltay))
            {
                newdir = WEST;
            }
            else if(std::abs(deltay) > std::abs(deltax))
            {
                newdir = NORTH;
            }
            else if(getDirection() != NORTH && getDirection() != WEST)
            {
                newdir = (rand() % 2 == 0 ? NORTH : WEST);
            }
        }
        //SW
        else if(deltax > 0 && deltay < 0)
        {
            if(std::abs(deltax) > std::abs(deltay))
            {
                newdir = EAST;
            }
            else if(std::abs(deltay) > std::abs(deltax))
            {
                newdir = NORTH;
            }
            else if(getDirection() != NORTH && getDirection() != EAST)
            {
                newdir = (rand() % 2 == 0 ? NORTH : EAST);
            }
        }
        //NW
        else if(deltax > 0 && deltay > 0)
        {
            if(std::abs(deltax) > std::abs(deltay))
            {
                newdir = EAST;
            }
            else if(std::abs(deltay) > std::abs(deltax))
            {
                newdir = SOUTH;
            }
            else if(getDirection() != SOUTH && getDirection() != EAST)
            {
                newdir = (rand() % 2 == 0 ? SOUTH : EAST);
            }
        }
        //NE
        else if(deltax < 0 && deltay > 0)
        {
            if(std::abs(deltax) > std::abs(deltay))
            {
                newdir = WEST;
            }
            else if(std::abs(deltay) > std::abs(deltax))
            {
                newdir = SOUTH;
            }
            else if(getDirection() != SOUTH && getDirection() != EAST)
            {
                newdir = (rand() % 2 == 0 ? SOUTH : WEST);
            }
        }
        //N
        else if(deltax == 0 && deltay > 0)
            newdir = SOUTH;
        //S
        else if(deltax == 0 && deltay < 0)
            newdir = NORTH;
        //W
        else if(deltax > 0 && deltay == 0)
            newdir = EAST;
        //E
        else if(deltax < 0 && deltay == 0)
            newdir = WEST;

        if(newdir != this->getDirection())
        {
            game->creatureTurn(this, newdir);
        }
    }
}

void NpcScript::onCreatureSay(int32_t cid, SpeakClasses type, const std::string &text)
{
    if (!npc->game->getPlayerByID(cid))		// Tibia Rules' fix
        return;

    //now we need to call the function
    lua_pushstring(luaState, "onCreatureSay");
    lua_gettable(luaState, LUA_GLOBALSINDEX);
    lua_pushnumber(luaState, cid);
    lua_pushnumber(luaState, type);
    lua_pushstring(luaState, text.c_str());
    if(lua_pcall(luaState, 3, 0, 0))
    {
        std::cerr << "NpcScript: onCreatureSay: lua error: " << lua_tostring(luaState, -1) << std::endl;
        lua_pop(luaState,1);
        std::cerr << "Backtrace: " << std::endl;
        lua_Debug* d = NULL;
        int32_t i = 0;
        while(lua_getstack(luaState, i++, d))
        {
            std::cerr << "    " << d->name << " @ " << d->currentline << std::endl;
        }
    }
}

int32_t NpcScript::registerFunctions()
{
    lua_register(luaState, "selfSay", NpcScript::luaActionSay);
    lua_register(luaState, "selfMove", NpcScript::luaActionMove);
    lua_register(luaState, "selfMoveTo", NpcScript::luaActionMoveTo);
    lua_register(luaState, "selfGetPosition", NpcScript::luaSelfGetPos);
    lua_register(luaState, "selfAttackCreature", NpcScript::luaActionAttackCreature);
    lua_register(luaState, "creatureGetName", NpcScript::luaCreatureGetName);
    lua_register(luaState, "creatureGetName2", NpcScript::luaCreatureGetName2);
    lua_register(luaState, "creatureGetPosition", NpcScript::luaCreatureGetPos);
    lua_register(luaState, "selfGetPosition", NpcScript::luaSelfGetPos);
    lua_register(luaState, "buy", NpcScript::luaBuyItem);
    lua_register(luaState, "buymanas", NpcScript::luaBuyActionId);
    lua_register(luaState, "sell", NpcScript::luaSellItem);
    lua_register(luaState, "pay", NpcScript::luaPayMoney);
    lua_register(luaState, "trade", NpcScript::luaTradeItem);
    lua_register(luaState, "BuyContainer", NpcScript::luaBuyCont);
    lua_register(luaState, "getPlayerStorageValue", NpcScript::luaGetPlayerStorageValue);
    lua_register(luaState, "setPlayerStorageValue", NpcScript::luaSetPlayerStorageValue);
    lua_register(luaState, "doPlayerRemoveItem", NpcScript::luaPlayerRemoveItem);
    lua_register(luaState, "getPlayerLevel", NpcScript::luaGetPlayerLevel);
    lua_register(luaState, "getPlayerVocation", NpcScript::luaGetPlayerVocation);
    lua_register(luaState, "setPlayerMasterPos", NpcScript::luaSetPlayerMasterPos);

#ifdef YUR_PREMIUM_PROMOTION
    lua_register(luaState, "isPremium", NpcScript::luaIsPremium);
    lua_register(luaState, "isPromoted", NpcScript::luaIsPromoted);
#endif //YUR_PREMIUM_PROMOTION

    lua_register(luaState, "setPlayerVocation", NpcScript::luaSetPlayerVocation);
    lua_register(luaState, "itemToStorage", NpcScript::luaItemToStorage);
#ifdef HUCZU_PAY_SYSTEM
    lua_register(luaState, "addPlayerPoints", NpcScript::luaAddPlayerPoints);
    lua_register(luaState, "getPlayerPoints", NpcScript::luaGetPlayerPoints);
    lua_register(luaState, "removePlayerPoints", NpcScript::luaRemovePlayerPoints);
#endif
    lua_register(luaState, "setPlayerOutfit", NpcScript::luaSetPlayerOutfit);
    return true;
}

Npc* NpcScript::getNpc(lua_State *L)
{
    lua_getglobal(L, "addressOfNpc");
    int32_t val = (int32_t)lua_tonumber(L, -1);
    lua_pop(L,1);
    Npc* mynpc = (Npc*)val;

    if(!mynpc)
    {
        return 0;
    }
    return mynpc;
}

int32_t NpcScript::luaCreatureGetName2(lua_State *L)
{
    const char* s = lua_tostring(L, -1);
    lua_pop(L,1);
    Npc* mynpc = getNpc(L);
    Creature *c = mynpc->game->getCreatureByName(std::string(s));

    if(c && c->access < g_config.ACCESS_PROTECT)
    {
        lua_pushnumber(L, c->getID());
    }
    else
        lua_pushnumber(L, 0);

    return 1;
}

int32_t NpcScript::luaCreatureGetName(lua_State *L)
{
    int32_t id = (int32_t)lua_tonumber(L, -1);
    lua_pop(L,1);
    Npc* mynpc = getNpc(L);
    lua_pushstring(L, mynpc->game->getCreatureByID(id)->getName().c_str());
    return 1;
}

int32_t NpcScript::luaCreatureGetPos(lua_State *L)
{
    int32_t id = (int32_t)lua_tonumber(L, -1);
    lua_pop(L,1);
    Npc* mynpc = getNpc(L);
    Creature* c = mynpc->game->getCreatureByID(id);

    if(!c)
    {
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushnil(L);
    }
    else
    {
        lua_pushnumber(L, c->pos.x);
        lua_pushnumber(L, c->pos.y);
        lua_pushnumber(L, c->pos.z);
    }
    return 3;
}

int32_t NpcScript::luaSelfGetPos(lua_State *L)
{
    lua_pop(L,1);
    Npc* mynpc = getNpc(L);
    lua_pushnumber(L, mynpc->pos.x);
    lua_pushnumber(L, mynpc->pos.y);
    lua_pushnumber(L, mynpc->pos.z);
    return 3;
}

int32_t NpcScript::luaActionSay(lua_State* L)
{
    int32_t len = (uint32_t)lua_strlen(L, -1);
    std::string msg(lua_tostring(L, -1), len);
    lua_pop(L,1);
    //now, we got the message, we now have to find out
    //what npc this belongs to

    Npc* mynpc=getNpc(L);
    if(mynpc)
        mynpc->doSay(msg);
    return 0;
}

int32_t NpcScript::luaActionMove(lua_State* L)
{
    int32_t dir=(int32_t)lua_tonumber(L, -1);
    lua_pop(L,1);
    Npc* mynpc=getNpc(L);
    if(mynpc)
        mynpc->doMove(dir);
    return 0;
}

int32_t NpcScript::luaActionMoveTo(lua_State* L)
{
    Position target;
    target.z=(int32_t)lua_tonumber(L, -1);
    lua_pop(L,1);
    target.y=(int32_t)lua_tonumber(L, -1);
    lua_pop(L,1);
    target.x=(int32_t)lua_tonumber(L, -1);
    lua_pop(L,1);
    Npc* mynpc=getNpc(L);
    if(mynpc)
        mynpc->doMoveTo(target);
    return 0;
}



int32_t NpcScript::luaActionAttackCreature(lua_State *L)
{
    int32_t id=(int32_t)lua_tonumber(L, -1);
    lua_pop(L,1);
    Npc* mynpc=getNpc(L);
    if(mynpc)
        mynpc->doAttack(id);
    return 0;
}

int32_t NpcScript::luaBuyItem(lua_State *L)
{
    int32_t cost = (int32_t)lua_tonumber(L, -1);
    int32_t count = (int32_t)lua_tonumber(L, -2);
    int32_t itemid = (int32_t)lua_tonumber(L, -3);
    int32_t cid = (int32_t)lua_tonumber(L, -4);
    lua_pop(L,4);

    Npc* mynpc = getNpc(L);
    Creature* creature = mynpc->game->getCreatureByID(cid);
    Player* player = creature? dynamic_cast<Player*>(creature) : NULL;

    if (player)
    {
        if (player->getCoins(cost))
        {
            if (player->removeCoins(cost)) // double check
            {
                player->TLMaddItem(itemid, count);
                mynpc->doSay("Prosze.");
            }
            else
                mynpc->doSay("Sorry, nie masz wystarczajacej ilosci pieniedzy.");
        }
        else
            mynpc->doSay("Sorry, nie masz wystarczajacej ilosci pieniedzy.");
    }

    return 0;
}

int32_t NpcScript::luaBuyActionId(lua_State *L)
{
    int32_t cost = (int32_t)lua_tonumber(L, -1);
    int32_t actionid = (int32_t)lua_tonumber(L, -2);
    int32_t count = (int32_t)lua_tonumber(L, -3);
    int32_t itemid = (int32_t)lua_tonumber(L, -4);
    int32_t cid = (int32_t)lua_tonumber(L, -5);
    lua_pop(L,5);

    Npc* mynpc = getNpc(L);
    Creature* creature = mynpc->game->getCreatureByID(cid);
    Player* player = creature? dynamic_cast<Player*>(creature) : NULL;
    if (player)
    {
        if (player->getCoins(cost))
        {
            if (player->removeCoins(cost)) // double check
            {
                player->addItemek(itemid, count, actionid);
                mynpc->doSay("Prosze.");
            }
            else
                mynpc->doSay("Sorry, nie masz wystarczajacej ilosci pieniedzy.");
        }
        else
            mynpc->doSay("Sorry, nie masz wystarczajacej ilosci pieniedzy.");
    }

    return 0;
}

int32_t NpcScript::luaTradeItem(lua_State *L)
{
    int32_t count2 = (int32_t)lua_tonumber(L, -1);
    int32_t itemid2 = (int32_t)lua_tonumber(L, -2);
    int32_t count = (int32_t)lua_tonumber(L, -3);
    int32_t itemid = (int32_t)lua_tonumber(L, -4);
    int32_t cid = (int32_t)lua_tonumber(L, -5);
    lua_pop(L,5);

    Npc* mynpc = getNpc(L);
    Creature* creature = mynpc->game->getCreatureByID(cid);
    Player* player = creature? dynamic_cast<Player*>(creature) : NULL;

    if (player && player->getItem(itemid, count))
    {
        player->removeItem(itemid, count);
        player->TLMaddItem(itemid2, count2);
        return 1;
    }
    else
    {
        return 0;
    }

}

int32_t NpcScript::luaSellItem(lua_State *L)
{
    int32_t cost = (int32_t)lua_tonumber(L, -1);
    int32_t count = (int32_t)lua_tonumber(L, -2);
    int32_t itemid = (int32_t)lua_tonumber(L, -3);
    int32_t cid = (int32_t)lua_tonumber(L, -4);
    lua_pop(L,4);

    Npc* mynpc = getNpc(L);
    Creature* creature = mynpc->game->getCreatureByID(cid);
    Player* player = creature? dynamic_cast<Player*>(creature) : NULL;

    if (player)
    {
        if (player->getItem(itemid, count))
        {
            if (player->removeItem(itemid, count)) // double check
            {
                player->payBack(cost);
                mynpc->doSay("Dzieki!");
            }
            else
                mynpc->doSay("Przykro mi, nie masz tego przedmiotu.");
        }
        else
            mynpc->doSay("Przykro mi, nie masz tego przedmiotu.");
    }

    return 0;
}

int32_t NpcScript::luaPayMoney(lua_State *L)
{
    int32_t cost = (int32_t)lua_tonumber(L, -1);
    int32_t cid = (int32_t)lua_tonumber(L, -2);
    lua_pop(L,2);

    Npc* mynpc = getNpc(L);
    Creature* creature = mynpc->game->getCreatureByID(cid);
    Player* player = creature? dynamic_cast<Player*>(creature) : NULL;

    if (player)
    {
        if (player->getCoins(cost))
        {
            if (player->removeCoins(cost)) // double check
                lua_pushboolean(L, true);
            else
                lua_pushboolean(L, false);
        }
        else
            lua_pushboolean(L, false);
    }
    else
        lua_pushboolean(L, false);

    return 1;
}

int32_t NpcScript::luaGetPlayerStorageValue(lua_State* L)
{
    int32_t id = (int32_t)lua_tonumber(L, -2);
    uint32_t key = (uint32_t)lua_tonumber(L, -1);
    lua_pop(L, 2);

    Npc* mynpc = getNpc(L);
    Creature* creature = mynpc->game->getCreatureByID(id);
    Player* player = creature? dynamic_cast<Player*>(creature) : NULL;

    int32_t value;
    if (player && player->getStorageValue(key, value))
        lua_pushnumber(L, value);
    else
        lua_pushnumber(L, -1);

    return 1;
}

int32_t NpcScript::luaSetPlayerStorageValue(lua_State* L)
{
    int32_t id = (int32_t)lua_tonumber(L, -3);
    uint32_t key = (uint32_t)lua_tonumber(L, -2);
    int32_t value = (int32_t)lua_tonumber(L, -1);
    lua_pop(L, 3);

    Npc* mynpc = getNpc(L);
    Creature* creature = mynpc->game->getCreatureByID(id);
    Player* player = creature? dynamic_cast<Player*>(creature) : NULL;

    if (player)
        player->addStorageValue(key, value);

    return 0;
}

int32_t NpcScript::luaPlayerRemoveItem(lua_State* L)
{
    int32_t id = (int32_t)lua_tonumber(L, -2);
    uint32_t item_id = (uint32_t)lua_tonumber(L, -1);
    lua_pop(L, 2);

    Npc* mynpc = getNpc(L);
    Creature* creature = mynpc->game->getCreatureByID(id);
    Player* player = creature? dynamic_cast<Player*>(creature) : NULL;

    if (player && player->removeItem(item_id, 1))
        lua_pushnumber(L, 0);
    else
        lua_pushnumber(L, -1);

    return 1;
}

int32_t NpcScript::luaGetPlayerLevel(lua_State* L)
{
    const char* name = lua_tostring(L, -1);
    lua_pop(L, 1);

    Npc* mynpc = getNpc(L);
    Creature* creature = mynpc->game->getCreatureByName(name);
    Player* player = creature? dynamic_cast<Player*>(creature) : NULL;

    if (player)
        lua_pushnumber(L, player->getLevel());
    else
        lua_pushnumber(L, -1);

    return 1;
}

int32_t NpcScript::luaSetPlayerMasterPos(lua_State* L)
{
    int32_t id = (int32_t)lua_tonumber(L, -4);
    int32_t x = (int32_t)lua_tonumber(L, -3);
    int32_t y = (int32_t)lua_tonumber(L, -2);
    int32_t z = (int32_t)lua_tonumber(L, -1);
    lua_pop(L, 4);

    Npc* mynpc = getNpc(L);
    Creature* creature = mynpc->game->getCreatureByID(id);
    Player* player = creature? dynamic_cast<Player*>(creature) : NULL;

    if (player)
    {
        Position pos(x,y,z);
        Tile* tile = mynpc->game->getTile(pos);

        if (tile)
            player->masterPos = pos;
        else
            std::cout << "NpcScript: luaSetPlayerMasterPos: given position is invalid!" << std::endl;
    }

    return 0;
}

int32_t NpcScript::luaSetPlayerVocation(lua_State* L)
{
    int32_t id = (int32_t)lua_tonumber(L, -2);
    int32_t voc = (int32_t)lua_tonumber(L, -1);
    lua_pop(L, 2);

    Npc* mynpc = getNpc(L);
    Creature* creature = mynpc->game->getCreatureByID(id);
    Player* player = creature? dynamic_cast<Player*>(creature) : NULL;

    if (player)
        player->setVocation((playervoc_t)voc);

    return 0;
}

int32_t NpcScript::luaGetPlayerVocation(lua_State* L)
{
    int32_t id = (int32_t)lua_tonumber(L, -1);
    lua_pop(L, 1);

    Npc* mynpc = getNpc(L);
    Creature* creature = mynpc->game->getCreatureByID(id);
    Player* player = creature? dynamic_cast<Player*>(creature) : NULL;

    if (player)
        lua_pushnumber(L, player->getVocation());
    else
        lua_pushnumber(L, -1);

    return 1;
}

#ifdef YUR_PREMIUM_PROMOTION
int32_t NpcScript::luaIsPromoted(lua_State* L)
{
    int32_t id = (int32_t)lua_tonumber(L, -1);
    lua_pop(L, 1);

    Npc* mynpc = getNpc(L);
    Creature* creature = mynpc->game->getCreatureByID(id);
    Player* player = creature? dynamic_cast<Player*>(creature) : NULL;

    if (player)
        lua_pushboolean(L, player->isPromoted());
    else
        lua_pushboolean(L, false);

    return 1;
}

int32_t NpcScript::luaIsPremium(lua_State* L)
{
    int32_t id = (int32_t)lua_tonumber(L, -1);
    lua_pop(L, 1);

    Npc* mynpc = getNpc(L);
    Creature* creature = mynpc->game->getCreatureByID(id);
    Player* player = dynamic_cast<Player*>(creature);

    if (player)
        lua_pushboolean(L, player->isPremium());
    else
        lua_pushboolean(L, false);

    return 1;
}
#endif //YUR_PREMIUM_PROMOTION

int32_t NpcScript::luaBuyCont(lua_State *L)
{
    int32_t actionid = (int32_t)lua_tonumber(L, -1);
    int32_t bpid = (int32_t)lua_tonumber(L, -2);
    int32_t cost = (int32_t)lua_tonumber(L, -3);
    int32_t count = (int32_t)lua_tonumber(L, -4);
    int32_t itemid = (int32_t)lua_tonumber(L, -5);
    int32_t cid = (int32_t)lua_tonumber(L, -6);
    lua_pop(L,6);
    Npc* mynpc = getNpc(L);
    Creature* creatore = mynpc->game->getCreatureByID(cid);
    Player *player = dynamic_cast<Player*>(creatore);
    if (player)
    {
        if (player->getCoins(cost))
        {
            if (player->removeCoins(cost))  // double check
            {

//Start of Container Stuff
                Container *backpack = dynamic_cast<Container*>(Item::CreateItem(bpid));
                if(backpack)
                {
                    for(int32_t i = 0; i < backpack->capacity(); ++i)
                    {
                        Item* additem = Item::CreateItem(itemid, count);
                        backpack->addItem(additem);
                        if(actionid > 0)
                            additem->setActionId(actionid);
                    }
                    player->addItem(backpack);
                }
//End
                mynpc->doSay("Trzymaj!");
            }
            else
            {
                mynpc->doSay("Sorry, nie posiadasz wystarczajacej ilosci pieniedzy.");
            }
        }
        else
        {
            mynpc->doSay("Sorry, nie posiadasz wystarczajacej ilosci pieniedzy.");
        }
    }
    return 1;
}

int32_t NpcScript::luaActionLook(lua_State* L)
{
    int32_t id = (int32_t)lua_tonumber(L, -1);
    lua_pop(L,1);
    Npc* mynpc=getNpc(L);
    Creature* c = mynpc->game->getCreatureByID(id);
    if(mynpc)
        mynpc->doLook(c);
    return 0;
}

int32_t NpcScript::luaItemToStorage(lua_State* L)
{
    //itemToStorage(cid, storage, value, itemid, count)
    int32_t count = (int32_t)lua_tonumber(L, -1);
    int32_t itemid = (int32_t)lua_tonumber(L, -2);
    int32_t value = (int32_t)lua_tonumber(L, -3);
    int32_t storage = (int32_t)lua_tonumber(L, -4);
    int32_t cid = (int32_t)lua_tonumber(L, -5);
    lua_pop(L,5);

    Npc* mynpc=getNpc(L);
    Creature* creature = mynpc->game->getCreatureByID(cid);
    Player *player = dynamic_cast<Player*>(creature);
    if (player)
    {
        if(player->removeItem(itemid,count))
        {
            player->addStorageValue(storage,value);
            lua_pushboolean(L, true);
        }
        else
            lua_pushboolean(L, false);
    }
    else
        lua_pushboolean(L, false);

    return 1;
}
#ifdef HUCZU_PAY_SYSTEM
int32_t NpcScript::luaAddPlayerPoints(lua_State* L)
{
    uint32_t count = (uint32_t)lua_tonumber(L, -1);
    int32_t cid = (int32_t)lua_tonumber(L, -2);
    lua_pop(L,2);

    Npc* mynpc=getNpc(L);
    Creature* creature = mynpc->game->getCreatureByID(cid);
    Player *player = dynamic_cast<Player*>(creature);
    if (player)
    {
        player->addPunkty(count);
        lua_pushboolean(L, true);
    }
    else
        lua_pushboolean(L, false);

    return 1;
}

int32_t NpcScript::luaGetPlayerPoints(lua_State* L)
{
    int32_t cid = (int32_t)lua_tonumber(L, -1);
    lua_pop(L,1);

    Npc* mynpc=getNpc(L);
    Creature* creature = mynpc->game->getCreatureByID(cid);
    Player *player = dynamic_cast<Player*>(creature);
    if (player)
        lua_pushnumber(L,player->getPunkty());
    else
        lua_pushboolean(L, false);

    return 1;
}

int32_t NpcScript::luaRemovePlayerPoints(lua_State* L)
{
    uint32_t count = (uint32_t)lua_tonumber(L, -1);
    int32_t cid = (int32_t)lua_tonumber(L, -2);
    lua_pop(L,2);

    Npc* mynpc=getNpc(L);
    Creature* creature = mynpc->game->getCreatureByID(cid);
    Player *player = dynamic_cast<Player*>(creature);
    if (player && player->removePunkty(count))
        lua_pushboolean(L, true);
    else
        lua_pushboolean(L, false);

    return 1;
}
#endif

int32_t NpcScript::luaSetPlayerOutfit(lua_State* L)
{
    uint32_t type = (uint32_t)lua_tonumber(L, -1);
    int32_t cid = (int32_t)lua_tonumber(L, -2);
    lua_pop(L,2);

    Npc* mynpc=getNpc(L);
    Creature* creature = mynpc->game->getCreatureByID(cid);
    Player *player = dynamic_cast<Player*>(creature);
    if (player)
    {
        mynpc->game->changeOutfit(cid, type);
        lua_pushboolean(L, true);
    }
    else
        lua_pushboolean(L, false);

    return 1;
}
