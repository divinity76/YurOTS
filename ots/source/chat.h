
#ifndef __CHAT_H
#define __CHAT_H

#include <map>
#include <list>
#include <string>

#include "const76.h"

class Player;

class ChatChannel
{
public:
    ChatChannel(uint16_t channelId, std::string channelName);
    /*virtual*/
    ~ChatChannel() {};

    bool addUser(Player *player);
    bool removeUser(Player *player);

    bool talk(Player *fromPlayer, SpeakClasses type, const std::string &text, uint16_t channelId);
    bool sendSpecialMsg(Player *fromPlayer, SpeakClasses type, const std::string &text, uint16_t channelId, const std::string &info, bool alone = true);

    const std::string& getName()
    {
        return m_name;
    };
    const uint16_t getId()
    {
        return m_id;
    };

    virtual const std::string getOwner()
    {
        return "";
    };

protected:
    typedef std::map<int32_t, Player*> UsersMap;
    UsersMap m_users;
    std::string m_name;
    uint16_t m_id;
};

class PrivateChatChannel : public ChatChannel
{
public:
    PrivateChatChannel(uint16_t channelId, std::string channelName);
    virtual ~PrivateChatChannel() {};

    const std::string getOwner()
    {
        return m_owner;
    };
    void setOwner(std::string owner)
    {
        m_owner = owner;
    };

    bool isInvited(const Player *player);

    void invitePlayer(Player *player, Player *invitePlayer);
    void excludePlayer(Player *player, Player *excludePlayer);

    bool addInvited(Player *player);
    bool removeInvited(Player *player);

    void closeChannel();

protected:
    typedef std::map<std::string, Player*> InvitedMap;
    InvitedMap m_invites;

    std::string m_owner;
};

typedef std::list<ChatChannel*> ChannelList;

class Chat
{
public:
    Chat();
    ~Chat() {};
    ChatChannel *createChannel(Player *player, uint16_t channelId);
    bool deleteChannel(Player *player, uint16_t channelId);

    bool addUserToChannel(Player *player, uint16_t channelId);
    bool removeUserFromChannel(Player *player, uint16_t channelId);
    void removeUserFromAllChannels(Player *player);

    bool talkToChannel(Player *player, SpeakClasses type, std::string &text, uint16_t channelId);
    bool sendSpecialMsgToChannel(Player *player, SpeakClasses type, const std::string &text, uint16_t channelId, const std::string &info, bool alone = true);

    std::string getChannelName(Player *player, uint16_t channelId);
    ChannelList getChannelList(Player *player);

    ChatChannel* getChannel(Player* player, uint16_t channelId);
    PrivateChatChannel* getPrivateChannel(Player* player);

private:

    typedef std::map<uint16_t, ChatChannel*> NormalChannelMap;
    typedef std::map<uint32_t, ChatChannel*> GuildChannelMap;
    NormalChannelMap m_normalChannels;
    GuildChannelMap m_guildChannels;

    typedef std::map<uint16_t, PrivateChatChannel*> PrivateChannelMap;
    PrivateChannelMap m_privateChannels;

    ChatChannel* dummyPrivate;
};

#endif
