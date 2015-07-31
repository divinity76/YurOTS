
#ifdef YUR_LOGIN_QUEUE
//#include "preheaders.h"
#include "loginqueue.h"
#include "luascript.h"
#include "player.h"
#include <iostream>

extern LuaScript g_config;
extern xmlMutexPtr xmlmutex;

/**
 * Method looks for given account number in the login queue.
 * @return position in queue (counting from 0), or -1 if not found
 */
LoginTryListIterator LoginQueue::findAccount(int32_t account, int32_t* realPos, int32_t *effectivePos)
{
    int32_t pos = 1, dead = 0;
    LoginTryListIterator iter = lq.begin();

    while (iter != lq.end())
    {
        if (iter->state == DEAD)
            dead++;

        if (iter->accountNumber == account)
        {
            if (realPos) *realPos = pos;
            if (effectivePos) *effectivePos = pos - dead;
            return iter;
        }

        ++pos;
        ++iter;
    }

    if (realPos) *realPos = -1;
    if (effectivePos) *effectivePos = -1;
    return iter;
}

/**
 * Method puts account number at the end of queue, or refreshes it's state if it already exists.
 */
void LoginQueue::push(int32_t account)
{
    LoginTryListIterator iter = findAccount(account, NULL, NULL);	// look for this account number in queue

    if (iter != lq.end())
    {
        iter->tryTime = time(0);	// if it exists, refresh try time and state
        iter->state = ACTIVE;
    }
    else
        lq.push_back(LoginTry(account));	// if it doesn't exist, place it at the end of queue

}

/**
 * Method freezes inactive entries and cleans logged/dead ones.
 */
void LoginQueue::removeDeadEntries()
{
    time_t currentTime = time(0), queuedTime;
    LoginTryListIterator iter = lq.begin();

    while (iter != lq.end())
    {
        queuedTime = currentTime - iter->tryTime;

        if (iter->state == ACTIVE)
        {
            if (queuedTime > ACTIVE_TIMEOUT)
                iter->state = DEAD;
        }
        else if ((iter->state == LOGGED && queuedTime > LOGGED_TIMEOUT) ||
                 (iter->state == DEAD && queuedTime > DEAD_TIMEOUT))
        {
            iter = lq.erase(iter);
            continue;	// this avoids ++iter
        }

        ++iter;
    }
}

bool LoginQueue::login(int32_t account, int32_t playersOnline, int32_t maxPlayers, int32_t* placeInQueue)
{
    removeDeadEntries();
    push(account);

    int32_t realPos, effectivePos;
    LoginTryListIterator iter = findAccount(account, &realPos, &effectivePos);

    if (placeInQueue)
        *placeInQueue = realPos;

    if (playersOnline + effectivePos <= maxPlayers)
    {
        iter->state = LOGGED;
        return true;
    }
    else
        return false;
}

bool LoginQueue::load()
{
    std::string file = g_config.DATA_DIR + "queue.xml";
    xmlDocPtr doc;
    xmlMutexLock(xmlmutex);

    doc = xmlParseFile(file.c_str());
    if (!doc)
        return false;

    xmlNodePtr root, entryNode;
    root = xmlDocGetRootElement(doc);
    if (xmlStrcmp(root->name, (const xmlChar*)"queue"))
    {
        xmlFreeDoc(doc);
        xmlMutexUnlock(xmlmutex);
        return false;
    }

    entryNode = root->children;
    while (entryNode)
    {
        if (strcmp((char*) entryNode->name, "entry") == 0)
        {
            int32_t account = atoi((const char*)xmlGetProp(entryNode, (const xmlChar *) "account"));
            qstate_t state = (qstate_t)atoi((const char*)xmlGetProp(entryNode, (const xmlChar *) "state"));
            lq.push_back(LoginTry(account, state));
        }
        entryNode = entryNode->next;
    }

    xmlFreeDoc(doc);
    xmlMutexUnlock(xmlmutex);
    return true;
}

bool LoginQueue::save()
{
    std::string file = g_config.DATA_DIR + "queue.xml";
    xmlDocPtr doc;
    xmlMutexLock(xmlmutex);

    xmlNodePtr root, entryNode;
    doc = xmlNewDoc((const xmlChar*)"1.0");

    doc->children = xmlNewDocNode(doc, NULL, (const xmlChar*)"queue", NULL);
    root = doc->children;
    char buf[10];
    char buffer[5];
    char bufferek[10];
    char buff[5];

    removeDeadEntries();	// clean before saving

    // first save players that are online
    for (AutoList<Player>::listiterator iter = Player::listPlayer.list.begin();
            iter != Player::listPlayer.list.end(); ++iter)
    {
        sprintf(buf,"%d",iter->second->getAccount());
        sprintf(buffer,"%d",LOGGED);
        entryNode = xmlNewNode(NULL, (const xmlChar*)"entry");
        xmlSetProp(entryNode, (const xmlChar*)"account", (const xmlChar*)buf);
        xmlSetProp(entryNode, (const xmlChar*)"state", (const xmlChar*)buffer);
        xmlAddChild(root, entryNode);
    }

    // then players waiting in queue
    for (LoginTryList::iterator iter = lq.begin(); iter != lq.end(); ++iter)
    {
        sprintf(bufferek, "%d", iter->accountNumber);
        sprintf(buff, "%d", iter->state);
        entryNode = xmlNewNode(NULL, (const xmlChar*)"entry");
        xmlSetProp(entryNode, (const xmlChar*)"account", (const xmlChar*)/*itoa(iter->accountNumber, buf, 10)*/bufferek);
        xmlSetProp(entryNode, (const xmlChar*)"state", (const xmlChar*)/*itoa(iter->state, buf, 10)*/buff);
        xmlAddChild(root, entryNode);
    }

    if (xmlSaveFile(file.c_str(), doc))
    {
        xmlFreeDoc(doc);
        xmlMutexUnlock(xmlmutex);
        return true;
    }
    else
    {
        xmlFreeDoc(doc);
        xmlMutexUnlock(xmlmutex);
        std::cout << "Failed to save " << file << std::endl;
        return false;
    }
}

void LoginQueue::show()		// for testing purposes only
{
    std::cout << " --- QUEUE --- (size: " << (int32_t)lq.size() << ")\n";
    LoginTryListIterator iter = lq.begin();
    while (iter != lq.end())
    {
        std::cout << "account: " << iter->accountNumber << ", try: " << (int32_t)iter->tryTime << ", state:";
        if (iter->state == ACTIVE) std::cout << "active\n";
        else if (iter->state == LOGGED) std::cout << "logged\n";
        else std::cout << "dead\n";
        ++iter;
    }
}
#endif //YUR_LOGIN_QUEUE
