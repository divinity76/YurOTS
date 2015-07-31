#include "chat.h"
#include "player.h"
#include "guilds.h"
#include "ioplayersql.h"
#include <sstream>
#include "game.h"
extern Game g_game;

PrivateChatChannel::PrivateChatChannel(uint16_t channelId, std::string channelName) :
    ChatChannel(channelId, channelName)
{
    m_owner = "";
}

bool PrivateChatChannel::isInvited(const Player *player)
{
    if(!player)
        return false;

    if(player->getName() == getOwner())
        return true;

    InvitedMap::iterator it = m_invites.find(player->getName());
    if(it != m_invites.end())
        return true;

    return false;
}

bool PrivateChatChannel::addInvited(Player *player)
{
    InvitedMap::iterator it = m_invites.find(player->getName());
    if(it != m_invites.end())
        return false;

    m_invites[player->getName()] = player;

    return true;
}

bool PrivateChatChannel::removeInvited(Player *player)
{
    InvitedMap::iterator it = m_invites.find(player->getName());
    if(it == m_invites.end())
        return false;

    m_invites.erase(it);
    return true;
}

void PrivateChatChannel::invitePlayer(Player *player, Player *invitePlayer)
{
    if(player != invitePlayer && addInvited(invitePlayer))
    {
        std::string msg;
        msg = player->getName();
        msg += " invites you to ";
        msg += (player->getSex() == PLAYERSEX_FEMALE ? "her" : "his");
        msg += " private chat channel.";
        invitePlayer->sendTextMessage(MSG_INFO, msg.c_str());

        msg = invitePlayer->getName();
        msg += " has been invited.";
        player->sendTextMessage(MSG_INFO, msg.c_str());
    }
}

void PrivateChatChannel::excludePlayer(Player *player, Player *excludePlayer)
{
    if(player != excludePlayer && removeInvited(excludePlayer))
    {
        removeUser(excludePlayer);

        std::string msg;
        msg = excludePlayer->getName();
        msg += " has been excluded.";
        player->sendTextMessage(MSG_INFO, msg.c_str());

        excludePlayer->sendClosePrivate(getId());
    }
}

void PrivateChatChannel::closeChannel()
{
    ChatChannel::UsersMap::iterator cit;
    for(cit = m_users.begin(); cit != m_users.end(); ++cit)
    {
        Player* toPlayer = cit->second->getPlayer();
        if(toPlayer)
        {
            toPlayer->sendClosePrivate(getId());
        }
    }
}

ChatChannel::ChatChannel(uint16_t channelId, std::string channelName)
{
    m_id = channelId;
    m_name = channelName;
}

bool ChatChannel::addUser(Player *player)
{
    UsersMap::iterator it = m_users.find(player->getID());
    if(it != m_users.end())
        return false;

    m_users[player->getID()] = player;
    return true;
}

bool ChatChannel::removeUser(Player *player)
{
    UsersMap::iterator it = m_users.find(player->getID());
    if(it == m_users.end())
        return false;

    m_users.erase(it);
    return true;
}

bool ChatChannel::talk(Player *fromPlayer, SpeakClasses type, const std::string &text, uint16_t channelId)
{
    bool success = false;
    UsersMap::iterator it;
    std::stringstream t;

    if (fromPlayer->access <= 2)
        t << "[" << fromPlayer->getLevel() << "]: " << text;
    else
        t << text;

    for(it = m_users.begin(); it != m_users.end(); ++it)
    {
        Player *toPlayer = dynamic_cast<Player*>(it->second);
        if(toPlayer && !g_game.creatureSaySpell(fromPlayer, text))  //nie pojawia sie spelle na kanalach, magia
        {
            toPlayer->sendToChannel(fromPlayer, type, t.str(), channelId);
            success = true;
        }
    }
    return success;
}

bool ChatChannel::sendSpecialMsg(Player *fromPlayer, SpeakClasses type, const std::string &text, uint16_t channelId, const std::string &info, bool alone /*=true*/)
{
    bool success = false;
    UsersMap::iterator it;
    if(!alone)
    {
        for(it = m_users.begin(); it != m_users.end(); ++it)
        {
            Player *toPlayer = dynamic_cast<Player*>(it->second);
            if(toPlayer)
            {
                toPlayer->sendToSpecialChannel(type, text, channelId, info);
                success = true;
            }
        }
    }
    else
        fromPlayer->sendToSpecialChannel(type, text, channelId, info);

    return success;
}

Chat::Chat()
{
    // Create the default channels
    ChatChannel *newChannel;

    newChannel = new ChatChannel(0x01, "System");
    if(newChannel)
        m_normalChannels[0x01] = newChannel;
#ifdef HUCZU_SERVER_LOG
    newChannel = new ChatChannel(0x02, "Server Log");
    if(newChannel)
        m_normalChannels[0x02] = newChannel;
#endif
#ifdef HUCZU_RRV
    newChannel = new ChatChannel(0x03, "Rule Violations");
    if(newChannel)
        m_normalChannels[0x03] = newChannel;
#endif
    newChannel = new ChatChannel(0x04, "Game-Chat");
    if(newChannel)
        m_normalChannels[0x04] = newChannel;

    newChannel = new ChatChannel(0x05, "Trade");
    if(newChannel)
        m_normalChannels[0x05] = newChannel;

    newChannel = new ChatChannel(0x06, "Loot");
    if(newChannel)
        m_normalChannels[0x06] = newChannel;

    newChannel = new PrivateChatChannel(0xFFFF, "Private Chat Channel");
    if(newChannel)
        dummyPrivate = newChannel;
}

ChatChannel *Chat::createChannel(Player *player, uint16_t channelId)
{
    if(getChannel(player, channelId))
        return NULL;

    if(channelId == 0x00)
    {
        ChatChannel *newChannel = new ChatChannel(channelId, player->getGuildName());
        if(!newChannel)
            return NULL;

        m_guildChannels[player->getGuildId()] = newChannel;
        return newChannel;
    }
    else if(channelId == 0xFFFF)
    {
        for(int32_t i = 10; i < 10000; ++i)
        {
            if(!getChannel(player, i))
            {
                PrivateChatChannel* newChannel = new PrivateChatChannel(i, player->getName() + "'s Channel");
                if(!newChannel)
                    return NULL;

                newChannel->setOwner(player->getName());

                m_privateChannels[i] = newChannel;
                return newChannel;
            }
        }
    }

    return NULL;
}

bool Chat::deleteChannel(Player *player, uint16_t channelId)
{
    if(channelId == 0x00)
    {
        GuildChannelMap::iterator it = m_guildChannels.find(player->getGuildId());
        if(it == m_guildChannels.end())
            return false;

        delete it->second;
        m_guildChannels.erase(it);
        return true;
    }
    else
    {
        PrivateChannelMap::iterator it = m_privateChannels.find(channelId);
        if(it == m_privateChannels.end())
            return false;

        it->second->closeChannel();

        delete it->second;
        m_privateChannels.erase(it);
        return true;
    }

    return false;
}

bool Chat::addUserToChannel(Player *player, uint16_t channelId)
{
    ChatChannel *channel = getChannel(player, channelId);
    if(!channel || !player || (channelId == 0x01 && player->access == 0))
        return false;
#ifdef HUCZU_RRV
    if (channelId == 0x03 && player->access < 1)// violations - edit as needed
        return false;
#endif
    if(channel->addUser(player))
        return true;
    else
        return false;

}

bool Chat::removeUserFromChannel(Player *player, uint16_t channelId)
{
    ChatChannel *channel = getChannel(player, channelId);
    if(!channel)
        return false;

    if(channel->removeUser(player))
    {
        if(channel->getOwner() == player->getName())
            deleteChannel(player, channelId);
        return true;
    }
    else
        return false;
}

void Chat::removeUserFromAllChannels(Player *player)
{
    ChannelList list = getChannelList(player);
    while(!list.empty())
    {
        ChatChannel *channel = list.front();
        list.pop_front();

        channel->removeUser(player);

        if(channel->getOwner() == player->getName())
            deleteChannel(player, channel->getId());
    }
}
bool Chat::sendSpecialMsgToChannel(Player *player, SpeakClasses type, const std::string &text, uint16_t channelId, const std::string &info, bool alone/*=true*/)
{
    ChatChannel *channel = getChannel(player, channelId);
    if(!channel)
        return false;

    if(channel->sendSpecialMsg(player, type, text, channelId, info, alone))
        return true;

    return false;
}

bool Chat::talkToChannel(Player *player, SpeakClasses type, std::string &text, uint16_t channelId)
{
    ChatChannel *channel = getChannel(player, channelId);
    if(!channel)
        return false;

    std::stringstream ss;
#ifdef FIXY
    if(player && player->level < 5 && player->access < 2)
    {
        if(channel->getName() == "Trade")
        {
            player->sendCancel("Musisz zdobyc 5 poziom bys mogl pisac na Trade.");
            return false;
        }
        else if(channel->getName() == "Game-Chat")
        {
            player->sendCancel("Musisz zdobyc 5 poziom bys mogl pisac na Game-Chat.");
            return false;
        }
    }
#endif //FIXY
    if(player->access == 0)
    {
        if(channel->getName() == "Game-Chat")
        {
            if(player->gameTicks > 0)
            {
                int32_t secondsleft0 = player->gameTicks / 1000;
                ss << "Musisz poczekac " << secondsleft0 << " sekund, aby wyslac nastepna wiadomosc na Game-Chat.";
                player->sendCancel(ss.str().c_str());
                ss.str("");
                return false;
            }
            else
            {
                if(channel->talk(player, type, text, channelId))
                {
                    player->gameTicks = 6000;
                    return true;
                }
            }
        }
        else if(channel->getName() == "Trade")
        {
            if(player->tradeTicks > 0)
            {
                int32_t secondsleft1 = player->tradeTicks / 1000;
                ss << "Musisz poczekac " << secondsleft1 << " sekund, aby wyslac nastepna oferte na Trade channel.";
                player->sendCancel(ss.str().c_str());
                ss.str("");
                return false;
            }
            else
            {
                if(channel->talk(player, type, text, channelId))
                {
                    player->tradeTicks = 30000;
                    return true;
                }
            }
        }
    }
    if(channelId != 0x00 || (text[0] != '!' && text[0] != '/'))
    {
        if(channelId == 0x00)
        {
            switch(player->guildStatus)
            {
            case GUILD_VICE:
                return channel->talk(player, SPEAK_CHANNEL_O, text, channelId);
            case GUILD_LEADER:
                return channel->talk(player, SPEAK_CHANNEL_R1, text, channelId);
            default:
                break;
            }
        }

        return channel->talk(player, type, text, channelId);
    }

    if(!player->getGuildId())
    {
        player->sendCancel("Nie jestes w gildii.");
        return true;
    }

    if(!Guild::getInstance()->guildExists(player->getGuildId()))
    {
        player->sendCancel("Wyglada na to ze twoja gildia nie istnieje.");
        return true;
    }

    char buffer[350];
    if(text.substr(1) == "disband")
    {
        if(player->guildStatus == GUILD_LEADER)
        {
            uint32_t guildId = player->getGuildId();
            std::string tekst = "Twoja gildia zostala rozwiazania.";
            channel->talk(player, SPEAK_CHANNEL_R1, tekst, channelId);
            Guild::getInstance()->disbandGuild(guildId);
        }
        else
            player->sendCancel("Nie jestes liderem gildii.");
    }
    else if(text.substr(1, 6) == "invite")
    {
        if(player->guildStatus > GUILD_MEMBER)
        {
            if(text.length() > 7)
            {
                std::string param = text.substr(8);
                trimString(param);
                Player* paramPlayer = g_game.getPlayerByName(param);
                if(paramPlayer)
                {
                    if(paramPlayer->getGuildId() == 0)
                    {
                        if(!paramPlayer->isGuildInvited(player->getGuildId()))
                        {
                            sprintf(buffer, "%s zaprosil Cie do gildii, %s. Mozesz dolaczyc piszac: !joinguild %s", player->getName().c_str(), player->getGuildName().c_str(), player->getGuildName().c_str());
                            paramPlayer->sendTextMessage(MSG_INFO, buffer);
                            sprintf(buffer, "%s zaprosil %s do gildii.", player->getName().c_str(), paramPlayer->getName().c_str());
                            channel->talk(player, SPEAK_CHANNEL_R1, buffer, channelId);
                            paramPlayer->invitedToGuildsList.push_back(player->getGuildId());
                        }
                        else
                            player->sendCancel("Gracz o podanym nicku zostal juz zaproszony do gildii.");
                    }
                    else
                        player->sendCancel("Gracz o podanym nicku jest juz w gildii.");
                }
                else if(IOPlayerSQL::getInstance()->playerExists(param))
                {
                    uint32_t guid;
                    IOPlayerSQL::getInstance()->getGuidByName(guid, param);
                    if(!Guild::getInstance()->hasGuild(guid))
                    {
                        if(!Guild::getInstance()->isInvited(player->getGuildId(), guid))
                        {
                            if(Guild::getInstance()->guildExists(player->getGuildId()))
                            {
                                Guild::getInstance()->invitePlayer(player->getGuildId(), guid);
                                sprintf(buffer, "%s zaprosil %s do gildii.", player->getName().c_str(), param.c_str());
                                channel->talk(player, SPEAK_CHANNEL_R1, buffer, channelId);
                            }
                            else
                                player->sendCancel("Twoja gildia juz nie istnieje.");
                        }
                        else
                            player->sendCancel("Gracz o podanym nicku zostal juz zaproszony do gildii.");
                    }
                    else
                        player->sendCancel("Gracz o podanym nicku jest juz w gildii.");
                }
                else
                    player->sendCancel("Gracz o podanym nicku nie istnieje.");
            }
            else
                player->sendCancel("Niepoprawne parametry.");
        }
        else
            player->sendCancel("Nie masz uprawnien do zapraszenia graczy do gildii.");
    }
    else if(text.substr(1, 5) == "leave")
    {
        if(player->guildStatus < GUILD_LEADER)
        {
            sprintf(buffer, "%s opuscil gildie.", player->getName().c_str());
            channel->talk(player, SPEAK_CHANNEL_R1, buffer, channelId);
            player->leaveGuild();
        }
        else
            player->sendCancel("Nie mozesz opuscic gildii poniewaz jestes jej liderem, musisz oddac lidera komus innemu z gildii lub rozwiazac gildie.");
    }
    else if(text.substr(1, 6) == "revoke")
    {
        if(player->guildStatus > GUILD_MEMBER)
        {
            if(text.length() > 7)
            {
                std::string param = text.substr(8);
                trimString(param);
                Player* paramPlayer = g_game.getPlayerByName(param);
                if(paramPlayer)
                {
                    if(paramPlayer->getGuildId() == 0)
                    {
                        InvitedToGuildsList::iterator it = std::find(paramPlayer->invitedToGuildsList.begin(),paramPlayer->invitedToGuildsList.end(), player->getGuildId());
                        if(it != paramPlayer->invitedToGuildsList.end())
                        {
                            sprintf(buffer, "%s anulowal twoje zaproszenie do gildii.", player->getName().c_str());
                            paramPlayer->sendTextMessage(MSG_INFO, buffer);
                            sprintf(buffer, "%s anulowal zaproszenie dla %s.", player->getName().c_str(), paramPlayer->getName().c_str());
                            channel->talk(player, SPEAK_CHANNEL_R1, buffer, channelId);
                            paramPlayer->invitedToGuildsList.erase(it);
                            return true;
                        }
                        else
                            player->sendCancel("Gracz o podanym nicku nie jest zaproszony.");
                    }
                    else
                        player->sendCancel("Gracz o podanym nicku jest juz w gildii.");
                }
                else if(IOPlayerSQL::getInstance()->playerExists(param))
                {
                    uint32_t guid;
                    IOPlayerSQL::getInstance()->getGuidByName(guid, param);
                    if(Guild::getInstance()->isInvited(player->getGuildId(), guid))
                    {
                        if(Guild::getInstance()->guildExists(player->getGuildId()))
                        {
                            sprintf(buffer, "%s anulowal zaproszenie dla %s.", player->getName().c_str(), param.c_str());
                            channel->talk(player, SPEAK_CHANNEL_R1, buffer, channelId);
                            Guild::getInstance()->revokeInvite(player->getGuildId(), guid);
                        }
                        else
                            player->sendCancel("Wyglada na to ze twoja gildia juz nie istnieje.");
                    }
                    else
                        player->sendCancel("Gracz o podanym nicku nie jest zaproszony.");
                }
                else
                    player->sendCancel("Gracz o podanym nicku nie istnieje.");
            }
            else
                player->sendCancel("Niepoprawne parametry.");
        }
        else
            player->sendCancel("Nie masz praw do anulowania zaproszen do gildii.");
    }
    else if(text.substr(1, 7) == "promote" || text.substr(1, 6) == "demote" || text.substr(1, 14) == "passleadership" || text.substr(1, 4) == "kick")
    {
        if(player->guildStatus == GUILD_LEADER)
        {
            std::string param;
            uint32_t length = 0;
            if(text[2] == 'r')
                length = 9;
            else if(text[2] == 'e')
                length = 7;
            else if(text[2] == 'a')
                length = 16;
            else
                length = 6;

            if(text.length() < length)
            {
                player->sendCancel("Niepoprawne parametry.");
                return true;
            }

            param = text.substr(length);
            trimString(param);
            Player* paramPlayer = g_game.getPlayerByName(param);
            if(paramPlayer)
            {
                if(paramPlayer->getGuildId())
                {
                    if(Guild::getInstance()->guildExists(paramPlayer->getGuildId()))
                    {
                        if(player->getGuildId() == paramPlayer->getGuildId())
                        {
                            if(text[2] == 'r')
                            {
                                if(paramPlayer->guildStatus == GUILD_MEMBER)
                                {
                                    if(paramPlayer->isPremium())
                                    {
                                        paramPlayer->setGuildLevel(GUILD_VICE);
                                        sprintf(buffer, "%s awansowal %s do %s.", player->getName().c_str(), paramPlayer->getName().c_str(), paramPlayer->getRankName().c_str());
                                        channel->talk(player, SPEAK_CHANNEL_R1, buffer, channelId);
                                    }
                                    else
                                        player->sendCancel("Gracz nie posiada PACC.");
                                }
                                else
                                    player->sendCancel("Mozesz tylko awansowac zwyklych czlonkow do vice-liderow.");
                            }
                            else if(text[2] == 'e')
                            {
                                if(paramPlayer->guildStatus == GUILD_VICE)
                                {
                                    paramPlayer->setGuildLevel(GUILD_MEMBER);
                                    sprintf(buffer, "%s zdegradowal %s do %s.", player->getName().c_str(), paramPlayer->getName().c_str(), paramPlayer->getRankName().c_str());
                                    channel->talk(player, SPEAK_CHANNEL_R1, buffer, channelId);
                                }
                                else
                                    player->sendCancel("Mozesz tylko degradowac vice-liderow do zwyklych czlonkow.");
                            }
                            else if(text[2] == 'a')
                            {
                                if(paramPlayer->guildStatus == GUILD_VICE)
                                {
                                    const uint32_t levelToFormGuild = g_config.GUILD_FORM_LEVEL;
                                    if(paramPlayer->getLevel() >= levelToFormGuild || player->access > 1)
                                    {
                                        paramPlayer->setGuildLevel(GUILD_LEADER);
                                        player->setGuildLevel(GUILD_VICE);
                                        Guild::getInstance()->updateOwnerId(paramPlayer->getGuildId(), paramPlayer->getGUID());
                                        sprintf(buffer, "%s oddal lidera dla %s.", player->getName().c_str(), paramPlayer->getName().c_str());
                                        channel->talk(player, SPEAK_CHANNEL_R1, buffer, channelId);
                                    }
                                    else
                                    {
                                        sprintf(buffer, "Nowy guild lider musi posiadac %d poziom.", levelToFormGuild);
                                        player->sendCancel(buffer);
                                    }
                                }
                                else
                                    player->sendCancel("Gracz o podanym nicku nie jest vice-liderem.");
                            }
                            else
                            {
                                if(player->guildStatus > paramPlayer->guildStatus)
                                {
                                    sprintf(buffer, "%s zostal wykopany przez %s.", paramPlayer->getName().c_str(), player->getName().c_str());
                                    channel->talk(player, SPEAK_CHANNEL_R1, buffer, channelId);
                                    paramPlayer->leaveGuild();
                                }
                                else
                                    player->sendCancel("Mozesz wykopac graczy o nizszej randze.");
                            }
                        }
                        else
                            player->sendCancel("Nie jestes w tej samej gildii co podany gracz. You are not in the same guild as a player with that name.");
                    }
                    else
                        player->sendCancel("Nie mo¿na znalezc gildii gracza o tej nazwie.");
                }
                else
                    player->sendCancel("Gracz o podanym nicku nie jest w gildii.");
            }
            else if(IOPlayerSQL::getInstance()->playerExists(param))
            {
                uint32_t guid;
                IOPlayerSQL::getInstance()->getGuidByName(guid, param);
                if(Guild::getInstance()->hasGuild(guid))
                {
                    if(player->getGuildId() == Guild::getInstance()->getGuildId(guid))
                    {
                        if(text[2] == 'r')
                        {
                            if(Guild::getInstance()->getGuildLevel(guid) == GUILD_MEMBER)
                            {
                                Guild::getInstance()->setGuildLevel(guid, GUILD_VICE);
                                sprintf(buffer, "%s awansowal %s do %s.", player->getName().c_str(), param.c_str(), Guild::getInstance()->getRank(guid).c_str());
                                channel->talk(player, SPEAK_CHANNEL_R1, buffer, channelId);

                            }
                            else
                                player->sendCancel("Mozesz tylko awansowac zwyklych czlonkow do vice-liderow.");
                        }
                        else if(text[2] == 'e')
                        {
                            if(Guild::getInstance()->getGuildLevel(guid) == GUILD_VICE)
                            {
                                Guild::getInstance()->setGuildLevel(guid, GUILD_MEMBER);
                                sprintf(buffer, "%s zdegradowal %s do %s.", player->getName().c_str(), param.c_str(), Guild::getInstance()->getRank(guid).c_str());
                                channel->talk(player, SPEAK_CHANNEL_R1, buffer, channelId);
                            }
                            else
                                player->sendCancel("Mozesz tylko degradowac vice-liderow do zwyklych czlonkow.");
                        }
                        else if(text[2] == 'a')
                        {
                            if(Guild::getInstance()->getGuildLevel(guid) == GUILD_VICE)
                            {
                                const uint32_t levelToFormGuild = g_config.GUILD_FORM_LEVEL;
                                if(IOPlayerSQL::getInstance()->getLevel(guid) >= levelToFormGuild || player->access > 1)
                                {
                                    Guild::getInstance()->setGuildLevel(guid, GUILD_LEADER);
                                    player->setGuildLevel(GUILD_VICE);
                                    sprintf(buffer, "%s oddal lidera dla %s.", player->getName().c_str(), param.c_str());
                                    channel->talk(player, SPEAK_CHANNEL_R1, buffer, channelId);
                                }
                                else
                                {
                                    sprintf(buffer, "Nowy lider musi posiadac %d poziom.", levelToFormGuild);
                                    player->sendCancel(buffer);
                                }
                            }
                            else
                                player->sendCancel("Gracz o podanym nicku nie jest vice-liderem.");
                        }
                        else
                        {
                            sprintf(buffer, "%s zostal wykopany przez %s.", param.c_str(), player->getName().c_str());
                            channel->talk(player, SPEAK_CHANNEL_R1, buffer, channelId);
                            IOPlayerSQL::getInstance()->resetGuildInformation(guid);
                        }
                    }
                }
                else
                    player->sendCancel("Gracz o podanym nicku nie jest w gildii.");
            }
            else
                player->sendCancel("Gracz o podanym nicku nie istnieje.");
        }
        else
            player->sendCancel("Nie jestes liderem gildii.");
    }
    else if(text.substr(1, 4) == "nick" && text.length() > 5)
    {
        StringVec params = explodeString(text.substr(6), ",");
        if(params.size() >= 2)
        {
            std::string param1 = params[0], param2 = params[1];
            trimString(param1);
            trimString(param2);
            Player* paramPlayer = g_game.getPlayerByName(param1);
            if(paramPlayer)
            {
                if(paramPlayer->getGuildId())
                {
                    if(param2.length() > 2)
                    {
                        if(param2.length() < 21)
                        {
                            if(isValidName(param2, false))
                            {
                                if(Guild::getInstance()->guildExists(paramPlayer->getGuildId()))
                                {
                                    if(player->getGuildId() == paramPlayer->getGuildId())
                                    {
                                        if(paramPlayer->guildStatus < player->guildStatus || (player == paramPlayer && player->guildStatus > GUILD_MEMBER))
                                        {
                                            paramPlayer->setGuildNick(param2);
                                            if(player != paramPlayer)
                                                sprintf(buffer, "%s ustawil guildnick dla %s na \"%s\".", player->getName().c_str(), paramPlayer->getName().c_str(), param2.c_str());
                                            else
                                                sprintf(buffer, "%s ustawil sobie guild nick na \"%s\".", player->getName().c_str(), param2.c_str());
                                            channel->talk(player, SPEAK_CHANNEL_R1, buffer, channelId);
                                        }
                                        else
                                            player->sendCancel("Mozesz zmienic guildnick dla graczy z nizsza ranga.");
                                    }
                                    else
                                        player->sendCancel("Gracz o podanym nicku nie jest w twojej gildii.");
                                }
                                else
                                    player->sendCancel("Nie mozna znalezc gracza z ta nazwa gildii.");
                            }
                            else
                                player->sendCancel("Ten guildnick jest niepoprawny.");
                        }
                        else
                            player->sendCancel("Guildnick jest za dlugi, wybierz krotszy.");
                    }
                    else
                        player->sendCancel("Guildnick jest za krotki, wybierz dluzszy.");
                }
                else
                    player->sendCancel("Gracz o podanym nicku nie jest w gildii.");
            }
            else if(IOPlayerSQL::getInstance()->playerExists(param1))
            {
                uint32_t guid;
                IOPlayerSQL::getInstance()->getGuidByName(guid, (std::string&)param1);
                if(Guild::getInstance()->hasGuild(guid))
                {
                    if(param2.length() > 2)
                    {
                        if(param2.length() < 21)
                        {
                            if(isValidName(param2, false))
                            {
                                if(Guild::getInstance()->guildExists(guid))
                                {
                                    if(player->getGuildId() == Guild::getInstance()->getGuildId(guid))
                                    {
                                        if(Guild::getInstance()->getGuildLevel(guid) < player->guildStatus)
                                        {
                                            Guild::getInstance()->setGuildNick(guid, param2);
                                            sprintf(buffer, "%s ustawil guildnick dla %s na \"%s\".", player->getName().c_str(), param1.c_str(), param2.c_str());
                                            channel->talk(player, SPEAK_CHANNEL_R1, buffer, channelId);
                                        }
                                        else
                                            player->sendCancel("Mozesz zmienic guildnick dla graczy z nizsza ranga.");
                                    }
                                    else
                                        player->sendCancel("Gracz o podanym nicku nie jest w twojej gildii.");
                                }
                                else
                                    player->sendCancel("Nie mozna znalezc gracza z ta nazwa gildii.");
                            }
                            else
                                player->sendCancel("Ten guildnick jest niepoprawny.");
                        }
                        else
                            player->sendCancel("Guildnick jest za dlugi, wybierz krotszy.");
                    }
                    else
                        player->sendCancel("Guildnick jest za krotki, wybierz dluzszy.");
                }
                else
                    player->sendCancel("Gracz o podanym nicku nie jest w rzadnej gildii.");
            }
            else
                player->sendCancel("Gracz o podanym nicku nie istnieje.");
        }
        else
            player->sendCancel("Niepoprawne parametry.");
    }
    else if(text.substr(1, 11) == "setrankname" && text.length() > 12)
    {
        StringVec params = explodeString(text.substr(13), ",");
        if(params.size() >= 2)
        {
            std::string param1 = params[0], param2 = params[1];
            trimString(param1);
            trimString(param2);
            if(player->guildStatus == GUILD_LEADER)
            {
                if(param2.length() > 2)
                {
                    if(param2.length() < 21)
                    {
                        if(isValidName(param2, false))
                        {
                            if(Guild::getInstance()->getRankIdByName(player->getGuildId(), param1))
                            {
                                if(!Guild::getInstance()->getRankIdByName(player->getGuildId(), param2))
                                {
                                    Guild::getInstance()->changeRank(player->getGuildId(), param1, param2);
                                    sprintf(buffer, "%s zmienil guildrank: \"%s\", na: \"%s\".", player->getName().c_str(), param1.c_str(), param2.c_str());
                                    channel->talk(player, SPEAK_CHANNEL_R1, buffer, channelId);
                                }
                                else
                                    player->sendCancel("Istnieje juz ranga w gildii o tej nazwie.");
                            }
                            else
                                player->sendCancel("Nie ma takiego rankname w twojej gildii.");
                        }
                        else
                            player->sendCancel("Nowy guildrank zawiera niepoprane znaki.");
                    }
                    else
                        player->sendCancel("Nowy rankname jest za dlugi.");
                }
                else
                    player->sendCancel("Nowy rankname jest za krotki.");
            }
            else
                player->sendCancel("Nie jestes liderem gildii.");
        }
        else
            player->sendCancel("Niepoprawne parametry.");
    }
    else if(text.substr(1, 7) == "setmotd")
    {
        if(player->guildStatus == GUILD_LEADER)
        {
            if(text.length() > 8)
            {
                std::string param = text.substr(9);
                trimString(param);
                if(param.length() > 2)
                {
                    if(param.length() < 225)
                    {
                        Guild::getInstance()->setMotd(player->getGuildId(), param);
                        sprintf(buffer, "%s zmienil motd: %s", player->getName().c_str(), param.c_str());
                        channel->talk(player, SPEAK_CHANNEL_R1, buffer, channelId);
                    }
                    else
                        player->sendCancel("Motd jest za dlugi");
                }
                else
                    player->sendCancel("motd jest za krotki.");
            }
            else
                player->sendCancel("Niepoprawne parametry.");
        }
        else
            player->sendCancel("Tylko liderzy moga zmienic motd.");
    }
    else if(text.substr(1, 9) == "cleanmotd")
    {
        if(player->guildStatus == GUILD_LEADER)
        {
            Guild::getInstance()->setMotd(player->getGuildId(), "");
            sprintf(buffer, "%s wyczyscil motd.", player->getName().c_str());
            channel->talk(player, SPEAK_CHANNEL_R1, buffer, channelId);
        }
        else
            player->sendCancel("Tylko lider moze wyczyscic motd.");
    }
    else if(text.substr(1, 8) == "commands")
        player->sendToChannel(player, SPEAK_CHANNEL_R1, "Komendy z parametrami: disband, invite[name], leave, kick[name], revoke[name], demote[name], promote[name], passleadership[name], nick[name, nick], setrankname[oldName, newName], setmotd[text] i cleanmotd.", channelId);
    else
        return false;

    return false;
}
std::string Chat::getChannelName(Player *player, uint16_t channelId)
{
    ChatChannel *channel = getChannel(player, channelId);
    if(channel)
        return channel->getName();
    else
        return "";
}

ChannelList Chat::getChannelList(Player *player)
{
    ChannelList list;
    NormalChannelMap::iterator itn;
    PrivateChannelMap::iterator it;
    bool gotPrivate = false;

    // If has guild
#ifdef FIXY
    if(player->guildStatus >= GUILD_MEMBER && player->getGuildName().length())
    {
#else
    if(player->getGuildId() && player->getGuildName().length())
    {
#endif //FIXY
        ChatChannel *channel = getChannel(player, 0x00);
        if(channel)
            list.push_back(channel);
        else if(channel = createChannel(player, 0x00))
            list.push_back(channel);
    }

    for(itn = m_normalChannels.begin(); itn != m_normalChannels.end(); ++itn)
    {
        //TODO: Permisions for channels and checks
        ChatChannel *channel = itn->second;
        if ((itn->first == 0x01 && player->access == 0))
            continue;
#ifdef HUCZU_RRV
        if(itn->first == 0x03 && player->access < 1)   // violations
            continue;
#endif
        list.push_back(channel);
    }

    for(it = m_privateChannels.begin(); it != m_privateChannels.end(); ++it)
    {
        PrivateChatChannel* channel = it->second;

        if(channel)
        {
            if(channel->isInvited(player))
                list.push_back(channel);

            if(channel->getOwner() == player->getName())
                gotPrivate = true;
        }
    }

    if(!gotPrivate)
        list.push_front(dummyPrivate);

    return list;
}

ChatChannel *Chat::getChannel(Player *player, uint16_t channelId)
{
    if(channelId == 0x00)
    {
        GuildChannelMap::iterator it = m_guildChannels.find(player->getGuildId());
        if(it == m_guildChannels.end())
            return NULL;

        return it->second;
    }
    else
    {
        NormalChannelMap::iterator it = m_normalChannels.find(channelId);
        if(it != m_normalChannels.end())
        {
            return it->second;
        }
        else
        {
            PrivateChannelMap::iterator it = m_privateChannels.find(channelId);
            if(it == m_privateChannels.end())
                return NULL;

            return it->second;
        }
    }
}

PrivateChatChannel* Chat::getPrivateChannel(Player* player)
{
    for(PrivateChannelMap::iterator it = m_privateChannels.begin(); it != m_privateChannels.end(); ++it)
    {
        if(PrivateChatChannel* channel = it->second)
        {
            if(channel->getOwner() == player->getName())
            {
                return channel;
            }
        }
    }

    return NULL;
}
