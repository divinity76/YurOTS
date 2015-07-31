
//#include "preheaders.h"
#include "status.h"
#include "luascript.h"
#include <sstream>
#include "game.h"
#include "networkmessage.h"

#ifndef WIN32
#define SOCKET_ERROR -1
#define INVALID_SOCKET -1
#endif

extern LuaScript g_config;
extern Game g_game;
#define KENTANA_VERSION "3.0"

Status* Status::_Status = NULL;

Status* Status::instance()
{
    if(_Status == NULL)
        _Status = new Status();
    return _Status;
}

Status::Status()
{
    this->playersonline = 0;
    this->playersmax    = 0;
    this->playerspeak   = 0;

#ifdef YUR_LOGIN_QUEUE
    this->playerswaiting = 0;
#endif //YUR_LOGIN_QUEUE

    this->start=OTSYS_TIME();
}

void Status::addPlayer()
{
    this->playersonline++;
    if(playerspeak < playersonline)
        playerspeak = playersonline;
}
void Status::removePlayer()
{
    this->playersonline--;
}

std::string Status::getStatusString()
{
    std::string xml;

    std::stringstream ss;

    xmlDocPtr doc;
    xmlNodePtr p, root;

    doc = xmlNewDoc((const xmlChar*)"1.0");
    doc->children = xmlNewDocNode(doc, NULL, (const xmlChar*)"tsqp", NULL);
    root=doc->children;

    xmlSetProp(root, (const xmlChar*) "version", (const xmlChar*)"3.0");


    p=xmlNewNode(NULL,(const xmlChar*)"serverinfo");
    uint64_t running = (OTSYS_TIME() - this->start)/1000;
    ss << running;
    xmlSetProp(p, (const xmlChar*) "uptime", (const xmlChar*)ss.str().c_str());
    ss.str("");
    xmlSetProp(p, (const xmlChar*) "ip", (const xmlChar*)g_config.getGlobalString("ip", "").c_str());
    xmlSetProp(p, (const xmlChar*) "servername", (const xmlChar*)g_config.getGlobalString("servername", "").c_str());
    std::stringstream port;
    port << g_config.PORT;
    xmlSetProp(p, (const xmlChar*) "port", (const xmlChar*)port.str().c_str());
    xmlSetProp(p, (const xmlChar*) "location", (const xmlChar*)g_config.getGlobalString("location", "").c_str());
    xmlSetProp(p, (const xmlChar*) "url", (const xmlChar*)g_config.getGlobalString("url", "").c_str());
    xmlSetProp(p, (const xmlChar*) "server", (const xmlChar*)"Huczu Engine");
    xmlSetProp(p, (const xmlChar*) "version", (const xmlChar*)KENTANA_VERSION);
    xmlAddChild(root, p);

    p=xmlNewNode(NULL,(const xmlChar*)"owner");
    xmlSetProp(p, (const xmlChar*) "name", (const xmlChar*)"Huczu");
    xmlSetProp(p, (const xmlChar*) "email", (const xmlChar*)"admin@huczu.pl");
    xmlAddChild(root, p);

    p=xmlNewNode(NULL,(const xmlChar*)"players");
    ss << this->playersonline;
    xmlSetProp(p, (const xmlChar*) "online", (const xmlChar*)ss.str().c_str());
    ss.str("");
    ss << this->playersmax;
    xmlSetProp(p, (const xmlChar*) "max", (const xmlChar*)ss.str().c_str());
    ss.str("");
    ss << this->playerspeak;
    xmlSetProp(p, (const xmlChar*) "peak", (const xmlChar*)ss.str().c_str());
    ss.str("");
#ifdef YUR_LOGIN_QUEUE
    ss << this->playerswaiting;
    xmlSetProp(p, (const xmlChar*) "waiting", (const xmlChar*)ss.str().c_str());
    ss.str("");
#endif //YUR_LOGIN_QUEUE
    xmlAddChild(root, p);

    p=xmlNewNode(NULL,(const xmlChar*)"monsters");
    ss << g_game.getMonstersOnline();
    xmlSetProp(p, (const xmlChar*) "total", (const xmlChar*)ss.str().c_str());
    ss.str("");
    xmlAddChild(root, p);

    p=xmlNewNode(NULL,(const xmlChar*)"map");
    xmlSetProp(p, (const xmlChar*) "name", (const xmlChar*)this->mapname.c_str());
    xmlSetProp(p, (const xmlChar*) "author", (const xmlChar*)this->mapauthor.c_str());
    xmlSetProp(p, (const xmlChar*) "width", (const xmlChar*)"");
    xmlSetProp(p, (const xmlChar*) "height", (const xmlChar*)"");
    xmlAddChild(root, p);

    xmlNewTextChild(root, NULL, (const xmlChar*)"motd", (const xmlChar*)g_config.getGlobalString("motd", "").c_str());

    char *s = NULL;
    int32_t len = 0;
    xmlDocDumpMemory(doc, (xmlChar**)&s, &len);

    if(s)
        xml = std::string(s, len);
    else
        xml = "";

    xmlFreeOTSERV(s);
    xmlFreeDoc(doc);

    return xml;
}

void Status::getInfo(NetworkMessage &nm)
{
    // the client selects which information may be
    // sent back, so we'll save some bandwidth and
    // make many
    bool bserverinfo0 = nm.GetByte() == 1;
    bool bserverinfo1 = nm.GetByte() == 1;
    bool bserverinfo2 = nm.GetByte() == 1;
    bool bplayersinfo = nm.GetByte() == 1;
    bool bmapinfo     = nm.GetByte() == 1;

    nm.Reset();
    uint64_t running = (OTSYS_TIME() - this->start) / 1000;
    // since we haven't all the things on the right place like map's
    // creator/info and other things, i'll put the info chunked into
    // operators, so the httpd server will only receive the existing
    // properties of the server, such serverinfo, playersinfo and so

    if (bserverinfo0)
    {
        nm.AddByte(0x10); // server info
        nm.AddString(g_config.getGlobalString("servername", "").c_str());
        nm.AddString(g_config.getGlobalString("ip", "").c_str());
        std::stringstream port;
        port << g_config.PORT;
        nm.AddString(port.str().c_str());
    }

    if (bserverinfo1)
    {
        nm.AddByte(0x11); // server info - owner info
        nm.AddString("Huczu");
        nm.AddString("admin@huczu.pl");
    }

    if (bserverinfo2)
    {
        nm.AddByte(0x12); // server info - misc
        nm.AddString(g_config.getGlobalString("motd", "").c_str());
        nm.AddString(g_config.getGlobalString("location", "").c_str());
        nm.AddString(g_config.getGlobalString("url", "").c_str());
        nm.AddU32((uint32_t)(running >> 32)); // this method prevents a big number parsing
        nm.AddU32((uint32_t)(running));       // since servers can be online for months ;)
        nm.AddString(KENTANA_VERSION);
        //nm.AddString("0.4.1");
    }

    if (bplayersinfo)
    {
        nm.AddByte(0x20); // players info
        nm.AddU32(this->playersonline);
        nm.AddU32(this->playersmax);
        nm.AddU32(this->playerspeak);
    }

    if (bmapinfo)
    {
        nm.AddByte(0x30); // map info
        nm.AddString(this->mapname.c_str());
        nm.AddString(this->mapauthor.c_str());
        int32_t mw, mh;
        g_game.getMapDimensions(mw, mh);
        nm.AddU16(mw);
        nm.AddU16(mh);
    }

    return;
    // just one thing, I'm good with monospaced text, right?
    // if you haven't undertood the joke, look at the top ^^
}

bool Status::hasSlot()
{
    return this->playersonline < this->playersmax;
}

