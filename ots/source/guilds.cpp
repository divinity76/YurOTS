
#include "guilds.h"
#include "database.h"
#include "game.h"
#include "const76.h"

extern Game g_game;
extern LuaScript g_config;

bool Guild::getGuildId(uint32_t& id, const std::string& name)
{
    Database* db = Database::getInstance();
    DBResult* result;

    DBQuery query;
    query << "SELECT `id` FROM `guilds` WHERE `name` " << db->getStringComparison() << db->escapeString(name) << " LIMIT 1";
    if(!(result = db->storeQuery(query.str())))
        return false;

    id = result->getDataInt("id");
    result->free();
    return true;
}

bool Guild::getGuildById(std::string& name, uint32_t id)
{
    Database* db = Database::getInstance();
    DBResult* result;

    DBQuery query;
    query << "SELECT `name` FROM `guilds` WHERE `id` = " << id << " LIMIT 1";
    if(!(result = db->storeQuery(query.str())))
        return false;

    name = result->getDataString("name");
    result->free();
    return true;
}

bool Guild::swapGuildIdToOwner(uint32_t& value)
{
    Database* db = Database::getInstance();
    DBResult* result;

    DBQuery query;
    query << "SELECT `ownerid` FROM `guilds` WHERE `id` = " << value << " LIMIT 1";
    if(!(result = db->storeQuery(query.str())))
        return false;

    value = result->getDataInt("ownerid");
    result->free();
    return true;
}

bool Guild::guildExists(uint32_t guild)
{
    Database* db = Database::getInstance();
    DBResult* result;

    DBQuery query;
    query << "SELECT `id` FROM `guilds` WHERE `id` = " << guild << " LIMIT 1";
    if(!(result = db->storeQuery(query.str())))
        return false;

    result->free();
    return true;
}

uint32_t Guild::getRankIdByName(uint32_t guild, const std::string& name)
{
    Database* db = Database::getInstance();
    DBResult* result;

    DBQuery query;
    query << "SELECT `id` FROM `guild_ranks` WHERE `guild_id` = " << guild << " AND `name` " << db->getStringComparison() << db->escapeString(name) << " LIMIT 1";
    if(!(result = db->storeQuery(query.str())))
        return 0;

    const uint32_t id = result->getDataInt("id");
    result->free();
    return id;
}

uint32_t Guild::getRankIdByLevel(uint32_t guild, GuildLevel_t level)
{
    Database* db = Database::getInstance();
    DBResult* result;

    DBQuery query;
    query << "SELECT `id` FROM `guild_ranks` WHERE `guild_id` = " << guild << " AND `level` = " << level << " LIMIT 1";
    if(!(result = db->storeQuery(query.str())))
        return 0;

    const uint32_t id = result->getDataInt("id");
    result->free();
    return id;
}

bool Guild::getRankEx(uint32_t& id, std::string& name, uint32_t guild, GuildLevel_t level)
{
    Database* db = Database::getInstance();
    DBResult* result;

    DBQuery query;
    query << "SELECT `id`, `name` FROM `guild_ranks` WHERE `guild_id` = " << guild << " AND `level` = " << level;
    if(id)
        query << " AND `id` = " << id;

    query << " LIMIT 1";
    if(!(result = db->storeQuery(query.str())))
        return false;

    name = result->getDataString("name");
    if(!id)
        id = result->getDataInt("id");

    result->free();
    return true;
}

std::string Guild::getRank(uint32_t guid)
{
    Database* db = Database::getInstance();
    DBResult* result;

    DBQuery query;
    query << "SELECT `guild_ranks`.`name` FROM `players`, `guild_ranks` WHERE `players`.`id` = " << guid << " AND `guild_ranks`.`id` = `players`.`rank_id` LIMIT 1";
    if(!(result = db->storeQuery(query.str())))
        return "";

    const std::string name = result->getDataString("name");
    result->free();
    return name;
}

bool Guild::changeRank(uint32_t guild, const std::string& oldName, const std::string& newName)
{
    Database* db = Database::getInstance();
    DBResult* result;

    DBQuery query;
    query << "SELECT `id` FROM `guild_ranks` WHERE `guild_id` = " << guild << " AND `name` " << db->getStringComparison() << db->escapeString(oldName) << " LIMIT 1";
    if(!(result = db->storeQuery(query.str())))
        return false;

    const uint32_t id = result->getDataInt("id");
    result->free();

    query.str("");
    query << "UPDATE `guild_ranks` SET `name` = " << db->escapeString(newName) << " WHERE `id` = " << id << db->getUpdateLimiter();
    if(!db->executeQuery(query.str()))
        return false;

    for(AutoList<Player>::listiterator it = Player::listPlayer.list.begin(); it != Player::listPlayer.list.end(); ++it)
    {
        if(it->second->getRankId() == id)
            it->second->setRankName(newName);
    }

    return true;
}

bool Guild::createGuild(Player* player)
{
    Database* db = Database::getInstance();
    DBResult* result;

    DBQuery query;
    query << "INSERT INTO `guilds` (`id`, `name`, `ownerid`, `creationdata`, `motd`) VALUES (NULL, " << db->escapeString(player->getGuildName()) << ", " << player->getGUID() << ", " << time(NULL) << ", 'Your guild has been successfully created, to view all available commands type: !commands. If you would like to remove this message use !cleanmotd and to set new motd use !setmotd text.')";
    if(!db->executeQuery(query.str()))
        return false;

    query.str("");
    query << "SELECT `id` FROM `guilds` WHERE `ownerid` = " << player->getGUID() << " LIMIT 1";
    if(!(result = db->storeQuery(query.str())))
        return false;

    const uint32_t guildId = result->getDataInt("id");
    result->free();
    return joinGuild(player, guildId, true);
}

bool Guild::joinGuild(Player* player, uint32_t guildId, bool creation/* = false*/)
{
    Database* db = Database::getInstance();
    DBResult* result;

    DBQuery query;
    query << "SELECT `id` FROM `guild_ranks` WHERE `guild_id` = " << guildId << " AND `level` = " << (creation ? "3" : "1") << " LIMIT 1";
    if(!(result = db->storeQuery(query.str())))
        return false;

    const uint32_t rankId = result->getDataInt("id");
    result->free();

    std::string guildName;
    if(!creation)
    {
        query.str("");
        query << "SELECT `name` FROM `guilds` WHERE `id` = " << guildId << " LIMIT 1";
        if(!(result = db->storeQuery(query.str())))
            return false;

        guildName = result->getDataString("name");
        result->free();
    }

    query.str("");
    query << "UPDATE `players` SET `rank_id` = " << rankId << " WHERE `id` = " << player->getGUID() << db->getUpdateLimiter();
    if(!db->executeQuery(query.str()))
        return false;

    player->setGuildId(guildId);
    GuildLevel_t level = GUILD_MEMBER;
    if(!creation)
        player->setGuildName(guildName);
    else
        level = GUILD_LEADER;

    player->setGuildLevel(level, rankId);
    player->invitedToGuildsList.clear();
    return true;
}

bool Guild::disbandGuild(uint32_t guildId)
{
    Database* db = Database::getInstance();

    DBQuery query;
    query << "UPDATE `players` SET `rank_id` = '' AND `guildnick` = '' WHERE `rank_id` = " << getRankIdByLevel(guildId, GUILD_LEADER) << " OR rank_id = " << getRankIdByLevel(guildId, GUILD_VICE) << " OR rank_id = " << getRankIdByLevel(guildId, GUILD_MEMBER);
    if(!db->executeQuery(query.str()))
        return false;

    for(AutoList<Player>::listiterator it = Player::listPlayer.list.begin(); it != Player::listPlayer.list.end(); ++it)
    {
        if(it->second->getGuildId() == guildId)
            it->second->leaveGuild();
    }

    query.str("");
    query << "DELETE FROM `guilds` WHERE `id` = " << guildId << " LIMIT 1";
    if(!db->executeQuery(query.str()))
        return false;

    query.str("");
    query << "DELETE FROM `guild_invites` WHERE `guild_id` = " << guildId;
    if(!db->executeQuery(query.str()))
        return false;

    query.str("");
    query << "DELETE FROM `guild_ranks` WHERE `guild_id` = " << guildId;
    return db->executeQuery(query.str());
}

bool Guild::hasGuild(uint32_t guid)
{
    Database* db = Database::getInstance();
    DBResult* result;

    DBQuery query;
    query << "SELECT `rank_id` FROM `players` WHERE `id` = " << guid << " LIMIT 1";
    if(!(result = db->storeQuery(query.str())))
        return false;

    const bool ret = result->getDataInt("rank_id") != 0;
    result->free();
    return ret;
}

bool Guild::isInvited(uint32_t guild, uint32_t guid)
{
    Database* db = Database::getInstance();
    DBResult* result;

    DBQuery query;
    query << "SELECT `id` FROM `guild_invites` WHERE `player_id` = " << guid << " AND `guild_id`= " << guild << " LIMIT 1";
    if(!(result = db->storeQuery(query.str())))
        return false;

    result->free();
    return true;
}

bool Guild::invitePlayer(uint32_t guild, uint32_t guid)
{
    Database* db = Database::getInstance();
    DBQuery query;
    query << "INSERT INTO `guild_invites` (`player_id`, `guild_id`) VALUES ('" << guid << "', '" << guild << "')";
    return db->executeQuery(query.str());
}

bool Guild::revokeInvite(uint32_t guild, uint32_t guid)
{
    Database* db = Database::getInstance();
    DBQuery query;
    query << "DELETE FROM `guild_invites` WHERE `player_id` = " << guid << " AND `guild_id` = " << guild;
    return db->executeQuery(query.str());
}

uint32_t Guild::getGuildId(uint32_t guid)
{
    Database* db = Database::getInstance();
    DBResult* result;

    DBQuery query;
    query << "SELECT `guild_ranks`.`guild_id` FROM `players`, `guild_ranks` WHERE `players`.`id` = " << guid << " AND `guild_ranks`.`id` = `players`.`rank_id` LIMIT 1";
    if(!(result = db->storeQuery(query.str())))
        return 0;

    const uint32_t guildId = result->getDataInt("guild_id");
    result->free();
    return guildId;
}

GuildLevel_t Guild::getGuildLevel(uint32_t guid)
{
    Database* db = Database::getInstance();
    DBResult* result;

    DBQuery query;
    query << "SELECT `guild_ranks`.`level` FROM `players`, `guild_ranks` WHERE `players`.`id` = " << guid << " AND `guild_ranks`.`id` = `players`.`rank_id` LIMIT 1";
    if(!(result = db->storeQuery(query.str())))
        return GUILD_NONE;

    const GuildLevel_t level = (GuildLevel_t)result->getDataInt("level");
    result->free();
    return level;
}

bool Guild::setGuildLevel(uint32_t guid, GuildLevel_t level)
{
    Database* db = Database::getInstance();
    DBResult* result;

    DBQuery query;
    query << "SELECT `id` FROM `guild_ranks` WHERE `guild_id` = " << getGuildId(guid) << " AND `level` = " << level << " LIMIT 1";
    if(!(result = db->storeQuery(query.str())))
        return false;

    query.str("");
    query << "UPDATE `players` SET `rank_id` = " << result->getDataInt("id") << " WHERE `id` = " << guid << db->getUpdateLimiter();
    result->free();
    return db->executeQuery(query.str());
}

bool Guild::updateOwnerId(uint32_t guild, uint32_t guid)
{
    Database* db = Database::getInstance();
    DBQuery query;
    query << "UPDATE `guilds` SET `ownerid` = " << guid << " WHERE `id` = " << guild << db->getUpdateLimiter();
    return db->executeQuery(query.str());
}

bool Guild::setGuildNick(uint32_t guid, const std::string& nick)
{
    Database* db = Database::getInstance();
    DBQuery query;
    query << "UPDATE `players` SET `guildnick` = " << db->escapeString(nick) << " WHERE `id` = " << guid << db->getUpdateLimiter();
    return db->executeQuery(query.str());
}

bool Guild::setMotd(uint32_t guild, const std::string& newMessage)
{
    Database* db = Database::getInstance();
    DBQuery query;
    query << "UPDATE `guilds` SET `motd` = " << db->escapeString(newMessage) << " WHERE `id` = " << guild << db->getUpdateLimiter();
    return db->executeQuery(query.str());
}

std::string Guild::getMotd(uint32_t guild)
{
    Database* db = Database::getInstance();
    DBResult* result;

    DBQuery query;
    query << "SELECT `motd` FROM `guilds` WHERE `id` = " << guild << " LIMIT 1";
    if(!(result = db->storeQuery(query.str())))
        return "";

    const std::string motd = result->getDataString("motd");
    result->free();
    return motd;
}
