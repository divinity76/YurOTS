
#ifndef __OTSERV_STATUS_H
#define __OTSERV_STATUS_H

#include <string>
#include "otsystem.h"
#include "networkmessage.h"


class Status
{
public:
    // procs
    void addPlayer();
    void removePlayer();
    static Status* instance();
    std::string getStatusString();
    void getInfo(NetworkMessage &nm);
    bool hasSlot();

    // vars
    int32_t playersonline, playersmax, playerspeak;
    std::string ownername, owneremail;
    std::string motd;
    std::string mapname, mapauthor;
    int32_t mapsizex, mapsizey;
    std::string servername, location, url;
    std::string version;
    uint64_t start;

#ifdef YUR_LOGIN_QUEUE
    int32_t playerswaiting;
#endif //YUR_LOGIN_QUEUE

private:
    Status();
    static Status* _Status;
};
#endif
