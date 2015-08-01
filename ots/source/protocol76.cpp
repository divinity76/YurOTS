//#include "preheaders.h"

#include <string>
#include <iostream>
#include <sstream>
#include <list>
#include <fstream>

#include "networkmessage.h"
#include "protocol76.h"
#include "items.h"
#include "tile.h"
#include "creature.h"
#include "player.h"
#include "status.h"
#include "chat.h"
#include "town.h"

#include "luascript.h"
#include "otsystem.h"
#include "actions.h"
#include "game.h"
#include "ioplayersql.h"

extern LuaScript g_config;
extern Actions actions;
Chat g_chat;

Protocol76::Protocol76(SOCKET s)
{
    OTSYS_THREAD_LOCKVARINIT(bufferLock);

    player = NULL;
    pendingLogout = false;

    windowTextID = 0;
    readItem = NULL;
    this->s = s;
}


Protocol76::~Protocol76()
{
    OTSYS_THREAD_LOCKVARRELEASE(bufferLock);
    if(s)
    {
        closesocket(s);
        s = 0;
    }
    player = NULL;
}

void Protocol76::reinitializeProtocol(SOCKET _s)
{
    windowTextID = 0;
    readItem = NULL;
    OutputBuffer.Reset();
    knownPlayers.clear();
    if(s)
        closesocket(s);
    s = _s;
}

#ifdef YUR_LOGIN_QUEUE
bool Protocol76::ConnectPlayer(int32_t* placeInQueue)
{
    return game->placeCreature(player->pos, player, placeInQueue);
}
#else //YUR_LOGIN_QUEUE
bool Protocol76::ConnectPlayer()
{
    Status* stat = Status::instance();
    if(!stat->hasSlot() && player->access == 0)
        return false;
    else
        return game->placeCreature(player->pos, player);
}
#endif //YUR_LOGIN_QUEUE

void Protocol76::ReceiveLoop()
{
    NetworkMessage msg;
    do
    {
        while(pendingLogout == false && msg.ReadFromSocket(s))
        {
            parsePacket(msg);
        }

        if(s)
        {
            closesocket(s);
            s = 0;
        }
        // logout by disconnect?  -> kick
        if(pendingLogout == false)
        {
            game->playerSetAttackedCreature(player, 0);
            //player->setAttackedCreature(0);
            while(player->inFightTicks >= 1000 && player->isRemoved == false && s == 0)
            {
                OTSYS_SLEEP(250);
            }
            OTSYS_THREAD_LOCK(game->gameLock, "Protocol76::ReceiveLoop()")
            if(s == 0 && player->isRemoved == false)
            {
                /*#ifdef HUCZU_NOLOGOUT_TILE
                                Tile* tile = game->map->getTile(player->pos);
                                if(tile && !tile->isNoLogout())
                                {
                                    Status* stat = Status::instance();
                                    stat->removePlayer();
                #endif*/
                game->removeCreature(player);
                /*#ifdef HUCZU_NOLOGOUT_TILE
                                }
                #endif*/
            }
            OTSYS_THREAD_UNLOCK(game->gameLock, "Protocol76::ReceiveLoop()")
        }
    }
    while(s != 0 && player->isRemoved == false);
}


void Protocol76::parsePacket(NetworkMessage &msg)
{
    uint8_t recvbyte = msg.GetByte();
    if(msg.getMessageLength() <= 0)
        return;

    if((recvbyte == 0x6F || recvbyte == 0x70 || recvbyte == 0x71 || recvbyte == 0x72) && player->lastPacket == recvbyte)
        return;

    //a dead player can not performs actions
    if (player->isRemoved == true && recvbyte != 0x14)
    {
        OTSYS_SLEEP(10);
        return;
    }
    player->lastPacket = recvbyte;

    switch(recvbyte)
    {
    case 0x14: // logout
        parseLogout(msg);
        break;

    case 0x64: // client moving with steps
        parseMoveByMouse(msg);
        break;

    case 0x65: // move north
        parseMoveNorth(msg);
        break;

    case 0x66: // move east
        parseMoveEast(msg);
        break;

    case 0x67: // move south
        parseMoveSouth(msg);
        break;

    case 0x68: // move west
        parseMoveWest(msg);
        break;

    case 0x6A:
        parseMoveNorthEast(msg);
        break;

    case 0x6B:
        parseMoveSouthEast(msg);
        break;

    case 0x6C:
        parseMoveSouthWest(msg);
        break;

    case 0x6D:
        parseMoveNorthWest(msg);
        break;

    case 0x6F: // turn north
        parseTurnNorth(msg);
        break;

    case 0x70: // turn east
        parseTurnEast(msg);
        break;

    case 0x71: // turn south
        parseTurnSouth(msg);
        break;

    case 0x72: // turn west
        parseTurnWest(msg);
        break;

    case 0x7D: // Request trade
        parseRequestTrade(msg);
        break;

    case 0x7E: // Look at an item in trade
        parseLookInTrade(msg);
        break;

    case 0x7F: // Accept trade
        parseAcceptTrade(msg);
        break;

    case 0x80: // Close/cancel trade
        parseCloseTrade();
        break;

    case 0x78: // throw item
        parseThrow(msg);
        break;

    case 0x82: // use item
        parseUseItem(msg);
        break;

    case 0x83: // use item
        parseUseItemEx(msg);
        break;

    case 0x84: // battle window
        parseBattleWindow(msg);
        break;

    case 0x85:	//rotate item
        parseRotateItem(msg);
        break;

    case 0x87: // close container
        parseCloseContainer(msg);
        break;

    case 0x88: //"up-arrow" - container
        parseUpArrowContainer(msg);
        break;

    case 0x89:
        parseTextWindow(msg);
        break;

    case 0x8A: // save house text
        parseHouseWindow(msg);
        break;

    case 0x8C: // look at
        if(player->lookex == 0)
        {
            player->lookex += 1;
            parseLookAt(msg);
        }
        else
        {
            player->sendCancel("Za szybko!");
        }
        break;

    case 0x96:  // say something
        parseSay(msg);
        break;

#ifdef HUCZU_RRV
    case 0x9B: // send report (reportee)
        openViolation(msg);
        break;

    case 0x9C: // close report (gm)
        closeViolation(msg);
        break;

    case 0x9D: // cancel report (reportee) -- Vitor packetid
        cancelViolation(player->getName());
        break;
#endif

    case 0xA1: // attack
        parseAttack(msg);
        break;

#ifdef HUCZU_FOLLOW
    case 0xA2: // follow
        parseFollow(msg);
        break;
#endif //HUCZU_FOLLOW

    case 0xA3: // invite party
        parseInviteParty(msg);
        break;

    case 0xA5: // revoke party
        parseRevokeParty(msg);
        break;

    case 0xA7: // leave party
        game->LeaveParty(player);
        break;

    case 0xA4: // join party
        parseJoinParty(msg);
        break;

    case 0xA6: // pass leadership
        parsePassLeadership(msg);
        break;

    case 0xAA:
        parseCreatePrivateChannel(msg);
        break;

    case 0xAB:
        parseChannelInvite(msg);
        break;

    case 0xAC:
        parseChannelExclude(msg);
        break;

    case 0xD2: // request Outfit
        if(player->antyrainbow == 0)
        {
            player->antyrainbow += 5;
            parseRequestOutfit(msg);
        }
        else
        {
            player->sendCancel("Nie aktywne");
        }
        break;

    case 0xD3: // set outfit
        if(player->antyrainbow2 == 0)
        {
            player->antyrainbow2 += 5;
            parseSetOutfit(msg);
        }
        else
        {
            player->sendCancel("Nie aktywne");
        }
        break;

    case 0x97: // request Channels
        parseGetChannels(msg);
        break;

    case 0x98: // open Channel
        parseOpenChannel(msg);
        break;

    case 0x99: // close Channel
        parseCloseChannel(msg);
        break;

#ifdef HUCZU_BAN_SYSTEM
    case 0xe7:
        parseGM(msg);
        break;
#endif //HUCZU_BAN_SYSTEM

    case 0x9A: // open priv
        parseOpenPriv(msg);
        break;

    case 0xBE: // cancel move
        parseCancelMove(msg);
        break;

    case 0xA0: // set attack and follow mode
        parseModes(msg);
        break;

    case 0xDC:
        parseAddVip(msg);
        break;

    case 0xDD:
        parseRemVip(msg);
        break;

    case 0x69: // client quit without logout <- wrong
        if(game->stopEvent(player->eventAutoWalk))
        {
            sendCancelWalk();
        }
        break;

    case 0x1E: // keep alive / ping response
        player->receivePing();
        break;

    case 0xC9: // change position
        // update position
        break;

    default:
#ifdef __DEBUG__
        printf("unknown packet header: %x \n", recvbyte);
        parseDebug(msg);
#endif
        break;
    }
    game->flushSendBuffers();
}

void Protocol76::GetTileDescription(const Tile* tile, NetworkMessage &msg
#ifdef TRS_GM_INVISIBLE
                                    , Player* p
#endif //TRS_GM_INVISIBLE
                                   )
{
    if (tile)
    {
        int32_t count = 0;
        if(tile->ground)
        {
            msg.AddItem(tile->ground);
            count++;
        }

        if (tile->splash)
        {
            msg.AddItem(tile->splash);
            count++;
        }

        ItemVector::const_iterator it;
        for (it = tile->topItems.begin(); ((it !=tile->topItems.end()) && (count < 10)); ++it)
        {
            msg.AddItem(*it);
            count++;
        }

#ifdef TRS_GM_INVISIBLE
        CreatureVector::const_iterator itc;
        for (itc = tile->creatures.begin(); ((itc !=tile->creatures.end()) && (count < 10)); ++itc)
        {
            Player* player = dynamic_cast<Player*>((*itc));
            if(player)
            {
                if(!player->gmInvisible || player->gmInvisible && player == p || p->access >= player->access)
                {
                    bool known;
                    uint32_t removedKnown;
                    checkCreatureAsKnown((*itc)->getID(), known, removedKnown);
                    AddCreature(msg,*itc, known, removedKnown);
                    count++;
                }
            }
            else
            {
                bool known;
                uint32_t removedKnown;
                checkCreatureAsKnown((*itc)->getID(), known, removedKnown);
                AddCreature(msg,*itc, known, removedKnown);
                count++;
            }
        }
#else //TRS_GM_INVISIBLE
        CreatureVector::const_iterator itc;
        for (itc = tile->creatures.begin(); ((itc !=tile->creatures.end()) && (count < 10)); ++itc)
        {
            bool known;
            uint32_t removedKnown;
            checkCreatureAsKnown((*itc)->getID(), known, removedKnown);
            AddCreature(msg,*itc, known, removedKnown);
            count++;
        }
#endif //TRS_GM_INVISIBLE

        for (it = tile->downItems.begin(); ((it !=tile->downItems.end()) && (count < 10)); ++it)
        {
            msg.AddItem(*it);
            count++;
        }
    }
}

void Protocol76::GetMapDescription(uint16_t x, uint16_t y, unsigned char z,
                                   uint16_t width, uint16_t height,
                                   NetworkMessage &msg
#ifdef TRS_GM_INVISIBLE
                                   , Player* p
#endif //TRS_GM_INVISIBLE
                                  )
{
    Tile* tile;
    int32_t skip = -1;
    int32_t startz, endz, offset, zstep, cc = 0;
    if (z > 7)
    {
        startz = z - 2;
        endz = std::min(MAP_LAYER - 1, z + 2);
        zstep = 1;
    }
    else
    {
        startz = 7;
        endz = 0;

        zstep = -1;
    }

    for(int32_t nz = startz; nz != endz + zstep; nz += zstep)
    {
        offset = z - nz;

        for (int32_t nx = 0; nx < width; nx++)
            for (int32_t ny = 0; ny < height; ny++)
            {
                tile = game->getTile(x + nx + offset, y + ny + offset, nz);
                if (tile)
                {
                    if (skip >= 0)
                    {
                        msg.AddByte(skip);
                        msg.AddByte(0xFF);
                        cc +=skip;
                    }
                    skip = 0;

#ifdef TRS_GM_INVISIBLE
                    GetTileDescription(tile, msg, p);
#else
                    GetTileDescription(tile, msg);
#endif //TRS_GM_INVISIBLE
                    cc++;

                }
                else
                {
                    /*
                    if(skip == -1)
                    skip = 0;
                    	*/

                    skip++;
                    if (skip == 0xFF)
                    {
                        msg.AddByte(0xFF);
                        msg.AddByte(0xFF);
                        cc += skip;
                        skip = -1;
                    }
                }
            }
    }

    if (skip >= 0)
    {
        msg.AddByte(skip);
        msg.AddByte(0xFF);
        cc += skip;
    }

#ifdef __DEBUG__
    printf("tiles in total: %d \n", cc);
#endif
}





void Protocol76::checkCreatureAsKnown(uint32_t id, bool &known, uint32_t &removedKnown)
{
    // loop through the known player and check if the given player is in
    std::list<uint32_t>::iterator i;
    for(i = knownPlayers.begin(); i != knownPlayers.end(); ++i)
    {
        if((*i) == id)
        {
            // know... make the creature even more known...
            knownPlayers.erase(i);
            knownPlayers.push_back(id);

            known = true;
            return;
        }
    }

    // ok, he is unknown...
    known = false;

    // ... but not in future
    knownPlayers.push_back(id);

    // to many known players?
    if(knownPlayers.size() > 150)
    {
        // lets try to remove one from the end of the list
        for (int32_t n = 0; n < 150; n++)
        {
            removedKnown = knownPlayers.front();

            Creature *c = game->getCreatureByID(removedKnown);
            if ((!c) || (!CanSee(c)))
                break;

            // this creature we can't remove, still in sight, so back to the end
            knownPlayers.pop_front();
            knownPlayers.push_back(removedKnown);
        }

        // hopefully we found someone to remove :S, we got only 150 tries
        // if not... lets kick some players with debug errors :)
        knownPlayers.pop_front();
    }
    else
    {
        // we can cache without problems :)
        removedKnown = 0;
    }
}


// Parse methods
void Protocol76::parseLogout(NetworkMessage &msg)
{
    /*#ifdef HUCZU_NOLOGOUT_TILE
        Tile* tile = game->map->getTile(player->pos);
        if(tile)
        {
            if (tile->isPvpArena())
                game->teleport(player, tile->getPvpArenaExit());
            else if(tile->isNoLogout())
            {
                player->sendCancel("Nie mozesz sie tutaj wylogowac.");
                return;
            }
        }
    #endif*/
    if(player->inFightTicks >=1000 && player->isRemoved == false)
    {
        sendCancel("Nie mozesz sie wylogowac po walce!");
        return;
    }

    logout();
}

void Protocol76::logout()
{
    // we ask the game to remove us
    if(player->isRemoved == false)
    {
        if(game->removeCreature(player))
            pendingLogout = true;
    }
    else
    {
        pendingLogout = true;
    }
}

void Protocol76::parseCreatePrivateChannel(NetworkMessage& msg)
{
    OTSYS_THREAD_LOCK_CLASS lockClass(game->gameLock, "Protocol76::parseCreatePrivateChannel()");
    if(player->isRemoved == true)
    {
        return;
    }

    ChatChannel *channel = g_chat.createChannel(player, 0xFFFF);

    if(channel)
    {
        if(channel->addUser(player))
        {
            NetworkMessage msg;
            msg.AddByte(0xB2);
            msg.AddU16(channel->getId());
            msg.AddString(channel->getName());

            WriteBuffer(msg);
        }
    }
}

void Protocol76::parseChannelInvite(NetworkMessage& msg)
{
    std::string name = msg.GetString();

    OTSYS_THREAD_LOCK_CLASS lockClass(game->gameLock, "Protocol76::parseChannelInvite()");
    if(player->isRemoved == true)
    {
        return;
    }

    PrivateChatChannel *channel = g_chat.getPrivateChannel(player);

    if(channel)
    {
        Player *invitePlayer = game->getPlayerByName(name);

        if(invitePlayer)
        {
            channel->invitePlayer(player, invitePlayer);
        }
    }
}

void Protocol76::parseChannelExclude(NetworkMessage& msg)
{
    std::string name = msg.GetString();

    OTSYS_THREAD_LOCK_CLASS lockClass(game->gameLock, "Protocol76::parseChannelExclude()");
    if(player->isRemoved == true)
    {
        return;
    }

    PrivateChatChannel *channel = g_chat.getPrivateChannel(player);

    if(channel)
    {
        Player *excludePlayer = game->getPlayerByName(name);

        if(excludePlayer)
        {
            channel->excludePlayer(player, excludePlayer);
        }
    }
}

void Protocol76::parseGetChannels(NetworkMessage &msg)
{
    sendChannelsDialog();
}

void Protocol76::parseOpenChannel(NetworkMessage &msg)
{
    uint16_t channelId = msg.GetU16();
    OTSYS_THREAD_LOCK_CLASS lockClass(game->gameLock, "Protocol76::parseOpenChannel()");
#ifdef HUCZU_RRV
    if (channelId == 0x03 && player->access >= 1) // violations only for couns++
    {
        sendViolationChannel(player);
        return;
    }
    else
    {
#endif
        if(g_chat.addUserToChannel(player, channelId))
            sendChannel(channelId, g_chat.getChannelName(player, channelId));
#ifdef HUCZU_RRV
    }
#endif
}

void Protocol76::parseCloseChannel(NetworkMessage &msg)
{
    uint16_t channelId = msg.GetU16();
    OTSYS_THREAD_LOCK_CLASS lockClass(game->gameLock, "Protocol76::parseCloseChannel()");
#ifdef HUCZU_RRV
    if (channelId == 0x03)
        player->hasViolationsChannelOpen = false;
    else
#endif
        g_chat.removeUserFromChannel(player, channelId);
#ifdef HUCZU_SERVER_LOG
    if(channelId == 0x02)
        sendChannel(0x02, "Server Log");
#endif
}

void Protocol76::parseOpenPriv(NetworkMessage &msg)
{
    std::string receiver;
    receiver = msg.GetString();
    OTSYS_THREAD_LOCK_CLASS lockClass(game->gameLock, "Protocol76::parseOpenPriv()");
    Player* player = game->getPlayerByName(receiver);
    if(player)
    {
        sendOpenPriv(player->getName());
    }
}

void Protocol76::sendOpenPriv(const std::string &receiver)
{
    NetworkMessage newmsg;
    newmsg.AddByte(0xAD);
    newmsg.AddString(receiver);
    WriteBuffer(newmsg);
}

void Protocol76::parseCancelMove(NetworkMessage &msg)
{
    game->playerSetAttackedCreature(player, 0);
#ifdef HUCZU_FOLLOW
    game->stopEvent(player->eventCheckFollow);
    game->playerSetFollowCreature(player, 0);
#endif //HUCZU_FOLLOW
    game->stopEvent(player->eventAutoWalk);
    player->sendCancelWalk();
}

void Protocol76::parseModes(NetworkMessage &msg)
{
    player->setFightMode(msg.GetByte());
#ifdef HUCZU_FOLLOW
    player->followMode = msg.GetByte();
#endif //HUCZU_FOLLOW
    player->atkMode = msg.GetByte();
}

void Protocol76::parseDebug(NetworkMessage &msg)
{
    int32_t dataLength = msg.getMessageLength()-3;
    if(dataLength!=0)
    {
        printf("data: ");
        size_t data = msg.GetByte();
        while( dataLength > 0)
        {
            printf("%d ", data);
            if(--dataLength >0)
                data = msg.GetByte();
        }
        printf("\n");
    }
}


void Protocol76::parseMoveByMouse(NetworkMessage &msg)
{
    // first we get all directions...
    std::list<Direction> path;
    size_t numdirs = msg.GetByte();
    for (size_t i = 0; i < numdirs; ++i)
    {
        unsigned char rawdir = msg.GetByte();
        Direction dir = SOUTH;

        switch(rawdir)
        {
        case 1:
            dir = EAST;
            break;
        case 2:
            dir = NORTHEAST;
            break;
        case 3:
            dir = NORTH;
            break;
        case 4:
            dir = NORTHWEST;
            break;
        case 5:
            dir = WEST;
            break;
        case 6:
            dir = SOUTHWEST;
            break;
        case 7:
            dir = SOUTH;
            break;
        case 8:
            dir = SOUTHEAST;
            break;

        default:
            continue;
        };

        /*
        #ifdef __DEBUG__
        std::cout << "Walk by mouse: Direction: " << dir << std::endl;
        #endif
        */
        path.push_back(dir);
    }

    game->playerAutoWalk(player, path);
}

void Protocol76::parseMoveNorth(NetworkMessage &msg)
{
    player->notAfk();

    if(game->stopEvent(player->eventAutoWalk))
    {
        player->sendCancelWalk();
    }

    this->sleepTillMove();

    game->thingMove(player, player,
                    player->pos.x, player->pos.y-1, player->pos.z, 1);
}

void Protocol76::parseMoveEast(NetworkMessage &msg)
{
    player->notAfk();

    if(game->stopEvent(player->eventAutoWalk))
    {
        player->sendCancelWalk();
    }

    this->sleepTillMove();

    game->thingMove(player, player,
                    player->pos.x+1, player->pos.y, player->pos.z, 1);
}


void Protocol76::parseMoveSouth(NetworkMessage &msg)
{
    player->notAfk();

    if(game->stopEvent(player->eventAutoWalk))
    {
        player->sendCancelWalk();
    }

    this->sleepTillMove();

    game->thingMove(player, player,
                    player->pos.x, player->pos.y+1, player->pos.z, 1);
}


void Protocol76::parseMoveWest(NetworkMessage &msg)
{
    player->notAfk();

    if(game->stopEvent(player->eventAutoWalk))
    {
        player->sendCancelWalk();
    }

    this->sleepTillMove();

    game->thingMove(player, player,
                    player->pos.x-1, player->pos.y, player->pos.z, 1);
}

void Protocol76::parseMoveNorthEast(NetworkMessage &msg)
{
    player->notAfk();

    if(game->stopEvent(player->eventAutoWalk))
    {
        player->sendCancelWalk();
    }

    this->sleepTillMove();
    this->sleepTillMove();

    game->thingMove(player, player,
                    (player->pos.x+1), (player->pos.y-1), player->pos.z, 1);
}

void Protocol76::parseMoveSouthEast(NetworkMessage &msg)
{
    player->notAfk();

    if(game->stopEvent(player->eventAutoWalk))
    {
        player->sendCancelWalk();
    }

    this->sleepTillMove();
    this->sleepTillMove();

    game->thingMove(player, player,
                    (player->pos.x+1), (player->pos.y+1), player->pos.z, 1);
}

void Protocol76::parseMoveSouthWest(NetworkMessage &msg)
{
    player->notAfk();

    if(game->stopEvent(player->eventAutoWalk))
    {
        player->sendCancelWalk();
    }

    this->sleepTillMove();
    this->sleepTillMove();

    game->thingMove(player, player,
                    (player->pos.x-1), (player->pos.y+1), player->pos.z, 1);
}

void Protocol76::parseMoveNorthWest(NetworkMessage &msg)
{
    player->notAfk();

    if(game->stopEvent(player->eventAutoWalk))
    {
        player->sendCancelWalk();
    }

    this->sleepTillMove();
    this->sleepTillMove();

    game->thingMove(player, player,
                    (player->pos.x-1), (player->pos.y-1), player->pos.z, 1);
}


void Protocol76::parseTurnNorth(NetworkMessage &msg)
{
    player->notAfk();
    game->creatureTurn(player, NORTH);
}


void Protocol76::parseTurnEast(NetworkMessage &msg)
{
    player->notAfk();
    game->creatureTurn(player, EAST);
}


void Protocol76::parseTurnSouth(NetworkMessage &msg)
{
    player->notAfk();
    game->creatureTurn(player, SOUTH);
}


void Protocol76::parseTurnWest(NetworkMessage &msg)
{
    player->notAfk();
    game->creatureTurn(player, WEST);
}

void Protocol76::parseRequestOutfit(NetworkMessage &msg)
{
    msg.Reset();

    msg.AddByte(0xC8);
    msg.AddByte(player->looktype);
    msg.AddByte(player->lookhead);
    msg.AddByte(player->lookbody);
    msg.AddByte(player->looklegs);
    msg.AddByte(player->lookfeet);

    switch (player->getSex())
    {
    case PLAYERSEX_FEMALE:
        msg.AddByte(PLAYER_FEMALE_1);
#ifdef YUR_PREMIUM_PROMOTION
        msg.AddByte(player->isPremium()? PLAYER_FEMALE_7 : PLAYER_FEMALE_4);
#else
        msg.AddByte(PLAYER_FEMALE_4);
#endif //YUR_PREMIUM_PROMOTION
        break;

    case PLAYERSEX_MALE:
        msg.AddByte(PLAYER_MALE_1);
#ifdef YUR_PREMIUM_PROMOTION
        msg.AddByte(player->isPremium()? PLAYER_MALE_7 : PLAYER_MALE_4);
#else
        msg.AddByte(PLAYER_MALE_4);
#endif //YUR_PREMIUM_PROMOTION
        break;

    case PLAYERSEX_OLDMALE:
        msg.AddByte(160);
        msg.AddByte(160);
        break;

    case PLAYERSEX_DWARF:
        msg.AddByte(PLAYER_DWARF);
        msg.AddByte(PLAYER_DWARF);
        break;

    case PLAYERSEX_NIMFA:
        msg.AddByte(PLAYER_NIMFA);
        msg.AddByte(PLAYER_NIMFA);
        break;

    default:
        msg.AddByte(PLAYER_MALE_1);
#ifdef YUR_PREMIUM_PROMOTION
        msg.AddByte(player->isPremium()? PLAYER_MALE_7 : PLAYER_MALE_4);
#else
        msg.AddByte(PLAYER_MALE_4);
#endif //YUR_PREMIUM_PROMOTION
    }

    WriteBuffer(msg);
}

void Protocol76::sendSetOutfit(const Creature* creature)
{
    if (CanSee(creature))
    {
        NetworkMessage newmsg;
        newmsg.AddByte(0x8E);
        newmsg.AddU32(creature->getID());
        if (creature->looktype > 1000)
        {
            newmsg.AddByte(0);
            newmsg.AddU16(Item::items[creature->looktype].clientId);
        }
        else
        {
            if (creature->isInvisible())
            {
                newmsg.AddByte(0);
                newmsg.AddU16(0);
            }
            else
            {
                newmsg.AddByte(creature->looktype);
                newmsg.AddByte(creature->lookhead);
                newmsg.AddByte(creature->lookbody);
                newmsg.AddByte(creature->looklegs);
                newmsg.AddByte(creature->lookfeet);
            }
        }

        WriteBuffer(newmsg);
    }
}

void Protocol76::sendInventory(unsigned char sl_id)
{
    NetworkMessage msg;
    AddPlayerInventoryItem(msg,player,sl_id);
    WriteBuffer(msg);
}

void Protocol76::sendStats()
{
    NetworkMessage msg;
    AddPlayerStats(msg,player);
    WriteBuffer(msg);
}

void Protocol76::sendTextMessage(MessageClasses mclass, const char* message)
{
    NetworkMessage msg;
    AddTextMessage(msg,mclass, message);
    WriteBuffer(msg);
}

void Protocol76::sendTextMessage(MessageClasses mclass, const char* message,const Position &pos, unsigned char type)
{
    NetworkMessage msg;
    AddMagicEffect(msg,pos,type);
    AddTextMessage(msg,mclass, message);
    WriteBuffer(msg);
}

void Protocol76::sendClosePrivate(uint16_t channelId)
{
    NetworkMessage msg;

    msg.AddByte(0xB3);
    msg.AddU16(channelId);

    WriteBuffer(msg);
}

void Protocol76::sendChannelsDialog()
{
    NetworkMessage newmsg;
    ChannelList list;

    list = g_chat.getChannelList(player);

    newmsg.AddByte(0xAB);

    newmsg.AddByte((unsigned char)list.size()); //how many

    while(!list.empty())
    {
        ChatChannel *channel;
        channel = list.front();
        list.pop_front();

        newmsg.AddU16(channel->getId());
        newmsg.AddString(channel->getName());
    }
    WriteBuffer(newmsg);
}

void Protocol76::sendChannel(uint16_t channelId, std::string channelName)
{
    NetworkMessage newmsg;

    newmsg.AddByte(0xAC);
    newmsg.AddU16(channelId);
    newmsg.AddString(channelName);

    WriteBuffer(newmsg);
}

void Protocol76::sendIcons(int32_t icons)
{
    NetworkMessage newmsg;
    newmsg.AddByte(0xA2);
    newmsg.AddByte(icons);
    WriteBuffer(newmsg);
}

void Protocol76::parseSetOutfit(NetworkMessage &msg)
{
    int32_t temp = msg.GetByte();
    if ((player->getSex() == PLAYERSEX_FEMALE && temp >= PLAYER_FEMALE_1 &&
#ifdef YUR_PREMIUM_PROMOTION
            temp <= (player->isPremium()? PLAYER_FEMALE_7 : PLAYER_FEMALE_4)) ||
#else
            temp <= PLAYER_FEMALE_7) ||
#endif //YUR_PREMIUM_PROMOTION
            (player->getSex() == PLAYERSEX_DWARF && temp == PLAYER_DWARF) || (player->getSex() == PLAYERSEX_NIMFA && temp == PLAYER_NIMFA) || (player->getSex() == PLAYERSEX_MALE && temp >= PLAYER_MALE_1 &&
#ifdef YUR_PREMIUM_PROMOTION
                    temp <= (player->isPremium()? PLAYER_MALE_7 : PLAYER_MALE_4)))
#else
                    temp <= PLAYER_MALE_7))
#endif //YUR_PREMIUM_PROMOTION
    {
        player->looktype= temp;
        player->lookmaster = player->looktype;
        player->lookhead=msg.GetByte();
        player->lookbody=msg.GetByte();
        player->looklegs=msg.GetByte();
        player->lookfeet=msg.GetByte();

        game->creatureChangeOutfit(player);
    }
}


void Protocol76::parseUseItemEx(NetworkMessage &msg)
{
    Position pos_from = msg.GetPosition();
    uint16_t itemid = msg.GetItemId();
    unsigned char from_stackpos = msg.GetByte();
    Position pos_to = msg.GetPosition();
    /*uint16_t tile_id = */
    msg.GetU16();
    unsigned char to_stackpos = msg.GetByte();

    game->playerUseItemEx(player,pos_from,from_stackpos, pos_to, to_stackpos, itemid);

}

void Protocol76::parseBattleWindow(NetworkMessage &msg)
{
    Position pos_from = msg.GetPosition();
    uint16_t itemid = msg.GetItemId();
    unsigned char from_stackpos = msg.GetByte();
    uint32_t creatureid = msg.GetU32();

    game->playerUseBattleWindow(player, pos_from, from_stackpos, itemid, creatureid);
}

void Protocol76::sendContainer(unsigned char index, Container *container)
{
    if(!container)
        return;

    NetworkMessage msg;

    player->addContainer(index, container);

    msg.AddByte(0x6E);
    msg.AddByte(index);

    //msg.AddU16(container->getID());
    msg.AddItemId(container);
    msg.AddString(container->getName());
    msg.AddByte(container->capacity());
    if(container->getParent() != NULL)
        msg.AddByte(0x01); // container up ID (can go up)
    else
        msg.AddByte(0x00);

    msg.AddByte(container->size());

    ContainerList::const_iterator cit;
    for (cit = container->getItems(); cit != container->getEnd(); ++cit)
    {
        msg.AddItem(*cit);
    }
    WriteBuffer(msg);
}

void Protocol76::sendTradeItemRequest(const Player* player, const Item* item, bool ack)
{
    NetworkMessage msg;
    if(ack)
    {
        msg.AddByte(0x7D);
    }
    else
    {
        msg.AddByte(0x7E);
    }

    msg.AddString(player->getName());

    const Container *tradeContainer = dynamic_cast<const Container*>(item);
    if(tradeContainer)
    {

        std::list<const Container*> stack;
        stack.push_back(tradeContainer);

        std::list<const Item*> itemstack;
        itemstack.push_back(tradeContainer);

        ContainerList::const_iterator it;

        while(!stack.empty())
        {
            const Container *container = stack.front();
            stack.pop_front();

            for (it = container->getItems(); it != container->getEnd(); ++it)
            {
                Container *container = dynamic_cast<Container*>(*it);
                if(container)
                {
                    stack.push_back(container);
                }

                itemstack.push_back(*it);
            }
        }

        msg.AddByte((unsigned char)itemstack.size());
        while(!itemstack.empty())
        {
            const Item* item = itemstack.front();
            itemstack.pop_front();
            msg.AddItem(item);
        }
    }
    else
    {
        msg.AddByte(1);
        msg.AddItem(item);
    }

    WriteBuffer(msg);
}

void Protocol76::sendCloseTrade()
{
    NetworkMessage msg;
    msg.AddByte(0x7F);

    WriteBuffer(msg);
}

void Protocol76::sendCloseContainer(unsigned char containerid)
{
    NetworkMessage msg;

    msg.AddByte(0x6F);
    msg.AddByte(containerid);
    WriteBuffer(msg);
}

void Protocol76::parseUseItem(NetworkMessage &msg)
{
    Position pos = msg.GetPosition();
    uint16_t itemid = msg.GetItemId();
    unsigned char stack = msg.GetByte();
    unsigned char index = msg.GetByte();

    game->playerUseItem(player, pos, stack, itemid, index);
}

void Protocol76::parseCloseContainer(NetworkMessage &msg)
{
    unsigned char containerid = msg.GetByte();
    player->closeContainer(containerid);
    sendCloseContainer(containerid);
}

void Protocol76::parseUpArrowContainer(NetworkMessage &msg)
{
    unsigned char containerid = msg.GetByte();
    OTSYS_THREAD_LOCK_CLASS lockClass(game->gameLock, "Protocol76::parseUpArrowContainer()");
    Container *container = player->getContainer(containerid);
    if(!container)
        return;

    Container *parentcontainer = container->getParent();
    if(parentcontainer)
    {
        sendContainer(containerid, parentcontainer);
    }
}

void Protocol76::parseThrow(NetworkMessage &msg)
{
    uint16_t from_x = msg.GetU16();
    uint16_t from_y = msg.GetU16();
    unsigned char from_z = msg.GetByte();
    uint16_t itemid = msg.GetItemId();
    unsigned char from_stack = msg.GetByte();
    uint16_t to_x = msg.GetU16();
    uint16_t to_y = msg.GetU16();
    unsigned char to_z = msg.GetByte();
    unsigned char count1 = msg.GetByte();
    unsigned char count = count1;
    /*
    std::cout << "parseThrow: " << "from_x: " << (int32_t)from_x << ", from_y: " << (int32_t)from_y
    << ", from_z: " << (int32_t)from_z << ", item: " << (int32_t)itemid << ", from_stack: "
    << (int32_t)from_stack << " to_x: " << (int32_t)to_x << ", to_y: " << (int32_t)to_y
    << ", to_z: " << (int32_t)to_z
    << ", count: " << (int32_t)count << std::endl;*/
    bool toInventory = false;
    bool fromInventory = false;

    if(from_x == to_x && from_y == to_y && from_z == to_z)
        return;
    Tile *t = game->getTile(to_x,to_y,to_z);
    if(player->access < g_config.ACCESS_REMOTE && t && (t->hasItem(ITEM_DEPO1) || t->hasItem(ITEM_DEPO2) || t->hasItem(ITEM_DEPO3) || t->hasItem(ITEM_DEPO4)) &&
            ((abs(player->pos.x - to_x) > g_config.ODLEGLOSC_OD_DEPO) || (abs(player->pos.y - to_y) > g_config.ODLEGLOSC_OD_DEPO) || (player->pos.z != to_z)))
    {
        player->sendCancel("Przykro mi, to nie mozliwe. :)");
        return;
    }
    if(count1 == 0)
    {
        player->sendCancel("What are you doing dipstick?");
        time_t ticks = time(0);
        char buf[80];
#ifdef USING_VISUAL_2005
        tm now;
        localtime_s(&now, &ticks);
        strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M", &now);
#else
        strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M", localtime(&ticks));
#endif //USING_VISUAL_2005
        player->sendTextMessage(MSG_RED_TEXT,"You have been banned for packet editing PWNED.");
        game->banPlayer(player, "Edytowanie Pakietow", "AccountBan+FinalWarning", "2", 0);
        if(g_config.BANMSG)
            std::cout << "::" << player->getName() <<" Was just banned" << std::endl;

        std::ofstream out("data/logs/klonowanie.log", std::ios::app);
        out << "[" << buf << "] "<< "BANNED for attempted packet editing" << ": " << player->getName() << std::endl;
        out.close();
        std::stringstream fullMessage;
        fullMessage << player->getName() << " zostal zbanowany za edytowanie pakietow." <<std::endl; //Message

        for(AutoList<Player>::listiterator it = Player::listPlayer.list.begin(); it != Player::listPlayer.list.end(); ++it)
        {
            if(dynamic_cast<Player*>(it->second))
                (*it).second->sendTextMessage(MSG_INFO, fullMessage.str().c_str());
        }
        return;
    }
#ifdef HUCZU_PARCEL_SYSTEM
    if(to_x != 0xFFFF && (itemid == ITEM_PARCEL || itemid == ITEM_LETTER))
    {
        Tile* t = game->getTile(to_x, to_y, to_z);
        if(t && (t->hasItem(ITEM_MAILBOX1) || t->hasItem(ITEM_MAILBOX2)))
        {
            Item* parcel = dynamic_cast<Item*>(game->getThing(Position(from_x, from_y, from_z), from_stack, player));
            Position pozycja;
            pozycja.x = from_x;
            pozycja.y = from_y;
            pozycja.z = from_z;
            if(parcel)
                game->sendParcel(player, parcel, pozycja);
        }
    }
#endif

//container/inventory to container/inventory
    if(from_x == 0xFFFF && to_x == 0xFFFF)
    {
        unsigned char from_cid;
        unsigned char to_cid;

        if(from_y & 0x40)
            from_cid = from_y & 0x0F;
        else
        {
            fromInventory = true;
            from_cid = static_cast<unsigned char>(from_y);
        }

        if(to_y & 0x40)
            to_cid = static_cast<unsigned char>(to_y & 0x0F);
        else
        {
            toInventory = true;
            to_cid = static_cast<unsigned char>(to_y);
        }

        game->thingMove(player, from_cid, from_z, itemid,fromInventory, to_cid, to_z, toInventory, count);
    }
//container/inventory to ground
    else if(from_x == 0xFFFF && to_x != 0xFFFF)
    {
        unsigned char from_cid;

        if(0x40 & from_y)
            from_cid = static_cast<unsigned char>(from_y & 0x0F);
        else
        {
            fromInventory = true;
            from_cid = static_cast<unsigned char>(from_y);
        }

        game->thingMove(player, from_cid, from_z, itemid, fromInventory, Position(to_x, to_y, to_z), count);
    }
//ground to container/inventory
    else if(from_x != 0xFFFF && to_x == 0xFFFF)
    {
        unsigned char to_cid;

        if(0x40 & to_y)
            to_cid = static_cast<unsigned char>(to_y & 0x0F);
        else
        {
            toInventory = true;
            to_cid = static_cast<unsigned char>(to_y);

            if(to_cid > 11)
                return;

            if(to_cid == 0)
                return;

        }

        game->thingMove(player, Position(from_x, from_y, from_z), from_stack, itemid, to_cid, to_z, toInventory, count);
    }
//ground to ground
    else
    {
        Tile *fromTile = game->getTile(from_x, from_y, from_z);
        if(!fromTile)
            return;

        Creature *movingCreature = dynamic_cast<Creature*>(fromTile->getThingByStackPos(from_stack));
        if(movingCreature)
        {
            Player *movingPlayer = dynamic_cast<Player*>(movingCreature);
            if(player == movingPlayer)
                this->sleepTillMove();
        }

        game->thingMove(player, from_x, from_y, from_z, from_stack, itemid, to_x, to_y, to_z, count);
    }
}

void Protocol76::parseLookAt(NetworkMessage &msg)
{
    Position LookPos = msg.GetPosition();
    uint16_t ItemNum = msg.GetU16();
    unsigned char stackpos = msg.GetByte();

#ifdef __DEBUG__
    std::cout << "look at: " << LookPos << std::endl;
    std::cout << "itemnum: " << ItemNum << " stackpos: " << (int32_t)stackpos<< std::endl;
#endif

    NetworkMessage newmsg;
    std::stringstream ss;

    /*
    #ifdef __DEBUG__
    ss << "You look at " << LookPos << " and see Item # " << ItemNum << ".";
    AddTextMessage(newmsg,MSG_INFO, ss.str().c_str());
    #else
    */
    Item *item = NULL;
    Creature *creature = NULL;
    OTSYS_THREAD_LOCK_CLASS lockClass(game->gameLock, "Protocol76::parseLookAt()");

    if(LookPos.x != 0xFFFF)
    {
        Tile* tile = game->getTile(LookPos.x, LookPos.y, LookPos.z);
        if(tile)
        {
            item = dynamic_cast<Item*>(tile->getTopThing());
            creature = dynamic_cast<Creature*>(tile->getTopThing());
        }
    }
    else
    {
        //from container/inventory
        if(LookPos.y & 0x40)
        {
            unsigned char from_cid = LookPos.y & 0x0F;
            unsigned char slot = LookPos.z;

            Container *parentcontainer = player->getContainer(from_cid);
            if(!parentcontainer)
                return;

            item = parentcontainer->getItem(slot);
        }
        else
        {
            unsigned char from_cid = static_cast<unsigned char>(LookPos.y);
            item = player->getItem(from_cid);
        }
    }

    if(item)
    {
#ifdef TLM_HOUSE_SYSTEM
        Tile* doorTile = game->getTile(LookPos);
        Thing* doorThing = doorTile? doorTile->getTopThing() : NULL;
        Item* doorItem = doorThing? dynamic_cast<Item*>(doorThing) : NULL;

        if (doorItem && Item::items[doorItem->getID()].isDoor && doorTile && doorTile->getHouse())
            AddTextMessage(newmsg, MSG_INFO, doorTile->getHouse()->getDescription().c_str());
        else
#endif //TLM_HOUSE_SYSTEM
        {
            bool fullDescription = false;
            if(LookPos.x == 0xFFFF)
            {
                fullDescription = true;
            }
            else if(std::abs(player->pos.x - LookPos.x) <= 1 && std::abs(player->pos.y - LookPos.y) <= 1 &&
                    LookPos.z == player->pos.z)
            {
                fullDescription = true;
            }

            std::stringstream ss;
            ss << "You see " << item->getDescription(fullDescription);
            if (player->access >= g_config.ACCESS_LOOK)
                ss << "\nId: " << Item::items.reverseLookUp(ItemNum)
                   << ". Pos: " << LookPos.x << ' ' << LookPos.y << ' ' << LookPos.z << '.';

            AddTextMessage(newmsg,MSG_INFO, ss.str().c_str());
        }
    }
    else if(creature)
    {
        if(player == creature)
        {
            std::stringstream ss;
            ss << "You see " << creature->getDescription(true);
            AddTextMessage(newmsg,MSG_INFO, ss.str().c_str());
        }
#ifdef _BBK_GM_INVISIBLE
        else
        {
            Player* gm = dynamic_cast<Player*>(creature);
            if(gm && gm->gmInvisible && player->access < g_config.ACCESS_PROTECT)
            {
                std::stringstream ss;
                Tile* tile = game->getTile(gm->pos);
                if(tile)
                {
                    Item *item = tile->getTopTopItem();
                    if(item)
                    {
                        ss << "You see " << item->getDescription(true);
                    }
                    else
                    {
                        Item *thing = dynamic_cast<Item*>(tile->getTopDownItem());
                        if(thing)
                            ss << "You see " << thing->getDescription(true);
                        else
                            ss << "You see " << tile->ground->getDescription(true);
                    }
                }
                else
                    ss << "Yyy..";

                AddTextMessage(newmsg,MSG_INFO, ss.str().c_str());
            }
            else
            {
                std::stringstream ss;
                ss << "You see " << creature->getDescription().c_str();
                AddTextMessage(newmsg,MSG_INFO, ss.str().c_str());
            }
        }
    }
#else
        else
        {
            std::stringstream ss;
            ss << "You see " << creature->getDescription().c_str();
            AddTextMessage(newmsg,MSG_INFO, ss.str().c_str());
        }
    }
#endif //_BBK_GM_INVISIBLE

    sendNetworkMessage(&newmsg);
}



void Protocol76::parseSay(NetworkMessage &msg)
{
    SpeakClasses type = (SpeakClasses)msg.GetByte();

    std::string receiver;
    uint16_t channelId = 0;
    if(type == SPEAK_PRIVATE ||
#ifdef HUCZU_RRV
            type == SPEAK_CHANNEL_RV2 ||
#endif
            type == SPEAK_PRIVATE_RED)
        receiver = msg.GetString();
    if(type == SPEAK_CHANNEL_Y ||
            type == SPEAK_CHANNEL_R1 ||
            type == SPEAK_CHANNEL_R2)
        channelId = msg.GetU16();

    std::string text = msg.GetString();

    if(game->creatureSaySpell(player, text))
        type = SPEAK_MONSTER1;

    switch (type)
    {
    case SPEAK_MONSTER1:
    case SPEAK_SAY:
#ifdef __MIZIAK_TALKACTIONS__
        if(!actions.SayTalk(player, text))
#endif //__MIZIAK_TALKACTIONS__
            game->creatureSay(player, type, text);
#ifdef __MIZIAK_TALKACTIONS__
        else
            player->sendTextMessage(MSG_RED_INFO, text.c_str());
#endif //__MIZIAK_TALKACTIONS__
        break;
    case SPEAK_WHISPER:
        game->creatureWhisper(player, text);
        break;
    case SPEAK_YELL:
        game->creatureYell(player, text);
        break;
    case SPEAK_PRIVATE:
    case SPEAK_PRIVATE_RED:
        game->creatureSpeakTo(player, type, receiver, text);
        break;
    case SPEAK_CHANNEL_Y:
    case SPEAK_CHANNEL_R1:
    case SPEAK_CHANNEL_R2:
    case SPEAK_CHANNEL_O:
        game->creatureTalkToChannel(player, type, text, channelId);
        break;
    case SPEAK_BROADCAST:
        game->creatureBroadcastMessage(player, text);
        break;
#ifdef HUCZU_RRV
    case SPEAK_CHANNEL_RV1:
        addViolation(player, text);
        break;
    case SPEAK_CHANNEL_RV2:
        speakToReporter(receiver, text);
        break;
    case SPEAK_CHANNEL_RV3:
        speakToCounsellor(text);
        break;
#endif
    }
}

#ifdef HUCZU_FOLLOW
void Protocol76::parseAttack(NetworkMessage &msg)
{
    uint32_t creatureid = msg.GetU32();
    if(game && player)
    {
        game->playerSetAttackedCreature(player, creatureid);
        if(player->followMode != 0)//{
            game->playerSetFollowCreature(player, creatureid);
        /*  }else
             game->playerSetFollowCreature(player, 0);*/
    }
}
#else
void Protocol76::parseAttack(NetworkMessage &msg)
{
    uint32_t creatureid = msg.GetU32();
    game->playerSetAttackedCreature(player, creatureid);
}
#endif //HUCZU_FOLLOW

#ifdef HUCZU_FOLLOW
void Protocol76::parseFollow(NetworkMessage &msg)
{
    uint32_t creatureid = msg.GetU32();
    game->playerSetFollowCreature(player, creatureid);
}
#endif //HUCZU_FOLLOW

void Protocol76::parseTextWindow(NetworkMessage &msg)
{
    uint32_t id = msg.GetU32();
    std::string new_text = msg.GetString();
    if(readItem && windowTextID == id)
    {
        readItem->setText(new_text);
        readItem->setWriter(player->getName());
        readItem->releaseThing();
        readItem = NULL;
    }
}

void Protocol76::parseRequestTrade(NetworkMessage &msg)
{
    Position pos = msg.GetPosition();
    uint16_t itemid = msg.GetItemId();
    unsigned char stack = msg.GetByte();
    uint32_t playerid = msg.GetU32();

    game->playerRequestTrade(player, pos, stack, itemid, playerid);
}

void Protocol76::parseAcceptTrade(NetworkMessage &msg)
{
    game->playerAcceptTrade(player);
}

void Protocol76::parseLookInTrade(NetworkMessage &msg)
{
    bool counterOffer = (msg.GetByte() == 0x01);
    int32_t index = msg.GetByte();

    game->playerLookInTrade(player, counterOffer, index);
}

void Protocol76::parseCloseTrade()
{
    game->playerCloseTrade(player);
}

void Protocol76::parseRotateItem(NetworkMessage &msg)
{
    Position pos = msg.GetPosition();
    uint16_t itemid = msg.GetItemId();
    unsigned char stackpos = msg.GetByte();

    game->playerRotateItem(player, pos, stackpos, itemid);

}

bool Protocol76::CanSee(int32_t x, int32_t y, int32_t z) const
{
#ifdef __DEBUG__
    if(z < 0 || z >= MAP_LAYER)
    {
        std::cout << "WARNING! Protocol76::CanSee() Z-value is out of range!" << std::endl;
    }
#endif

    /*underground 8->15*/
    if(player->pos.z > 7 && z < 6 /*8 - 2*/)
    {
        return false;
    }
    /*ground level and above 7->0*/
    else if(player->pos.z <= 7 && z > 7)
    {
        return false;
    }

    //negative offset means that the action taken place is on a lower floor than ourself
    int32_t offsetz = player->pos.z - z;

    if ((x >= player->pos.x - 8 + offsetz) && (x <= player->pos.x + 9 + offsetz) &&
            (y >= player->pos.y - 6 + offsetz) && (y <= player->pos.y + 7 + offsetz))
        return true;

    return false;
}

bool Protocol76::CanSee(const Creature *c) const
{
    if(c->isRemoved == true)
        return false;

    return CanSee(c->pos.x, c->pos.y, c->pos.z);
}

void Protocol76::sendNetworkMessage(NetworkMessage *msg)
{
    WriteBuffer(*msg);
}

void Protocol76::AddTileUpdated(NetworkMessage &msg, const Position &pos)
{
#ifdef __DEBUG__
    std::cout << "Pop-up item from below..." << std::endl;
#endif

    if (CanSee(pos.x, pos.y, pos.z))
    {
        msg.AddByte(0x69);
        msg.AddPosition(pos);

        Tile* tile = game->getTile(pos.x, pos.y, pos.z);
        if(tile)
        {
#ifdef TRS_GM_INVISIBLE
            GetTileDescription(tile, msg, player);
#else
            GetTileDescription(tile, msg);
#endif //TRS_GM_INVISIBLE
            msg.AddByte(0);
            msg.AddByte(0xFF);
        }
        else
        {
            msg.AddByte(0x01);
            msg.AddByte(0xFF);
        }
    }
}

void Protocol76::sendTileUpdated(const Position &pos)
{
    NetworkMessage msg;

    AddTileUpdated(msg, pos);
    WriteBuffer(msg);
}

//container to container
void Protocol76::sendThingMove(const Creature *creature, const Container *fromContainer, uint16_t from_slotid,
                               const Item* fromItem, int32_t oldFromCount, Container *toContainer, uint16_t to_slotid, const Item *toItem, int32_t oldToCount, int32_t count)
{
    if(player->NeedUpdateStats())
    {
        player->sendStats();
    }

    NetworkMessage msg;

    //Auto-close container's
    const Container* moveContainer = dynamic_cast<const Container*>(fromItem);
    if(moveContainer)
    {
        bool hasToContainerOpen = false;
        Position toMapPos = toContainer->getTopParent()->pos;

        if(toMapPos.x != 0xFFFF && std::abs(player->pos.x - toMapPos.x) <= 1 && std::abs(player->pos.y - toMapPos.y) <= 1)
        {
            for(containerLayout::const_iterator cit = player->getContainers(); cit != player->getEndContainer(); ++cit)
            {

                bool skipContainer = false;
                Container* tmpcontainer = cit->second;
                while(tmpcontainer != NULL)
                {
                    if(tmpcontainer == moveContainer)
                    {
                        skipContainer = true;
                        break;
                    }

                    tmpcontainer = tmpcontainer->getParent();
                }

                if(skipContainer)
                    continue;

                if(cit->second == toContainer || cit->second->getTopParent()->isHoldingItem(toContainer))
                {
                    hasToContainerOpen = true;
                    break;
                }
            }
        }

        if(!hasToContainerOpen && !player->isHoldingContainer(toContainer))
        {
            autoCloseContainers(moveContainer, msg);
        }
    }

    Item *container = NULL;
    for(containerLayout::const_iterator cit = player->getContainers(); cit != player->getEndContainer(); ++cit)
    {
        container  = cit->second;
        unsigned char cid = cit->first;

        if(container && container == fromContainer)
        {
            if(toContainer == fromContainer)
            {
                if(fromItem->isStackable())
                {
                    if(toItem && fromItem->getID() == toItem->getID() && toItem->getItemCountOrSubtype() != oldToCount)
                    {
                        //update count
                        TransformItemContainer(msg,cid,to_slotid,toItem);

                        if(fromItem->getItemCountOrSubtype() > 0 && count != oldFromCount)
                        {
                            //update count
                            TransformItemContainer(msg,cid,from_slotid,fromItem);
                        }
                        else
                        {
                            //remove item
                            RemoveItemContainer(msg,cid,from_slotid);
                        }

                        //surplus items
                        if(oldToCount + count > 100)
                        {
                            //add item
                            AddItemContainer(msg,cid,fromItem,oldToCount + count - 100);
                        }
                    }
                    else
                    {
                        if(count == oldFromCount)
                        {
                            //remove item
                            RemoveItemContainer(msg,cid,from_slotid);
                        }
                        else
                        {
                            //update count
                            TransformItemContainer(msg,cid,from_slotid,fromItem);
                        }

                        //add item
                        AddItemContainer(msg,cid,fromItem,count);
                    }
                }
                else
                {
                    //remove item
                    RemoveItemContainer(msg,cid,from_slotid);

                    //add item
                    AddItemContainer(msg,cid,fromItem);
                }
            }
            else
            {
                if(fromItem->isStackable() && fromItem->getItemCountOrSubtype() > 0 && count != oldFromCount)
                {
                    //update count
                    TransformItemContainer(msg,cid,from_slotid,fromItem);
                }
                else
                {
                    //remove item
                    RemoveItemContainer(msg,cid,from_slotid);
                }
            }
        }

        if(container && container == toContainer && toContainer != fromContainer)
        {
            if(fromItem->isStackable())
            {
                if(toItem && fromItem->getID() == toItem->getID() && toItem->getItemCountOrSubtype() != oldToCount)
                {
                    //update count
                    TransformItemContainer(msg,cid,to_slotid,toItem);

                    //surplus items
                    if(oldToCount + count > 100)
                    {
                        //add item
                        AddItemContainer(msg,cid,fromItem,oldToCount + count - 100);
                    }
                }
                else
                {
                    //add item
                    AddItemContainer(msg,cid,fromItem,count);
                }
            }
            else
            {
                //add item
                AddItemContainer(msg,cid,fromItem);
            }
        }
    }

    WriteBuffer(msg);
}

//inventory to container
void Protocol76::sendThingMove(const Creature *creature, slots_t fromSlot, const Item* fromItem,
                               int32_t oldFromCount, const Container *toContainer, uint16_t to_slotid, const Item *toItem, int32_t oldToCount, int32_t count)
{
    if(player->NeedUpdateStats())
    {
        player->sendStats();
    }

    NetworkMessage msg;

    for(containerLayout::const_iterator cit = player->getContainers(); cit != player->getEndContainer(); ++cit)
    {
        if(cit->second == toContainer)
        {
            unsigned char cid = cit->first;

            if(fromItem->isStackable())
            {
                if(toItem && fromItem->getID() == toItem->getID() && toItem->getItemCountOrSubtype() != oldToCount)
                {
                    //update count
                    TransformItemContainer(msg,cid,to_slotid,toItem);

                    //surplus items
                    if(oldToCount + count > 100)
                    {
                        //add item
                        AddItemContainer(msg,cid,fromItem,oldToCount + count - 100);
                    }
                }
                else
                {
                    //add item
                    AddItemContainer(msg,cid,fromItem,count);
                }
            }
            else
            {
                //add item
                AddItemContainer(msg,cid,fromItem);
            }
        }
    }

    if(creature == player)
    {
        AddPlayerInventoryItem(msg,player, fromSlot);

        //Update up-arrow
        const Container* moveContainer = dynamic_cast<const Container*>(fromItem);
        if(moveContainer)
        {
            for(containerLayout::const_iterator cit = player->getContainers(); cit != player->getEndContainer(); ++cit)
            {
                if(cit->second == moveContainer)
                {
                    sendContainer(cit->first, cit->second);
                }
            }
        }
        //
    }

    WriteBuffer(msg);
}

//inventory to inventory
void Protocol76::sendThingMove(const Creature *creature, slots_t fromSlot, const Item* fromItem,
                               int32_t oldFromCount, slots_t toSlot, const Item* toItem, int32_t oldToCount, int32_t count)
{
    NetworkMessage msg;
    if(creature == player)
    {
        if(player->NeedUpdateStats())
        {
            player->sendStats();
        }

        AddPlayerInventoryItem(msg, player, fromSlot);
        AddPlayerInventoryItem(msg, player, toSlot);
    }

    WriteBuffer(msg);
}

//container to inventory
void Protocol76::sendThingMove(const Creature *creature, const Container *fromContainer,
                               uint16_t from_slotid, const Item* fromItem, int32_t oldFromCount, slots_t toSlot, const Item *toItem, int32_t oldToCount, int32_t count)
{
    if(player->NeedUpdateStats())
    {
        player->sendStats();
    }

    NetworkMessage msg;

    for(containerLayout::const_iterator cit = player->getContainers(); cit != player->getEndContainer(); ++cit)
    {
        if(cit->second == fromContainer)
        {
            unsigned char cid = cit->first;

            if(!fromItem->isStackable() || (oldFromCount == count && oldToCount + count <= 100))
            {
                //remove item
                RemoveItemContainer(msg,cid,from_slotid);
            }
            else
            {
                //update count
                TransformItemContainer(msg,cid,from_slotid,fromItem);
            }

            if(toItem && toItem->getID() != fromItem->getID())
            {
                //add item
                AddItemContainer(msg,cid,toItem);
            }
        }
    }

    if(creature == player)
    {
        AddPlayerInventoryItem(msg,player, toSlot);

        //Update up-arrow
        const Container* moveContainer = dynamic_cast<const Container*>(fromItem);
        if(moveContainer)
        {
            for(containerLayout::const_iterator cit = player->getContainers(); cit != player->getEndContainer(); ++cit)
            {
                if(cit->second == moveContainer)
                {
                    sendContainer(cit->first, cit->second);
                }
            }
        }
        //
    }
    else
    {
        const Container* moveContainer = dynamic_cast<const Container*>(fromItem);
        if(moveContainer)
        {
            autoCloseContainers(moveContainer, msg);
        }
    }

    WriteBuffer(msg);
}

//container to ground
void Protocol76::sendThingMove(const Creature *creature, const Container *fromContainer, unsigned char from_slotid,
                               const Item* fromItem, int32_t oldFromCount, const Position &toPos, const Item *toItem, int32_t oldToCount, int32_t count)
{
    if(player->NeedUpdateStats())
    {
        player->sendStats();
    }

    NetworkMessage msg;

    //Auto-close container's
    const Container* moveContainer = dynamic_cast<const Container*>(fromItem);
    bool updateContainerArrow = false;

    if(moveContainer)
    {
        //Auto-close container's
        if(std::abs(player->pos.x - toPos.x) > 1 || std::abs(player->pos.y - toPos.y) > 1)
        {
            autoCloseContainers(moveContainer, msg);
        }
        else
            updateContainerArrow = true;
    }

    if(CanSee(toPos.x, toPos.y, toPos.z))
    {
        if(toItem && toItem->getID() == fromItem->getID() && fromItem->isStackable() && toItem->getItemCountOrSubtype() != oldToCount)
        {
            AddTileUpdated(msg, toItem->pos);
        }
        else
        {
            AddAppearThing(msg, toPos);
            if(fromItem->isStackable())
            {
                msg.AddItem(fromItem->getID(), count);
            }
            else
                msg.AddItem(fromItem);
        }
    }

    for(containerLayout::const_iterator cit = player->getContainers(); cit != player->getEndContainer(); ++cit)
    {
        unsigned char cid = cit->first;
        if(cit->second == fromContainer)
        {

            if(!fromItem->isStackable() || fromItem->getItemCountOrSubtype() == 0 || count == oldFromCount)
            {
                //remove item
                RemoveItemContainer(msg,cid,from_slotid);
            }
            else
            {
                //update count
                TransformItemContainer(msg,cid,from_slotid,fromItem);
            }
        }
    }

    //Update up-arrow
    if(updateContainerArrow)
    {
        const Container* moveContainer = dynamic_cast<const Container*>(fromItem);
        if(moveContainer)
        {
            for(containerLayout::const_iterator cit = player->getContainers(); cit != player->getEndContainer(); ++cit)
            {
                if(cit->second == moveContainer)
                {
                    sendContainer(cit->first, cit->second);
                }
            }
        }
    }
    //

    WriteBuffer(msg);
}

//inventory to ground
void Protocol76::sendThingMove(const Creature *creature, slots_t fromSlot,
                               const Item* fromItem, int32_t oldFromCount, const Position &toPos, const Item *toItem, int32_t oldToCount, int32_t count)
{
    NetworkMessage msg;

    if(creature == player)
    {
        if(player->NeedUpdateStats())
        {
            player->sendStats();
        }

        //Auto-closing containers
        const Container* moveContainer = dynamic_cast<const Container*>(fromItem);
        if(moveContainer)
        {
            //Auto-close container's
            if(std::abs(player->pos.x - toPos.x) > 1 || std::abs(player->pos.y - toPos.y) > 1)
            {
                autoCloseContainers(moveContainer, msg);
            }
        }
    }

    if(CanSee(toPos.x, toPos.y, toPos.z))
    {
        if(toItem && toItem->getID() == fromItem->getID() && fromItem->isStackable() && toItem->getItemCountOrSubtype() != oldToCount)
        {
            AddTileUpdated(msg, toItem->pos);
        }
        else
        {
            AddAppearThing(msg, toPos);
            if(fromItem->isStackable())
            {
                msg.AddItem(fromItem->getID(), count);
            }
            else
                msg.AddItem(fromItem);
        }
    }

    if(creature == player)
    {
        AddPlayerInventoryItem(msg, player, fromSlot);
    }

    WriteBuffer(msg);
}

//ground to container
void Protocol76::sendThingMove(const Creature *creature, const Position &fromPos, int32_t stackpos, const Item* fromItem,
                               int32_t oldFromCount, const Container *toContainer, unsigned char to_slotid, const Item *toItem, int32_t oldToCount, int32_t count)
{
    if(player->NeedUpdateStats())
    {
        player->sendStats();
    }

    NetworkMessage msg;

    //Auto-closing containers
    const Container* moveContainer = dynamic_cast<const Container*>(fromItem);
    bool updateContainerArrow = false;
    if(moveContainer)
    {
        bool hasToContainerOpen = false;
        Position toMapPos = toContainer->getTopParent()->pos;

        if(toMapPos.x != 0xFFFF && std::abs(player->pos.x - toMapPos.x) <= 1 && std::abs(player->pos.y - toMapPos.y) <= 1)
        {
            for(containerLayout::const_iterator cit = player->getContainers(); cit != player->getEndContainer(); ++cit)
            {

                bool skipContainer = false;
                Container* tmpcontainer = cit->second;
                while(tmpcontainer != NULL)
                {
                    if(tmpcontainer == moveContainer)
                    {
                        skipContainer = true;
                        break;
                    }

                    tmpcontainer = tmpcontainer->getParent();
                }

                if(skipContainer)
                    continue;

                if(cit->second == toContainer || cit->second->getTopParent()->isHoldingItem(toContainer))
                {
                    hasToContainerOpen = true;
                    break;
                }
            }
        }

        if(!hasToContainerOpen && !player->isHoldingContainer(toContainer))
        {
            autoCloseContainers(moveContainer, msg);
        }
        else
            updateContainerArrow = true;
    }
    //

    if(CanSee(fromPos.x, fromPos.y, fromPos.z))
    {
        if(!fromItem->isStackable() || (oldFromCount == count && oldToCount + count <= 100))
        {
            AddRemoveThing(msg, fromPos, stackpos);
        }
        else
            AddTileUpdated(msg, fromPos);
    }

    Container *container = NULL;
    for(containerLayout::const_iterator cit = player->getContainers(); cit != player->getEndContainer(); ++cit)
    {
        container = cit->second;
        if(container == toContainer)
        {
            unsigned char cid = cit->first;

            if(fromItem->isStackable())
            {
                if(toItem && fromItem->getID() == toItem->getID() && toItem->getItemCountOrSubtype() != oldToCount)
                {
                    //update count
                    TransformItemContainer(msg,cid,to_slotid,toItem);

                    //surplus items
                    if(oldToCount + count > 100)
                    {
                        //add item
                        AddItemContainer(msg,cid,fromItem,oldToCount + count - 100);
                    }
                }
                else
                {
                    //add item
                    AddItemContainer(msg,cid,fromItem,count);
                }
            }
            else
            {
                //add item
                AddItemContainer(msg,cid,fromItem);
            }
        }
    }

    if(updateContainerArrow)
    {
        const Container* moveContainer = dynamic_cast<const Container*>(fromItem);
        if(moveContainer)
        {
            for(containerLayout::const_iterator cit = player->getContainers(); cit != player->getEndContainer(); ++cit)
            {

                if(cit->second == moveContainer)
                {
                    sendContainer(cit->first, cit->second);
                }
            }
        }
    }

    WriteBuffer(msg);
}

//ground to inventory
void Protocol76::sendThingMove(const Creature *creature, const Position &fromPos, int32_t stackpos, const Item* fromItem,
                               int32_t oldFromCount, slots_t toSlot, const Item *toItem, int32_t oldToCount, int32_t count)
{
    if(player->NeedUpdateStats())
    {
        player->sendStats();
    }

    NetworkMessage msg;

    if(creature == player)
    {
        AddPlayerInventoryItem(msg, player, toSlot);
    }
    else
    {
        //Auto-closing containers
        const Container* moveContainer = dynamic_cast<const Container*>(fromItem);
        if(moveContainer)
        {
            autoCloseContainers(moveContainer, msg);
        }
    }

    if(CanSee(fromPos.x, fromPos.y, fromPos.z))
    {
        if(!fromItem->isStackable() || (oldFromCount == count && oldToCount + count <= 100))
        {
            AddRemoveThing(msg, fromPos, stackpos);

            if(toItem && (!toItem->isStackable() || toItem->getID() != fromItem->getID()))
            {
                AddAppearThing(msg, fromPos);
                msg.AddItem(toItem);
            }
        }
        else
            AddTileUpdated(msg, fromPos);
    }

    WriteBuffer(msg);
}

//ground to ground
void Protocol76::sendThingMove(const Creature *creature, const Thing *thing,
                               const Position *oldPos, unsigned char oldStackPos, unsigned char oldcount, unsigned char count, bool tele)
{
    NetworkMessage msg;

    const Creature* c = dynamic_cast<const Creature*>(thing);
    if(c && c->isRemoved)
        return;

    if (!tele && c && (CanSee(oldPos->x, oldPos->y, oldPos->z)) && (CanSee(thing->pos.x, thing->pos.y, thing->pos.z)))
    {
        msg.AddByte(0x6D);
        msg.AddPosition(*oldPos);
        msg.AddByte(oldStackPos);
        msg.AddPosition(thing->pos);

        Tile *fromTile = game->getTile(oldPos->x, oldPos->y, oldPos->z);
        if(fromTile && fromTile->getThingCount() > 8)
        {
            //We need to pop up this item
            Thing *newthing = fromTile->getThingByStackPos(9);

            if(newthing != NULL)
            {
                AddTileUpdated(msg, *oldPos);
            }
        }
    }
    else
    {
        if (!tele && CanSee(oldPos->x, oldPos->y, oldPos->z))
        {
            AddRemoveThing(msg,*oldPos,oldStackPos);
        }

        if (!(tele && thing == this->player) && CanSee(thing->pos.x, thing->pos.y, thing->pos.z))
        {
            AddAppearThing(msg,thing->pos);
            if (c)
            {
                bool known;
                uint32_t removedKnown;
                checkCreatureAsKnown(((Creature*)thing)->getID(), known, removedKnown);
                AddCreature(msg,(Creature*)thing, known, removedKnown);
            }
            else
            {
                msg.AddItem((Item*)thing);

                //Auto-close container's
                const Container* moveContainer = dynamic_cast<const Container*>(thing);
                if(moveContainer)
                {
                    if(std::abs(player->pos.x - thing->pos.x) > 1 || std::abs(player->pos.y - thing->pos.y) > 1 || player->pos.z != thing->pos.z )
                    {
                        autoCloseContainers(moveContainer, msg);
                    }
                }
            }
        }
    }

    if (thing == this->player)
    {
#ifdef TRS_GM_INVISIBLE
        if(tele)
        {
            msg.AddByte(0x64);
            msg.AddPosition(player->pos);
            GetMapDescription(player->pos.x-8, player->pos.y-6, player->pos.z, 18, 14, msg, this->player);
        }
        else
        {
            if (oldPos->y > thing->pos.y)   // north, for old x
            {
                msg.AddByte(0x65);
                GetMapDescription(oldPos->x - 8, thing->pos.y - 6, thing->pos.z, 18, 1, msg, this->player);
            }
            else if (oldPos->y < thing->pos.y)     // south, for old x
            {
                msg.AddByte(0x67);
                GetMapDescription(oldPos->x - 8, thing->pos.y + 7, thing->pos.z, 18, 1, msg, this->player);
            }
            if (oldPos->x < thing->pos.x)   // east, [with new y]
            {
                msg.AddByte(0x66);
                GetMapDescription(thing->pos.x + 9, thing->pos.y - 6, thing->pos.z, 1, 14, msg, this->player);
            }
            else if (oldPos->x > thing->pos.x)     // west, [with new y]
            {
                msg.AddByte(0x68);
                GetMapDescription(thing->pos.x - 8, thing->pos.y - 6, thing->pos.z, 1, 14, msg, this->player);
            }
        }
#else //TRS_GM_INVISIBLE
        if(tele)
        {
            msg.AddByte(0x64);
            msg.AddPosition(player->pos);
            GetMapDescription(player->pos.x-8, player->pos.y-6, player->pos.z, 18, 14, msg);
        }
        else
        {
            if (oldPos->y > thing->pos.y)   // north, for old x
            {
                msg.AddByte(0x65);
                GetMapDescription(oldPos->x - 8, thing->pos.y - 6, thing->pos.z, 18, 1, msg);
            }
            else if (oldPos->y < thing->pos.y)     // south, for old x
            {
                msg.AddByte(0x67);
                GetMapDescription(oldPos->x - 8, thing->pos.y + 7, thing->pos.z, 18, 1, msg);
            }
            if (oldPos->x < thing->pos.x)   // east, [with new y]
            {
                msg.AddByte(0x66);
                GetMapDescription(thing->pos.x + 9, thing->pos.y - 6, thing->pos.z, 1, 14, msg);
            }
            else if (oldPos->x > thing->pos.x)     // west, [with new y]
            {
                msg.AddByte(0x68);
                GetMapDescription(thing->pos.x - 8, thing->pos.y - 6, thing->pos.z, 1, 14, msg);
            }
        }
#endif //TRS_GM_INVISIBLE

        //Auto-close container's
        std::vector<Container *> containers;
        for(containerLayout::const_iterator cit = player->getContainers(); cit != player->getEndContainer(); ++cit)
        {
            Container *container = cit->second;

            while(container->getParent())
            {
                container = container->getParent();
            }

            //Only add those we need to close
            if(container->pos.x != 0xFFFF)
            {
                if(std::abs(player->pos.x - container->pos.x) > 1 || std::abs(player->pos.y - container->pos.y) > 1 || player->pos.z != container->pos.z)
                {
                    containers.push_back(cit->second);
                }
            }
        }

        for(std::vector<Container *>::const_iterator it = containers.begin(); it != containers.end(); ++it)
        {
            autoCloseContainers(*it, msg);
        }
    }

    WriteBuffer(msg);
}


//close container and its child containers
void Protocol76::autoCloseContainers(const Container *container, NetworkMessage &msg)
{
    std::vector<unsigned char> containerlist;
    for(containerLayout::const_iterator cit = player->getContainers(); cit != player->getEndContainer(); ++cit)
    {
        Container *tmpcontainer = cit->second;
        while(tmpcontainer != NULL)
        {
            if(tmpcontainer == container)
            {
                containerlist.push_back(cit->first);
                break;
            }

            tmpcontainer = tmpcontainer->getParent();
        }
    }

    for(std::vector<unsigned char>::iterator it = containerlist.begin(); it != containerlist.end(); ++it)
    {
        player->closeContainer(*it);
        msg.AddByte(0x6F);
        msg.AddByte(*it);
    }
}

void Protocol76::sendCreatureTurn(const Creature *creature, unsigned char stackPos)
{
    if (CanSee(creature))
    {
        NetworkMessage msg;

        msg.AddByte(0x6B);
        msg.AddPosition(creature->pos);
        msg.AddByte(stackPos);

        msg.AddByte(0x63);
        msg.AddByte(0x00);
        msg.AddU32(creature->getID());
        msg.AddByte(creature->getDirection());
        WriteBuffer(msg);
    }
}


void Protocol76::sendCreatureSay(const Creature *creature, SpeakClasses type, const std::string &text)
{
    if(text == player->msgB)
        return;
    else
        player->msgB = text;

    NetworkMessage msg;
    AddCreatureSpeak(msg,creature, type, text, 0);
    WriteBuffer(msg);
}

void Protocol76::sendToChannel(const Creature * creature, SpeakClasses type, const std::string &text, uint16_t channelId)
{
    if(text == player->msgB)
        return;
    else
        player->msgB = text;

    NetworkMessage msg;
    AddCreatureSpeak(msg,creature, type, text, channelId);
    WriteBuffer(msg);
}
#ifdef HUCZU_SERVER_LOG
void Protocol76::sendFromSystem(SpeakClasses type, const std::string &text)
{
    NetworkMessage newmsg;
    AddSystemSpeak(newmsg, type, text, 0x02);
    WriteBuffer(newmsg);
}

void Protocol76::AddSystemSpeak(NetworkMessage &msg, SpeakClasses type, std::string text, uint16_t channelId)
{
    msg.AddByte(0xAA);
    msg.AddString("System");
    msg.AddByte(type);
    switch(type)
    {
    case SPEAK_SAY:
    case SPEAK_WHISPER:
    case SPEAK_CHANNEL_Y:
    case SPEAK_CHANNEL_R1:
    case SPEAK_CHANNEL_R2:
    case SPEAK_CHANNEL_O:
        msg.AddU16(channelId);
        break;
    }
    msg.AddString(text);
}
#endif
void Protocol76::sendCancel(const char *msg)
{
    NetworkMessage netmsg;
    AddTextMessage(netmsg,MSG_SMALLINFO, msg);
    WriteBuffer(netmsg);
}

void Protocol76::sendCancelAttacking()
{
    NetworkMessage netmsg;
    netmsg.AddByte(0xa3);
    WriteBuffer(netmsg);
}

void Protocol76::sendChangeSpeed(const Creature *creature)
{
    NetworkMessage netmsg;
    netmsg.AddByte(0x8F);

    netmsg.AddU32(creature->getID());
    netmsg.AddU16(creature->getSpeed());
    WriteBuffer(netmsg);
}

void Protocol76::sendCancelWalk()
{
    NetworkMessage netmsg;
    netmsg.AddByte(0xB5);
    netmsg.AddByte(player->getDirection()); // direction
    WriteBuffer(netmsg);
}

void Protocol76::sendThingDisappear(const Thing *thing, unsigned char stackPos, bool tele)
{
    NetworkMessage msg;
    const Creature* creature = dynamic_cast<const Creature*>(thing);
    //Auto-close trade
    if(player->getTradeItem() && dynamic_cast<const Item*>(thing) == player->getTradeItem())
    {
        game->playerCloseTrade(player);
    }

    if(creature && creature->isRemoved)
        return;

    if(!tele)
    {
        if(creature && creature->health > 0)
        {
            const Player* remove_player = dynamic_cast<const Player*>(creature);
            if(remove_player == player)
                return;

            if(CanSee(creature))
            {
                AddMagicEffect(msg,thing->pos, NM_ME_PUFF);
            }
        }
    }

    if(CanSee(thing->pos.x, thing->pos.y, thing->pos.z))
    {
        AddRemoveThing(msg,thing->pos, stackPos);
        if(creature && stackPos > 9)
        {
            AddCreatureHealth(msg,creature);
        }
        Tile *tile = game->getTile(thing->pos.x, thing->pos.y, thing->pos.z);
        if(tile && tile->getThingCount() > 8)
        {
            //We need to pop up this item
            Thing *newthing = tile->getThingByStackPos(9);
            if(newthing != NULL)
            {
                AddTileUpdated(msg, thing->pos);
            }
        }
    }

    WriteBuffer(msg);
}

void Protocol76::sendThingAppear(const Thing *thing)
{
    NetworkMessage msg;
    const Creature* creature = dynamic_cast<const Creature*>(thing);
    if(creature)
    {
        const Player* add_player = dynamic_cast<const Player*>(creature);
        if(add_player == player)
        {
            msg.AddByte(0x0A);
            msg.AddU32(player->getID());

            msg.AddByte(0x32);
            msg.AddByte(0x00);

            msg.AddByte(0x00); //can report bugs 0,1

#ifdef HUCZU_BAN_SYSTEM
            std::stringstream myIP2;
            std::string myIP;
            unsigned char ip[4];
            *(uint32_t*)&ip = player->getIP();
            myIP2 << (uint32_t)ip[0] << "." << (uint32_t)ip[1] <<
                  "." << (uint32_t)ip[2] << "." << (uint32_t)ip[3];
            myIP = myIP2.str();
            if(add_player->access >= 1)
            {
                msg.AddByte(0x0B);
                msg.AddByte(0xFF);
                msg.AddByte(0xFF);
                msg.AddByte(0xFF);
                msg.AddByte(0xFF);
                msg.AddByte(0xFF);
                msg.AddByte(0xFF);
                msg.AddByte(0xFF);
                msg.AddByte(0xFF);
                msg.AddByte(0xFF);
                msg.AddByte(0xFF);
                msg.AddByte(0xFF);
                msg.AddByte(0xFF);
                msg.AddByte(0xFF);
                msg.AddByte(0xFF);
                msg.AddByte(0xFF);
                msg.AddByte(0xFF);
                msg.AddByte(0xFF);
                msg.AddByte(0xFF);
                msg.AddByte(0xFF);
                msg.AddByte(0xFF);
                msg.AddByte(0xFF);
                msg.AddByte(0xFF);
                msg.AddByte(0xFF);
                msg.AddByte(0xFF);
                msg.AddByte(0xFF);
                msg.AddByte(0xFF);
                msg.AddByte(0xFF);
                msg.AddByte(0xFF);
                msg.AddByte(0xFF);
                msg.AddByte(0xFF);
                msg.AddByte(0xFF);
                msg.AddByte(0xFF);
            }
#endif //HUCZU_BAN_SYSTEM

            msg.AddByte(0x64);
            msg.AddPosition(player->pos);
#ifdef TRS_GM_INVISIBLE
            GetMapDescription(player->pos.x-8, player->pos.y-6, player->pos.z, 18, 14, msg, player);
#else
            GetMapDescription(player->pos.x-8, player->pos.y-6, player->pos.z, 18, 14, msg);
#endif //TRS_GM_INVISIBLE
            AddMagicEffect(msg,player->pos, NM_ME_ENERGY_AREA);

            AddPlayerStats(msg,player);

            msg.AddByte(0x82);
#ifdef CVS_DAY_CYCLE
            msg.AddByte(game->getLightLevel());
#else
            msg.AddByte(0x6F); //LIGHT LEVEL
#endif //CVS_DAY_CYCLE
            msg.AddByte(0xD7);//light? (seems constant)

            /*msg.AddByte(0x8d);//8d
              msg.AddU32(player->getID());
              msg.AddByte(0x03);//00
              msg.AddByte(0xd7);//d7
            */

            AddPlayerSkills(msg,player);

            AddPlayerInventoryItem(msg,player, 1);
            AddPlayerInventoryItem(msg,player, 2);
            AddPlayerInventoryItem(msg,player, 3);
            AddPlayerInventoryItem(msg,player, 4);
            AddPlayerInventoryItem(msg,player, 5);
            AddPlayerInventoryItem(msg,player, 6);
            AddPlayerInventoryItem(msg,player, 7);
            AddPlayerInventoryItem(msg,player, 8);
            AddPlayerInventoryItem(msg,player, 9);
            AddPlayerInventoryItem(msg,player, 10);
#ifdef HUCZU_SERVER_LOG
            NetworkMessage newmsg;
            newmsg.AddByte(0xAC);
            newmsg.AddU16(0x02);
            newmsg.AddString("Server Log");
            WriteBuffer(newmsg);
#endif
            AddTextMessage(msg,MSG_EVENT, g_config.LOGIN_MSG.c_str());
            time_t lastlogin = player->getLastLoginSaved();
#ifdef USING_VISUAL_2005
            char buf[128];
            ctime_s(buf, sizeof(buf), &lastlogin);
            std::string data = "Ostatni raz byles online: "
                               data += ctime(&lastlogin);
#else
            std::string data = "Ostatni raz ";
            std::string param = "";
            param += ctime(&lastlogin);
            std::string czasownik = (player->getSex() == PLAYERSEX_FEMALE ? "zalogowalas sie " : "zalogowales sie ");
            std::string D(param,0,3), M(param,4,3), dd(param,8,2), gg(param,11,8), rrrr(param,20,4);
            std::string dzien, miesiac;
            int32_t         dq = atoi(dd.c_str());
            dzien = (D=="Mon"?"w Pon":(D=="Tue"?"we Wt":(D=="Wed"?"w Sr":(D=="Thu"?"w Czw":
                                       (D=="Fri"?"w Pt":(D=="Sat"?"w Sob":(D=="Sun"?"w Niedz":"")))))));
            miesiac = (M=="Jan"?"01":(M=="Feb"?"02":(M=="Mar"?"03":(M=="Apr"?"04":
                                      (M=="May"?"05":(M=="Jun"?"06":(M=="Jul"?"07":(M=="Aug"?"08":
                                              (M=="Sep"?"09":(M=="Oct"?"10":(M=="Nov"?"11":(M=="Dec"?"12":""))))))))))));
            if(dq<=9)
            {
                dd.erase(0,1);
            }
            data = data + czasownik + dzien + ", " + dd + "." + miesiac + "." + rrrr + "r, o godzinie " + gg + ".";
#endif //USING_VISUAL_2005
            WriteBuffer(msg);

            for(VIPListSet::iterator it = player->VIPList.begin(); it != player->VIPList.end(); it++)
            {
                bool online;
                std::string vip_name;

                if(IOPlayerSQL::getInstance()->getNameByGuid((*it), vip_name))
                {
                    online = (game->getPlayerByName(vip_name) != NULL);
                    sendVIP((*it), vip_name, online);
                }
            }

            //force flush
            flushOutputBuffer();
            return;
        }
        else if(CanSee(creature))
        {
            bool known;
            uint32_t removedKnown;
            checkCreatureAsKnown(creature->getID(), known, removedKnown);
            AddAppearThing(msg,creature->pos);
            AddCreature(msg,creature, known, removedKnown);
            // login bubble
            AddMagicEffect(msg,creature->pos, NM_ME_ENERGY_AREA);
        }
    }
    else if(CanSee(thing->pos.x, thing->pos.y, thing->pos.z))
    {
        const Item *item = dynamic_cast<const Item*>(thing);
        if(item)
        {
            Tile *tile = game->getTile(item->pos.x,item->pos.y,item->pos.z);
            if(tile->getThingCount() > 8)
            {
                AddTileUpdated(msg,item->pos);
            }
            else
            {
                AddAppearThing(msg,item->pos);
                msg.AddItem(item);
            }
        }
    }

    WriteBuffer(msg);
}

void Protocol76::sendThingTransform(const Thing* thing,int32_t stackpos)
{
    if(CanSee(thing->pos.x, thing->pos.y, thing->pos.z))
    {
        const Item *item = dynamic_cast<const Item*>(thing);
        if(item)
        {
            NetworkMessage msg;
            if(stackpos == 0)
            {
                AddTileUpdated(msg,thing->pos);
            }
            else if(stackpos < 10)
            {
                msg.AddByte(0x6B);
                msg.AddPosition(thing->pos);
                msg.AddByte(stackpos);
                msg.AddItem(item);
            }
            else
            {
                return;
            }
            WriteBuffer(msg);
        }
        //update container icon
        if(dynamic_cast<const Container*>(item))
        {
            const Container *updateContainer = dynamic_cast<const Container*>(item);
            for(containerLayout::const_iterator cit = player->getContainers(); cit != player->getEndContainer(); ++cit)
            {
                Container *container = cit->second;
                if(container == updateContainer)
                {
                    sendContainer(cit->first, container);
                }
            }
        }
    }
}

void Protocol76::sendSkills()
{
    NetworkMessage msg;
    AddPlayerSkills(msg,player);
    WriteBuffer(msg);
}

void Protocol76::sendPing()
{
    NetworkMessage msg;
    msg.AddByte(0x1E);
    WriteBuffer(msg);
}

void Protocol76::sendThingRemove(const Thing *thing)
{
    //Auto-close trade
    if(player->getTradeItem() && dynamic_cast<const Item*>(thing) == player->getTradeItem())
    {
        game->playerCloseTrade(player);
    }

    NetworkMessage msg;

    //Auto-close container's
    const Container* moveContainer = dynamic_cast<const Container *>(thing);
    if(moveContainer)
    {
        autoCloseContainers(moveContainer, msg);
    }

    WriteBuffer(msg);
}

void Protocol76::sendDistanceShoot(const Position &from, const Position &to, unsigned char type)
{
    NetworkMessage msg;
    AddDistanceShoot(msg,from, to,type);
    WriteBuffer(msg);
}
void Protocol76::sendMagicEffect(const Position &pos, unsigned char type)
{
    NetworkMessage msg;
    AddMagicEffect(msg,pos,type);
    WriteBuffer(msg);
}
void Protocol76::sendAnimatedText(const Position &pos, unsigned char color, std::string text)
{
    NetworkMessage msg;
    AddAnimatedText(msg,pos,color,text);
    WriteBuffer(msg);
}
void Protocol76::sendCreatureHealth(const Creature *creature)
{
    NetworkMessage msg;
    AddCreatureHealth(msg,creature);
    WriteBuffer(msg);
}

void Protocol76::sendItemAddContainer(const Container *container, const Item *item)
{
    NetworkMessage msg;
    unsigned char cid = player->getContainerID(container);
    if(cid != 0xFF)
    {
        AddItemContainer(msg,cid,item);
        WriteBuffer(msg);
    }
}

void Protocol76::sendItemRemoveContainer(const Container *container, const unsigned char slot)
{
    NetworkMessage msg;
    unsigned char cid = player->getContainerID(container);
    if(cid != 0xFF)
    {
        RemoveItemContainer(msg,cid,slot);
        WriteBuffer(msg);
    }
}

void Protocol76::sendItemUpdateContainer(const Container *container, const Item* item,const unsigned char slot)
{
    NetworkMessage msg;
    unsigned char cid = player->getContainerID(container);
    if(cid != 0xFF)
    {
        TransformItemContainer(msg,cid,slot,item);
        WriteBuffer(msg);
    }
}

void Protocol76::sendTextWindow(Item* item,const uint16_t maxlen, const bool canWrite)
{
    NetworkMessage msg;
    msg.AddByte(0x96);
    windowTextID++;
    msg.AddU32(windowTextID);
    //msg.AddU16(item->getID());
    msg.AddItemId(item);
    if(canWrite)
    {
        msg.AddU16(maxlen);
        msg.AddString(item->getText());
        item->useThing();
        readItem = item;
    }
    else
    {
        msg.AddU16((uint16_t)item->getText().size());
        msg.AddString(item->getText());
        readItem = NULL;
    }
    msg.AddString(item->getWriter());

    WriteBuffer(msg);
}



////////////// Add common messages
void Protocol76::AddTextMessage(NetworkMessage &msg,MessageClasses mclass, const char* message)
{
    msg.AddByte(0xB4);
    msg.AddByte(mclass);
    msg.AddString(message);
}

void Protocol76::AddAnimatedText(NetworkMessage &msg,const Position &pos, unsigned char color, std::string text)
{
#ifdef __DEBUG__
    if(text.length() == 0)
    {
        std::cout << "Warning: 0-Length string in AddAnimatedText()" << std::endl;
    }
#endif

    msg.AddByte(0x84);
    msg.AddPosition(pos);
    msg.AddByte(color);
    msg.AddString(text);
}


void Protocol76::AddMagicEffect(NetworkMessage &msg,const Position &pos, unsigned char type)
{
    msg.AddByte(0x83);
    msg.AddPosition(pos);
    msg.AddByte(type+1);
}


void Protocol76::AddDistanceShoot(NetworkMessage &msg,const Position &from, const Position &to, unsigned char type)
{
    msg.AddByte(0x85);
    msg.AddPosition(from);
    msg.AddPosition(to);
    msg.AddByte(type+1);
}


void Protocol76::AddCreature(NetworkMessage &msg,const Creature *creature, bool known, uint32_t remove)
{
    if (known)
    {
        msg.AddByte(0x62);
        msg.AddByte(0x00);
        msg.AddU32(creature->getID());
    }
    else
    {
        msg.AddByte(0x61);
        msg.AddByte(0x00);
        //AddU32(0);
        msg.AddU32(remove);
        msg.AddU32(creature->getID());
        msg.AddString(creature->getName());
    }

    msg.AddByte(std::max(1LL, creature->health*100/creature->healthmax));
    msg.AddByte((unsigned char)creature->getDirection());

    if (creature->looktype > 1000)
    {
        msg.AddByte(0x00);
        msg.AddU16(Item::items[creature->looktype].clientId);
    }
    else
    {
        if (creature->isInvisible())
        {
            msg.AddByte(0);
            msg.AddU16(0);
        }
        else
        {
            msg.AddByte(creature->looktype);
            msg.AddByte(creature->lookhead);
            msg.AddByte(creature->lookbody);
            msg.AddByte(creature->looklegs);
            msg.AddByte(creature->lookfeet);
        }
    }

#ifdef CVS_DAY_CYCLE
    const Player* p = dynamic_cast<const Player*>(creature);
    if (p)
    {
        msg.AddByte(p->getLightLevel());
        msg.AddByte(p->getLightColor());
    }
    else
#endif //CVS_DAY_CYCLE
    {
        msg.AddByte(0x00); // light level
        msg.AddByte(0xD7); // light color
    }

    msg.AddU16(creature->getSpeed());

    Creature *ctarget = game->getCreatureByID(creature->getID());
    Player*target=dynamic_cast<Player*>(ctarget);
    std::vector<Player*>::iterator invited = std::find(player->invitedplayers.begin(), player->invitedplayers.end(), target);
    std::vector<Player*>::iterator inviter = std::find(player->inviterplayers.begin(), player->inviterplayers.end(), target);

    if(creature->skullType == SKULL_WHITE || creature->skullType == SKULL_RED)
        msg.AddByte(creature->skullType);
    else if(target && target->skullType == SKULL_NONE && target->party != 0 && player->party == target->party)
        msg.AddByte(2);
    else if(target && target->skullType == SKULL_YELLOW && player->isYellowTo(target))
        msg.AddByte(1);
    else
        msg.AddByte(0x00);

    if(target && target->party != 0 && player->party != 0 && target->party == player->party)
    {
        if(target->getID() == player->party)
            msg.AddByte(4);
        else
            msg.AddByte(3);
    }
    else if(invited != player->invitedplayers.end())
        msg.AddByte(2);
    else if(inviter != player->inviterplayers.end())
        msg.AddByte(1);
    else
        msg.AddByte(0x00);
}


void Protocol76::AddPlayerStats(NetworkMessage &msg,const Player *player)
{
    msg.AddByte(0xA0);
    msg.AddU16(player->getHealth());
    msg.AddU16(player->getPlayerInfo(PLAYERINFO_MAXHEALTH));
    msg.AddU16((uint16_t)std::floor(player->getFreeCapacity()));
    if(player->getExperience()>std::numeric_limits<int32_t>::max()){//YES, MUST BE SIGNED. (bug in tibia 7.6 client, the protocol can take unsigned max, but the client can not. )
        msg.AddU32(player->getLevel()>std::numeric_limits<int32_t>::max()? 0:player->getLevel());
    } else {
        msg.AddU32(player->getExperience());
    }
    msg.AddU16(player->getLevel()>std::numeric_limits<uint16_t>::max()?0:player->getLevel());
    msg.AddByte(player->getPlayerInfo(PLAYERINFO_LEVELPERCENT));
    msg.AddU16(player->getMana());
    msg.AddU16(player->getPlayerInfo(PLAYERINFO_MAXMANA));
    msg.AddByte(player->getMagicLevel()>std::numeric_limits<uint8_t>::max()?0:player->getMagicLevel());
    msg.AddByte(player->getPlayerInfo(PLAYERINFO_MAGICLEVELPERCENT));
    msg.AddByte(player->getPlayerInfo(PLAYERINFO_SOUL));
}

void Protocol76::AddPlayerSkills(NetworkMessage &msg,const Player *player)
{
    msg.AddByte(0xA1);

    msg.AddByte(player->getSkill(SKILL_FIST,   SKILL_LEVEL));
    msg.AddByte(player->getSkill(SKILL_FIST,   SKILL_PERCENT));
    msg.AddByte(player->getSkill(SKILL_CLUB,   SKILL_LEVEL));
    msg.AddByte(player->getSkill(SKILL_CLUB,   SKILL_PERCENT));
    msg.AddByte(player->getSkill(SKILL_SWORD,  SKILL_LEVEL));
    msg.AddByte(player->getSkill(SKILL_SWORD,  SKILL_PERCENT));
    msg.AddByte(player->getSkill(SKILL_AXE,    SKILL_LEVEL));
    msg.AddByte(player->getSkill(SKILL_AXE,    SKILL_PERCENT));
    msg.AddByte(player->getSkill(SKILL_DIST,   SKILL_LEVEL));
    msg.AddByte(player->getSkill(SKILL_DIST,   SKILL_PERCENT));
    msg.AddByte(player->getSkill(SKILL_SHIELD, SKILL_LEVEL));
    msg.AddByte(player->getSkill(SKILL_SHIELD, SKILL_PERCENT));
    msg.AddByte(player->getSkill(SKILL_FISH,   SKILL_LEVEL));
    msg.AddByte(player->getSkill(SKILL_FISH,   SKILL_PERCENT));
}


void Protocol76::AddPlayerInventoryItem(NetworkMessage &msg,const Player *player, int32_t item)
{
    if (player->getItem(item) == NULL)
    {
        msg.AddByte(0x79);
        msg.AddByte(item);
    }
    else
    {
        msg.AddByte(0x78);
        msg.AddByte(item);
        msg.AddItem(player->getItem(item));
    }
}


void Protocol76::AddCreatureSpeak(NetworkMessage &msg,const Creature *creature, SpeakClasses  type, std::string text, uint16_t channelId)
{
    msg.AddByte(0xAA);
    msg.AddString(creature->getName());
    msg.AddByte(type);
    switch(type)
    {
    case SPEAK_SAY:
    case SPEAK_WHISPER:
    case SPEAK_YELL:
    case SPEAK_MONSTER1:
    case SPEAK_MONSTER2:
        msg.AddPosition(creature->pos);
        break;
    case SPEAK_CHANNEL_Y:
    case SPEAK_CHANNEL_R1:
    case SPEAK_CHANNEL_R2:
    case SPEAK_CHANNEL_O:
        msg.AddU16(channelId);
        break;
    }
    msg.AddString(text);
}

void Protocol76::AddCreatureHealth(NetworkMessage &msg,const Creature *creature)
{
    msg.AddByte(0x8C);
    msg.AddU32(creature->getID());
    msg.AddByte(std::max(1LL, creature->health*100/creature->healthmax));
}

void Protocol76::AddRemoveThing(NetworkMessage &msg, const Position &pos,unsigned char stackpos)
{
    if(stackpos < 10)
    {
        if(CanSee(pos.x, pos.y, pos.z))
        {
            msg.AddByte(0x6C);
            msg.AddPosition(pos);
            msg.AddByte(stackpos);
        }
    }
    else
    {
        //This will cause some problem, we remove an item (example: a player gets removed due to death) from the map, but the client cant see it
        //(above the 9 limit), real tibia has the same problem so I don't think there is a way to fix this.
        //Problem: The client won't be informed that the player has been killed
        //and will show the player as alive (0 hp).
        //Solution: re-log.
    }

    Tile *fromTile = game->getTile(pos.x, pos.y, pos.z);
    if(fromTile && fromTile->getThingCount() > 8)
    {
        //We need to pop up this item
        Thing *newthing = fromTile->getThingByStackPos(9);

        if(newthing != NULL)
        {
            AddTileUpdated(msg, pos);
        }
    }
}

void Protocol76::AddAppearThing(NetworkMessage &msg, const Position &pos)
{
    if(CanSee(pos.x, pos.y, pos.z))
    {
        msg.AddByte(0x6A);
        msg.AddPosition(pos);
    }
}

void Protocol76::AddItemContainer(NetworkMessage &msg,unsigned char cid, const Item *item)
{
    msg.AddByte(0x70);
    msg.AddByte(cid);
    msg.AddItem(item);
}

void Protocol76::AddItemContainer(NetworkMessage &msg,unsigned char cid, const Item *item,unsigned char count)
{
    msg.AddByte(0x70);
    msg.AddByte(cid);
    msg.AddItem(item->getID(), count);
}

void Protocol76::TransformItemContainer(NetworkMessage &msg,unsigned char cid,uint16_t slot,const  Item *item)
{
    msg.AddByte(0x71);
    msg.AddByte(cid);
    msg.AddByte(slot);
    msg.AddItem(item);
}

void Protocol76::RemoveItemContainer(NetworkMessage &msg,unsigned char cid,uint16_t slot)
{
    msg.AddByte(0x72);
    msg.AddByte(cid);
    msg.AddByte(slot);
}

//////////////////////////

void Protocol76::flushOutputBuffer()
{
    OTSYS_THREAD_LOCK_CLASS lockClass(bufferLock, "Protocol76::flushOutputBuffer()");
    //force writetosocket
    OutputBuffer.WriteToSocket(s);
    OutputBuffer.Reset();

    return;
}

void Protocol76::WriteBuffer(NetworkMessage &add)
{

    game->addPlayerBuffer(player);

    OTSYS_THREAD_LOCK(bufferLock, "Protocol76::WriteBuffer")
    if(OutputBuffer.getMessageLength() + add.getMessageLength() >= NETWORKMESSAGE_MAXSIZE - 16)
    {
        this->flushOutputBuffer();
    }
    OutputBuffer.JoinMessages(add);
    OTSYS_THREAD_UNLOCK(bufferLock, "Protocol76::WriteBuffer")
    return;
}


#ifdef CVS_DAY_CYCLE
void Protocol76::sendWorldLightLevel(unsigned char lightlevel, unsigned char color)
{
    NetworkMessage msg;
    msg.AddByte(0x82);
    msg.AddByte(lightlevel);		 // 6F - Light level
    msg.AddByte(color /*0xA7*/); // D7 - Light color
    WriteBuffer(msg);
}

void Protocol76::sendPlayerLightLevel(const Player* player)
{
    NetworkMessage msg;
    msg.AddByte(0x8D);
    msg.AddU32(player->getID());
    msg.AddByte(player->getLightLevel());
    msg.AddByte(player->getLightColor());		//
    WriteBuffer(msg);
}
#endif //CVS_DAY_CYCLE

void Protocol76::sendHouseWindow(std::string members)
{
    NetworkMessage msg;
    msg.AddByte(0x97);
    msg.AddByte(0x01);// window ID?
    msg.AddByte(0x4E);//  U16 Part 1     ItemId
    msg.AddByte(0xC5);//  U16 Part 2
    msg.AddU16((uint16_t)members.size());
    msg.AddString(members);
    sendNetworkMessage(&msg);
}

void Protocol76::parseHouseWindow(NetworkMessage& msg)
{
    uint32_t id     = msg.GetU32();
    uint16_t size  = msg.GetU16();
    msg.GetByte();
    std::string new_text = msg.GetRaw();
    player->receiveHouseWindow(new_text);
}

void Protocol76::sendVIPLogIn(unsigned long guid)
{
    NetworkMessage msg;
    msg.AddByte(0xD3);
    msg.AddU32(guid);
    WriteBuffer(msg);
}

void Protocol76::sendVIPLogOut(unsigned long guid)
{
    NetworkMessage msg;
    msg.AddByte(0xD4);
    msg.AddU32(guid);
    WriteBuffer(msg);
}

void Protocol76::sendVIP(uint32_t guid, const std::string &name, bool isOnline)
{
    NetworkMessage msg;

    msg.AddByte(0xD2);
    msg.AddU32(guid);
    msg.AddString(name);
    msg.AddByte(isOnline == true ? 1 : 0);

    WriteBuffer(msg);
}

void Protocol76::parseAddVip(NetworkMessage &msg)
{
    std::string vip_name = msg.GetString();
    if(vip_name.size() > 32)
        return;
    game->requestAddVip(player, vip_name);
}

void Protocol76::parseRemVip(NetworkMessage &msg)
{
    uint32_t id = msg.GetU32();
    player->removeVIP(id);
}

void Protocol76::sendSkull(const Player *player)
{
    NetworkMessage msg;
    msg.AddByte(0x90);
    msg.AddU32(player->getID());
    if(player && player->skullType == SKULL_NONE && player->party != 0)
        msg.AddByte(2);//green skull
    else
        msg.AddByte(player->skullType);
    WriteBuffer(msg);
}

void Protocol76::sendPartyIcons(const Player *playa, int32_t icontype, bool skull, bool removeskull)
{
    NetworkMessage msg;
    msg.AddByte(0x91);
    msg.AddU32(playa->getID());
    msg.AddByte(icontype);
    WriteBuffer(msg);
    msg.Reset();
    if(skull || removeskull)
    {
        msg.AddByte(0x90);
        msg.AddU32(playa->getID());
        if(skull)
        {
            if(playa->skullType == SKULL_WHITE || playa->skullType == SKULL_RED)
                msg.AddByte(playa->skullType);
            else
                msg.AddByte(2);
        }
        if(removeskull)
        {
            if(playa->skullType == SKULL_WHITE || playa->skullType == SKULL_RED)
                msg.AddByte(playa->skullType);
            else
                msg.AddByte(0);
        }
        WriteBuffer(msg);
    }
}

void Protocol76::parseInviteParty(NetworkMessage &msg)
{
    uint32_t creatureid = msg.GetU32();
    Creature* creature = game->getCreatureByID(creatureid);
    Player* target = dynamic_cast<Player*>(creature);

    if (!target)
        return;

    if (target->party != 0)
    {
        std::stringstream bericht;
        bericht << target->getName() << " is already a member of a party.";
        player->sendTextMessage(MSG_INFO, bericht.str().c_str());
        return;
    }

    player->party = player->getID();
    target->inviterplayers.push_back(player);
    player->invitedplayers.push_back(target);

    std::stringstream bericht1;
    bericht1 << target->getName() << " has been invited.";
    player->sendTextMessage(MSG_INFO, bericht1.str().c_str());

    std::stringstream bericht2;
    if(player->getSex() == PLAYERSEX_MALE)
    {
        bericht2 << player->getName() <<" invites you to his party.";
    }
    else
    {
        bericht2 << player->getName() <<" invites you to her party.";
    }

    target->sendTextMessage(MSG_INFO, bericht2.str().c_str());
    target->onPartyIcons(player, 2, false, false);
    sendPartyIcons(player, 1, true, false);
    sendPartyIcons(player, 4, true, false);
    sendPartyIcons(target, 2, false, false);
    target->onPartyIcons(player, 1, false, false);
}

void Protocol76::parseRevokeParty(NetworkMessage &msg)
{
    int32_t members = 0;
    uint32_t creatureid = msg.GetU32();
    Creature* creature = game->getCreatureByID(creatureid);
    Player* target = dynamic_cast<Player*>(creature);
    std::vector<Player*>::iterator invited = std::find(player->invitedplayers.begin(), player->invitedplayers.end(), target);
    if(invited != player->invitedplayers.end())
        player->invitedplayers.erase(invited);
    std::stringstream bericht1;
    bericht1 << "Invitation for " << target->getName() << " has been revoked.";
    player->sendTextMessage(MSG_INFO, bericht1.str().c_str());
    std::stringstream bericht2;
    bericht2 << player->getName() << " has revoked his invitation.";
    target->sendTextMessage(MSG_INFO, bericht2.str().c_str());
    target->onPartyIcons(player, 0, false, false);
    sendPartyIcons(target, 0, false, false);
    for(AutoList<Player>::listiterator it = Player::listPlayer.list.begin(); it != Player::listPlayer.list.end(); ++it)
    {
        if((*it).second->party == player->party)
            members++;
    }
    if(members < 2)
        game->disbandParty(player->party);
}

void Protocol76::parseJoinParty(NetworkMessage &msg)
{
    uint32_t creatureid = msg.GetU32();
    Creature* creature = game->getCreatureByID(creatureid);
    Player* target = dynamic_cast<Player*>(creature);
    if(!target)
        return;

    player->party = target->party;
    std::vector<Player*>::iterator invited = std::find(target->invitedplayers.begin(), target->invitedplayers.end(), player);
    if(invited != target->invitedplayers.end())
        target->invitedplayers.erase(invited);
    player->inviterplayers.clear();
    std::stringstream bericht1;
    bericht1 <<  "You have joined "<< target->getName() << "'s party.";
    player->sendTextMessage(MSG_INFO, bericht1.str().c_str());
    std::stringstream bericht2;
    bericht2 << player->getName() << " has joined the party.";
    for(AutoList<Player>::listiterator it = Player::listPlayer.list.begin(); it != Player::listPlayer.list.end(); ++it)
    {
        if((*it).second->party == target->party)
        {
            if((*it).second->getID() != player->getID())
            {
                (*it).second->sendTextMessage(MSG_INFO, bericht2.str().c_str());
                (*it).second->onPartyIcons(player, 3, true, false);
            }
            player->onPartyIcons((*it).second, 3, true, false);
        }
        if((*it).second->getID() == player->party)
        {
            player->onPartyIcons((*it).second, 4, true, false);
        }
    }
}

void Protocol76::parsePassLeadership(NetworkMessage &msg)
{
    uint32_t creatureid = msg.GetU32();
    Creature* creature = game->getCreatureByID(creatureid);
    Player* target = dynamic_cast<Player*>(creature);
    target->sendTextMessage(MSG_INFO, "You are now leader of your party.");
    std::stringstream bericht;
    bericht << target->getName() << " is now the leader of your party.";
    int32_t oldpartyid = player->getID();
    for(AutoList<Player>::listiterator it = Player::listPlayer.list.begin(); it != Player::listPlayer.list.end(); ++it)
    {
        if((*it).second->party == oldpartyid)
        {
            if((*it).second->getID() != target->getID())
                (*it).second->sendTextMessage(MSG_INFO, bericht.str().c_str());
            (*it).second->onPartyIcons(target, 4, true, false);
            (*it).second->onPartyIcons(player, 3, true, false);
            (*it).second->party = target->getID();
        }
    }
}

#ifdef HUCZU_BAN_SYSTEM
void Protocol76::parseGM(NetworkMessage &msg)
{
    std::string name2 = msg.GetString();
    int32_t reason = msg.GetByte();
    std::string comment = msg.GetString();
    int32_t action = msg.GetByte();
    unsigned char IPban = msg.GetByte();
    Creature *c = game->getCreatureByName(name2.c_str());
    Player *bannedPlayer = dynamic_cast<Player*>(c);
    if(player && bannedPlayer)
    {
        std::stringstream myIP2;
        std::string myIP, Reason, Action;
        unsigned char ip[4];
        *(uint32_t*)&ip = bannedPlayer->getIP();
        myIP2 << (uint32_t)ip[0] << "." << (uint32_t)ip[1] <<
              "." << (uint32_t)ip[2] << "." << (uint32_t)ip[3];
        myIP = myIP2.str();
        if(player->access > bannedPlayer->access || myIP != "127.0.0.1")
        {
            if(comment.size() > 0 && comment.size() < 9999)
            {
                switch(reason)
                {
                case 0:
                    Reason = "Spamowanie";
                    break;
                case 1:
                    Reason = "Obrazliwe wyrazanie sie";
                    break;
                case 2:
                    Reason = "Destruktywne zachowanie";
                    break;
                case 3:
                    Reason = "Podawanie sie za czlonka ekipy";
                    break;
                case 4:
                    Reason = "Udawanie Game Mastera";
                    break;
                case 5:
                    Reason = "Handel/Wymiana Postaciami i Przedmiotami";
                    break;
                case 6:
                    Reason = "Sprzedawanie postaci";
                    break;
                case 7:
                    Reason = "Blokowanie Areny";
                    break;
                case 8:
                    Reason = "Pisanie niezgodnie z tematem na kanalach";
                    break;
                case 9:
                    Reason = "Reklamowanie innego OTS'a";
                    break;
                case 10:
                    Reason = "Reklama nie zwiazana z serwerem";
                    break;
                case 11:
                    Reason = "Wykorzystywanie bledow serwera";
                    break;
                case 12:
                    Reason = "Dzielenie konta";
                    break;
                case 13:
                    Reason = "Uzywanie Macro/Botow";
                    break;
                case 14:
                    Reason = "Hackowanie";
                    break;
                case 15:
                    Reason = "Multi-clienting";
                    break;
                case 16:
                    Reason = "Edytowanie pakietow";
                    break;
                case 17:
                    Reason = "Nick lamiacy regulamin";
                    break;
                default:
                    Reason = "Zlamanie Regulaminu";
                    break;
                }
                switch(action)
                {
                case 0:
                    Action = "BanNaDni";
                    break;
                case 1:
                    Action = "Namelock";
                    break;
                case 2:
                    Action = "AccountBan";
                    break;
                case 3:
                    Action = "Namelock/AccountBan";
                    break;
                case 4:
                    Action = "AccountBan+FinalWarning";
                    break;
                case 5:
                    Action = "Namelock/AccountBan+FinalWarning";
                    break;
                default:
                    Action = "Nieznana akcja";
                    break;
                }
                std::stringstream Yourtxt;
                Yourtxt << name2 << " zostal zbanowany za " << Reason << ".";
                if(IPban != 0)
                    Yourtxt << " Otrzymal tez bana na IP.";
                game->banPlayer(bannedPlayer, Reason, Action, comment,IPban);
                //game->addBan(player, bannedPlayer, Reason, Action, comment, IPban);
                for(AutoList<Player>::listiterator it = Player::listPlayer.list.begin(); it != Player::listPlayer.list.end(); ++it)
                {
                    if(dynamic_cast<Player*>(it->second))
                        (*it).second->sendTextMessage(MSG_RED_INFO, Yourtxt.str().c_str());
                }
            }
            else
                player->sendTextMessage(MSG_RED_INFO, "W comment wpisz ilosc dni.");
        }
        else
            player->sendTextMessage(MSG_RED_INFO, "Wybacz ale tej osoby nie zbanujesz bo jest wyzsza ranga.");
    }
    else
        player->sendTextMessage(MSG_RED_INFO, "Postac o tym imieniu nie jest zalogowana.");
}
#endif //HUCZU_BAN_SYSTEM

void Protocol76::sendToSpecialChannel(const Creature * creature, SpeakClasses type, const std::string &text, uint16_t channelId, const std::string &info)
{
    NetworkMessage msg;
    AddCreatureSpecialSpeak(msg,creature, type, text, channelId, info);
    WriteBuffer(msg);
}

void Protocol76::AddCreatureSpecialSpeak(NetworkMessage &msg,const Creature *creature, SpeakClasses  type, std::string text, uint16_t channelId, std::string info)
{
    msg.AddByte(0xAA);
    msg.AddString(info);
    msg.AddByte(type);
    switch(type)
    {
    case SPEAK_SAY:
    case SPEAK_WHISPER:
    case SPEAK_YELL:
    case SPEAK_MONSTER1:
    case SPEAK_MONSTER2:
        msg.AddPosition(creature->pos);
        break;
    case SPEAK_CHANNEL_Y:
    case SPEAK_CHANNEL_R1:
    case SPEAK_CHANNEL_R2:
    case SPEAK_CHANNEL_O:
        msg.AddU16(channelId);
        break;
    }
    msg.AddString(text);
}

#ifdef HUCZU_RRV
void Protocol76::addViolation(Player* reporter, std::string message)
{
    // reporter files report
    OTSYS_THREAD_LOCK_CLASS lockClass(game->gameLock, "Protocol76::addViolation()");
    reporter->violationTime = OTSYS_TIME();
    reporter->violationReport = message;
    reporter->hasOpenViolation = true;

    game->openViolations.push_back(reporter);

    NetworkMessage msg;
    msg.AddByte(0xAA);
    //msg.AddU32(0);
    msg.AddString(reporter->getName());
    msg.AddByte(SPEAK_CHANNEL_RV1);
    msg.AddU32(0x00000000); // cause its 0 secs since report ;p
    msg.AddString(reporter->violationReport);
    updateViolationsChannel(msg);
}

void Protocol76::removeViolation(std::string rname)
{
    // close off open violation - reporter logout/cancelled or gm answers
    OTSYS_THREAD_LOCK_CLASS lockClass(game->gameLock, "Protocol76::removeViolation()");
    NetworkMessage msg;
    Player* reporter = game->getPlayerByName(rname);
    //if (reporter && reporter->hasOpenViolation)
    //{
    msg.AddByte(0xAF);
    msg.AddString(rname);
    updateViolationsChannel(msg);
    //}
    for(std::vector<Player*>::iterator it = game->openViolations.begin(); it != game->openViolations.end(); ++it)
    {
        // iterate thru vector and pop out the one matching this guy's
        if ((*it) && (*it)->getName() == rname) // its 'ours'
        {
            (*it)->hasOpenViolation = false;
            (*it)->violationName = "";
            (*it)->violationTime = 0;
            (*it)->violationReport = "";
            game->openViolations.erase(it);
            return; // leave or crash
        }
    }
}

void Protocol76::cancelViolation(std::string rname)
{
    // reporter cancels report
    OTSYS_THREAD_LOCK_CLASS lockClass(game->gameLock, "Protocol76::cancelViolation()");
    NetworkMessage msg;
    Player* gm = game->getPlayerByName(player->violationName);
    if (gm)
    {
        // has attached gm

        //msg.AddU32(0);
        msg.AddByte(0xB0); // lock gm's channel
        msg.AddString(rname);
        gm->sendNetworkMessage(&msg);

        gm->violationName = "";
        player->violationName = "";
    }
    removeViolation(rname);

}

void Protocol76::openViolation(NetworkMessage &msg)
{
    // gm answers report in violations window
    OTSYS_THREAD_LOCK_CLASS lockClass(game->gameLock, "Protocol76::openViolation()");
    std::string rname = msg.GetString();
    NetworkMessage newmsg;

    Player* reporter = game->getPlayerByName(rname);
    if (reporter)
    {
        removeViolation(rname);

        reporter->violationName = player->getName();
        player->violationName = rname;
    }

}

void Protocol76::closeViolation(NetworkMessage &msg)
{
    // report done and gm closes their end
    OTSYS_THREAD_LOCK_CLASS lockClass(game->gameLock, "Protocol76::closeViolation()");
    std::string rname = msg.GetString();
    NetworkMessage newmsg;

    Player* reporter = game->getPlayerByName(rname);

    if (reporter)
    {
        if (reporter->hasOpenViolation)
            removeViolation(rname);

        newmsg.AddByte(0xB1);   // Vitor - packetid byte - locks reporters channel
        reporter->sendNetworkMessage(&newmsg);

        reporter->violationName = "";
        player->violationName = "";
    }

}

void Protocol76::speakToReporter(std::string rname, std::string message)
{
    // gm/couns to reportee
    OTSYS_THREAD_LOCK_CLASS lockClass(game->gameLock, "Protocol76::speakToReporter()");
    NetworkMessage newmsg;
    Player* reporter = game->getPlayerByName(rname);

    if (reporter)
    {
        // send message
        newmsg.AddByte(0xAA);
        //newmsg.AddU32(0);
        newmsg.AddString("Counsellor");
        newmsg.AddByte(SPEAK_CHANNEL_RV2);
        newmsg.AddString(message);
        reporter->sendNetworkMessage(&newmsg);
        // send status text
        newmsg.Reset();
        newmsg.AddByte(0xB4);
        newmsg.AddByte(MSG_SMALLINFO);
        std::stringstream ss;
        ss << "Message sent to " << reporter->getName() << ".";
        newmsg.AddString(ss.str());
        WriteBuffer(newmsg);
    }

}

void Protocol76::speakToCounsellor(std::string message)
{
    // reportee to couns/gm
    OTSYS_THREAD_LOCK_CLASS lockClass(game->gameLock, "Protocol76::speakToCounsellor()");
    NetworkMessage newmsg;
    Player* gm = game->getPlayerByName(player->violationName);

    if (gm)
    {
        newmsg.AddByte(0xAA);
        //newmsg.AddU32(0);
        newmsg.AddString(player->getName());
        newmsg.AddByte(SPEAK_CHANNEL_RV3);
        newmsg.AddString(message.c_str());
        gm->sendNetworkMessage(&newmsg);
        // send status text
        newmsg.Reset();
        newmsg.AddByte(0xB4);
        newmsg.AddByte(MSG_SMALLINFO);
        newmsg.AddString("Message sent to Counsellor.");
        WriteBuffer(newmsg);
    }

}

void Protocol76::updateViolationsChannel(NetworkMessage &update) // gamelock in add/remove
{
    // update all open violation channels
    for(AutoList<Player>::listiterator it = Player::listPlayer.list.begin(); it != Player::listPlayer.list.end(); ++it)
        if ((*it).second->hasViolationsChannelOpen)
            (*it).second->sendNetworkMessage(&update);

}

void Protocol76::sendViolationChannel(Player* gm) // gamelock in send channel
{
    // couns/gm opens violations channel
    player->hasViolationsChannelOpen = true;
    NetworkMessage newmsg;

    newmsg.AddByte(0xAE);
    newmsg.AddU16(0x0003);

    for(std::vector<Player*>::iterator it = game->openViolations.begin(); it != game->openViolations.end(); ++it)
    {
        // use vector to get right order
        if (it != game->openViolations.end())
            if ((*it) && (*it)->hasOpenViolation) // its valid
            {
                newmsg.AddByte(0xAA);
                //newmsg.AddU32(0);
                newmsg.AddString((*it)->getName());
                newmsg.AddByte(SPEAK_CHANNEL_RV1);

                uint64_t secs = (OTSYS_TIME() - (*it)->violationTime)/1000;
                newmsg.AddU32((int32_t)(secs));

                newmsg.AddString((*it)->violationReport);
            }
    }
    WriteBuffer(newmsg);

}
#endif
