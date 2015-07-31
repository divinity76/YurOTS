#include "ioplayersql.h"
#include "ioaccountsql.h"
#include "item.h"

#include "tools.h"
#include "definitions.h"

#include <iostream>
#include <iomanip>



bool IOPlayerSQL::loadPlayer(Player* player, std::string name)
{
    Database* db = Database::getInstance();
    DBQuery query;
    query << "SELECT `id`, `account_id`, `sex`, `lookdir`, `level`, `exp`, `maglevel`, `vocation`, `access`, `frags`, `namelock`, ";
    query << "`premticks`, `promoted`, `punkty`, `cap`, `maxdepotitems`, `lastlogin`, `mana`, `manamax`, `manaspent`, `health`, ";
    query << "`healthmax`, `food`, `looktype`, `lookhead`, `lookbody`, `looklegs`, `lookfeet`, `posx`, ";
    query << "`posy`, `posz`, `banned`, `banstart`, `banend`, `comment`, `reason`, `action`, `deleted`, ";
    query << "`finalwarning`, `banrealtime`, `templex`, `templey`, `templez`, `skulltype`, `skullkills`, ";
    query << "`skullticks`, `skullabsolve`, `rank_id`, `guildnick` FROM `players` WHERE `name` ";
    query << db->getStringComparison() << db->escapeString(name) << " AND `deleted` = 0 LIMIT 1";

    DBResult* result;
    if(!(result = db->storeQuery(query.str())))
        return false;

    uint32_t accno = result->getDataInt("account_id");
    if(accno < 1)
        return false;

    // Getting all player properties
    player->setGUID(result->getDataInt("id"));

    nameCacheMap[player->getGUID()] = name;
    guidCacheMap[name] = player->getGUID();

    player->accountNumber = accno;
    player->sex = ((playersex_t)result->getDataInt("sex"));
    player->setDirection((Direction)result->getDataInt("lookdir"));
    player->level = result->getDataInt("level");
    exp_t experience = (exp_t)result->getDataLong("exp");
    exp_t currExpCount = player->getExpForLv(player->level);
    exp_t nextExpCount = player->getExpForNextLevel();
    if(player->experience < currExpCount || experience > nextExpCount)
        experience = currExpCount;

    player->experience = experience;
    if(currExpCount < nextExpCount)
        player->level_percent  = (unsigned char)(100*(player->experience-player->getExpForLv(player->level))/(1.*player->getExpForLv(player->level+1)-player->getExpForLv(player->level)));
    else
        player->level_percent = 0;


    player->maglevel = result->getDataInt("maglevel");
    player->vocation =((playervoc_t)result->getDataInt("vocation"));
    player->access = result->getDataInt("access");
    player->frags = result->getDataInt("frags");
    player->namelock = result->getDataInt("namelock");
    player->premiumTicks = result->getDataInt("premticks");
    player->promoted = result->getDataInt("promoted");
    player->capacity = result->getDataInt("cap");
    player->punkty = result->getDataInt("punkty");
    player->banned = result->getDataInt("banned");
    player->max_depot_items = result->getDataInt("maxdepotitems");
    player->lastLoginSaved = result->getDataLong("lastlogin");
    player->mana = result->getDataInt("mana");
    player->manamax = result->getDataInt("manamax");
    uint64_t manaSpent = result->getDataLong("manaspent");
    uint64_t nextManaCount = player->getReqMana(player->maglevel + 1, player->vocation);
    if(manaSpent > nextManaCount)
        manaSpent = 0;

    player->manaspent = manaSpent;

    player->maglevel_percent  = (unsigned char)(100*(player->manaspent/(1.*player->getReqMana(player->maglevel+1, player->vocation))));
    player->health = result->getDataInt("health");
    if(player->health <= 0)
        player->health = 100;

    player->healthmax = result->getDataInt("healthmax");
    if(player->healthmax <= 0)
        player->healthmax = 100;

    player->food = result->getDataInt("food");
    player->looktype = result->getDataInt("looktype");
    player->lookhead = result->getDataInt("lookhead");
    player->lookbody = result->getDataInt("lookbody");
    player->looklegs = result->getDataInt("looklegs");
    player->lookfeet = result->getDataInt("lookfeet");
    player->lookmaster = player->looktype;

    player->pos.x = atoi(result->getDataString("posx").c_str());
    player->pos.y = atoi(result->getDataString("posy").c_str());
    player->pos.z = atoi(result->getDataString("posz").c_str());
#ifdef HUCZU_BAN_SYSTEM
    player->banned = result->getDataInt("banned");
    player->banstart = result->getDataInt("banstart");
    player->banend = result->getDataInt("banend");
    player->comment = result->getDataString("comment");
    player->reason = result->getDataString("reason");
    player->action = result->getDataString("action");
    player->deleted = result->getDataInt("deleted");
    player->finalwarning = result->getDataInt("finalwarning");
    player->banrealtime = result->getDataString("banrealtime");
#endif //HUCZU_BAN_SYSTEM
    player->masterPos.x = result->getDataInt("templex");
    player->masterPos.y = result->getDataInt("templey");
    player->masterPos.z = result->getDataInt("templez");
#ifdef HUCZU_SKULLS
    player->skullType = ((skull_t)result->getDataInt("skulltype"));
    player->skullKills = result->getDataInt("skullkills");
    player->skullTicks = result->getDataInt("skullticks");
    player->absolveTicks = result->getDataInt("skullabsolve");
#endif //HUCZU_SKULLS

    const uint32_t rankId = result->getDataInt("rank_id");
    const std::string nick = result->getDataString("guildnick");

    result->free();
    if(rankId > 0)
    {
        query.str("");
        query << "SELECT `guild_ranks`.`name` AS `rank`, `guild_ranks`.`guild_id` AS `guildid`, `guild_ranks`.`level` AS `level`, `guilds`.`name` AS `guildname` FROM `guild_ranks`, `guilds` WHERE `guild_ranks`.`id` = " << rankId << " AND `guild_ranks`.`guild_id` = `guilds`.`id` LIMIT 1";
        if((result = db->storeQuery(query.str())))
        {
            player->guildName = result->getDataString("guildname");
            player->guildStatus = (GuildLevel_t)result->getDataInt("level");
            player->guildId = result->getDataInt("guildid");
            player->rankName = result->getDataString("rank");
            player->rankId = rankId;
            player->guildNick = nick;

            result->free();
        }
    }
    //gildie w grze
    query.str("");
    query << "SELECT `guild_id` FROM `guild_invites` WHERE `player_id` = " << player->getGUID();
    if((result = db->storeQuery(query.str())))
    {
        do
            player->invitedToGuildsList.push_back((uint32_t)result->getDataInt("guild_id"));
        while(result->next());
        result->free();
    }

    query.str("");
    query << "SELECT `password` FROM `accounts` WHERE `id` = " << accno << " LIMIT 1";
    if(!(result = db->storeQuery(query.str())))
        return false;

    player->password = result->getDataString("password");
    result->free();

    // so we query the skill table
    query.str("");
    query << "SELECT `skillid`, `value`, `count` FROM `player_skills` WHERE `player_id` = " << player->getGUID();
    if((result = db->storeQuery(query.str())))
    {
        //now iterate over the skills
        do
        {
            uint16_t skillId = result->getDataInt("skillid");
            if(skillId < SKILL_FIST || skillId > SKILL_FISH)
                continue;

            uint32_t skillLevel = result->getDataInt("value");
            uint64_t nextSkillCount = player->getReqSkillTries(skillId, skillLevel + 1, player->vocation), skillCount = result->getDataLong("count");
            if(skillCount > nextSkillCount)
                skillCount = 0;

            player->skills[skillId][SKILL_LEVEL] = skillLevel;
            player->skills[skillId][SKILL_TRIES] = skillCount;
            player->skills[skillId][SKILL_PERCENT] = (uint32_t)(100*(player->skills[skillId][SKILL_TRIES])/(1.*player->getReqSkillTries (skillId, (player->skills[skillId][SKILL_LEVEL]+1), player->vocation)));
        }
        while(result->next());
        result->free();
    }

    query.str("");
    ItemMap itemMap;
    ItemMap::iterator it;
    query << "SELECT * FROM `player_items` WHERE `player_id` = " << player->getGUID();
    if((result = db->storeQuery(query.str())))
    {
        do
        {
            int32_t type = result->getDataInt("id");
            int32_t count = result->getDataInt("count");
            Item* myItem = Item::CreateItem(type, count);
            if(result->getDataInt("actionid") != 0)
                myItem->setActionId(result->getDataInt("actionid"));
            if(result->getDataInt("charges") != 0)
                myItem->setCharges(result->getDataInt("charges"));
            if(result->getDataInt("time") != 0)
                myItem->setItemTime(result->getDataInt("time"));
            if(result->getDataInt("uniqueid") != 0)
                myItem->setUniqueId(result->getDataInt("uniqueid"));
            if(result->getDataString("text") != "")
                myItem->setText(result->getDataString("text"));
            if(result->getDataString("writer") != "")
                myItem->setWriter(result->getDataString("writer"));
            if(result->getDataString("special_description") != "")
                myItem->setSpecialDescription(result->getDataString("special_description"));
            std::pair<Item*, int32_t> myPair(myItem, result->getDataInt("pid"));
            itemMap[result->getDataInt("sid")] = myPair;
            if(int32_t slotid = result->getDataInt("slot"))
                if(slotid > 0 && slotid <= 10)
                    player->addItemInventory(myItem, slotid);
        }
        while(result->next());

        result->free();

    }
    for(int32_t i = (int32_t)itemMap.size(); i > 0; --i)
    {
        it = itemMap.find(i);
        if(it == itemMap.end())
            continue;

        if(int32_t p=(*it).second.second)
            if(Container* c = itemMap[p].first->getContainer())
                c->addItem((*it).second.first);
    }
    itemMap.clear();

    query.str("");
    query << "SELECT * FROM `player_depotitems` WHERE `player_id` = " << player->getGUID();
    if((result = db->storeQuery(query.str())))
    {
        do
        {
            int32_t type = result->getDataInt("id");
            int32_t count = result->getDataInt("count");
            Item* myItem = Item::CreateItem(type, count);
            if(result->getDataInt("actionid") != 0)
                myItem->setActionId(result->getDataInt("actionid"));
            if(result->getDataInt("charges") != 0)
                myItem->setCharges(result->getDataInt("charges"));
            if(result->getDataInt("time") != 0)
                myItem->setItemTime(result->getDataInt("time"));
            if(result->getDataInt("uniqueid") != 0)
                myItem->setUniqueId(result->getDataInt("uniqueid"));
            if(result->getDataString("text") != "")
                myItem->setText(result->getDataString("text"));
            if(result->getDataString("writer") != "")
                myItem->setWriter(result->getDataString("writer"));
            if(result->getDataString("special_description") != "")
                myItem->setSpecialDescription(result->getDataString("special_description"));
            std::pair<Item*, int32_t> myPair(myItem, result->getDataInt("pid"));
            itemMap[result->getDataInt("sid")] = myPair;
            if(int32_t slotid = result->getDataInt("slot"))
            {
                if(Container* container = myItem->getContainer())
                {
                    if(slotid > 0)
                        player->addDepot(container, slotid);
                }
                else
                {
                    std::cout << "2. Error loading depot "<< slotid << " for player " <<
                              player->getGUID() << std::endl;
                    delete myItem;
                }
            }
        }
        while(result->next());

        result->free();

    }
    for(int32_t i = (int32_t)itemMap.size(); i > 0; --i)
    {
        it = itemMap.find(i);
        if(it == itemMap.end())
            continue;

        if(int32_t p=(*it).second.second)
            if(Container* c = itemMap[p].first->getContainer())
                c->addItem((*it).second.first);
    }
    itemMap.clear();

    player->updateInventoryWeigth();
    player->sendPlayerLightLevel(player);
    player->setNormalSpeed();

    //load storage map
    query.str("");
    query << "SELECT `key`,`value` FROM `player_storage` WHERE `player_id` = " << player->getGUID();
    if((result = db->storeQuery(query.str())))
    {
        do
            player->addStorageValue((uint32_t)result->getDataInt("key"), result->getDataInt("value"));
        while(result->next());
        result->free();
    }

    //load vip
    query.str("");
    query << "SELECT `vip_id` AS `vip` FROM `player_viplist` WHERE `player_id` = " << player->getGUID();
    if((result = db->storeQuery(query.str())))
    {
        std::string dummy;
        do
        {
            uint32_t vid = result->getDataInt("vip");
            if(storeNameByGuid(vid))
                player->addVIP(vid, dummy, false, true);
        }
        while(result->next());
        result->free();
    }

    query.str("");
    query << "SELECT * FROM `deathlist` WHERE `player_id` = " << player->getGUID();
    if((result = db->storeQuery(query.str())))
    {
        do
        {
            Death death;
            death.killer = result->getDataString("killer");
            death.level  = result->getDataInt("level");
            death.time   = (time_t)result->getDataLong("time");
            player->deathList.push_back(death);
        }
        while(result->next());
    }


    return true;
}

bool IOPlayerSQL::savePlayer(Player* player)
{
    player->preSave();

    Database* db = Database::getInstance();
    DBQuery query;
    query << "SELECT `save` FROM `players` WHERE `id` = " << player->getGUID() << " LIMIT 1";

    DBResult* result;
    if(!(result = db->storeQuery(query.str())))
        return false;

    const bool save = result->getDataInt("save");
    result->free();

    DBTransaction trans(db);
    if(!trans.begin())
        return false;

    query.str("");
    query << "UPDATE `players` SET `lastlogin` = " << player->lastlogin;
    if(!save)
    {
        query << " WHERE `id` = " << player->getGUID() << db->getUpdateLimiter();
        if(!db->executeQuery(query.str()))
            return false;

        return trans.commit();
    }

    //First, an UPDATE query to write the player itself
    //query << "UPDATE `players` SET ";
    query << ", ";
    query << "`level` = " << player->level << ", ";
    query << "`vocation` = " << (int32_t)player->getVocation() << ", ";
    query << "`maglevel` = " << player->maglevel << ", ";
    query << "`health` = " << player->health << ", ";
    query << "`healthmax` = " << player->healthmax << ", ";
    query << "`food` = " << player->food << ", ";
    query << "`lookdir` = " << (int32_t)player->getDirection() << ", ";
    query << "`exp` = " << player->experience << ", ";
    query << "`lookbody` = " << (int32_t)player->lookbody << ", ";
    query << "`lookfeet` = " << (int32_t)player->lookfeet << ", ";
    query << "`lookhead` = " << (int32_t)player->lookhead << ", ";
    query << "`looklegs` = " << (int32_t)player->looklegs << ", ";
    query << "`looktype` = " << (int32_t)player->looktype << ", ";
    query << "`mana` = " << player->mana << ", ";
    query << "`manamax` = " << player->manamax << ", ";
    query << "`manaspent` = " << player->manaspent << ", ";
    query << "`posx` = " << player->pos.x << ", ";
    query << "`posy` = " << player->pos.y << ", ";
    query << "`posz` = " << player->pos.z << ", ";
    query << "`cap` = " << player->getCapacity() << ", ";
    query << "`punkty` = " << player->punkty << ", ";
    query << "`sex` = " << player->getSex() << ", ";
    query << "`namelock` = " << player->namelock << ", ";
#ifdef YUR_PREMIUM_PROMOTION
    query << "`premticks` = " << player->premiumTicks << ", ";
    query << "`promoted` = " << player->promoted << ", ";
#endif //YUR_PREMIUM_PROMOTION
#ifdef HUCZU_SKULLS
    query << "`skulltype` = " << player->skullType << ", ";
    query << "`skullkills` = " << player->skullKills << ", ";
    query << "`skullticks` = " << player->skullTicks << ", ";
    query << "`skullabsolve` = " << player->absolveTicks << ", ";
#endif //HUCZU_SKULLS
#ifdef HUCZU_BAN_SYSTEM
    query << "`banned` = " << player->banned << ", ";
    query << "`banstart` = " << player->banstart << ", ";
    query << "`banend` = " << player->banend << ", ";
    query << "`comment` = " << db->escapeString(player->comment) <<  ", ";
    query << "`reason` = " << db->escapeString(player->reason) <<  ", ";
    query << "`action` = " << db->escapeString(player->action) <<  ", ";
    query << "`deleted` = " << player->deleted <<  ", ";
    query << "`finalwarning` = " << player->finalwarning <<  ", ";
    query << "`banrealtime` = " << db->escapeString(player->banrealtime) <<  ", ";
#endif //HUCZU_BAN_SYSTEM
    //query << "`lastip` = " << player->lastip << ", ";
    query << "`access` = " << player->access << ", ";
    query << "`frags` = " << player->frags << ", ";
    query << "`maxdepotitems` = " << player->max_depot_items << ", ";
    query << "`templex` = " << player->masterPos.x << ", ";
    query << "`templey` = " << player->masterPos.y << ", ";
    query << "`templez` = " << player->masterPos.z << ", ";
    query << "`guildnick` = " << db->escapeString(player->guildNick) << ", ";
    query << "`rank_id` = " << Guild::getInstance()->getRankIdByLevel(player->getGuildId(), player->guildStatus) /*<< ", "*/;

    query << " WHERE `id` = "<< player->getGUID();


    if(!db->executeQuery(query.str()))
        return false;

    // skills
    for(int32_t i = SKILL_FIST; i <= SKILL_FISH; ++i)
    {
        query.str("");
        query << "UPDATE `player_skills` SET `value` = " << player->skills[i][SKILL_LEVEL] << ", `count` = " << player->skills[i][SKILL_TRIES] << " WHERE `player_id` = " << player->getGUID() << " AND `skillid` = " << i << db->getUpdateLimiter();
        if(!db->executeQuery(query.str()))
            return false;
    }

    //item saving
    query.str("");
    query << "DELETE FROM `player_items` WHERE `player_id` = " << player->getGUID();
    if(!db->executeQuery(query.str()))
        return false;

    //now item saving
    DBInsert query_insert(db);
    char buffer[280];
    int32_t runningID = 0, parentid = 0;
    Item* item = NULL;
    Container* container = NULL;
    Container* topcontainer = NULL;
    std::stringstream streamitems;

    query_insert.setQuery("INSERT INTO `player_items` (`player_id` , `slot` , `sid` , `pid` , `id` , `count` , `actionid` , `charges`, `time`, `uniqueid`, `text`, `writer`, `special_description`) VALUES");

    ContainerList::const_iterator it;
    for (int32_t slotid = 1; slotid <= 10; ++slotid)
    {
        if(!player->items[slotid])
            continue;

        item = player->items[slotid];
        ++runningID;

        streamitems << player->getGUID() <<"," << slotid << ","<< runningID <<","<< parentid <<"," << item->getID()<<","<< (int32_t)item->getItemCountOrSubtype() << "," <<
                    (int32_t)item->getActionId()<< "," << item->getCharges() << "," << item->getTime() << "," << item->getUniqueId() << "," << db->escapeString(item->getText()) << "," << db->escapeString(item->getWriter()) << "," << db->escapeString(item->getSpecialDescription());

        topcontainer = dynamic_cast<Container*>(item);
        if(topcontainer)
        {
            stack.push_back(containerStackPair(topcontainer, runningID));
        }

        if(!query_insert.addRow(streamitems.str().c_str()))
            return false;
        streamitems.str("");
    }
    while(stack.size() > 0)
    {

        containerStackPair csPair = stack.front();
        container = csPair.first;
        parentid = csPair.second;
        stack.pop_front();

        for (it = container->getItems(); it != container->getEnd(); ++it)
        {
            ++runningID;
            Container *container = dynamic_cast<Container*>(*it);
            if(container)
            {
                stack.push_back(containerStackPair(container, runningID));
            }

            streamitems << player->getGUID() <<"," << 0 << ","<< runningID <<","<< parentid <<"," << (*it)->getID()<<","<< (int32_t)(*it)->getItemCountOrSubtype() << "," <<
                        (int32_t)(*it)->getActionId() << "," << (*it)->getCharges() << "," << (*it)->getTime() << "," << (*it)->getUniqueId() << "," << db->escapeString((*it)->getText()) << "," << db->escapeString((*it)->getWriter()) << "," << db->escapeString((*it)->getSpecialDescription());
            if(!query_insert.addRow(streamitems.str().c_str()))
                return false;
            streamitems.str("");
        }

    }
    if(!query_insert.execute())
        return false;

    //czyszczenie
    query.str("");
    runningID = 0;
    parentid = 0;
    item = NULL;
    container = NULL;
    topcontainer = NULL;
    stack.clear();

    query << "DELETE FROM `player_depotitems` WHERE `player_id` = " << player->getGUID();
    if(!db->executeQuery(query.str()))
        return false;


    //save depot items
    query_insert.setQuery("INSERT INTO `player_depotitems` (`player_id` , `slot` , `sid` , `pid` , `id` , `count` , `actionid` , `charges`, `time`, `uniqueid` , `text`, `writer`, `special_description` ) VALUES");
    for(DepotMap::reverse_iterator dit = player->depots.rbegin(); dit !=player->depots.rend() ; ++dit)
    {
        item = dit->second;
        ++runningID;

        streamitems << player->getGUID() <<"," << dit->first << ","<< runningID <<","<< parentid <<"," << item->getID()<<"," << (int32_t)item->getItemCountOrSubtype() << "," <<
                    (int32_t)item->getActionId() << "," << item->getCharges() << "," << item->getTime() << "," << item->getUniqueId() << "," << db->escapeString(item->getText()) << "," << db->escapeString(item->getWriter()) << "," << db->escapeString(item->getSpecialDescription());

        topcontainer = dynamic_cast<Container*>(item);
        if(topcontainer)
        {
            stack.push_back(containerStackPair(topcontainer, runningID));
        }
        if(!query_insert.addRow(streamitems.str().c_str()))
            return false;
        streamitems.str("");
    }

    while(stack.size() > 0)
    {
        containerStackPair csPair_depot = stack.front();
        container = csPair_depot.first;
        parentid = csPair_depot.second;
        stack.pop_front();

        for (it = container->getItems(); it != container->getEnd(); ++it)
        {
            ++runningID;
            Container *container = dynamic_cast<Container*>(*it);
            if(container)
            {
                stack.push_back(containerStackPair(container, runningID));
            }

            streamitems << player->getGUID() << "," << 0 << "," << runningID << "," << parentid << "," << (*it)->getID()<< "," << (int32_t)(*it)->getItemCountOrSubtype() << "," <<
                        (int32_t)(*it)->getActionId() << "," << (*it)->getCharges() << "," << (*it)->getTime() << "," << (*it)->getUniqueId() << "," <<  db->escapeString((*it)->getText()) << "," << db->escapeString((*it)->getWriter()) << "," << db->escapeString((*it)->getSpecialDescription());
            if(!query_insert.addRow(streamitems.str().c_str()))
                return false;
            streamitems.str("");
        }
    }
    if(!query_insert.execute())
        return false;

    query.str("");
    query << "DELETE FROM `player_storage` WHERE `player_id` = " << player->getGUID();
    if(!db->executeQuery(query.str()))
        return false;

    query_insert.setQuery("INSERT INTO `player_storage` (`player_id`, `key`, `value`) VALUES ");
    for(StorageMap::const_iterator cit = player->getStorageIteratorBegin(); cit != player->getStorageIteratorEnd(); ++cit)
    {
        sprintf(buffer, "%u, %u, %d", player->getGUID(), cit->first, cit->second);
        if(!query_insert.addRow(buffer))
            return false;
    }

    if(!query_insert.execute())
        return false;

    //save guild invites
    query.str("");
    query << "DELETE FROM `guild_invites` WHERE player_id = " << player->getGUID();
    if(!db->executeQuery(query.str()))
        return false;

    query_insert.setQuery("INSERT INTO `guild_invites` (`player_id`, `guild_id`) VALUES ");
    for(InvitedToGuildsList::const_iterator it = player->invitedToGuildsList.begin(); it != player->invitedToGuildsList.end(); ++it)
    {
        sprintf(buffer, "%d, %d", player->getGUID(), *it);
        if(!query_insert.addRow(buffer))
            return false;
    }

    if(!query_insert.execute())
        return false;

    query.str("");
    query << "DELETE FROM `deathlist` WHERE player_id='" << player->getGUID() << "'";

    if(!db->executeQuery(query.str()))
        return false;

    query_insert.setQuery("INSERT INTO `deathlist` (`player_id`, `killer`, `level`, `time`) VALUES ");
    for(std::list<Death>::iterator it = player->deathList.begin(); it != player->deathList.end(); it++)
    {
        std::stringstream death_list;
        death_list << player->getGUID() << ", " << db->escapeString((*it).killer) << ", " << (*it).level << ", " << (*it).time;
        if(!query_insert.addRow(death_list.str().c_str()))
            return false;
    }

    if(!query_insert.execute())
        return false;

    //save vip list- FIXME: merge it to one config query?
    query.str("");
    query << "DELETE FROM `player_viplist` WHERE `player_id` = " << player->getGUID();
    if(!db->executeQuery(query.str()))
        return false;

    query_insert.setQuery("INSERT INTO `player_viplist` (`player_id`, `vip_id`) VALUES ");
    for(VIPListSet::iterator it = player->VIPList.begin(); it != player->VIPList.end(); it++)
    {
        if(!playerExists(*it, false))
            continue;

        sprintf(buffer, "%d, %d", player->getGUID(), *it);

        if(!query_insert.addRow(buffer))
            return false;
    }

    if(!query_insert.execute())
        return false;

    //End the transaction
    return trans.commit();
}

bool IOPlayerSQL::storeNameByGuid(uint32_t guid)
{
    NameCacheMap::iterator it = nameCacheMap.find(guid);
    if(it != nameCacheMap.end())
        return true;

    Database* db = Database::getInstance();
    DBQuery query;
    query << "SELECT `name` FROM `players` WHERE `id` = " << guid << " AND `deleted` = 0 LIMIT 1";

    DBResult* result;
    if(!(result = db->storeQuery(query.str())))
        return false;

    nameCacheMap[guid] = result->getDataString("name");
    result->free();
    return true;
}

bool IOPlayerSQL::getNameByGuid(uint32_t guid, std::string& name)
{
    NameCacheMap::iterator it = nameCacheMap.find(guid);
    if(it != nameCacheMap.end())
    {
        name = it->second;
        return true;
    }

    Database* db = Database::getInstance();
    DBQuery query;

    query << "SELECT `name` FROM `players` WHERE `id` = " << guid << " AND `deleted` = 0 LIMIT 1";

    DBResult* result;
    if(!(result = db->storeQuery(query.str())))
        return false;

    name = result->getDataString("name");
    result->free();

    nameCacheMap[guid] = name;
    return true;
}

bool IOPlayerSQL::getGuidByName(uint32_t &guid, std::string& name)
{
    GuidCacheMap::iterator it = guidCacheMap.find(name);
    if(it != guidCacheMap.end())
    {
        name = it->first;
        guid = it->second;
        return true;
    }

    Database* db = Database::getInstance();
    DBQuery query;
    query << "SELECT `name`, `id` FROM `players` WHERE `name` " << db->getStringComparison() << db->escapeString(name) << " AND `deleted` = 0 LIMIT 1";

    DBResult* result;
    if(!(result = db->storeQuery(query.str())))
        return false;

    name = result->getDataString("name");
    guid = result->getDataInt("id");
    guidCacheMap[name] = guid;
    result->free();
    return true;
}

bool IOPlayerSQL::playerExists(uint32_t guid, bool checkCache /*= true*/)
{
    if(checkCache)
    {
        NameCacheMap::iterator it = nameCacheMap.find(guid);
        if(it != nameCacheMap.end())
            return true;
    }

    Database* db = Database::getInstance();
    DBQuery query;

    query << "SELECT `name` FROM `players` WHERE `id` = " << guid << " AND `deleted` = 0 LIMIT 1";

    DBResult* result;
    if(!(result = db->storeQuery(query.str())))
        return false;

    const std::string name = result->getDataString("name");
    result->free();

    nameCacheMap[guid] = name;
    return true;
}

bool IOPlayerSQL::playerExists(std::string& name, bool checkCache /*= true*/)
{
    if(checkCache)
    {
        GuidCacheMap::iterator it = guidCacheMap.find(name);
        if(it != guidCacheMap.end())
        {
            name = it->first;
            return true;
        }
    }

    Database* db = Database::getInstance();
    DBQuery query;

    query << "SELECT `id`, `name` FROM `players` WHERE `name` " << db->getStringComparison() << db->escapeString(name) << " AND `deleted` = 0";

    query << " LIMIT 1";
    DBResult* result;
    if(!(result = db->storeQuery(query.str())))
        return false;

    name = result->getDataString("name");
    guidCacheMap[name] = result->getDataInt("id");

    result->free();
    return true;
}

uint32_t IOPlayerSQL::getLevel(uint32_t guid) const
{
    Database* db = Database::getInstance();
    DBQuery query;
    query << "SELECT `level` FROM `players` WHERE `id` = " << guid << " AND `deleted` = 0 LIMIT 1";

    DBResult* result;
    if(!(result = db->storeQuery(query.str())))
        return 0;

    uint32_t level = result->getDataInt("level");
    result->free();
    return level;
}

bool IOPlayerSQL::resetGuildInformation(uint32_t guid)
{
    Database* db = Database::getInstance();
    DBQuery query;
    query << "UPDATE `players` SET `rank_id` = 0, `guildnick` = '' WHERE `id` = " << guid << " AND `deleted` = 0" << db->getUpdateLimiter();
    return db->executeQuery(query.str());
}

bool IOPlayerSQL::updateOnlineStatus(uint32_t guid, bool login)
{
    Database* db = Database::getInstance();
    DBQuery query;

    uint16_t value = login;

    query << "UPDATE `players` SET `online` = " << value << " WHERE `id` = " << guid << db->getUpdateLimiter();
    return db->executeQuery(query.str());
}
