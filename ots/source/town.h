//town tworzone przez Huczu

#ifndef TOWN_H
#define TOWN_H

#include "position.h"

#include <string>
#include <map>

class Town
{
public:

    static bool loadTowns();
    struct miasto
    {
        std::string name;
        Position pos;
        uint16_t premium;
    };
    typedef std::map<uint32_t, struct miasto> TownMap;
    static TownMap town;

private:

};

#endif //TOWN_H
