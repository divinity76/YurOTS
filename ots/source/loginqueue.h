
#ifdef YUR_LOGIN_QUEUE

#ifndef LOGINQUEUE_H
#define LOGINQUEUE_H
#include <time.h>
#include <list>
#include <iterator>
#include <string>

struct LoginTry;
typedef std::list<LoginTry> LoginTryList;
typedef LoginTryList::iterator LoginTryListIterator;

enum qstate_t
{
    LOGGED = 0,
    ACTIVE = 1,
    DEAD = 2
};

struct LoginTry
{
    int32_t accountNumber;
    time_t tryTime;		///< time of last login try
    qstate_t state;
    LoginTry(int32_t acc, qstate_t stat = ACTIVE): accountNumber(acc), tryTime(time(0)), state(stat) {}
};

class LoginQueue
{
private:
    static const int32_t LOGGED_TIMEOUT = 30, ACTIVE_TIMEOUT = 60, DEAD_TIMEOUT = 15*60;
    LoginTryList lq;
    LoginTryListIterator findAccount(int32_t account, int32_t* realPos, int32_t* effectivePos);
    void push(int32_t account);
    void removeDeadEntries();

public:
    LoginQueue() {}
    bool load();
    bool save();
    bool login(int32_t account, int32_t playersOnline, int32_t maxPlayers, int32_t* placeInQueue);
    size_t size() const
    {
        return lq.size();
    }
    void show();
};

#endif //LOGINQUEUE_H
#endif //YUR_LOGIN_QUEUE
