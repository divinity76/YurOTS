
#ifdef YUR_PVP_ARENA
//#include "preheaders.h"
#include "pvparena.h"
#include "game.h"
#include "luascript.h"

extern LuaScript g_config;


bool PvpArena::Load(Game* game)
{
    std::string file = g_config.DATA_DIR + "pvparenas.xml";
    xmlDocPtr doc;

    doc = xmlParseFile(file.c_str());
    if (!doc)
        return false;

    Position pos, exit;
    xmlNodePtr root, tileNode, arenaNode;

    root = xmlDocGetRootElement(doc);
    if (xmlStrcmp(root->name, (const xmlChar*)"pvparenas"))
    {
        xmlFreeDoc(doc);
        return false;
    }

    arenaNode = root->children;
    while (arenaNode)
    {
        if (strcmp((char*) arenaNode->name, "pvparena") == 0)
        {
            exit.x = atoi((const char*)xmlGetProp(arenaNode, (const xmlChar *) "exitx"));
            exit.y = atoi((const char*)xmlGetProp(arenaNode, (const xmlChar *) "exity"));
            exit.z = atoi((const char*)xmlGetProp(arenaNode, (const xmlChar *) "exitz"));

            tileNode = arenaNode->children;
            while (tileNode)
            {
                if (strcmp((char*) tileNode->name, "tile") == 0)
                {
                    pos.x = atoi((const char*)xmlGetProp(tileNode, (const xmlChar *) "x"));
                    pos.y = atoi((const char*)xmlGetProp(tileNode, (const xmlChar *) "y"));
                    pos.z = atoi((const char*)xmlGetProp(tileNode, (const xmlChar *) "z"));

                    if (!AddArenaTile(game, pos, exit))
                    {
                        xmlFreeDoc(doc);
                        return false;
                    }
                }
                else if (strcmp((const char*) tileNode->name, "tiles") == 0)
                {
                    int32_t fromx = atoi((const char*) xmlGetProp(tileNode, (const xmlChar*) "fromx"));
                    int32_t fromy = atoi((const char*) xmlGetProp(tileNode, (const xmlChar*) "fromy"));
                    int32_t fromz = atoi((const char*) xmlGetProp(tileNode, (const xmlChar*) "fromz"));
                    int32_t tox = atoi((const char*) xmlGetProp(tileNode, (const xmlChar*) "tox"));
                    int32_t toy = atoi((const char*) xmlGetProp(tileNode, (const xmlChar*) "toy"));
                    int32_t toz = atoi((const char*) xmlGetProp(tileNode, (const xmlChar*) "toz"));

                    if (fromx > tox) std::swap(fromx, tox);
                    if (fromy > toy) std::swap(fromy, toy);
                    if (fromz > toz) std::swap(fromz, toz);

                    for (int32_t x = fromx; x <= tox; x++)
                        for (int32_t y = fromy; y <= toy; y++)
                            for (int32_t z = fromz; z <= toz; z++)
                                if (!AddArenaTile(game, Position(x,y,z), exit))
                                {
                                    xmlFreeDoc(doc);
                                    return false;
                                }
                }
                tileNode = tileNode->next;
            }
        }
        arenaNode = arenaNode->next;
    }

    xmlFreeDoc(doc);
    return true;
}

bool PvpArena::AddArenaTile(Game* game, const Position& pos, const Position& exit)
{
    Tile* tile = game->getTile(pos);
    if (!tile)
    {
        std::cout << "\nTile " << pos << " is not valid!";
        return false;
    }

    if (tile->isPvpArena())
    {
        std::cout << "\nTile " << pos << " is already assigned to arena!";
        return false;
    }

    tile->setPvpArena(exit);
    return true;
}
#endif //YUR_PVP_ARENA
