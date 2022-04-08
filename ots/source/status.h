//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// Status-Singleton for OTServ
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

#ifndef __OTSERV_STATUS_H
#define __OTSERV_STATUS_H

#include <string>
#ifdef HHB_STATUS_MAX_4_PER_IP
#include <map>
#include <shared_mutex>
#endif
#include "otsystem.h"
#include "definitions.h"
#include "networkmessage.h"


class Status{
  public:
  // procs       
	void addPlayer(
#ifdef HHB_STATUS_MAX_4_PER_IP
		const uint32_t player_ip
#endif
	);
	void removePlayer(
#ifdef HHB_STATUS_MAX_4_PER_IP
		const uint32_t player_ip
#endif
	);
	static Status* instance();
	std::string getStatusString();
	void getInfo(NetworkMessage &nm);
	bool hasSlot();
	
	// vars
	int playersonline, playersmax, playerspeak;
	std::string ownername, owneremail;
	std::string motd;
	std::string mapname, mapauthor;
	int mapsizex, mapsizey;
	std::string servername, location, url;
	std::string version;
	uint64_t start;

#ifdef YUR_LOGIN_QUEUE
	int playerswaiting;
#endif //YUR_LOGIN_QUEUE

  private:
	Status();
	static Status* _Status;
#ifdef HHB_STATUS_MAX_4_PER_IP
	std::map<uint32_t,size_t> ip_counts;
	std::shared_mutex ip_counts_mutex;	
#endif
	// the stats of our server
};

#endif
