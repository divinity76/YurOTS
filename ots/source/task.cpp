#ifdef __MIZIAK_TASKS__
#include "task.h"
#include "luascript.h"
#include "tools.h"
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

extern LuaScript g_config;
Tasks::TasksMap Tasks::task;

int32_t Tasks::getTaskStorage(std::string name)
{
    toLowerCaseString(name);
    TasksMap::iterator iter = task.find(name);

    if (iter != task.end())
        return iter->second.storage;
    else
        return -1;
}

int32_t Tasks::getTaskCount(std::string name)
{
    toLowerCaseString(name);
    TasksMap::iterator iter = task.find(name);

    if (iter != task.end())
        return iter->second.count;
    else
        return -1;
}

bool Tasks::isTaskMonster(std::string name)
{
    toLowerCaseString(name);
    TasksMap::iterator iter = task.find(name);

    if (iter != task.end())
        return true;
    else
        return false;
}

bool Tasks::Load()
{
    std::string file = g_config.DATA_DIR + "tasks.xml";
    xmlDocPtr doc;

    doc = xmlParseFile(file.c_str());
    if (!doc)
        return false;

    xmlNodePtr root, taskNode;
    root = xmlDocGetRootElement(doc);

    if (xmlStrcmp(root->name, (const xmlChar*)"tasks"))
    {
        xmlFreeDoc(doc);
        return false;
    }

    taskNode = root->children;
    while (taskNode)
    {
        if (strcmp((char*) taskNode->name, "task") == 0)
        {
            std::string name = (const char*)xmlGetProp(taskNode, (const xmlChar *) "name");
            int32_t c = atoi((const char*)xmlGetProp(taskNode, (const xmlChar *) "count"));
            int32_t s = atoi((const char*)xmlGetProp(taskNode, (const xmlChar *) "storage"));
            toLowerCaseString(name);
            Taskarr tab;
            tab.storage = s;
            tab.count = c;
            task[name] = tab;
        }
        taskNode = taskNode->next;
    }

    xmlFreeDoc(doc);
    return true;
}
#endif //__MIZIAK_TASKS__
