#include <iostream>

#include "databasemanager.h"
#include "tools.h"

#include "luascript.h"
extern LuaScript g_config;

bool DatabaseManager::optimizeTables()
{
    Database* db = Database::getInstance();
    DBQuery query;

    query << "SELECT `TABLE_NAME` FROM `information_schema`.`tables` WHERE `TABLE_SCHEMA` = " << db->escapeString(g_config.SQL_DB) << " AND `DATA_FREE` > 0;";

    DBResult* result;

    if(!(result = db->storeQuery(query.str())))
        return false;

    query.str("");
    do
    {
        std::cout << "> Optimizing table: " << result->getDataString("TABLE_NAME") << "... ";
        query << "OPTIMIZE TABLE `" << result->getDataString("TABLE_NAME") << "`;";
        if(db->executeQuery(query.str()))
            std::cout << "[success]" << std::endl;
        else
            std::cout << "[failure]" << std::endl;

        query.str("");
    }
    while(result->next());

    result->free();
    return true;

    return false;
}

bool DatabaseManager::triggerExists(std::string trigger)
{
    Database* db = Database::getInstance();
    DBQuery query;

    query << "SELECT `TRIGGER_NAME` FROM `information_schema`.`triggers` WHERE `TRIGGER_SCHEMA` = " << db->escapeString(g_config.SQL_DB) << " AND `TRIGGER_NAME` = " << db->escapeString(trigger) << ";";

    DBResult* result;
    if(!(result = db->storeQuery(query.str())))
        return false;

    result->free();
    return true;
}

bool DatabaseManager::tableExists(std::string table)
{
    Database* db = Database::getInstance();
    DBQuery query;

    query << "SELECT `TABLE_NAME` FROM `information_schema`.`tables` WHERE `TABLE_SCHEMA` = " << db->escapeString(g_config.SQL_DB) << " AND `TABLE_NAME` = " << db->escapeString(table) << ";";

    DBResult* result;
    if(!(result = db->storeQuery(query.str())))
        return false;

    result->free();
    return true;
}

bool DatabaseManager::isDatabaseSetup()
{
    Database* db = Database::getInstance();
    DBQuery query;
    query << "SELECT `TABLE_NAME` FROM `information_schema`.`tables` WHERE `TABLE_SCHEMA` = " << db->escapeString(g_config.SQL_DB) << ";";

    DBResult* result;
    if(!(result = db->storeQuery(query.str())))
        return false;

    result->free();
    return true;

    return false;
}

void DatabaseManager::checkTriggers()
{
    Database* db = Database::getInstance();
    std::string triggerName[] =
    {
        "oncreate_guilds",
        "ondelete_guilds",
        "oncreate_players",
    };

    std::string triggerStatement[] =
    {
        "CREATE TRIGGER `oncreate_guilds` AFTER INSERT ON `guilds` FOR EACH ROW BEGIN INSERT INTO `guild_ranks` (`name`, `level`, `guild_id`) VALUES ('Leader', 3, NEW.`id`); INSERT INTO `guild_ranks` (`name`, `level`, `guild_id`) VALUES ('Vice-Leader', 2, NEW.`id`); INSERT INTO `guild_ranks` (`name`, `level`, `guild_id`) VALUES ('Member', 1, NEW.`id`); END;",
        "CREATE TRIGGER `ondelete_guilds` BEFORE DELETE ON `guilds` FOR EACH ROW BEGIN UPDATE `players` SET `guildnick` = '', `rank_id` = 0 WHERE `rank_id` IN (SELECT `id` FROM `guild_ranks` WHERE `guild_id` = OLD.`id`); END;",
        "CREATE TRIGGER `oncreate_players` AFTER INSERT ON `players` FOR EACH ROW BEGIN INSERT INTO `player_skills` (`player_id`, `skillid`, `value`) VALUES (NEW.`id`, 0, 10); INSERT INTO `player_skills` (`player_id`, `skillid`, `value`) VALUES (NEW.`id`, 1, 10); INSERT INTO `player_skills` (`player_id`, `skillid`, `value`) VALUES (NEW.`id`, 2, 10); INSERT INTO `player_skills` (`player_id`, `skillid`, `value`) VALUES (NEW.`id`, 3, 10); INSERT INTO `player_skills` (`player_id`, `skillid`, `value`) VALUES (NEW.`id`, 4, 10); INSERT INTO `player_skills` (`player_id`, `skillid`, `value`) VALUES (NEW.`id`, 5, 10); INSERT INTO `player_skills` (`player_id`, `skillid`, `value`) VALUES (NEW.`id`, 6, 10); END;",
    };

    DBQuery query;
    for(uint32_t i = 0; i < sizeof(triggerName) / sizeof(std::string); i++)
    {
        if(!triggerExists(triggerName[i]))
        {
            std::cout << "> Trigger: " << triggerName[i] << " does not exist, creating it..." << std::endl;
            db->executeQuery(triggerStatement[i]);
        }
    }
}
