#ifndef __SPAWN_H
#define __SPAWN_H

////#include "preheaders.h"
#include "tile.h"
#include "position.h"
#include "monster.h"
#include "templates.h"

#include <vector>
#include <map>

class Game;
class Spawn;
typedef std::list<Spawn*> spawnsList;

class Spawn /*: public Event*/
{
public:
    Spawn(Game *igame, Position pos, int32_t _radius);
    void idle(int32_t t);
    bool addMonster(std::string name, Direction dir, int32_t x, int32_t y, int32_t spawntime);
    bool addNPC(std::string name, Direction dir, int32_t x, int32_t y);

public:
    bool startup();

    /*
    virtual void onCreatureEnter(const Creature *creature, const Position &pos);
    virtual void onCreatureLeave(const Creature *creature, const Position &pos);
    */

private:
    Game *game;
    Position centerPos;
    int32_t radius;

    bool isInSpawnRange(const Position &pos);
    Monster* respawn(uint32_t spawnid, Position &pos, std::string &name, Direction dir);
    bool spawnNpc(uint32_t spawnid, Position& pos, std::string &name, Direction dir);

    struct spawninfo
    {
        Position pos;
        std::string name;
        Direction dir;
        int32_t spawntime;
        uint64_t lastspawn;
    };

    //List of monsters in the spawn
    typedef std::map<uint32_t, struct spawninfo> SpawnMap;
    SpawnMap spawnmap;

    //For spawned monsters
    typedef std::multimap<uint32_t, Monster*, std::less<uint32_t> > SpawnedMap;
    typedef SpawnedMap::value_type spawned_pair;
    SpawnedMap spawnedmap;
};

class SpawnManager
{
public:
    SpawnManager();
    ~SpawnManager();

    static SpawnManager* instance();
    static bool initialize(Game *igame);
    static bool addSpawn(Spawn* spawn);
    static bool loadSpawnsXML(std::string filename);
    static bool startup();

    void checkSpawns(int32_t t);
protected:
    static SpawnManager* _instance;
    static spawnsList spawns;
    static Game *game;
};
#endif
