//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// otserv main. The only place where things get instantiated.
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

#include "definitions.h"

#include <string>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <ctime>

#include "otsystem.h"
#include "networkmessage.h"
#include "protocol76.h"

#include <stdlib.h>
#include <time.h>
#include "game.h"

#include "ioaccount.h"
#include "ioplayer.h"

#include "status.h"
#include "spells.h"
#include "monsters.h"
#include "actions.h"
#include "commands.h"

#include "luascript.h"
#include "account.h"

#include "tools.h"
#include "md5.h"

#include "summons.h"
#include "pvparena.h"
#include "readables.h"
#include "aac.h"

#ifdef __OTSERV_ALLOCATOR__
#include "allocator.h"
#endif

#ifdef WIN32
	#define ERROR_EINTR WSAEINTR
#else
	#include <fcntl.h>
	#include <arpa/inet.h>
	#include <signal.h>
	#include <errno.h>

	#define SOCKET_ERROR -1
	#define ERROR_EINTR EINTR

	/* Comment below line if you want to execute otserv with root user (NOT RECOMMENDED) */
	#define _NO_ROOT_PERMISSION_
	#define _HOMEDIR_CONF_
	
	extern int errno; 
#endif

#ifdef __DEBUG_CRITICALSECTION__
OTSYS_THREAD_LOCK_CLASS::LogList OTSYS_THREAD_LOCK_CLASS::loglist;
#endif

#ifdef MSVC_EXCEPTION_TRACER
#include "mdump.h"
MiniDumper miniDumper("YurOTS");
#endif //MSVC_EXCEPTION_TRACER

std::vector< std::pair<unsigned long, unsigned long> > serverIPs;
std::vector< std::pair<unsigned long, unsigned long> > bannedIPs;

LuaScript g_config;

Items Item::items;
ReverseItemMap Items::revItems;
Game g_game;
Spells spells(&g_game);
Actions actions(&g_game);
Commands commands(&g_game);
Monsters g_monsters;

#if defined __EXCEPTION_TRACER__
#include "exception.h"
#endif
#include "networkmessage.h"

enum passwordType_t{
	PASSWORD_TYPE_PLAIN,
	PASSWORD_TYPE_MD5,
};

passwordType_t passwordType;

bool passwordTest(std::string &plain, std::string &hash)
{
	if(passwordType == PASSWORD_TYPE_MD5){
		MD5_CTX m_md5;
		std::stringstream hexStream;
		std::string plainHash;

		MD5Init(&m_md5, 0);
		MD5Update(&m_md5, (const unsigned char*)plain.c_str(), (unsigned int)plain.length());
		MD5Final(&m_md5);

		hexStream.flags(std::ios::hex);
		for(int i=0;i<16;i++){
			hexStream << std::setw(2) << std::setfill('0') << (unsigned long)m_md5.digest[i];
		}

		plainHash = hexStream.str();
		std::transform(plainHash.begin(), plainHash.end(), plainHash.begin(), upchar);
		std::transform(hash.begin(), hash.end(), hash.begin(), upchar);
		if(plainHash == hash){
			return true;
		}
		else{
			return false;
		}
	}
	else{
		if(plain == hash){
			return true;
		}
		else{
			return false;
		}

	}

}

bool isclientBanished(SOCKET s)
{
	sockaddr_in sain;
	socklen_t salen = sizeof(sockaddr_in);

	if (getpeername(s, (sockaddr*)&sain, &salen) == 0)
	{
		unsigned long clientip = *(unsigned long*)&sain.sin_addr;

		for (size_t i = 0; i < bannedIPs.size(); ++i) {
      if ((bannedIPs[i].first & bannedIPs[i].second) == (clientip & bannedIPs[i].second))
				return true;
		}
	}

	return false;
}


#ifdef YUR_LOGIN_QUEUE
std::string MakeRejectMessage(int place)
{
	if (place < 0)
		return std::string("Wrong temple position!");
	else
	{
		std::ostringstream msg;
		msg << "Too many Players online.\n";

		if (!g_config.QUEUE_PREMMY)
			msg << "Only premium players are allowed to enter.\n";

		msg << "\nYou are at place " << place << " on the waiting list.";
		return msg.str();
	}
}
#endif //YUR_LOGIN_QUEUE


OTSYS_THREAD_RETURN ConnectionHandler(void *dat)
{
#if defined __EXCEPTION_TRACER__
	ExceptionHandler playerExceptionHandler;
	playerExceptionHandler.InstallHandler();
#endif

	srand((unsigned)time(NULL));

	SOCKET s = *(SOCKET*)dat;

	NetworkMessage msg;
	if (msg.ReadFromSocket(s))
	{
		unsigned short protId = msg.GetU16();

		// login server connection
		if (protId == 0x0201)
		{
			msg.SkipBytes(15);
			unsigned int accnumber = msg.GetU32();
			std::string  password  = msg.GetString();

			int serverip = serverIPs[0].first;

			sockaddr_in sain;
			socklen_t salen = sizeof(sockaddr_in);
			if (getpeername(s, (sockaddr*)&sain, &salen) == 0)
			{
				unsigned long clientip = *(unsigned long*)&sain.sin_addr;
				for (unsigned int i = 0; i < serverIPs.size(); i++)
					if ((serverIPs[i].first & serverIPs[i].second) == (clientip & serverIPs[i].second))
					{
						serverip = serverIPs[i].first;
						break;
					}
			}

			msg.Reset();

			if(isclientBanished(s)) {
				msg.AddByte(0x0A);
				msg.AddString("Your IP is banished!");
			}
			else {
				//char accstring[16];
				//sprintf(accstring, "%i", accnumber);

				Account account = IOAccount::instance()->loadAccount(accnumber);
				if (account.accnumber == accnumber && passwordTest(password,account.password)) // seems to be a successful load
				{
					msg.AddByte(0x14);
					std::stringstream motd;
					motd << g_config.getGlobalString("motdnum");
					motd << "\n";
					motd << g_config.getGlobalString("motd");
					msg.AddString(motd.str());

					msg.AddByte(0x64);
					msg.AddByte((uint8_t)account.charList.size());

					std::list<std::string>::iterator it;
					for (it = account.charList.begin(); it != account.charList.end(); it++)
					{
						msg.AddString((*it));
						msg.AddString(g_config.getGlobalString("worldname","YurOTS"));
						msg.AddU32(serverip);
						msg.AddU16(atoi(g_config.getGlobalString("port").c_str()));
					}

					msg.AddU16(account.premDays);
				}
				else
				{
					msg.AddByte(0x0A);
					msg.AddString("Please enter a valid account number and password.");
				}
			}

			msg.WriteToSocket(s);
		}
		// gameworld connection tibia 7.6
		else if (protId == 0x020A)
		{
			unsigned char  clientos = msg.GetByte();
			unsigned short version  = msg.GetU16();
			unsigned char  unknown = msg.GetByte();
			unsigned long accnumber = msg.GetU32();
			std::string name     = msg.GetString();
			std::string password = msg.GetString();
		
			if(version != 760){
				msg.Reset();
				msg.AddByte(0x14);
				msg.AddString("Only clients with protocol 7.6 allowed!");
				msg.WriteToSocket(s);
			}
			else if(isclientBanished(s)){
				msg.Reset();
				msg.AddByte(0x14);
				msg.AddString("Your IP is banished!");
				msg.WriteToSocket(s);
			}
			else{
				OTSYS_SLEEP(1000);
				std::string acc_pass;
				if(IOAccount::instance()->getPassword(accnumber, name, acc_pass) && passwordTest(password,acc_pass))
				{
					bool isLocked = true;
					OTSYS_THREAD_LOCK(g_game.gameLock, "ConnectionHandler()")
					Player* player = g_game.getPlayerByName(name);
					bool playerexist = (player != NULL);
					if(player){
						//reattach player?
						if(player->client->s == 0 && player->isRemoved == false && !g_config.getGlobalNumber("allowclones", 0)){
							player->lastlogin = std::time(NULL);
							player->client->reinitializeProtocol();
							player->client->s = s;
							player->client->sendThingAppear(player);
							player->lastip = player->getIP();
							s = 0;
						}

						//guess not...
						player = NULL;
					}

					//OTSYS_THREAD_UNLOCK(g_game.gameLock, "ConnectionHandler()")

					if(s){
						Protocol76* protocol;
						protocol = new Protocol76(s);
						player = new Player(name, protocol);
						player->useThing();
						player->setID();
						IOPlayer::instance()->loadPlayer(player, name);
#ifdef YUR_LOGIN_QUEUE
						int placeInQueue = -1;
#endif //YUR_LOGIN_QUEUE
						static const int ACCESS_ENTER = g_config.getGlobalNumber("accessenter",3);

						if(playerexist && !g_config.getGlobalNumber("allowclones", 0)){
							#ifdef __DEBUG_PLAYERS__
							std::cout << "reject player... (already logged in)" << std::endl;
							#endif
							msg.Reset();
							msg.AddByte(0x14);
							msg.AddString("You are already logged in.");
							msg.WriteToSocket(s);
						}
#ifdef TLM_SKULLS_PARTY
						else if (player->banned)
						{
							msg.Reset();
							msg.AddByte(0x14);
							msg.AddString("Your character has been banished!");
							msg.WriteToSocket(s);
						}
#endif //TLM_SKULLS_PARTY
						else if(g_game.getGameState() == GAME_STATE_SHUTDOWN){
							//nothing to do
						}
						else if(g_game.getGameState() == GAME_STATE_CLOSED && player->access < ACCESS_ENTER){
							msg.Reset();
							msg.AddByte(0x14);
							msg.AddString("Server temporarly closed.");
							msg.WriteToSocket(s);
						}
#ifdef YUR_LOGIN_QUEUE
						else if (!protocol->ConnectPlayer(&placeInQueue)) {
							Status* stat = Status::instance();
							stat->playerswaiting = (int)g_game.loginQueue.size();
							#ifdef __DEBUG_PLAYERS__
							if (placeInQueue < 0)
								std::cout << "reject player... wrong temple position" << std::endl;
							else
								std::cout << "reject player... place in queue: " << placeInQueue << std::endl;
							#endif
							msg.Reset();
							msg.AddByte(0x16);
							msg.AddString(MakeRejectMessage(placeInQueue));
							msg.AddByte(30);
							msg.WriteToSocket(s);
						}
#else
						else if(!protocol->ConnectPlayer()) {
							#ifdef __DEBUG_PLAYERS__
							std::cout << "reject player..." << std::endl;
							#endif
							msg.Reset();
							msg.AddByte(0x16);
							msg.AddString("Too many Players online.");
							msg.AddByte(45);
							msg.WriteToSocket(s);
						}
#endif //YUR_LOGIN_QUEUE
						else{
							Status* stat = Status::instance();
							stat->addPlayer(
#ifdef HHB_STATUS_MAX_4_PER_IP
								player->getIP(),
								player->getName()
#endif
							);
							player->lastlogin = std::time(NULL);
							player->lastip = player->getIP();
							s = 0;            // protocol/player will close socket

							OTSYS_THREAD_UNLOCK(g_game.gameLock, "ConnectionHandler()")
							isLocked = false;

							protocol->ReceiveLoop();
							stat->removePlayer(
#ifdef HHB_STATUS_MAX_4_PER_IP
								player->getIP(),
								player->getName()
#endif
							);
						}

						if(player){
							g_game.FreeThing(player);
						}
					}

					if(isLocked){
						OTSYS_THREAD_UNLOCK(g_game.gameLock, "ConnectionHandler()")
					}
				}
			}
		}
		// Since Cip made 02xx as Tibia protocol,
		// Lets make FFxx as "our great info protocol" ;P
		else if (protId == 0xFFFF) {
#ifdef HHB_STATUS_MAX_4_PER_IP
		const std::string raw = msg.GetRaw();
		if(raw == "info" || raw == "info_debug"){
#else
			if (msg.GetRaw() == "info"){
#endif //HHB_STATUS_MAX_4_PER_IP
				Status* status = Status::instance();

				uint64_t running = (OTSYS_TIME() - status->start)/1000;
				#ifdef __DEBUG__
				std::cout << ":: Uptime: " << running << std::endl;
				#endif
				std::string str = status->getStatusString(
#ifdef HHB_STATUS_MAX_4_PER_IP
		bool(raw == "info_debug")
#endif
				);
				send(s, str.c_str(), (int)str.size(), 0);
			}
		}
		// Another ServerInfo protocol
		// Starting from 01, so the above could be 00 ;)
		else if (protId == 0xFF01) {
			// This one doesn't need to read nothing, so we could save time and bandwidth
			// Can be called thgough a program that understand the NetMsg protocol
			Status* status = Status::instance();
			status->getInfo(msg);
			msg.WriteToSocket(s);
		}
#ifdef YUR_BUILTIN_AAC
		else if (protId == 0xFFAC) 
		{
			std::string response = AccountCreator::process(msg.GetRaw());
			msg.Reset();
			msg.AddString(response);
			msg.WriteToSocket(s);
		}
#endif //YUR_BUILTIN_AAC
  }
  if(s)
	  closesocket(s);
#if defined __EXCEPTION_TRACER__
  playerExceptionHandler.RemoveHandler();
#endif


#if defined WIN32 || defined WINDOWS
#else
  return 0;
#endif
}



void ErrorMessage(const char* message) {
  std::cout << std::endl << std::endl << "Error: " << message;

  std::string s;
  std::cin >> s;
}

int main(int argc, char *argv[])
{
#ifdef __OTSERV_ALLOCATOR_STATS__
	OTSYS_CREATE_THREAD(allocatorStatsThread, NULL);
#endif
#if defined __EXCEPTION_TRACER__
	ExceptionHandler mainExceptionHandler;
	mainExceptionHandler.InstallHandler();
#endif
#ifdef __WIN_LOW_FRAG_HEAP__
	ULONG  HeapFragValue = 2;

    if(HeapSetInformation(GetProcessHeap(),HeapCompatibilityInformation,&HeapFragValue,sizeof(HeapFragValue))){
		std::cout << "Heap Success" << std::endl;
	}
    else{
		std::cout << "Heap Error" << std::endl;
	}
#endif

	std::cout << ":: YurOTS 0.9.4f" << std::endl;
	std::cout << ":: ~~~~~~~~~~~~~" << std::endl;
	//std::cout << ":: OTServ Development-Version 0.5.0 - CVS Preview" << std::endl;
	//std::cout << ":: OTServ Version 0.4.1" << std::endl;
	//std::cout << ":: ====================" << std::endl;
	std::cout << "::" << std::endl;

#ifdef _NO_ROOT_PERMISSION_
	if( getuid() == 0 || geteuid() == 0 )
	{
		std::cout << std::endl << "OTServ executed as root user, please login with a normal user." << std::endl;
		return 1;
	}
#endif

	// ignore sigpipe...
#if defined __WINDOWS__ || defined WIN32
		//nothing yet
#else
	struct sigaction sigh;
	sigh.sa_handler = SIG_IGN;
	sigaction(SIGPIPE, &sigh, NULL);
#endif

//	LOG_MESSAGE("main", EVENT, 1, "Starting server");


	// random numbers generator
	std::cout << ":: Initializing random numbers...   ";
	srand ( (unsigned int)time(NULL) );
	std::cout << "[done]" << std::endl;

	// read global config
	std::cout << ":: Loading lua script config.lua... ";
#ifdef _HOMEDIR_CONF_
	std::string configpath;
	configpath = getenv("HOME");
	configpath += "/.otserv/config.lua";
	if (!g_config.OpenFile(configpath.c_str()))
#else
	if (!g_config.OpenFile("config.lua"))
#endif
	{
		ErrorMessage("Unable to load config.lua!");
		return -1;
	}
	std::cout << "[done]" << std::endl;

	//load spells data
	std::cout << ":: Loading spells.xml...            ";
	if(!spells.loadFromXml(g_config.getGlobalString("datadir")))
	{
		ErrorMessage("Unable to load spells.xml!");
		return -1;
	}
	std::cout << "[done]" << std::endl;

	//load actions data
	std::cout << ":: Loading actions.xml...           ";
	if(!actions.loadFromXml(g_config.getGlobalString("datadir")))
	{
		ErrorMessage("Unable to load actions.xml!");
		return -1;
	}
	std::cout << "[done]" << std::endl;

	//load commands data
	std::cout << ":: Loading commands.xml...          ";
	if(!commands.loadXml(g_config.getGlobalString("datadir")))
	{
		ErrorMessage("Unable to load commands.xml!");
		return -1;
	}
	std::cout << "[done]" << std::endl;

	// load item data
	std::cout << ":: Loading items.otb...             ";
	if (Item::items.loadFromOtb(g_config.getGlobalString("datadir") + "items/items.otb"))
	{
		ErrorMessage("Could not load items.otb!");
		return -1;
	}
	std::cout << "[done]" << std::endl;

	std::cout << ":: Loading items.xml...             ";
	if (!Item::items.loadXMLInfos(g_config.getGlobalString("datadir") + "items/items.xml"))
	{
		ErrorMessage("Could not load /items/items.xml ...!");
		return -1;
	}
	std::cout << "[done]" << std::endl;


#ifdef YUR_GUILD_SYSTEM
	std::cout << ":: Loading guilds.xml...            ";
	if (!Guilds::Load())
	{
		ErrorMessage("Could not load guilds.xml!");
		return -1;
	}
	std::cout << "[done]" << std::endl;
#endif //YUR_GUILD_SYSTEM

#ifdef YUR_LOGIN_QUEUE
	std::cout << ":: Loading queue.xml...             ";
	if (!g_game.loginQueue.load())
	{
		ErrorMessage("Could not load queue.xml!");
		return -1;
	}
	std::cout << "[done]" << std::endl;
#endif //YUR_LOGIN_QUEUE

#ifdef TR_SUMMONS
	std::cout << ":: Loading summons.xml...           ";
	if (!Summons::Load())
	{
		ErrorMessage("Could not load summons.xml!");
		return -1;
	}
	std::cout << "[done]" << std::endl;
#endif //TR_SUMMONS

	// load monster data
	std::cout << ":: Loading monsters.xml...          ";
	if(!g_monsters.loadFromXml(g_config.getGlobalString("datadir")))
	{
		ErrorMessage("Could not load monsters/monsters.xml!");
		return -1;
	}
	std::cout << "[done]" << std::endl;

	std::string worldtype = g_config.getGlobalString("worldtype");
	std::transform(worldtype.begin(), worldtype.end(), worldtype.begin(), upchar);
	if(worldtype == "PVP")
		g_game.setWorldType(WORLD_TYPE_PVP);
	else if(worldtype == "NO-PVP")
		g_game.setWorldType(WORLD_TYPE_NO_PVP);
	else if(worldtype == "PVP-ENFORCED")
		g_game.setWorldType(WORLD_TYPE_PVP_ENFORCED);
	else{
		ErrorMessage("Unknown world type!");
		return -1;
	}
	std::cout << ":: World Type: " << worldtype << std::endl;

	if(g_config.getGlobalString("md5passwords") == "yes"){
		passwordType = PASSWORD_TYPE_MD5;
		std::cout << ":: Use MD5 passwords" << std::endl;
	}
	else{
		passwordType = PASSWORD_TYPE_PLAIN;
	}

#ifdef YUR_CVS_MODS
	timer();
#endif

	// loads the map and, if needed, an extra-file spawns
	switch(g_game.loadMap(g_config.getGlobalString("map"), g_config.getGlobalString("mapkind"))){
	case SPAWN_BUILTIN:
		break;
	case MAP_LOADER_ERROR:
		std::cout << "FATAL: couldnt determine the map format! exiting2" << std::endl;
		exit(1);
		break;
	case SPAWN_XML:
		SpawnManager::initialize(&g_game);
		SpawnManager::instance()->loadSpawnsXML(g_game.getSpawnFile());
		SpawnManager::instance()->startup();
		break;
	#ifdef ENABLESQLMAPSUPPORT
	case SPAWN_SQL:
		SpawnManager::initialize(&g_game);
		SpawnManager::instance()->loadSpawnsSQL(g_config.getGlobalString("map"));
		SpawnManager::instance()->startup();
		break;
	#endif
	}

#ifdef YUR_CVS_MODS
	double t = timer();
	std::cout << "Loaded in " << t << " s" << std::endl;
#endif


#ifdef WOLV_LOAD_NPC
	std::cout << ":: Loading npc.xml...               ";
	if (!g_game.loadNpcs())
	{
		ErrorMessage("Could not load npc.xml!");
		return -1;
	}
	std::cout << "[done]" << std::endl;
#endif

#ifdef TLM_HOUSE_SYSTEM
	std::cout << ":: Loading houses.xml...            ";
	if (!Houses::Load(&g_game))
	{
		ErrorMessage("Could not load houses!");
		return -1;
	}
	std::cout << "[done]" << std::endl;
#endif //TLM_HOUSE_SYSTEM

#ifdef YUR_PVP_ARENA
	std::cout << ":: Loading pvparenas.xml...         ";
	if (!PvpArena::Load(&g_game))
	{
		ErrorMessage("Could not load pvparenas.xml!");
		return -1;
	}
	std::cout << "[done]" << std::endl;
#endif //YUR_PVP_ARENA

#ifdef YUR_READABLES
	std::cout << ":: Loading readables.xml...         ";
	if (!Readables::Load(&g_game))
	{
		ErrorMessage("Could not load readables.xml!");
		return -1;
	}
	std::cout << "[done]" << std::endl;
#endif //YUR_READABLES

#ifdef TLM_SERVER_SAVE
	if (g_config.getGlobalNumber("autosave") > 0)
		g_game.addEvent(makeTask(g_config.getGlobalNumber("autosave")*60000, std::mem_fun(&Game::autoServerSave)));
	else
		std::cout << ":: Auto server save disabled!" << std::endl;
#endif //TLM_SERVER_SAVE


  	// Call to WSA Startup on Windows Systems...
#ifdef WIN32
	WORD wVersionRequested;
	WSADATA wsaData;
	wVersionRequested = MAKEWORD( 1, 1 );

	if (WSAStartup(wVersionRequested, &wsaData) != 0)
	{
		ErrorMessage("Winsock startup failed!!");
		return -1;
	}

	if ((LOBYTE(wsaData.wVersion) != 1) || (HIBYTE(wsaData.wVersion) != 1))
	{
		WSACleanup( );
		ErrorMessage("No Winsock 1.1 found!");
		return -1;
	}
#endif

#ifdef YUR_CVS_MODS
	std::string localip = "127.0.0.1";
#endif

	std::pair<unsigned long, unsigned long> IpNetMask;
	IpNetMask.first  = inet_addr("127.0.0.1");
	IpNetMask.second = 0xFFFFFFFF;
	serverIPs.push_back(IpNetMask);

	char szHostName[128];
	if (gethostname(szHostName, 128) == 0)
	{
		std::cout << "::" << std::endl << ":: Running on host " << szHostName << std::endl;

		hostent *he = gethostbyname(szHostName);

		if (he)
		{
			std::cout << ":: Local IP address(es):  ";
			unsigned char** addr = (unsigned char**)he->h_addr_list;

#ifdef YUR_CVS_MODS
			if (addr[0])
			{
				localip = str(int(addr[0][0])) + "." + str(int(addr[0][1])) + 
					"." + str(int(addr[0][2])) + "." + str(int(addr[0][3]));
			}
#endif //YUR_CVS_MODS

			while (addr[0] != NULL)
			{
				std::cout << (unsigned int)(addr[0][0]) << "."
				<< (unsigned int)(addr[0][1]) << "."
				<< (unsigned int)(addr[0][2]) << "."
				<< (unsigned int)(addr[0][3]) << "  ";

			IpNetMask.first  = *(unsigned long*)(*addr);
			IpNetMask.second = 0x0000FFFF;
			serverIPs.push_back(IpNetMask);

			addr++;
			}

		std::cout << std::endl;
    		}
  	}

	std::cout << ":: Global IP address:     ";
	std::string ip;

	if(argc > 1)
		ip = argv[1];
	else
		ip = g_config.getGlobalString("ip", "127.0.0.1");

#ifdef YUR_CVS_MODS
	if (ip == "auto")
		ip = localip;
#endif //YUR_CVS_MODS

	std::cout << ip << std::endl << "::" << std::endl;

#ifdef YUR_CVS_MODS
	if (ip == "127.0.0.1")
		std::cout << "WARNING: Change your IP in config.lua!\n" << std::endl;
#endif //YUR_CVS_MODS

	IpNetMask.first  = inet_addr(ip.c_str());
	IpNetMask.second = 0;
	serverIPs.push_back(IpNetMask);
	std::cout << ":: Starting Server... ";

	Status* status = Status::instance();
	status->playersmax = g_config.getGlobalNumber("maxplayers");

	// start the server listen...
	int listen_errors;
	int accept_errors;
	listen_errors = 0;
	while(g_game.getGameState() != GAME_STATE_SHUTDOWN && listen_errors < 100){
		sockaddr_in local_adress;
		memset(&local_adress, 0, sizeof(sockaddr_in)); // zero the struct

		local_adress.sin_family      = AF_INET;
		local_adress.sin_port        = htons(atoi(g_config.getGlobalString("port").c_str()));
		local_adress.sin_addr.s_addr = htonl(INADDR_ANY);

		// first we create a new socket
		SOCKET listen_socket = socket(AF_INET, SOCK_STREAM, 0);

		if(listen_socket <= 0){
#ifdef WIN32
	    		WSACleanup();
#endif
			ErrorMessage("Unable to create server socket (1)!");
			return -1;
  		} // if (listen_socket <= 0)

#ifndef WIN32
		int yes = 1;
		// lose the pesky "Address already in use" error message
		if(setsockopt(listen_socket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof (int)) == -1){
			ErrorMessage("Unable to set socket options!");
			return -1;
		}
#endif

		// bind socket on port
		if(bind(listen_socket, (struct sockaddr*)&local_adress, sizeof(struct sockaddr_in)) < 0){
#ifdef WIN32
	    		WSACleanup();
#endif
			ErrorMessage("Unable to create server socket (2)!");
			return -1;
  		} // if (bind(...))

		// now we start listen on the new socket
		if(listen(listen_socket, 10) == SOCKET_ERROR){
#ifdef WIN32
    			WSACleanup();
#endif
    			ErrorMessage("Listen on server socket not possible!");
			return -1;
		} // if (listen(*listen_socket, 10) == -1)


		std::cout << "[done]" << std::endl << ":: " << g_config.getGlobalString("servername") << " Server Running..." << std::endl;
		accept_errors = 0;
		while(g_game.getGameState() != GAME_STATE_SHUTDOWN && accept_errors < 100){
			fd_set listen_set;
			timeval tv;
			FD_ZERO(&listen_set);
			FD_SET(listen_socket, &listen_set);
			tv.tv_sec = 1;
			tv.tv_usec = 0;

			int reads = select(1, &listen_set, NULL, NULL, &tv);
			int errnum;
#ifdef WIN32
			errnum = WSAGetLastError();
#else
			errnum = errno;
#endif
			if(reads == SOCKET_ERROR){
				break;
			}
			else if(reads == 0 && errnum == ERROR_EINTR){
				accept_errors++;
				continue;
			}

			SOCKET s = accept(listen_socket, NULL, NULL); // accept a new connection
			if(s > 0){
				OTSYS_CREATE_THREAD(ConnectionHandler, (void*)&s);
			}
			else{
					accept_errors++;
			}
		}
		closesocket(listen_socket);
		listen_errors++;
  	}
#ifdef WIN32
  	WSACleanup();
#endif

#if defined __EXCEPTION_TRACER__
  	mainExceptionHandler.RemoveHandler();
#endif
	return 0;
}
