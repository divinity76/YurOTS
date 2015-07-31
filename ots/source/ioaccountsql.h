#ifndef __IOACCOUNTSQL_H
#define __IOACCOUNTSQL_H

#include <string>
#include "account.h"

/** Player-Loaders implemented with SQL */
class IOAccountSQL
{
public:
    virtual ~IOAccountSQL() {}
    static IOAccountSQL* getInstance()
    {
        static IOAccountSQL instance;
        return &instance;
    }
    virtual Account loadAccount(uint32_t accno);
    virtual bool getPassword(uint32_t accno, const std::string& name, std::string& password);

    virtual bool accountExists(uint32_t accno);
    virtual bool saveAccount(Account account);

protected:
    IOAccountSQL() {}
};

#endif
