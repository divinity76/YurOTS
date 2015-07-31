#ifndef __IOMAP_H
#define __IOMAP_H

#include <string>
#include "tile.h"
#include "item.h"

#include "map.h"
//class Map;

class IOMap
{
public:
    IOMap() {};
    virtual ~IOMap() {};
    //virtual char* getSourceDescription()=0;
    virtual bool loadMap(Map* map, std::string identifier)=0;
};

#endif
