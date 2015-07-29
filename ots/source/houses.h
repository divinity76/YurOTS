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

#ifndef HOUSES_H
#define HOUSES_H

#include "position.h"
#include "map.h"
#include <string>
#include <vector>
#include <map>

enum rights_t {
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
};

#endif //HOUSES_H

#endif //TLM_HOUSE_SYSTEM
