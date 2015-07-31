
#ifndef __OTSERV_CONTAINER_H
#define __OTSERV_CONTAINER_H

#include "item.h"

typedef std::list<Item *> ContainerList;

class Container : public Item
{
private:
    int32_t useCount;
    Container *parent;
    uint16_t maxitems; //number of max items in container
    uint16_t actualitems; // number of items in container
    ContainerList lcontained;
#ifdef HUCZU_LOOT_INFO
    std::stringstream& getContentDescription(std::stringstream& s);
#endif
public:
    Container(const uint16_t _type);
    virtual ~Container();
    virtual void useThing()
    {
        //std::cout << "Container: useThing() " << this << std::endl;
        useCount++;
    };

    virtual void releaseThing()
    {
        useCount--;
        //std::cout << "Container: releaseThing() " << this << std::endl;
        //if (useCount == 0)
        if (useCount <= 0)
            delete this;
    };
    virtual Container* getContainer()
    {
        return this;
    }
    virtual const Container* getContainer() const
    {
        return this;
    }
    uint32_t depot;
    int32_t size() const
    {
        return actualitems;
    };
    int32_t capacity() const
    {
        return maxitems;
    };
    void setParent(Container* container)
    {
        parent = container;
    };
    Container *getParent()
    {
        return parent;
    }
    Container *getParent() const
    {
        return parent;
    }
    Container *getTopParent();
    const Container *getTopParent() const;

    ContainerList::const_iterator getItems() const;     // begin();
    ContainerList::const_iterator getEnd() const;       // iterator beyond the last element
    bool addItem(Item* newitem);     // add an item to the container
    bool removeItem(Item* item); //remove an item from the container
    void moveItem(uint16_t from_slot, unsigned char to_slot);
    Item* getItem(uint32_t slot_num);
    const Item* getItem(uint32_t slot_num) const;
    unsigned char getSlotNumberByItem(const Item* item) const;
    bool isHoldingItem(const Item* item) const;
    int32_t getItemHoldingCount() const;
    virtual double getWeight() const;
#ifdef HUCZU_LOOT_INFO
    std::string getContentDescription();
#endif
};

#endif //__OTSERV_CONTAINER_H

