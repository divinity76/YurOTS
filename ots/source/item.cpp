//#include "preheaders.h"
#include "container.h"
#include "magic.h"
#include "player.h"
#include "tile.h"
#include "actions.h"

#include <iostream>
#include <sstream>
#include <iomanip>


Item* Item::CreateItem(const uint16_t _type, uint16_t _count /*= 0*/)
{
    Item *newItem;

    //const ItemType& it = Item::items[_type];

    if(items[_type].isContainer())
    {
        newItem = new Container(_type);
    }
    else if(items[_type].isTeleport())
    {
        newItem = new Teleport(_type);
    }
    else if(items[_type].isMagicField())
    {
        newItem =  new Item(_type, _count);
    }
    else
    {
        newItem =  new Item(_type, _count);
    }
    newItem->isRemoved = false;
    newItem->useThing();
    return newItem;
}

//////////////////////////////////////////////////
// returns the ID of this item's ItemType
uint16_t Item::getID() const
{
    return id;
}

// sets the ID of this item's ItemType
void Item::setID(uint16_t newid)
{
    id = newid;
}

// return how many items are stacked or 0 if non stackable
uint16_t Item::getItemCountOrSubtype() const
{
    if(isStackable())
    {
        return count;
    }
    else if(isFluidContainer() || isSplash())
        return fluid;
    //else if(chargecount != 0)
    else if(items[id].runeMagLevel != -1)
        return chargecount;
    else
        return 0;
}

void Item::setItemCountOrSubtype(unsigned char n)
{
    if(isStackable())
    {
        /*if(n == 0){
        	count = 1;
        }*/
        if(n > 100)
        {
            count = 100;
        }
        else
        {
            count = n;
        }
    }
    else if(isFluidContainer() || isSplash())
        fluid = n;
    else if(items[id].runeMagLevel != -1)
        chargecount = n;
};

void Item::setActionId(uint16_t n)
{
    /* if(n < 100)
       n = 100; */
    actionId = n;
}

uint16_t Item::getActionId() const
{
    return actionId;
}

void Item::setUniqueId(uint16_t n)
{
    //uniqueId only can be set 1 time
    if(uniqueId != 0)
        return;
    if(n < 1000)
        n = 1000;
    uniqueId = n;
    ActionScript::AddThingToMapUnique(this);
}

uint16_t Item::getUniqueId() const
{
    return uniqueId;
}

Item::Item(const uint16_t _type)
{
    //std::cout << "Item constructor1 " << this << std::endl;
    id = _type;
    count = 0;
    chargecount = 0;
    fluid = 0;
    actionId = 0;
    uniqueId = 0;
    throwRange = 6;
    useCount = 0;
    isDecaying  = 0;
    specialDescription = NULL;
    text = NULL;
    writer = NULL;
    owner = NULL;
    ownerTime = 0;
#ifdef YUR_RINGS_AMULETS
    const ItemType& it = items[id];
    time = it.newTime;
    charges = it.newCharges;
#endif //YUR_RINGS_AMULETS
    readable = NULL;
    decoration = false;
}

Item::Item(const Item &i)
{
    //std::cout << "Item copy constructor " << this << std::endl;
    id = i.id;
    count = i.count;
    chargecount = i.chargecount;
    throwRange = i.throwRange;
    useCount = 0;
    isDecaying  = 0;
    actionId = i.actionId;
    uniqueId = i.uniqueId;
    if(i.specialDescription != NULL)
    {
        specialDescription = new std::string(*(i.specialDescription));
    }
    else
    {
        specialDescription = NULL;
    }
    if(i.text != NULL)
    {
        text = new std::string(*(i.text));
    }
    else
    {
        text = NULL;
    }
    writer = NULL;
    owner = NULL;
    ownerTime = 0;
#ifdef YUR_RINGS_AMULETS
    time = i.time;
    charges = i.charges;
#endif //YUR_RINGS_AMULETS
    if (i.readable)
        readable = new std::string(*(i.readable));
    else
        readable = NULL;
    decoration = i.decoration;
}

Item* Item::decay()
{
    uint16_t decayTo   = Item::items[getID()].decayTo;

    if(decayTo == 0)
    {
        return NULL;
    }

    if(dynamic_cast<Container*>(this))
    {
        if(items[decayTo].isContainer())
        {
            //container -> container
            setID(decayTo);
            return this;
        }
        else
        {
            //container -> no container
            Item *item = Item::CreateItem(decayTo,getItemCountOrSubtype());
            item->pos = this->pos;
            return item;
        }
    }
    else
    {
        if(items[decayTo].isContainer())
        {
            //no container -> container
            Item *item = Item::CreateItem(decayTo,getItemCountOrSubtype());
            item->pos = this->pos;
            return item;
        }
        else
        {
            //no contaier -> no container
            setID(decayTo);
            return this;
        }
    }
}

int32_t Item::getDecayTime()
{
    return items[id].decayTime*1000;
}

bool Item::rotate()
{
    if(items[id].rotable && items[id].rotateTo)
    {
        id = items[id].rotateTo;
        return true;
    }
    return false;
}

Item::Item(const uint16_t _type, uint16_t _count)
{
    //std::cout << "Item constructor2 " << this << std::endl;
    id = _type;
    count = 0;
    chargecount = 0;
    fluid = 0;
    actionId = 0;
    uniqueId = 0;
    useCount = 0;
    isDecaying  = 0;
    specialDescription = NULL;
    text = NULL;
    writer = NULL;
    owner = NULL;
    ownerTime = 0;
    setItemCountOrSubtype((unsigned char)_count);
    /*
    if(isStackable()){
    	if(_count == 0){
    		count = 1;
    	}
    	else if(_count > 100){
    		count = 100;
    	}
    	else{
    		count = _count;
    	}
    }
    else if(isFluidContainer() || isMultiType() )
    	fluid = _count;
    else
    	chargecount = _count;
    */
    throwRange = 6;

#ifdef YUR_RINGS_AMULETS
    const ItemType& it = items[id];
    time = it.newTime;
    charges = it.newCharges;
#endif //YUR_RINGS_AMULETS
    readable = NULL;
    decoration = false;
}

Item::Item()
{
    //std::cout << "Item constructor3 " << this << std::endl;
    id = 0;
    count = 0;
    chargecount = 0;
    throwRange = 6;
    useCount = 0;
    isDecaying  = 0;
    actionId = 0;
    uniqueId = 0;
    specialDescription = NULL;
    text = NULL;
    writer = NULL;
    owner = NULL;
    ownerTime = 0;
#ifdef YUR_RINGS_AMULETS
    time = 0;
    charges = 0;
#endif //YUR_RINGS_AMULETS
    readable = NULL;
    decoration = false;
}

Item::~Item()
{
    //std::cout << "Item destructor " << this << std::endl;
    if(specialDescription)
        delete specialDescription;
    if(text)
        delete text;

    if (readable)
        delete readable;
}

bool Item::canMovedTo(const Tile *tile) const
{
    if(tile)
    {
        int32_t objectstate = 0;

        if(isPickupable() || !isNotMoveable())
        {
            objectstate |= BLOCK_PICKUPABLE;
        }

        if(isBlocking())
        {
            objectstate |= BLOCK_SOLID;
        }

        return (tile->isBlocking(objectstate) == RET_NOERROR);
    }

    return false;
}

int32_t Item::unserialize(xmlNodePtr p)
{
    char *tmp;
    tmp = (char*)xmlGetProp(p, (const xmlChar *) "id");
    if(tmp)
    {
        id = atoi(tmp);
        xmlFreeOTSERV(tmp);
    }

    tmp = (char*)xmlGetProp(p, (const xmlChar *) "special_description");
    if(tmp)
    {
        specialDescription = new std::string(tmp);
        xmlFreeOTSERV(tmp);
    }

    tmp = (char*)xmlGetProp(p, (const xmlChar *) "text");
    if(tmp)
    {
        text = new std::string(tmp);
        xmlFreeOTSERV(tmp);
    }

    tmp = (char*)xmlGetProp(p, (const xmlChar *) "writer");
    if(tmp)
    {
        writer = new std::string(tmp);
        xmlFreeOTSERV(tmp);
    }

    tmp = (char*)xmlGetProp(p, (const xmlChar *) "count");
    if(tmp)
    {
        setItemCountOrSubtype(atoi(tmp));
        xmlFreeOTSERV(tmp);
    }

    tmp = (char*)xmlGetProp(p, (const xmlChar *) "actionId");
    if(tmp)
    {
        setActionId(atoi(tmp));
        xmlFreeOTSERV(tmp);
    }

    tmp = (char*)xmlGetProp(p, (const xmlChar *) "uniqueId");
    if(tmp)
    {
        setUniqueId(atoi(tmp));
        xmlFreeOTSERV(tmp);
    }

#ifdef YUR_RINGS_AMULETS
    tmp = (char*)xmlGetProp(p, (const xmlChar *) "charges");
    if(tmp)
    {
        charges = atoi(tmp);
        xmlFreeOTSERV(tmp);
    }

    tmp = (char*)xmlGetProp(p, (const xmlChar *) "time");
    if(tmp)
    {
        time = atoi(tmp);
        xmlFreeOTSERV(tmp);
    }
#endif //YUR_RINGS_AMULETS

    return 0;
}

xmlNodePtr Item::serialize()
{
    std::stringstream s;
    xmlNodePtr ret;
    ret = xmlNewNode(NULL,(const xmlChar*)"item");
    s.str(""); //empty the stringstream
    s << getID();
    xmlSetProp(ret, (const xmlChar*)"id", (const xmlChar*)s.str().c_str());

    if(specialDescription)
    {
        s.str(""); //empty the stringstream
        s << *specialDescription;
        xmlSetProp(ret, (const xmlChar*)"special_description", (const xmlChar*)s.str().c_str());
    }

    if(text)
    {
        s.str(""); //empty the stringstream
        s << *text;
        xmlSetProp(ret, (const xmlChar*)"text", (const xmlChar*)s.str().c_str());
    }
    if(writer && text)//No need to save if text is empty..
    {
        s.str(""); //empty the stringstream
        s << *writer;
        xmlSetProp(ret, (const xmlChar*)"writer", (const xmlChar*)s.str().c_str());
    }
    s.str(""); //empty the stringstream
    if(getItemCountOrSubtype() != 0)
    {
        s << getItemCountOrSubtype();
        xmlSetProp(ret, (const xmlChar*)"count", (const xmlChar*)s.str().c_str());
    }

    s.str("");
    if(actionId != 0)
    {
        s << actionId;
        xmlSetProp(ret, (const xmlChar*)"actionId", (const xmlChar*)s.str().c_str());
    }

    s.str("");
    if(uniqueId != 0)
    {
        s << uniqueId;
        xmlSetProp(ret, (const xmlChar*)"uniqueId", (const xmlChar*)s.str().c_str());
    }

#ifdef YUR_RINGS_AMULETS
    s.str("");
    if(charges != 0)
    {
        s << charges;
        xmlSetProp(ret, (const xmlChar*)"charges", (const xmlChar*)s.str().c_str());
    }

    s.str("");
    if (time != 0)
    {
        s << time;
        xmlSetProp(ret, (const xmlChar*)"time", (const xmlChar*)s.str().c_str());
    }
#endif //YUR_RINGS_AMULETS

    return ret;
}

bool Item::isBlocking() const
{
    const ItemType& it = items[id];
    return it.blockSolid;
}

bool Item::isStackable() const
{
    return items[id].stackable;
}

/*
bool Item::isMultiType() const {
	//return items[id].multitype;
	return (items[id].group == ITEM_GROUP_SPLASH);
}
*/

bool Item::isFluidContainer() const
{
    return (items[id].isFluidContainer());
}

#ifdef HUCZU_LOOT_INFO
bool Item::isContainer() const
{
    return items[id].isContainer();
}
#endif //HUCZU_LOOT_INFO

bool Item::isAlwaysOnTop() const
{
    return items[id].alwaysOnTop;
}

bool Item::isNotMoveable() const
{
    return !items[id].moveable;
}

bool Item::isGroundTile() const
{
    return items[id].isGroundTile();
}

bool Item::isSplash() const
{
    return items[id].isSplash();
}

bool Item::isPickupable() const
{
    return items[id].pickupable;
}

bool Item::isUseable() const
{
    return items[id].useable;
}

bool Item::floorChangeDown() const
{
    return items[id].floorChangeDown;
}

bool Item::floorChangeNorth() const
{
    return items[id].floorChangeNorth;
}
bool Item::floorChangeSouth() const
{
    return items[id].floorChangeSouth;
}
bool Item::floorChangeEast() const
{
    return items[id].floorChangeEast;
}
bool Item::floorChangeWest() const
{
    return items[id].floorChangeWest;
}

bool Item::isWeapon() const
{
    //now also returns true on SHIELDS!!! Check back with getWeponType!
    //old: return (items[id].weaponType != NONE && items[id].weaponType != SHIELD && items[id].weaponType != AMO);
    return (items[id].weaponType != NONE && items[id].weaponType != AMO);
}

WeaponType Item::getWeaponType() const
{
    return items[id].weaponType;
}

amu_t Item::getAmuType() const
{
    return items[id].amuType;
}

subfight_t Item::getSubfightType() const
{
    return items[id].shootType;
}

int32_t Item::getAttack() const
{
    return items[id].attack;
}

int32_t Item::getArmor() const
{
    return items[id].armor;
}

int32_t Item::getDefense() const
{
    return items[id].defence;
}

int32_t Item::getSlotPosition() const
{
    return items[id].slot_position;
}

double Item::getWeight() const
{
    if(isStackable())
    {
        return items[id].weight * std::max(1, (int32_t)count);
    }

    return items[id].weight;
}
#ifdef HUCZU_LOOT_INFO
std::string Item::getLootDescription() const
{
    std::stringstream s;
    std::string str;
    const Container* container;
    const ItemType& it = items[id];

    if(specialDescription)
    {
        s << (*specialDescription);
    }
    else if(it.name.length())
    {
        if(isStackable() && count > 1)
        {
            s << (int32_t)count << " " << it.name << "s";
        }
        else
        {
            if(isWeapon() && (getAttack() || getDefense()))
            {
                s << article(it.name) << " (Atk:" << (int32_t)getAttack() << " Def:" << (int32_t)getDefense() << ")";
            }
            else if(getArmor())
            {
                s << article(it.name) << " (Arm:"<< (int32_t)getArmor() << ")";
            }
            else if(isSplash())
            {
                s << article(it.name) << " of ";
                if(fluid == 0)
                {
                    s << items[1].name;
                }
                else
                {
                    s << items[fluid].name;
                }
            }
            else if(it.isKey())
            {
                s << article(it.name) << " (Key:" << actionId << ")";
            }
            else if(it.isContainer() && (container = dynamic_cast<const Container*>(this)))
            {
                s << article(it.name) << " (Vol:" << container->capacity() << ")";
            }
            else
            {
                s << article(it.name);
            }
        }
    }
    str = s.str();
    return str;
}
#endif //HUCZU_LOOT_INFO

std::string Item::getDescription(bool fullDescription) const
{
    std::stringstream s;
    std::string str;
    const Container* container;
    const ItemType& it = items[id];

    if(specialDescription)
    {
        s << (*specialDescription) << ".";

        if(fullDescription)
        {
            if(it.weight > 0)
                s << std::endl << "It weighs " << std::fixed << std::setprecision(1) << it.weight << " oz.";
        }
    }
    else if (it.name.length())
    {
        if(isStackable() && count > 1)
        {
            s << (int32_t)count << " " << it.name << "s.";

            if(fullDescription)
            {
                s << std::endl << "They weight " << std::fixed << std::setprecision(1) << ((double) count * it.weight) << " oz.";
            }
        }
        else
        {
            if(items[id].runeMagLevel != -1)
            {
                s << "a spell rune for level " << it.runeMagLevel << "." << std::endl;

                s << "It's an \"" << it.name << "\" spell (";
                if(getItemCharge())
                    s << (int32_t)getItemCharge();
                else
                    s << "1";
                s << "x)";
            }
            else if(isWeapon() && (getAttack() || getDefense()))
            {
                s << article(it.name) << " (Atk:" << (int32_t)getAttack() << " Def:" << (int32_t)getDefense() << ")";
            }
            else if(getArmor())
            {
                s << article(it.name) << " (Arm:"<< (int32_t)getArmor() << ")";
            }
            else if(isFluidContainer())
            {
                s << article(it.name);
                if(fluid == 0)
                {
                    if(actionId == 0)
                        s << ". To jest puste";
                    else
                        s << " of " << items[5].name << " Posiada " << actionId << " uzyc";
                }
                else
                {
                    s << " of " << items[fluid].name;
                    if(actionId > 0 && fluid == 5)
                        s << " Posiada " << actionId << " uzyc";
                }
            }
            else if(isSplash())
            {
                s << article(it.name) << " of ";
                if(fluid == 0)
                {
                    s << items[1].name;
                }
                else
                {
                    s << items[fluid].name;
                }
            }
            else if(it.isKey())
            {
                s << article(it.name) << " (Key:" << actionId << ")";
            }
            else if(it.isGroundTile()) //groundtile
            {
                s << it.name;
            }
            else if(items[id].isDoor && getActionId() > 1000)
            {
                s << article(it.name) << ". ";
                s << "\nPotrzebujesz " << getActionId() - 1000 << " poziomu by moc przejsc.";
            }
#ifdef YUR_RINGS_AMULETS
            else if (charges)
            {
                s << article(it.name) << ". ";
                if (charges == 1)
                    s << "\nIt has 1 charge left";
                else
                    s << "\nIt has " << charges << " charges left";
            }
            else if (time)
            {
                s << article(it.name) << ". ";
                if (time < 60*1000)
                    s << "\nIt has less than a minute left";
                else if (time == items[id].newTime)
                    s << "\nIt is brand new";
                else
                    s << "\nIt has " << (int32_t)ceil(time/(60.0*1000.0)) << " minutes left";
            }
#endif //YUR_RINGS_AMULETS

            else if(it.isContainer() && (container = dynamic_cast<const Container*>(this)))
            {
                s << article(it.name) << " (Vol:" << container->capacity() << ")";
            }
            else
            {
                s << article(it.name);

                if (readable)
                {
                    if (readable->empty())
                        s << "\nNothing is written on it";
                    else
                        s << "\nCzytasz: " << *readable;
                }
            }
            s << ".";
            if(fullDescription)
            {
                double weight = getWeight();
                if(weight > 0)
                    s << std::endl << "It weighs " << std::fixed << std::setprecision(1) << weight << " oz.";

                if(items[id].description.length())
                {
                    s << std::endl << items[id].description;
                }
            }
        }
    }
    else
        s<<"an item of type " << id <<".";

    str = s.str();
    return str;
}

std::string Item::getName() const
{
    return items[id].name;
}

void Item::setSpecialDescription(std::string desc)
{
    if(specialDescription)
    {
        delete specialDescription;
        specialDescription = NULL;
    }
    if(desc.length() > 1)
        specialDescription = new std::string(desc);
}

std::string Item::getSpecialDescription()
{
    if(!specialDescription)
        return std::string("");
    return *specialDescription;
}

void Item::clearSpecialDescription()
{
    if(specialDescription)
        delete specialDescription;
    specialDescription = NULL;
}

void Item::setText(std::string desc)
{
    if(text)
    {
        delete text;
        text = NULL;
    }
    if(desc.length() > 1)
    {
        text = new std::string(desc);
        if(items[id].readOnlyId != 0) //write 1 time
        {
            id = items[id].readOnlyId;
        }
    }
}

void Item::clearText()
{
    if(text)
        delete text;
    text = NULL;
}

std::string Item::getText()
{
    if(!text)
        return std::string("");
    return *text;
}

int32_t Item::getRWInfo() const
{
    return items[id].RWInfo;
}

bool Item::canDecay()
{
    if(isRemoved)
        return false;
    return items[id].canDecay;
}
//Teleport class
Teleport::Teleport(const uint16_t _type) : Item(_type)
{
    useCount = 0;
    destPos.x = 0;
    destPos.y = 0;
    destPos.z = 0;
}

Teleport::~Teleport()
{
}

int32_t Teleport::unserialize(xmlNodePtr p)
{
    Item::unserialize(p);
    char *tmp = (char*)xmlGetProp(p, (const xmlChar *) "destx");
    if(tmp)
    {
        destPos.x = atoi(tmp);
        xmlFreeOTSERV(tmp);
    }
    tmp = (char*)xmlGetProp(p, (const xmlChar *) "desty");
    if(tmp)
    {
        destPos.y = atoi(tmp);
        xmlFreeOTSERV(tmp);
    }
    tmp = (char*)xmlGetProp(p, (const xmlChar *) "destz");
    if(tmp)
    {
        destPos.z = atoi(tmp);
        xmlFreeOTSERV(tmp);
    }


    return 0;
}

xmlNodePtr Teleport::serialize()
{
    xmlNodePtr xmlptr = Item::serialize();

    std::stringstream s;

    s.str(""); //empty the stringstream
    s << (int32_t) destPos.x;
    xmlSetProp(xmlptr, (const xmlChar*)"destx", (const xmlChar*)s.str().c_str());

    s.str(""); //empty the stringstream
    s << (int32_t) destPos.y;
    xmlSetProp(xmlptr, (const xmlChar*)"desty", (const xmlChar*)s.str().c_str());

    s.str(""); //empty the stringstream
    s << (int32_t)destPos.z;
    xmlSetProp(xmlptr, (const xmlChar*)"destz", (const xmlChar*)s.str().c_str());

    return xmlptr;
}

int32_t Item::getWorth() const
{
    switch(getID())
    {
    case ITEM_COINS_GOLD:
        return getItemCountOrSubtype();
    case ITEM_COINS_PLATINUM:
        return getItemCountOrSubtype() * 100;
    case ITEM_COINS_CRYSTAL:
        return getItemCountOrSubtype() * 10000;
    case ITEM_COINS_SCARAB:
        return getItemCountOrSubtype() * 1000000;
    default:
        return 0;
    }
}

#ifdef YUR_RINGS_AMULETS
void Item::setGlimmer()
{
    switch (getID())
    {
    case ITEM_TIME_RING:
        setID(ITEM_TIME_RING_IN_USE);
        break;
    case ITEM_LIFE_RING:
        setID(ITEM_LIFE_RING_IN_USE);
        break;
    case ITEM_RING_OF_HEALING:
        setID(ITEM_RING_OF_HEALING_IN_USE);
        break;
    case ITEM_SWORD_RING:
        setID(ITEM_SWORD_RING_IN_USE);
        break;
    case ITEM_AXE_RING:
        setID(ITEM_AXE_RING_IN_USE);
        break;
    case ITEM_CLUB_RING:
        setID(ITEM_CLUB_RING_IN_USE);
        break;
    case ITEM_POWER_RING:
        setID(ITEM_POWER_RING_IN_USE);
        break;
    case ITEM_ENERGY_RING:
        setID(ITEM_ENERGY_RING_IN_USE);
        break;
    case ITEM_STEALTH_RING:
        setID(ITEM_STEALTH_RING_IN_USE);
        break;
    case ITEM_DWARVEN_RING:
        setID(ITEM_DWARVEN_RING_IN_USE);
        break;
    }
}

void Item::removeGlimmer()
{
    switch (getID())
    {
    case ITEM_TIME_RING_IN_USE:
        setID(ITEM_TIME_RING);
        break;
    case ITEM_LIFE_RING_IN_USE:
        setID(ITEM_LIFE_RING);
        break;
    case ITEM_RING_OF_HEALING_IN_USE:
        setID(ITEM_RING_OF_HEALING);
        break;
    case ITEM_SWORD_RING_IN_USE:
        setID(ITEM_SWORD_RING);
        break;
    case ITEM_AXE_RING_IN_USE:
        setID(ITEM_AXE_RING);
        break;
    case ITEM_CLUB_RING_IN_USE:
        setID(ITEM_CLUB_RING);
        break;
    case ITEM_POWER_RING_IN_USE:
        setID(ITEM_POWER_RING);
        break;
    case ITEM_ENERGY_RING_IN_USE:
        setID(ITEM_ENERGY_RING);
        break;
    case ITEM_STEALTH_RING_IN_USE:
        setID(ITEM_STEALTH_RING);
        break;
    case ITEM_DWARVEN_RING_IN_USE:
        setID(ITEM_DWARVEN_RING);
        break;
    }
}
#endif //YUR_RINGS_AMULETS

std::string Item::getOwner()
{
    if(!owner)
        return std::string("");
    return *owner;
}
void Item::setOwner(std::string name)
{
    if(owner)
    {
        delete owner;
        owner = NULL;
    }
    if(name.length() > 1)
    {
        owner = new std::string(name);
    }
}

int32_t Item::getOwnerTime()
{
    if(!ownerTime)
        return int("");
    return *ownerTime;
}

void Item::setOwnerTime(int32_t _time)
{
    if(ownerTime)
    {
        delete ownerTime;
        ownerTime = NULL;
    }
    if(_time > 1)
    {
        ownerTime = new int32_t(_time);
    }
}
std::string Item::getWriter()
{
    if(!writer)
        return std::string("");
    return *writer;
}

void Item::setWriter(std::string name)
{
    if(writer)
    {
        delete writer;
        writer = NULL;
    }
    if(name.length() > 1)
    {
        writer = new std::string(name);
    }
}
