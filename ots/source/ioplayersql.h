#ifndef __OTSERV_IOPLAYERSQL_H__
#define __OTSERV_IOPLAYERSQL_H__

#include <string>

#include "player.h"
#include "database.h"

class IOPlayerSQL
{
public:
    virtual ~IOPlayerSQL() {}
    static IOPlayerSQL* getInstance()
    {
        static IOPlayerSQL instance;
        return &instance;
    }
    /** Get a textual description of what source is used
    * \returns Name of the source*/
    virtual bool loadPlayer(Player* player, std::string name);

    /** Save a player
    * \returns Wheter the player was successfully saved
    * \param player the player to save
    */
    virtual bool savePlayer(Player* player);

    virtual bool getGuidByName(uint32_t& guid, std::string& name);
    virtual bool getNameByGuid(uint32_t guid, std::string& name);
    virtual bool playerExists(uint32_t guid, bool checkCache = true);
    virtual bool playerExists(std::string& name, bool checkCache = true);
    virtual bool resetGuildInformation(uint32_t guid);
    bool updateOnlineStatus(uint32_t guid, bool login);
    uint32_t getLevel(uint32_t guid) const;

protected:
    IOPlayerSQL() {}
    struct StringCompareCase
    {
        bool operator()(const std::string& l, const std::string& r) const
        {
            return strcasecmp(l.c_str(), r.c_str()) < 0;
        }
    };

    typedef std::map<std::string, uint32_t, StringCompareCase> GuidCacheMap;
    GuidCacheMap guidCacheMap;

    typedef std::map<uint32_t, std::string> NameCacheMap;
    NameCacheMap nameCacheMap;

    typedef std::pair<Container*, int32_t> containerStackPair;
    std::list<containerStackPair> stack;

    typedef std::map<int32_t, std::pair<Item*, int32_t> > ItemMap;

    bool storeNameByGuid(uint32_t guid);
};

#endif
