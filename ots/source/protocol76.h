
#ifndef tprot76_h
#define tprot76_h

#include "networkmessage.h"
#include "protocol.h"
#include "creature.h"
#include "item.h"
#include "container.h"
#include <string>

class NetworkMessage;

class Protocol76 : public Protocol
{
public:
    // our constructor get's the socket of the client and the initial
    // message the client sent
    Protocol76(SOCKET s);
    virtual ~Protocol76();

    bool ConnectPlayer(
#ifdef YUR_LOGIN_QUEUE
        int32_t* placeInQueue = NULL
#endif //YUR_LOGIN_QUEUE
    );
    void ReceiveLoop();
    void WriteBuffer(NetworkMessage &add);
    void reinitializeProtocol(SOCKET s);

private:
    // the socket the player is on...
    NetworkMessage OutputBuffer;
    std::list<uint32_t> knownPlayers;
    void checkCreatureAsKnown(uint32_t id, bool &known, uint32_t &removedKnown);

    // we have all the parse methods
    void parsePacket(NetworkMessage &msg);

    void parseLogout(NetworkMessage &msg);

    void parseCancelMove(NetworkMessage &msg);
    void parseModes(NetworkMessage &msg);
    void parseDebug(NetworkMessage &msg);

    void parseMoveByMouse(NetworkMessage &msg);

    void parseMoveNorth(NetworkMessage &msg);
    void parseMoveEast(NetworkMessage &msg);
    void parseMoveSouth(NetworkMessage &msg);
    void parseMoveWest(NetworkMessage &msg);
    void parseMoveNorthEast(NetworkMessage &msg);
    void parseMoveSouthEast(NetworkMessage &msg);
    void parseMoveSouthWest(NetworkMessage &msg);
    void parseMoveNorthWest(NetworkMessage &msg);

    void parseTurnNorth(NetworkMessage &msg);
    void parseTurnEast(NetworkMessage &msg);
    void parseTurnSouth(NetworkMessage &msg);
    void parseTurnWest(NetworkMessage &msg);

    void parseRequestOutfit(NetworkMessage &msg);
    void parseSetOutfit(NetworkMessage &msg);

    void parseSay(NetworkMessage &msg);

    void parseLookAt(NetworkMessage &msg);

    void parseAttack(NetworkMessage &msg);
#ifdef HUCZU_FOLLOW
    void parseFollow(NetworkMessage &msg);
#endif //HUCZU_FOLLOW
    void parseThrow(NetworkMessage &msg);
    void parseUseItemEx(NetworkMessage &msg);
    void parseBattleWindow(NetworkMessage &msg);
    void parseUseItem(NetworkMessage &msg);
#ifdef HUCZU_BAN_SYSTEM
    void parseGM(NetworkMessage &msg);
    unsigned char IPban;
#endif //HUCZU_BAN_SYSTEM
    void parseCloseContainer(NetworkMessage &msg);
    void parseUpArrowContainer(NetworkMessage &msg);
    void parseTextWindow(NetworkMessage &msg);

    void parseRequestTrade(NetworkMessage &msg);
    void parseLookInTrade(NetworkMessage &msg);
    void parseAcceptTrade(NetworkMessage &msg);
    void parseCloseTrade();

#ifdef CVS_DAY_CYCLE
    virtual void sendWorldLightLevel(unsigned char lightlevel, unsigned char color);
    virtual void sendPlayerLightLevel(const Player* player);
#endif //CVS_DAY_CYCLE

    void sendHouseWindow(std::string members);
    void parseHouseWindow(NetworkMessage& msg);
    Game* getGame()
    {
        return game;
    }

    virtual void sendSkull(const Player *player);
    virtual void sendPartyIcons(const Player *playa, int32_t icontype, bool skull, bool removeskull);
    void parseInviteParty(NetworkMessage &msg);
    void parseRevokeParty(NetworkMessage &msg);
    void parseJoinParty(NetworkMessage &msg);
    void parsePassLeadership(NetworkMessage &msg);

    void parseRotateItem(NetworkMessage &msg);

    // channel tabs
    void parseCreatePrivateChannel(NetworkMessage& msg);
    void parseChannelInvite(NetworkMessage& msg);
    void parseChannelExclude(NetworkMessage& msg);
    void parseGetChannels(NetworkMessage &msg);
    void parseOpenChannel(NetworkMessage &msg);
    void parseOpenPriv(NetworkMessage &msg);
    void parseCloseChannel(NetworkMessage &msg);
    void sendClosePrivate(uint16_t channelId);
    virtual void sendChannelsDialog();
    virtual void sendChannel(uint16_t channelId, std::string channelName);
    virtual void sendOpenPriv(const std::string &receiver);
    virtual void sendToChannel(const Creature *creature, SpeakClasses type, const std::string &text, uint16_t channelId);
    virtual void AddCreatureSpecialSpeak(NetworkMessage &msg,const Creature *creature, SpeakClasses type, std::string text, uint16_t channelId, std::string info);
    virtual void sendToSpecialChannel(const Creature * creature, SpeakClasses type, const std::string &text, uint16_t channelId, const std::string &info);
    virtual void sendNetworkMessage(NetworkMessage *msg);
    virtual void sendIcons(int32_t icons);
#ifdef HUCZU_SERVER_LOG
    virtual void sendFromSystem(SpeakClasses type, const std::string &text);
    virtual void AddSystemSpeak(NetworkMessage &msg, SpeakClasses type, std::string text, uint16_t channelId);
#endif
#ifdef HUCZU_RRV
    virtual void addViolation(Player* reporter, std::string message); // player uses ctrl-r
    virtual void removeViolation(std::string rname);    // remove open violation from stack
    void cancelViolation(std::string rname);    // player cancels violation/logsout
    virtual void openViolation(NetworkMessage &msg);    // gm answers in violations
    virtual void closeViolation(NetworkMessage &msg);   // gm closes report
    virtual void speakToReporter(std::string rname, std::string message);
    virtual void speakToCounsellor(std::string message);
    virtual void updateViolationsChannel(NetworkMessage &update); // new/canceled/opened report
    virtual void sendViolationChannel(Player* gm);  // gm opens violations
#endif
    //container to container
    virtual void sendThingMove(const Creature *creature, const Container *fromContainer, uint16_t from_slotid,
                               const Item* fromItem, int32_t oldFromCount, Container *toContainer, uint16_t to_slotid, const Item *toItem, int32_t oldToCount, int32_t count);

    //inventory to container
    virtual void sendThingMove(const Creature *creature, slots_t fromSlot, const Item* fromItem,
                               int32_t oldFromCount, const Container *toContainer, uint16_t to_slotid, const Item *toItem, int32_t oldToCount, int32_t count);

    //inventory to inventory
    virtual void sendThingMove(const Creature *creature, slots_t fromSlot, const Item* fromItem,
                               int32_t oldFromCount, slots_t toSlot, const Item* toItem, int32_t oldToCount, int32_t count);

    //container to inventory
    virtual void sendThingMove(const Creature *creature, const Container *fromContainer, uint16_t from_slotid,
                               const Item* fromItem, int32_t oldFromCount, slots_t toSlot, const Item *toItem, int32_t oldToCount, int32_t count);

    //container to ground
    virtual void sendThingMove(const Creature *creature, const Container *fromContainer, unsigned char from_slotid,
                               const Item* fromItem, int32_t oldFromCount, const Position &toPos, const Item *toItem, int32_t oldToCount, int32_t count);

    //inventory to ground
    virtual void sendThingMove(const Creature *creature, slots_t fromSlot,
                               const Item* fromItem, int32_t oldFromCount, const Position &toPos, const Item *toItem, int32_t oldToCount, int32_t count);

    //ground to container
    virtual void sendThingMove(const Creature *creature, const Position &fromPos, int32_t stackpos, const Item* fromItem,
                               int32_t oldFromCount, const Container *toContainer, unsigned char to_slotid, const Item *toItem, int32_t oldToCount, int32_t count);

    //ground to inventory
    virtual void sendThingMove(const Creature *creature, const Position &fromPos, int32_t stackpos, const Item* fromItem,
                               int32_t oldFromCount, slots_t toSlot, const Item *toItem, int32_t oldToCount, int32_t count);

    //ground to ground
    virtual void sendThingMove(const Creature *creature, const Thing *thing,
                               const Position *oldPos, unsigned char oldstackpos, unsigned char oldcount,
                               unsigned char count, bool tele = false);

    void autoCloseContainers(const Container *container, NetworkMessage &msg);

    void sendMoveCreature(const Creature* creature, const Position& newPos, const Position& oldPos,
                          uint32_t oldStackPos, bool teleport);

    virtual void sendThingDisappear(const Thing *thing, unsigned char stackPos, bool tele = false);
    virtual void sendThingAppear(const Thing *thing);
    virtual void sendThingTransform(const Thing* thing,int32_t stackpos);
    virtual void sendThingRemove(const Thing *thing);
    virtual void sendDistanceShoot(const Position &from, const Position &to, unsigned char type);
    virtual void sendMagicEffect(const Position &pos, unsigned char type);
    virtual void sendAnimatedText(const Position &pos, unsigned char color, std::string text);
    virtual void sendCreatureHealth(const Creature *creature);
    virtual void sendSkills();
    virtual void sendPing();
    virtual void sendCreatureTurn(const Creature *creature, unsigned char stackpos);
    virtual void sendCreatureSay(const Creature *creature, SpeakClasses type, const std::string &text);

    virtual void sendCancel(const char *msg);
    virtual void sendCancelWalk();
    virtual void sendChangeSpeed(const Creature* creature);
    virtual void sendCancelAttacking();
    void sendSetOutfit(const Creature* creature);
    virtual void sendTileUpdated(const Position &Pos);
    virtual void sendInventory(unsigned char sl_id);
    virtual void sendStats();
    virtual void sendTextMessage(MessageClasses mclass, const char* message);
    virtual void sendTextMessage(MessageClasses mclass, const char* message,const Position &pos, unsigned char type);

    virtual bool CanSee(int32_t x, int32_t y, int32_t z) const;
    virtual bool CanSee(const Creature*) const;
    virtual void logout();

    void flushOutputBuffer();
    void WriteMsg(NetworkMessage &msg);

    virtual void sendContainer(unsigned char index, Container *container);
    virtual void sendTradeItemRequest(const Player* player, const Item* item, bool ack);
    virtual void sendCloseTrade();

    //VIP methods
    virtual void sendVIP(uint32_t guid, const std::string &name, bool isOnline);
	virtual void sendVIPLogIn(unsigned long guid);
	virtual void sendVIPLogOut(unsigned long guid);
    void parseAddVip(NetworkMessage &msg);
    void parseRemVip(NetworkMessage &msg);

    virtual void sendCloseContainer(unsigned char containerid);
    void sendItemAddContainer(const Container *container, const Item *item);
    void sendItemRemoveContainer(const Container* container,const unsigned char slot);
    void sendItemUpdateContainer(const Container* container,const Item* item,const unsigned char slot);
    void sendTextWindow(Item* item,const uint16_t maxlen, const bool canWrite);


#ifdef TRS_GM_INVISIBLE
    void GetTileDescription(const Tile* tile, NetworkMessage &msg, Player* p);
    void GetMapDescription(uint16_t x, uint16_t y, unsigned char z,
                           uint16_t width, uint16_t height,
                           NetworkMessage &msg, Player* p);
#else
    void GetTileDescription(const Tile* tile, NetworkMessage &msg);
    void GetMapDescription(uint16_t x, uint16_t y, unsigned char z,
                           uint16_t width, uint16_t height,
                           NetworkMessage &msg);
#endif //TRS_GM_INVISIBLE

    virtual void AddTextMessage(NetworkMessage &msg,MessageClasses mclass, const char* message);
    virtual void AddAnimatedText(NetworkMessage &msg,const Position &pos, unsigned char color, std::string text);
    virtual void AddMagicEffect(NetworkMessage &msg,const Position &pos, unsigned char type);
    virtual void AddDistanceShoot(NetworkMessage &msg,const Position &from, const Position &to, unsigned char type);
    virtual void AddCreature(NetworkMessage &msg,const Creature *creature, bool known, uint32_t remove);
    virtual void AddPlayerStats(NetworkMessage &msg,const Player *player);
    virtual void AddPlayerInventoryItem(NetworkMessage &msg,const Player *player, int32_t item);
    virtual void AddCreatureSpeak(NetworkMessage &msg,const Creature *creature, SpeakClasses type, std::string text, uint16_t channelId);
    virtual void AddCreatureHealth(NetworkMessage &msg,const Creature *creature);
    virtual void AddPlayerSkills(NetworkMessage &msg,const Player *player);
    virtual void AddRemoveThing(NetworkMessage &msg, const Position &pos,unsigned char stackpos);
    virtual void AddAppearThing(NetworkMessage &msg, const Position &pos);
    virtual void AddTileUpdated(NetworkMessage &msg, const Position &pos);
    virtual void AddItemContainer(NetworkMessage &msg,unsigned char cid, const Item *item);
    virtual void AddItemContainer(NetworkMessage &msg,unsigned char cid, const Item *item,unsigned char count);
    virtual void TransformItemContainer(NetworkMessage &msg,unsigned char cid,uint16_t slot, const Item *item);
    virtual void RemoveItemContainer(NetworkMessage &msg,unsigned char cid,uint16_t slot);


    OTSYS_THREAD_LOCKVAR bufferLock;
    uint32_t windowTextID;
    Item *readItem;

    friend OTSYS_THREAD_RETURN ConnectionHandler(void *dat);
};
#endif
