//#include "preheaders.h"
#include <sstream>

#include "definitions.h"
#include "const76.h"
#include "player.h"
#include "monster.h"
#include "npc.h"
#include "game.h"
#include "item.h"
#include "town.h"
#include "tools.h"

#include "actions.h"

Actions::Actions(Game* igame)
    :game(igame)
{
    //
}

Actions::~Actions()
{
    clear();
}

void Actions::clear()
{
    ActionUseMap::iterator it = useItemMap.begin();
    while(it != useItemMap.end())
    {
        delete it->second;
        useItemMap.erase(it);
        it = useItemMap.begin();
    }
    it = uniqueItemMap.begin();
    while(it != uniqueItemMap.end())
    {
        delete it->second;
        uniqueItemMap.erase(it);
        it = uniqueItemMap.begin();
    }
    it = actionItemMap.begin();
    while(it != actionItemMap.end())
    {
        delete it->second;
        actionItemMap.erase(it);
        it = actionItemMap.begin();
    }
#ifdef __MIZIAK_TALKACTIONS__
    ActionTalkMap::iterator tk = actionTalkMap.begin();
    while(tk != actionTalkMap.end())
    {
        delete tk->second;
        actionTalkMap.erase(tk);
        tk = actionTalkMap.begin();
    }
#endif //__MIZIAK_TALKACTIONS__
#ifdef __MIZIAK_CREATURESCRIPTS__
    ActionCreatureMap::iterator cs = creatureScriptMap.begin();
    while(cs != creatureScriptMap.end())
    {
        delete cs->second;
        creatureScriptMap.erase(cs);
        cs = creatureScriptMap.begin();
    }
#endif //__MIZIAK_CREATURESCRIPTS__
}

bool Actions::reload()
{
    this->loaded = false;
    //unload
    clear();
    //load
    return loadFromXml(datadir);
}

bool Actions::loadFromXml(const std::string &_datadir)
{
    this->loaded = false;
    Action *action = NULL;

    datadir = _datadir;

    std::string filename = datadir + "actions/actions.xml";
    //std::transform(filename.begin(), filename.end(), filename.begin(), tolower);
    toLowerCaseString(filename);
    xmlDocPtr doc = xmlParseFile(filename.c_str());

    if (doc)
    {
        this->loaded=true;
        xmlNodePtr root, p;
        root = xmlDocGetRootElement(doc);

        if (xmlStrcmp(root->name,(const xmlChar*) "actions"))
        {
            xmlFreeDoc(doc);
            return false;
        }
        p = root->children;

        while (p)
        {
            const char* str = (char*)p->name;

            if (strcmp(str, "action") == 0)
            {
                int32_t itemid,uniqueid,actionid;
                if(readXMLInteger(p,"itemid",itemid))
                {
                    action = loadAction(p);
                    useItemMap[itemid] = action;
                    action = NULL;
                }
                else if(readXMLInteger(p,"uniqueid",uniqueid))
                {
                    action = loadAction(p);
                    uniqueItemMap[uniqueid] = action;
                    action = NULL;
                }
                else if(readXMLInteger(p,"actionid",actionid))
                {
                    action = loadAction(p);
                    actionItemMap[actionid] = action;
                    action = NULL;
                }
#ifdef __MIZIAK_TALKACTIONS__
                else if(strcmp(str, "talk") == 0)
                {
                    std::string word;
                    if(readXMLString(p,"word",word))
                    {
                        action = loadAction(p);
                        actionTalkMap[word] = action;
                        action = NULL;
                    }
                    else
                        std::cout << "missing word declatation." << std::endl;
                }
#endif //__MIZIAK_TALKACTIONS__
#ifdef __MIZIAK_CREATURESCRIPTS__
                else if(strcmp(str, "creature") == 0)
                {
                    std::string evt;
                    if(readXMLString(p,"event",evt))
                    {
                        action = loadAction(p);
                        std::string id = (char*)xmlGetProp(p,(xmlChar*)"id");
                        evt += id;
                        creatureScriptMap[evt] = action;
                        action = NULL;
                    }
                    else
                        std::cout << "missing event declatation." << std::endl;
                }
#endif //__MIZIAK_CREATURESCRIPTS__
                else
                {
                    std::cout << "missing action id." << std::endl;
                }
            }
            p = p->next;
        }

        xmlFreeDoc(doc);
    }
    return this->loaded;
}

Action *Actions::loadAction(xmlNodePtr xmlaction)
{
    Action *action = NULL;
    char* scriptfile = (char*)xmlGetProp(xmlaction,(xmlChar*)"script");
    if(scriptfile)
    {
        action = new Action(game,datadir, datadir + std::string("actions/scripts/") + scriptfile);
        if(action->isLoaded())
        {
            char* sallow = (char*)xmlGetProp(xmlaction,(xmlChar*)"allowfaruse");
            if(sallow)
            {
                if(strcmp(sallow,"1") == 0)
                {
                    action->setAllowFarUse(true);
                }
                xmlFreeOTSERV(sallow);
            }
            sallow = (char*)xmlGetProp(xmlaction,(xmlChar*)"blockwalls");
            if(sallow)
            {
                if(strcmp(sallow,"0") == 0)
                {
                    action->setBlockWalls(false);
                }
                xmlFreeOTSERV(sallow);
            }
#ifdef __MIZIAK_TALKACTIONS__
            sallow = (char*)xmlGetProp(xmlaction,(xmlChar*)"access");
            if(sallow)
            {
                action->setAccess(atoi(sallow));
                xmlFreeOTSERV(sallow);
            }
#endif //__MIZIAK_TALKACTIONS__
        }
        else
        {
            delete action;
            action = NULL;
        }
        xmlFreeOTSERV(scriptfile);
    }
    else
    {
        std::cout << "Missing script tag."  << std::endl;
    }
    return action;
}

int32_t Actions::canUse(const Player *player,const Position &pos) const
{
    if(pos.x != 0xFFFF)
    {
        int32_t dist_x = std::abs(pos.x - player->pos.x);
        int32_t dist_y = std::abs(pos.y - player->pos.y);
        if(dist_x > 1 || dist_y > 1 || (pos.z != player->pos.z))
        {
            return TOO_FAR;
        }
    }
    return CAN_USE;
}

int32_t Actions::canUseFar(const Player *player,const Position &to_pos, const bool blockWalls) const
{
    if(to_pos.x == 0xFFFF)
    {
        return CAN_USE;
    }
    if(std::abs(player->pos.x - to_pos.x) > 7 || std::abs(player->pos.y - to_pos.y) > 5 || player->pos.z != to_pos.z)
    {
        return TOO_FAR;
    }

    if(canUse(player,to_pos) == TOO_FAR)
    {
        if(blockWalls && (game->map->canThrowObjectTo(player->pos, to_pos, BLOCK_PROJECTILE) != RET_NOERROR))
        {
            return CAN_NOT_THTOW;
        }
    }

    return CAN_USE;
}

Action *Actions::getAction(const Item *item)
{
    if(item->getUniqueId() != 0)
    {
        ActionUseMap::iterator it = uniqueItemMap.find(item->getUniqueId());
        if (it != uniqueItemMap.end())
        {
            return it->second;
        }
    }
    if(item->getActionId() != 0)
    {
        ActionUseMap::iterator it = actionItemMap.find(item->getActionId());
        if (it != actionItemMap.end())
        {
            return it->second;
        }
    }
    ActionUseMap::iterator it = useItemMap.find(item->getID());
    if (it != useItemMap.end())
    {
        return it->second;
    }

    return NULL;
}

#ifdef __MIZIAK_TALKACTIONS__
Action *Actions::getActionTalk(std::string word)
{
    if(word != "")
    {
        ActionTalkMap::iterator it = actionTalkMap.find(word);
        if (it != actionTalkMap.end())
        {
            return it->second;
        }
    }
    return NULL;
}
#endif //__MIZIAK_TALKACTIONS__

bool Actions::luaWalk(Player* player, const Position &pos, //CHANGE onWalk
                      const uint16_t itemid, const uint32_t itemuid, const uint32_t itemaid)//, const unsigned char index)
{
    Item *item = dynamic_cast<Item*>(game->getThing(pos,0,player));
    Action *action = getAction(item);
    if(action)
        if(action->executeWalk(player,itemid,itemuid,itemaid,pos))
            return true;

    return false;
}

bool Actions::luaWalkOff(Player* player, const Position &pos, //CHANGE onWalk
                         const uint16_t itemid, const uint32_t itemuid, const uint32_t itemaid)//, const unsigned char index)
{
    Item *item = dynamic_cast<Item*>(game->getThing(pos,0,player));
    Action *action = getAction(item);
    if(action)
        if(action->executeWalkOff(player,itemid,itemuid,itemaid,pos))
            return true;

    return false;
}

bool Actions::UseItem(Player* player, const Position &pos,const unsigned char stack,
                      const uint16_t itemid, const unsigned char index)
{
    if(canUse(player,pos)== TOO_FAR)
    {
        player->sendCancel("Za daleko.");
        return false;
    }
    Item *item = dynamic_cast<Item*>(game->getThing(pos,stack,player));
    if(!item)
    {
#ifdef __DEBUG__
        std::cout << "no item" << std::endl;
#endif
        player->sendCancel("Nie mozesz uzyc tego obiektu.");
        return false;
    }

    if((item->getID() != itemid) || (item->getID() >= 408) && (item->getID() <= 411) || (item->getID() == 423) || (item->getID() >= 427) && (item->getID() <= 429) || (item->getID() >= 432) && (item->getID() <= 433) || (item->getID() >= 479) && (item->getID() <= 480) || (item->getID() >= 3135) && (item->getID() <= 3138) || (item->getID() >= 3219) && (item->getID() <= 3220) || (item->getID() >= 4834) && (item->getID() <= 4837))
    {
#ifdef __DEBUG__
        std::cout << "no id" << std::endl;
#endif
        player->sendCancel("Nie mozesz uzyc tego obiektu.");
        return false;
    }

#ifdef TLM_HOUSE_SYSTEM
    if (Item::items[itemid].isDoor)
    {
        Tile* tile = game->getTile(pos);
        House* house = tile? tile->getHouse() : NULL;

        if (house && player->access < g_config.ACCESS_HOUSE && house->getPlayerRights(pos, player->getName()) <= HOUSE_GUEST)
        {
            player->sendCancel("Nie jestes upowazniony do otwierania tych drzwi.");
            return false;
        }
    }
#endif //TLM_HOUSE_SYSTEM

    if(item->getOwner() != "" && player->getName() != item->getOwner() && (player->party == 0 || game->getPlayerByName(item->getOwner())->party == 0 || player->party != game->getPlayerByName(item->getOwner())->party))
    {
        std::stringstream info;
        if(item->getOwnerTime() > g_config.OWNER_TIME)
        {
            info << "Nie jestes wlascicielem. Poczekaj 1 sekunde.";
        }
        else
        {
            info << "Nie jestes wlascicielem. Poczekaj " << item->getOwnerTime() << (item->getOwnerTime()>4?" sekund.":(item->getOwnerTime()>1?" sekundy.":(item->getOwnerTime()>0?" sekunde.":" sekund.")));
        }
        player->sendCancel(info.str().c_str());
        return false;
    }
    //look for the item in action maps
    Action *action = getAction(item);

    //if found execute it
    if(action)
    {
        Position itempos = game->getThingMapPos(player, pos);
        game->autoCloseTrade(item);
        PositionEx posEx(pos,stack);
        if(action->executeUse(player,item,posEx,posEx))
        {
            return true;
        }
    }

    //if it is a container try to open it
    if(dynamic_cast<Container*>(item))
    {
        if(openContainer(player,dynamic_cast<Container*>(item),index))
            return true;
    }

    //we dont know what to do with this item
    player->sendCancel("Nie mozesz uzyc tego obiektu.");
    return false;

    if (item->getID() == ITEM_COINS_GOLD || item->getID() == ITEM_COINS_PLATINUM || item->getID() == ITEM_COINS_CRYSTAL || item->getID() == ITEM_COINS_SCARAB)
    {
        return true;
    }
}
bool Actions::openContainer(Player *player,Container *container, const unsigned char index)
{
    if(container->depot == 0)  //normal container
    {
        unsigned char oldcontainerid = player->getContainerID(container);
        if(oldcontainerid != 0xFF)
        {
            player->closeContainer(oldcontainerid);
            player->sendCloseContainer(oldcontainerid);
        }
        else
        {
            player->sendContainer(index, container);
        }
    }
    else // depot container
    {
        Container *container2 = player->getDepot(container->depot);
        if(container2)
        {
            //update depot coordinates
            container2->pos = container->pos;
            player->sendContainer(index, container2);
        }
        else
        {
            return false;
        }
    }
    return true;
}

bool Actions::UseItemEx(Player* player, const Position &from_pos,
                        const unsigned char from_stack,const Position &to_pos,
                        const unsigned char to_stack,const uint16_t itemid)
{
    if(canUse(player,from_pos) == TOO_FAR)
    {
        player->sendCancel("Za daleko.");
        return false;
    }

    Item *item = dynamic_cast<Item*>(game->getThing(from_pos,from_stack,player));
    if(!item)
        return false;

    if(item->getID() != itemid)
        return false;

    if(!item->isUseable())
        return false;

    Action *action = getAction(item);

    if(action)
    {
        if(action->allowFarUse() == false)
        {
            if(canUse(player,to_pos) == TOO_FAR)
            {
                player->sendCancel("Za daleko.");
                return false;
            }
        }
        else if(canUseFar(player, to_pos, action->blockWalls()) == TOO_FAR)
        {
            player->sendCancel("Za daleko.");
            return false;
        }
        else if(canUseFar(player, to_pos, action->blockWalls()) == CAN_NOT_THTOW)
        {
            player->sendCancel("Nie mozesz tam rzucic.");
            return false;
        }

        Position itempos = game->getThingMapPos(player, from_pos);
        game->autoCloseTrade(item);
        PositionEx posFromEx(from_pos,from_stack);
        PositionEx posToEx(to_pos,to_stack);
        if(action->executeUse(player,item,posFromEx,posToEx))
            return true;
    }

    //not found
    player->sendCancel("Nie mozesz uzyc tego obiektu.");
    return false;
}

#ifdef __MIZIAK_TALKACTIONS__
bool Actions::SayTalk(Player* player, std::string text)
{
    std::string cmd, param;
    std::string::size_type pos = text.find(" ", 0);
    if(pos == std::string::npos)
    {
        cmd = text;
        param = "";
    }
    else
    {
        cmd = text.substr(0, pos);
        text.erase(0, pos+1);
        param = text;
    }

    Action *action = getActionTalk(cmd);

    if(action)
    {
        if(player->access < action->getAccess())
        {
            player->sendCancel("You can't execute this command.");
            return false;
        }

        if(action->executeTalk(player, cmd, param))
            if(action->getAccess() == 0)
                return false;
            else
                return true;
    }

    return false;
}
#endif //__MIZIAK_TALKACTIONS__

#ifdef __MIZIAK_CREATURESCRIPTS__
bool Actions::creatureEvent(std::string evt, Player* player, Creature* creature, Item* item, int64_t tab[])
{
    bool ret = false;
    for(ActionCreatureMap::iterator it = creatureScriptMap.begin(); it != creatureScriptMap.end(); ++it)
    {
        Action *action = it->second;
        if(action)
        {
            if(evt == "login" && it->first.find("login") != std::string::npos)
                if(action->executeLogin(player))
                    ret = true;
            if(evt == "logout" && it->first.find("logout") != std::string::npos)
                if(action->executeLogout(player))
                    ret = true;
            if(evt == "death" && it->first.find("death") != std::string::npos)
                if(action->executeDeath(player, creature, item))
                    ret = true;
            if(evt == "advance" && it->first.find("advance") != std::string::npos)
                if(action->executeAdvance(player, tab[0], tab[1], tab[2]))
                    ret = true;
            if(evt == "kill" && it->first.find("kill") != std::string::npos)
                if(action->executeKill(player, creature, item))
                    ret = true;
        }
    }

    return ret;
}
#endif //__MIZIAK_CREATURESCRIPTS__


Action::Action(Game* igame,const std::string &datadir, const std::string &scriptname)
{
    loaded = false;
    allowfaruse = false;
    blockwalls = true;
    script = new ActionScript(igame,datadir,scriptname);
    if(script->isLoaded())
        loaded = true;
}

Action::~Action()
{
    if(script)
    {
        delete script;
        script = NULL;
    }
}


bool Action::executeUse(Player *player,Item* item, PositionEx &posFrom, PositionEx &posTo)
{
    //onUse(uidplayer, item1,position1,item2,position2)
    script->ClearMap();
    script->_player = player;
    PositionEx playerpos = player->pos;
    uint32_t cid = script->AddThingToMap((Thing*)player,playerpos);
    uint32_t itemid1 = script->AddThingToMap(item,posFrom);
    lua_State*  luaState = script->getLuaState();

    lua_pushstring(luaState, "onUse");
    lua_gettable(luaState, LUA_GLOBALSINDEX);

    lua_pushnumber(luaState, cid);
    script->internalAddThing(luaState,item,itemid1);
    script->internalAddPositionEx(luaState,posFrom);
    //std::cout << "posTo" <<  (Position)posTo << " stack" << (int32_t)posTo.stackpos <<std::endl;
    Thing *thing = script->game->getThing((Position)posTo,posTo.stackpos,player);
    if(thing && posFrom != posTo)
    {
        int32_t thingId2 = script->AddThingToMap(thing,posTo);
        script->internalAddThing(luaState,thing,thingId2);
        script->internalAddPositionEx(luaState,posTo);
    }
    else
    {
        script->internalAddThing(luaState,NULL,0);
        PositionEx posEx;
        script->internalAddPositionEx(luaState,posEx);
    }

    lua_pcall(luaState, 5, 1, 0);

    bool ret = (script->internalGetNumber(luaState) != 0);

    return ret;
}

#ifdef __MIZIAK_TALKACTIONS__
bool Action::executeTalk(Player *player, std::string cmd, std::string param)
{
    //onTalk(uidplayer, word, param)
    script->ClearMap();
    script->_player = player;
    PositionEx playerpos = player->pos;
    uint32_t cid = script->AddThingToMap((Thing*)player,playerpos);
    lua_State*  luaState = script->getLuaState();

    lua_pushstring(luaState, "onTalk");
    lua_gettable(luaState, LUA_GLOBALSINDEX);
    lua_pushnumber(luaState, cid);
    lua_pushstring(luaState, cmd.c_str());
    lua_pushstring(luaState, param.c_str());

    lua_pcall(luaState, 3, 1, 0);

    bool ret = (script->internalGetNumber(luaState) != 0);

    return ret;
}
#endif //__MIZIAK_TALKACTIONS__

#ifdef __MIZIAK_CREATURESCRIPTS__
bool Action::executeLogin(Player *player)
{
    //onLogin(uidplayer)
    script->ClearMap();
    script->_player = player;
    PositionEx playerpos = player->pos;
    uint32_t cid = script->AddThingToMap((Thing*)player,playerpos);
    lua_State*  luaState = script->getLuaState();

    lua_pushstring(luaState, "onLogin");
    lua_gettable(luaState, LUA_GLOBALSINDEX);
    lua_pushnumber(luaState, cid);

    lua_pcall(luaState, 1, 1, 0);

    bool ret = (script->internalGetNumber(luaState) != 0);

    return ret;
}

bool Action::executeLogout(Player *player)
{
    //onLogout(uidplayer)
    script->ClearMap();
    script->_player = player;
    PositionEx playerpos = player->pos;
    uint32_t cid = script->AddThingToMap((Thing*)player,playerpos);
    lua_State*  luaState = script->getLuaState();

    lua_pushstring(luaState, "onLogout");
    lua_gettable(luaState, LUA_GLOBALSINDEX);
    lua_pushnumber(luaState, cid);

    lua_pcall(luaState, 1, 1, 0);

    bool ret = (script->internalGetNumber(luaState) != 0);

    return ret;
}

bool Action::executeDeath(Player *player, Creature* cre, Item* corpse)
{
    //onDeath(uidplayer, attackeruid, corpse)
    script->ClearMap();
    script->_player = player;
    PositionEx playerpos = player->pos;
    uint32_t cid = script->AddThingToMap((Thing*)player,playerpos);
    //script->_cre = cre;
    PositionEx crepos = cre->pos;
    uint32_t acid = script->AddThingToMap((Thing*)cre,crepos);
    PositionEx corpos = corpse->pos;
    uint32_t itemid = script->AddThingToMap(corpse,corpos);
    lua_State*  luaState = script->getLuaState();

    lua_pushstring(luaState, "onDeath");
    lua_gettable(luaState, LUA_GLOBALSINDEX);
    lua_pushnumber(luaState, cid);
    lua_pushnumber(luaState, acid);
    script->internalAddThing(luaState,corpse,itemid);

    lua_pcall(luaState, 3, 1, 0);

    bool ret = (script->internalGetNumber(luaState) != 0);

    return ret;
}

bool Action::executeAdvance(Player *player, int32_t skill, int32_t oldlvl, int32_t newlvl)
{
    //onAdvance(uidplayer, skill, oldlvl, newlvl)
    script->ClearMap();
    script->_player = player;
    PositionEx playerpos = player->pos;
    uint32_t cid = script->AddThingToMap((Thing*)player,playerpos);
    lua_State*  luaState = script->getLuaState();

    lua_pushstring(luaState, "onAdvance");
    lua_gettable(luaState, LUA_GLOBALSINDEX);
    lua_pushnumber(luaState, cid);
    lua_pushnumber(luaState, skill);
    lua_pushnumber(luaState, oldlvl);
    lua_pushnumber(luaState, newlvl);

    lua_pcall(luaState, 4, 1, 0);

    bool ret = (script->internalGetNumber(luaState) != 0);

    return ret;
}

bool Action::executeKill(Player *player, Creature* cre, Item* corpse)
{
    //onKill(killer, target, corpse)
    script->ClearMap();
    script->_player = player;
    PositionEx playerpos = player->pos;
    uint32_t cid = script->AddThingToMap((Thing*)player,playerpos);
    PositionEx crepos = cre->pos;
    uint32_t acid = script->AddThingToMap((Thing*)cre,crepos);
    PositionEx corpos = corpse->pos;
    uint32_t itemid = script->AddThingToMap(corpse,corpos);
    lua_State*  luaState = script->getLuaState();

    lua_pushstring(luaState, "onKill");
    lua_gettable(luaState, LUA_GLOBALSINDEX);
    lua_pushnumber(luaState, cid);
    lua_pushnumber(luaState, acid);
    script->internalAddThing(luaState,corpse,itemid);

    lua_pcall(luaState, 3, 1, 0);

    bool ret = (script->internalGetNumber(luaState) != 0);

    return ret;
}
#endif //__MIZIAK_CREATURESCRIPTS__

bool Action::executeWalk(Player *player,const uint16_t item,const uint32_t itemUID, const uint32_t itemaid, const Position &posTo)//CHANGE onWalk
{

    script->ClearMap();
    script->_player = player;
    PositionEx playerpos = player->pos;
    uint32_t cid = script->AddThingToMap((Thing*)player,playerpos);

    lua_State*  luaState = script->getLuaState();

    lua_pushstring(luaState, "onWalk");
    lua_gettable(luaState, LUA_GLOBALSINDEX);

    lua_pushnumber(luaState, cid);
    lua_pushnumber(luaState, item);
    lua_pushnumber(luaState, itemUID);
    lua_pushnumber(luaState, itemaid);
    lua_pushnumber(luaState, posTo.x);
    lua_pushnumber(luaState, posTo.y);
    lua_pushnumber(luaState, posTo.z);

    lua_pcall(luaState, 7, 1, 0);

    bool ret = (script->internalGetNumber(luaState) != 0);

    return ret;
}

bool Action::executeWalkOff(Player *player,const uint16_t item,const uint32_t itemUID, const uint32_t itemaid, const Position &posTo)//CHANGE onWalk
{

    script->ClearMap();
    script->_player = player;
    PositionEx playerpos = player->pos;
    uint32_t cid = script->AddThingToMap((Thing*)player,playerpos);

    lua_State*  luaState = script->getLuaState();

    lua_pushstring(luaState, "offWalk");
    lua_gettable(luaState, LUA_GLOBALSINDEX);

    lua_pushnumber(luaState, cid);
    lua_pushnumber(luaState, item);
    lua_pushnumber(luaState, itemUID);
    lua_pushnumber(luaState, itemaid);
    lua_pushnumber(luaState, posTo.x);
    lua_pushnumber(luaState, posTo.y);
    lua_pushnumber(luaState, posTo.z);

    lua_pcall(luaState, 7, 1, 0);

    bool ret = (script->internalGetNumber(luaState) != 0);

    return ret;
}

std::map<uint32_t,KnownThing*> ActionScript::uniqueIdMap;

ActionScript::ActionScript(Game *igame,const std::string &datadir,const std::string &scriptname):
    game(igame),
    _player(NULL)
{
    this->loaded = false;
    lastuid = 0;
    if(scriptname == "")
        return;
    luaState = lua_open();
    luaopen_loadlib(luaState);
    luaopen_base(luaState);
    luaopen_math(luaState);
    luaopen_string(luaState);
    luaopen_io(luaState);
    lua_dofile(luaState, std::string(datadir + "actions/lib/actions.lua").c_str());

#ifdef USING_VISUAL_2005
    FILE* in = NULL;
    fopen_s(&in, scriptname.c_str(), "r");
#else
    FILE* in=fopen(scriptname.c_str(), "r");
#endif //USING_VISUAL_2005
    if(!in)
    {
        std::cout << "Error: Can not open " << scriptname.c_str() << std::endl;
        return;
    }
    else
        fclose(in);
    lua_dofile(luaState, scriptname.c_str());
    this->setGlobalNumber("addressOfActionScript", (int32_t)this);
    this->loaded = true;
    this->registerFunctions();
}

ActionScript::~ActionScript()
{
    std::map<uint32_t,KnownThing*>::iterator it;
    for(it = ThingMap.begin(); it != ThingMap.end(); ++it)
    {
        delete it->second;
    }

    ThingMap.clear();
    /*
    for(it = uniqueIdMap.begin(); it != uniqueIdMap.end();it++ ){
    	delete it->second;
    }

    uniqueIdMap.clear();
    */
}

void ActionScript::ClearMap()
{
    std::map<uint32_t,KnownThing*>::iterator it;
    for(it = ThingMap.begin(); it != ThingMap.end(); ++it)
    {
        delete it->second;
        it->second = NULL;
    }
    ThingMap.clear();
    lastuid = 0;
}

void ActionScript::AddThingToMapUnique(Thing *thing)
{
    Item *item = dynamic_cast<Item*>(thing);
    if(item && item->getUniqueId() != 0 )
    {
        uint16_t uid = item->getUniqueId();
        KnownThing *tmp = uniqueIdMap[uid];
        if(!tmp)
        {
            KnownThing *tmp = new KnownThing;
            tmp->thing = thing;
            tmp->type = thingTypeItem;
            uniqueIdMap[uid] = tmp;
        }
        else
        {
            std::cout << tmp->pos << "Duplicate uniqueId " <<  uid << std::endl;
        }
    }
}

void ActionScript::UpdateThingPos(int32_t uid, PositionEx &pos)
{
    KnownThing *tmp = ThingMap[uid];
    if(tmp)
    {
        tmp->pos = pos;
    }
}

uint32_t ActionScript::AddThingToMap(Thing *thing,PositionEx &pos)
{
    Item *item = dynamic_cast<Item*>(thing);
    if(item && item->pos.x != 0xFFFF && item->getUniqueId())
    {
        uint16_t uid = item->getUniqueId();
        KnownThing *tmp = uniqueIdMap[uid];
        if(!tmp)
        {
            std::cout << "Item with unique id not included in the map!." << std::endl;
        }
        KnownThing *newKT = new KnownThing;
        newKT->thing = thing;
        newKT->type = thingTypeItem;
        newKT->pos = pos;
        ThingMap[uid] = newKT;
        return uid;
    }

    std::map<uint32_t,KnownThing*>::iterator it;
    for(it = ThingMap.begin(); it != ThingMap.end(); ++it)
    {
        if(it->second->thing == thing)
        {
            return it->first;
        }
    }

    KnownThing *tmp = new KnownThing;
    tmp->thing = thing;
    tmp->pos = pos;

    if(dynamic_cast<Item*>(thing))
        tmp->type = thingTypeItem;
    else if(dynamic_cast<Player*>(thing))
        tmp->type = thingTypePlayer;
    else if(dynamic_cast<Monster*>(thing))
        tmp->type = thingTypeMonster;
    else if(dynamic_cast<Npc*>(thing))
        tmp->type = thingTypeNpc;
    else
        tmp->type = thingTypeUnknown;

    lastuid++;
    while(ThingMap[lastuid])
    {
        lastuid++;
    }
    ThingMap[lastuid] = tmp;
    return lastuid;
}

const KnownThing* ActionScript::GetThingByUID(int32_t uid)
{
    KnownThing *tmp = ThingMap[uid];
    if(tmp)
        return tmp;
    tmp = uniqueIdMap[uid];
    if(tmp && tmp->thing->pos.x != 0xFFFF)
    {
        KnownThing *newKT = new KnownThing;
        newKT->thing = tmp->thing;
        newKT->type = tmp->type;
        newKT->pos = tmp->thing->pos;
        ThingMap[uid] = newKT;
        return newKT;
    }
    return NULL;
}

const KnownThing* ActionScript::GetItemByUID(int32_t uid)
{
    const KnownThing *tmp = GetThingByUID(uid);
    if(tmp)
    {
        if(tmp->type == thingTypeItem)
            return tmp;
    }
    return NULL;
}

const KnownThing* ActionScript::GetCreatureByUID(int32_t uid)
{
    const KnownThing *tmp = GetThingByUID(uid);
    if(tmp)
    {
        if(tmp->type == thingTypePlayer || tmp->type == thingTypeMonster
                || tmp->type == thingTypeNpc )
            return tmp;
    }
    return NULL;
}

const KnownThing* ActionScript::GetPlayerByUID(int32_t uid)
{
    const KnownThing *tmp = GetThingByUID(uid);
    if(tmp)
    {
        if(tmp->type == thingTypePlayer)
            return tmp;
    }
    return NULL;
}

const KnownThing* ActionScript::GetMonsterByUID(int32_t uid)
{
    const KnownThing *tmp = GetThingByUID(uid);
    if(tmp)
    {
        if(tmp->type == thingTypeMonster)
            return tmp;
    }
    return NULL;
}

int32_t ActionScript::registerFunctions()
{
    //getPlayerFood(uid)
    lua_register(luaState, "getPlayerFood", ActionScript::luaActionGetPlayerFood);
    //getPlayerHealth(uid)
    lua_register(luaState, "getPlayerHealth", ActionScript::luaActionGetPlayerHealth);
    //getPlayerMana(uid)
    lua_register(luaState, "getPlayerMana", ActionScript::luaActionGetPlayerMana);
    //getPlayerLevel(uid)
    lua_register(luaState, "getPlayerLevel", ActionScript::luaActionGetPlayerLevel);
    //getPlayerMagLevel(uid)
    lua_register(luaState, "getPlayerMagLevel", ActionScript::luaActionGetPlayerMagLevel);
    //getPlayerName(uid)
    lua_register(luaState, "getPlayerName", ActionScript::luaActionGetPlayerName);
    //getPlayerAccess(uid)
    lua_register(luaState, "getPlayerAccess", ActionScript::luaActionGetPlayerAccess);
    //getPlayerPosition(uid)
    lua_register(luaState, "getPlayerPosition", ActionScript::luaActionGetPlayerPosition);
    //getPlayerSkill(uid,skillid)
    lua_register(luaState, "getPlayerSkill", ActionScript::luaActionGetPlayerSkill);
    //getPlayerMasterPos(cid)
    lua_register(luaState, "getPlayerMasterPos", ActionScript::luaActionGetPlayerMasterPos);
    //getPlayerVocation(cid)
    lua_register(luaState, "getPlayerVocation", ActionScript::luaActionGetPlayerVocation);
    /*//getPlayerGuildId(cid)
    lua_register(luaState, "getPlayerGuildId", ActionScript::luaActionGetPlayerGuildId);*/
    //getPlayerItemCount(uid,itemid)
    //getPlayerItem(uid,itemid)


    //getPlayerStorageValue(uid,valueid)
    lua_register(luaState, "getPlayerStorageValue", ActionScript::luaActionGetPlayerStorageValue);
    //setPlayerStorageValue(uid,valueid, newvalue)
    lua_register(luaState, "setPlayerStorageValue", ActionScript::luaActionSetPlayerStorageValue);

    //getTilePzInfo(pos) 1 is pz. 0 no pz.
    lua_register(luaState, "getTilePzInfo", ActionScript::luaActionGetTilePzInfo);

    //getItemRWInfo(uid)
    lua_register(luaState, "getItemRWInfo", ActionScript::luaActionGetItemRWInfo);
    //getThingfromPos(pos)
    lua_register(luaState, "getThingfromPos", ActionScript::luaActionGetThingfromPos);
    //getThingPos(uid)

    //doRemoveItem(uid,n)
    lua_register(luaState, "doRemoveItem", ActionScript::luaActionDoRemoveItem);
    //doPlayerFeed(uid,food)
    lua_register(luaState, "doPlayerFeed", ActionScript::luaActionDoFeedPlayer);
    //doPlayerSendCancel(uid,text)
    lua_register(luaState, "doPlayerSendCancel", ActionScript::luaActionDoSendCancel);
    //doTeleportThing(uid,newpos)
    lua_register(luaState, "doTeleportThing", ActionScript::luaActionDoTeleportThing);
    //doTransformItem(uid,toitemid)
    lua_register(luaState, "doTransformItem", ActionScript::luaActionDoTransformItem);
    //doPlayerSay(uid,text,type)
    lua_register(luaState, "doPlayerSay", ActionScript::luaActionDoPlayerSay);
    //doSendMagicEffect(uid,position,type)
    lua_register(luaState, "doSendMagicEffect", ActionScript::luaActionDoSendMagicEffect);
    //doChangeTypeItem(uid,new_type)
    lua_register(luaState, "doChangeTypeItem", ActionScript::luaActionDoChangeTypeItem);
    //doSetItemActionId(uid,actionid)
    lua_register(luaState, "doSetItemActionId", ActionScript::luaActionDoSetItemActionId);
    //doSetItemText(uid,text)
    lua_register(luaState, "doSetItemText", ActionScript::luaActionDoSetItemText);
    //doSetItemSpecialDescription(uid,desc)
    lua_register(luaState, "doSetItemSpecialDescription", ActionScript::luaActionDoSetItemSpecialDescription);
    //doSendAnimatedText(position,text,color)
    lua_register(luaState, "doSendAnimatedText", ActionScript::luaActionDoSendAnimatedText);
    //doPlayerAddSkillTry(uid,skillid,n)
    lua_register(luaState, "doPlayerAddSkillTry", ActionScript::luaActionDoPlayerAddSkillTry);
    //doPlayerAddHealth(uid,health)
    lua_register(luaState, "doPlayerAddHealth", ActionScript::luaActionDoPlayerAddHealth);
    //doPlayerAddMana(uid,mana)
    lua_register(luaState, "doPlayerAddMana", ActionScript::luaActionDoPlayerAddMana);
    //doPlayerAddItem(uid,itemid,count or type) . returns uid of the created item
    lua_register(luaState, "doPlayerAddItem", ActionScript::luaActionDoPlayerAddItem);
    //doPlayerSendTextMessage(uid,MessageClasses,message)
    lua_register(luaState, "doPlayerSendTextMessage", ActionScript::luaActionDoPlayerSendTextMessage);
    //doPlayerRemoveMoney(uid,money)
    lua_register(luaState, "doPlayerRemoveMoney", ActionScript::luaActionDoPlayerRemoveMoney);
    //doShowTextWindow(uid,maxlen,canWrite)
    lua_register(luaState, "doShowTextWindow", ActionScript::luaActionDoShowTextWindow);
    //doDecayItem(uid)
    lua_register(luaState, "doDecayItem", ActionScript::luaActionDoDecayItem);
    //doCreateItem(itemid,type or count,position) .only working on ground. returns uid of the created item
    lua_register(luaState, "doCreateItem", ActionScript::luaActionDoCreateItem);
    //doSummonCreature(name, position)
    lua_register(luaState, "doSummonCreature", ActionScript::luaActionDoSummonCreature);
    //doPlayerSetMasterPos(cid,pos)
    lua_register(luaState, "doPlayerSetMasterPos", ActionScript::luaActionDoPlayerSetMasterPos);
    //doPlayerSetVocation(cid,voc)
    lua_register(luaState, "doPlayerSetVocation", ActionScript::luaActionDoPlayerSetVocation);
    //doPlayerRemoveItem(cid,itemid,count)
    lua_register(luaState, "doPlayerRemoveItem", ActionScript::luaActionDoPlayerRemoveItem);
    //getSlotItem(cid,slot)
    lua_register(luaState, "getSlotItem", ActionScript::luaActionGetPlayerSlotItem);
    //doPlayerSetDrunk(cid,time)
    lua_register(luaState, "doPlayerSetDrunk", ActionScript::luaActionDoPlayerSetDrunk);
    //getCreatureName(cid)
    lua_register(luaState, "getCreatureName", ActionScript::luaActionGetCreatureName);
    //doMoveItem(uid,toPos)
    //doMovePlayer(cid,direction)

    //doPlayerAddCondition(....)

    lua_register(luaState, "getItemName", ActionScript::luaActionGetItemName);
    lua_register(luaState, "doCreateCondition", ActionScript::luaActionDoCreateCondition);
    lua_register(luaState, "addTeleport", ActionScript::luaActionDoAddTeleport);
    lua_register(luaState, "removeTeleport", ActionScript::luaActionDoRemoveTeleport);
    lua_register(luaState, "getTownIDByName", ActionScript::luaActionGetTownIDByName);
    lua_register(luaState, "getTownNameByID", ActionScript::luaActionGetTownNameByID);
    lua_register(luaState, "doSetItemUniqueID", ActionScript::luaActionDoSetItemUniqueID);
    lua_register(luaState, "isPlayer", ActionScript::luaActionIsPlayer);
    lua_register(luaState, "isMonster", ActionScript::luaActionIsMonster);
    return true;
}

ActionScript* ActionScript::getActionScript(lua_State *L)
{
    lua_getglobal(L, "addressOfActionScript");
    int32_t val = (int32_t)internalGetNumber(L);
    ActionScript* myaction = (ActionScript*)val;
    if(!myaction)
    {
        return 0;
    }
    return myaction;
}


Position ActionScript::internalGetRealPosition(ActionScript *action, Player *player, const Position &pos)
{
    if(action)
        return action->game->getThingMapPos(player, pos);
    else
    {
        Position dummyPos(0,0,0);
        return dummyPos;
    }
}

void ActionScript::internalAddThing(lua_State *L, const Thing* thing, const uint32_t thingid)
{
    lua_newtable(L);
    if(dynamic_cast<const Item*>(thing))
    {
        const Item *item = dynamic_cast<const Item*>(thing);
        setField(L,"uid", thingid);
        setField(L,"itemid", item->getID());
        setField(L,"type", item->getItemCountOrSubtype());
        setField(L,"actionid", item->getActionId());
    }
    else if(dynamic_cast<const Creature*>(thing))
    {
        setField(L,"uid", thingid);
        setField(L,"itemid", 1);
        char type;
        if(dynamic_cast<const Player*>(thing))
        {
            type = 1;
        }
        else if(dynamic_cast<const Monster*>(thing))
        {
            type = 2;
        }
        else //npc
        {
            type = 3;
        }
        setField(L,"type", type);
        setField(L,"actionid", 0);
    }
    else
    {
        setField(L,"uid", 0);
        setField(L,"itemid", 0);
        setField(L,"type", 0);
        setField(L,"actionid", 0);
    }
}

void ActionScript::internalAddPositionEx(lua_State *L, const PositionEx& pos)
{
    lua_newtable(L);
    setField(L,"z", pos.z);
    setField(L,"y", pos.y);
    setField(L,"x", pos.x);
    setField(L,"stackpos",pos.stackpos);
}

void ActionScript::internalGetPositionEx(lua_State *L, PositionEx& pos)
{
    pos.z = (int32_t)getField(L,"z");
    pos.y = (int32_t)getField(L,"y");
    pos.x = (int32_t)getField(L,"x");
    pos.stackpos = (int32_t)getField(L,"stackpos");
    lua_pop(L, 1); //table
}

uint32_t ActionScript::internalGetNumber(lua_State *L)
{
    lua_pop(L,1);
    return (uint32_t)lua_tonumber(L, 0);
}
const char* ActionScript::internalGetString(lua_State *L)
{
    lua_pop(L,1);
    return lua_tostring(L, 0);
}

int32_t ActionScript::internalGetPlayerInfo(lua_State *L, ePlayerInfo info)
{
    uint32_t cid = (uint32_t)internalGetNumber(L);
    ActionScript *action = getActionScript(L);


    const KnownThing* tmp = action->GetPlayerByUID(cid);
    if(tmp)
    {
        PositionEx pos;
        Tile *tile;
        Player *player = (Player*)(tmp->thing);
        int64_t value;
        switch(info)
        {
        case PlayerInfoAccess:
            value = player->access;
            break;
        case PlayerInfoLevel:
            value = player->level;
            break;
        case PlayerInfoMagLevel:
            value = player->maglevel;
            break;
        case PlayerInfoMana:
            value = player->mana;
            break;
        case PlayerInfoHealth:
            value = player->health;
            break;
        case PlayerInfoName:
            lua_pushstring(L, player->name.c_str());
            return 1;
            break;
        case PlayerInfoPosition:
            pos = player->pos;
            //tile = action->game->map->getTile(player->pos.x, player->pos.y, player->pos.z);
            tile = action->game->map->getTile(player->pos);
            if(tile)
                pos.stackpos = tile->getCreatureStackPos(player);
            internalAddPositionEx(L,pos);
            return 1;
            break;
        case PlayerInfoMasterPos:
            pos = player->masterPos;
            internalAddPositionEx(L,pos);
            return 1;
            break;
        case PlayerInfoFood:
            value = player->food/1000;
            break;
        case PlayerInfoVocation:
            value = player->vocation;
            break;
        case PlayerInfoGuildId:
            value = player->guildId;
            break;
        default:
            std::cout << "GetPlayerInfo: Unkown player info " << info << std::endl;
            value = 0;
            break;
        }
        lua_pushnumber(L,value);
        return 1;
    }
    else
    {
        lua_pushnumber(L, -1);
        std::cout << "GetPlayerInfo(" << info << "): player not found" << std::endl;
        return 1;
    }

    lua_pushnumber(L, 0);
    return 1;
}
//getPlayer[Info](uid)
int32_t ActionScript::luaActionGetPlayerFood(lua_State *L)
{
    return internalGetPlayerInfo(L,PlayerInfoFood);
}

int32_t ActionScript::luaActionGetPlayerAccess(lua_State *L)
{
    return internalGetPlayerInfo(L,PlayerInfoAccess);
}

int32_t ActionScript::luaActionGetPlayerLevel(lua_State *L)
{
    return internalGetPlayerInfo(L,PlayerInfoLevel);
}

int32_t ActionScript::luaActionGetPlayerMagLevel(lua_State *L)
{
    return internalGetPlayerInfo(L,PlayerInfoMagLevel);
}

int32_t ActionScript::luaActionGetPlayerMana(lua_State *L)
{
    return internalGetPlayerInfo(L,PlayerInfoMana);
}

int32_t ActionScript::luaActionGetPlayerHealth(lua_State *L)
{
    return internalGetPlayerInfo(L,PlayerInfoHealth);
}

int32_t ActionScript::luaActionGetPlayerName(lua_State *L)
{
    return internalGetPlayerInfo(L,PlayerInfoName);
}

int32_t ActionScript::luaActionGetPlayerPosition(lua_State *L)
{
    return internalGetPlayerInfo(L,PlayerInfoPosition);
}

int32_t ActionScript::luaActionGetPlayerVocation(lua_State *L)
{
    return internalGetPlayerInfo(L,PlayerInfoVocation);
}

int32_t ActionScript::luaActionGetPlayerMasterPos(lua_State *L)
{
    return internalGetPlayerInfo(L,PlayerInfoMasterPos);
}

int32_t ActionScript::luaActionGetPlayerGuildId(lua_State *L)
{
    return internalGetPlayerInfo(L,PlayerInfoGuildId);
}
//

int32_t ActionScript::luaActionDoRemoveItem(lua_State *L)
{
    //doRemoveItem(uid,n)
    char n = (unsigned char)internalGetNumber(L);
    uint16_t itemid = (uint16_t)internalGetNumber(L);

    ActionScript *action = getActionScript(L);

    const KnownThing* tmp = action->GetItemByUID(itemid);
    Item *tmpitem = NULL;
    PositionEx tmppos;
    if(tmp)
    {
        tmpitem = (Item*)tmp->thing;
        tmppos = tmp->pos;
        if(tmpitem->isSplash())
        {
            lua_pushnumber(L, -1);
            std::cout << "luaDoRemoveItem: can not remove a splash" << std::endl;
            return 1;
        }
    }
    else
    {
        lua_pushnumber(L, -1);
        std::cout << "luaDoRemoveItem: item not found" << std::endl;
        return 1;
    }

    if(tmpitem->isStackable() && (tmpitem->getItemCountOrSubtype() - n) > 0)
    {
        tmpitem->setItemCountOrSubtype(tmpitem->getItemCountOrSubtype() - n);
        action->game->sendUpdateThing(action->_player,(Position&)tmppos,tmpitem,tmppos.stackpos);
    }
    else
    {
        if(action->game->removeThing(action->_player,(Position&)tmppos,tmpitem))
        {
            action->game->FreeThing(tmpitem);
        }
    }

    lua_pushnumber(L, 0);
    return 1;
}

int32_t ActionScript::luaActionDoPlayerRemoveItem(lua_State *L)
{
    //doPlayerRemoveItem(cid,itemid,count)
    int32_t count = (unsigned char)internalGetNumber(L);
    uint16_t itemid = (uint16_t)internalGetNumber(L);
    uint32_t cid = (uint32_t)internalGetNumber(L);

    ActionScript *action = getActionScript(L);

    const KnownThing* tmp = action->GetPlayerByUID(cid);
    if(tmp)
    {
        Player *player = (Player*)(tmp->thing);
        if(player->removeItem(itemid, count))
        {
            lua_pushnumber(L, 1);
        }
        else
        {
            lua_pushnumber(L, 0);
        }
    }
    else
    {
        lua_pushnumber(L, -1);
        std::cout << "luaDoPlayerRemoveItem: player not found" << std::endl;
        return 1;
    }

    return 1;
}

int32_t ActionScript::luaActionDoFeedPlayer(lua_State *L)
{
    //doFeedPlayer(uid,food)
    int32_t food = (int32_t)internalGetNumber(L);
    uint32_t cid = (uint32_t)internalGetNumber(L);

    ActionScript *action = getActionScript(L);

    const KnownThing* tmp = action->GetPlayerByUID(cid);
    if(tmp)
    {
        Player *player = (Player*)(tmp->thing);
        player->food += food*1000;
    }
    else
    {
        lua_pushnumber(L, -1);
        std::cout << "luaDoFeedPlayer: player not found" << std::endl;
        return 1;
    }

    lua_pushnumber(L, 0);
    return 1;
}

int32_t ActionScript::luaActionDoSendCancel(lua_State *L)
{
    //doSendCancel(uid,text)
    const char * text = internalGetString(L);
    uint32_t cid = (uint32_t)internalGetNumber(L);

    ActionScript *action = getActionScript(L);

    const KnownThing* tmp = action->GetPlayerByUID(cid);
    if(tmp)
    {
        Player *player = (Player*)(tmp->thing);
        player->sendCancel(text);
    }
    else
    {
        lua_pushnumber(L, -1);
        std::cout << "luaSendCancel: player not found" << std::endl;
        return 1;
    }
    lua_pushnumber(L, 0);
    return 1;
}


int32_t ActionScript::luaActionDoTeleportThing(lua_State *L)
{
    //doTeleportThing(uid,newpos)
    PositionEx pos;
    internalGetPositionEx(L,pos);
    uint32_t id = (uint32_t)internalGetNumber(L);

    ActionScript *action = getActionScript(L);
    Thing *tmpthing;

    const KnownThing* tmp = action->GetThingByUID(id);
    if(tmp)
    {
        tmpthing = tmp->thing;
    }
    else
    {
        lua_pushnumber(L, -1);
        std::cout << "luaTeleport: thing not found" << std::endl;
        return 1;
    }
    //std::cout << "new pos: " << (Position&)pos << std::endl;
    if(tmp->type == thingTypeItem)
    {
        //avoid teleport notMoveable items
        if(((Item*)tmp->thing)->isNotMoveable())
        {
            lua_pushnumber(L, -1);
            std::cout << "luaTeleport: item is not moveable" << std::endl;
            return 1;
        }
    }

    action->game->teleport(tmpthing,(Position&)pos);
    //Tile *tile = action->game->getTile(pos.x, pos.y, pos.z);
    Tile *tile = action->game->map->getTile(pos);
    if(tile)
    {
        pos.stackpos = tile->getThingStackPos(tmpthing);
    }
    else
    {
        pos.stackpos = 1;
    }
    action->UpdateThingPos(id,pos);

    lua_pushnumber(L, 0);
    return 1;
}


int32_t ActionScript::luaActionDoTransformItem(lua_State *L)
{
    //doTransformItem(uid,toitemid)
    uint32_t toid = (uint32_t)internalGetNumber(L);
    uint32_t itemid = (uint32_t)internalGetNumber(L);

    ActionScript *action = getActionScript(L);

    const KnownThing* tmp = action->GetItemByUID(itemid);
    Item *tmpitem = NULL;
    PositionEx tmppos;
    if(tmp)
    {
        tmpitem = (Item*)tmp->thing;
        tmppos = tmp->pos;
    }
    else
    {
        lua_pushnumber(L, -1);
        std::cout << "luaDoTransform: Item not found" << std::endl;
        return 1;
    }

    tmpitem->setID(toid);

    action->game->sendUpdateThing(action->_player,(Position&)tmppos,tmpitem,tmppos.stackpos);

    lua_pushnumber(L, 0);
    return 1;
}

int32_t ActionScript::luaActionDoPlayerSay(lua_State *L)
{
    //doPlayerSay(uid,text,type)
    int32_t type = (int32_t)internalGetNumber(L);
    const char * text = internalGetString(L);
    uint32_t cid = (uint32_t)internalGetNumber(L);

    ActionScript *action = getActionScript(L);

    const KnownThing* tmp = action->GetPlayerByUID(cid);
    if(tmp)
    {
        Player *player = (Player*)(tmp->thing);
        action->game->creatureSay(player,(SpeakClasses)type,std::string(text));
    }
    else
    {
        lua_pushnumber(L, -1);
        std::cout << "luaDoPlayerSay: player not found" << std::endl;
        return 1;
    }

    lua_pushnumber(L, 0);
    return 1;
}

int32_t ActionScript::luaActionDoSendMagicEffect(lua_State *L)
{
    //doSendMagicEffect(position,type)
    int32_t type = (int32_t)internalGetNumber(L);
    PositionEx pos;
    internalGetPositionEx(L,pos);

    ActionScript *action = getActionScript(L);

    Position realpos = internalGetRealPosition(action, action->_player,(Position&)pos);
    SpectatorVec list;
    SpectatorVec::iterator it;

    action->game->getSpectators(Range(realpos, true), list);

    for(it = list.begin(); it != list.end(); ++it)
    {
        Player *p = dynamic_cast<Player*>(*it);
        if(p)
            p->sendMagicEffect(realpos,type);
    }

    lua_pushnumber(L, 0);
    return 1;
}

int32_t ActionScript::luaActionDoChangeTypeItem(lua_State *L)
{
    //doChangeTypeItem(uid,new_type)
    uint32_t new_type = (uint32_t)internalGetNumber(L);
    uint32_t itemid = (uint32_t)internalGetNumber(L);

    ActionScript *action = getActionScript(L);

    const KnownThing* tmp = action->GetItemByUID(itemid);
    Item *tmpitem = NULL;
    PositionEx tmppos;
    if(tmp)
    {
        tmpitem = (Item*)tmp->thing;
        tmppos = tmp->pos;
    }
    else
    {
        lua_pushnumber(L, -1);
        std::cout << "luaDoChangeTypeItem: Item not found" << std::endl;
        return 1;
    }

    tmpitem->setItemCountOrSubtype(new_type);

    action->game->sendUpdateThing(action->_player,(Position&)tmppos,tmpitem,tmppos.stackpos);

    lua_pushnumber(L, 0);
    return 1;
}


int32_t ActionScript::luaActionDoPlayerAddSkillTry(lua_State *L)
{
    //doPlayerAddSkillTry(uid,skillid,n)
    int32_t n = (int32_t)internalGetNumber(L);
    int32_t skillid = (int32_t)internalGetNumber(L);
    uint32_t cid = (uint32_t)internalGetNumber(L);

    ActionScript *action = getActionScript(L);

    const KnownThing* tmp = action->GetPlayerByUID(cid);
    if(tmp)
    {
        Player *player = (Player*)(tmp->thing);
        player->addSkillTryInternal(n,skillid);
    }
    else
    {
        lua_pushnumber(L, -1);
        std::cout << "luaDoPlayerAddSkillTry: player not found" << std::endl;
        return 1;
    }

    lua_pushnumber(L, 0);
    return 1;
}


int32_t ActionScript::luaActionDoPlayerAddHealth(lua_State *L)
{
    //doPlayerAddHealth(uid,health)
    int64_t addhealth = (int64_t)internalGetNumber(L);
    uint32_t cid = (uint32_t)internalGetNumber(L);

    ActionScript *action = getActionScript(L);

    const KnownThing* tmp = action->GetPlayerByUID(cid);
    if(tmp)
    {
        Player *player = (Player*)(tmp->thing);
        int64_t tmp = player->health + addhealth;
        if(tmp <= 0)
        {
            player->health = 1;
        }
        else if(tmp > player->healthmax)
        {
            player->health = player->healthmax;
        }
        else
        {
            player->health = tmp;
        }
        player->sendStats();

        SpectatorVec list;
        SpectatorVec::iterator it;

        action->game->getSpectators(Range(player->pos,true), list);
        for(it = list.begin(); it != list.end(); ++it)
        {
            Player* p = dynamic_cast<Player*>(*it);
            if(p)
                p->sendCreatureHealth(player);
        }
    }
    else
    {
        lua_pushnumber(L, -1);
        std::cout << "luaDoPlayerAddHealth: player not found" << std::endl;
        return 1;
    }

    lua_pushnumber(L, 0);
    return 1;
}

int32_t ActionScript::luaActionDoPlayerAddMana(lua_State *L)
{
    //doPlayerAddMana(uid,mana)
    int64_t addmana = internalGetNumber(L);
    uint32_t cid = (uint32_t)internalGetNumber(L);

    ActionScript *action = getActionScript(L);

    const KnownThing* tmp = action->GetPlayerByUID(cid);
    if(tmp)
    {
        Player *player = (Player*)(tmp->thing);
        player->mana = std::min(player->manamax,player->mana+addmana);
        player->sendStats();
    }
    else
    {
        lua_pushnumber(L, -1);
        std::cout << "luaDoPlayerAddMana: player not found" << std::endl;
        return 1;
    }

    lua_pushnumber(L, 0);
    return 1;
}

int32_t ActionScript::luaActionDoPlayerAddItem(lua_State *L)
{
    //doPlayerAddItem(uid,itemid,count or type)
    int32_t type = (int32_t)internalGetNumber(L);
    int32_t itemid = (int32_t)internalGetNumber(L);
    uint32_t cid = (uint32_t)internalGetNumber(L);

    ActionScript *action = getActionScript(L);
    uint32_t uid;
    const KnownThing* tmp = action->GetPlayerByUID(cid);
    if(tmp)
    {
        PositionEx pos;
        Player *player = (Player*)(tmp->thing);
        Item *newitem = Item::CreateItem(itemid,type);
        if(!player->addItem(newitem))
        {
            //add item on the ground
            action->game->addThing(NULL,action->_player->pos,newitem);
            //Tile *tile = action->game->getTile(newitem->pos.x, newitem->pos.y, newitem->pos.z);
            Tile *tile = action->game->map->getTile(newitem->pos);
            if(tile)
            {
                pos.stackpos = tile->getThingStackPos(newitem);
            }
            else
            {
                pos.stackpos = 1;
            }
        }
        pos.x = newitem->pos.x;
        pos.y = newitem->pos.y;
        pos.z = newitem->pos.z;
        uid = action->AddThingToMap((Thing*)newitem,pos);
    }
    else
    {
        lua_pushnumber(L, -1);
        std::cout << "luaDoPlayerAddItem: player not found" << std::endl;
        return 1;
    }

    lua_pushnumber(L, uid);
    return 1;
}


int32_t ActionScript::luaActionDoPlayerSendTextMessage(lua_State *L)
{
    //doPlayerSendTextMessage(uid,MessageClasses,message)
    const char * text = internalGetString(L);
    unsigned char messageClass = (unsigned char)internalGetNumber(L);
    uint32_t cid = (uint32_t)internalGetNumber(L);

    ActionScript *action = getActionScript(L);

    const KnownThing* tmp = action->GetPlayerByUID(cid);
    if(tmp)
    {
        Player *player = (Player*)(tmp->thing);
        player->sendTextMessage((MessageClasses)messageClass,text);;
    }
    else
    {
        lua_pushnumber(L, -1);
        std::cout << "luaSendTextMessage: player not found" << std::endl;
        return 1;
    }
    lua_pushnumber(L, 0);
    return 1;
}

int32_t ActionScript::luaActionDoSendAnimatedText(lua_State *L)
{
    //doSendAnimatedText(position,text,color)
    int32_t color = (int32_t)internalGetNumber(L);
    const char * text = internalGetString(L);
    PositionEx pos;
    internalGetPositionEx(L,pos);

    ActionScript *action = getActionScript(L);

    Position realpos = internalGetRealPosition(action, action->_player,(Position&)pos);
    SpectatorVec list;
    SpectatorVec::iterator it;

    action->game->getSpectators(Range(realpos, true), list);

    for(it = list.begin(); it != list.end(); ++it)
    {
        Player *p = dynamic_cast<Player*>(*it);
        if(p)
            p->sendAnimatedText(realpos, color, text);
    }

    lua_pushnumber(L, 0);
    return 1;
}

int32_t ActionScript::luaActionGetPlayerSkill(lua_State *L)
{
    //getPlayerSkill(uid,skillid)
    unsigned char skillid = (uint32_t)internalGetNumber(L);
    uint32_t cid = (uint32_t)internalGetNumber(L);

    ActionScript *action = getActionScript(L);

    const KnownThing* tmp = action->GetPlayerByUID(cid);
    if(tmp)
    {
        if(skillid > 6)
        {
            lua_pushnumber(L, -1);
            std::cout << "GetPlayerSkill: invalid skillid" << std::endl;
            return 1;
        }
        Player *player = (Player*)(tmp->thing);
        int32_t value = player->skills[skillid][SKILL_LEVEL];
        lua_pushnumber(L,value);
        return 1;
    }
    else
    {
        lua_pushnumber(L, -1);
        std::cout << "GetPlayerSkill: player not found" << std::endl;
        return 1;
    }
}

int32_t ActionScript::luaActionDoShowTextWindow(lua_State *L)
{
    //doShowTextWindow(uid,maxlen,canWrite)
    bool canWrite = (internalGetNumber(L) != 0);
    uint16_t maxlen = (uint16_t)internalGetNumber(L);
    uint32_t uid = (uint32_t)internalGetNumber(L);

    ActionScript *action = getActionScript(L);

    const KnownThing* tmp = action->GetItemByUID(uid);
    Item *tmpitem = NULL;
    if(tmp)
    {
        tmpitem = (Item*)tmp->thing;
    }
    else
    {
        lua_pushnumber(L, -1);
        std::cout << "luadoShowTextWindow: Item not found" << std::endl;
        return 1;
    }

    action->_player->sendTextWindow(tmpitem,maxlen,canWrite);

    lua_pushnumber(L, 0);
    return 1;
}

int32_t ActionScript::luaActionGetItemRWInfo(lua_State *L)
{
    //getItemRWInfo(uid)
    uint32_t uid = (uint32_t)internalGetNumber(L);

    ActionScript *action = getActionScript(L);

    const KnownThing* tmp = action->GetItemByUID(uid);
    Item *tmpitem = NULL;
    if(tmp)
    {
        tmpitem = (Item*)tmp->thing;
    }
    else
    {
        lua_pushnumber(L, -1);
        std::cout << "luagetItemRWInfo: Item not found" << std::endl;
        return 1;
    }

    lua_pushnumber(L, (int32_t)tmpitem->getRWInfo());

    return 1;
}

int32_t ActionScript::luaActionDoDecayItem(lua_State *L)
{
    //doDecayItem(uid)
    //Note: to stop decay set decayTo = 0 in items.xml
    uint32_t uid = (uint32_t)internalGetNumber(L);

    ActionScript *action = getActionScript(L);

    const KnownThing* tmp = action->GetItemByUID(uid);
    Item *tmpitem = NULL;
    if(tmp)
    {
        tmpitem = (Item*)tmp->thing;
    }
    else
    {
        lua_pushnumber(L, -1);
        std::cout << "luadoDecayItem: Item not found" << std::endl;
        return 1;
    }

    action->game->startDecay(tmpitem);

    lua_pushnumber(L, 0);
    return 1;
}

int32_t ActionScript::luaActionGetThingfromPos(lua_State *L)
{
    //getThingfromPos(pos)
    //Note:
    //	stackpos = 255. Get the top thing(item moveable or creature).
    //	stackpos = 254. Get MagicFieldtItem
    //	stackpos = 253. Get Creature

    PositionEx pos;
    internalGetPositionEx(L,pos);

    ActionScript *action = getActionScript(L);

    //Tile *tile = action->game->getTile(pos.x, pos.y, pos.z);
    Tile *tile = action->game->map->getTile(pos);

    Thing *thing = NULL;

    if(tile)
    {
        if(pos.stackpos == 255)
        {
            thing = tile->getTopMoveableThing();
        }
        else if(pos.stackpos == 254)
        {
            thing = tile->getFieldItem();
        }
        else if(pos.stackpos == 253)
        {
            thing = tile->getTopCreature();
        }
        else
        {
            thing = tile->getThingByStackPos(pos.stackpos);
        }

        if(thing)
        {
            if(pos.stackpos > 250)
            {
                pos.stackpos = tile->getThingStackPos(thing);
            }
            uint32_t thingid = action->AddThingToMap(thing,pos);
            internalAddThing(L,thing,thingid);
        }
        else
        {
            internalAddThing(L,NULL,0);
        }
        return 1;

    }//if(tile)
    else
    {
        std::cout << "luagetItemfromPos: Tile not found" << std::endl;
        internalAddThing(L,NULL,0);
        return 1;
    }
}

int32_t ActionScript::luaActionDoCreateItem(lua_State *L)
{
    //doCreateItem(itemid,type or count,position) .only working on ground. returns uid of the created item
    PositionEx pos;
    internalGetPositionEx(L,pos);
    int32_t type = (int32_t)internalGetNumber(L);
    int32_t itemid = (int32_t)internalGetNumber(L);

    ActionScript *action = getActionScript(L);

    Item *newitem = Item::CreateItem(itemid,type);
    action->game->addThing(NULL,(Position&)pos,newitem);
    //Tile *tile = action->game->getTile(pos.x, pos.y, pos.z);
    Tile *tile = action->game->map->getTile(pos);
    if(tile)
    {
        pos.stackpos = tile->getThingStackPos(newitem);
    }
    else
    {
        pos.stackpos = 1;
    }

    uint32_t uid = action->AddThingToMap((Thing*)newitem,pos);

    lua_pushnumber(L, uid);
    return 1;
}

int32_t ActionScript::luaActionGetPlayerStorageValue(lua_State *L)
{
    //getPlayerStorageValue(cid,valueid)
    uint32_t key = (uint32_t)internalGetNumber(L);
    uint32_t cid = (uint32_t)internalGetNumber(L);

    ActionScript *action = getActionScript(L);

    const KnownThing* tmp = action->GetPlayerByUID(cid);
    if(tmp)
    {
        Player *player = (Player*)(tmp->thing);
        int32_t value;
        if(player->getStorageValue(key,value))
        {
            lua_pushnumber(L,value);
        }
        else
        {
            lua_pushnumber(L,-1);
        }
        return 1;
    }
    else
    {
        lua_pushnumber(L, -1);
        std::cout << "GetPlayerStorageValue: player not found" << std::endl;
        return 1;
    }
}

int32_t ActionScript::luaActionSetPlayerStorageValue(lua_State *L)
{
    //setPlayerStorageValue(cid,valueid, newvalue)
    int32_t value = (uint32_t)internalGetNumber(L);
    uint32_t key = (uint32_t)internalGetNumber(L);
    uint32_t cid = (uint32_t)internalGetNumber(L);

    ActionScript *action = getActionScript(L);

    const KnownThing* tmp = action->GetPlayerByUID(cid);
    if(tmp)
    {
        Player *player = (Player*)(tmp->thing);
        player->addStorageValue(key,value);
    }
    else
    {
        lua_pushnumber(L, -1);
        std::cout << "SetPlayerStorageValue: player not found" << std::endl;
        return 1;
    }
    lua_pushnumber(L,0);
    return 1;
}

int32_t ActionScript::luaActionDoSetItemActionId(lua_State *L)
{
    //doSetItemActionId(uid,actionid)
    uint32_t actionid = (uint32_t)internalGetNumber(L);
    uint32_t itemid = (uint32_t)internalGetNumber(L);

    ActionScript *action = getActionScript(L);

    const KnownThing* tmp = action->GetItemByUID(itemid);
    Item *tmpitem = NULL;
    PositionEx tmppos;
    if(tmp)
    {
        tmpitem = (Item*)tmp->thing;
    }
    else
    {
        lua_pushnumber(L, -1);
        std::cout << "luaDoSetActionId: Item not found" << std::endl;
        return 1;
    }

    tmpitem->setActionId(actionid);

    lua_pushnumber(L, 0);
    return 1;
}

int32_t ActionScript::luaActionDoSetItemText(lua_State *L)
{
    //doSetItemText(uid,text)
    const char *text = internalGetString(L);
    uint32_t itemid = (uint32_t)internalGetNumber(L);

    ActionScript *action = getActionScript(L);

    const KnownThing* tmp = action->GetItemByUID(itemid);
    Item *tmpitem = NULL;
    PositionEx tmppos;
    if(tmp)
    {
        tmpitem = (Item*)tmp->thing;
    }
    else
    {
        lua_pushnumber(L, -1);
        std::cout << "luaDoSetText: Item not found" << std::endl;
        return 1;
    }

    tmpitem->setText(text);

    lua_pushnumber(L, 0);
    return 1;
}

int32_t ActionScript::luaActionDoSetItemSpecialDescription(lua_State *L)
{
    //doSetItemSpecialDescription(uid,desc)
    const char *desc = internalGetString(L);
    uint32_t itemid = (uint32_t)internalGetNumber(L);

    ActionScript *action = getActionScript(L);

    const KnownThing* tmp = action->GetItemByUID(itemid);
    Item *tmpitem = NULL;
    PositionEx tmppos;
    if(tmp)
    {
        tmpitem = (Item*)tmp->thing;
    }
    else
    {
        lua_pushnumber(L, -1);
        std::cout << "luaDoSetSpecialDescription: Item not found" << std::endl;
        return 1;
    }

    tmpitem->setSpecialDescription(desc);

    lua_pushnumber(L, 0);
    return 1;
}

int32_t ActionScript::luaActionGetTilePzInfo(lua_State *L)
{
    //getTilePzInfo(pos)
    PositionEx pos;
    internalGetPositionEx(L,pos);

    ActionScript *action = getActionScript(L);

    //Tile *tile = action->game->getTile(pos.x, pos.y, pos.z);
    Tile *tile = action->game->map->getTile(pos);

    if(tile)
    {
        if(tile->isPz())
        {
            lua_pushnumber(L, 1);
        }
        else
        {
            lua_pushnumber(L, 0);
        }
    }//if(tile)
    else
    {
        std::cout << "luagetTilePzInfo: Tile not found" << std::endl;
        lua_pushnumber(L, -1);
    }
    return 1;
}

int32_t ActionScript::luaActionDoSummonCreature(lua_State *L)
{
    //doSummonCreature(name, position)
    PositionEx pos;
    internalGetPositionEx(L,pos);
    const char *name = internalGetString(L);

    ActionScript *action = getActionScript(L);

    //Monster *monster = new Monster(name, action->game);
    Monster* monster = Monster::createMonster(name, action->game);
    //if(!monster->isLoaded()){
    if(!monster)
    {
        //delete monster;
        lua_pushnumber(L, 0);
        std::cout << "luadoSummonCreature: Monster not found" << std::endl;
        return 1;
    }

    if(!action->game->placeCreature((Position&)pos, monster))
    {
        delete monster;
        lua_pushnumber(L, 0);
        std::cout << "luadoSummonCreature: Can not place the monster" << std::endl;
        return 1;
    }

    uint32_t cid = action->AddThingToMap((Thing*)monster,pos);

    lua_pushnumber(L, cid);
    return 1;
}


int32_t ActionScript::luaActionDoPlayerRemoveMoney(lua_State *L)
{
    //doPlayerRemoveMoney(uid,money)
    int32_t money = (int32_t)internalGetNumber(L);
    uint32_t cid = (uint32_t)internalGetNumber(L);

    ActionScript *action = getActionScript(L);

    const KnownThing* tmp = action->GetPlayerByUID(cid);
    if(tmp)
    {
        Player *player = (Player*)(tmp->thing);
        if(player->substractMoney(money))
        {
            lua_pushnumber(L, 1);
        }
        else
        {
            lua_pushnumber(L, 0);
        }
    }
    else
    {
        lua_pushnumber(L, -1);
        std::cout << "doPlayerRemoveMoney: player not found" << std::endl;
        return 1;
    }

    return 1;
}

int32_t ActionScript::luaActionDoPlayerSetMasterPos(lua_State *L)
{
    //doPlayerSetMasterPos(cid,pos)
    PositionEx pos;
    internalGetPositionEx(L,pos);
    uint32_t cid = (uint32_t)internalGetNumber(L);

    ActionScript *action = getActionScript(L);

    const KnownThing* tmp = action->GetPlayerByUID(cid);
    if(tmp)
    {
        Player *player = (Player*)tmp->thing;
        player->masterPos = pos;
    }
    else
    {
        lua_pushnumber(L, -1);
        std::cout << "doPlayerSetMasterPos: player not found" << std::endl;
        return 1;
    }
    lua_pushnumber(L, 0);
    return 1;
}

int32_t ActionScript::luaActionDoPlayerSetVocation(lua_State *L)
{
    //doPlayerSetVocation(cid,voc)
    int32_t voc = (int32_t)internalGetNumber(L);
    uint32_t cid = (uint32_t)internalGetNumber(L);

    ActionScript *action = getActionScript(L);

    const KnownThing* tmp = action->GetPlayerByUID(cid);
    if(tmp)
    {
        Player *player = (Player*)(tmp->thing);
        player->vocation = (playervoc_t)voc;
    }
    else
    {
        lua_pushnumber(L, -1);
        std::cout << "doPlayerSetVocation: player not found" << std::endl;
        return 1;
    }

    lua_pushnumber(L, 0);
    return 1;
}

int32_t ActionScript::luaActionGetItemName(lua_State *L)
{
    //getItemName(id)
    int32_t id = (int32_t)internalGetNumber(L);
    lua_pushstring(L, Item::items[id].name.c_str());
    return 1;
}

int32_t ActionScript::luaActionDoPlayerSetDrunk(lua_State *L)
{
//doPlayerSetDrunk(cid,time)
    int32_t time = (int32_t)internalGetNumber(L);
    uint32_t cid = (uint32_t)internalGetNumber(L);
    ActionScript *action = getActionScript(L);
    const KnownThing* tmp = action->GetPlayerByUID(cid);

    if(tmp)
    {
        Player *player = (Player*)(tmp->thing);
        if(player && player->immunities != ATTACK_DRUNKNESS)
        {
            action->game->CreateCondition(player, NULL, 0xB4, NM_ME_MAGIC_BLOOD, NM_ME_MAGIC_BLOOD, ATTACK_DRUNKNESS, false, 0, 0, time, 1);
            return 1;
        }
//player->drunkTicks = time;
    }
    else
    {
        lua_pushnumber(L, -1);
        std::cout << "doPlayerSetDrunk: player not found" << std::endl;
        return 1;
    }
    lua_pushnumber(L, 0);
    return 1;
}

int32_t ActionScript::luaActionDoCreateCondition(lua_State *L)
{
//DoCreateCondition& (creatureid,animationColor,damageEffect,hitEffect,attackType,offensive,maxDamage,minDamage,ticks,count)
    int32_t count = (int32_t)internalGetNumber(L);
    int32_t ticks = (int32_t)internalGetNumber(L);
    int64_t minDamage = (int64_t)internalGetNumber(L);
    int64_t maxDamage = (int64_t)internalGetNumber(L);
    bool offensive = (bool)internalGetNumber(L);
    int32_t attacktype = (int32_t)internalGetNumber(L);
    unsigned char hitEffect = (unsigned char)internalGetNumber(L);
    unsigned char damageEffect = (unsigned char)internalGetNumber(L);
    unsigned char animationColor = (unsigned char)internalGetNumber(L);
    uint32_t cid = (uint32_t)internalGetNumber(L);

    ActionScript *action = getActionScript(L);
    const KnownThing* tmp = action->GetCreatureByUID(cid);

    if(tmp)
    {
        Creature *creature = (Creature*)(tmp->thing);
        attacktype_t attackType;
        switch(attacktype)
        {
        case 0:
            attackType = ATTACK_NONE;
            break;
        case 1:
            attackType = ATTACK_ENERGY;
            break;
        case 2:
            attackType = ATTACK_BURST;
            break;
        case 3:
            attackType = ATTACK_FIRE;
            break;
        case 4:
            attackType = ATTACK_PHYSICAL;
            break;
        case 5:
            attackType = ATTACK_POISON;
            break;
        case 6:
            attackType = ATTACK_PARALYZE;
            break;
        case 7:
            attackType = ATTACK_DRUNKNESS;
            break;
        default:
            break;
        }
        action->game->CreateCondition(creature, NULL, animationColor, damageEffect, hitEffect, attackType, offensive, maxDamage, minDamage, ticks, count);
    }
    else
    {
        lua_pushnumber(L, -1);
        std::cout << "DoCreateCondition: creature not found" << std::endl;
        return 1;
    }
    return 1;
}

int32_t ActionScript::luaActionDoAddTeleport(lua_State *L)
{
    PositionEx frompos;
    internalGetPositionEx(L,frompos);

    PositionEx pos;
    internalGetPositionEx(L,pos);

    ActionScript *action = getActionScript(L);

    Tile *tile = action->game->map->getTile(pos);
    Teleport *checkTP = tile->getTeleportItem();

    if(checkTP)
        return 0;

    Item *newitem = Item::CreateItem(5023);
    action->game->addThing(NULL,pos,newitem);

    if(tile)
    {
        Teleport *teleportitem = tile->getTeleportItem();
        if(teleportitem)
            teleportitem->setDestPos(frompos);
    }

    lua_pushnumber(L, 0);
    return 1;
}

int32_t ActionScript::luaActionDoRemoveTeleport(lua_State *L)
{

    PositionEx pos;
    internalGetPositionEx(L,pos);

    ActionScript *action = getActionScript(L);

    Tile *tile = action->game->map->getTile(pos);
    if(tile)
    {
        Teleport *teleportitem = tile->getTeleportItem();
        if(teleportitem)
            action->game->removeThing(NULL , pos, teleportitem);
    }
    lua_pushnumber(L, 0);
    return 1;
}

int32_t ActionScript::luaActionGetTownIDByName(lua_State *L)
{
    //getTownIDByName(name)
    std::string name = internalGetString(L);

    for(Town::TownMap::iterator sit = Town::town.begin(); sit != Town::town.end(); ++sit)
    {
        if(sit->second.name == name)
        {
            lua_pushnumber(L, sit->first);
            break;
        }
    }
    lua_pushnumber(L, 0);
    return 1;
}

int32_t ActionScript::luaActionGetTownNameByID(lua_State *L)
{
    //getTownNameByID(id)
    uint32_t id = (uint32_t)internalGetNumber(L);

    for(Town::TownMap::iterator sit = Town::town.begin(); sit != Town::town.end(); ++sit)
    {
        if(sit->first == id)
        {
            std::string nazwa = sit->second.name;
            lua_pushstring(L, nazwa.c_str());
            break;
        }
    }
    lua_pushstring(L,"NULL");
    return 1;
}
int32_t ActionScript::luaActionDoSetItemUniqueID(lua_State *L)
{
    //doSetItemUniqueID(uid, new_uid)
    uint32_t new_uid = (uint32_t)internalGetNumber(L);
    uint32_t id = (uint32_t)internalGetNumber(L);

    ActionScript *action = getActionScript(L);

    const KnownThing* tmp = action->GetItemByUID(id);
    Item *tmpitem = NULL;
    PositionEx tmppos;
    if(tmp)
    {
        tmpitem = (Item*)tmp->thing;
    }
    else
    {
        lua_pushnumber(L, -1);
        std::cout << "luaDoSetActionId: Item not found" << std::endl;
        return 1;
    }
    tmpitem->setUniqueId(new_uid);

    lua_pushnumber(L, 1);
    return 1;
}
int32_t ActionScript::luaActionIsPlayer(lua_State *L)
{
    //isPlayer(cid)
    uint32_t cid = (uint32_t)internalGetNumber(L);
    ActionScript *action = getActionScript(L);
    lua_pushboolean(L, action->GetPlayerByUID(cid) ? true : false);
    return 1;
}

int32_t ActionScript::luaActionIsMonster(lua_State *L)
{
    //isMonster(cid)
    uint32_t cid = (uint32_t)internalGetNumber(L);
    ActionScript *action = getActionScript(L);
    lua_pushboolean(L, action->GetMonsterByUID(cid) ? true : false);
    return 1;
}

int32_t ActionScript::luaActionGetPlayerSlotItem(lua_State *L)
{
    //getSlotItem(cid,slot)
    int32_t slot = (int32_t)internalGetNumber(L);
    uint32_t cid = (uint32_t)internalGetNumber(L);

    ActionScript *action = getActionScript(L);
    const KnownThing* tmp = action->GetPlayerByUID(cid);
    Player *player = (Player*)(tmp->thing);

    if(player)
    {
        if(player->items[slot])
        {
            lua_pushnumber(L, player->items[slot]->getID());
        }
        else
        {
            lua_pushnumber(L, 0);
        }
    }
    else
    {
        std::cout << "luaGetPlayerSlotItem: Player not found" << std::endl;
        lua_pushnumber(L, -1);
    }
    return 1;
}

int32_t ActionScript::luaActionGetCreatureName(lua_State *L)
{
    //getCreatureName(cid)
    uint32_t cid = (unsigned int)internalGetNumber(L);

    ActionScript *action = getActionScript(L);

    const KnownThing* tmp = action->GetThingByUID(cid);
    if(tmp->type != thingTypeItem)
    {
        Creature *c = dynamic_cast<Creature*>(tmp->thing);
        if(c)
            lua_pushstring(L, c->getName().c_str());
        return 1;
    }
    else
    {
        lua_pushnumber(L, -1);
        std::cout << "getCreatureName: creature not found" << std::endl;
        return 1;
    }

    lua_pushnumber(L, 0);
    return 1;
}
