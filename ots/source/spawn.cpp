
//#include "preheaders.h"
#include "spawn.h"
#include "game.h"
#include "player.h"
#include "npc.h"

extern LuaScript g_config;

SpawnManager* SpawnManager::_instance = NULL;
Game* SpawnManager::game = NULL;
spawnsList SpawnManager::spawns;

SpawnManager::SpawnManager()
{
    //
}

SpawnManager::~SpawnManager()
{
    for(spawnsList::iterator it = spawns.begin(); it != spawns.end(); ++it)
        delete *it;

    spawns.clear();
}

bool SpawnManager::initialize(Game *igame)
{
    game = igame;
    _instance = new SpawnManager();
    return (_instance != NULL);
}

SpawnManager* SpawnManager::instance()
{
    return _instance;
}

bool SpawnManager::addSpawn(Spawn* spawn)
{
    spawns.push_back(spawn);
    return true;
}

bool SpawnManager::loadSpawnsXML(std::string filename)
{
    //std::transform(filename.begin(), filename.end(), filename.begin(), tolower);
    toLowerCaseString(filename);
    xmlDocPtr doc = xmlParseFile(filename.c_str());

    if (doc)
    {
        xmlNodePtr root, p;
        char* nodeValue = NULL;
        root = xmlDocGetRootElement(doc);

        root = xmlDocGetRootElement(doc);

        if (xmlStrcmp(root->name,(const xmlChar*) "spawns"))
        {
            xmlFreeDoc(doc);
            return false;
        }

        p = root->children;

        while (p)
        {
            const char* str = (char*)p->name;

            if (strcmp(str, "spawn") == 0)
            {
                Position centerpos;
                int32_t radius;

                nodeValue = (char*)xmlGetProp(p, (const xmlChar *)"centerx");
                if(nodeValue)
                {
                    centerpos.x = atoi(nodeValue);
                    xmlFreeOTSERV(nodeValue);
                }
                else
                {
                    xmlFreeOTSERV(nodeValue);
                    return false;
                }

                nodeValue = (char*)xmlGetProp(p, (const xmlChar *)"centery");
                if(nodeValue)
                {
                    centerpos.y = atoi(nodeValue);
                    xmlFreeOTSERV(nodeValue);
                }
                else
                {
                    xmlFreeOTSERV(nodeValue);
                    xmlFreeDoc(doc);
                    return false;
                }

                nodeValue = (char*)xmlGetProp(p, (const xmlChar *)"centerz");
                if(nodeValue)
                {
                    centerpos.z = atoi(nodeValue);
                    xmlFreeOTSERV(nodeValue);
                }
                else
                {
                    xmlFreeOTSERV(nodeValue);
                    xmlFreeDoc(doc);
                    return false;
                }

                nodeValue = (char*)xmlGetProp(p, (const xmlChar *)"radius");
                if(nodeValue)
                {
                    radius = atoi(nodeValue);
                    xmlFreeOTSERV(nodeValue);
                }
                else
                {
                    xmlFreeOTSERV(nodeValue);
                    xmlFreeDoc(doc);
                    return false;
                }

                //Spawn *spawn = new Spawn(game, centerpos, radius);
                //spawns.push_back(spawn);

                std::string name;
                int32_t x, y, spawntime;
                Direction direction = NORTH;
                int32_t rawdir = 0; //NORTH

                xmlNodePtr tmp = p->children;
                while (tmp)
                {
                    str = (char*)tmp->name;
                    if (strcmp(str, "monster") == 0)
                    {
                        nodeValue = (char*)xmlGetProp(tmp, (const xmlChar *)"name");
                        if(nodeValue)
                        {
                            name = nodeValue;
                            xmlFreeOTSERV(nodeValue);
                        }
                        else
                        {
                            tmp = tmp->next;
                            break;
                        }

                        nodeValue = (char*)xmlGetProp(tmp, (const xmlChar *)"direction");
                        if(nodeValue)
                        {
                            rawdir = atoi(nodeValue);
                            xmlFreeOTSERV(nodeValue);
                        }

                        nodeValue = (char*)xmlGetProp(tmp, (const xmlChar *)"x");
                        if(nodeValue)
                        {
                            x = atoi(nodeValue);
                            xmlFreeOTSERV(nodeValue);
                        }
                        else
                        {
                            tmp = tmp->next;
                            break;
                        }

                        nodeValue = (char*)xmlGetProp(tmp, (const xmlChar *)"y");
                        if(nodeValue)
                        {
                            y = atoi(nodeValue);
                            xmlFreeOTSERV(nodeValue);
                        }
                        else
                        {
                            tmp = tmp->next;
                            break;
                        }

                        nodeValue = (char*)xmlGetProp(tmp, (const xmlChar *)"spawntime");
                        if(nodeValue)
                        {
                            spawntime = atoi(nodeValue);
                            xmlFreeOTSERV(nodeValue);
                        }
                        else
                        {
                            tmp = tmp->next;
                            break;
                        }

                        switch(rawdir)
                        {
                        case 0:
                            direction = NORTH;
                            break;
                        case 1:
                            direction = EAST;
                            break;
                        case 2:
                            direction = SOUTH;
                            break;
                        case 3:
                            direction = WEST;
                            break;
                        default:
                            direction = NORTH;
                            break;
                        }

                        Position spawnPos;
                        spawnPos.x = centerpos.x+x;
                        spawnPos.y = centerpos.y+y;
                        spawnPos.z = centerpos.z;
                        Spawn *spawn = new Spawn(game, spawnPos, radius);
                        spawns.push_back(spawn);
                        spawn->addMonster(name, direction, 0, 0, spawntime * 1000);
                    }
                    else if(xmlStrcmp(tmp->name, (const xmlChar*)"npc") == 0 && g_config.LOAD_NPC != "file")
                    {
                        Position centerPos;

                        nodeValue = (char*)xmlGetProp(tmp, (const xmlChar *)"name");
                        if(nodeValue)
                        {
                            name = nodeValue;
                            xmlFreeOTSERV(nodeValue);
                        }
                        else
                        {
                            tmp = tmp->next;
                            break;
                        }

                        nodeValue = (char*)xmlGetProp(tmp, (const xmlChar *)"direction");
                        if(nodeValue)
                        {
                            rawdir = atoi(nodeValue);
                            xmlFreeOTSERV(nodeValue);
                        }

                        nodeValue = (char*)xmlGetProp(tmp, (const xmlChar *)"x");
                        if(nodeValue)
                        {
                            x = atoi(nodeValue);
                            xmlFreeOTSERV(nodeValue);
                        }
                        else
                        {
                            tmp = tmp->next;
                            break;
                        }

                        nodeValue = (char*)xmlGetProp(tmp, (const xmlChar *)"y");
                        if(nodeValue)
                        {
                            y = atoi(nodeValue);
                            xmlFreeOTSERV(nodeValue);
                        }
                        else
                        {
                            tmp = tmp->next;
                            break;
                        }

                        switch(rawdir)
                        {
                        case 0:
                            direction = NORTH;
                            break;
                        case 1:
                            direction = EAST;
                            break;
                        case 2:
                            direction = SOUTH;
                            break;
                        case 3:
                            direction = WEST;
                            break;
                        default:
                            direction = NORTH;
                            break;
                        }
                        Position pos;
                        pos.x = centerpos.x + x;
                        pos.y = centerpos.y + y;
                        pos.z = centerpos.z;
                        Npc* npc = new Npc(name, game);
                        if(!npc->isLoaded())
                        {
                            delete npc;
                            tmp = tmp->next;
                            continue;
                        }

                        npc->setDirection(direction);
                        npc->masterPos = pos;

                        // Place the npc
                        if(!game->placeCreature(pos, npc))
                        {
                            delete npc;
                            std::cout << "Nie mozna postawic npc!" << std::endl;
                            tmp = tmp->next;
                            continue;
                        }
                    }

                    tmp = tmp->next;
                }
            }
            p = p->next;
        }
        xmlFreeDoc(doc);
        return true;
    }
    return false;
}

bool SpawnManager::startup()
{
    for(spawnsList::iterator it = spawns.begin(); it != spawns.end(); ++it)
    {
        (*it)->startup();
    }

    if(!spawns.empty())
    {
        game->addEvent(makeTask(20000, std::bind2nd(std::mem_fun(&Game::checkSpawns), 20000)));
    }

    return true;
}

void SpawnManager::checkSpawns(int32_t t)
{
    for(spawnsList::iterator it = spawns.begin(); it != spawns.end(); ++it)
    {
        (*it)->idle(t);
    }
}

Spawn::Spawn(Game *igame, Position pos, int32_t _radius)
{
    game = igame;
    centerPos = pos;
    radius = _radius;
}

bool Spawn::startup()
{
    for(SpawnMap::iterator sit = spawnmap.begin(); sit != spawnmap.end(); ++sit)
    {
        respawn(sit->first, sit->second.pos, sit->second.name, sit->second.dir);
    }

    return true;
}

bool Spawn::addMonster(std::string name, Direction dir, int32_t x, int32_t y, int32_t spawntime)
{
    Position tmpPos(centerPos.x + x, centerPos.y, centerPos.z);
    if(!isInSpawnRange(tmpPos))
    {
// #ifdef __DEBUG__
        std::cout << "Monster is outside the spawn-area!" << std::endl;
// #endif
        return false;
    }

    struct spawninfo si;
    si.name = name;
    si.dir = dir;
    si.pos.x = centerPos.x + x;
    si.pos.y = centerPos.y + y;
    si.pos.z = centerPos.z;
    si.spawntime = spawntime;
    si.lastspawn = 0;

    uint32_t spawnid = (int32_t)spawnmap.size() + 1;
    spawnmap[spawnid] = si;

    return true;
}



Monster* Spawn::respawn(uint32_t spawnid, Position &pos, std::string &name, Direction dir)
{
    //Monster *monster = new Monster(name, game);
    Monster* monster = Monster::createMonster(name, game);
    if(monster)
    {
        //if(monster->isLoaded()) {
        monster->setDirection(dir);
        monster->masterPos = centerPos;

        if(game->placeCreature(pos, monster))
        {
            monster->useThing();
            spawnedmap.insert(spawned_pair(spawnid, monster));
            spawnmap[spawnid].lastspawn = OTSYS_TIME();
            return monster;
        }
        //}
        //not loaded, or could not place it on the map
        delete monster;
        monster = NULL;
    }
    return NULL;
}

bool Spawn::isInSpawnRange(const Position &p)
{
    if ((p.x >= centerPos.x - radius) && (p.x <= centerPos.x + radius) &&
            (p.y >= centerPos.y - radius) && (p.y <= centerPos.y + radius))
        return true;

    return false;
}

void Spawn::idle(int32_t t)
{
    SpawnedMap::iterator it;
    for(it = spawnedmap.begin(); it != spawnedmap.end();)
    {
        if (it->second->isRemoved == true /*it->second->health <= 0*/)
        {
            if(it->first != 0)
            {
                spawnmap[it->first].lastspawn = OTSYS_TIME();
            }
            it->second->releaseThing();
            //delete it->second;
            spawnedmap.erase(it++);
        }
        else if(!isInSpawnRange(it->second->pos) && it->first != 0)
        {
            spawnedmap.insert(spawned_pair(0, it->second));
            spawnedmap.erase(it++);
        }
        else
            ++it;
    }

    for(SpawnMap::iterator sit = spawnmap.begin(); sit != spawnmap.end(); ++sit)
    {

        if(spawnedmap.count(sit->first) == 0)
        {
            if((OTSYS_TIME() - sit->second.lastspawn) >= sit->second.spawntime)
            {

                SpectatorVec list;
                SpectatorVec::iterator it;

                game->getSpectators(Range(sit->second.pos, true), list);

                bool playerFound = false;
                for(it = list.begin(); it != list.end(); ++it)
                {
                    Player *player = dynamic_cast<Player*>(*it);

                    if(player && player->access < g_config.ACCESS_PROTECT)
                    {
                        playerFound = true;
                        break;
                    }
                }

                if(playerFound)
                {
                    sit->second.lastspawn = OTSYS_TIME();
                    continue;
                }

                respawn(sit->first, sit->second.pos, sit->second.name, sit->second.dir);
            }
        }
    }
}
