#ifdef __MIZIAK_TASKS__

#ifndef TASKS_H
#define TASKS_H
#include <string>
#include <map>

class Tasks
{
private:
    typedef struct
    {
        int32_t storage;
        int32_t count;
    } Taskarr;
    typedef std::map<std::string, Taskarr> TasksMap;
    static TasksMap task;
public:
    static bool Load();
    static int32_t getTaskStorage(std::string);
    static int32_t getTaskCount(std::string);
    static bool isTaskMonster(std::string);
};

#endif //TASKS_H
#endif //__MIZIAK_TASKS__
