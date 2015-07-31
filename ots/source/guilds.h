#ifndef __GUILD__
#define __GUILD__
#include "otsystem.h"

#include "player.h"
class Guild
{
public:
    virtual ~Guild() {}
    static Guild* getInstance()
    {
        static Guild instance;
        return &instance;
    }

    bool guildExists(uint32_t guild);
    bool createGuild(Player* player);
    bool disbandGuild(uint32_t guild);

    std::string getMotd(uint32_t guild);
    bool setMotd(uint32_t guild, const std::string& newMessage);

    GuildLevel_t getGuildLevel(uint32_t guid);
    bool setGuildLevel(uint32_t guid, GuildLevel_t level);

    bool invitePlayer(uint32_t gulid, uint32_t guid);
    bool revokeInvite(uint32_t guild, uint32_t guid);
    bool joinGuild(Player* player, uint32_t guildId, bool creation = false);

    std::string getRank(uint32_t guid);
    bool changeRank(uint32_t guild, const std::string& oldName, const std::string& newName);

    bool hasGuild(uint32_t guid);
    bool isInvited(uint32_t guild, uint32_t guid);

    bool getGuildId(uint32_t& id, const std::string& name);
    bool getGuildById(std::string& name, uint32_t id);

    uint32_t getRankIdByLevel(uint32_t guild, GuildLevel_t level);
    uint32_t getRankIdByName(uint32_t guild, const std::string& name);
    bool getRankEx(uint32_t& id, std::string& name, uint32_t guild, GuildLevel_t level);

    uint32_t getGuildId(uint32_t guid);
    bool setGuildNick(uint32_t guid, const std::string& nick);

    bool swapGuildIdToOwner(uint32_t& value);
    bool updateOwnerId(uint32_t guild, uint32_t guid);

private:
    Guild() {}
};
#endif
