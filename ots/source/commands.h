
#ifndef __commands_h_
#define __commands_h_

#include <string>
#include <map>
#include "creature.h"

class Game;
struct Command;
struct s_defcommands;

class Commands
{
public:
    Commands():game(NULL),loaded(false) {};
    Commands(Game* igame);

    //bool loadXml(const std::string &_datadir);
    bool loadFromDB();
    bool isLoaded()
    {
        return loaded;
    }
    bool reload();

    bool exeCommand(Creature *creature, const std::string &cmd);
    void logAction(Player *p, int32_t maxaccess, std::string filename, std::string text);

protected:
    bool loaded;
    Game *game;
    std::string datadir;

    //commands
    bool placeNpc(Creature* c, const std::string &cmd, const std::string &param);
    bool placeMonster(Creature* c, const std::string &cmd, const std::string &param);
    bool broadcastMessage(Creature* c, const std::string &cmd, const std::string &param);
    bool RedBezNicka(Creature* c, const std::string &cmd, const std::string &param);
    bool teleportMasterPos(Creature* c, const std::string &cmd, const std::string &param);
    bool teleportHere(Creature* c, const std::string &cmd, const std::string &param);
    bool teleportTo(Creature* c, const std::string &cmd, const std::string &param);
    bool createItems(Creature* c, const std::string &cmd, const std::string &param);
    bool reloadInfo(Creature* c, const std::string &cmd, const std::string &param);
    bool getInfo(Creature* c, const std::string &cmd, const std::string &param);
    bool closeServer(Creature* c, const std::string &cmd, const std::string &param);
    bool openServer(Creature* c, const std::string &cmd, const std::string &param);
    bool teleportNTiles(Creature* c, const std::string &cmd, const std::string &param);
    bool kickPlayer(Creature* c, const std::string &cmd, const std::string &param);
    bool namelockPlayer(Creature* c, const std::string &cmd, const std::string &param);
    bool male(Creature* c, const std::string &cmd, const std::string &param);
    bool female(Creature* c, const std::string &cmd, const std::string &param);
    bool dwarf(Creature* c, const std::string &cmd, const std::string &param);
    bool nimfa(Creature* c, const std::string &cmd, const std::string &param);

#ifdef TRS_GM_INVISIBLE
    bool gmInvisible(Creature* c, const std::string &cmd, const std::string &param);
#endif //TRS_GM_INVISIBLE

    bool goUp(Creature* c, const std::string &cmd, const std::string &param);
    bool goDown(Creature* c, const std::string &cmd, const std::string &param);
    bool showExpForLvl(Creature* c, const std::string &cmd, const std::string &param);
    bool showManaForLvl(Creature* c, const std::string &cmd, const std::string &param);
    bool whoIsOnline(Creature* c, const std::string &cmd, const std::string &param);
    bool teleportPlayerTo(Creature* c, const std::string &cmd, const std::string &param);
    bool showPos(Creature* c, const std::string &cmd, const std::string &param);
    bool showUptime(Creature* c, const std::string &cmd, const std::string &param);
    bool setMaxPlayers(Creature* c, const std::string &cmd, const std::string &param);

#ifdef TLM_HOUSE_SYSTEM
    bool reloadRights(Creature* c, const std::string &cmd, const std::string &param);
    bool setHouseOwner(Creature* c, const std::string &cmd, const std::string &param);
#endif //TLM_HOUSE_SYSTEM

#ifdef HUCZU_SKULLS
    bool noskull(Creature* c, const std::string &cmd, const std::string &param);
    bool PokazRs(Creature* c, const std::string &cmd, const std::string &param);
    bool PokazFragi(Creature* c, const std::string &cmd, const std::string &param);
    bool PokazPz(Creature* c, const std::string &cmd, const std::string &param);
#endif //HUCZU_SKULLS

    bool forceServerSave(Creature* c, const std::string &cmd, const std::string &param);
    bool shutdown(Creature* c, const std::string &cmd, const std::string &param);
    bool cleanMap(Creature* c, const std::string &cmd, const std::string &param);

#ifdef YUR_PREMIUM_PROMOTION
    bool promote(Creature* c, const std::string &cmd, const std::string &param);
    bool premmy(Creature* c, const std::string &cmd, const std::string &param);
    bool showPremmy(Creature* c, const std::string &cmd, const std::string &param);
#endif //YUR_PREMIUM_PROMOTION

    bool GadkaBDD(Creature* c, const std::string &cmd, const std::string &param);
    bool nowanazwaitema(Creature* c, const std::string &cmd, const std::string &param);
    bool playerRemoveItem(Creature* c, const std::string &cmd, const std::string &param);
    bool countItem(Creature* c, const std::string& cmd, const std::string& param);
#ifdef HUCZU_ENFO
    bool countFrags(Creature* c, const std::string& cmd, const std::string& param);
#endif
    bool cleanHouses(Creature* c, const std::string& cmd, const std::string& param);
    bool teleporter(Creature* c, const std::string &cmd, const std::string &param);
#ifdef HUCZU_PAY_SYSTEM
    bool doladowanie(Creature* c, const std::string &cmd, const std::string &param);
    bool showPunkty(Creature* c, const std::string &cmd, const std::string &param);
#endif
#ifdef RAID
    bool doRaid(Creature* c, const std::string &cmd, const std::string &param);
#endif
    bool guildJoin(Creature* c, const std::string &cmd, const std::string &param);
    bool guildCreate(Creature* c, const std::string &cmd, const std::string &param);

    //table of commands
    static s_defcommands defined_commands[];

    typedef std::map<std::string,Command*> CommandMap;
    CommandMap commandMap;
};

typedef  bool (Commands::*CommandFunc)(Creature*,const std::string&,const std::string&);

struct Command
{
    CommandFunc f;
    int32_t accesslevel;
    bool loaded;
};

struct s_defcommands
{
    const char *name;
    CommandFunc f;
};

#endif //__commands_h_
