
//#include "preheaders.h"
#include "tile.h"
#include "otsystem.h"
#include <string>
#include "protocol.h"

class Player;

extern Game g_game;


Protocol::Protocol()
{
    player = NULL;
    game = NULL;
    pendingLogout = false;
}


Protocol::~Protocol()
{
    if(s)
    {
        closesocket(s);
        s = 0;
    }

    player = NULL;
    game = NULL;
}

uint32_t Protocol::getIP() const
{
    sockaddr_in sain;
    socklen_t salen = sizeof(sockaddr_in);
    if (getpeername(s, (sockaddr*)&sain, &salen) == 0)
    {
#if defined WIN32 || defined __WINDOWS__
        return sain.sin_addr.S_un.S_addr;
#else
        return sain.sin_addr.s_addr;
#endif
    }

    return 0;
}

void Protocol::setPlayer(Player* p)
{
    player = p;
    game   = &g_game;
}

void Protocol::sleepTillMove()
{
    int64_t delay = player->getSleepTicks();
    if(delay > 0 )
    {

#if __DEBUG__
        std::cout << "Delaying "<< player->getName() << " --- " << delay << std::endl;
#endif

        OTSYS_SLEEP((uint32_t)delay);
    }

    player->lastmove = OTSYS_TIME();
}
