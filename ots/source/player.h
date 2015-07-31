#ifndef __player_h_
#define __player_h_

#include "definitions.h"
#include "creature.h"
#include "container.h"

#include <vector>
#include <ctime>
#include <algorithm>
#include "templates.h"
#include "map.h"
#include "houses.h"
#include "guilds.h"
#include "networkmessage.h"
#include "luascript.h"

extern LuaScript g_config;

class Protocol;

struct Death
{
    std::string killer;
    int32_t level;
    time_t time;
};

enum skills_t
{
    SKILL_FIST=0,
    SKILL_CLUB=1,
    SKILL_SWORD=2,
    SKILL_AXE=3,
    SKILL_DIST=4,
    SKILL_SHIELD=5,
    SKILL_FISH=6
};

enum skillsid_t
{
    SKILL_LEVEL=0,
    SKILL_TRIES=1,
    SKILL_PERCENT=2
};

enum playerinfo_t
{
    PLAYERINFO_LEVEL,
    PLAYERINFO_LEVELPERCENT,
    PLAYERINFO_HEALTH,
    PLAYERINFO_MAXHEALTH,
    PLAYERINFO_MANA,
    PLAYERINFO_MAXMANA,
    PLAYERINFO_MANAPERCENT,
    PLAYERINFO_MAGICLEVEL,
    PLAYERINFO_MAGICLEVELPERCENT,
    PLAYERINFO_SOUL,
};

enum playersex_t
{
    PLAYERSEX_FEMALE = 0,
    PLAYERSEX_MALE = 1,
    PLAYERSEX_OLDMALE = 2,
    PLAYERSEX_DWARF = 3,
    PLAYERSEX_NIMFA = 4
};

//0 = None, 1 = Sorcerer, 2 = Druid, 3 = Paladin, 4 = Knight
enum playervoc_t
{
    VOCATION_NONE = 0,
    VOCATION_SORCERER = 1,
    VOCATION_DRUID = 2,
    VOCATION_PALADIN = 3,
    VOCATION_KNIGHT = 4
};

enum freeslot_t
{
    SLOT_TYPE_NONE,
    SLOT_TYPE_INVENTORY,
    SLOT_TYPE_CONTAINER
};

enum trade_state
{
    TRADE_NONE,
    TRADE_INITIATED,
    TRADE_ACCEPT,
    TRADE_ACKNOWLEDGE
};

typedef std::pair<unsigned char, Container*> containerItem;
typedef std::vector<containerItem> containerLayout;
typedef std::map<uint32_t, Container*> DepotMap;
typedef std::map<uint32_t,int32_t> StorageMap;
typedef std::set<uint32_t> VIPListSet;
typedef std::list<uint32_t> InvitedToGuildsList;
const int32_t MAX_VIPS = 50;

//////////////////////////////////////////////////////////////////////
// Defines a player...

class Player : public Creature
{
public:
    GuildLevel_t guildStatus;
    InvitedToGuildsList invitedToGuildsList;
    uint32_t guildId;
#ifdef __MIZIAK_SUPERMANAS__
    uint32_t eventManas;
    Item* getItemByID(int32_t itemid, int32_t count, bool actionId = false);
    Item* getItemByIDContainer(Container* container, int32_t itemid, int32_t count, bool actionId = false);
#endif
#ifdef HUCZU_HITS_KOMENDA
    bool showHits;
#endif
    uint32_t getPunkty()
    {
        return punkty;
    }
    void addPunkty(uint32_t _punkty)
    {
        punkty += _punkty;
    }
    bool removePunkty(uint32_t _punkty);
    void setGuildNick(const std::string& newNick)
    {
        guildNick = newNick;
    }
    void setGuildId(uint32_t newId)
    {
        guildId = newId;
    }
    uint32_t getRankId() const
    {
        return rankId;
    }
    void setRankId(uint32_t newId)
    {
        rankId = newId;
    }
    void setGuildName(const std::string& newName)
    {
        guildName = newName;
    }
    void setRankName(const std::string& newName)
    {
        rankName = newName;
    }
    const std::string& getRankName() const
    {
        return rankName;
    }
    bool setGuildLevel(GuildLevel_t newLevel, uint32_t rank = 0);
    bool isGuildInvited(uint32_t guildId) const;
    void leaveGuild();
    bool removeVIP(uint32_t _guid);

    std::string msgB;
    unsigned char atkMode;
#ifdef __MIZIAK_CREATURESCRIPTS__
    bool dieorlogout;
#endif //__MIZIAK_CREATURESCRIPTS__

#ifdef HUCZU_SERVER_LOG
    void sendFromSystem(SpeakClasses type, const std::string &text);
#endif
#ifdef HUCZU_RRV
    bool hasViolationsChannelOpen; // for updates of newly added/answered by other/cancelled
    bool hasOpenViolation;  // to cancel/close violation on logout etc.
    std::string violationName;  // reporter name or gm name, depends on side of convo
    uint64_t violationTime;     // time when reported
    std::string violationReport; // the report string
    void cancelPlayerViolation();
#endif
    time_t lastlogin;
    time_t lastLoginSaved;
    void sendNetworkMessage(NetworkMessage *msg);
    uint32_t frags;
    unsigned char lastPacket;
    void mcCheck();
    void botMessage(BotType typBota);

    char fightMode;
#ifdef HUCZU_FOLLOW
    char followMode;
    Creature *oldAttackedCreature;
#endif //HUCZU_FOLLOW
    bool hasTwoSquareItem() const;

    Player(const std::string& name, Protocol* p);
    virtual ~Player();
    virtual Player* getPlayer()
    {
        return this;
    };
    virtual const Player* getPlayer() const
    {
        return this;
    };
    void setGUID(uint32_t _guid)
    {
        guid = _guid;
    };
    uint32_t getGUID() const
    {
        return guid;
    };
    virtual uint32_t idRange()
    {
        return 0x10000000;
    }
    static AutoList<Player> listPlayer;
    void removeList();
    void addList();
    void kickPlayer();
#ifdef HUCZU_BAN_SYSTEM
    int32_t banned , banstart, banend, deleted, finalwarning;
    std::string comment, reason, action, banrealtime;
    uint16_t namelock;
#endif //HUCZU_BAN_SYSTEM
    void sendToSpecialChannel(SpeakClasses type, const std::string &text, uint16_t channelId, const std::string &info);

    bool addItem(Item* item, bool test = false);
    bool internalAddItemContainer(Container *container,Item* item);

    freeslot_t getFreeSlot(Container **container,unsigned char &slot, const Item* item);
    Container* getFreeContainerSlot(Container *parent);

    bool removeItemSmart(uint16_t itemid, int32_t count, bool depot);
    //bool removeItem(uint16_t id,int32_t count);
    bool removeItem(Item* item, bool test = false);
    bool internalRemoveItemContainer(Container *parent, Item* item, bool test = false);
    int32_t getItemCount(uint16_t id);

    int32_t removeItemInventory(int32_t pos, bool internal = false);
    int32_t addItemInventory(Item* item, int32_t pos, bool internal = false);

    containerLayout::const_iterator getContainers() const
    {
        return vcontainers.begin();
    }
    containerLayout::const_iterator getEndContainer() const
    {
        return vcontainers.end();
    }

    Container* getContainer(uint16_t containerid);
    unsigned char getContainerID(const Container* container) const;
    bool isHoldingContainer(const Container* container) const;
    void addContainer(unsigned char containerid, Container *container);
    containerLayout::const_iterator closeContainer(unsigned char containerid);
    void addStorageValue(const uint32_t key, const int32_t value);
    bool getStorageValue(const uint32_t key, int32_t &value) const;
    inline StorageMap::const_iterator getStorageIteratorBegin() const
    {
        return storageMap.begin();
    }
    inline StorageMap::const_iterator getStorageIteratorEnd() const
    {
        return storageMap.end();
    }

    int32_t getLevel() const
    {
        return level;
    }
    int32_t getHealth() const
    {
        return health;
    }
    int32_t getMana() const
    {
        return mana;
    }
    int32_t getMagicLevel() const
    {
        return maglevel;
    }
    playersex_t getSex()
    {
        return sex;
    }
    bool gainManaTick();
    bool gainHealthTick();
    bool aol;
    bool starlight;
    const std::string& getName() const
    {
        return name;
    };
    const std::string& getGuildName() const
    {
        return guildName;
    };
    uint32_t getGuildId() const
    {
        return guildId;
    };


    int32_t getPlayerInfo(playerinfo_t playerinfo) const;
    int32_t getSkill(skills_t skilltype, skillsid_t skillinfo) const;
    std::string getSkillName(int32_t skillid);
    void addSkillTry(int32_t skilltry);
    void addSkillShieldTry(int32_t skilltry);

    exp_t getExperience() const
    {
        return experience;
    };
    double getCapacity() const
    {
        if(access < g_config.ACCESS_PROTECT)
        {
            return capacity;
        }
        else
            return 0.00;
    }

    virtual exp_t getLostExperience()
    {
        if (starlight)
            return (exp_t)(experience * g_config.DIE_PERCENT_SL / 100.0);
        else
            return (exp_t)(experience * g_config.DIE_PERCENT_EXP / 100.0);
    }

    double getFreeCapacity() const
    {
        if(access < g_config.ACCESS_PROTECT)
        {
            return std::max(0.00, capacity - inventoryWeight);
        }
        else
            return 0.00;
    }

    time_t getLastLoginSaved() const
    {
        return lastLoginSaved;
    };

    void updateInventoryWeigth();

    Item* getItem(int32_t pos) const;
    Item* GetDistWeapon() const;

    void addManaSpent(uint32_t spent);
    void addExp(exp_t exp);
    virtual int32_t getWeaponDamage() const;
    virtual int32_t getArmor() const;
    virtual int32_t getDefense() const;
    uint32_t getMoney();
    bool substractMoney(uint32_t money);
    bool substractMoneyItem(Item *item, uint32_t money);


    uint32_t eventAutoWalk;

    //items
    containerLayout vcontainers;
    void preSave();
    virtual void useThing()
    {
        //std::cout << "Player: useThing() " << this << std::endl;
        useCount++;
    };

    virtual void releaseThing()
    {
        useCount--;
        //std::cout << "Player: releaseThing() " << this << std::endl;
        if (useCount == 0)
            delete this;
    };

//V.I.P. functions
    VIPListSet VIPList;

    uint32_t getIP() const;
    Container* getDepot(uint32_t depotId);
    bool addDepot(Container* depot,uint32_t depotIs);
    //depots
    DepotMap depots;
    int32_t max_depot_items;

    virtual void removeDistItem();
    fight_t getFightType();
    subfight_t getSubFightType();

    bool CanSee(int32_t x, int32_t y, int32_t z) const;

    void sendIcons();
    void sendChangeSpeed(Creature* creature);
    void sendToChannel(Creature *creature, SpeakClasses type, const std::string &text, uint16_t channelId);
    virtual void sendCancel(const char *msg) const;
    virtual void sendCancelWalk() const;
    int32_t sendInventory(unsigned char sl_id);
    void sendStats();
    void sendTextMessage(MessageClasses mclass, const char* message) const;
    void sendTextMessage(MessageClasses mclass, const char* message,const Position &pos, unsigned char type) const;
    void sendPing(uint32_t interval);
    void sendCloseContainer(unsigned char containerid);
    void sendContainer(unsigned char index, Container *container);
    void sendTextWindow(Item* item,const uint16_t maxlen, const bool canWrite);
    void sendDistanceShoot(const Position &from, const Position &to, unsigned char type);
    void sendMagicEffect(const Position &pos, unsigned char type);
    void sendAnimatedText(const Position &pos, unsigned char color, std::string text);
    void sendCreatureHealth(const Creature *creature);
    void sendTradeItemRequest(const Player* player, const Item* item, bool ack);
    void sendCloseTrade();
    void receivePing();
    void flushMsg();

    void die();      //player loses exp/skills/maglevel on death

    //virtual void setAttackedCreature(uint32_t id);
    virtual bool isAttackable() const
    {
        return (access < g_config.ACCESS_PROTECT);
    };
    virtual bool isPushable() const;
    virtual void dropLoot(Container *corpse);
    virtual int32_t getLookCorpse();
    bool NeedUpdateStats();

    virtual std::string getDescription(bool self = false) const;

    //ground
    void onThingAppear(const Thing* thing);
    void onThingTransform(const Thing* thing,int32_t stackpos);
    void onThingDisappear(const Thing* thing, unsigned char stackPos);
    void onThingRemove(const Thing* thing); //auto-close containers

    //container
    void onItemAddContainer(const Container* container,const Item* item);
    void onItemRemoveContainer(const Container* container,const unsigned char slot);
    void onItemUpdateContainer(const Container* container,const Item* item,const unsigned char slot);

    //inventory - for this use int32_t sendInventory(unsigned char sl_id)
    //void onItemAddInvnetory(const unsigned char sl_id);
    //void onItemRemoveInvnetory(const unsigned char sl_id);
    //void onItemUpdateInvnetory(const unsigned char sl_id);

    void setAcceptTrade(bool b);
    bool getAcceptTrade()
    {
        return (tradeState == TRADE_ACCEPT);
    };
    Item* getTradeItem()
    {
        return tradeItem;
    };

    playervoc_t getVocation() const
    {
        return vocation;
    }

#ifdef CVS_DAY_CYCLE
    void sendWorldLightLevel(unsigned char lightlevel, unsigned char color);
    void sendPlayerLightLevel(Player* player);
    virtual unsigned char getLightLevel() const
    {
        return lightlevel;
    }
    virtual unsigned char getLightColor() const
    {
        return lightcolor;
    }
    virtual void setLightLevel(unsigned char light, unsigned char color)
    {
        lightlevel = light;
        lightcolor = color;
    }
#endif //CVS_DAY_CYCLE

    bool getCoins(uint32_t requiredcoins);
    uint32_t getContainerCoins(Container* container, uint32_t coins);
    bool removeCoins(int32_t cost);
    int32_t removeContainerCoins(Container* container, int32_t cost);
    void TLMaddItem(int32_t itemid, unsigned char count);
    void addItemek(int32_t itemid, unsigned char count, uint16_t actionid);
    bool removeItem(int32_t itemid, int32_t count);
    int32_t removeContainerItem(Container* container, int32_t itemid, int32_t count);
    void payBack(uint32_t cost);
    bool getItem(int32_t itemid, int32_t count);
    int32_t getContainerItem(Container* container, int32_t itemid, int32_t count);

#ifdef TLM_HOUSE_SYSTEM
    bool houseRightsChanged;
#endif //TLM_HOUSE_SYSTEM
    //other
    void sendClosePrivate(uint16_t channelId);

protected:
    House* editedHouse;
    Position editedHousePos;
    rights_t editedHouseRights;
public:
    void sendHouseWindow(House* house, const Position& pos, rights_t rights);
    void receiveHouseWindow(std::string members);

//	void setGuildInfo(gstat_t gstat, uint32_t gid, std::string gname, std::string grank, std::string nick);
    void checkAfk(int32_t thinkTics);
    void notAfk();
    bool addVIP(uint32_t _guid, std::string &name, bool isOnline, bool internal = false);
    void sendVipLogin(std::string vipname);
    void sendVipLogout(std::string vipname);
    void notifyLogIn(Player* player);
    void notifyLogOut(Player* player);
    std::string vip[MAX_VIPS];

#ifdef YUR_BOH
    void checkBoh();
#endif //YUR_BOH

#ifdef YUR_RINGS_AMULETS
    void checkRing(int32_t thinkTics);
#endif //YUR_RINGS_AMULETS

    exp_t getExpForNextLevel();
    uint32_t getManaForNextMLevel();

#ifdef YUR_LOGIN_QUEUE
    int32_t getAccount() const
    {
        return accountNumber;
    }
#endif //YUR_LOGIN_QUEUE

    void setFightMode(char fm)
    {
        fightMode = fm;
    }

#ifdef TRS_GM_INVISIBLE
    int32_t oldlookhead, oldlookbody, oldlooklegs, oldlookfeet, oldlooktype, oldlookcorpse, oldlookmaster;
    bool gmInvisible;
#endif //TRS_GM_INVISIBLE
    int32_t tradeTicks;
    int32_t gameTicks;

    std::vector<Player*> inviterplayers;
    std::vector<Player*> invitedplayers;
    uint32_t party;
    void onPartyIcons(const Player *playa, int32_t icontype, bool skull, bool removeskull);

#ifdef HUCZU_SKULLS
    int32_t skullTicks, skullKills, absolveTicks;
    void onSkull(Player* player);
    bool checkSkull(int32_t thinkTics);
    typedef std::set<int32_t> AttackedSet;
    AttackedSet attackedSet;
    std::vector<Player*> hasAsYellow;
    bool isYellowTo(Player* player);
    void removeFromYellowList(Player* player);
    bool hasAttacked(const Player* attacked) const;
    void addAttacked(const Player* attacked);
    void clearAttacked();
    void clearYellowList();
#endif //HUCZU_SKULLS

//	static void LoadGainsMuls();

#ifdef YUR_PREMIUM_PROMOTION
    bool isPremium() const
    {
        return g_config.FREE_PREMMY || premiumTicks > 0;
    }
    void checkPremium(int32_t);
    bool isPromoted() const
    {
        return promoted && isPremium();
    }
    void promote()
    {
        promoted = true;
    }
#endif //YUR_PREMIUM_PROMOTION

    bool isRookie() const;
    void setVocation(playervoc_t voc);
    bool isUsingSpears() const;


    void checkLightItem(int32_t thinkTics);
    bool isUsingBurstArrows() const;
    bool isUsingGoldBolt() const;
    bool isUsingSilverWand() const;
    void addDeath(const std::string& killer, int32_t level, time_t time);
    int32_t getWandId() const;

protected:
    void sendCancelAttacking();
    void addSkillTryInternal(int32_t skilltry,int32_t skill);
    virtual void onCreatureAppear(const Creature *creature);
    virtual void onCreatureDisappear(const Creature *creature, unsigned char stackPos, bool tele);
    virtual void onCreatureTurn(const Creature *creature, unsigned char stackpos);
    virtual void onCreatureSay(const Creature *creature, SpeakClasses type, const std::string &text);
    virtual void onCreatureChangeOutfit(const Creature* creature);
    virtual void onTeleport(const Creature *creature, const Position *oldPos, unsigned char oldstackpos);
    virtual int32_t onThink(int32_t& newThinkTicks);

    virtual void onTileUpdated(const Position &pos);

    //container to container
    virtual void onThingMove(const Creature *creature, const Container *fromContainer, uint16_t from_slotid,
                             const Item* fromItem, int32_t oldFromCount, Container *toContainer, uint16_t to_slotid,
                             const Item *toItem, int32_t oldToCount, int32_t count);

    //inventory to container
    virtual void onThingMove(const Creature *creature, slots_t fromSlot, const Item* fromItem,
                             int32_t oldFromCount, const Container *toContainer, uint16_t to_slotid, const Item *toItem, int32_t oldToCount, int32_t count);

    //inventory to inventory
    virtual void onThingMove(const Creature *creature, slots_t fromSlot, const Item* fromItem,
                             int32_t oldFromCount, slots_t toSlot, const Item* toItem, int32_t oldToCount, int32_t count);

    //container to inventory
    virtual void onThingMove(const Creature *creature, const Container *fromContainer, uint16_t from_slotid,
                             const Item* fromItem, int32_t oldFromCount, slots_t toSlot, const Item *toItem, int32_t oldToCount, int32_t count);

    //container to ground
    virtual void onThingMove(const Creature *creature, const Container *fromContainer, unsigned char from_slotid,
                             const Item* fromItem, int32_t oldFromCount, const Position &toPos, const Item *toItem, int32_t oldToCount, int32_t count);

    //inventory to ground
    virtual void onThingMove(const Creature *creature, slots_t fromSlot,
                             const Item* fromItem, int32_t oldFromCount, const Position &toPos, const Item *toItem, int32_t oldToCount, int32_t count);

    //ground to container
    virtual void onThingMove(const Creature *creature, const Position &fromPos, int32_t stackpos, const Item* fromItem,
                             int32_t oldFromCount, const Container *toContainer, unsigned char to_slotid, const Item *toItem, int32_t oldToCount, int32_t count);

    //ground to inventory
    virtual void onThingMove(const Creature *creature, const Position &fromPos, int32_t stackpos, const Item* fromItem,
                             int32_t oldFromCount, slots_t toSlot, const Item *toItem, int32_t oldToCount, int32_t count);

    //ground to ground
    virtual void onThingMove(const Creature *creature, const Thing *thing, const Position *oldPos,
                             unsigned char oldstackpos, unsigned char oldcount, unsigned char count);

protected:
    Protocol *client;
    int32_t useCount;
    //uint32_t experience;

    int32_t accountNumber;
    exp_t experience;
    playervoc_t vocation;
    playersex_t sex;
    int32_t food;
    double capacity;
    std::string guildName, rankName, guildNick;
#ifdef YUR_PREMIUM_PROMOTION
    bool promoted;
    int32_t premiumTicks;
#endif //YUR_PREMIUM_PROMOTION
    uint32_t punkty;
    uint32_t countItem(uint16_t itemid);
    unsigned char level_percent;
    unsigned char maglevel_percent;

    uint32_t rankId;

    std::string password;
    Item* items[11]; //equipement of the player
    //for skill advances
    uint32_t getReqSkillTries (int32_t skill, int32_t level, playervoc_t voc);

    //for magic level advances
    uint32_t getReqMana(int32_t maglevel, playervoc_t voc);
    //player advances variables
    uint32_t skills[7][3];
    typedef std::list<Death> DeathList;
    DeathList deathList;


#ifdef CVS_DAY_CYCLE
    unsigned char lightlevel;
    unsigned char lightcolor;
    int32_t last_worldlightlevel;
#endif //CVS_DAY_CYCLE

    int32_t idleTime;
    bool warned;
    bool energyRing;
    bool stealthRing;
    bool dwarvenRing;
    int32_t lightItem;

    double inventoryWeight;


    bool SendBuffer;
    uint32_t internal_ping;
    uint32_t npings;

    //account variables


    uint32_t lastip;

    //inventory variables



#ifdef CVS_GAINS_MULS
    //reminder: 0 = None, 1 = Sorcerer, 2 = Druid, 3 = Paladin, 4 = Knight
    static int32_t CapGain[5];          //for level advances
    static int32_t ManaGain[5];
    static int32_t HPGain[5];
#endif //CVS_GAINS_MULS

    static const int32_t gainManaVector[5][2];
    static const int32_t gainHealthVector[5][2];
    uint16_t manaTick;
    uint16_t healthTick;

#ifdef YUR_PREMIUM_PROMOTION
    static const int32_t promotedGainManaVector[5][2];
    static const int32_t promotedGainHealthVector[5][2];
#endif //YUR_PREMIUM_PROMOTION


    //trade variables
    uint32_t tradePartner;
    trade_state tradeState;
    //bool acceptTrade;
    Item *tradeItem;

    //autowalking
    std::list<Direction> pathlist;

    //cache some data
    struct SkillCache
    {
        uint32_t tries;
        int32_t level;
        //int32_t voc;
        playervoc_t vocation;
    };

    SkillCache SkillAdvanceCache[7][2];
    struct SentStats
    {
        int32_t health;
        int32_t healthmax;
        exp_t experience;
        int32_t level;
        double freeCapacity;
        //int32_t cap;
        int32_t mana;
        int32_t manamax;
        int32_t manaspent;
        int32_t maglevel;
    };

    SentStats lastSentStats;
    // we need our name
    std::string name;
    uint32_t guid;



    std::string guildRank;



    StorageMap storageMap;

    struct MoneyItem
    {
        Item* item;
        freeslot_t location;
        int32_t slot;
        Container *parent;
    };
    typedef std::multimap<int32_t, struct MoneyItem*, std::less<int32_t> > MoneyMap;
    typedef MoneyMap::value_type moneymap_pair;



    friend OTSYS_THREAD_RETURN ConnectionHandler(void *dat);

    friend class Game;
    friend class ActionScript;
    friend class Commands;
    friend class Map;
    friend class IOPlayerSQL;
};


#endif // __player_h_
