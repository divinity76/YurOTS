//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// Summons
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
#ifdef TR_SUMMONS
#include "summons.h"
#include "luascript.h"
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <algorithm>

extern LuaScript g_config;

Summons::SummonMap Summons::summons;

int64_t Summons::getRequiredMana(std::string name)
{
	std::transform(name.begin(), name.end(), name.begin(), (int(*)(int))tolower);
	SummonMap::iterator iter = summons.find(name);

	if (iter != summons.end())
		return iter->second;
	else
		return -1;
}

bool Summons::Load()
{
	std::string file = g_config.getGlobalString("datadir") + "summons.xml";
	xmlDocPtr doc;

	doc = xmlParseFile(file.c_str());
	if (!doc)
		return false;

	xmlNodePtr root, summonNode;
	root = xmlDocGetRootElement(doc);

	if (xmlStrcmp(root->name, (const xmlChar*)"summons"))
	{
		xmlFreeDoc(doc);
		return false;
	}

	summonNode = root->children;
	while (summonNode)
	{
		if (strcmp((char*) summonNode->name, "summon") == 0)
		{
			std::string name = (const char*)xmlGetProp(summonNode, (const xmlChar *) "name");
			int64_t reqMana = atoll((const char*)xmlGetProp(summonNode, (const xmlChar *) "mana"));
			std::transform(name.begin(), name.end(), name.begin(), (int(*)(int))tolower);
			summons[name] = reqMana;
		}
		summonNode = summonNode->next;
	}

	xmlFreeDoc(doc);
	return true;
}
#endif //TR_SUMMONS
