
#ifndef __ACCOUNT_H__
#define __ACCOUNT_H__

#include <list>
#include <string>

//class Player;


class Account
{
    //superflous
    friend class IOAccountSQL;
public:
    Account()
    {
        accnumber = 0;
        premDays = 0;
        key = "";
        email = "";
    };
    ~Account()
    {
        charList.clear();
    };

    uint32_t accnumber;
    int32_t premDays;    // Premium days

    std::string key;
    std::string email;
    std::string password;

    std::list<std::string> charList;

};

#endif  // #ifndef __ACCOUNT_H__
