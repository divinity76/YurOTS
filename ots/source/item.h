#ifndef __OTSERV_ITEM_H
#define __OTSERV_ITEM_H

#include <libxml/parser.h>
#include <iostream>
#include <list>
#include <vector>

#include "thing.h"
#include "items.h"

class Creature;
class Player;
class Container;

class Item : public Thing
{
protected:
    uint16_t id;  // the same id as in ItemType
    unsigned char count; // number of stacked items
    unsigned char chargecount; //number of charges on the item
    unsigned char fluid;
    uint16_t actionId;
    uint16_t uniqueId;
    std::string *specialDescription;
    std::string *text;	//text written
    std::string *writer; //Last person that wrote
    std::string *owner;
    int *ownerTime;
#ifdef YUR_RINGS_AMULETS
    int32_t time;
    int32_t charges;
#endif //YUR_RINGS_AMULETS
    std::string *readable;

private:
    int32_t useCount;

public:
    static Item* CreateItem(const uint16_t _type, uint16_t _count = 0); //Factory member to create item of right type based on type
    static Items items;

    std::string getWriter();
    void setWriter(std::string name);
    std::string getOwner();
    void setOwner(std::string name);
    int getOwnerTime();
    void setOwnerTime(int32_t time);

    virtual Container* getContainer()
    {
        return NULL;
    }
    virtual const Container* getContainer() const
    {
        return NULL;
    }

    uint16_t getID() const;    // ID as in ItemType
    void setID(uint16_t newid);
    WeaponType getWeaponType() const;
    amu_t	getAmuType() const;
    subfight_t getSubfightType() const;
    virtual double getWeight() const;
    int64_t getAttack() const;
    int64_t getArmor() const;
    int64_t getDefense() const;
    int32_t getSlotPosition() const;
    int32_t getRWInfo() const;
    int32_t getWorth() const;

    bool isBlocking() const;
    bool isStackable() const;
    bool isFluidContainer() const;
#ifdef HUCZU_LOOT_INFO
    bool isContainer() const;
#endif //HUCZU_LOOT_INFO
    //bool isMultiType() const;
    bool isAlwaysOnTop() const;
    bool isGroundTile() const;
    bool isSplash() const;
    bool isNotMoveable() const;
    bool isPickupable() const;
    bool isWeapon() const;
    bool isUseable() const;

    bool floorChangeDown() const;
    bool floorChangeNorth() const;
    bool floorChangeSouth() const;
    bool floorChangeEast() const;
    bool floorChangeWest() const;

#ifdef YUR_RINGS_AMULETS
    void setItemTime(int32_t _time)
    {
        time = _time;
    }
    void setCharges(int32_t _charges)
    {
        charges = _charges;
    }
    int32_t getCharges() const
    {
        return charges;
    }
    int32_t getTime() const
    {
        return time;
    }
    void useCharge()
    {
        --charges;
    }
    void useTime(int32_t thinkTicks)
    {
        time -= thinkTicks;
    }
    void setGlimmer();
    void removeGlimmer();
#endif //YUR_RINGS_AMULETS
    void setReadable(const std::string& text)
    {
        readable = new std::string(text);
    }
#ifdef TP_TRASH_BINS
    bool isDeleter() const
    {
        return items[id].isDeleter;
    }
#endif //TP_TRASH_BINS
    bool decoration;
#ifdef HUCZU_LOOT_INFO
    virtual std::string getLootDescription() const;
#endif //HUCZU_LOOT_INFO
    virtual std::string getDescription(bool fullDescription) const;
    std::string getName() const ;
    void setSpecialDescription(std::string desc);
    std::string getSpecialDescription();
    void clearSpecialDescription();
    void setText(std::string desc);
    void clearText();
    std::string getText();

    virtual int32_t unserialize(xmlNodePtr p);
    virtual xmlNodePtr serialize();

    // get the number of items
    uint16_t getItemCountOrSubtype() const;
    void setItemCountOrSubtype(unsigned char n);

    unsigned char getItemCharge() const
    {
        return chargecount;
    };
    void setItemCharge(unsigned char n)
    {
        chargecount = n;
    };

    unsigned char getFluidType() const
    {
        return fluid;
    };
    void setFluidType(unsigned char n)
    {
        fluid = n;
    };

    void setActionId(uint16_t n);
    uint16_t getActionId() const;

    void setUniqueId(uint16_t n);
    uint16_t getUniqueId() const;

    virtual int32_t getDecayTime();
    bool canDecay();

    virtual Item* decay();
    bool isDecaying;

    bool rotate();

    // Constructor for items
    Item(const uint16_t _type);
    Item(const uint16_t _type, uint16_t _count);
    Item();
    Item(const Item &i);

    virtual ~Item();
    virtual void useThing()
    {
        //std::cout << "Item: useThing() " << this << std::endl;
        useCount++;
    };

    virtual void releaseThing()
    {
        useCount--;
        //std::cout << "Item: releaseThing() " << this << std::endl;
        //if (useCount == 0)
        if (useCount <= 0)
            delete this;
    };

    virtual bool canMovedTo(const Tile *tile) const;
};

class Teleport : public Item
{
public:
    Teleport(const uint16_t _type);
    virtual ~Teleport();
    virtual void useThing()
    {
        //std::cout << "Teleport: useThing() " << this << std::endl;
        useCount++;
    };

    virtual void releaseThing()
    {
        useCount--;
        //std::cout << "Teleport: releaseThing() " << this << std::endl;
        //if (useCount == 0)
        if (useCount <= 0)
            delete this;
    };

    void setDestPos(const Position &pos)
    {
        destPos = pos;
    };
    const Position& getDestPos() const
    {
        return destPos;
    };
private:
    int32_t useCount;
    virtual int32_t unserialize(xmlNodePtr p);
    virtual xmlNodePtr serialize();
    Position destPos;
};

#endif
