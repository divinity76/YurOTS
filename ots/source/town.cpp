//town tworzone przez Huczu

#include "town.h"
#include "tools.h"
#include <algorithm>

Town::TownMap Town::town;

bool Town::loadTowns()
{
    xmlDocPtr doc;
    std::string file = "data/towns.xml";
    doc = xmlParseFile(file.c_str());
    Position pos;
    if(doc)
    {
        xmlNodePtr root, miasto;
        root = xmlDocGetRootElement(doc);
        if(xmlStrcmp(root->name, (const xmlChar*)"towns"))
        {
            xmlFreeDoc(doc);
            return false;
        }
        miasto = root->children;
        while(miasto)
        {
            if(strcmp((char*) miasto->name, "town") == 0)
            {
                uint32_t id = atoi((const char*)xmlGetProp(miasto, (const xmlChar *) "id"));
                std::string name = (const char*)xmlGetProp(miasto, (const xmlChar *) "name");
                toLowerCaseString(name);
                pos.x = atoi((const char*)xmlGetProp(miasto, (const xmlChar *) "x"));
                pos.y = atoi((const char*)xmlGetProp(miasto, (const xmlChar *) "y"));
                pos.z = atoi((const char*)xmlGetProp(miasto, (const xmlChar *) "z"));
                uint16_t premmy = atoi((const char*)xmlGetProp(miasto, (const xmlChar *) "premium"));
                town[id].name = name;
                town[id].pos = pos;
                town[id].premium = premmy;
            }
            miasto = miasto->next;
        }
        xmlFreeDoc(doc);
        return true;
    }
    return false;
}
// na razie nic nie robimy, ale potem kto wie...
