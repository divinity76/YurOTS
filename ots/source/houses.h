#ifdef TLM_HOUSE_SYSTEM

#ifndef HOUSES_H
#define HOUSES_H

#include "position.h"
#include "map.h"
#include <string>
#include <vector>
#include <map>

enum rights_t
{
    HOUSE_NONE = 0,
    HOUSE_GUEST = 1,
    HOUSE_DOOROWNER = 2,
    HOUSE_SUBOWNER = 3,
    HOUSE_OWNER = 4
};

typedef std::vector<std::string> MembersList;
typedef std::map<Position, MembersList> DoorOwnersMap;

class House
{
private:
    std::string name;
    std::string file;
    std::string owner;
    Position frontDoor;
    MembersList subOwners;
    MembersList guests;
    DoorOwnersMap doorOwners;

public:
    std::vector<Position> tiles;
    House(std::string name);
    bool load();
    bool save();

    std::string getOwner() const;
    std::string getSubOwners() const;
    std::string getDoorOwners(const Position& pos) const;
    std::string getGuests() const;
    std::string getDescription() const;
#ifdef __KIRO_AKT__
    std::string getName() const
    {
        return name;
    }
#endif
    bool isBought();
    static int32_t getHouseSQM(std::string housename);
    int32_t checkHouseCount(Player* player);
    Position getFrontDoor() const;
    rights_t getPlayerRights(std::string nick);
    rights_t getPlayerRights(const Position& pos, std::string nick);

    void setOwner(std::string member);
    void setSubOwners(std::string members);
    void setDoorOwners(std::string members, const Position& pos);
    void setGuests(std::string members);
};

class Houses
{
private:
    static std::vector<House*> houses;

    static bool AddHouseTile(Game* game, House* house, const Position& pos);
    static bool LoadHouseItems(Game* game);
    static bool LoadContainer(xmlNodePtr nodeitem, Container* ccontainer);
    static bool SaveHouseItems(Game* game);
    static bool SaveContainer(xmlNodePtr nodeitem, Container* ccontainer);

public:
    static bool Load(Game* game);
    static bool Save(Game* game);
    static int32_t  cleanHouse(Game* game, uint32_t days);
};

#endif //HOUSES_H

#endif //TLM_HOUSE_SYSTEM
