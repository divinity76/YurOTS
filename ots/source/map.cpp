//#include "preheaders.h"

#include <string>
#include <sstream>
#include <map>
#include <algorithm>

using namespace std;

#include "iomap.h"
#include "iomapotbm.h"
#include "otsystem.h"
#include "items.h"
#include "map.h"
#include "tile.h"
#include "player.h"
#include "tools.h"
#include "npc.h"
#include "spells.h"
#include "luascript.h"

#define EVENT_CHECKPLAYER          123
#define EVENT_CHECKPLAYERATTACKING 124

extern LuaScript g_config;
extern Spells spells;
//extern std::map<int32_t, Creature*> channel;

Map::Map() :
    spawnfile(""),
    mapwidth(0),
    mapheight(0)
{
}


Map::~Map()
{
}


int32_t Map::loadMap(std::string filename, std::string filekind)
{
    int32_t ret;
    IOMap* loader;

    if(filekind == "OTBM")
    {
        loader = new IOMapOTBM();
        ret = SPAWN_XML;
    }
    else
    {
        std::cout << "FATAL: couldnt determine the map format! exiting1" << std::endl;
        exit(1);
    }

    //std::cout << "==| Loading map from: " << loader->getSourceDescription() << std::endl; // zbedne, linux
    bool success = loader->loadMap(this, filename);
    delete loader;

    if(success)
    {
        return ret;
    }
    else
    {
        return MAP_LOADER_ERROR;
    }
}

Tile* Map::getTile(uint16_t _x, uint16_t _y, unsigned char _z)
{
    if(_z < MAP_LAYER)
    {
        // _x & 0x7F  is like _x % 128
        //TileMap *tm = &tileMaps[_x & 0x1F][_y & 0x1F][_z];
        //TileMap *tm = &tileMaps[_x & 0xFF][_y & 0xFF];
        TileMap *tm = &tileMaps[_x & 0x7F][_y & 0x7F];
        if(!tm)
            return NULL;

        // search in the stl map for the requested tile
        //TileMap::iterator it = tm->find((_x << 16) | _y);
        //TileMap::iterator it = tm->find(_x & 0xFF00) << 8 | (_y & 0xFF00) | _z);
        TileMap::iterator it = tm->find((_x & 0xFF80) << 16 | (_y & 0xFF80) << 1 | _z);

        // ... found
        if(it != tm->end())
            return it->second;
    }

    // or not
    return NULL;
}

Tile* Map::getTile(const Position &pos)
{
    return getTile(pos.x, pos.y, pos.z);
}

void Map::setTile(uint16_t _x, uint16_t _y, unsigned char _z, uint16_t groundId)
{
    Tile* tile = setTile(_x, _y, _z);

    if(tile->ground)
        //delete tile->ground;
        tile->ground->releaseThing();

    if(groundId != 0 && Item::items[groundId].isGroundTile())
    {
        tile->ground = Item::CreateItem(groundId);
        tile->ground->pos.x = _x;
        tile->ground->pos.y = _y;
        tile->ground->pos.z = _z;
    }
}

Tile* Map::setTile(uint16_t _x, uint16_t _y, unsigned char _z)
{
    Tile *tile = getTile(_x, _y, _z);

    if(!tile)
    {
        tile = new Tile();
        //tileMaps[_x & 0x1F][_y & 0x1F][_z][(_x << 16) | _y] = tile;
        //tileMaps[_x & 0xFF][_y & 0xFF][ _x & 0xFF00) << 8 | (_y & 0xFF00) | _z] = tile;
        tileMaps[_x & 0x7F][_y & 0x7F][ (_x & 0xFF80) << 16 | (_y & 0xFF80) << 1 | _z] = tile;
    }
    return tile;
}

bool Map::placeCreature(Position &pos, Creature* c)
{
    Tile* tile = getTile(pos.x, pos.y, pos.z);
    bool success = tile && !tile->floorChange() && !tile->getTeleportItem() && c->canMovedTo(tile);

#ifdef TLM_HOUSE_SYSTEM
    Player* p = dynamic_cast<Player*>(c);

    if (success && p && p->access < g_config.ACCESS_HOUSE && tile->isHouse() &&
            tile->getHouse()->getPlayerRights(p->pos, p->getName()) == HOUSE_NONE)
        success = false;
#endif //TLM_HOUSE_SYSTEM

    if(!success)
    {
        for(int32_t cx =pos.x - 1; cx <= pos.x + 1 && !success; cx++)
        {
            for(int32_t cy = pos.y - 1; cy <= pos.y + 1 && !success; cy++)
            {
#ifdef __DEBUG__
                std::cout << "search pos x: " << cx <<" y: "<< cy << std::endl;
#endif

                tile = getTile(cx, cy, pos.z);
                success = tile && !tile->floorChange() && !tile->getTeleportItem() && c->canMovedTo(tile);

#ifdef TLM_HOUSE_SYSTEM
                if (success && p && p->access < g_config.ACCESS_HOUSE && tile->isHouse() &&
                        tile->getHouse()->getPlayerRights(Position(cx,cy,pos.z), p->getName()) == HOUSE_NONE)
                    success = false;
#endif //TLM_HOUSE_SYSTEM

                if(success)
                {
                    pos.x = cx;
                    pos.y = cy;
                }
            }
        }

        if(!success)
        {
            Player *player = dynamic_cast<Player*>(c);
            if(player)
            {
                pos.x = c->masterPos.x;
                pos.y = c->masterPos.y;
                pos.z = c->masterPos.z;

                tile = getTile(pos.x, pos.y, pos.z);
                success = tile && !tile->floorChange() && !tile->getTeleportItem() && player->canMovedTo(tile);
            }
        }

    }

    if(!success || !tile)
    {
#ifdef __DEBUG__
        std::cout << "Failed to place creature onto map!" << std::endl;
#endif
        return false;
    }
#ifdef __DEBUG__
    std::cout << "POS: " << c->pos << std::endl;
#endif
    tile->addThing(c);
    c->pos = pos;

    return true;
}

bool Map::removeCreature(Creature* c)
{
    //OTSYS_THREAD_LOCK(mapLock)
    //bool ret = true;

    Tile *tile = getTile(c->pos.x, c->pos.y, c->pos.z);
    if(!tile || !tile->removeThing(c))
        return false;

    //OTSYS_THREAD_UNLOCK(mapLock)
    return true;
}

void Map::getSpectators(const Range& range, SpectatorVec& list)
{
    /*
    #ifdef __DEBUG__
    	std::cout << "Viewer position at x: " << range.centerpos.x
    		<< ", y: " << range.centerpos.y
    		<< ", z: " << range.centerpos.z << std::endl;
    	std::cout << "Min Range x: " << range.minRange.x
    		<< ", y: " << range.minRange.y
    		<< ", z: " << range.minRange.z << std::endl;
        std::cout << "Max Range x: " << range.maxRange.x
    		<< ", y: " << range.maxRange.y
    		<< ", z: " << range.maxRange.z << std::endl;
    #endif
    */

    int32_t offsetz;
    CreatureVector::iterator cit;
    Tile *tile;

    for(int32_t nz = range.minRange.z; nz != range.maxRange.z + range.zstep; nz += range.zstep)
    {
        offsetz = range.centerpos.z - nz;
        //negative offset means that the player is on a lower floor than ourself

        for (int32_t nx = range.minRange.x + offsetz; nx <= range.maxRange.x + offsetz; ++nx)
        {
            for (int32_t ny = range.minRange.y + offsetz; ny <= range.maxRange.y + offsetz; ++ny)
            {
                tile = getTile(nx + range.centerpos.x, ny + range.centerpos.y, nz);
                if (tile)
                {
                    for (cit = tile->creatures.begin(); cit != tile->creatures.end(); ++cit)
                    {
                        /*
                        #ifdef __DEBUG__
                        						std::cout << "Found " << (*cit)->getName() << " at x: " << (*cit)->pos.x << ", y: " << (*cit)->pos.y << ", z: " << (*cit)->pos.z << ", offset: " << offsetz << std::endl;
                        #endif
                        */
                        list.push_back((*cit));
                    }
                }
            }
        }
    }
}

//bool Map::canThrowItemTo(Position from, Position to, bool creaturesBlock /* = true*/, bool isProjectile /*= false*/)
ReturnValue Map::canThrowObjectTo(Position from, Position to, int32_t objectstate /*= BLOCK_PROJECTILE*/)
{
    Position start = from;
    Position end = to;

    /*if(start.x > end.x) {
    	swap(start.x, end.x);
    	swap(start.y, end.y);
    }*/

    bool steep = std::abs(end.y - start.y) > abs(end.x - start.x);

    if(steep)
    {
        swap(start.x, start.y);
        swap(end.x, end.y);
    }

    int32_t deltax = abs(end.x - start.x);
    int32_t deltay = abs(end.y - start.y);
    int32_t error = 0;
    int32_t deltaerr = deltay;
    int32_t y = start.y;
    Tile *t = NULL;
    int32_t xstep = ((start.x < end.x) ? 1 : -1);
    int32_t ystep = ((start.y < end.y) ? 1 : -1);

    //for(int32_t x = start.x; x != end.x; x += xstep) {
    for(int32_t x = start.x; x != end.x + xstep; x += xstep)
    {
        int32_t rx = (steep ? y : x);
        int32_t ry = (steep ? x : y);

        if(!(from.x == rx && from.y == ry))
        {

            t = getTile(rx, ry, start.z);
            ReturnValue ret = RET_NOERROR;

            if(t)
            {
                if((to.x == rx && to.y == ry))
                {
                    ret = t->isBlocking(objectstate);
                }
                else
                {
                    ret = t->isBlocking(BLOCK_PROJECTILE);
                }
            }

            if(ret != RET_NOERROR)
                return ret;
        }

        error += deltaerr;

        if(2 * error >= deltax)
        {
            y += ystep;
            error -= deltax;
        }
    }

    return RET_NOERROR;
    //return true;
}

ReturnValue Map::isPathValid(Creature *creature, const std::list<Position>& path, int32_t pathSize,
                             bool ignoreMoveableBlockingItems /*= false)*/)
{
    int32_t pathCount = 0;
    std::list<Position>::const_iterator iit;
    for(iit = path.begin(); iit != path.end(); ++iit)
    {

        Tile *t = getTile(iit->x, iit->y, iit->z);
        if(t)
        {
            ReturnValue ret = t->isBlocking(BLOCK_SOLID | BLOCK_PATHFIND, false, ignoreMoveableBlockingItems);
            if(ret == RET_CREATUREBLOCK && t->getCreature() == creature && t->creatures.size() == 1)
                ret = RET_NOERROR;

            if(ret != RET_NOERROR)
            {
                //std::cout << "isPathValid - false: " << (int32_t) ret << std::endl;
                return ret;
            }
        }
        else
            return RET_NOTILE;
        //return false;

        if(pathCount++ >= pathSize)
            return RET_NOERROR;
    }

    return RET_NOERROR;
    //return true;
}
std::list<Position> Map::getPathToEx(Creature *creature, Position start, Position to,
                                     bool creaturesBlock /*=true*/, bool ignoreMoveableBlockingItems /*= false*/, int32_t maxNodSize /*= 100*/)
{
    std::list<Position> path;

    AStarNodes nodes;
    AStarNode* found = NULL;
    int32_t z = start.z;

    AStarNode* startNode = nodes.createOpenNode();
    startNode->parent = NULL;
    startNode->h = 0;
    startNode->x = start.x;
    startNode->y = start.y;

    while(!found && nodes.countClosedNodes() < (uint32_t)maxNodSize)
    {
        AStarNode* current = nodes.getBestNode();
        if(!current)
            return path;

        nodes.closeNode(current);

        for(int32_t dx=-1; dx <= 1; dx++)
        {
            for(int32_t dy=-1; dy <= 1; dy++)
            {
                if(1 == 1/*std::abs(dx) != std::abs(dy)*/)
                {

                    int32_t x = current->x + dx;
                    int32_t y = current->y + dy;

                    if(dx == -1 && dy == -1)
                    {
                        Tile *ctest1 = getTile(x+1, y, z);
                        Tile *ctest2 = getTile(x, y+1, z);
                        if(ctest1 && ctest2)
                        {

                            ReturnValue ctest1er = ctest1->isBlocking(BLOCK_SOLID | BLOCK_PATHFIND, !creaturesBlock, ignoreMoveableBlockingItems);
                            ReturnValue ctest2er = ctest2->isBlocking(BLOCK_SOLID | BLOCK_PATHFIND, !creaturesBlock, ignoreMoveableBlockingItems);

                            if(ctest1er == RET_CREATUREBLOCK && ctest1->getCreature() == creature && ctest1->creatures.size() == 1)
                                ctest1er = RET_NOERROR;
                            if(ctest2er == RET_CREATUREBLOCK && ctest2->getCreature() == creature && ctest2->creatures.size() == 1)
                                ctest2er = RET_NOERROR;

                            if(ctest1er == RET_NOERROR || ctest2er == RET_NOERROR)
                            {
                                continue;
                            }
                        }

                    }

                    if(dx == 1 && dy == -1)
                    {
                        Tile *ctest1 = getTile(x-1, y, z);
                        Tile *ctest2 = getTile(x, y+1, z);
                        if(ctest1 && ctest2)
                        {

                            ReturnValue ctest1er = ctest1->isBlocking(BLOCK_SOLID | BLOCK_PATHFIND, !creaturesBlock, ignoreMoveableBlockingItems);
                            ReturnValue ctest2er = ctest2->isBlocking(BLOCK_SOLID | BLOCK_PATHFIND, !creaturesBlock, ignoreMoveableBlockingItems);

                            if(ctest1er == RET_CREATUREBLOCK && ctest1->getCreature() == creature && ctest1->creatures.size() == 1)
                                ctest1er = RET_NOERROR;
                            if(ctest2er == RET_CREATUREBLOCK && ctest2->getCreature() == creature && ctest2->creatures.size() == 1)
                                ctest2er = RET_NOERROR;

                            if(ctest1er == RET_NOERROR || ctest2er == RET_NOERROR)
                            {
                                continue;
                            }
                        }

                    }

                    if(dx == 1 && dy == 1)
                    {
                        Tile *ctest1 = getTile(x-1, y, z);
                        Tile *ctest2 = getTile(x, y-1, z);
                        if(ctest1 && ctest2)
                        {

                            ReturnValue ctest1er = ctest1->isBlocking(BLOCK_SOLID | BLOCK_PATHFIND, !creaturesBlock, ignoreMoveableBlockingItems);
                            ReturnValue ctest2er = ctest2->isBlocking(BLOCK_SOLID | BLOCK_PATHFIND, !creaturesBlock, ignoreMoveableBlockingItems);

                            if(ctest1er == RET_CREATUREBLOCK && ctest1->getCreature() == creature && ctest1->creatures.size() == 1)
                                ctest1er = RET_NOERROR;
                            if(ctest2er == RET_CREATUREBLOCK && ctest2->getCreature() == creature && ctest2->creatures.size() == 1)
                                ctest2er = RET_NOERROR;

                            if(ctest1er == RET_NOERROR || ctest2er == RET_NOERROR)
                            {
                                continue;
                            }
                        }

                    }

                    if(dx == -1 && dy == 1)
                    {
                        Tile *ctest1 = getTile(x-1, y, z);
                        Tile *ctest2 = getTile(x, y+1, z);
                        if(ctest1 && ctest2)
                        {

                            ReturnValue ctest1er = ctest1->isBlocking(BLOCK_SOLID | BLOCK_PATHFIND, !creaturesBlock, ignoreMoveableBlockingItems);
                            ReturnValue ctest2er = ctest2->isBlocking(BLOCK_SOLID | BLOCK_PATHFIND, !creaturesBlock, ignoreMoveableBlockingItems);

                            if(ctest1er == RET_CREATUREBLOCK && ctest1->getCreature() == creature && ctest1->creatures.size() == 1)
                                ctest1er = RET_NOERROR;
                            if(ctest2er == RET_CREATUREBLOCK && ctest2->getCreature() == creature && ctest2->creatures.size() == 1)
                                ctest2er = RET_NOERROR;

                            if(ctest1er == RET_NOERROR || ctest2er == RET_NOERROR)
                            {
                                continue;
                            }
                        }

                    }

                    Tile *t = getTile(x, y, z);
                    if(t)
                    {
                        ReturnValue ret = t->isBlocking(BLOCK_SOLID | BLOCK_PATHFIND, !creaturesBlock, ignoreMoveableBlockingItems);
                        if(ret == RET_CREATUREBLOCK && t->getCreature() == creature && t->creatures.size() == 1)
                            ret = RET_NOERROR;

                        if(ret != RET_NOERROR)
                        {
                            continue;
                        }
                    }
                    else
                        continue;

                    if(!nodes.isInList(x,y))
                    {
                        AStarNode* n = nodes.createOpenNode();
                        if(n)
                        {
                            n->x = x;
                            n->y = y;
                            n->h = abs(n->x - to.x)*abs(n->x - to.x) + abs(n->y - to.y)*abs(n->y - to.y);
                            n->parent = current;
                            if(x == to.x && y == to.y)
                            {
                                found = n;
                            }
                        }
                    }

                }
            }
        }
    }

    while(found)
    {
        Position p;
        p.x = found->x;
        p.y = found->y;
        p.z = z;
        path.push_front(p);
        found = found->parent;
    }

    return path;
}

std::list<Position> Map::getPathTo(Creature *creature, Position start, Position to,
                                   bool creaturesBlock /*=true*/, bool ignoreMoveableBlockingItems /*= false*/, int32_t maxNodSize /*= 100*/)
{
    std::list<Position> path;
    /*	if(start.z != to.z)
    		return path;
    */
    AStarNodes nodes;
    AStarNode* found = NULL;
    int32_t z = start.z;

    AStarNode* startNode = nodes.createOpenNode();
    startNode->parent = NULL;
    startNode->h = 0;
    startNode->x = start.x;
    startNode->y = start.y;

    while(!found && nodes.countClosedNodes() < (uint32_t)maxNodSize)
    {
        AStarNode* current = nodes.getBestNode();
        if(!current)
            return path; //no path

        nodes.closeNode(current);

        for(int32_t dx=-1; dx <= 1; dx++)
        {
            for(int32_t dy=-1; dy <= 1; dy++)
            {
                if(std::abs(dx) != std::abs(dy))
                {
                    int32_t x = current->x + dx;
                    int32_t y = current->y + dy;

                    Tile *t = getTile(x, y, z);
                    if(t)
                    {
                        ReturnValue ret = t->isBlocking(BLOCK_SOLID | BLOCK_PATHFIND, !creaturesBlock, ignoreMoveableBlockingItems);
                        if(ret == RET_CREATUREBLOCK && t->getCreature() == creature && t->creatures.size() == 1)
                            ret = RET_NOERROR;

                        if(ret != RET_NOERROR)
                        {
                            continue;
                        }
                    }
                    else
                        continue;

                    if(!nodes.isInList(x,y))
                    {
                        AStarNode* n = nodes.createOpenNode();
                        if(n)
                        {
                            n->x = x;
                            n->y = y;
                            n->h = abs(n->x - to.x)*abs(n->x - to.x) + abs(n->y - to.y)*abs(n->y - to.y);
                            n->parent = current;
                            if(x == to.x && y == to.y)
                            {
                                found = n;
                            }
                        }
                    }
                    /*					else{
                    						if(current->g + 1 < child->g)
                    							child->parent = current;
                    							child->g=current->g+1;
                    					}*/
                }
            }
        }
    }
    //cleanup the mess
    while(found)
    {
        Position p;
        p.x = found->x;
        p.y = found->y;
        p.z = z;
        path.push_front(p);
        found = found->parent;
    }

    return path;
}


AStarNodes::AStarNodes()
{
    curNode = 0;
    openNodes.reset();
}

AStarNode* AStarNodes::createOpenNode()
{
    if(curNode >= MAX_NODES)
        return NULL;

    uint32_t ret_node = curNode;
    curNode++;
    openNodes[ret_node] = 1;
    return &nodes[ret_node];
}

AStarNode* AStarNodes::getBestNode()
{
    if(curNode == 0)
        return NULL;

    int32_t best_node_h;
    uint32_t best_node;
    bool found;

    best_node_h = 100000;
    best_node = 0;
    found = false;

    for(uint32_t i = 0; i < curNode; i++)
    {
        if(nodes[i].h < best_node_h && openNodes[i] == 1)
        {
            found = true;
            best_node_h = nodes[i].h;
            best_node = i;
        }
    }
    if(found)
    {
        return &nodes[best_node];
    }
    else
    {
        return NULL;
    }
}

void AStarNodes::closeNode(AStarNode* node)
{
    uint32_t pos = (uint32_t)GET_NODE_INDEX(node);
    if(pos < 0 || pos >= MAX_NODES)
    {
        std::cout << "AStarNodes. trying to close node out of range" << std::endl;
        return;
    }
    openNodes[pos] = 0;
}

uint32_t AStarNodes::countClosedNodes()
{
    uint32_t counter = 0;
    for(uint32_t i = 0; i < curNode; i++)
    {
        if(openNodes[i] == 0)
        {
            counter++;
        }
    }
    return counter;
}

uint32_t AStarNodes::countOpenNodes()
{
    uint32_t counter = 0;
    for(uint32_t i = 0; i < curNode; i++)
    {
        if(openNodes[i] == 1)
        {
            counter++;
        }
    }
    return counter;
}


bool AStarNodes::isInList(uint32_t x, uint32_t y)
{
    for(unsigned i = 0; i < curNode; i++)
    {
        if(nodes[i].x == x && nodes[i].y == y)
        {
            return true;
        }
    }
    return false;
}

int32_t Map::clean()
{
    int32_t count = 0;

    for (int32_t i = 0; i < 128; i++)
        for (int32_t j = 0; j < 128; j++)
        {
            TileMap::iterator iter = tileMaps[i][j].begin();

            while (iter != tileMaps[i][j].end())
            {
                count += iter->second->clean();
                ++iter;
            }
        }

    return count;
}
