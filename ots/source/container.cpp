
#include "container.h"
#ifdef HUCZU_LOOT_INFO
#include <sstream>
#endif //HUCZU_LOOT_INFO
Container::Container(const uint16_t _type) : Item(_type)
{
    //std::cout << "Container constructor " << this << std::endl;
    maxitems = items[this->getID()].maxItems;
    actualitems = 0;
    parent = NULL;
    depot = 0;
    useCount = 0;
}

Container::~Container()
{
    //std::cout << "Container destructor " << this << std::endl;
    for(ContainerList::iterator cit = lcontained.begin(); cit != lcontained.end(); ++cit)
    {
        Container* container = dynamic_cast<Container*>(*cit);
        if(container)
            container->setParent(NULL);
        (*cit)->releaseThing();
    }

    lcontained.clear();
}

bool Container::addItem(Item *newitem)
{
    //first check if we are a container, there is an item to be added and if we can add more items...
    //if (!iscontainer) throw TE_NoContainer();
    if (newitem == NULL)
        return false;

    // seems we should add the item...
    // new items just get placed in front of the items we already have...
    if(lcontained.size() < maxitems)
    {
        newitem->pos.x = 0xFFFF;

        //FIXME: is this correct? i dont get what it does. tliff
        Container* container = dynamic_cast<Container*>(newitem);
        if(container)
        {
            container->setParent(this);
        }

        lcontained.push_front(newitem);

        // increase the itemcount
        ++actualitems;
        return true;
    }

    return false;
}

bool Container::removeItem(Item* item)
{
    for (ContainerList::iterator cit = lcontained.begin(); cit != lcontained.end(); ++cit)
    {
        if((*cit) == item)
        {

            Container* container = dynamic_cast<Container*>(*cit);
            if(container)
            {
                container->setParent(NULL);
            }

            lcontained.erase(cit);
            --actualitems;
            return true;
        }
    }

    return false;
}

void Container::moveItem(uint16_t from_slot, unsigned char to_slot)
{
    int32_t n = 0;
    for (ContainerList::iterator cit = lcontained.begin(); cit != lcontained.end(); ++cit)
    {
        if(n == from_slot)
        {
            Item *item = (*cit);
            lcontained.erase(cit);
            lcontained.push_front(item);
            break;
        }
        ++n;
    }
}

Item* Container::getItem(uint32_t slot_num)
{
    size_t n = 0;
    for (ContainerList::const_iterator cit = getItems(); cit != getEnd(); ++cit)
    {
        if(n == slot_num)
            return *cit;
        else
            ++n;
    }

    return NULL;
}

const Item* Container::getItem(uint32_t slot_num) const
{
    size_t n = 0;
    for (ContainerList::const_iterator cit = getItems(); cit != getEnd(); ++cit)
    {
        if(n == slot_num)
            return *cit;
        else
            ++n;
    }

    return NULL;
}

unsigned char Container::getSlotNumberByItem(const Item* item) const
{
    unsigned char n = 0;
    for (ContainerList::const_iterator cit = getItems(); cit != getEnd(); ++cit)
    {
        if(*cit == item)
            return n;
        else
            ++n;
    }

    return 0xFF;
}

int32_t Container::getItemHoldingCount() const
{
    int32_t holdcount = 0;

    std::list<const Container*> stack;
    stack.push_back(this);

    ContainerList::const_iterator it;

    while(!stack.empty())
    {
        const Container *container = stack.front();
        stack.pop_front();

        for (it = container->getItems(); it != container->getEnd(); ++it)
        {
            Container *container = dynamic_cast<Container*>(*it);
            if(container)
            {
                stack.push_back(container);
            }

            ++holdcount;
        }
    }

    return holdcount;
}

bool Container::isHoldingItem(const Item* item) const
{
    std::list<const Container*> stack;
    stack.push_back(this);

    ContainerList::const_iterator it;

    while(!stack.empty())
    {
        const Container *container = stack.front();
        stack.pop_front();

        for (it = container->getItems(); it != container->getEnd(); ++it)
        {

            if(*it == item)
            {
                return true;
            }

            Container *containerIt = dynamic_cast<Container*>(*it);
            if(containerIt)
            {
                stack.push_back(containerIt);
            }
        }
    }

    return false;
}

double Container::getWeight() const
{
    double weight = items[id].weight;
    std::list<const Container*> stack;

    ContainerList::const_iterator it;
    stack.push_back(this);

    while(!stack.empty())
    {
        const Container *container = stack.front();
        stack.pop_front();

        for (it = container->getItems(); it != container->getEnd(); ++it)
        {
            Container *container = dynamic_cast<Container*>(*it);
            if(container)
            {
                stack.push_back(container);
                weight += items[container->getID()].weight;
            }
            else
                weight += (*it)->getWeight();
        }
    }

    return weight;
}

ContainerList::const_iterator Container::getItems() const
{
    return lcontained.begin();
}

ContainerList::const_iterator Container::getEnd() const
{
    return lcontained.end();
}

Container *Container::getTopParent()
{
    if(getParent() == NULL)
        return this;

    Container *aux = this->getParent();
    while(aux->getParent() != NULL)
    {
        aux = aux->getParent();
    }
    return aux;
}

const Container *Container::getTopParent() const
{
    if(getParent() == NULL)
        return this;

    Container *aux = this->getParent();

    while(aux->getParent() != NULL)
    {
        aux = aux->getParent();
    }
    return aux;
}

#ifdef HUCZU_LOOT_INFO
std::string Container::getContentDescription()
{
    std::stringstream s;
    return getContentDescription(s).str();
}

std::stringstream& Container::getContentDescription(std::stringstream& s)
{
    bool begin = true;
    Container* container = dynamic_cast<Container*>(this);
    ContainerList::const_iterator it;
    ContainerList::const_iterator cit;
    for(it = container->getItems(); it != container->getEnd(); ++it)
    {

        if(!begin)
            s << ", ";
        else
            begin = false;

        s << (*it)->getLootDescription();

        if((*it)->isContainer())
        {
            Container* containerek = dynamic_cast<Container*>(*it);
            for(cit = containerek->getItems(); cit != containerek->getEnd(); ++cit)
            {

                if(!begin)
                    s << ", ";
                else
                    begin = false;

                s << (*cit)->getLootDescription();
            }
        }
    }
    if(begin)
        s << "nic";

    return s;
}
#endif //HUCZU_LOOT_INFO
