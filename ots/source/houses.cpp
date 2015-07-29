//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// Houses by Yurez (based on code from TLM)
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
#ifdef TLM_HOUSE_SYSTEM
#include "houses.h"
#include "luascript.h"
#include "game.h"
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <sstream>
#include <algorithm>
#include <boost/tokenizer.hpp>

extern LuaScript g_config;
extern xmlMutexPtr xmlmutex;
std::vector<House*> Houses::houses;

House::House(std::string name)
{
	this->name = name;
	file = g_config.DATA_DIR + "houses/" + name + ".xml";
}

bool House::load()
{
	xmlDocPtr doc;
	doc = xmlParseFile(file.c_str());

	if (doc)
	{
		xmlNodePtr root, tmp;
		root = xmlDocGetRootElement(doc); 

		if (xmlStrcmp(root->name, (const xmlChar*)"house"))
		{
			xmlFreeDoc(doc);
			return false;
		}

		tmp = root->children;
		while (tmp)
		{
			if (!strcmp((const char*)tmp->name, "frontdoor"))
			{
				frontDoor.x = atoi((const char*)xmlGetProp(tmp, (const xmlChar *)"x"));
				frontDoor.y = atoi((const char*)xmlGetProp(tmp, (const xmlChar *)"y"));
				frontDoor.z = atoi((const char*)xmlGetProp(tmp, (const xmlChar *)"z"));
			}
			else if (!strcmp((const char*)tmp->name, "owner"))
			{
				owner = (const char*)xmlGetProp(tmp, (const xmlChar *)"name");
			}
			else if (!strcmp((const char*)tmp->name, "subowner"))
			{
				subOwners.push_back((const char*)xmlGetProp(tmp, (const xmlChar *)"name"));
			}
			else if (!strcmp((const char*)tmp->name, "guest"))
			{
				guests.push_back((const char*)xmlGetProp(tmp, (const xmlChar *)"name"));
			}
			else if (!strcmp((const char*)tmp->name, "doorowner"))
			{
				Position pos;
				pos.x = atoi((const char*)xmlGetProp(tmp, (const xmlChar *)"x"));
				pos.y = atoi((const char*)xmlGetProp(tmp, (const xmlChar *)"y"));
				pos.z = atoi((const char*)xmlGetProp(tmp, (const xmlChar *)"z"));
				doorOwners[pos].push_back((const char*)xmlGetProp(tmp, (const xmlChar *)"name"));
			}
			tmp = tmp->next;
		}

		xmlFreeDoc(doc);
		return true;
	}

	return false;
}

bool House::save()
{
	xmlDocPtr doc;
	xmlNodePtr root, tmp;
	xmlMutexLock(xmlmutex);

	doc = xmlNewDoc((const xmlChar*)"1.0");
	doc->children = xmlNewDocNode(doc, NULL, (const xmlChar*)"house", NULL);
    root = doc->children;

	std::stringstream sb;
	tmp = xmlNewNode(NULL, (const xmlChar*)"frontdoor");
	sb << frontDoor.x; xmlSetProp(tmp, (const xmlChar*) "x", (const xmlChar*)sb.str().c_str()); sb.str("");
	sb << frontDoor.y; xmlSetProp(tmp, (const xmlChar*) "y", (const xmlChar*)sb.str().c_str()); sb.str("");
	sb << frontDoor.z; xmlSetProp(tmp, (const xmlChar*) "z", (const xmlChar*)sb.str().c_str()); sb.str("");
	xmlAddChild(root, tmp);

	tmp = xmlNewNode(NULL, (const xmlChar*)"owner");
	xmlSetProp(tmp, (const xmlChar*)"name", (const xmlChar*)owner.c_str());
	xmlAddChild(root, tmp);

	for (size_t i = 0; i < subOwners.size(); i++)
	{
		tmp = xmlNewNode(NULL, (const xmlChar*)"subowner");
		xmlSetProp(tmp, (const xmlChar*)"name", (const xmlChar*)subOwners[i].c_str());
		xmlAddChild(root, tmp);
	}

	for (size_t i = 0; i < guests.size(); i++)
	{
		tmp = xmlNewNode(NULL, (const xmlChar*)"guest");
		xmlSetProp(tmp, (const xmlChar*)"name", (const xmlChar*)guests[i].c_str());
		xmlAddChild(root, tmp);
	}

	DoorOwnersMap::const_iterator iter = doorOwners.begin();
	while (iter != doorOwners.end())
	{
		Position pos = iter->first;
		for (size_t i = 0; i < iter->second.size(); i++)
		{
			tmp = xmlNewNode(NULL, (const xmlChar*)"doorowner");	// TODO: optimize
			sb << pos.x; xmlSetProp(tmp, (const xmlChar*) "x", (const xmlChar*)sb.str().c_str()); sb.str("");
			sb << pos.y; xmlSetProp(tmp, (const xmlChar*) "y", (const xmlChar*)sb.str().c_str()); sb.str("");
			sb << pos.z; xmlSetProp(tmp, (const xmlChar*) "z", (const xmlChar*)sb.str().c_str()); sb.str("");
			xmlSetProp(tmp, (const xmlChar*)"name", (const xmlChar*)iter->second[i].c_str());
			xmlAddChild(root, tmp);
		}
		++iter;
	}

	xmlSaveFile(file.c_str(), doc);
    xmlFreeDoc(doc);
	xmlMutexUnlock(xmlmutex);
	return true;
}

std::string House::getOwner() const
{
	return owner;
}

std::string House::getGuests() const
{
	std::string members;
	for (size_t i = 0; i < guests.size(); i++)
		members += (guests[i] + "\n");
	return members;
}

std::string House::getSubOwners() const
{
	std::string members;
	for (size_t i = 0; i < subOwners.size(); i++)
		members += (subOwners[i] + "\n");
	return members;
}

std::string House::getDoorOwners(const Position& doorPos) const
{
	std::string members;
	DoorOwnersMap::const_iterator iter = doorOwners.find(doorPos);

	if (iter != doorOwners.end())
		for (size_t i = 0; i < iter->second.size(); i++)
			members += (iter->second[i] + "\n");
	return members;
}

Position House::getFrontDoor() const
{
	return frontDoor;
}

std::string House::getDescription() const
{
	std::stringstream doortext;
	doortext << "You see a door.\n It belongs to house '" << name << "'.\n " << 
		(owner.empty()? "Nobody" : owner) << " owns this house.";	
	return doortext.str();
}

rights_t House::getPlayerRights(std::string nick)	// ignores door-owners
{
	rights_t rights = HOUSE_NONE;
	if (nick == owner)
		rights = HOUSE_OWNER;
	else if (std::find(subOwners.begin(), subOwners.end(), nick) != subOwners.end())
		rights = HOUSE_SUBOWNER;
	else if (std::find(guests.begin(), guests.end(), nick) != guests.end())
		rights = HOUSE_GUEST;
	return rights;
}

rights_t House::getPlayerRights(const Position& pos, std::string nick)
{
	rights_t rights = HOUSE_NONE;
	if (nick == owner)
		rights = HOUSE_OWNER;
	else if (std::find(subOwners.begin(), subOwners.end(), nick) != subOwners.end())
		rights = HOUSE_SUBOWNER;
	else
	{
		DoorOwnersMap::const_iterator iter = doorOwners.find(pos);
		if (iter != doorOwners.end())
			if (std::find(doorOwners[pos].begin(), doorOwners[pos].end(), nick) != doorOwners[pos].end())
				rights = HOUSE_DOOROWNER;

		if (rights == HOUSE_NONE)
		{
			if (std::find(guests.begin(), guests.end(), nick) != guests.end())
				rights = HOUSE_GUEST;
		}
	}
	return rights;
}

void House::setOwner(std::string member)
{
	if (member.empty())
		owner.clear();
	else
	{
		boost::tokenizer<> tokens(member, boost::char_delimiters_separator<char>(false, NULL, "\n"));
		for (boost::tokenizer<>::iterator tok = tokens.begin(); tok != tokens.end(); ++tok)
		{
			owner = *tok;	// pick first name from the list
			return;
		}
	}
}

void House::setSubOwners(std::string members)
{
	subOwners.clear();
	boost::tokenizer<> tokens(members, boost::char_delimiters_separator<char>(false, NULL, "\n"));
	for (boost::tokenizer<>::iterator tok = tokens.begin(); tok != tokens.end(); ++tok)
		subOwners.push_back(*tok);
}

void House::setDoorOwners(std::string members, const Position& pos)
{
	doorOwners[pos].clear();
	boost::tokenizer<> tokens(members, boost::char_delimiters_separator<char>(false, NULL, "\n"));
	for (boost::tokenizer<>::iterator tok = tokens.begin(); tok != tokens.end(); ++tok)
		doorOwners[pos].push_back(*tok);
}

void House::setGuests(std::string members)
{
	guests.clear();
	boost::tokenizer<> tokens(members, boost::char_delimiters_separator<char>(false, NULL, "\n"));
	for (boost::tokenizer<>::iterator tok = tokens.begin(); tok != tokens.end(); ++tok)
		guests.push_back(*tok);
}

bool Houses::Load(Game* game)
{
	if (!LoadHouseItems(game))
		return false;

	std::string file = g_config.DATA_DIR + "houses.xml";
	xmlDocPtr doc;
	doc = xmlParseFile(file.c_str());

	if (doc)
	{
		xmlNodePtr root, houseNode, tileNode;
		root = xmlDocGetRootElement(doc);
		if (xmlStrcmp(root->name, (const xmlChar*)"houses")) 
		{
			xmlFreeDoc(doc);
			return false;
		}

		houseNode = root->children;
		while (houseNode)
		{
			if (strcmp((char*) houseNode->name, "house") == 0)
			{
				std::string name = (const char*)xmlGetProp(houseNode, (const xmlChar *) "name");
				House* house = new House(name);

				if (!house->load())
				{
					xmlFreeDoc(doc);
					return false;
				}

				houses.push_back(house);
				tileNode = houseNode->children;

				while (tileNode)
				{            
					if (strcmp((const char*) tileNode->name, "tile") == 0)
					{          
						int tx = atoi((const char*) xmlGetProp(tileNode, (const xmlChar*) "x"));
						int ty = atoi((const char*) xmlGetProp(tileNode, (const xmlChar*) "y"));
						int tz = atoi((const char*) xmlGetProp(tileNode, (const xmlChar*) "z"));	

						if (!AddHouseTile(game, house, Position(tx, ty, tz)))
						{
							xmlFreeDoc(doc);
							return false;
						}
					}
					else if (strcmp((const char*) tileNode->name, "tiles") == 0)
					{
						int fromx = atoi((const char*) xmlGetProp(tileNode, (const xmlChar*) "fromx"));
						int fromy = atoi((const char*) xmlGetProp(tileNode, (const xmlChar*) "fromy"));
						int fromz = atoi((const char*) xmlGetProp(tileNode, (const xmlChar*) "fromz"));
						int tox = atoi((const char*) xmlGetProp(tileNode, (const xmlChar*) "tox"));
						int toy = atoi((const char*) xmlGetProp(tileNode, (const xmlChar*) "toy"));
						int toz = atoi((const char*) xmlGetProp(tileNode, (const xmlChar*) "toz"));

						if (fromx > tox) std::swap(fromx, tox);
						if (fromy > toy) std::swap(fromy, toy);
						if (fromz > toz) std::swap(fromz, toz);

						for (int x = fromx; x <= tox; x++)
							for (int y = fromy; y <= toy; y++)
								for (int z = fromz; z <= toz; z++)
									if (!AddHouseTile(game, house, Position(x,y,z)))
									{
										xmlFreeDoc(doc);
										return false;
									}								
					}
					tileNode = tileNode->next;
				}

				Tile* doorTile = game->getTile(house->getFrontDoor());		// include front door
				if (doorTile && !doorTile->isHouse())
					doorTile->setHouse(house);
			}
			houseNode = houseNode->next;
		}
		xmlFreeDoc(doc);
		return true;
	}

	return false;
}

bool Houses::AddHouseTile(Game* game, House* house, const Position& pos)
{
	Tile* tile = game->getTile(pos);
	if (!tile)
	{
		std::cout << "\nTile " << pos << " is not valid!";
		return false;
	}

	if (!tile->setHouse(house))
	{
		std::cout << "\nTile " << pos << " is already assigned to a house!";
		return false;
	}

	house->tiles.push_back(pos);
	return true;
}

bool Houses::Save(Game* game)
{
	for (size_t i = 0; i < houses.size(); i++)
		if (!houses[i]->save())
			return false;

	return SaveHouseItems(game);
}

bool Houses::LoadHouseItems(Game* game)
{
	xmlDocPtr doc;
	doc = xmlParseFile((g_config.DATA_DIR + "houseitems.xml").c_str());

	if (doc)
	{
		xmlNodePtr root, tileNode, itemNode;
		root = xmlDocGetRootElement(doc);
		if (xmlStrcmp(root->name, (const xmlChar*)"houseitems"))
		{
			xmlFreeDoc(doc);
			return false;
		}

		tileNode = root->children;
		while (tileNode)
		{
			if (strcmp((const char*)tileNode->name, "tile") == 0)
			{
				int x = atoi((const char*) xmlGetProp(tileNode, (const xmlChar*) "x"));
				int y = atoi((const char*) xmlGetProp(tileNode, (const xmlChar*) "y"));
				char z = atoi((const char*) xmlGetProp(tileNode, (const xmlChar*) "z"));

				Tile *tile = game->getTile(x, y, z);
				if (tile)
				{
					itemNode = tileNode->children;
					while (itemNode)
					{
						if (strcmp((const char*)itemNode->name, "item") == 0)
						{
							int itemid = atoi((const char*) xmlGetProp(itemNode, (const xmlChar*) "id"));
							Item* item = Item::CreateItem(itemid);

							if (item)
							{
								item->unserialize(itemNode);
								item->pos = Position(x, y, z);
								tile->addThing(item);

								Container *container = dynamic_cast<Container*>(item);
								if (container)
									LoadContainer(itemNode, container);			
							}
						}
						itemNode = itemNode->next;
					}
				}
			}
			tileNode = tileNode->next;

		}
		xmlFreeDoc(doc);
		return true;
	}
	return false;
}

bool Houses::LoadContainer(xmlNodePtr nodeitem, Container* ccontainer)
{
	xmlNodePtr tmp,p;
	if (!nodeitem)
		return false;

	tmp = nodeitem->children;
	if (!tmp)
		return false;
	                 
	if (strcmp((const char*)tmp->name, "inside") == 0)
	{
		p=tmp->children;
		while(p)
		{  
			int id = atoi((const char*)xmlGetProp(p, (const xmlChar *)"id"));
			Item* myitem = Item::CreateItem(id);
			myitem->unserialize(p);  
			ccontainer->addItem(myitem);
			 
			Container* in_container = dynamic_cast<Container*>(myitem);
			if (in_container)
				LoadContainer(p,in_container);
			p = p->next;
		}
		return true;
	}
	return false;
}

bool Houses::SaveHouseItems(Game* game)
{
	std::string filename = g_config.DATA_DIR + "houseitems.xml";
	std::stringstream sb;
	   
	xmlDocPtr doc;
	xmlNodePtr root, tileNode, itemNode;
	xmlMutexLock(xmlmutex);

	doc = xmlNewDoc((const xmlChar*)"1.0");
	doc->children = xmlNewDocNode(doc, NULL, (const xmlChar*)"houseitems", NULL);
	root = doc->children;

	for (size_t i = 0; i < houses.size(); i++)
	{
		for (size_t j = 0; j < houses[i]->tiles.size(); j++)
		{
			Position& pos = houses[i]->tiles[j];
			Tile *housetile = game->getTile(pos.x, pos.y, pos.z);
			bool firstItem = true;
					
			for (int i = housetile->getThingCount(); i >= 0; i--)
			{
				Item* item = dynamic_cast<Item*>(housetile->getThingByStackPos(i));
				if (item && !item->isNotMoveable()) 
				{
					if (firstItem)
					{
						tileNode = xmlNewNode(NULL, (const xmlChar*)"tile");
						sb << pos.x; xmlSetProp(tileNode, (const xmlChar*) "x", (const xmlChar*)sb.str().c_str()); sb.str("");
						sb << pos.y; xmlSetProp(tileNode, (const xmlChar*) "y", (const xmlChar*)sb.str().c_str()); sb.str("");
						sb << pos.z; xmlSetProp(tileNode, (const xmlChar*) "z", (const xmlChar*)sb.str().c_str()); sb.str("");
						firstItem = false;
					}

					itemNode = item->serialize();
					Container *container = dynamic_cast<Container*>(item);

					if (container)
						SaveContainer(itemNode, container);

					xmlAddChild(tileNode, itemNode);
				}
			}

			if (!firstItem)
				xmlAddChild(root, tileNode);
		}
	}
 
	if (xmlSaveFile(filename.c_str(), doc))
	{
		xmlFreeDoc(doc);
		xmlMutexUnlock(xmlmutex);
		return true;
	}
	else
	{
		std::cout << "Could not save " << filename << "!" << std::endl;
		xmlFreeDoc(doc);
		xmlMutexUnlock(xmlmutex);
		return false;
	}
}

bool Houses::SaveContainer(xmlNodePtr nodeitem, Container* ccontainer)
{
	xmlNodePtr pn,nn;
	std::stringstream sb;
	if (ccontainer->size() != 0)
	{
		pn = xmlNewNode(NULL,(const xmlChar*)"inside");
		for(int i = ccontainer->size()-1; i >= 0; i--)
		{
			Item * citem = ccontainer->getItem(i);
			nn = citem->serialize();
			Container* in_container = dynamic_cast<Container*>(citem);
			 
			if (in_container)
				SaveContainer(nn,in_container);	
			xmlAddChild(pn, nn);
		}
		xmlAddChild(nodeitem, pn);
	}
	return true;
}
#endif //TLM_HOUSE_SYSTEM
