#include "ioaccountsql.h"
#include <iostream>
#include <iomanip>

#include "tools.h"
#include "database.h"

Account IOAccountSQL::loadAccount(uint32_t accno)
{
    Account account;
    Database* db = Database::getInstance();
    DBQuery query;

    query << "SELECT `id`, `password`, `premdays`, `rkey`, `email` FROM `accounts` WHERE `id` = " << accno << " LIMIT 1";
    DBResult* result;
    if(!(result = db->storeQuery(query.str())))
        return account;

    account.accnumber = result->getDataInt("id");
    account.password = result->getDataString("password");
    account.premDays = result->getDataInt("premdays");
    account.key = result->getDataString("rkey");
    account.email = result->getDataString("email");

    query.str("");
    result->free();

    query << "SELECT `name` FROM `players` WHERE `account_id` = " << accno << " AND `deleted` = 0";
    if(!(result = db->storeQuery(query.str())))
        return account;

    do
    {
        std::string ss = result->getDataString("name");
        account.charList.push_back(ss.c_str());
    }
    while(result->next());

    result->free();
    account.charList.sort();

    return account;
}

bool IOAccountSQL::saveAccount(Account account)
{
    Database* db = Database::getInstance();
    DBQuery query;

    query << "UPDATE `accounts` SET `premdays` = " << account.premDays << ", `email` = " << account.email << ", `rkey` = " << account.key << " WHERE `id` = " << account.accnumber << db->getUpdateLimiter();

    return db->executeQuery(query.str());
}

bool IOAccountSQL::getPassword(uint32_t accno, const std::string &name, std::string &password)
{
    Database* db = Database::getInstance();
    DBQuery query;
    query << "SELECT `password` FROM `accounts` WHERE `id` = " << accno << " LIMIT 1";

    DBResult* result;
    if(!(result = db->storeQuery(query.str())))
        return false;

    if(name.empty())
    {
        password = result->getDataString("password");
        result->free();
        return true;
    }

    std::string tmpPassword = result->getDataString("password");
    result->free();
    query.str("");

    query << "SELECT `name` FROM `players` WHERE `account_id` = " << accno;
    if(!(result = db->storeQuery(query.str())))
        return false;

    do
    {
        if(result->getDataString("name") != name)
            continue;

        password = tmpPassword;
        result->free();
        return true;
    }
    while(result->next());

    result->free();
    return false;
}

bool IOAccountSQL::accountExists(uint32_t accno)
{
    Database* db = Database::getInstance();
    DBQuery query;
    query << "SELECT `id` FROM `accounts` WHERE `id` = " << accno << " LIMIT 1";

    DBResult* result;
    if(!(result = db->storeQuery(query.str())))
        return false;

    result->free();
    return true;
}
