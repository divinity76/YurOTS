
#ifndef __OTSERV_ITEMS_H
#define __OTSERV_ITEMS_H

using namespace std;

#include <map>
#include <string>
#include "const76.h"
#include "itemloader.h"

#define SLOTP_WHEREEVER 0xFFFFFFFF
#define SLOTP_HEAD 1
#define	SLOTP_NECKLACE 2
#define	SLOTP_BACKPACK 4
#define	SLOTP_ARMOR 8
#define	SLOTP_RIGHT 16
#define	SLOTP_LEFT 32
#define	SLOTP_LEGS 64
#define	SLOTP_FEET 128
#define	SLOTP_RING 256
#define	SLOTP_AMMO 512
#define	SLOTP_DEPOT 1024
#define	SLOTP_TWO_HAND 2048

enum eRWInfo
{
    CAN_BE_READ = 1,
    CAN_BE_WRITTEN = 2
};

class ItemType
{
public:
    ItemType();
    ~ItemType();

    itemgroup_t group;

    bool isGroundTile() const;
    bool isContainer() const;
    bool isTeleport() const;
    bool isMagicField() const;
    bool isKey() const;
    bool isSplash() const;
    bool isFluidContainer() const;

    uint16_t id;
    uint16_t clientId;

    uint16_t maxItems;   // maximum size if this is a container
    double weight;						 // weight of the item, e.g. throwing distance depends on it
    std::string			name;			 // the name of the item
    std::string			description;	 // additional description... as in "The blade is a magic flame." for fireswords
    WeaponType			weaponType;
    amu_t						amuType;
    subfight_t			shootType;
    int32_t							attack;
    int32_t							defence;
    int32_t							armor;
    uint16_t	slot_position;
    uint16_t	decayTo;
    uint16_t	decayTime;
    bool						canDecay;

    uint16_t speed;

    // other bools
    int32_t             magicfieldtype;
    int32_t             RWInfo;
    uint16_t  readOnlyId;
    bool            stackable;
    bool            useable;
    bool            moveable;
    bool            alwaysOnTop;
    int32_t             runeMagLevel;
    bool            pickupable;
    bool            rotable;
    int32_t 			rotateTo;

#ifdef TLM_HOUSE_SYSTEM
    bool isDoor;
#endif //TLM_HOUSE_SYSTEM
#ifdef YUR_RINGS_AMULETS
    int32_t newCharges;
    int32_t newTime;
#endif //YUR_RINGS_AMULETS
#ifdef TP_TRASH_BINS
    bool isDeleter;
#endif //TP_TRASH_BINS

    int32_t             lightLevel;
    int32_t             lightColor;

    bool						floorChangeDown;
    bool						floorChangeNorth;
    bool						floorChangeSouth;
    bool						floorChangeEast;
    bool						floorChangeWest;
    bool            hasHeight; //blockpickupable

    bool blockSolid;
    bool blockPickupable;
    bool blockProjectile;
    bool blockPathFind;

    //bool            readable;
    //bool            ismagicfield;
    //bool            issplash;
    //bool            iskey;

    //uint16_t	damage;
    //bool isteleport;
    //bool            fluidcontainer;
    //bool            multitype;
    //bool            iscontainer;
    //bool            groundtile;
    //bool						blockpickupable;
    //bool						canWalkThrough;
    //bool						notMoveable;
    //bool						blocking;						// people can walk on it
    //bool						blockingProjectile;
    //bool						noFloorChange;
    //bool						isDoor;
    //bool						isDoorWithLock;
};

typedef map<uint32_t, uint32_t> ReverseItemMap;

class Items
{
public:
    Items();
    ~Items();

    int32_t loadFromOtb(std::string);
    bool loadXMLInfos(std::string);

    const ItemType& operator[](int32_t id);

    static uint32_t reverseLookUp(uint32_t id);

    static int32_t dwMajorVersion;
    static int32_t dwMinorVersion;
    static int32_t dwBuildNumber;

protected:
    typedef map<uint16_t, ItemType*> ItemMap;

    ItemMap items;
    static ReverseItemMap revItems;

    ItemType dummyItemType; // use this for invalid ids
};

#endif
