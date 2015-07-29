//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// Guilds by Yurez
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
#ifdef YUR_GUILD_SYSTEM

#ifndef GUILDS_H
#define GUILDS_H
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <string>
#include <vector>
#include <map>
class Player;

enum gstat_t {
	GUILD_NONE,
	GUILD_INVITED,
	GUILD_MEMBER,
	GUILD_VICE,
	GUILD_LEADER
};

class Guilds
{
private:
	class Guild
	{
	private:
		struct Member
		{
			gstat_t status;
			std::string name;
			std::string rank;
			std::string nick;
		};
		unsigned long gid;
		static unsigned long counter;
		std::string gname;
		std::vector<Member> members;

	public:
		Guild(std::string name);
		void save(xmlNodePtr guildNode);
		std::string getName() const;
		void addMember(std::string name, gstat_t status, std::string rank, std::string nick);
		gstat_t getGuildStatus(std::string name) const;
		bool setGuildStatus(std::string name, gstat_t status);
		void setGuildInfo(std::string name, gstat_t status, std::string rank);
		bool clearGuildInfo(std::string name);
		bool setGuildNick(std::string name, std::string nick);
		bool reloadGuildInfo(Player* player);
	};

	static std::vector<Guild*> guilds;

public:
	static bool Load();
	static bool Save();
	static bool AddNewGuild(std::string gname);
	static void DeleteGuild(std::string gname);
	static std::string GetGuildName(std::string name);
	static gstat_t GetGuildStatus(std::string name);
	static void SetGuildStatus(std::string name, gstat_t status);
	static void SetGuildInfo(std::string name, gstat_t status, std::string rank, std::string gname);
	static void ClearGuildInfo(std::string name);
	static void SetGuildNick(std::string name, std::string nick);
	static void ReloadGuildInfo(Player* player);
};

#endif //GUILDS_H

#endif //YUR_GUILD_SYSTEM
