
#ifndef __TEMPLATES_H__
#define __TEMPLATES_H__

#include <set>
#include <map>

#include "creature.h"
#include "otsystem.h"

template<class T> class AutoList
{
public:
    AutoList() {}

    ~AutoList()
    {
        list.clear();
    }

    void addList(T* t)
    {
        list[t->getID()] = t;
    }

    void removeList(uint32_t _id)
    {
        list.erase(_id);
    }

    typedef std::map<uint32_t, T*> list_type;
    list_type list;

    typedef typename list_type::iterator listiterator;
};

class AutoID
{
public:
    AutoID()
    {
        OTSYS_THREAD_LOCK_CLASS lockClass(autoIDLock);
        count++;
        if(count >= 0xFFFFFF)
            count = 1000;

        while(list.find(count) != list.end())
        {
            if(count >= 0xFFFFFF)
                count = 1000;
            else
                count++;
        }
        list.insert(count);
        auto_id = count;
    }
    virtual ~AutoID()
    {
        list_type::iterator it = list.find(auto_id);
        if(it != list.end())
            list.erase(it);
    }

    typedef std::set<uint32_t> list_type;

    uint32_t auto_id;
    static OTSYS_THREAD_LOCKVAR autoIDLock;

protected:
    static uint32_t count;
    static list_type list;
};
#endif
