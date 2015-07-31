//#include "preheaders.h"

#include <string>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <ctime>

#include "networkmessage.h"
#include "protocol76.h"
#include "game.h"
#include "ioaccountsql.h"
#include "ioplayersql.h"
#ifdef TR_SUMMONS
#include "summons.h"
#endif //TR_SUMMONS
#ifdef __MIZIAK_TASKS__
#include "task.h"
#endif //__MIZIAK_TASKS__
#include "status.h"
#include "spells.h"
#include "monsters.h"
#include "actions.h"
#include "commands.h"
#include "town.h"
#include "luascript.h"
#include "account.h"
#include "tools.h"
#include "pvparena.h"
#ifdef __OTSERV_ALLOCATOR__
#include "allocator.h"
#endif
#include "databasemanager.h"
#ifdef WIN32
#define ERROR_EINTR WSAEINTR
#else
/* Comment below line if you want to execute otserv with root user (NOT RECOMMENDED) */
#define _NO_ROOT_PERMISSION_

extern int32_t errno;
#endif

#ifdef __DEBUG_CRITICALSECTION__
OTSYS_THREAD_LOCK_CLASS::LogList OTSYS_THREAD_LOCK_CLASS::loglist;
#endif

std::vector< std::pair<uint32_t, uint32_t> > serverIPs;
std::vector< std::pair<uint32_t, uint32_t> > bannedIPs;
std::vector< std::pair<uint32_t[3], uint32_t> > IPs;

LuaScript g_config;

Items Item::items;
ReverseItemMap Items::revItems;
Game g_game;
Spells spells(&g_game);
Actions actions(&g_game);
Commands commands(&g_game);
Monsters g_monsters;

struct AccSuspensaUCB
{
    uint32_t conta;
    uint32_t tentativas;
    std::time_t tempo;
    bool bloqueado;
};

struct IpSuspensoUCB
{
    uint32_t ip;
    uint32_t tentativas;
    std::time_t tempo;
    bool bloqueado;
};

std::vector< struct AccSuspensaUCB > ListaAccSuspensas;
std::vector< struct IpSuspensoUCB  > ListaIpsSuspensos;
std::time_t tempo_reciclagem = std::time(NULL) + 3600;

#include "networkmessage.h"

enum passwordType_t
{
    PASSWORD_TYPE_PLAIN,
    PASSWORD_TYPE_MD5,
};

passwordType_t passwordType;

bool passwordTest(std::string &plain, std::string &hash)
{
    if(passwordType != PASSWORD_TYPE_MD5)
        if(plain == hash)
            return true;
        else
            return false;

    return false; //VISUAL
}

/*int32_t polaczenia(SOCKET s)
{
    time_t czas;
    time(&czas);
    struct tm * timeinfo = localtime(&czas);
    int32_t minuta = timeinfo->tm_sec+timeinfo->tm_min*60;
	sockaddr_in sain;
	socklen_t salen = sizeof(sockaddr_in);

	if (getpeername(s, (sockaddr*)&sain, &salen) == 0)
	{
		uint32_t clientip = *(uint32_t*)&sain.sin_addr;
		for (size_t i = 0; i < IPs.size(); ++i) {
            if ((IPs[i].first[0]) == (clientip)){
               int32_t cos = IPs[i].first[2] - minuta;
                if(abs(cos) >= 30){
                    IPs[i].first[1] = 0;
                    IPs[i].first[2] = minuta;
                }
                else if(IPs[i].first[1]>=5)
                    return -2;//banned
                else{
                    IPs[i].first[1]++;
                    return IPs[i].first[1];
                }
            }
		}
        std::pair<uint32_t[3], uint32_t> IpNetMask;
        IpNetMask.first[0] = clientip;
        IpNetMask.first[1] = 0;
        IpNetMask.first[2] = minuta;
        IpNetMask.second = 0xFFFFFFFF;
        IPs.push_back(IpNetMask);
        return 0;
    }
	return -1;
}*/

bool isclientBanished(SOCKET s)
{
    sockaddr_in sain;
    socklen_t salen = sizeof(sockaddr_in);

    if (getpeername(s, (sockaddr*)&sain, &salen) == 0)
    {
        uint32_t clientip = *(uint32_t*)&sain.sin_addr;

        for (size_t i = 0; i < bannedIPs.size(); ++i)
        {
            if ((bannedIPs[i].first & bannedIPs[i].second) == (clientip & bannedIPs[i].second))
                return true;
        }
    }

    return false;
}

// Ulisses (Proglin) - IP/ACC Suspend System
// April / 2006 for turion.no-ip.info
bool isAccIPSuspend(int32_t acc, SOCKET s)
{
    bool suspensa = false;
    bool achouip = false;
    bool achouacc = false;

    sockaddr_in sain;
    socklen_t salen = sizeof(sockaddr_in);

    std::time_t now = std::time(NULL);

    /* Recycling Lists (to save memory ;) ) */
    if( now > tempo_reciclagem )
    {
        OTSYS_THREAD_LOCK(g_game.gameLock, "UCB_IP_Seach");
        OTSYS_THREAD_LOCK(g_game.gameLock, "UCB_ACC_Seach");
        tempo_reciclagem = now + 3600;
        bool achou = true;

        std::cout << "Ips  suspend before recycling: " << ListaIpsSuspensos.size() << std::endl;
        std::cout << "Accs suspend before recycling: " << ListaAccSuspensas.size() << std::endl;

        /* Ips list */
        std::vector< struct IpSuspensoUCB >::iterator i = ListaIpsSuspensos.begin();
        while( achou )
        {
            achou = false;
            while(i != ListaIpsSuspensos.end())
            {
                if((*i).tempo < now)
                {
                    ListaIpsSuspensos.erase(i);
                    achou = true;
                    break;
                }
                ++i;
            }
        }

        /* Acc list */
        achou = true;
        std::vector< struct AccSuspensaUCB >::iterator ii = ListaAccSuspensas.begin();
        while( achou )
        {
            achou = false;
            while(ii != ListaAccSuspensas.end())
            {
                if((*ii).tempo < now)
                {
                    ListaAccSuspensas.erase(ii);
                    achou = true;
                    break;
                }
                ++ii;
            }
        }
        OTSYS_THREAD_UNLOCK(g_game.gameLock, "UCB_ACC_Seach");
        OTSYS_THREAD_UNLOCK(g_game.gameLock, "UCB_IP_Seach");
        std::cout << "Ips  suspend  after recycling: " << ListaIpsSuspensos.size() << std::endl;
        std::cout << "Accs suspend  after recycling: " << ListaAccSuspensas.size() << std::endl;
    }

    /* Search for IP */
    OTSYS_THREAD_LOCK(g_game.gameLock, "UCB_IP_Seach");
    if (getpeername(s, (sockaddr*)&sain, &salen) == 0 )
    {
        uint32_t clientip = *(uint32_t*)&sain.sin_addr;
        for (size_t i = 0; i < ListaIpsSuspensos.size(); ++i)
        {
            if( ListaIpsSuspensos[i].ip == clientip )
            {
                achouip = true;
                if( ListaIpsSuspensos[i].tempo < now )
                {
                    ListaIpsSuspensos[i].tentativas = 0;
                    ListaIpsSuspensos[i].bloqueado = false;
                    ListaIpsSuspensos[i].tempo = now + 60*g_config.SUSPEND_TIME_MAX;
                }
                if( !ListaIpsSuspensos[i].bloqueado )
                {
                    if( ListaIpsSuspensos[i].tentativas == g_config.SUSPEND_IP_TRIES - 1 )  //VISUAL
                    {
                        ListaIpsSuspensos[i].bloqueado = true;
                        ListaIpsSuspensos[i].tempo = now + 60*g_config.SUSPEND_TIME_MAX;
                    }
                }
                ListaIpsSuspensos[i].tentativas++;
                suspensa = ListaIpsSuspensos[i].bloqueado;
            }
        }
        if( !achouip )
        {
            struct IpSuspensoUCB candidatoip;
            candidatoip.ip = clientip;
            candidatoip.tentativas = 1;
            candidatoip.tempo = now + 60*g_config.SUSPEND_TIME_MAX;
            candidatoip.bloqueado = false;
            ListaIpsSuspensos.push_back(candidatoip);
        }
    }
    OTSYS_THREAD_UNLOCK(g_game.gameLock, "UCB_IP_Seach");

    /* Search for Account */
    OTSYS_THREAD_LOCK(g_game.gameLock, "UCB_ACC_Seach");
    for (size_t i = 0; i < ListaAccSuspensas.size(); ++i)
    {
        if( ListaAccSuspensas[i].conta == acc )
        {
            achouacc = true;
            if( ListaAccSuspensas[i].tempo < now )
            {
                ListaAccSuspensas[i].tentativas = 0;
                ListaAccSuspensas[i].bloqueado = false;
                ListaAccSuspensas[i].tempo = now + 60*g_config.SUSPEND_TIME_MAX;
            }
            if( !ListaAccSuspensas[i].bloqueado )
            {
                if( ListaAccSuspensas[i].tentativas == g_config.SUSPEND_IP_TRIES - 1 )  //VISUAL
                {
                    ListaAccSuspensas[i].bloqueado = true;
                    ListaAccSuspensas[i].tempo = now + 60*g_config.SUSPEND_TIME_MAX;
                }
            }
            ListaAccSuspensas[i].tentativas++;
            if( !suspensa )
                suspensa = ListaAccSuspensas[i].bloqueado;
        }
    }
    if( !achouacc )
    {
        struct AccSuspensaUCB candidatoacc;
        candidatoacc.conta = acc;
        candidatoacc.tentativas = 1;
        candidatoacc.tempo = now + 60*g_config.SUSPEND_TIME_MAX;
        candidatoacc.bloqueado = false;
        ListaAccSuspensas.push_back(candidatoacc);
    }
    OTSYS_THREAD_UNLOCK(g_game.gameLock, "UCB_ACC_Seach");

    return suspensa;
}

#ifdef YUR_LOGIN_QUEUE
std::string MakeRejectMessage(int32_t place)
{
    if (place < 0)
        return std::string("Zla pozycja temple! Skontaktuj sie z administratorem!");
    else
    {
        std::ostringstream msg;
        msg << "Za duzo graczy online.\n";

        if (!g_config.QUEUE_PREMMY)
            msg << "Tylko gracze z Premium moga teraz wejsc.\n";

        msg << "\nJestes na " << place << " pozycji na liscie oczekujacych.";
        return msg.str();
    }
}
#endif //YUR_LOGIN_QUEUE


OTSYS_THREAD_RETURN ConnectionHandler(void *dat)
{

    srand((unsigned)time(NULL));

    SOCKET s = *(SOCKET*)dat;

    NetworkMessage msg;
    if (msg.ReadFromSocket(s))
    {
//	    if(polaczenia(s) == -2)
//            std::cout << "Proba ataku!";

        //if(polaczenia(s) != -2){

        uint16_t protId = msg.GetU16();

        // login server connection
        if (protId == 0x0201)
        {
            msg.SkipBytes(15);
            uint32_t accnumber = msg.GetU32();
            std::string  password  = msg.GetString();

            int32_t serverip = serverIPs[0].first;

            sockaddr_in sain;
            socklen_t salen = sizeof(sockaddr_in);
            if (getpeername(s, (sockaddr*)&sain, &salen) == 0)
            {
                uint32_t clientip = *(uint32_t*)&sain.sin_addr;
                for (uint32_t i = 0; i < serverIPs.size(); i++)
                    if ((serverIPs[i].first & serverIPs[i].second) == (clientip & serverIPs[i].second))
                    {
                        serverip = serverIPs[i].first;
                        break;
                    }
            }

            msg.Reset();

            bool recebeuban = false;
            if(isclientBanished(s))
            {
                recebeuban = true;
                msg.AddByte(0x0A);
                msg.AddString("Twoje IP zostalo zbanowane!");
            }
            if( isAccIPSuspend(accnumber, s) || recebeuban )
            {
                if( !recebeuban )
                {
                    msg.AddByte(0x0A);
                    msg.AddString(g_config.SUSPEND_MSG);
                }
            }
            else
            {
                //char accstring[16];
                //sprintf(accstring, "%i", accnumber);

                Account account = IOAccountSQL::getInstance()->loadAccount(accnumber);
                if (account.accnumber == accnumber && passwordTest(password,account.password) && account.accnumber != 0 && accnumber != 0) // seems to be a successful load
                {
                    msg.AddByte(0x14);
                    msg.AddString(g_config.MOTD);

                    msg.AddByte(0x64);
                    msg.AddByte((uint8_t)account.charList.size());

                    std::list<std::string>::iterator it;
                    for (it = account.charList.begin(); it != account.charList.end(); ++it)
                    {
                        msg.AddString((*it));
                        msg.AddString(g_config.WORLD_NAME);
                        msg.AddU32(serverip);
                        msg.AddU16(g_config.PORT);
                    }

                    msg.AddU16(account.premDays);
                }
                else
                {
                    msg.AddByte(0x0A);
                    msg.AddString("Zly numer konta lub haslo!");
                }
            }

            msg.WriteToSocket(s);
        }
        // gameworld connection tibia 7.6
        else if (protId == 0x020A)
        {
            unsigned char  clientos = msg.GetByte();
            uint16_t version  = msg.GetU16();
            unsigned char  unknown = msg.GetByte();
            uint32_t accnumber = msg.GetU32();
            std::string name     = msg.GetString();
            std::string password = msg.GetString();

            if(version != 760)
            {
                msg.Reset();
                msg.AddByte(0x14);
                msg.AddString("Protokol 7.6 obowiazuje!");
                msg.WriteToSocket(s);
            }
            else if(isclientBanished(s))
            {
                msg.Reset();
                msg.AddByte(0x14);
                msg.AddString("Twoje IP zostalo zbanowane!");
                msg.WriteToSocket(s);
            }
            else
            {
                OTSYS_SLEEP(1000);
                std::string acc_pass;
                if(IOAccountSQL::getInstance()->getPassword(accnumber, name, acc_pass) && passwordTest(password,acc_pass))
                {
                    bool isLocked = true;
                    OTSYS_THREAD_LOCK(g_game.gameLock, "ConnectionHandler()")
                    Player* player = g_game.getPlayerByName(name);
                    bool playerexist = (player != NULL);
                    if(player)
                    {
                        //reattach player?
                        if(player->client->s == 0 && player->isRemoved == false)
                        {
                            player->lastlogin = std::time(NULL);
                            player->client->reinitializeProtocol(s);
                            player->client->s = s;
                            player->client->sendThingAppear(player);
                            player->lastip = player->getIP();
                            s = 0;
                        }

                        //guess not...
                        player = NULL;
                    }
                    //OTSYS_THREAD_UNLOCK(g_game.gameLock, "ConnectionHandler()")

                    if(s)  //tak mi sie wydaje
                    {
                        int64_t timeNow = std::time(NULL); // VISUAL
                        Protocol76* protocol;
                        protocol = new Protocol76(s);
                        player = new Player(name, protocol);
                        player->useThing();
                        player->setID();
                        IOPlayerSQL::getInstance()->loadPlayer(player, name);
#ifdef YUR_LOGIN_QUEUE
                        int32_t placeInQueue = -1;
#endif //YUR_LOGIN_QUEUE
                        static const int32_t ACCESS_ENTER = g_config.ACCESS_ENTER;
                        if(playerexist)
                        {
                            std::cout << "Odrzucenie gracza: " << player->getName() << " aktulanie zalogowany." << std::endl;
                            msg.Reset();
                            msg.AddByte(0x14);
                            msg.AddString("Aktualnie jestes zalogowany.");
                            msg.WriteToSocket(s);
                        }
#ifdef HUCZU_BAN_SYSTEM
                        else if (player->banned && timeNow < player->banend)
                        {
                            msg.Reset();
                            msg.AddByte(0x14);
                            time_t endBan = player->banend;
                            std::stringstream txt;
                            if(player->deleted == 0) // checks if not deleted
                                txt << "Twoja postac zostala zablokowana! Powod: " << player->reason << ".\nTwoj ban zostanie zdjety " << ctime(&endBan) << "Sprobuj zalogowac sie po tej dacie.";
                            if(player->namelock != 0) // gdy namelock
                                txt << "Dostales rowniez namelocka. Zmien swoj nick na stronie.";
                            if(player->deleted != 0)// deleted == 1
                                txt << "Twoja postac zostala usunieta z serwera!\nNigdy nie zostanie juz odbanowana!";
                            msg.AddString(txt.str().c_str());
                            msg.WriteToSocket(s);
                        }
                        else if(player->banned && timeNow > player->banend && player->deleted == 0)
                        {
                            player->banned = 0;
                            player->comment = "";
                            player->reason = "";
                            player->action = "";
                            player->banrealtime = "";
                            IOPlayerSQL::getInstance()->savePlayer(player);
                            msg.Reset();
                            msg.AddByte(0x14);
                            msg.AddString("Zostales odbanowany. Mozesz sie teraz zalogowac.");
                            msg.WriteToSocket(s);
                        }
                        else if(player->namelock != 0)
                        {
                            msg.Reset();
                            msg.AddByte(0x14);
                            msg.AddString("Posiadasz namelocka. Zmien swoj nick na stronie.");
                            msg.WriteToSocket(s);
                        }
#endif //HUCZU_BAN_SYSTEM
                        else if(player->vocation > 4 || player->vocation < 0 || player->healthmax <=0 || player->manamax < 0 || player->level <= 0 || player->experience < 0 || player->maglevel < 0)
                        {
                            msg.Reset();
                            msg.AddByte(0x14);
                            msg.AddString("Your character is bugged!");
                            msg.WriteToSocket(s);
                        }
                        /*else if(g_game.getGameState() == GAME_STATE_SHUTDOWN){
                        	//nothing to do
                        }*/
                        else if(g_game.getGameState() == GAME_STATE_CLOSED && player->access < ACCESS_ENTER)
                        {
                            msg.Reset();
                            msg.AddByte(0x14);
                            msg.AddString("Serwer jest tymczasowo zamkniety. Prosze sprobowac za chwile.");
                            msg.WriteToSocket(s);
                        }

#ifdef YUR_LOGIN_QUEUE
                        else if (!protocol->ConnectPlayer(&placeInQueue))
                        {
                            Status* stat = Status::instance();
                            stat->playerswaiting = (int32_t)g_game.loginQueue.size();
                            if (placeInQueue < 0)
                                std::cout << "Odrzucenie gracza: " << player->getName() << " ,bledna pozycja temple."<< std::endl;
                            else
                                std::cout << "Odrzucenie gracza: " << player->getName() << ". Ustawiony w kolejce: " << placeInQueue << std::endl;
                            msg.Reset();
                            msg.AddByte(0x16);
                            msg.AddString(MakeRejectMessage(placeInQueue));
                            msg.AddByte(30);
                            msg.WriteToSocket(s);
                        }
#else
                        else if(!protocol->ConnectPlayer())
                        {
                            std::cout << "Odrzucenie gracza: " << player->getName() << " brak miejsca." << std::endl;
                            msg.Reset();
                            msg.AddByte(0x16);
                            msg.AddString("Za duzo graczy online.");
                            msg.AddByte(45);
                            msg.WriteToSocket(s);
                        }
#endif //YUR_LOGIN_QUEUE
                        else
                        {
                            Status* stat = Status::instance();
                            stat->addPlayer();
                            player->lastlogin = std::time(NULL);
                            player->lastip = player->getIP();
                            s = 0;            // protocol/player will close socket

                            OTSYS_THREAD_UNLOCK(g_game.gameLock, "ConnectionHandler()")
                            isLocked = false;

                            protocol->ReceiveLoop();
                            /*#ifdef HUCZU_NOLOGOUT_TILE
                                                        //stat->removePlayer();
                            #else*/
                            stat->removePlayer();
//#endif
                        }
                        g_game.FreeThing(player);
                    }

                    if(isLocked)
                    {
                        OTSYS_THREAD_UNLOCK(g_game.gameLock, "ConnectionHandler()")
                    }
                }
            }
        }
        // Since Cip made 02xx as Tibia protocol,
        // Lets make FFxx as "our great info protocol" ;P
        else if (protId == 0xFFFF)
        {
            if (msg.GetRaw() == "info")
            {
                Status* status = Status::instance();
                /*uint64_t running = (OTSYS_TIME() - status->start)/1000;
                #ifdef __DEBUG__
                std::cout << ":: Uptime: " << running << std::endl;
                #endif*///fix na blokade
                std::string str = status->getStatusString();
                send(s, str.c_str(), (int32_t)str.size(), 0);
            }
        }
        // Another ServerInfo protocol
        // Starting from 01, so the above could be 00 ;)
        else if (protId == 0xFF02)  //tez fix
        {
            // This one doesn't need to read nothing, so we could save time and bandwidth
            // Can be called thgough a program that understand the NetMsg protocol
            Status* status = Status::instance();
            status->getInfo(msg);
            msg.WriteToSocket(s);
        }
        //}
    }
    if(s)
        closesocket(s);

#if defined WIN32 || defined WINDOWS
#else
    return 0;
#endif
}



void ErrorMessage(const char* message)
{
    std::cout << std::endl << std::endl << "Error: " << message;

    std::string s;
    std::cin >> s;
}

int32_t main(int32_t argc, char *argv[])
{
#ifdef __OTSERV_ALLOCATOR_STATS__
    OTSYS_CREATE_THREAD(allocatorStatsThread, NULL);
#endif
#ifdef __WIN_LOW_FRAG_HEAP__
    ULONG  HeapFragValue = 2;

    if(HeapSetInformation(GetProcessHeap(),HeapCompatibilityInformation,&HeapFragValue,sizeof(HeapFragValue)))
    {
        std::cout << "Heap Success" << std::endl;
    }
    else
    {
        std::cout << "Heap Error" << std::endl;
    }
#endif
//    system("color 80"); //kolorek dla szpaniku :D
//    system("title Kentana OTS Engine"); // tytul ladny
    std::cout << ":: Kentana OTS 3.0" << std::endl;
    std::cout << ":: ~~By Sid/Huczu~~" << std::endl;
    std::cout << ":: Skompilowany przy pomocy " << BOOST_COMPILER << " o " << __DATE__ << ", " << __TIME__ << std::endl;
    std::cout << "::" << std::endl;

#ifdef _NO_ROOT_PERMISSION_
    if( getuid() == 0 || geteuid() == 0 )
    {
        std::cout << std::endl << "Silnik zostal uruchomiony z konta administatratora. Odpal ze zwyklego konta." << std::endl;
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
    std::cout << ":: Generator liczb...            ";
    srand ( (uint32_t)time(NULL) );
    std::cout << "[Gotowe]" << std::endl;

    // read global config
    std::cout << ":: Ladowanie config.lua...            ";
    if (!g_config.OpenFile("config.lua"))
    {
        ErrorMessage("Blad! Nie mozna zaladowac config.lua!");
        return -1;
    }
    std::cout << "[Gotowe]" << std::endl;
#if defined __WINDOWS__ || defined WIN32
    std::string Priorytet = g_config.PRIORYTET;
    if(Priorytet == "rzeczywisty")
        SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS);
    if(Priorytet == "wysoki")
        SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
    if(Priorytet == "najwyzszy")
        SetPriorityClass(GetCurrentProcess(), ABOVE_NORMAL_PRIORITY_CLASS);
#endif
    Database* db = Database::getInstance();
    if(db && db->isConnected())
    {
        std::cout << ":: Running Database Manager" << std::endl;
        if(!DatabaseManager::getInstance()->isDatabaseSetup())
            ErrorMessage("The database you specified in config.lua is empty, please import schemas/<dbengine>.sql to the database (if you are using MySQL, please read doc/MYSQL_HELP for more information).");

        DatabaseManager::getInstance()->checkTriggers();
        if(g_config.OPTIMIZE_DB_AT_STARTUP && !DatabaseManager::getInstance()->optimizeTables())
            std::cout << "> No tables were optimized." << std::endl;
#ifdef HUCZU_RECORD
        DBQuery query;
        query << "SELECT `record` FROM `server` LIMIT 1";
        DBResult* result;
        if((result = db->storeQuery(query.str())))
        {
            g_game.record = result->getDataInt("record");
            result->free();
        }
#endif
    }
    else
    {
        ErrorMessage("Couldn't estabilish connection to SQL database!");
        return -1;
    }

    //load commands data
    std::cout << ":: Ladowanie komend...          ";
    if(!commands.loadFromDB()/*loadXml(g_config.DATA_DIR)*/)
    {
        ErrorMessage("Blad! Nie mozna zaladowac komend!");
        return -1;
    }
    std::cout << "[Gotowe]" << std::endl;

    //load spells data
    std::cout << ":: Ladowanie spells.xml...            ";
    if(!spells.loadFromXml(g_config.DATA_DIR))
    {
        ErrorMessage("Blad! Nie mozna zaladowac spells.xml!");
        return -1;
    }
    std::cout << "[Gotowe]" << std::endl;

    //load actions data
    std::cout << ":: Ladowanie actions.xml...           ";
    if(!actions.loadFromXml(g_config.DATA_DIR))
    {
        ErrorMessage("Blad! Nie mozna zaladowac actions.xml!");
        return -1;
    }
    std::cout << "[Gotowe]" << std::endl;

    // load item data
    std::cout << ":: Ladowanie items.otb...             ";
    if (Item::items.loadFromOtb(g_config.DATA_DIR + "items/items.otb"))
    {
        ErrorMessage("Blad! Nie mozna zaladowac items.otb!");
        return -1;
    }
    std::cout << "[Gotowe]" << std::endl;

    std::cout << ":: Ladowanie items.xml...             ";
    if (!Item::items.loadXMLInfos(g_config.DATA_DIR + "items/items.xml"))
    {
        ErrorMessage("Blad! Nie mozna zaladowac /items/items.xml ...!");
        return -1;
    }
    std::cout << "[Gotowe]" << std::endl;

#ifdef YUR_LOGIN_QUEUE
    std::cout << ":: Ladowanie queue.xml...             ";
    if (!g_game.loginQueue.load())
    {
        ErrorMessage("Blad! Nie mozna zaladowac queue.xml!");
        return -1;
    }
    std::cout << "[Gotowe]" << std::endl;
#endif //YUR_LOGIN_QUEUE
#ifdef TR_SUMMONS
    std::cout << ":: Ladowanie summons.xml...             ";
    if (!Summons::Load())
    {
        ErrorMessage("Blad! Nie mozna zaladowac summons.xml!");
        return -1;
    }
    std::cout << "[Gotowe]" << std::endl;
#endif //TR_SUMMONS
#ifdef __MIZIAK_TASKS__
    std::cout << ":: Ladowanie tasks.xml...                ";
    if (!Tasks::Load())
    {
        ErrorMessage("Blad! Nie mozna zaladowac tasks.xml!");
        return -1;
    }
    std::cout << "[Gotowe]" << std::endl;
#endif //__MIZIAK_TASKS__
    // load monster data
    std::cout << ":: Ladowanie monsters.xml...              ";
    if(!g_monsters.loadFromXml(g_config.DATA_DIR))
    {
        ErrorMessage("Blad! Nie mozna zaladowac monsters/monsters.xml!");
        return -1;
    }
    std::cout << "[Gotowe]" << std::endl;

#ifdef HUCZU_STAGE_EXP
    if(g_config.STAGE_EXP)
    {
        std::cout << ":: Ladowanie stages.xml...                ";
        if(!g_game.loadStageExp())
        {
            ErrorMessage("Blad! Nie mozna zaladowac stages.xml!");
            return -1;
        }
        std::cout << "[Gotowe]" << std::endl;
    }
#endif

    std::string worldtype = g_config.WORLD_TYPE;
    std::transform(worldtype.begin(), worldtype.end(), worldtype.begin(), upchar);
    if(worldtype == "PVP")
        g_game.setWorldType(WORLD_TYPE_PVP);
    else if(worldtype == "NO-PVP")
        g_game.setWorldType(WORLD_TYPE_NO_PVP);
    else if(worldtype == "PVP-ENFORCED")
        g_game.setWorldType(WORLD_TYPE_PVP_ENFORCED);
    else
    {
        ErrorMessage("Nieznany World Type!");
        return -1;
    }
    std::cout << ":: World Type: " << worldtype << std::endl;

#ifdef YUR_CVS_MODS
    timer();
#endif

    // loads the map and, if needed, an extra-file spawns
    switch(g_game.loadMap(g_config.MAP_PATH, "OTBM"))
    {
    case MAP_LOADER_ERROR:
        std::cout << "FATAL: couldnt determine the map format! exiting" << std::endl;
        exit(1);
        break;
    case SPAWN_XML:
        SpawnManager::initialize(&g_game);
        SpawnManager::instance()->loadSpawnsXML(g_game.getSpawnFile());
        SpawnManager::instance()->startup();
        break;
    }

#ifdef YUR_CVS_MODS
    double t = timer();
    std::cout << ":: Mapa zaladowana w " << t << " sekund." << std::endl;
#endif

    if(g_config.LOAD_NPC != "spawn")
    {
        std::cout << ":: Ladowanie npc.xml...          ";
        if (!g_game.loadNpcs())
        {
            ErrorMessage("Nie mozna zaladowac npc.xml!");
            return -1;
        }
        std::cout << "[Gotowe]" << std::endl;
    }

#ifdef TLM_HOUSE_SYSTEM
    std::cout << ":: Ladowanie houses.xml...            ";
    if (!Houses::Load(&g_game))
    {
        ErrorMessage("Blad! Nie mozna zaladowac houses.xml!");
        return -1;
    }
    std::cout << "[Gotowe]" << std::endl;
#endif //TLM_HOUSE_SYSTEM

#ifdef YUR_PVP_ARENA
    std::cout << ":: Ladowanie pvparenas.xml...         ";
    if (!PvpArena::Load(&g_game))
    {
        ErrorMessage("Blad! Nie mozna zaladowac pvparenas.xml!");
        return -1;
    }
    std::cout << "[Gotowe]" << std::endl;
#endif //YUR_PVP_ARENA

    std::cout << ":: Ladowanie readables.xml...         ";
    if (!g_game.loadReadables())
    {
        ErrorMessage("Blad! Nie mozna zaladowac readables.xml!");
        return -1;
    }
    std::cout << "[Gotowe]" << std::endl;

    std::cout << ":: Ladowanie towns.xml...          ";
    if (!Town::loadTowns())
    {
        ErrorMessage("Blad! Nie mozna zaladowac towns.xml!");
        return -1;
    }
    std::cout << "[Gotowe]" << std::endl;

    std::cout << ":: Ladowanie napisy.xml...          ";
    if (!g_game.loadNapisy())
    {
        ErrorMessage("Blad! Nie mozna zaladowac napisy.xml!");
        return -1;
    }
    std::cout << "[Gotowe]" << std::endl;

    if (g_config.AUTO_SAVE > 0)
        g_game.addEvent(makeTask(g_config.AUTO_SAVE, std::mem_fun(&Game::autoServerSave)));
    else
        std::cout << ":: Automatyczny zapis serwera wylaczony!" << std::endl;

    if (g_config.AUTO_CLEAN > 0)
        g_game.addEvent(makeTask(g_config.AUTO_CLEAN, std::mem_fun(&Game::beforeClean)));
    else
        std::cout << ":: Automatyczny clean wylaczony!" << std::endl;

    if (g_config.AUTO_RESTART > 0)
        g_game.addEvent(makeTask(g_config.AUTO_RESTART, std::mem_fun(&Game::beforeRestart)));
    else
        std::cout << ":: Automatyczny restart wylaczony!" << std::endl;

    g_game.addEvent(makeTask(1000, std::mem_fun(&Game::checkOwner)));

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

    std::pair<uint32_t, uint32_t> IpNetMask;
    IpNetMask.first  = inet_addr("127.0.0.1");
    IpNetMask.second = 0xFFFFFFFF;
    serverIPs.push_back(IpNetMask);

    char szHostName[128];
    if (gethostname(szHostName, 128) == 0)
    {
        std::cout << "::" << std::endl << ":: Odpalony na hoscie " << szHostName << std::endl;

        hostent *he = gethostbyname(szHostName);

        if (he)
        {
            std::cout << ":: Lokalny adres IP :     ";
            unsigned char** addr = (unsigned char**)he->h_addr_list;

            while (addr[0] != NULL)
            {
                std::cout << (uint32_t)(addr[0][0]) << "."
                          << (uint32_t)(addr[0][1]) << "."
                          << (uint32_t)(addr[0][2]) << "."
                          << (uint32_t)(addr[0][3]) << "  ";

                IpNetMask.first  = *(uint32_t*)(*addr);
                IpNetMask.second = 0x0000FFFF;
                serverIPs.push_back(IpNetMask);

                addr++;
            }

            std::cout << std::endl;
        }
    }

    std::cout << ":: Globaly adres IP :     ";
    std::string ip;

    if(argc > 1)
        ip = argv[1];
    else
        ip = g_config.IP;

    std::cout << ip << std::endl << "::" << std::endl;

#ifdef YUR_CVS_MODS
    if (ip == "127.0.0.1")
        std::cout << "UWAGA! Zmien IP w config.lua!\n" << std::endl;
#endif //YUR_CVS_MODS

    IpNetMask.first  = inet_addr(ip.c_str());
    IpNetMask.second = 0;
    serverIPs.push_back(IpNetMask);
    std::cout << ":: Startowanie serwera... ";

    Status* status = Status::instance();
    status->playersmax = g_config.MAX_PLAYERS;

    // start the server listen...
    int32_t listen_errors;
    int32_t accept_errors;
    listen_errors = 0;
    g_game.setGameState(GAME_STATE_NORMAL);
    while(g_game.getGameState() != GAME_STATE_SHUTDOWN && listen_errors < 100)
    {
        sockaddr_in local_adress;
        memset(&local_adress, 0, sizeof(sockaddr_in)); // zero the struct

        local_adress.sin_family      = AF_INET;
        local_adress.sin_port        = htons(g_config.PORT);
        local_adress.sin_addr.s_addr = htonl(INADDR_ANY);

        // first we create a new socket
        SOCKET listen_socket = socket(AF_INET, SOCK_STREAM, 0);

        if(listen_socket <= 0)
        {
#ifdef WIN32
            WSACleanup();
#endif
            ErrorMessage("Unable to create server socket (1)!");
            return -1;
        } // if (listen_socket <= 0)

#ifndef WIN32
        int32_t yes = 1;
        // lose the pesky "Address already in use" error message
        if(setsockopt(listen_socket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof (int32_t)) == -1)
        {
            ErrorMessage("Unable to set socket options!");
            return -1;
        }
#endif

        // bind socket on port
        if(::bind(listen_socket, (struct sockaddr*)&local_adress, sizeof(struct sockaddr_in)) < 0)
        {
#ifdef WIN32
            WSACleanup();
#endif
            ErrorMessage("Unable to create server socket (2)!");
            return -1;
        } // if (bind(...))

        // now we start listen on the new socket
        if(listen(listen_socket, 10) == SOCKET_ERROR)
        {
#ifdef WIN32
            WSACleanup();
#endif
            ErrorMessage("Listen on server socket not possible!");
            return -1;
        } // if (listen(*listen_socket, 10) == -1)


        std::cout << "[Gotowe]" << std::endl << ":: " << g_config.SERVER_NAME << " uruchomiony..." << std::endl;
        accept_errors = 0;
        while(g_game.getGameState() != GAME_STATE_SHUTDOWN && accept_errors < 100)
        {

            fd_set listen_set;
            timeval tv;
            FD_ZERO(&listen_set);
            FD_SET(listen_socket, &listen_set);
            tv.tv_sec = 2;
            tv.tv_usec = 0;

            int32_t reads = (int32_t)select(listen_socket + 1, &listen_set, NULL, NULL, &tv); //VISUAL
            if(reads == SOCKET_ERROR)
            {
                int32_t errnum;
#ifdef WIN32
                errnum = WSAGetLastError();
#else
                errnum = errno;
#endif

                if(errnum == ERROR_EINTR)
                {
                    continue;
                }
                else
                {
                    SOCKET_PERROR("select");
                    break;
                }
            }
            else if(reads == 0)
            {
                continue;
            }

            SOCKET s = accept(listen_socket, NULL, NULL); // accept a new connection
            OTSYS_SLEEP(100);
            if(s > 0)
            {
                OTSYS_CREATE_THREAD(ConnectionHandler, (void*)&s);
            }
            else
            {
                accept_errors++;
                SOCKET_PERROR("accept");
            }
        }
        closesocket(listen_socket);
        listen_errors++;
    }
    if(listen_errors >= 100)
    {
        std::cout << "ERROR: Server shutted down because there where 100 listen errors." << std::endl;
    }

#ifdef WIN32
    WSACleanup();
#endif
    return 0;
}
