#ifndef __TILE_H__
#define __TILE_H__

#include "item.h"
#include "container.h"
#include "magic.h"
////#include "preheaders.h"
#include "templates.h"
#include "scheduler.h"

class Creature;
class House;

typedef std::vector<Item*> ItemVector;
typedef std::vector<Creature*> CreatureVector;

class Tile
{
public:
    Creature* getCreature() const
    {
        if(!creatures.empty())
            return creatures[0];
        else
            return NULL;
    }

    Tile()
    {
        pz     = false;
        splash = NULL;
        ground = NULL;
        /*#ifdef HUCZU_NOLOGOUT_TILE
                noLogout = false;
        #endif*/
#ifdef TLM_HOUSE_SYSTEM
        house = NULL;
#endif //TLM_HOUSE_SYSTEM

#ifdef YUR_PVP_ARENA
        pvpArena = false;
#endif //YUR_PVP_ARENA
    }

    Item*          ground;
    Item*          splash;
    ItemVector     topItems;
    CreatureVector creatures;
    ItemVector     downItems;

#ifdef TLM_HOUSE_SYSTEM
    bool isHouse() const;
    House* getHouse() const;
    bool setHouse(House* newHouse);
#endif //TLM_HOUSE_SYSTEM

#ifdef YUR_PVP_ARENA
    bool isPvpArena() const;
    void setPvpArena(const Position& exit);
    Position getPvpArenaExit() const;
#endif //YUR_PVP_ARENA

    int32_t clean();

#ifdef YUR_CVS_MODS
    int32_t getItemHoldingCount() const;
#endif //YUR_CVS_MODS

    bool removeThing(Thing *thing);
    void addThing(Thing *thing);
    bool insertThing(Thing *thing, int32_t stackpos);
    MagicEffectItem* getFieldItem();
    Teleport* getTeleportItem() const;

    Thing* getTopMoveableThing();
    Creature* getTopCreature();
    Item* getTopTopItem();
    Item* getTopDownItem();
    Item* getMoveableBlockingItem();

    int32_t getCreatureStackPos(Creature *c) const;
    int32_t getThingStackPos(const Thing *thing) const;
    int32_t getThingCount() const;

    Thing* getTopThing();
    Thing* getThingByStackPos(int32_t pos);

    //bool isBlockingProjectile() const;
    //bool isBlocking(bool ispickupable = false, bool ignoreMoveableBlocking = false) const;
    ReturnValue isBlocking(int32_t objectstate, bool ignoreCreature = false, bool ignoreMoveableBlocking = false) const;

    bool isPz() const;
    void setPz();
    /*#ifdef HUCZU_NOLOGOUT_TILE
        bool isNoLogout() const;
        void setNoLogout();
    #endif*/
    bool hasItem(uint32_t id) const;

    bool floorChange() const;
    bool floorChangeDown() const;
    bool floorChange(Direction direction) const;

    std::string getDescription() const;
protected:
    bool pz;
    /*#ifdef HUCZU_NOLOGOUT_TILE
        bool noLogout;
    #endif*/
#ifdef TLM_HOUSE_SYSTEM
    House* house;
#endif //TLM_HOUSE_SYSTEM

#ifdef YUR_PVP_ARENA
    bool pvpArena;
    Position arenaExit;
#endif //YUR_PVP_ARENA
};
#endif // #ifndef __TILE_H__
