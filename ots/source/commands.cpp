#include <string>
#include <sstream>
#include <fstream>
#include <utility>

#include "commands.h"
#include "chat.h"
#include "player.h"
#include "npc.h"
#include "game.h"
#include "guilds.h"
#include "actions.h"
#include "map.h"
#include "status.h"
#include "monsters.h"
#include "town.h"
#include "tools.h"
#include "database.h"

extern std::vector< std::pair<uint32_t, uint32_t> > bannedIPs;
extern Actions actions;
extern Monsters g_monsters;
extern Chat g_chat;

//table of commands
s_defcommands Commands::defined_commands[] =
{
    {"/s",&Commands::placeNpc},
    {"/m",&Commands::placeMonster},
    {"/B",&Commands::broadcastMessage},
    {"/say",&Commands::RedBezNicka},
    {"/t",&Commands::teleportMasterPos},
    {"/c",&Commands::teleportHere},
    {"/i",&Commands::createItems},
    {"/reload",&Commands::reloadInfo},
    {"/goto",&Commands::teleportTo},
    {"/info",&Commands::getInfo},
    {"/closeserver",&Commands::closeServer},
    {"/openserver",&Commands::openServer},
    {"/a",&Commands::teleportNTiles},
    {"/kick",&Commands::kickPlayer},
    {"/namelock",&Commands::namelockPlayer},
    {"/male",&Commands::male},
    {"/female",&Commands::female},
    {"/dwarf",&Commands::dwarf},
    {"/nimfa",&Commands::nimfa},
    {"/up",&Commands::goUp},
    {"/down",&Commands::goDown},
    {"/pos",&Commands::showPos},
    {"/send",&Commands::teleportPlayerTo},
    {"/max",&Commands::setMaxPlayers},
    {"!exp",&Commands::showExpForLvl},
    {"!mana",&Commands::showManaForLvl},
    {"!online",&Commands::whoIsOnline},
    {"!uptime",&Commands::showUptime},
#ifdef TLM_HOUSE_SYSTEM
    {"/owner",&Commands::setHouseOwner},
    {"!house",&Commands::reloadRights},
#endif //TLM_HOUSE_SYSTEM
#ifdef TRS_GM_INVISIBLE
    {"/invisible",&Commands::gmInvisible},
#endif //TRS_GM_INVISIBLE
#ifdef HUCZU_SKULLS
    {"!frags",&Commands::PokazFragi},
    {"!rs",&Commands::PokazRs},
    {"!pz",&Commands::PokazPz},
    {"/noskull",&Commands::noskull},
#endif //HUCZU_SKULLS
    {"/save",&Commands::forceServerSave},
    {"/shutdown",&Commands::shutdown},
    {"/clean",&Commands::cleanMap},
#ifdef YUR_PREMIUM_PROMOTION
    {"/promote",&Commands::promote},
    {"/premmy",&Commands::premmy},
    {"!premmy",&Commands::showPremmy},
#endif //YUR_PREMIUM_PROMOTION
    {"/warn",&Commands::GadkaBDD},
    {"/opisz",&Commands::nowanazwaitema},
    {"/removeitem",&Commands::playerRemoveItem},
    {"/count", &Commands::countItem},
#ifdef HUCZU_ENFO
    {"!fragi", &Commands::countFrags},
#endif
    {"/cleanhouse", &Commands::cleanHouses},
    {"/tp", &Commands::teleporter},
#ifdef HUCZU_PAY_SYSTEM
    {"!doladuj", &Commands::doladowanie},
    {"!punkty", &Commands::showPunkty},
#endif
#ifdef RAID
    {"/raid",&Commands::doRaid},
#endif
    {"!createguild", &Commands::guildCreate},
    {"!joinguild", &Commands::guildJoin},
#ifdef HUCZU_HITS_KOMENDA
    {"!hits", &Commands::showHits},
#endif
};


Commands::Commands(Game* igame):
    game(igame),
    loaded(false)
{
    //setup command map
    for(int32_t i = 0; i< sizeof(defined_commands)/sizeof(defined_commands[0]); i++)
    {
        Command *tmp = new Command;
        tmp->loaded = false;
        tmp->accesslevel = 1;
        tmp->f = defined_commands[i].f;
        std::string key = defined_commands[i].name;
        commandMap[key] = tmp;
    }
}

bool Commands::loadFromDB()
{
    Database* db = Database::getInstance();
    DBQuery query;

    query << "SELECT `nazwa`, `access` FROM `komendy` WHERE `commands` = 1";
    DBResult* result;
    if((result = db->storeQuery(query.str())))
    {
        this->loaded = true;
        do
        {
            std::string cmd = result->getDataString("nazwa");
            CommandMap::iterator it = commandMap.find(cmd);
            if(it != commandMap.end())
            {
                int32_t alevel = result->getDataInt("access");
                if(!it->second->loaded)
                {
                    it->second->accesslevel = alevel;
                    it->second->loaded = true;
                }
            }
            else
                std::cout << "Brak komendy " << cmd << std::endl;
        }
        while(result->next());
        for(CommandMap::iterator it = commandMap.begin(); it != commandMap.end(); ++it)
        {
            if(it->second->loaded == false)
                std::cout << "Uwaga! Zapomniales ustawic access dla komendy " << it->first << std::endl;
        //register command tag in game
        game->addCommandTag(it->first.substr(0,1));
        }
    }
    return this->loaded;
}
/*bool Commands::loadXml(const std::string &_datadir)
{
    int32_t alevel;
    datadir = _datadir;

    std::string filename = datadir + "commands.xml";
    //std::transform(filename.begin(), filename.end(), filename.begin(), tolower);
    toLowerCaseString(filename);
    xmlDocPtr doc = xmlParseFile(filename.c_str());

    if (doc)
    {
        this->loaded = true;
        xmlNodePtr root, p;
        root = xmlDocGetRootElement(doc);

        if (xmlStrcmp(root->name,(const xmlChar*) "commands"))
        {
            xmlFreeDoc(doc);
            return false;
        }
        p = root->children;

        while (p)
        {
            const char* str = (char*)p->name;

            if (strcmp(str, "command") == 0)
            {
                char *tmp = (char*)xmlGetProp(p, (const xmlChar *) "cmd");
                if(tmp)
                {
                    CommandMap::iterator it = commandMap.find(tmp);

                    if(it != commandMap.end())
                    {
                        if(readXMLInteger(p,"access",alevel))
                        {
                            if(!it->second->loaded)
                            {
                                it->second->accesslevel = alevel;
                                it->second->loaded = true;
                            }
                            else
                            {
                                std::cout << "Podwojna komenda " << tmp << std::endl;
                            }
                        }
                        else
                        {
                            std::cout << "missing access tag for " << tmp << std::endl;
                        }
                    }
                    else
                    {
                        //error
                        std::cout << "Brak komendy " << tmp << std::endl;
                    }
                    xmlFreeOTSERV(tmp);
                }
                else
                {
                    std::cout << "missing cmd." << std::endl;
                }
            }
            p = p->next;
        }
        xmlFreeDoc(doc);
    }

    //
    for(CommandMap::iterator it = commandMap.begin(); it != commandMap.end(); ++it)
    {
        if(it->second->loaded == false)
        {
            std::cout << "Uwaga! Zapomniales ustawic access dla komendy " << it->first << std::endl;
        }
        //register command tag in game
        game->addCommandTag(it->first.substr(0,1));
    }


    return this->loaded;
}*/

bool Commands::reload()
{
    this->loaded = false;
    for(CommandMap::iterator it = commandMap.begin(); it != commandMap.end(); ++it)
    {
        it->second->accesslevel = 1;
        it->second->loaded = false;
    }
    game->resetCommandTag();
    //this->loadXml(datadir);
    this->loadFromDB();
    return true;
}

bool Commands::exeCommand(Creature *creature, const std::string &cmd)
{

    std::string str_command;
    std::string str_param;

    uint32_t loc = (uint32_t)cmd.find( ' ', 0 );
    if( loc != std::string::npos && loc >= 0)
    {
        str_command = std::string(cmd, 0, loc);
        str_param = std::string(cmd, (loc+1), cmd.size()-loc-1);
    }
    else
    {
        str_command = cmd;
        str_param = std::string("");
    }

    //find command
    CommandMap::iterator it = commandMap.find(str_command);
    if(it == commandMap.end())
    {
        return false;
    }
    Player *player = dynamic_cast<Player*>(creature);
    //check access for this command
    if(creature->access < it->second->accesslevel)
    {
        if(creature->access > 0)
        {
            if (player)
                player->sendTextMessage(MSG_SMALLINFO,"Nie mozesz uzyc tej komendy.");
            else
                std::cout << "UWAGA! " << creature->getName() << " nie posiada access do wykonania komendy " << cmd << '!' << std::endl;
            return true;
        }
        else
        {
            return false;
        }
    }
//execute command
    CommandFunc cfunc = it->second->f;
    (this->*cfunc)(creature, str_command, str_param);
    if(player)
        player->sendTextMessage(MSG_RED_TEXT,cmd.c_str());

    return true;
}

bool Commands::placeNpc(Creature* c, const std::string &cmd, const std::string &param)
{
    Npc *npc = new Npc(param, game);
    if(!npc->isLoaded())
    {
        delete npc;
        return true;
    }
    Position pos;
    // Set the NPC pos
    if(c->direction == NORTH)
    {
        pos.x = c->pos.x;
        pos.y = c->pos.y - 1;
        pos.z = c->pos.z;
    }
    // South
    if(c->direction == SOUTH)
    {
        pos.x = c->pos.x;
        pos.y = c->pos.y + 1;
        pos.z = c->pos.z;
    }
    // East
    if(c->direction == EAST)
    {
        pos.x = c->pos.x + 1;
        pos.y = c->pos.y;
        pos.z = c->pos.z;
    }
    // West
    if(c->direction == WEST)
    {
        pos.x = c->pos.x - 1;
        pos.y = c->pos.y;
        pos.z = c->pos.z;
    }
    // Place the npc
    if(!game->placeCreature(pos, npc))
    {
        delete npc;
        Player *player = dynamic_cast<Player*>(c);
        if(player)
        {
            player->sendMagicEffect(player->pos, NM_ME_PUFF);
            player->sendCancel("Przykro mi, nie ma miejsca.");
        }
        return true;
    }
    std::string text = c->getName() + " Postawil NPC: " + param;
    logAction(dynamic_cast<Player*>(c), 5, "NPC.txt", text);
    return true;
}

bool Commands::placeMonster(Creature* c, const std::string &cmd, const std::string &param)
{
    //Monster* monster = new Monster(param, game);
    Monster* monster = Monster::createMonster(param, game);
    //if(!monster->isLoaded()){
    if(!monster)
    {
        //delete monster;
        return true;
    }
    Position pos;

    // Set the Monster pos
    if(c->direction == NORTH)
    {
        pos.x = c->pos.x;
        pos.y = c->pos.y - 1;
        pos.z = c->pos.z;
    }
    // South
    if(c->direction == SOUTH)
    {
        pos.x = c->pos.x;
        pos.y = c->pos.y + 1;
        pos.z = c->pos.z;
    }
    // East
    if(c->direction == EAST)
    {
        pos.x = c->pos.x + 1;
        pos.y = c->pos.y;
        pos.z = c->pos.z;
    }
    // West
    if(c->direction == WEST)
    {
        pos.x = c->pos.x - 1;
        pos.y = c->pos.y;
        pos.z = c->pos.z;
    }

    // Place the monster
    if(!game->placeCreature(pos, monster))
    {
        delete monster;
        Player *player = dynamic_cast<Player*>(c);
        if(player)
        {
            player->sendMagicEffect(player->pos, NM_ME_PUFF);
            player->sendCancel("Za malo miejsca!");
        }
        return true;
    }
    else
    {
#ifdef FIXY
        bool canReach;
        Creature *target = monster->findTarget(0, canReach, c);
        if(target)
            monster->selectTarget(target, canReach);
#endif //FIXY
        std::string text = c->getName() + " Postawil Monstera: " + param;
        logAction(dynamic_cast<Player*>(c), 5, "POTWORY.txt", text);
        return true;
    }
}

bool Commands::broadcastMessage(Creature* c, const std::string &cmd, const std::string &param)
{
    game->creatureBroadcastMessage(c,param);
    return true;
}

bool Commands::RedBezNicka(Creature* c, const std::string &cmd, const std::string &param)
{
    std::string message = param.c_str();
    std::stringstream FM;
    FM << message.c_str() << std::endl;

    for(AutoList<Player>::listiterator it = Player::listPlayer.list.begin(); it != Player::listPlayer.list.end(); ++it)
    {
        if(dynamic_cast<Player*>(it->second))
            (*it).second->sendTextMessage(MSG_RED_INFO, FM.str().c_str());
    }
    return true;
}

bool Commands::teleportMasterPos(Creature* c, const std::string &cmd, const std::string &param)
{
    game->teleport(c, c->masterPos);
    return true;
}

bool Commands::teleportHere(Creature* c, const std::string &cmd, const std::string &param)
{
    Creature* creature = game->getCreatureByName(param);
    if(creature)
    {
        game->teleport(creature, c->pos);
    }
    return true;
}

bool Commands::createItems(Creature* c, const std::string &cmd, const std::string &param)
{
    uint32_t itemid, count;
    std::istringstream in(param.c_str());
    in >> itemid >> count;

    if(count == 0 || !count || count > 8214673)
        count = 1;

    if(count > 100)
        count = 100;

    if(!itemid || itemid < 100 || (itemid > 5089 && itemid < 20000) || itemid > 200028)
        itemid = 100;

    Item *newItem = Item::CreateItem(itemid, count);
    if(!newItem)
        return true;

    Player *player = dynamic_cast<Player *>(c);
    if(!player ||	Item::items[itemid].isSplash() ||
            Item::items[itemid].isGroundTile() ||
            Item::items[itemid].isTeleport() ||
            Item::items[itemid].isMagicField() ||
            Item::items[itemid].moveable == false ||
            Item::items[itemid].pickupable == false)
    {
        newItem->pos = c->pos;
        Tile *t = game->map->getTile(c->pos);
        if(!t)
        {
            delete newItem;
            return true;
        }

        game->addThing(NULL,c->pos,newItem);
    }

    else if(player)
        player->TLMaddItem(itemid, count);
    std::string text = c->getName() + " zrobil item: " + param;
    logAction(dynamic_cast<Player*>(c), 5, "ITEMS.txt", text);
    return true;
}

bool Commands::reloadInfo(Creature* c, const std::string &cmd, const std::string &param)
{
    if(param == "actions")
    {
        actions.reload();
    }
    else if(param == "commands")
    {
        this->reload();
    }
    else if(param == "monsters")
    {
        g_monsters.reload();
    }
    else if(param == "config")
    {
        g_config.OpenFile("config.lua");
    }
    else
    {
        Player *player = dynamic_cast<Player*>(c);
        if(player)
            player->sendCancel("Opcja nie znaleziona.");
    }

    return true;
}

bool Commands::teleportTo(Creature* c, const std::string &cmd, const std::string &param)
{
    Creature* creature = game->getCreatureByName(param);
    if(creature)
    {
#ifdef TRS_GM_INVISIBLE
        Position pos = creature->pos;
        pos.x++;
        pos.y++;
        game->teleport(c, pos);
#else
        game->teleport(c, creature->pos);
#endif //TRS_GM_INVISIBLE
    }
    else	// teleport to position
    {
        std::istringstream in(param.c_str());
        Position pos;
        in >> pos.x >> pos.y >> pos.z;

        if (in)
            game->teleport(c, pos);
    }
    return true;
}

bool Commands::getInfo(Creature* c, const std::string &cmd, const std::string &param)
{
    Player *player = dynamic_cast<Player*>(c);
    if(!player)
        return true;

    Player* paramPlayer = game->getPlayerByName(param);
    if(paramPlayer)
    {
        std::stringstream info;
        unsigned char ip[4];
        if(paramPlayer->access >= player->access && player != paramPlayer)
        {
            player->sendTextMessage(MSG_BLUE_TEXT,"Nie mozesz wyciagnac info z tego gracza.");
            return true;
        }
        *(uint32_t*)&ip = paramPlayer->lastip;
        info << "Nick:   " << paramPlayer->getName() << std::endl <<
        "Account: " << paramPlayer->getAccount()<< std::endl <<
        "Access: " << paramPlayer->access << std::endl <<
        "Poziom:  " << paramPlayer->getPlayerInfo(PLAYERINFO_LEVEL) << std::endl <<
        "Magpoziom: " << paramPlayer->getPlayerInfo(PLAYERINFO_MAGICLEVEL) << std::endl <<
        "Money: " << paramPlayer->getMoney()/1000000 << "sc" << std::endl <<
        "Pozycja:" << paramPlayer->pos << std::endl <<
        "IP: " << (uint32_t)ip[0] << "." << (uint32_t)ip[1] <<
        "." << (uint32_t)ip[2] << "." << (uint32_t)ip[3];
        player->sendTextMessage(MSG_BLUE_TEXT,info.str().c_str());
    }
    else
    {
        player->sendTextMessage(MSG_BLUE_TEXT,"Gracz nie znaleziony.");
    }

    return true;
}


bool Commands::closeServer(Creature* c, const std::string &cmd, const std::string &param)
{
    game->setGameState(GAME_STATE_CLOSED);
    //kick players with access = 0
    std::stringstream FM;
    FM << "Serwer zostal zamkniety!";
    AutoList<Player>::listiterator it = Player::listPlayer.list.begin();
    while(it != Player::listPlayer.list.end())
    {
        if((*it).second->access == 0)
        {
            (*it).second->kickPlayer();
            it = Player::listPlayer.list.begin();
        }
        else
        {
            (*it).second->sendTextMessage(MSG_RED_INFO, FM.str().c_str());
            ++it;
        }
    }

    return true;
}

bool Commands::openServer(Creature* c, const std::string &cmd, const std::string &param)
{
    game->setGameState(GAME_STATE_NORMAL);
    std::stringstream FM;
    FM << "Serwer zostal otwarty!";
    for(AutoList<Player>::listiterator it = Player::listPlayer.list.begin(); it != Player::listPlayer.list.end(); ++it)
    {
        if(dynamic_cast<Player*>(it->second))
            (*it).second->sendTextMessage(MSG_RED_INFO, FM.str().c_str());
    }
    return true;
}

bool Commands::teleportNTiles(Creature* c, const std::string &cmd, const std::string &param)
{

    int32_t ntiles = atoi(param.c_str());
    if(ntiles != 0)
    {
        Position new_pos;
        new_pos = c->pos;
        switch(c->direction)
        {
        case NORTH:
            new_pos.y = new_pos.y - ntiles;
            break;
        case SOUTH:
            new_pos.y = new_pos.y + ntiles;
            break;
        case EAST:
            new_pos.x = new_pos.x + ntiles;
            break;
        case WEST:
            new_pos.x = new_pos.x - ntiles;
            break;
        }
        game->teleport(c, new_pos);
    }
    return true;
}

bool Commands::kickPlayer(Creature* c, const std::string &cmd, const std::string &param)
{
    Player* playerKick = game->getPlayerByName(param);
    if(playerKick)
    {
        Player* player = dynamic_cast<Player*>(c);
        if(player && player->access <= playerKick->access)
        {
            player->sendTextMessage(MSG_BLUE_TEXT,"Nie mozesz kicknac tego gracza.");
            return true;
        }
        playerKick->kickPlayer();
        return true;
    }
    return false;
}

bool Commands::goUp(Creature* c, const std::string &cmd, const std::string &param)
{
    Position pos = c->pos;
    pos.z--;
    game->teleport(c, pos);
    return true;
}

bool Commands::goDown(Creature* c, const std::string &cmd, const std::string &param)
{
    Position pos = c->pos;
    pos.z++;
    game->teleport(c, pos);
    return true;
}
bool Commands::showExpForLvl(Creature* c, const std::string &cmd, const std::string &param)
{
    Player* player = dynamic_cast<Player*>(c);
    if (player->mmo > 0)
    {
        player->sendMagicEffect(player->pos, NM_ME_PUFF);
        player->sendTextMessage(MSG_SMALLINFO, "Poczekaj 5 sekund miedzy kolejna proba.");
    }
    if (player && player->mmo == 0 )
    {
        char buf[128];
        sprintf(buf,"%lld",player->getExpForNextLevel());

        std::string msg = std::string("Potrzebujesz ") + std::string(buf) + std::string(" exp do kolejnego poziomu.");
        player->sendTextMessage(MSG_BLUE_TEXT, msg.c_str());
        player->mmo += 5;
    }
    return true;
}

bool Commands::showManaForLvl(Creature* c, const std::string &cmd, const std::string &param)
{
    Player* player = dynamic_cast<Player*>(c);
    if (player)
    {
        if (player->mmo > 0)
        {
            player->sendMagicEffect(player->pos, NM_ME_PUFF);
            player->sendTextMessage(MSG_SMALLINFO, "Poczekaj 5 sekund miedzy kolejna proba.");
            return false;
        }
        else
        {
            char buf[128];
            sprintf(buf,"%ld",(int32_t)player->getManaForNextMLevel());
            std::string msg = std::string("Potrzebujesz spalic ") + std::string(buf) + std::string(" many do kolejnego poziomu.");
            player->sendTextMessage(MSG_BLUE_TEXT, msg.c_str());
            player->mmo += 5;
            return true;
        }
    }
    return false;
}

bool Commands::whoIsOnline(Creature* c, const std::string &cmd, const std::string &param)
{
    Player* player = dynamic_cast<Player*>(c);
    if (player->mmo > 0)
    {
        player->sendMagicEffect(player->pos, NM_ME_PUFF);
        player->sendTextMessage(MSG_SMALLINFO, "Poczekaj 5 sekund miedzy kolejna proba.");
    }
    if (player->mmo == 0 )
    {
        uint32_t alevelmin = 0;
        uint32_t alevelmax = 5;
        int32_t i,n;
        if(!player)
            return false;

        if(param == "gm")
            alevelmin = 1;
        else if(param == "normal")
            alevelmax = 0;

        std::stringstream players;
        players << "Gracze zalogowani: " << std::endl;

        i = 0;
        n = 0;
        AutoList<Player>::listiterator it = Player::listPlayer.list.begin();
        for(; it != Player::listPlayer.list.end(); ++it)
        {
            if((*it).second->access >= (int32_t)alevelmin && (*it).second->access <= (int32_t)alevelmax)
            {
                players << (*it).second->getName() << ", ";
                n++;
                i++;
            }
            if(i == 10)
            {
                player->sendTextMessage(MSG_BLUE_TEXT,players.str().c_str());
                players.str("");
                i = 0;
            }
        }
        if(i != 0)
            player->sendTextMessage(MSG_BLUE_TEXT,players.str().c_str());

        players.str("");
        players << "Wszystkich: " << n << "." << std::endl;
        player->sendTextMessage(MSG_BLUE_TEXT,players.str().c_str());
        player->mmo += 5;
        return true;
    }
    return false;
}

bool Commands::teleportPlayerTo(Creature* c, const std::string &cmd, const std::string &param)
{
    Position pos;
    std::string name;
    std::istringstream in(param.c_str());

    std::getline(in, name, ',');
    in >> pos.x >> pos.y >> pos.z;

    Creature* creature = game->getCreatureByName(name);
    Player* player = dynamic_cast<Player*>(creature);

    if (player)
        game->teleport(player, pos);

    return true;
}

bool Commands::showPos(Creature* c, const std::string &cmd, const std::string &param)
{
    Player* player = dynamic_cast<Player*>(c);
    if (player)
    {
        if(player->mmo > 0)
        {
            player->sendMagicEffect(player->pos, NM_ME_PUFF);
            player->sendTextMessage(MSG_SMALLINFO, "Poczekaj 5 sekund miedzy kolejna proba.");
        }
        else
        {
            std::stringstream msg;
            msg << "Twoja pozycja to: " << player->pos.x << ' ' << player->pos.y << ' ' << player->pos.z << std::ends;
            player->sendTextMessage(MSG_BLUE_TEXT, msg.str().c_str());
            player->mmo += 5;
            return true;
        }

    }
    return false;
}

bool Commands::showUptime(Creature* c, const std::string &cmd, const std::string &param)
{
    Player* player = dynamic_cast<Player*>(c);
    if (player)
    {
        uint64_t uptime = (OTSYS_TIME() - Status::instance()->start)/1000;
        int32_t h = (int32_t)floor(uptime / 3600.0);
        int32_t m = (int32_t)floor((uptime - h*3600) / 60.0);
        int32_t s = (int32_t)(uptime - h*3600 - m*60);

        std::stringstream msg;
        msg << "Server has been running for " << h << (h != 1? " hours " : " hour ") <<
        m << (m != 1? " minutes " : " minute ") << s << (s != 1? " seconds. " : " second.") << std::ends;

        player->sendTextMessage(MSG_BLUE_TEXT, msg.str().c_str());
    }
    return true;
}

bool Commands::setMaxPlayers(Creature* c, const std::string &cmd, const std::string &param)
{
    if (!param.empty())
    {
        int32_t newmax = atoi(param.c_str());
        if (newmax > 0)
        {
            game->setMaxPlayers(newmax);

            Player* player = dynamic_cast<Player*>(c);
            if (player)
                player->sendTextMessage(MSG_BLUE_TEXT, (std::string("Limit graczy wynosi: ")+param).c_str());
        }
    }
    return true;
}


#ifdef TLM_HOUSE_SYSTEM
bool Commands::reloadRights(Creature* c, const std::string &cmd, const std::string &param)
{
    Player* player = dynamic_cast<Player*>(c);

    if (player)
    {
        player->houseRightsChanged = true;
        player->sendTextMessage(MSG_BLUE_TEXT, "House rights reloaded.");
    }

    return true;
}

bool Commands::setHouseOwner(Creature* c, const std::string &cmd, const std::string &param)
{
    Tile* tile = game->getTile(c->pos);
    House* house = tile? tile->getHouse() : NULL;

    if (house)
    {
        Creature* creature = game->getCreatureByName(house->getOwner());
        Player* prevOwner = creature? dynamic_cast<Player*>(creature) : NULL;

        house->setOwner(param);

        creature = game->getCreatureByName(param);
        Player* newOwner = creature? dynamic_cast<Player*>(creature) : NULL;

        if (prevOwner)
            prevOwner->houseRightsChanged = true;
        if (newOwner)
            newOwner->houseRightsChanged = true;
    }
    return true;
}
#endif //TLM_HOUSE_SYSTEM


#ifdef TRS_GM_INVISIBLE
bool Commands::gmInvisible(Creature* c, const std::string &cmd, const std::string &param)
{
    Player *player = dynamic_cast<Player*>(c);

    if (!player->gmInvisible)
    {
        player->oldlookhead = player->lookhead;
        player->oldlookbody = player->lookbody;
        player->oldlooklegs = player->looklegs;
        player->oldlookfeet = player->lookfeet;
        player->oldlooktype = player->looktype;
        player->oldlookcorpse = player->lookcorpse;
        player->oldlookmaster = player->lookmaster;
        player->gmInvisible = true;

        Tile* tile = game->getTile(player->pos.x,player->pos.y,player->pos.z);
        SpectatorVec list;
        game->getSpectators(Range(player->pos, true), list);
        int32_t osp = tile->getThingStackPos(player);

        for(SpectatorVec::iterator it = list.begin(); it != list.end(); ++it)
        {
            if((*it) != player && (*it)->access == 0)
                (*it)->onCreatureDisappear(player, osp, true);
        }

        player->sendTextMessage(MSG_INFO, "Jestes niewidoczny.");
        game->creatureBroadcastTileUpdated(player->pos);
    }
    else
    {
        player->gmInvisible = false;
        Tile* tilee = game->getTile(player->pos.x,player->pos.y,player->pos.z);

        int32_t osp = tilee->getThingStackPos(player);
        SpectatorVec list;
        game->getSpectators(Range(player->pos, true), list);

        for(SpectatorVec::iterator it = list.begin(); it != list.end(); ++it)
            game->creatureBroadcastTileUpdated(player->pos);

        game->creatureChangeOutfit(player);
        player->sendTextMessage(MSG_INFO, "Jestes widzialny.");
    }

    return true;
}
#endif //TRS_GM_INVISIBLE


#ifdef HUCZU_SKULLS
bool Commands::noskull(Creature* c, const std::string &cmd, const std::string &param)
{
    Creature* creature = game->getCreatureByName(param.c_str());
    if (creature->skullType = SKULL_RED)
        creature->skullType = SKULL_NONE;

    return true;
}

bool Commands::PokazFragi(Creature* c, const std::string &cmd, const std::string &param)
{
    Player* player = dynamic_cast<Player*>(c);
    if (player)
    {
        if(player->mmo > 0)
        {
            player->sendMagicEffect(player->pos, NM_ME_PUFF);
            player->sendTextMessage(MSG_SMALLINFO, "Poczekaj 5 sekund miedzy kolejna proba.");
        }
        else
        {
            std::ostringstream info;
            info << "You have " << player->skullKills
            << " unjustified kills. You will lose a frag in " << tickstr(player->absolveTicks) << ".";
            player->sendTextMessage(MSG_BLUE_TEXT, info.str().c_str());
            player->mmo += 5;
            return true;
        }
    }
    return false;
}

bool Commands::PokazRs(Creature* c, const std::string &cmd, const std::string &param)
{
    Player* player = dynamic_cast<Player*>(c);
    if(player)
    {
        if(player->mmo > 0)
        {
            player->sendMagicEffect(player->pos, NM_ME_PUFF);
            player->sendTextMessage(MSG_SMALLINFO, "Poczekaj 5 sekund miedzy kolejna proba.");
        }
        else
        {
            if (player->skullType == SKULL_RED)
            {
                std::ostringstream info;
                info << "Redskull zniknie za " << tickstr(player->skullTicks) << ".";
                player->sendTextMessage(MSG_BLUE_TEXT, info.str().c_str());
            }
            else
            {
                std::ostringstream info;
                info << "Nie masz Red Skull'a.";
                player->sendTextMessage(MSG_BLUE_TEXT, info.str().c_str());
            }
            player->mmo += 5;
            return true;
        }
    }
    return false;
}

bool Commands::PokazPz(Creature* c, const std::string &cmd, const std::string &param)
{
    Player* player = dynamic_cast<Player*>(c);
    if(player)
    {
        if(player->mmo > 0)
        {
            player->sendMagicEffect(player->pos, NM_ME_PUFF);
            player->sendTextMessage(MSG_SMALLINFO, "Poczekaj 5 sekund miedzy kolejna proba.");
        }
        else
        {
            if (player->inFightTicks != 0)
            {
                std::ostringstream info;
                info << "PZ zniknie za " << tickstr(player->inFightTicks) << ".";
                player->sendTextMessage(MSG_BLUE_TEXT, info.str().c_str());
            }
            else
            {
                std::ostringstream info;
                info << "Nie masz PZ!";
                player->sendTextMessage(MSG_BLUE_TEXT, info.str().c_str());
            }
            player->mmo += 5;
            return true;
        }
    }
    return false;
}
#endif //HUCZU_SKULLS

bool Commands::forceServerSave(Creature* c, const std::string &cmd, const std::string &param)
{
    Player* player = dynamic_cast<Player*>(c);
    if (player)
    {
        game->serverSave();
        player->sendTextMessage(MSG_BLUE_TEXT, "Zapisywanie serwera wykonane.");
    }
    return true;
}

bool Commands::shutdown(Creature* c, const std::string &cmd, const std::string &param)
{
    Player* player = dynamic_cast<Player*>(c);
    if (player && !param.empty())
        game->sheduleShutdown(atoi(param.c_str()));
    return true;
}

bool Commands::cleanMap(Creature* c, const std::string &cmd, const std::string &param)
{
    std::ostringstream info;
    Player* player = dynamic_cast<Player*>(c);

    if (player)
    {
        std::cout << ":: clean... ";

        timer();
        int32_t count = game->cleanMap();
        double sec = timer();

        info << "Clean completed. Zebrano " << count << (count==1? " item." : " items.") << std::ends;
        player->sendTextMessage(MSG_BLUE_TEXT, info.str().c_str());

        std::cout << "[" << sec << " s]\n";
    }
    return true;
}

#ifdef YUR_PREMIUM_PROMOTION
bool Commands::premmy(Creature* c, const std::string &cmd, const std::string &param)
{
    Player* player = dynamic_cast<Player*>(c);
    std::string tickTack = param.c_str();
    std::string pacct;

    for(int32_t a = 0; a<param.length(); ++a)
    {
        if(!isdigit(param[a]))
        {
            pacct = param;
            pacct.erase(a,1-param.length());
            tickTack.erase(0,1+a);
            break;
        }
        else
            pacct = param.c_str();
    }
    uint32_t newPacc = atoi(pacct.c_str());
    if(newPacc <= 0 || newPacc >= 500)
        return false;

    if(Player* toChange = game->getPlayerByName(tickTack))
    {
        if(toChange->premiumTicks >= 1800000001)
            return false;

        toChange->premiumTicks += 1000*60*60*newPacc;
        return true;
    }
    else
        return false;

    return false;
}

bool Commands::showPremmy(Creature* c, const std::string &cmd, const std::string &param)
{
    Player* player = dynamic_cast<Player*>(c);
    if (player)
    {
        if(player->mmo > 0)
        {
            player->sendMagicEffect(player->pos, NM_ME_PUFF);
            player->sendTextMessage(MSG_SMALLINFO, "Poczekaj 5 sekund miedzy kolejna proba.");
        }
        else
        {
            std::ostringstream info;
            if (g_config.FREE_PREMMY)
                info << "Posiadasz nielimitowane Premium." << std::ends;
            else
                info << "Posiadasz " << tickstr(player->premiumTicks) << " PACC." << std::ends;
            player->sendTextMessage(MSG_BLUE_TEXT, info.str().c_str());
            player->mmo += 5;
            return true;
        }
    }
    return false;
}

bool Commands::promote(Creature* c, const std::string &cmd, const std::string &param)
{
    Creature* creature = game->getCreatureByName(param);
    Player* target = dynamic_cast<Player*>(creature);

    if (target)
        target->promote();

    return true;
}
#endif //YUR_PREMIUM_PROMOTION

bool Commands::male(Creature* c, const std::string &cmd, const std::string &param)
{
    Creature* creature = game->getCreatureByName(param);
    if(!creature)
        return false;
    Player* target = dynamic_cast<Player*>(creature);

    if (target->sex != PLAYERSEX_MALE)
    {
        target->sex = PLAYERSEX_MALE;
        target->sendMagicEffect(target->pos, NM_ME_MAGIC_ENERGIE);
        target->sendTextMessage(MSG_RED_INFO, "Gratulacje, zmiana plci zakonczyla sie sukcesem. Od teraz jestes mezczyzna.");
    }
    else
    {
        target->sendMagicEffect(target->pos, NM_ME_PUFF);
        target->sendTextMessage(MSG_RED_INFO, "Jestes juz mezczyzna! Nie mozesz zmienic plci na ta sama.");
    }

    return true;
}

bool Commands::female(Creature* c, const std::string &cmd, const std::string &param)
{
    Creature* creature = game->getCreatureByName(param);
    if(!creature)
        return false;
    Player* target = dynamic_cast<Player*>(creature);

    if (target->sex != PLAYERSEX_FEMALE)
    {
        target->sex = PLAYERSEX_FEMALE;
        target->sendMagicEffect(target->pos, NM_ME_MAGIC_ENERGIE);
        target->sendTextMessage(MSG_RED_INFO, "Gratulacje, zmiana plci zakonczyla sie sukcesem. Od teraz jestes kobieta.");
    }
    else
    {
        target->sendMagicEffect(target->pos, NM_ME_PUFF);
        target->sendTextMessage(MSG_RED_INFO, "Jestes juz kobieta! Nie mozesz zmienic plci na ta sama.");
    }
    return true;
}

bool Commands::dwarf(Creature* c, const std::string &cmd, const std::string &param)
{
    Player* player = dynamic_cast<Player*>(c);
    Creature* creature = game->getCreatureByName(param);
    if(!creature)
        return true;
    Player* target = dynamic_cast<Player*>(creature);

    if (target->sex != PLAYERSEX_DWARF)
    {
        target->sex = PLAYERSEX_DWARF;
        target->sendMagicEffect(target->pos, NM_ME_MAGIC_ENERGIE);
        target->sendTextMessage(MSG_RED_INFO, "Gratulacje, zmiana plci zakonczyla sie sukcesem. Od teraz jestes krasnoludem.");
    }
    else
    {
        target->sendMagicEffect(target->pos, NM_ME_PUFF);
        target->sendTextMessage(MSG_RED_INFO, "Jestes juz krasnoludem! Nie mozesz zmienic plci na ta sama.");
    }
    return true;

}

bool Commands::nimfa(Creature* c, const std::string &cmd, const std::string &param)
{
    Creature* creature = game->getCreatureByName(param);
    if(!creature)
        return false;
    Player* target = dynamic_cast<Player*>(creature);

    if (target->sex != PLAYERSEX_NIMFA)
    {
        target->sex = PLAYERSEX_NIMFA;
        target->sendMagicEffect(target->pos, NM_ME_MAGIC_ENERGIE);
        target->sendTextMessage(MSG_RED_INFO, "Gratulacje, zmiana plci zakonczyla sie sukcesem. Od teraz jestes nimfa.");
    }
    else
    {
        target->sendMagicEffect(target->pos, NM_ME_PUFF);
        target->sendTextMessage(MSG_RED_INFO, "Jestes juz nimfa! Nie mozesz zmienic plci na ta sama.");
    }
    return true;
}


bool Commands::namelockPlayer(Creature* c, const std::string &cmd, const std::string &param)
{
    Player *paramplayer = game->getPlayerByName(param);

    if(paramplayer)
    {
        paramplayer->namelock = 1;
        paramplayer->sendTextMessage(MSG_INFO,"Dostales namelocka!");
        paramplayer->kickPlayer();
        std::string text = c->getName() + " dal namelock: " + param;
        logAction(dynamic_cast<Player*>(c), 5, "NAMELOCK.txt", text);
        return true;
    }
    else
        return false;
}

bool Commands::GadkaBDD(Creature* c, const std::string &cmd, const std::string &param)
{
    std::string tmp = param;
    std::string::size_type pos;
    std::string message;

    pos = tmp.find(",");
    std::string name = tmp.substr(0, pos).c_str();
    tmp.erase(0, pos+1);

    message = tmp;

    Creature* creature = game->getCreatureByName(name);
    Player* cel = creature? dynamic_cast<Player*>(creature) : NULL;

    if(cel)
    {
        std::string text = message.c_str();
        std::stringstream wiadomosc;
        wiadomosc << message.c_str() << std::endl;
        cel->sendTextMessage(MSG_RED_INFO, wiadomosc.str().c_str());
        return true;
    }
    return false;
}

void Commands::logAction(Player *p, int32_t maxaccess, std::string filename, std::string text)
{
    if(p && p->access <= maxaccess)
    {
        std::stringstream file;
        time_t now = std::time(NULL);
        std::string time = ctime(&now);
        std::string datadir = g_config.DATA_DIR;
        file << datadir << "logs/" << filename;
#ifdef USING_VISUAL_2005
        FILE* f = NULL;
        fopen_s(&f, file.str().c_str(), "a");
#else
        FILE* f = fopen(file.str().c_str(), "a");
#endif //USING_VISUAL_2005
        if(f)
        {
            fputs(time.c_str(), f);
            fputs(text.c_str(), f);
            fputs("\n", f);
            fclose(f);
        }
    }
}

bool Commands::nowanazwaitema(Creature* c, const std::string &cmd, const std::string &param)
{
    Player *player = dynamic_cast<Player*>(c);
    Item* przedmiot = player->getItem(SLOT_LEFT);
    if(przedmiot)
    {
        przedmiot->setSpecialDescription(param);
    }
    else
    {
        player->sendTextMessage(MSG_BLUE_TEXT,"Aby dodac nowy opis (tekst) przedmiotowi:\n wlóz go do lewej reki,\n i napisz: /opisz tekst\nAby zresetowac nazwe na orginalna, napisz tylko: /opisz");
    }
    return true;
}

bool Commands::playerRemoveItem(Creature* c, const std::string &cmd, const std::string &param)
{
    Player *player = dynamic_cast<Player*>(c);
    std::string name;
    int32_t itemid;
    std::stringstream info;

    std::istringstream in(param.c_str());
    std::getline(in, name, ',');
    in >> itemid;
    Player* paramPlayer = game->getPlayerByName(name);
    if(!paramPlayer)
    {
        player->sendTextMessage(MSG_SMALLINFO,"Gracz nie istnieje.");
        return true;
    }
    if( c->access > paramPlayer->access &&
            paramPlayer->removeItemSmart(itemid, 100, true))
        info << "Przedmiot " << Item::items[itemid].name << " usuniety od gracza " << name << ".";
    else
        info << "Nie mozna usunac " << Item::items[itemid].name << " od gracza " << name << ".";
    if(player)
        player->sendTextMessage(MSG_BLUE_TEXT,info.str().c_str());
#ifdef __DEBUG__
    std::cout << info << std::endl;
#endif
    return true;
}

bool Commands::countItem(Creature* c, const std::string& cmd, const std::string& param)
{
    Player *player = dynamic_cast<Player*>(c);
    std::stringstream info;
    std::string name;
    uint16_t itemid;
    std::istringstream in(param.c_str());
    std::getline(in, name, ',');
    in >> itemid;
    Player* paramPlayer = game->getPlayerByName(name);
    if(!paramPlayer)
    {
        player->sendTextMessage(MSG_SMALLINFO,"Gracz nie istnieje.");
        return true;
    }
    info << paramPlayer->getName() << " posiada " << paramPlayer->countItem(itemid) << " sztuk przedmiotu o id " << itemid << ".";
    player->sendTextMessage(MSG_RED_TEXT, info.str().c_str());
    return true;
}
#ifdef HUCZU_ENFO
bool Commands::countFrags(Creature* c, const std::string& cmd, const std::string& param)
{
    Player *player = dynamic_cast<Player*>(c);
    std::stringstream info;
    if(player)
    {
        info << "Posiadasz " << player->frags << " fragow.";
        player->sendTextMessage(MSG_RED_TEXT, info.str().c_str());
    }
    return true;
}
#endif
bool Commands::cleanHouses(Creature* c, const std::string& cmd, const std::string& param)
{
    Player* player = dynamic_cast<Player*>(c);
    std::stringstream info;
    uint32_t days = atoi(param.c_str());
    int32_t count = game->checkHouseOwners(days);
    if(player)
    {
        info << "Clean domkow zakonczony. Oczyszczono" << count << "domkow.";
        player->sendTextMessage(MSG_RED_TEXT, info.str().c_str());
    }
}

bool Commands::teleporter(Creature* c, const std::string &cmd, const std::string &param)
{
    Player *player = dynamic_cast<Player*>(c);
    std::stringstream info;
    bool found = false;
    if(player)
    {
        for(Town::TownMap::iterator sit = Town::town.begin(); sit != Town::town.end(); ++sit)
        {
            if(sit->second.name == param)
            {
                game->teleport(player, sit->second.pos);
                found = true;
                info << "Witamy w " << sit->second.name << ".";
                break;
            }
        }
        if(!found)
            info << "Nie znaleziono miasta.";

        player->sendTextMessage(MSG_RED_TEXT, info.str().c_str());
    }
    return true;
}

#ifdef HUCZU_PAY_SYSTEM
bool Commands::doladowanie(Creature* c, const std::string &cmd, const std::string &param)
{
    Player *player = dynamic_cast<Player*>(c);
    uint32_t ilosc_punktow = 0;
    std::stringstream informacja;
    std::string kod;
    if(g_config.PAY_SYSTEM)
    {
        if(param == "")
        {
            informacja << "By doladowac konto punktami wyslij SMS o tresci HPAY.BOTS na numer 7955(Koszt 11.07zl brutto). Otrzymany kod wpisz !doladuj 1,KOD i dodaladujesz konto 100 punktami.";
            informacja << "\nLub wyslij SMS o tresci HPAY.BOTS2 na numer 91955(Koszt 23.37zl brutto). Otrzymany kod wpisz !doladuj 2,KOD i doladujesz konto 250 punktami.";
            player->sendTextMessage(MSG_RED_TEXT, informacja.str().c_str());
            return true;
        }
        std::string tmp = param;
        std::string::size_type pos;
        pos = tmp.find(",");
        std::string usluga = tmp.substr(0, pos).c_str();
        tmp.erase(0, pos+1);
        kod = tmp;
        if(kod != "" && kod.length() > 8)
        {
            informacja << "Twoj kod jest za dlugi!";
            player->sendTextMessage(MSG_RED_TEXT, informacja.str().c_str());
            return true;
        }
        if(usluga != "")
        {
            int32_t id_uslugi = atoi(usluga.c_str());
            switch(id_uslugi)
            {
                case 1:
                    usluga = "2096";
                    ilosc_punktow = 30;
                    break;
                case 2:
                    usluga = "2097";
                    ilosc_punktow = 70;
                    break;
                case 3:
                    usluga = "2098";
                    ilosc_punktow = 120;
                    break;
                case 4:
                    usluga = "2099";
                    ilosc_punktow = 250;
                    break;
                case 5:
                    usluga = "2100";
                    ilosc_punktow = 350;
                    break;
            }
        }
        if(player)
        {
            if(game->checkHomepay(player, kod, usluga))
            {
                player->addPunkty(ilosc_punktow);
                game->logPunktow(player, kod, usluga);
                informacja << "Dodano " << ilosc_punktow << " punktow!";
                player->sendTextMessage(MSG_INFO, informacja.str().c_str());
                return true;
            }
            else
            {
                player->sendTextMessage(MSG_RED_TEXT, "Nie poprawny kod lub usluga.");
                return false;
            }
        }
    }
    else
    {
        informacja << "SMS Shop w grze wylaczony. Kup na stronie: http://varden76.pl/index.php?subtopic=buypoints";
        player->sendTextMessage(MSG_RED_TEXT, informacja.str().c_str());
    }
    return false;
}

bool Commands::showPunkty(Creature* c, const std::string &cmd, const std::string &param)
{
    Player *player = dynamic_cast<Player*>(c);
    std::stringstream informacja;
    if(player)
    {
        informacja << "Posiadasz " << player->getPunkty() << " punktow.";
        player->sendTextMessage(MSG_RED_TEXT, informacja.str().c_str());
        return true;
    }
    return false;
}
#endif

#ifdef RAID
bool Commands::doRaid(Creature* c, const std::string &cmd, const std::string &param)
{
    game->loadRaid(param);
    return true;
}
#endif
bool Commands::guildJoin(Creature* c, const std::string &cmd, const std::string &param)
{
    Player* player = dynamic_cast<Player*>(c);
    if(!player)
        return false;

    std::string param_ = param;
    trimString(param_);
    if(!player->getGuildId())
    {
        uint32_t guildId;
        if(Guild::getInstance()->getGuildId(guildId, param_))
        {
            if(player->isGuildInvited(guildId))
            {
                Guild::getInstance()->joinGuild(player, guildId);
                player->sendTextMessage(MSG_INFO, "Dolaczyles do gildii.");

                char buffer[80];
                sprintf(buffer, "%s dolaczyl do gildii.", player->getName().c_str());
                if(ChatChannel* guildChannel = g_chat.getChannel(player, 0x00))
                    guildChannel->talk(player, SPEAK_CHANNEL_R1, buffer, 0x00);
            }
            else
                player->sendCancel("Nie jestes zaproszony do tej gildii.");
        }
        else
            player->sendCancel("Nie ma gildii z ta nazwa.");
    }
    else
        player->sendCancel("Jestes juz w gildii.");

    return true;
}

bool Commands::guildCreate(Creature* c, const std::string &cmd, const std::string &param)
{
    Player* player = dynamic_cast<Player*>(c);
    if(!player)
        return false;

    if(player->getGuildId())
    {
        player->sendCancel("Jestes juz w gildii.");
        return true;
    }

    std::string param_ = param;
    trimString(param_);
    if(!isValidName(param_))
    {
        player->sendCancel("Nazwa gildii zawiera niepoprawne znaki, wybierz inna nazwe.");
        return true;
    }

    const uint32_t minLength = 3;
    const uint32_t maxLength = 30;
    if(param_.length() < minLength)
    {
        player->sendCancel("Nazwa gildii jest za krotka, wybierz dluzsza.");
        return true;
    }

    if(param_.length() > maxLength)
    {
        player->sendCancel("Nazwa gildii jest za dluga, wybierz krotsza.");
        return true;
    }

    uint32_t guildId;
    if(Guild::getInstance()->getGuildId(guildId, param_))
    {
        player->sendCancel("Istnieje juz gildia z taka nazwa.");
        return true;
    }

    const uint32_t levelToFormGuild = g_config.GUILD_FORM_LEVEL;
    if(player->getLevel() < levelToFormGuild || player->access > 1)
    {
        char buffer[70 + levelToFormGuild];
        sprintf(buffer, "Musisz posiadac %d poziom do stworzenia gildii.", levelToFormGuild);
        player->sendCancel(buffer);
        return true;
    }

    player->setGuildName(param_);
    Guild::getInstance()->createGuild(player);

    char buffer[50 + maxLength];
    sprintf(buffer, "Stworzyles gildie \"%s\"!", param_.c_str());
    player->sendTextMessage(MSG_INFO, buffer);
    return true;
}

#ifdef HUCZU_HITS_KOMENDA
bool Commands::showHits(Creature* c, const std::string &cmd, const std::string &param)
{
    Player* player = dynamic_cast<Player*>(c);
    std::stringstream info;
    if(player)
    {
        if(player->showHits)
        {
            player->showHits = false;
            info << "Pokazywanie obrazen wylaczone.";
        }
        else
        {
            player->showHits = true;
            info << "Pokazywanie obrazen wlaczone.";
        }
        player->sendTextMessage(MSG_RED_TEXT, info.str().c_str());
        return true;
    }
    return false;
}
#endif
