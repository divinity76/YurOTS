//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// Readables by Yurez
//////////////////////////////////////////////////////////////////////
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software Foundation,
// Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//////////////////////////////////////////////////////////////////////
#ifdef YUR_READABLES
#include "readables.h"
#include "game.h"
#include "luascript.h"
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

extern LuaScript g_config;
extern xmlMutexPtr xmlmutex;

bool Readables::Load(Game *game)
{
	std::string file = g_config.getGlobalString("datadir") + "readables.xml";
	xmlDocPtr doc;
	xmlMutexLock(xmlmutex);

	doc = xmlParseFile(file.c_str());
	if (!doc)
		return false;

	xmlNodePtr root, readableNode;
	root = xmlDocGetRootElement(doc);
	if (xmlStrcmp(root->name, (const xmlChar*)"readables"))
	{
		xmlFreeDoc(doc);
		xmlMutexUnlock(xmlmutex);
		return false;
	}

	readableNode = root->children;
	while (readableNode)
	{
		if (strcmp((char*) readableNode->name, "readable") == 0)
		{
			int x = atoi((const char*)xmlGetProp(readableNode, (const xmlChar *) "x"));
			int y = atoi((const char*)xmlGetProp(readableNode, (const xmlChar *) "y"));
			int z = atoi((const char*)xmlGetProp(readableNode, (const xmlChar *) "z"));
			std::string text = (const char*)xmlGetProp(readableNode, (const xmlChar *) "text");

			for (size_t i = 0; i < text.length()-1; i++)	// make real newlines
				if (text.at(i) == '\\' && text.at(i+1) == 'n')
				{
					text[i] = ' ';
					text[i+1] = '\n';
				}

			Tile* tile = game->getTile(x, y, z);
			if (tile)
			{
				Thing* thing = tile->getTopThing();
				Item* item = thing? dynamic_cast<Item*>(thing) : NULL;

				if (item)
					item->setReadable(text);
				else
				{
					std::cout << "\nTop thing at " << Position(x,y,z) << " is not an item!";
					return false;
				}
			}
			else
			{
				std::cout << "\nTile " << Position(x,y,z) << " is not valid!";
				return false;
			}
		}
		readableNode = readableNode->next;
	}

	xmlFreeDoc(doc);
	xmlMutexUnlock(xmlmutex);
	return true;
}
#endif //YUR_READABLES
