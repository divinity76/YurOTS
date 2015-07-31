//#include "preheaders.h"

#include <string>
#include <sstream>
#include <iostream>
#include <algorithm>

using namespace std;

#include "protocol.h"
#include "player.h"
#include "luascript.h"
#include "const76.h"
#include "chat.h"
#include "networkmessage.h"
#ifdef __MIZIAK_CREATURESCRIPTS__
#include "actions.h"
extern Actions actions;
#endif //__MIZIAK_CREATURESCRIPTS__
extern LuaScript g_config;
extern Game g_game;
extern Chat g_chat;

AutoList<Player> Player::listPlayer;

#ifdef YUR_PREMIUM_PROMOTION
const int32_t Player::promotedGainManaVector[5][2] = {{6,1},{2,1},{2,1},{3,1},{6,1}};
const int32_t Player::promotedGainHealthVector[5][2] = {{6,1},{6,1},{6,1},{3,1},{2,1}};
#endif //YUR_PREMIUM_PROMOTION
const int32_t Player::gainManaVector[5][2] = {{6,1},{3,1},{3,1},{4,1},{6,1}};
const int32_t Player::gainHealthVector[5][2] = {{6,1},{6,1},{6,1},{4,1},{3,1}};

#ifdef CVS_GAINS_MULS
int32_t Player::CapGain[5] = {10, 10, 10, 20, 25};
int32_t Player::ManaGain[5] = {5, 30, 30, 15, 5};
int32_t Player::HPGain[5] = {5, 5, 5, 10, 15};
#endif //CVS_GAINS_MULS

Player::Player(const std::string& name, Protocol *p) :
    Creature()
{
    client     = p;
    if (client)
        client->setPlayer(this);
    guid       = 0;
    looktype   = PLAYER_MALE_1;
    vocation   = VOCATION_NONE;
    capacity = 300.00;
    mana       = 0;
    manamax    = 0;
    manaspent  = 0;
    this->name = name;
    food       = 0;
    aol        = false;
    starlight  = false;
    rankId     = 0;
    guildId    = 0;
    guildNick  = "";
    lastPacket = 0;
    tradeTicks = 0;
    gameTicks  = 0;
    frags      = 0;
#ifdef __MIZIAK_CREATURESCRIPTS__
    dieorlogout = false;
#endif //__MIZIAK_CREATURESCRIPTS__
#ifdef HUCZU_RRV
    bool hasViolationsChannelOpen = false;
    bool hasOpenViolation = false;
    std::string violationName = "";
    uint64_t violationTime = 0;
    std::string violationReport = "";
#endif
    msgB = "";
    punkty = 0;
#ifdef HUCZU_BAN_SYSTEM
    banned     = 0;
    banstart = 0;
    banend     = 0;
    comment    = "";
    reason     = "";
    action     = "";
    deleted = 0;
    finalwarning = 0;
    banrealtime = "";
    namelock = 0;
#endif //HUCZU_BAN_SYSTEM
#ifdef HUCZU_FOLLOW
    oldAttackedCreature = NULL;
#endif //HUCZU_FOLLOW
#ifdef CVS_DAY_CYCLE
    lightlevel = 0;
    last_worldlightlevel = -1;
#endif //CVS_DAY_CYCLE
    guildStatus = GUILD_NONE;
    editedHouse = NULL;
    editedHouseRights = HOUSE_NONE;
#ifdef TLM_HOUSE_SYSTEM
    houseRightsChanged = true;
#endif //TLM_HOUSE_SYSTEM
    idleTime   = 0;
    warned     = false;
#ifdef TRS_GM_INVISIBLE
    gmInvisible = false;
    oldlookhead = 0;
    oldlookbody = 0;
    oldlooklegs = 0;
    oldlookfeet = 0;
    oldlookmaster = 0;
    oldlooktype = PLAYER_MALE_1;
    oldlookcorpse = ITEM_HUMAN_CORPSE;
#endif //TRS_GM_INVISIBLE
    party = 0;
#ifdef HUCZU_SKULLS
    skullTicks = 0;
    skullKills = 0;
    absolveTicks = 0;
#endif //HUCZU_SKULLS
#ifdef YUR_RINGS_AMULETS
    energyRing = false;
    stealthRing = false;
    dwarvenRing = false; //the ring
#endif //YUR_RINGS_AMULETS
#ifdef YUR_PREMIUM_PROMOTION
    premiumTicks = 0;
    promoted = false;
#endif //YUR_PREMIUM_PROMOTION
    lightItem = 0;
    atkMode = 0;
    eventAutoWalk = 0;
#ifdef __MIZIAK_SUPERMANAS__
    eventManas = 0;
#endif
    level      = 1;
    experience = 1;
    maglevel   = 1;
#ifdef HUCZU_HITS_KOMENDA
    showHits = true;
#endif
    access     = 0;
    lastlogin  = 1;
    lastLoginSaved = 1;
    SendBuffer = false;
    npings = 0;
    internal_ping = 0;
    fightMode = 0;
#ifdef HUCZU_FOLLOW
    followMode = 0;
#endif //HUCZU_FOLLOW

    tradePartner = 0;
    tradeState = TRADE_NONE;
    tradeItem = NULL;

    for(int32_t i = 0; i < 7; i++)
    {
        skills[i][SKILL_LEVEL] = 10;
        skills[i][SKILL_TRIES] = 0;
        skills[i][SKILL_PERCENT] = 0;

        for(int32_t j = 0; j < 2; j++)
        {
            SkillAdvanceCache[i][j].level = 0;
            SkillAdvanceCache[i][j].vocation = VOCATION_NONE;
            SkillAdvanceCache[i][j].tries = 0;
        }
    }

    lastSentStats.health = 0;
    lastSentStats.healthmax = 0;
    lastSentStats.freeCapacity = 0;
    lastSentStats.experience = 0;
    lastSentStats.level = 0;
    lastSentStats.mana = 0;
    lastSentStats.manamax = 0;
    lastSentStats.manaspent = 0;
    lastSentStats.maglevel = 0;
    level_percent = 0;
    maglevel_percent = 0;

    //set item pointers to NULL
    for(int32_t i = 0; i < 11; i++)
        items[i] = NULL;

    useCount = 0;
#ifdef FIXY
    if(isPremium())
        max_depot_items = g_config.MAX_DEPOTITEMS_PREMMY;
    else
        max_depot_items = g_config.MAX_DEPOTITEMS_FREE;
#endif //FIXY

    manaTick = 0;
    healthTick = 0;
}


Player::~Player()
{
    for (int32_t i = 0; i < 11; i++)
    {
        if (items[i])
        {
            items[i]->releaseThing();
        }
    }

    DepotMap::iterator it;
    for(it = depots.begin(); it != depots.end(); ++it)
    {
        it->second->releaseThing();
    }
    //std::cout << "Player destructor " << this->getID() << std::endl;
    delete client;
}

bool Player::isPushable() const
{
    return ((getSleepTicks() <= 0) && access < g_config.ACCESS_PROTECT);
}

std::string Player::getDescription(bool self) const
{
    std::stringstream s;
    std::string str;

    if(self)
    {
        s << "yourself.";
        if(vocation == VOCATION_NONE)
        {
            if(sex == PLAYERSEX_FEMALE || sex == PLAYERSEX_NIMFA)
                s << " You are Rook Girl.";
            else
                s << "You are Rook Man.";
        }
        if(vocation != VOCATION_NONE)
        {
#ifdef YUR_PREMIUM_PROMOTION
            if (isPromoted())
                s << " You are " << g_config.PROMOTED_VOCATIONS[(int32_t)vocation] << ".";
            else
#endif //YUR_PREMIUM_PROMOTION
                s << " You are " << g_config.VOCATIONS[(int32_t)vocation] << ".";
        }
    }
    else
    {
        s << name << " (Level " << level <<").";

        if(vocation == VOCATION_NONE)
        {
            if(sex == PLAYERSEX_FEMALE || sex == PLAYERSEX_NIMFA)
                s << " She is Rook Girl.";
            else
                s << " He is Rook Man.";
        }
        if(vocation != VOCATION_NONE)
        {
            if(sex == PLAYERSEX_FEMALE || sex == PLAYERSEX_NIMFA)
                s << " She";
            else
                s << " He";

#ifdef YUR_PREMIUM_PROMOTION
            if (isPromoted())
                s << " is "<< g_config.PROMOTED_VOCATIONS[(int32_t)vocation] << ".";
            else
#endif //YUR_PREMIUM_PROMOTION
                s << " is "<< g_config.VOCATIONS[(int32_t)vocation] << ".";
        }
    }

    /*	if (guildStatus >= GUILD_MEMBER)
    	{
    		if(self)
    			s << " You are ";
    		else
    		{
    			if(sex == PLAYERSEX_FEMALE || sex == PLAYERSEX_NIMFA)
    				s << " She is ";
    			else
    				s << " He is ";
    		}

    		if(guildRank.length())
    			s << guildRank;
    		else
    			s << "a member";

    		s << " of " << guildName;

    		if(guildNick.length())
    			s << " (" << guildNick << ")";

    		s << ".";
    	}*/
    if(guildId)
    {
        if(self)
            s << " You are ";
        else
        {
            if(sex == PLAYERSEX_FEMALE || sex == PLAYERSEX_NIMFA)
                s << " She is ";
            else
                s << " He is ";
        }

        s << (rankName.empty() ? "a member" : rankName)<< " of the " << guildName;
        if(!guildNick.empty())
            s << " (" << guildNick << ")";

        s << ".";
    }

    str = s.str();
    return str;
}

Item* Player::getItem(int32_t pos) const
{
    if(pos>0 && pos <11)
        return items[pos];
    return NULL;
}

int32_t Player::getWeaponDamage() const
{
    double mul = 1.0;
    mul = (fightMode==1? 2.0 : (fightMode==2? 1.5 : 1.0));

    double damagemax = 0;
    //TODO:what happens with 2 weapons?
    for (int32_t slot = SLOT_RIGHT; slot <= SLOT_LEFT; slot++)
        if (items[slot])
        {
            if ((items[slot]->isWeapon()))
            {
                // check which kind of skill we use...
                // and calculate the damage dealt
                Item *distitem;
                switch (items[slot]->getWeaponType())
                {
                case SWORD:
                    //damagemax = 3*skills[SKILL_SWORD][SKILL_LEVEL] + 2*Item::items[items[slot]->getID()].attack;
                    if(getVocation() == VOCATION_KNIGHT && hasTwoSquareItem())
                        damagemax = (mul*getSkill(SKILL_SWORD,SKILL_LEVEL) *Item::items[items[slot]->getID()].attack/15.0 + getLevel()*1.0 + Item::items[items[slot]->getID()].attack)*2.0;
                    else
                        damagemax = mul*getSkill(SKILL_SWORD,SKILL_LEVEL) *Item::items[items[slot]->getID()].attack/20.0 + getLevel()*1.5 + Item::items[items[slot]->getID()].attack;
                    break;
                case CLUB:
                    //damagemax = 3*skills[SKILL_CLUB][SKILL_LEVEL] + 2*Item::items[items[slot]->getID()].attack;
                    damagemax = mul*getSkill(SKILL_CLUB,SKILL_LEVEL)*Item::items[items[slot]->getID()].attack/20.0 + getLevel()*1.5 + Item::items[items[slot]->getID()].attack;
                    break;
                case AXE:
                    //damagemax = 3*skills[SKILL_AXE][SKILL_LEVEL] + 2*Item::items[items[slot]->getID()].attack;
                    damagemax = mul*getSkill(SKILL_AXE,SKILL_LEVEL)*Item::items[items[slot]->getID()].attack/20.0 + getLevel()*1.5 + Item::items[items[slot]->getID()].attack;
                    break;
                case DIST:
                    distitem = GetDistWeapon();
                    if(distitem)
                    {
                        //damagemax = 3*skills[SKILL_DIST][SKILL_LEVEL]+ 2*Item::items[distitem->getID()].attack;
                        damagemax = mul*getSkill(SKILL_DIST,SKILL_LEVEL)*Item::items[distitem->getID()].attack/20.0 + getLevel()*1.5 + Item::items[distitem->getID()].attack;

                        //hit or miss
                        int32_t hitchance;
                        if(distitem->getWeaponType() == AMO) //projectile
                        {
                            hitchance = 90;
                        }
                        else //thrown weapons
                        {
                            hitchance = 50;
                        }
                        if(rand()%100 < hitchance)  //hit
                        {
                            return 1+(int32_t)(damagemax*rand()/(RAND_MAX+1.0));
                        }
                        else 	//miss
                        {
                            return 0;
                        }
                    }
                    break;
                case MAGIC:
                    damagemax = (level*2.0+maglevel*3.0) * 1.25;
                    break;
                case AMO:
                case NONE:
                case SHIELD:
                    // nothing to do
                    break;
                }
            }
            if(damagemax != 0)
                break;
        }

    // no weapon found -> fist fighting
    if (damagemax == 0)
        damagemax = 0.5*mul*getSkill(SKILL_FIST,SKILL_LEVEL) + 1.0;

    // return it
    return 1+(int32_t)(damagemax*rand()/(RAND_MAX+1.0));
}

int32_t Player::getArmor() const
{
    int32_t armor=0;

    if(items[SLOT_HEAD])
        armor += items[SLOT_HEAD]->getArmor();
    if(items[SLOT_NECKLACE])
        armor += items[SLOT_NECKLACE]->getArmor();
    if(items[SLOT_ARMOR])
        armor += items[SLOT_ARMOR]->getArmor();
    if(items[SLOT_LEGS])
        armor += items[SLOT_LEGS]->getArmor();
    if(items[SLOT_FEET])
        armor += items[SLOT_FEET]->getArmor();
    if(items[SLOT_RING])
        armor += items[SLOT_RING]->getArmor();

    return armor;
}

int32_t Player::getDefense() const
{
    int32_t defense=0;

    if(items[SLOT_LEFT])
    {
        if(items[SLOT_LEFT]->getWeaponType() == SHIELD)
            defense += skills[SKILL_SHIELD][SKILL_LEVEL] + items[SLOT_LEFT]->getDefense();
        else
            defense += items[SLOT_LEFT]->getDefense();
    }
    if(items[SLOT_RIGHT])
    {
        if(items[SLOT_RIGHT]->getWeaponType() == SHIELD)
            defense += skills[SKILL_SHIELD][SKILL_LEVEL] + items[SLOT_RIGHT]->getDefense();
        else
            defense += items[SLOT_RIGHT]->getDefense();
    }
    //////////
    if(defense == 0)
        defense = (int32_t)random_range(0,(int32_t)skills[SKILL_SHIELD][SKILL_LEVEL]);
    else
        defense += (int32_t)random_range(0,(int32_t)skills[SKILL_SHIELD][SKILL_LEVEL]);

    defense = int32_t(double(defense) * (fightMode==1? 1.0 : (fightMode==2? 1.5 : 2.0)));

    return random_range(int32_t(defense*0.25), int32_t(1+(int32_t)(defense*rand())/(RAND_MAX+1.0)));
    ///////////
    //return defense;
}
uint32_t Player::countItem(uint16_t itemid)
{
    uint32_t count = 0;
    std::list<const Container*> stack;
    ContainerList::const_iterator cit;
    for(int32_t i=0; i< 11; i++)
    {
        if(items[i])
        {
            if(Container *tmpcontainer = dynamic_cast<Container*>(items[i]))
            {
                stack.push_back(tmpcontainer);
            }
            else
            {
                if(items[i]->getID() == itemid)
                    count++;
            }
        }
    }

    while(!stack.empty())
    {
        const Container *container = stack.front();
        stack.pop_front();
        for(cit = container->getItems(); cit != container->getEnd(); ++cit)
        {
            if((*cit)->getID() == itemid)
                count++;
            Container *container = dynamic_cast<Container*>(*cit);
            if(container)
            {
                stack.push_back(container);
            }
        }
    }
    return count;
}

uint32_t Player::getMoney()
{
    uint32_t money = 0;
    std::list<const Container*> stack;
    ContainerList::const_iterator cit;
    for(int32_t i=0; i< 11; i++)
    {
        if(items[i])
        {
            if(Container *tmpcontainer = dynamic_cast<Container*>(items[i]))
            {
                stack.push_back(tmpcontainer);
            }
            else
            {
                money = money + items[i]->getWorth();
            }
        }
    }

    while(!stack.empty())
    {
        const Container *container = stack.front();
        stack.pop_front();
        for(cit = container->getItems(); cit != container->getEnd(); ++cit)
        {
            money = money + (*cit)->getWorth();
            Container *container = dynamic_cast<Container*>(*cit);
            if(container)
            {
                stack.push_back(container);
            }
        }
    }
    return money;
}

bool Player::substractMoney(uint32_t money)
{
    if(getMoney() < money)
        return false;

    std::list<Container*> stack;
    MoneyMap moneyMap;
    MoneyItem* tmp;

    ContainerList::iterator it;
    for(int32_t i = 0; i < 11 && money > 0 ; i++)
    {
        if(items[i])
        {
            if(Container *tmpcontainer = dynamic_cast<Container*>(items[i]))
            {
                stack.push_back(tmpcontainer);
            }
            else
            {
                if(items[i]->getWorth() != 0)
                {
                    tmp = new MoneyItem;
                    tmp->item = items[i];
                    tmp->slot = i;
                    tmp->location = SLOT_TYPE_INVENTORY;
                    tmp->parent = NULL;
                    moneyMap.insert(moneymap_pair(items[i]->getWorth(),tmp));
                }
            }
        }
    }

    while(!stack.empty() && money > 0)
    {
        Container *container = stack.front();
        stack.pop_front();
        for(int32_t i = 0; i < container->size() && money > 0; i++)
        {
            Item *item = container->getItem(i);
            if(item && item->getWorth() != 0)
            {
                tmp = new MoneyItem;
                tmp->item = item;
                tmp->slot = 0;
                tmp->location = SLOT_TYPE_CONTAINER;
                tmp->parent = container;
                moneyMap.insert(moneymap_pair(item->getWorth(),tmp));
            }
            Container *containerItem = dynamic_cast<Container*>(item);
            if(containerItem)
            {
                stack.push_back(containerItem);
            }
        }
    }

    MoneyMap::iterator it2;
    for(it2 = moneyMap.begin(); it2 != moneyMap.end() && money > 0; it2++)
    {
        Item *item = it2->second->item;

        if(it2->second->location == SLOT_TYPE_INVENTORY)
        {
            removeItemInventory(it2->second->slot);
        }
        else if(it2->second->location == SLOT_TYPE_CONTAINER)
        {
            Container *container = it2->second->parent;
            unsigned char slot = container->getSlotNumberByItem(item);
            onItemRemoveContainer(container,slot);
            container->removeItem(item);
        }

        if((uint32_t)it2->first <= money)
        {
            money = money - it2->first;
        }
        else
        {
            substractMoneyItem(item, money);
            money = 0;
        }
        g_game.FreeThing(item);
        item = NULL;
        delete it2->second;
        it2->second = NULL;
    }
    for(; it2 != moneyMap.end(); it2++)
    {
        delete it2->second;
        it2->second = NULL;
    }

    moneyMap.clear();

    if(money != 0)
        return false;

    return true;
}

bool Player::substractMoneyItem(Item *item, uint32_t money)
{
    if(money >= (uint32_t)item->getWorth())
        return false;

    int32_t remaind = item->getWorth() - money;
    int32_t scar = remaind / 1000000;
    remaind = remaind - scar * 1000000;
    int32_t crys = remaind / 10000;
    remaind = remaind - crys * 10000;
    int32_t plat = remaind / 100;
    remaind = remaind - plat * 100;
    int32_t gold = remaind;

    if(scar != 0)
    {
        Item *remaindItem = Item::CreateItem(ITEM_COINS_SCARAB, scar);
        if(!this->addItem(remaindItem))
            g_game.addThing(NULL,this->pos,remaindItem);
    }

    if(crys != 0)
    {
        Item *remaindItem = Item::CreateItem(ITEM_COINS_CRYSTAL, crys);
        if(!this->addItem(remaindItem))
            g_game.addThing(NULL,this->pos,remaindItem);

    }

    if(plat != 0)
    {
        Item *remaindItem = Item::CreateItem(ITEM_COINS_PLATINUM, plat);
        if(!this->addItem(remaindItem))
            g_game.addThing(NULL,this->pos,remaindItem);
    }

    if(gold != 0)
    {
        Item *remaindItem = Item::CreateItem(ITEM_COINS_GOLD, gold);
        if(!this->addItem(remaindItem))
            g_game.addThing(NULL,this->pos,remaindItem);
    }

    return true;
}

bool Player::removeItemSmart(uint16_t itemid, int32_t count, bool depot)
{

    for (int32_t slot = 1; slot <= 10 && count > 0; slot++)
    {
        Item *item = items[slot];
        if (item)
        {
            Container *container = dynamic_cast<Container*>(item);
            if (item->getID() == itemid)
            {
                if (item->isStackable() && item->getItemCountOrSubtype()-count > 1)
                {
                    item->setItemCountOrSubtype(item->getItemCountOrSubtype()-count);
                    count = 0;
                }
                else if(item->isStackable() && item->getItemCountOrSubtype()-count <= 0)
                {
                    count -= item->getItemCountOrSubtype();
                    items[slot] = NULL;
                }
                else
                {
                    items[slot] = NULL;
                    count = 0;
                }
                sendInventory(slot);
            }
            else if (container)
                count = removeContainerItem(container, itemid, count);
        }
    }

    if(depot && count > 0)
    {
        for(DepotMap::iterator dit = depots.begin(); dit != depots.end(); ++dit)
        {
            Container* depotc= dynamic_cast<Container*>(dit->second);
            if(depotc)
                count = removeContainerItem(depotc, itemid, count);
        }
    }

    if (count <= 0)
        return true;
    else
        return false;
}

/*bool Player::removeItem(uint16_t id,int32_t count)
{
	if(getItemCount(id) < count)
		return false;

	std::list<Container*> stack;

	ContainerList::iterator it;
	for(int32_t i = 0; i < 11 && count > 0 ;i++){
		if(items[i]){
			if(items[i]->getID() == id){
				if(items[i]->isStackable()){
					if(items[i]->getItemCountOrSubtype() > count){
						items[i]->setItemCountOrSubtype((unsigned char)(items[i]->getItemCountOrSubtype() - count));
						sendInventory(i);
						count = 0;
					}
					else{
						count = count - items[i]->getItemCountOrSubtype();
						g_game.FreeThing(items[i]);
						removeItemInventory(i);
					}
				}
				else{
					count--;
					g_game.FreeThing(items[i]);
					removeItemInventory(i);
				}
			}
			else if(Container *tmpcontainer = dynamic_cast<Container*>(items[i])){
				stack.push_back(tmpcontainer);
			}
		}
	}

	while(!stack.empty() && count > 0){
		Container *container = stack.front();
		stack.pop_front();
		for(int32_t i = 0; i < container->size() && count > 0; i++){
			Item *item = container->getItem(i);
			if(item->getID() == id){
				if(item->isStackable()){
					if(item->getItemCountOrSubtype() > count){
						item->setItemCountOrSubtype((unsigned char)(item->getItemCountOrSubtype() - count));
						onItemUpdateContainer(container,item, i);
						count = 0;
					}
					else{
						count = count - item->getItemCountOrSubtype();
						g_game.FreeThing(item);
						onItemRemoveContainer(container,i);
						container->removeItem(item);
					}
				}
				else{
					count--;
					g_game.FreeThing(item);
					onItemRemoveContainer(container,i);
					container->removeItem(item);
				}
			}
			else if(dynamic_cast<Container*>(item)){
				stack.push_back(dynamic_cast<Container*>(item));
			}
		}
	}
	if(count == 0)
		return true;
	else
		return false;
}*/

int32_t Player::getItemCount(uint16_t id)
{
    uint32_t counter = 0;
    std::list<const Container*> stack;
    ContainerList::const_iterator cit;
    for(int32_t i=0; i< 11; i++)
    {
        if(items[i])
        {
            if(items[i]->getID() == id)
            {
                if(items[i]->isStackable())
                {
                    counter = counter + items[i]->getItemCountOrSubtype();
                }
                else
                {
                    counter++;
                }
            }
            if(Container *tmpcontainer = dynamic_cast<Container*>(items[i]))
            {
                stack.push_back(tmpcontainer);
            }
        }
    }

    while(!stack.empty())
    {
        const Container *container = stack.front();
        stack.pop_front();
        for(cit = container->getItems(); cit != container->getEnd(); ++cit)
        {
            if((*cit)->getID() == id)
            {
                if((*cit)->isStackable())
                {
                    counter = counter + (*cit)->getItemCountOrSubtype();
                }
                else
                {
                    counter++;
                }
            }
            Container *container = dynamic_cast<Container*>(*cit);
            if(container)
            {
                stack.push_back(container);
            }
        }
    }
    return counter;
}

void Player::sendIcons()
{
    int32_t icons = 0;
    if(inFightTicks >= 6000 || inFightTicks ==4000 || inFightTicks == 2000)
    {
        icons |= ICON_SWORDS;
    }
    if(manaShieldTicks >= 1000)
    {
        icons |= ICON_MANASHIELD;
    }
    if(conditions.hasCondition(ATTACK_DRUNKNESS)/*drunkTicks >= 1000*/)
    {
        icons |= ICON_DRUNK;
    }
    if(speed > getNormalSpeed())
    {
        icons |= ICON_HASTE;
    }
    if(conditions.hasCondition(ATTACK_FIRE) /*burningTicks >= 1000*/)
    {
        icons |= ICON_BURN | ICON_SWORDS;
    }
    if(conditions.hasCondition(ATTACK_ENERGY) /*energizedTicks >= 1000*/)
    {
        icons |= ICON_ENERGY | ICON_SWORDS;
    }
    if(conditions.hasCondition(ATTACK_POISON)/*poisonedTicks >= 1000*/)
    {
        icons |= ICON_POISON | ICON_SWORDS;
    }
    if(conditions.hasCondition(ATTACK_PARALYZE) /*speed < getNormalSpeed()*/ /*paralyzeTicks >= 1000*/)
    {
        icons |= ICON_PARALYZE | ICON_SWORDS;
    }

    client->sendIcons(icons);
}

void Player::updateInventoryWeigth()
{
    inventoryWeight = 0.00;
    if(access < g_config.ACCESS_PROTECT)
    {
        for(int32_t slotid = 0; slotid < 11; ++slotid)
        {
            if(getItem(slotid))
            {
                inventoryWeight += getItem(slotid)->getWeight();
            }
        }
    }
}

int32_t Player::sendInventory(unsigned char sl_id)
{
    client->sendInventory(sl_id);
    return true;
}

int32_t Player::addItemInventory(Item* item, int32_t pos, bool internal /*= false*/)
{
#ifdef __DEBUG__
    std::cout << "Should add item at " << pos <<std::endl;
#endif
    if(pos > 0 && pos < 11)
    {
        if (items[pos])
        {
            items[pos]->releaseThing();
        }

        items[pos] = item;
        if(items[pos])
        {
            items[pos]->pos.x = 0xFFFF;
        }

        updateInventoryWeigth();

        if(!internal)
        {
            client->sendStats();
            client->sendInventory(pos);
        }
    }
    else
        return false;

    return true;
}

bool Player::addItem(Item *item, bool test /*=false*/)
{
    if(!item)
        return false;

    if(access < g_config.ACCESS_PROTECT && getFreeCapacity() < item->getWeight())
    {
        return false;
    }

    Container *container;
    unsigned char slot;

    switch(getFreeSlot(&container,slot, item))
    {
    case SLOT_TYPE_NONE:
        return false;
        break;
    case SLOT_TYPE_INVENTORY:
        if(!test)
            addItemInventory(item,slot);
        return true;
        break;
    case SLOT_TYPE_CONTAINER:
        if(container->isHoldingItem(item) == true)
        {
            return false;
        }
        if(!test)
        {
            //add the item
            container->addItem(item);
            updateInventoryWeigth();
            client->sendStats();

            //update container
            client->sendItemAddContainer(container,item);
        }
        return true;
        break;
    }
    return false;
}

freeslot_t Player::getFreeSlot(Container **container,unsigned char &slot, const Item* item)
{
    *container = NULL;
    if(!(item->getSlotPosition() & SLOTP_TWO_HAND) || (!items[SLOT_RIGHT] && !items[SLOT_LEFT]))
    {
        //first look free slot in inventory
        if(!items[SLOT_RIGHT])
        {
            if(!(items[SLOT_LEFT] && (items[SLOT_LEFT]->getSlotPosition() & SLOTP_TWO_HAND)))
            {
                slot = SLOT_RIGHT;
                return SLOT_TYPE_INVENTORY;
            }
        }

        if(!items[SLOT_LEFT])
        {
            if(!(items[SLOT_RIGHT] && (items[SLOT_RIGHT]->getSlotPosition() & SLOTP_TWO_HAND)))
            {
                slot = SLOT_LEFT;
                return SLOT_TYPE_INVENTORY;
            }
        }
    }

    if(!items[SLOT_AMMO])
    {
        slot = SLOT_AMMO;
        return SLOT_TYPE_INVENTORY;
    }

    //look in containers
    for(int32_t i=0; i< 11; i++)
    {
        Container *tmpcontainer = dynamic_cast<Container*>(items[i]);
        if(tmpcontainer)
        {
            Container *container_freeslot = getFreeContainerSlot(tmpcontainer);
            if(container_freeslot)
            {
                *container = container_freeslot;
                return SLOT_TYPE_CONTAINER;
            }
        }
    }
    return SLOT_TYPE_NONE;
}

Container* Player::getFreeContainerSlot(Container *parent)
{
    if(parent == getTradeItem())
    {
        return NULL;
    }
    //check if it is full
    if(parent->size() < parent->capacity())
    {
        return parent;
    }
    else  //look for more containers inside
    {
        for(ContainerList::const_iterator cit = parent->getItems(); cit != parent->getEnd(); ++cit)
        {
            Container * temp_container = dynamic_cast<Container*>(*cit);
            if(temp_container)
            {
                return getFreeContainerSlot(temp_container);
            }
        }
    }
    return NULL;
}

bool Player::removeItem(Item* item, bool test /*=false*/)
{
    Container *tmpcontainer;
    //look for the item
    for(int32_t i=0; i< 11; i++)
    {
        if(item == items[i])
        {
            if(!test)
            {
                removeItemInventory(i);
            }
            return true;
        }
        else if(tmpcontainer = dynamic_cast<Container*>(items[i]))
        {
            if(internalRemoveItemContainer(tmpcontainer,item, test) == true)
            {
                return true;
            }
        }
    }
    return false;
}

bool Player::internalRemoveItemContainer(Container *parent, Item* item, bool test /*=false*/)
{
    Container * temp_container;
    for(ContainerList::const_iterator cit = parent->getItems();
            cit != parent->getEnd(); ++cit)
    {
        if(*cit == item)
        {
            unsigned char slot =  parent->getSlotNumberByItem(item);
            if(slot != 0xFF)
            {
                if(!test)
                {
                    parent->removeItem(item);
                    updateInventoryWeigth();
                    client->sendStats();
                    client->sendItemRemoveContainer(parent,slot);
                }
                return true;
            }
            else
            {
                return false;
            }
        }
        else if(temp_container = dynamic_cast<Container*>(*cit))
        {
            if(internalRemoveItemContainer(temp_container, item, test))
                return true;
        }
    }
    return false;
}

int32_t Player::removeItemInventory(int32_t pos, bool internal /*= false*/)
{
    if(pos > 0 && pos < 11)
    {

        if(items[pos])
        {
            items[pos] = NULL;
        }

        updateInventoryWeigth();

        if(!internal)
        {
            client->sendStats();
            client->sendInventory(pos);
        }
    }
    else
        return false;

    return true;
}

uint32_t Player::getReqSkillTries (int32_t skill, int32_t level, playervoc_t voc)
{
    //first find on cache
    for(int32_t i=0; i<2; i++)
    {
        if(SkillAdvanceCache[skill][i].level == level && SkillAdvanceCache[skill][i].vocation == voc)
        {
#ifdef __DEBUG__
            std::cout << "Skill cache hit: " << this->name << " " << skill << " " << level << " " << voc <<std::endl;
#endif
            return SkillAdvanceCache[skill][i].tries;
        }
    }
    // follows the order of enum skills_t
    uint16_t SkillBases[7] = { 50, 50, 50, 50, 30, 100, 20 };
    float SkillMultipliers[7][5] =
    {
        {1.5f, 1.5f, 1.5f, 1.2f, 1.1f},     // Fist
        {2.0f, 2.0f, 1.8f, 1.2f, 1.1f},     // Club
        {2.0f, 2.0f, 1.8f, 1.2f, 1.1f},     // Sword
        {2.0f, 2.0f, 1.8f, 1.2f, 1.1f},     // Axe
        {2.0f, 2.0f, 1.8f, 1.1f, 1.4f},     // Distance
        {1.5f, 1.5f, 1.5f, 1.1f, 1.1f},     // Shielding
        {1.1f, 1.1f, 1.1f, 1.1f, 1.1f}      // Fishing
    };
#ifdef __DEBUG__
    std::cout << "Skill cache miss: " << this->name << " "<< skill << " " << level << " " << voc <<std::endl;
#endif
    //update cache
    //remove minor level
    int32_t j;
    if(SkillAdvanceCache[skill][0].level > SkillAdvanceCache[skill][1].level)
    {
        j = 1;
    }
    else
    {
        j = 0;
    }
    SkillAdvanceCache[skill][j].level = level;
    SkillAdvanceCache[skill][j].vocation = voc;
    SkillAdvanceCache[skill][j].tries = (uint32_t) ( SkillBases[skill] * pow((float) SkillMultipliers[skill][voc], (float) ( level - 11) ) );
    return SkillAdvanceCache[skill][j].tries;
}

void Player::addSkillTry(int32_t skilltry)
{
    int32_t skill;
    bool foundSkill;
    foundSkill = false;
    std::string skillname;

    for (int32_t slot = SLOT_RIGHT; slot <= SLOT_LEFT; slot++)
    {
        if (items[slot])
        {
            if (items[slot]->isWeapon())
            {
                switch (items[slot]->getWeaponType())
                {
                case SWORD:
                    skill = 2;
                    break;
                case CLUB:
                    skill = 1;
                    break;
                case AXE:
                    skill = 3;
                    break;
                case DIST:
                    if(GetDistWeapon())
                        skill = 4;
                    else
                        skill = 0;
                    break;
                case SHIELD:
                    continue;
                    break;
                case MAGIC:
                    return;
                    break;//TODO: should add skill try?
                default:
                    skill = 0;
                    break;
                }//switch

                addSkillTryInternal(skilltry * (skill==4? g_config.DIST_MUL[vocation] : g_config.WEAPON_MUL[vocation]), skill);
                foundSkill = true;
                break;
            }
        }
    }
    if(foundSkill == false)
        addSkillTryInternal(skilltry * g_config.WEAPON_MUL[vocation],0);//add fist try
}

void Player::addSkillShieldTry(int32_t skilltry)
{

    skilltry *= g_config.SHIELD_MUL[vocation];

    //look for a shield
    for (int32_t slot = SLOT_RIGHT; slot <= SLOT_LEFT; slot++)
    {
        if (items[slot])
        {
            if (items[slot]->isWeapon())
            {
                if (items[slot]->getWeaponType() == SHIELD)
                {
                    addSkillTryInternal(skilltry,5);
                    break;
                }
            }
        }
    }
}

int32_t Player::getPlayerInfo(playerinfo_t playerinfo) const
{
    switch(playerinfo)
    {
    case PLAYERINFO_LEVEL:
        return level;
        break;
    case PLAYERINFO_LEVELPERCENT:
        return level_percent;
        break;
    case PLAYERINFO_MAGICLEVEL:
        return maglevel;
        break;
    case PLAYERINFO_MAGICLEVELPERCENT:
        return maglevel_percent;
        break;
    case PLAYERINFO_HEALTH:
        return health;
        break;
    case PLAYERINFO_MAXHEALTH:
        return healthmax;
        break;
    case PLAYERINFO_MANA:
        return mana;
        break;
    case PLAYERINFO_MAXMANA:
        return manamax;
        break;
    case PLAYERINFO_MANAPERCENT:
        return maglevel_percent;
        break;
    case PLAYERINFO_SOUL:
        return 100;
        break;
    default:
        return 0;
        break;
    }

    return 0;
}

int32_t Player::getSkill(skills_t skilltype, skillsid_t skillinfo) const
{
#ifdef YUR_RINGS_AMULETS
    if (skillinfo == SKILL_LEVEL && items[SLOT_RING])
    {
        int32_t id = items[SLOT_RING]->getID();

        if (skilltype == SKILL_FIST && id == ITEM_POWER_RING_IN_USE)
            return skills[skilltype][skillinfo] + 6;
        else if ((skilltype == SKILL_SWORD && id == ITEM_SWORD_RING_IN_USE) ||
                 (skilltype == SKILL_AXE && id == ITEM_AXE_RING_IN_USE) ||
                 (skilltype == SKILL_CLUB && id == ITEM_CLUB_RING_IN_USE))
            return skills[skilltype][skillinfo] + 4;
    }
#endif //YUR_RINGS_AMULETS
    return skills[skilltype][skillinfo];
}

std::string Player::getSkillName(int32_t skillid)
{
    std::string skillname;
    switch(skillid)
    {
    case 0:
        skillname = "walce piesciami";
        break;
    case 1:
        skillname = "walce obuchem";
        break;
    case 2:
        skillname = "walce mieczem";
        break;
    case 3:
        skillname = "walce toporem";
        break;
    case 4:
        skillname = "walce na dystans";
        break;
    case 5:
        skillname = "obronie";
        break;
    case 6:
        skillname = "lowieniu";
        break;
    default:
        skillname = "unknown";
        break;
    }
    return skillname;
}

void Player::addSkillTryInternal(int32_t skilltry,int32_t skill)
{

    skills[skill][SKILL_TRIES] += skilltry;
    //for skill level advances
    //int32_t reqTries = (int32_t) ( SkillBases[skill] * pow((float) VocMultipliers[skill][voc], (float) ( skills[skill][SKILL_LEVEL] - 10) ) );

    /* #if __DEBUG__
    	//for debug
    	cout << Creature::getName() << ", has the vocation: " << (int32_t)vocation << " and is training his " << getSkillName(skill) << "(" << skill << "). Tries: " << skills[skill][SKILL_TRIES] << "(" << getReqSkillTries(skill, (skills[skill][SKILL_LEVEL] + 1), vocation) << ")" << std::endl;
    	cout << "Current skill: " << skills[skill][SKILL_LEVEL] << std::endl;
    #endif */

    //Need skill up?
    if (skills[skill][SKILL_TRIES] >= getReqSkillTries(skill, (skills[skill][SKILL_LEVEL] + 1), vocation))
    {
#ifdef __MIZIAK_CREATURESCRIPTS__
        int32_t tab[] = {skill, skills[skill][SKILL_LEVEL], skills[skill][SKILL_LEVEL]+1};
        actions.creatureEvent("advance", this, NULL, NULL, tab);
#endif //__MIZIAK_CREATURESCRIPTS__
        skills[skill][SKILL_LEVEL]++;
        skills[skill][SKILL_TRIES] = 0;
        skills[skill][SKILL_PERCENT] = 0;
        std::stringstream advMsg;
        advMsg << "Awansowales w " << getSkillName(skill) << ".";
        client->sendTextMessage(MSG_ADVANCE, advMsg.str().c_str());
        client->sendSkills();
    }
    else
    {
        //update percent
        int32_t new_percent = (uint32_t)(100*(skills[skill][SKILL_TRIES])/(1.*getReqSkillTries(skill, (skills[skill][SKILL_LEVEL]+1), vocation)));

        if(skills[skill][SKILL_PERCENT] != new_percent)
        {
            skills[skill][SKILL_PERCENT] = new_percent;
            client->sendSkills();
        }
    }
}


uint32_t Player::getReqMana(int32_t maglevel, playervoc_t voc)
{
    //ATTENTION: MAKE SURE THAT CHARS HAVE REASONABLE MAGIC LEVELS. ESPECIALY KNIGHTS!!!!!!!!!!!
    float ManaMultiplier[5] = { 1.0f, 1.1f, 1.1f, 1.4f, 3};

    //will calculate required mana for a magic level
    uint32_t reqMana = (uint32_t) ( 400 * pow(ManaMultiplier[(int32_t)voc], maglevel-1) );

    if (reqMana % 20 < 10) //CIP must have been bored when they invented this odd rounding
        reqMana = reqMana - (reqMana % 20);
    else
        reqMana = reqMana - (reqMana % 20) + 20;

    return reqMana;
}

Container* Player::getContainer(uint16_t containerid)
{
    for(containerLayout::iterator cl = vcontainers.begin(); cl != vcontainers.end(); ++cl)
    {
        if(cl->first == containerid)
            return cl->second;
    }

    return NULL;
}

bool Player::isHoldingContainer(const Container* container) const
{
    const Container* topContainer = container->getTopParent();

    for(int32_t i = 0; i < 11; i++)
    {
        Container *container = dynamic_cast<Container*>(items[i]);
        if(container && topContainer == container)
        {
            return true;
        }
    }

    return false;
}

unsigned char Player::getContainerID(const Container* container) const
{
    for(containerLayout::const_iterator cl = vcontainers.begin(); cl != vcontainers.end(); ++cl)
    {
        if(cl->second == container)
            return cl->first;
    }

    return 0xFF;
}

void Player::addContainer(unsigned char containerid, Container *container)
{

    /* #ifdef __DEBUG__
    	cout << Creature::getName() << ", addContainer: " << (int32_t)containerid << std::endl;
    #endif */

    if(containerid > 0xF)
        return;

    for(containerLayout::iterator cl = vcontainers.begin(); cl != vcontainers.end(); ++cl)
    {
        if(cl->first == containerid)
        {
            cl->second = container;
            return;
        }
    }

    //id doesnt exist, create it
    containerItem vItem;
    vItem.first = containerid;
    vItem.second = container;

    vcontainers.push_back(vItem);
}

containerLayout::const_iterator Player::closeContainer(unsigned char containerid)
{
    for (containerLayout::iterator cl = vcontainers.begin(); cl != vcontainers.end(); ++cl)
        if (cl->first == containerid)
            return vcontainers.erase(cl);

    return vcontainers.end();
}

int32_t Player::getLookCorpse()
{
    if(sex == 0)
    {
        return ITEM_FEMALE_CORPSE;
    }
    else if(sex == 1)
    {
        return ITEM_MALE_CORPSE;
    }
    else if(sex == 3)
    {
        return ITEM_DWARF_CORPSE;
    }
    else if(sex == 4)
    {
        return ITEM_ELF_CORPSE;
    }
    else
    {
        return ITEM_MALE_CORPSE;
    }
}

void Player::dropLoot(Container *corpse)
{
#ifdef HUCZU_SKULLS
    if (skullType == SKULL_RED)
    {
        for (int32_t slot = 0; slot < 11; slot++)
        {
            if (items[slot])
            {
                corpse->addItem(items[slot]);
                items[slot] = NULL;
            }
        }
        return;
    }
#endif //HUCZU_SKULLS

    if (items[SLOT_NECKLACE] && items[SLOT_NECKLACE]->getID() == ITEM_AOL)
    {
        removeItemInventory(SLOT_NECKLACE);
        return;
    }
    if (items[SLOT_NECKLACE] && items[SLOT_NECKLACE]->getID() == ITEM_STAR_LIGHT)
    {
        removeItemInventory(SLOT_NECKLACE);
        starlight = true;
        return;
    }
#ifdef HUCZU_AMULET
    if (items[SLOT_NECKLACE] && items[SLOT_NECKLACE]->getID() == ITEM_TYMERIA_AMULET && items[SLOT_NECKLACE]->getCharges() > 0)
    {
        aol = true;
        items[SLOT_NECKLACE]->useCharge();
        return;
    }
#endif

    for (int32_t slot = 0; slot < 11; slot++)
    {
        Item *item = items[slot];
        if (!item)
            continue;

        if (dynamic_cast<Container*>(item))
        {
            if (random_range(1, 100) <= g_config.DIE_PERCENT_BP)
            {
                corpse->addItem(item);
                items[slot] = NULL;
            }
        }
        else if (random_range(1, 100) <= g_config.DIE_PERCENT_EQ)
        {
            corpse->addItem(item);
            items[slot] = NULL;
        }
    }
}

fight_t Player::getFightType()
{
    for (int32_t slot = SLOT_RIGHT; slot <= SLOT_LEFT; slot++)
    {
        if (items[slot])
        {
            if ((items[slot]->isWeapon()))
            {
                Item *DistItem;
                switch (items[slot]->getWeaponType())
                {
                case DIST:
                    DistItem = GetDistWeapon();
                    if(DistItem)
                    {
                        return FIGHT_DIST;
                    }
                    else
                    {
                        return FIGHT_MELEE;
                    }
                    break;
                case MAGIC:
                    return FIGHT_MAGICDIST;
                default:
                    break;
                }
            }
        }
    }
    return FIGHT_MELEE;
}

void Player::removeDistItem()
{
    Item *DistItem = GetDistWeapon();
    unsigned char sl_id;
    if(DistItem)
    {
        if(DistItem->isStackable() == false)
            return;

        if(DistItem == getTradeItem())
            g_game.playerCloseTrade(this);

        //remove one dist item
        unsigned char n = (unsigned char)DistItem->getItemCountOrSubtype();
        if(DistItem == items[SLOT_RIGHT])
        {
            sl_id = SLOT_RIGHT;
        }
        else if(DistItem == items[SLOT_LEFT])
        {
            sl_id = SLOT_LEFT;
        }
        else if(DistItem == items[SLOT_AMMO])
        {
            sl_id = SLOT_AMMO;
        }

        if(n > 1)
        {
            DistItem->setItemCountOrSubtype(n-1);
        }
        else
        {
            //remove the item
            items[sl_id] = NULL;
            DistItem->releaseThing();
            //delete DistItem;
        }

        updateInventoryWeigth();
        client->sendStats();

        //update inventory
        client->sendInventory(sl_id);
    }
    return;
}

subfight_t Player::getSubFightType()
{
    fight_t type = getFightType();
    if(type == FIGHT_DIST)
    {
        Item *DistItem = GetDistWeapon();
        if(DistItem)
        {
            return DistItem->getSubfightType();
        }
    }
    if(type == FIGHT_MAGICDIST)
    {
        Item *DistItem = GetDistWeapon();
        if(DistItem)
        {
            return DistItem->getSubfightType();
        }
    }
    return DIST_NONE;
}

Item * Player::GetDistWeapon() const
{
    for (int32_t slot = SLOT_RIGHT; slot <= SLOT_LEFT; slot++)
    {
        if (items[slot])
        {
            if ((items[slot]->isWeapon()))
            {
                switch (items[slot]->getWeaponType())
                {
                case DIST:
                    //find ammunition
                    if(items[slot]->getAmuType() == AMU_NONE)
                    {
                        return items[slot];
                    }
                    if(items[SLOT_AMMO])
                    {
                        //compare ammo types
                        if(items[SLOT_AMMO]->getWeaponType() == AMO &&
                                items[slot]->getAmuType() == items[SLOT_AMMO]->getAmuType())
                        {
                            return items[SLOT_AMMO];
                        }
                        else
                        {
                            return NULL;
                        }

                    }
                    else
                    {
                        return NULL;
                    }
                    break;

                case MAGIC:
                    return items[slot];
                default:
                    break;
                }//switch
            }//isweapon
        }//item[slot]
    }//for
    return NULL;
}

void Player::addStorageValue(const uint32_t key, const int32_t value)
{
    storageMap[key] = value;
}

bool Player::getStorageValue(uint32_t key, int32_t &value) const
{
    StorageMap::const_iterator it;
    it = storageMap.find(key);
    if(it != storageMap.end())
    {
        value = it->second;
        return true;
    }
    else
    {
        value = 0;
        return false;
    }
}

bool Player::CanSee(int32_t x, int32_t y, int32_t z) const
{
    return client->CanSee(x, y, z);
}

void Player::setAcceptTrade(bool b)
{
    if(b)
    {
        tradeState = TRADE_ACCEPT;
        //acceptTrade = true;
    }
    else
    {
        tradeItem = NULL;
        tradePartner = 0;
        tradeState = TRADE_NONE;
        //acceptTrade = false;
    }
}

Container* Player::getDepot(uint32_t depotId)
{
    DepotMap::iterator it = depots.find(depotId);
    if (it != depots.end())
    {
        return it->second;
    }
    return NULL;
}

bool Player::addDepot(Container* depot,uint32_t depotId)
{
    Container *bdep = getDepot(depotId);
    if(bdep)
        return false;

    depot->pos.x = 0xFFFF;
    depot->depot = depotId;

    depots[depotId] = depot;
    return true;
}

void Player::sendCancel(const char *msg) const
{
    client->sendCancel(msg);
}
void Player::sendChangeSpeed(Creature* creature)
{
    client->sendChangeSpeed(creature);
}

void Player::sendToChannel(Creature *creature,SpeakClasses type, const std::string &text, uint16_t channelId)
{
    if(this)
        client->sendToChannel(creature, type, text, channelId);
}

void Player::sendCancelAttacking()
{
    attackedCreature = 0;
    client->sendCancelAttacking();
}

void Player::sendCancelWalk() const
{
    client->sendCancelWalk();
}

void Player::sendStats()
{
    //update level and maglevel percents
    if(lastSentStats.experience != this->experience || lastSentStats.level != this->level)
        level_percent  = (unsigned char)(100*(experience-getExpForLv(level))/(1.*getExpForLv(level+1)-getExpForLv(level)));

    if(lastSentStats.manaspent != this->manaspent || lastSentStats.maglevel != this->maglevel)
        maglevel_percent  = (unsigned char)(100*(manaspent/(1.*getReqMana(maglevel+1,vocation))));

    //save current stats
    lastSentStats.health = this->health;
    lastSentStats.healthmax = this->healthmax;
    lastSentStats.freeCapacity = this->getFreeCapacity();
    //lastSentStats.capacity = this->capacity;
    //lastSentStats.cap = this->cap;
    lastSentStats.experience = this->experience;
    lastSentStats.level = this->level;
    lastSentStats.mana = this->mana;
    lastSentStats.manamax = this->manamax;
    lastSentStats.manaspent = this->manaspent;
    lastSentStats.maglevel = this->maglevel;

    client->sendStats();
}

void Player::sendTextMessage(MessageClasses mclass, const char* message) const
{
    client->sendTextMessage(mclass,message);
}

void Player::flushMsg()
{
    client->flushOutputBuffer();
}
void Player::sendTextMessage(MessageClasses mclass, const char* message,const Position &pos, unsigned char type) const
{
    client->sendTextMessage(mclass,message,pos,type);
}

/*void Player::sendPing(){
	internal_ping++;
	if(internal_ping >= 5){ //1 ping each 5 seconds
		internal_ping = 0;
		npings++;
		client->sendPing();
	}
	if(npings >= 6){
		//std::cout << "logout" << std::endl;
		if(inFightTicks >=1000 && health >0) {
			//logout?
			//client->logout();
		}
		else{
			//client->logout();
		}
	}
}*/
void Player::sendPing(uint32_t interval)
{
    internal_ping = internal_ping + interval;
    if(internal_ping >= 5000)  //1 ping each 5 seconds
    {
        internal_ping = 0;
        npings++;
        client->sendPing();
    }
    if(npings >= 6)
    {
        //std::cout << "logout" << std::endl;
        if(inFightTicks >=1000 && health > 0)
        {
            //logout?
            //client->logout();
        }
        else
        {
            //client->logout();
        }
    }
}

void Player::receivePing()
{
    if(isRemoved == false)
    {
        return;
    }
    if(npings > 0)
        npings--;
}

void Player::sendDistanceShoot(const Position &from, const Position &to, unsigned char type)
{
    client->sendDistanceShoot(from, to,type);
}

void Player::sendMagicEffect(const Position &pos, unsigned char type)
{
    client->sendMagicEffect(pos,type);
}
void Player::sendAnimatedText(const Position &pos, unsigned char color, std::string text)
{
    if(this)
        client->sendAnimatedText(pos,color,text);
}

void Player::sendCreatureHealth(const Creature *creature)
{
    client->sendCreatureHealth(creature);
}

void Player::sendTradeItemRequest(const Player* player, const Item* item, bool ack)
{
    client->sendTradeItemRequest(player, item, ack);
}

void Player::sendCloseTrade()
{
    client->sendCloseTrade();
}

void Player::sendTextWindow(Item* item,const uint16_t maxlen, const bool canWrite)
{
    client->sendTextWindow(item,maxlen,canWrite);
}

void Player::sendCloseContainer(unsigned char containerid)
{
    client->sendCloseContainer(containerid);
}

void Player::sendClosePrivate(uint16_t channelId)
{
    client->sendClosePrivate(channelId);
}
void Player::sendContainer(unsigned char index, Container *container)
{
    client->sendContainer(index,container);
}

bool Player::NeedUpdateStats()
{
    if(lastSentStats.health != this->health ||
            lastSentStats.healthmax != this->healthmax ||
            //lastSentStats.cap != this->cap ||
            //(int32_t)lastSentStats.capacity != (int32_t)this->capacity ||
            (int32_t)lastSentStats.freeCapacity != (int32_t)this->getFreeCapacity() ||
            lastSentStats.experience != this->experience ||
            lastSentStats.level != this->level ||
            lastSentStats.mana != this->mana ||
            lastSentStats.manamax != this->manamax ||
            lastSentStats.manaspent != this->manaspent ||
            lastSentStats.maglevel != this->maglevel)
    {
        return true;
    }
    else
    {
        return false;
    }
}


#ifdef HUCZU_FOLLOW
void Player::onThingMove(const Creature *creature, const Thing *thing, const Position *oldPos,
                         unsigned char oldstackpos, unsigned char oldcount, unsigned char count)
{
    const Creature *constantCreature = dynamic_cast<const Creature*>(thing);
    if(constantCreature && constantCreature->getID() == this->followCreature)
    {
        g_game.playerFollow(this, const_cast<Creature*>(constantCreature));
    }
    client->sendThingMove(creature, thing, oldPos, oldstackpos, oldcount, count);
}
#else
void Player::onThingMove(const Creature *creature, const Thing *thing, const Position *oldPos,
                         unsigned char oldstackpos, unsigned char oldcount, unsigned char count)
{
    client->sendThingMove(creature, thing, oldPos, oldstackpos, oldcount, count);
}
#endif //HUCZU_FOLLOW

//container to container
void Player::onThingMove(const Creature *creature, const Container *fromContainer, uint16_t from_slotid,
                         const Item* fromItem, int32_t oldFromCount, Container *toContainer, uint16_t to_slotid,
                         const Item *toItem, int32_t oldToCount, int32_t count)
{
    client->sendThingMove(creature, fromContainer, from_slotid, fromItem, oldFromCount, toContainer, to_slotid, toItem, oldToCount, count);
}

//inventory to container
void Player::onThingMove(const Creature *creature, slots_t fromSlot, const Item* fromItem,
                         int32_t oldFromCount, const Container *toContainer, uint16_t to_slotid, const Item *toItem, int32_t oldToCount, int32_t count)
{
#ifdef YUR_RINGS_AMULETS
    if (fromSlot == SLOT_RING)
    {
        const_cast<Item*>(fromItem)->removeGlimmer();
        client->sendSkills();
    }
#endif //YUR_RINGS_AMULETS
    client->sendThingMove(creature, fromSlot, fromItem, oldFromCount, toContainer, to_slotid, toItem, oldToCount, count);
#ifdef YUR_BOH
    if (fromSlot == SLOT_FEET)
        checkBoh();
#endif //YUR_BOH
}

//inventory to inventory
void Player::onThingMove(const Creature *creature, slots_t fromSlot, const Item* fromItem,
                         int32_t oldFromCount, slots_t toSlot, const Item* toItem, int32_t oldToCount, int32_t count)
{
#ifdef YUR_RINGS_AMULETS
    if (fromSlot == SLOT_RING)
    {
        const_cast<Item*>(fromItem)->removeGlimmer();
        client->sendSkills();
    }
    else if (toSlot == SLOT_RING)
    {
        const_cast<Item*>(fromItem)->setGlimmer();
        client->sendSkills();
    }
#endif //YUR_RINGS_AMULETS
    client->sendThingMove(creature, fromSlot, fromItem, oldFromCount, toSlot, toItem, oldToCount, count);
#ifdef YUR_BOH
    if (fromSlot == SLOT_FEET || toSlot == SLOT_FEET)
        checkBoh();
#endif //YUR_BOH
}

//container to inventory
void Player::onThingMove(const Creature *creature, const Container *fromContainer, uint16_t from_slotid,
                         const Item* fromItem, int32_t oldFromCount, slots_t toSlot, const Item *toItem, int32_t oldToCount, int32_t count)
{
#ifdef YUR_RINGS_AMULETS
    if (toSlot == SLOT_RING)
    {
        const_cast<Item*>(fromItem)->setGlimmer();
        client->sendSkills();
    }
#endif //YUR_RINGS_AMULETS
    client->sendThingMove(creature, fromContainer, from_slotid, fromItem, oldFromCount, toSlot, toItem, oldToCount, count);
#ifdef YUR_BOH
    if (toSlot == SLOT_FEET)
        checkBoh();
#endif //YUR_BOH
}

//container to ground
void Player::onThingMove(const Creature *creature, const Container *fromContainer, unsigned char from_slotid,
                         const Item* fromItem, int32_t oldFromCount, const Position &toPos, const Item *toItem, int32_t oldToCount, int32_t count)
{
    client->sendThingMove(creature, fromContainer, from_slotid, fromItem, oldFromCount, toPos, toItem, oldToCount, count);
}

//inventory to ground
void Player::onThingMove(const Creature *creature, slots_t fromSlot,
                         const Item* fromItem, int32_t oldFromCount, const Position &toPos, const Item *toItem, int32_t oldToCount, int32_t count)
{
#ifdef YUR_RINGS_AMULETS
    if (fromSlot == SLOT_RING)
    {
        const_cast<Item*>(fromItem)->removeGlimmer();
        client->sendSkills();
    }
#endif //YUR_RINGS_AMULETS
    client->sendThingMove(creature, fromSlot, fromItem, oldFromCount, toPos, toItem, oldToCount, count);
#ifdef YUR_BOH
    if (fromSlot == SLOT_FEET)
        checkBoh();
#endif //YUR_BOH
}

//ground to container
void Player::onThingMove(const Creature *creature, const Position &fromPos, int32_t stackpos, const Item* fromItem,
                         int32_t oldFromCount, const Container *toContainer, unsigned char to_slotid, const Item *toItem, int32_t oldToCount, int32_t count)
{
    client->sendThingMove(creature, fromPos, stackpos, fromItem, oldFromCount, toContainer, to_slotid, toItem, oldToCount, count);
}

//ground to inventory
void Player::onThingMove(const Creature *creature, const Position &fromPos, int32_t stackpos, const Item* fromItem,
                         int32_t oldFromCount, slots_t toSlot, const Item *toItem, int32_t oldToCount, int32_t count)
{
#ifdef YUR_RINGS_AMULETS
    if (toSlot == SLOT_RING)
    {
        const_cast<Item*>(fromItem)->setGlimmer();
        client->sendSkills();
    }
#endif //YUR_RINGS_AMULETS
    client->sendThingMove(creature, fromPos, stackpos, fromItem, oldFromCount, toSlot, toItem, oldToCount, count);
#ifdef YUR_BOH
    if (toSlot == SLOT_FEET)
        checkBoh();
#endif //YUR_BOH
}

/*
void Player::setAttackedCreature(uint32_t id)
{
  attackedCreature = id;
}
*/

void Player::onCreatureAppear(const Creature *creature)
{
    client->sendThingAppear(creature);
}

void Player::onCreatureDisappear(const Creature *creature, unsigned char stackPos, bool tele /*= false*/)
{
    client->sendThingDisappear(creature, stackPos, tele);
}

void Player::onThingAppear(const Thing* thing)
{
    client->sendThingAppear(thing);
}

void Player::onThingTransform(const Thing* thing,int32_t stackpos)
{
    client->sendThingTransform(thing,stackpos);
}

void Player::onThingDisappear(const Thing* thing, unsigned char stackPos)
{
    client->sendThingDisappear(thing, stackPos, false);
}
//auto-close containers
void Player::onThingRemove(const Thing* thing)
{
    client->sendThingRemove(thing);
}

void Player::onItemAddContainer(const Container* container,const Item* item)
{
    client->sendItemAddContainer(container,item);
}

void Player::onItemRemoveContainer(const Container* container,const unsigned char slot)
{
    client->sendItemRemoveContainer(container,slot);
}

void Player::onItemUpdateContainer(const Container* container,const Item* item,const unsigned char slot)
{
    client->sendItemUpdateContainer(container,item,slot);
}

void Player::onCreatureTurn(const Creature *creature, unsigned char stackPos)
{
    client->sendCreatureTurn(creature, stackPos);
}

void Player::onCreatureSay(const Creature *creature, SpeakClasses type, const std::string &text)
{
    /*if(text.length() > 256)
       client->sendCreatureSay(creature, type, "Tekst jest za dlugi.");
    else */
    client->sendCreatureSay(creature, type, text);
}

void Player::onCreatureChangeOutfit(const Creature* creature)
{
    client->sendSetOutfit(creature);
}

int32_t Player::onThink(int32_t& newThinkTicks)
{
    newThinkTicks = 1000;
    return newThinkTicks;
}

void Player::onTileUpdated(const Position &pos)
{
    client->sendTileUpdated(pos);
}

void Player::onTeleport(const Creature *creature, const Position *oldPos, unsigned char oldstackpos)
{
    client->sendThingMove(creature, creature,oldPos, oldstackpos, true, 1, 1);
}

void Player::addManaSpent(uint32_t spent)
{
    if(spent == 0)
        return;
    if(this->maglevel+1 == 15 && this->vocation == VOCATION_KNIGHT)
        return;

    spent *= g_config.MANA_MUL[vocation];

    this->manaspent += spent;
    //Magic Level Advance
    int32_t reqMana = this->getReqMana(this->maglevel+1, this->vocation);
    if (this->access < g_config.ACCESS_PROTECT && this->manaspent >= reqMana)
    {
        this->manaspent -= reqMana;
#ifdef __MIZIAK_CREATURESCRIPTS__
        int32_t tab[] = {7, this->maglevel, this->maglevel+1};
        actions.creatureEvent("advance", this, NULL, NULL, tab);
#endif //__MIZIAK_CREATURESCRIPTS__
        this->maglevel++;
        std::stringstream MaglvMsg;
        MaglvMsg << "Awansowales z poziomu magicznego " << (this->maglevel - 1) << " na poziom " << this->maglevel << ".";
        this->sendTextMessage(MSG_ADVANCE, MaglvMsg.str().c_str());
        this->sendStats();
    }
    //End Magic Level Advance*/
}

void Player::addExp(exp_t exp)
{
    this->experience += exp;
    int32_t lastLv = this->level;
    while (this->experience >= this->getExpForLv(this->level+1))
    {
        this->level++;
        this->healthmax += g_config.HP_GAIN[(int32_t)vocation];
        this->health += g_config.HP_GAIN[(int32_t)vocation];
        this->manamax += g_config.MANA_GAIN[(int32_t)vocation];
        this->mana += g_config.MANA_GAIN[(int32_t)vocation];
        this->capacity += g_config.CAP_GAIN[(int32_t)vocation];
    }
    if(lastLv != this->level)
    {
        this->setNormalSpeed();
        g_game.changeSpeed(this->getID(), this->getSpeed());
#ifdef __MIZIAK_CREATURESCRIPTS__
        int32_t tab[] = {8, lastLv, level};
        actions.creatureEvent("advance", this, NULL, NULL, tab);
#endif //__MIZIAK_CREATURESCRIPTS__
        std::stringstream lvMsg;
        lvMsg << "Awansowales z poziomu " << lastLv << " na poziom " << level << ".";
        this->sendTextMessage(MSG_ADVANCE,lvMsg.str().c_str());
        this->sendStats();
    }
}

uint32_t Player::getIP() const
{
    return client->getIP();
}

void Player::die()
{

#ifdef HUCZU_SKULLS
    if(skullType != SKULL_RED)
    {
        skullType = SKULL_NONE;
        skullTicks = 0;
        inFightTicks = 0;
    }
#endif
    if(aol)
        return;
    //Magic Level downgrade
    uint32_t sumMana = 0;
    int32_t lostMana = 0;
    for (int32_t i = 1; i <= maglevel; i++)                //sum up all the mana
    {
        sumMana += getReqMana(i, vocation);
    }

    sumMana += manaspent;
    if (items[SLOT_NECKLACE] && items[SLOT_NECKLACE]->getID() == ITEM_STAR_LIGHT)
        lostMana = (int32_t)(sumMana * g_config.DIE_PERCENT_SL/100.0);
    else
        lostMana = (int32_t)(sumMana * g_config.DIE_PERCENT_MANA/100.0);   //player loses 10% of all spent mana when he dies

    while(lostMana > manaspent)
    {
        lostMana -= manaspent;
        manaspent = getReqMana(maglevel, vocation);
        maglevel--;
    }
    manaspent -= lostMana;
    //End Magic Level downgrade

    //Skill loss
    int32_t lostSkillTries;
    uint32_t sumSkillTries;
    for (int32_t i = 0; i <= 6; i++)    //for each skill
    {
        lostSkillTries = 0;         //reset to 0
        sumSkillTries = 0;

        for (unsigned c = 11; c <= skills[i][SKILL_LEVEL]; c++)   //sum up all required tries for all skill levels
        {
            sumSkillTries += getReqSkillTries(i, c, vocation);
        }

        sumSkillTries += skills[i][SKILL_TRIES];
        if (items[SLOT_NECKLACE] && items[SLOT_NECKLACE]->getID() == ITEM_STAR_LIGHT)
            lostSkillTries = (int32_t) (sumSkillTries * g_config.DIE_PERCENT_SL/100.0);           //player loses 10% of his skill tries
        else
            lostSkillTries = (int32_t) (sumSkillTries * g_config.DIE_PERCENT_SKILL/100.0);

        while(lostSkillTries > (int32_t)skills[i][SKILL_TRIES])
        {
            lostSkillTries -= skills[i][SKILL_TRIES];
            skills[i][SKILL_TRIES] = getReqSkillTries(i, skills[i][SKILL_LEVEL], vocation);
            if(skills[i][SKILL_LEVEL] > 10)
            {
                skills[i][SKILL_LEVEL]--;
            }
            else
            {
                skills[i][SKILL_LEVEL] = 10;
                skills[i][SKILL_TRIES] = 0;
                lostSkillTries = 0;
                break;
            }
        }
        skills[i][SKILL_TRIES] -= lostSkillTries;
    }
    //End Skill loss

    //Level Downgrade
    int32_t newLevel = level;
    while((experience - getLostExperience()) < getExpForLv(newLevel)) //0.1f is also used in die().. maybe we make a little function for exp-loss?
    {
        if(newLevel > 1)
            newLevel--;
        else
            break;
    }

    if(newLevel != level)
    {
        std::stringstream lvMsg;
        lvMsg << "Zostales zdegradowany z poziomu " << level << " na poziom " << newLevel << ".";
        client->sendTextMessage(MSG_ADVANCE, lvMsg.str().c_str());
    }
}

void Player::preSave()
{
    if (health <= 0)
    {
        health = healthmax;
        pos.x = masterPos.x;
        pos.y = masterPos.y;
        pos.z = masterPos.z;

        if(!aol)
            experience -= getLostExperience();
        else
            aol = false;
//(int32_t)(experience*0.1f);        //0.1f is also used in die().. maybe we make a little function for exp-loss?

        while(experience < getExpForLv(level))
        {
            if(level > 1)
                level--;
            else
                break;

            // This checks (but not the downgrade sentences) aren't really necesary cause if the
            // player has a "normal" hp,mana,etc when he gets level 1 he will not lose more
            // hp,mana,etc... but here they are :P
            if ((healthmax -= g_config.HP_GAIN[(int32_t)vocation]) < 0) //This could be avoided with a proper use of unsigend int32_t
                healthmax = 0;

            health = healthmax;

            if ((manamax -= g_config.MANA_GAIN[(int32_t)vocation]) < 0) //This could be avoided with a proper use of unsigend int32_t
                manamax = 0;

            mana = manamax;

            if ((capacity -= g_config.CAP_GAIN[(int32_t)vocation]) < 0) //This could be avoided with a proper use of unsigend int32_t
                capacity = 0.0;
        }
    }
}

void Player::kickPlayer()
{
    client->logout();
}

bool Player::gainManaTick()
{
	int add;
	manaTick++;
	if(vocation >= 0 && vocation < 5)
	{
#ifdef YUR_PREMIUM_PROMOTION
		if (promoted)
		{
			if(manaTick < promotedGainManaVector[vocation][0])
				return false;
			else if (healthTick < (promotedGainHealthVector[vocation][0] - ((promotedGainHealthVector[vocation][0] * 25) / 100)) && items[SLOT_BACKPACK] && (items[SLOT_BACKPACK]->getID()!= ITEM_MAGIC_BACKPACK || items[SLOT_BACKPACK]->getID() != ITEM_MAGIC_BACKPACK))
                return false;
		else if (healthTick < (promotedGainHealthVector[vocation][0] - ((promotedGainHealthVector[vocation][0] * 25) / 100)) && items[SLOT_FEET] && (items[SLOT_FEET]->getID()!= ITEM_SOFT_BOOTS || items[SLOT_FEET]->getID() != ITEM_SOFT_BOOTS))
                return false;
                		else if (healthTick < (promotedGainHealthVector[vocation][0] - ((promotedGainHealthVector[vocation][0] * 25) / 100)) && items[SLOT_FEET] && (items[SLOT_FEET]->getID()!= ITEM_KLAP_BOOTS || items[SLOT_FEET]->getID() != ITEM_KLAP_BOOTS))
                return false;
			manaTick = 0;
			add = promotedGainManaVector[vocation][1];
		}
		else
#endif //YUR_PREMIUM_PROMOTION
		{
			if(manaTick < gainManaVector[vocation][0])
				return false;
	            else if (manaTick < (gainManaVector[vocation][0] - ((gainManaVector[vocation][0] * 25) / 100)) && items[SLOT_BACKPACK] && (items[SLOT_BACKPACK]->getID()!= ITEM_MAGIC_BACKPACK || items[SLOT_BACKPACK]->getID() != ITEM_MAGIC_BACKPACK))
                 return false;
			else if (manaTick < (gainManaVector[vocation][0] - ((gainManaVector[vocation][0] * 25) / 100)) && items[SLOT_FEET] && (items[SLOT_FEET]->getID()!= ITEM_SOFT_BOOTS || items[SLOT_FEET]->getID() != ITEM_SOFT_BOOTS))
                 return false;
                 			else if (manaTick < (gainManaVector[vocation][0] - ((gainManaVector[vocation][0] * 25) / 100)) && items[SLOT_FEET] && (items[SLOT_FEET]->getID()!= ITEM_KLAP_BOOTS || items[SLOT_FEET]->getID() != ITEM_KLAP_BOOTS))
                 return false;
			manaTick = 0;
			add = gainManaVector[vocation][1];
		}
	}
	else{
		add = 5;
	}

    if(vocation != 0)
        mana += min(add * g_config.MANA_TICK_MUL, manamax - mana);
    else
        mana += min(add * g_config.MANA_TICK_MUL_ROOK, manamax - mana);


    return true;
}

bool Player::gainHealthTick()
{
    int32_t add;
    healthTick++;
    if(vocation >= 0 && vocation < 5)
    {
#ifdef YUR_PREMIUM_PROMOTION
        if (promoted)
        {
            if(healthTick < promotedGainHealthVector[vocation][0])
                return false;
            healthTick = 0;
            add = promotedGainHealthVector[vocation][1];
        }
        else
#endif //YUR_PREMIUM_PROMOTION
        {
            if(healthTick < gainHealthVector[vocation][0])
                return false;
            healthTick = 0;
            add = gainHealthVector[vocation][1];
        }
    }
    else
    {
        add = 5;
    }

    if(vocation != 0)
        health += min(add * g_config.HEALTH_TICK_MUL, healthmax - health);
    else
        health += min(add * g_config.HEALTH_TICK_MUL_ROOK, healthmax - health);

    return true;
}

void Player::removeList()
{
    listPlayer.removeList(getID());
    for(AutoList<Player>::listiterator it = Player::listPlayer.list.begin(); it != Player::listPlayer.list.end(); ++it)
        (*it).second->notifyLogOut(this);
}

void Player::addList()
{
    for(AutoList<Player>::listiterator it = Player::listPlayer.list.begin(); it != Player::listPlayer.list.end(); ++it)
        (*it).second->notifyLogIn(this);

    listPlayer.addList(this);
}

bool Player::getCoins(uint32_t requiredcoins)
{
    uint32_t coins = 0;
    for(int32_t slot = 1; slot <= 10; slot++)
    {
        Item *item = items[slot];
        if (item)
        {
            Container *container = dynamic_cast<Container*>(item);
            if (item->getID() == ITEM_COINS_GOLD)
            {
                coins += item->getItemCountOrSubtype();
            }
            else if(item->getID() == ITEM_COINS_PLATINUM)
            {
                coins += 100*item->getItemCountOrSubtype();
            }
            else if(item->getID() == ITEM_COINS_CRYSTAL)
            {
                coins += 10000*item->getItemCountOrSubtype();
            }
            else if(item->getID() == ITEM_COINS_SCARAB)
            {
                coins += 1000000*item->getItemCountOrSubtype();
            }
            else if(container)
            {
                coins = getContainerCoins(container, coins);
            }
        }
    }
    if (coins >= requiredcoins)
    {
        return true;
    }
    else
    {
        return false;
    }
}

uint32_t Player::getContainerCoins(Container* container, uint32_t coins)
{
    for(int32_t number = container->size()-1; number >= 0; number--)
    {
        Container *subcontainer = dynamic_cast<Container*>(container->getItem(number));
        if (container->getItem(number)->getID() == ITEM_COINS_GOLD)
        {
            coins += container->getItem(number)->getItemCountOrSubtype();
        }
        else if(container->getItem(number)->getID() == ITEM_COINS_PLATINUM)
        {
            coins += 100*container->getItem(number)->getItemCountOrSubtype();
        }
        else if(container->getItem(number)->getID() == ITEM_COINS_CRYSTAL)
        {
            coins += 10000*container->getItem(number)->getItemCountOrSubtype();
        }
        else if(container->getItem(number)->getID() == ITEM_COINS_SCARAB)
        {
            coins += 1000000*container->getItem(number)->getItemCountOrSubtype();
        }
        else if(subcontainer)
        {
            coins = getContainerCoins(subcontainer, coins);
        }
    }
    return coins;
}

bool Player::getItem(int32_t itemid, int32_t count)
{
    for(int32_t slot = 1; slot <= 10; slot++)
    {
        if (count <= 0)
        {
            return true;
        }
        Item *item = items[slot];
        if (item)
        {
            Container *container = dynamic_cast<Container*>(item);
            if (item->getID() == itemid)
            {
                if (item->isStackable())
                {
                    count -= item->getItemCountOrSubtype();
                }
                else
                {
                    return true;
                }
            }
            else if(container)
            {
                count = getContainerItem(container, itemid, count);
            }
        }
    }
    return false;
}

int32_t Player::getContainerItem(Container* container, int32_t itemid, int32_t count)
{
    for(int32_t number = container->size()-1; number >= 0; number--)
    {
        Container *subcontainer = dynamic_cast<Container*>(container->getItem(number));
        if (container->getItem(number)->getID() == itemid)
        {
            if (container->getItem(number)->isStackable())
            {
                count -= container->getItem(number)->getItemCountOrSubtype();
            }
            else
            {
                count = 0;
                return count;
            }
        }
        else if(subcontainer)
        {
            Container *subcontainer = dynamic_cast<Container*>(container->getItem(number));
            count = getContainerItem(subcontainer, itemid, count);
        }
    }
    return count;
}

bool Player::removeCoins(int32_t cost)
{
    int32_t value;
    for(int32_t slot = 0; slot <= 10; slot++)
    {
        Item *item = items[slot];
        if (item && cost > 0)
        {
            Container *container = dynamic_cast<Container*>(item);
            if (item->getID() == ITEM_COINS_GOLD || item->getID() == ITEM_COINS_PLATINUM || item->getID() == ITEM_COINS_CRYSTAL || item->getID() == ITEM_COINS_SCARAB)
            {
                value = 1;
                if (item->getID() == ITEM_COINS_PLATINUM)
                {
                    value = 100;
                }
                if(item->getID() == ITEM_COINS_CRYSTAL)
                {
                    value = 10000;
                }
                else if(item->getID() == ITEM_COINS_SCARAB)
                {
                    value = 1000000;
                }
                if (item->getItemCountOrSubtype()*value > cost)
                {
                    int32_t worth = (cost/value);
                    if((double)(cost/value) <= 1)
                    {
                        worth = 1;
                    }
                    int32_t newcost = -1*(value*(item->getItemCountOrSubtype()-(item->getItemCountOrSubtype()-worth))-cost);
                    if (item->getItemCountOrSubtype()-worth > 0)
                    {
                        item->setItemCountOrSubtype(item->getItemCountOrSubtype()-worth);
                    }
                    else
                    {
                        items[slot] = NULL;
                    }
                    cost = newcost;
                    if (cost < 0)
                    {
                        payBack(cost*-1);
                        cost = 0;
                    }
                }
                else if(item->getItemCountOrSubtype()*value <= cost)
                {
                    cost -= item->getItemCountOrSubtype()*value;
                    items[slot] = NULL;
                }
                sendInventory(slot);
            }
            else if(container)
            {
                cost = removeContainerCoins(container, cost);
            }
        }
    }
    if (cost > 0)
    {
        if(!removeCoins(cost))
        {
            return false;
        }
    }
    return true;
}

int32_t Player::removeContainerCoins(Container* container, int32_t cost)
{
    int32_t value;
    for(int32_t number = 0; number < container->size(); number++)
    {
        if (cost <= 0)
        {
            return cost;
        }
        Item *item = container->getItem(number);
        Container *subcontainer = dynamic_cast<Container*>(item);
        if (item->getID() == ITEM_COINS_GOLD || item->getID() == ITEM_COINS_PLATINUM || item->getID() == ITEM_COINS_CRYSTAL || item->getID() == ITEM_COINS_SCARAB)
        {
            value = 1;
            if (item->getID() == ITEM_COINS_PLATINUM)
            {
                value = 100;
            }
            if(item->getID() == ITEM_COINS_CRYSTAL)
            {
                value = 10000;
            }
            else if(item->getID() == ITEM_COINS_SCARAB)
            {
                value = 1000000;
            }
            if (item->getItemCountOrSubtype()*value > cost)
            {
                int32_t worth = (cost/value);
                if((double)(cost/value) <= 1)
                {
                    worth = 1;
                }
                int32_t newcost = -1*(value*(item->getItemCountOrSubtype()-(item->getItemCountOrSubtype()-worth))-cost);
                if (item->getItemCountOrSubtype()-worth > 0)
                {
                    item->setItemCountOrSubtype(item->getItemCountOrSubtype()-worth);
                    onItemUpdateContainer(container, item, number);
                }
                else
                {
                    container->removeItem(item);
                    onItemRemoveContainer(container, number);
                    number--;
                }
                cost = newcost;
                if (cost < 0)
                {
                    payBack(cost*-1);
                    cost = 0;
                }
            }
            else if(item->getItemCountOrSubtype()*value <= cost)
            {
                cost -= item->getItemCountOrSubtype()*value;
                container->removeItem(item);
                onItemRemoveContainer(container, number);
                number--;
            }
        }
        else if(subcontainer)
        {
            cost = removeContainerCoins(subcontainer, cost);
        }
    }
    return cost;
}

bool Player::removeItem(int32_t itemid, int32_t count)
{
    for (int32_t slot = 1; slot <= 10 && count > 0; slot++)
    {
        Item *item = items[slot];
        if (item)
        {
            Container *container = dynamic_cast<Container*>(item);
            if (item->getID() == itemid)
            {
                if (item->isStackable() && item->getItemCountOrSubtype()-count > 1)
                {
                    item->setItemCountOrSubtype(item->getItemCountOrSubtype()-count);
                    count = 0;
                }
                else if(item->isStackable() && item->getItemCountOrSubtype()-count <= 0)
                {
                    count -= item->getItemCountOrSubtype();
                    items[slot] = NULL;
                }
                else
                {
                    items[slot] = NULL;
                    count = 0;
                }
                sendInventory(slot);
            }
            else if (container)
                count = removeContainerItem(container, itemid, count);
        }
    }

    if (count <= 0)
        return true;
    else
        return false;
}

int32_t Player::removeContainerItem(Container* container, int32_t itemid, int32_t count)
{
    for(int32_t number = 0; number < container->size(); number++)
    {
        if (count > 0)
        {
            Item *item = container->getItem(number);
            Container *subcontainer = dynamic_cast<Container*>(item);
            if (item->getID() == itemid)
            {
                if (item->isStackable() && item->getItemCountOrSubtype()-count > 0)
                {
                    item->setItemCountOrSubtype(item->getItemCountOrSubtype()-count);
                    onItemUpdateContainer(container, item, number);
                    count = 0;
                }
                else if(item->isStackable() && item->getItemCountOrSubtype()-count <= 0)
                {
                    count -= item->getItemCountOrSubtype();
                    container->removeItem(item);
                    onItemRemoveContainer(container, number);
                    number--;
                }
                else
                {
                    container->removeItem(item);
                    onItemRemoveContainer(container, number);
                    count = 0;
                }
            }
            else if(subcontainer)
            {
                count = removeContainerItem(subcontainer, itemid, count);
            }
        }
        else
        {
            return count;
        }
    }
    return count;
}

void Player::payBack(uint32_t cost)
{
    if (cost/1000000 > 100)
    {
        std::cout << "Player: payBack: too much money to pay back!" << std::endl;
        return;
    }
    if (cost/1000000 > 0)
    {
        TLMaddItem(ITEM_COINS_SCARAB, (unsigned char)(cost/1000000));
        cost -= 1000000*(cost/1000000);
    }
    if (cost/10000 > 0)
    {
        TLMaddItem(ITEM_COINS_CRYSTAL, (unsigned char)(cost/10000));
        cost -= 10000*(cost/10000);
    }
    if (cost/100 > 0)
    {
        TLMaddItem(ITEM_COINS_PLATINUM, (unsigned char)(cost/100));
        cost -= 100*(cost/100);
    }
    if (cost > 0 && cost < 100)
    {
        TLMaddItem(ITEM_COINS_GOLD, (unsigned char)cost);
    }
}

void Player::TLMaddItem(int32_t itemid, unsigned char count)
{
    Item *item = Item::CreateItem(itemid, count);
    if(!items[1] && item->getSlotPosition() & SLOTP_HEAD)
        addItemInventory(item, 1);
    else if(!items[2] && item->getSlotPosition() & SLOTP_NECKLACE)
        addItemInventory(item, 2);
    else if(!items[3] && item->getSlotPosition() & SLOTP_BACKPACK)
        addItemInventory(item, 3);
    else if(!items[4] && item->getSlotPosition() & SLOTP_ARMOR)
        addItemInventory(item, 4);
    else if(!items[7] && item->getSlotPosition() & SLOTP_LEGS)
        addItemInventory(item, 7);
    else if(!items[8] && item->getSlotPosition() & SLOTP_FEET)
        addItemInventory(item, 8);
    else if(!items[9] && item->getSlotPosition() & SLOTP_RING)
        addItemInventory(item, 9);
    else if((!items[5] && !items[6]) || (!items[5] && items[6] && !(items[6]->getSlotPosition() & SLOTP_TWO_HAND)))
        addItemInventory(item, 5);
    else if((!items[6] && !items[5]) || (!items[6] && items[5] && !(items[5]->getSlotPosition() & SLOTP_TWO_HAND)))
        addItemInventory(item, 6);
    else if(!items[10])
        addItemInventory(item, 10);
    else
    {
        for(int32_t slot = 1; slot <= 10; slot++)
        {
            Container *container = dynamic_cast<Container*>(items[slot]);
            if (container && container->size() < container->capacity())
            {
                container->addItem(item);
                onItemAddContainer(container,item);
                return;
            }
        }
        Tile *playerTile = g_game.getTile(pos.x, pos.y, pos.z);
        if (item->isStackable())
        {
            Item *toItem = dynamic_cast<Item*>(playerTile->getThingByStackPos(playerTile->getThingCount() - 1));
            if (toItem)
            {
                if (item->getID() == toItem->getID())
                {
                    if (toItem->getItemCountOrSubtype()+item->getItemCountOrSubtype() <= 100)
                    {
                        toItem->setItemCountOrSubtype(toItem->getItemCountOrSubtype()+item->getItemCountOrSubtype());
                    }
                    else
                    {
                        int32_t oldcount = toItem->getItemCountOrSubtype();
                        toItem->setItemCountOrSubtype(100);
                        playerTile->addThing(Item::CreateItem(item->getID(), 100-oldcount));
                    }
                }
            }
            else
            {
                playerTile->addThing(item);
            }
        }
        else
        {
            playerTile->addThing(item);
        }
        item->pos =  pos;
        g_game.sendAddThing(this, pos, item);
    }
}

void Player::addItemek(int32_t itemid, unsigned char count, uint16_t actionid)
{
    Item *item = Item::CreateItem(itemid, count);
    if((!items[5] && !items[6]) || (!items[5] && items[6] && !(items[6]->getSlotPosition() & SLOTP_TWO_HAND)))
    {
        item->setActionId(actionid);
        addItemInventory(item, 5);
    }
    else if((!items[6] && !items[5]) || (!items[6] && items[5] && !(items[5]->getSlotPosition() & SLOTP_TWO_HAND)))
    {
        item->setActionId(actionid);
        addItemInventory(item, 6);
    }
    else if(!items[10])
    {
        item->setActionId(actionid);
        addItemInventory(item, 10);
    }
    else
    {
        for(int32_t slot = 1; slot <= 10; slot++)
        {
            Container *container = dynamic_cast<Container*>(items[slot]);
            if (container && container->size() < container->capacity())
            {
                item->setActionId(actionid);
                container->addItem(item);
                onItemAddContainer(container,item);
                return;
            }
        }
        Tile *playerTile = g_game.getTile(pos.x, pos.y, pos.z);
        if(!item->isStackable())
        {
            playerTile->addThing(item);
        }
        item->pos =  pos;
        g_game.sendAddThing(this, pos, item);
    }
}

void Player::sendHouseWindow(House* house, const Position& pos, rights_t rights)
{
    if (!house)
        return;

    editedHouse = house;
    editedHousePos = pos;
    editedHouseRights = rights;

    std::string members;
    if (rights == HOUSE_OWNER)
        members = house->getOwner();
    else if (rights == HOUSE_SUBOWNER)
        members = house->getSubOwners();
    else if (rights == HOUSE_DOOROWNER)
        members = house->getDoorOwners(pos);
    else if (rights == HOUSE_GUEST)
        members = house->getGuests();
    client->sendHouseWindow(members);
}

void Player::receiveHouseWindow(std::string membersAfter)
{
    if (!editedHouse)
    {
        std::cout << "Player: receiveHouseWindow: house window was never sent!" << std::endl;
        return;
    }

    std::string membersBefore;
    //std::stringstream textmsg;


    if (editedHouseRights == HOUSE_GUEST)
    {
        membersBefore = editedHouse->getGuests();
        editedHouse->setGuests(membersAfter);
    }
    else if (editedHouseRights == HOUSE_DOOROWNER)
    {
        membersBefore = editedHouse->getDoorOwners(editedHousePos);
        editedHouse->setDoorOwners(membersAfter, editedHousePos);
    }
    else if (editedHouseRights == HOUSE_SUBOWNER)
    {
        membersBefore = editedHouse->getSubOwners();
        editedHouse->setSubOwners(membersAfter);
    }
    else if (editedHouseRights == HOUSE_OWNER)
    {
        membersBefore = editedHouse->getOwner();
        /*	Game* game = client->getGame(); // fix it
        	Creature* creaturka = game->getCreatureByName(editedHouse->getOwner());
        	//Creature* typek = game->getCreatureByName(membersAfter);
        	Player* typus = game->getPlayerByName(membersAfter);
        	Player* playerek = creaturka? dynamic_cast<Player*>(creaturka) : NULL;
        		if(editedHouse->checkHouseCount(typus) >= g_config.getGlobalNumber("maxhouses", 1)){
                textmsg << " Ten gracz nie moze miec wiecej niz " << g_config.getGlobalNumber("maxhouses", 0) << " domek.";
                playerek->sendTextMessage(MSG_ADVANCE, textmsg.str().c_str());
                editedHouse->setOwner(membersBefore);
                }else*/
        editedHouse->setOwner(membersAfter);
    }

    std::string members = membersBefore + std::string("\n") + membersAfter;
    boost::tokenizer<> tokens(members, boost::char_delimiters_separator<char>(false, NULL, "\n"));

    for (boost::tokenizer<>::iterator tok = tokens.begin(); tok != tokens.end(); ++tok)
    {
        Game* game = client->getGame();
        Creature* creature = game->getCreatureByName(*tok);
        Player* player = creature? dynamic_cast<Player*>(creature) : NULL;

        if (player)
        {
            player->houseRightsChanged = true;

            Tile* tile = game->getTile(player->pos);	// kick player from house if he has no rights
            if (tile && tile->getHouse() && tile->getHouse()->getPlayerRights(*tok) == HOUSE_NONE)
                game->teleport(player, tile->getHouse()->getFrontDoor());
            bool last = false;
            for (int32_t x = player->pos.x-1; x <= player->pos.x+1 && !last; x++)
            {
                for(int32_t y = player->pos.y-1; y <= player->pos.y+1 && !last; y++)
                {
                    Position doorPos(x, y, player->pos.z);
                    Tile* tile = game->getTile(doorPos);
                    House* house = tile? tile->getHouse() : NULL;
                    if(house)
                    {
                        house->save();
                        last = true;
                    }
                }
            }
        }
    }

    editedHouse = NULL;
    editedHouseRights = HOUSE_NONE;
}

#ifdef YUR_BOH
void Player::checkBoh()
{
    bool bohNow = (items[SLOT_FEET] && items[SLOT_FEET]->getID() == ITEM_BOH);

    if (boh != bohNow)
    {
        boh = bohNow;
        setNormalSpeed();
        hasteTicks = 0;
        sendChangeSpeed(this);
        sendIcons();
    }
}
#endif //YUR_BOH

/*void Player::setGuildInfo(gstat_t gstat, uint32_t gid, std::string gname, std::string rank, std::string nick)
{
	guildStatus = gstat;
	guildId = gid;
	guildName = gname;
	guildRank = rank;
	guildNick = nick;
}*/

void Player::notAfk()
{
    idleTime = 0;
    warned = false;
}

void Player::checkAfk(int32_t thinkTicks)
{
/*#ifdef HUCZU_NOLOGOUT_TILE
    Tile *tile = g_game.map->getTile(this->pos);
    if(tile && tile->isNoLogout())
        return;
#endif*/
    if (idleTime < g_config.KICK_TIME)
        idleTime += thinkTicks;

    if (access < g_config.ACCESS_PROTECT && idleTime < g_config.KICK_TIME && idleTime > (g_config.KICK_TIME - 60000) && !warned)	// send warning for kick (1 min) poprawka z gm.
    {
        sendTextMessage(MSG_RED_TEXT,"Uwaga: Byles AFK za dlugo. Zostaniesz kickniety za minute.");
        warned = true;
    }

    if (idleTime >= g_config.KICK_TIME && warned)
        if(this->access < 2)
        {
            kickPlayer();
            client->setPlayer(this);
        }
}

void Player::notifyLogIn(Player* login_player)
{
    VIPListSet::iterator it = VIPList.find(login_player->getGUID());
    if(it != VIPList.end())
    {
        client->sendVIPLogIn(login_player->getGUID());
        sendTextMessage(MSG_SMALLINFO, (login_player->getName() + " has logged in.").c_str());
    }
}

void Player::notifyLogOut(Player* logout_player)
{
    VIPListSet::iterator it = VIPList.find(logout_player->getGUID());
    if(it != VIPList.end())
    {
        client->sendVIPLogOut(logout_player->getGUID());
        sendTextMessage(MSG_SMALLINFO, (logout_player->getName() + " has logged out.").c_str());
    }
}

void Player::sendVipLogin(std::string vipname)
{
    for (int32_t i = 0; i < MAX_VIPS; i++)
    {
        if (!vip[i].empty() && vip[i] == vipname)
        {
            NetworkMessage msgs;
            msgs.Reset();
            msgs.AddByte(0xD3);
            msgs.AddU32(i+1);
            client->sendNetworkMessage(&msgs);
        }
    }
}

void Player::sendVipLogout(std::string vipname)
{
    for (int32_t i = 0; i < MAX_VIPS; i++)
    {
        if (!vip[i].empty() && vip[i] == vipname)
        {
            NetworkMessage msgs;
            msgs.Reset();
            msgs.AddByte(0xD4);
            msgs.AddU32(i+1);
            client->sendNetworkMessage(&msgs);
        }
    }
}

bool Player::removeVIP(uint32_t _guid)
{
    VIPListSet::iterator it = VIPList.find(_guid);
    if(it != VIPList.end())
    {
        VIPList.erase(it);
        return true;
    }
    return false;
}

bool Player::addVIP(uint32_t _guid, std::string &name, bool isOnline, bool internal /*=false*/)
{
    if(guid == _guid)
    {
        if(!internal)
            sendTextMessage(MSG_SMALLINFO, "You cannot add yourself.");
        return false;
    }

    if(VIPList.size() > 50)
    {
        if(!internal)
            sendTextMessage(MSG_SMALLINFO, "You cannot add more players.");
        return false;
    }

    VIPListSet::iterator it = VIPList.find(_guid);
    if(it != VIPList.end())
    {
        if(!internal)
            sendTextMessage(MSG_SMALLINFO, "You have already added this player.");
        return false;
    }

    VIPList.insert(_guid);

    if(!internal)
        client->sendVIP(_guid, name, isOnline);

    return true;
}

exp_t Player::getExpForNextLevel()
{
    return getExpForLv(level + 1) - experience;
}

uint32_t Player::getManaForNextMLevel()
{
    return getReqMana(maglevel+1, vocation) - manaspent;
}

#ifdef YUR_RINGS_AMULETS
void Player::checkRing(int32_t thinkTics)
{
    if (items[SLOT_RING] && items[SLOT_RING]->getTime() > 0) 	// eat time from ring
    {
        items[SLOT_RING]->useTime(thinkTics);
        if (items[SLOT_RING]->getTime() <= 0)
        {
            removeItemInventory(SLOT_RING);
            client->sendSkills();	// TODO: send only if it was skill ring
        }
    }

    bool timeRingNow = (items[SLOT_RING] && items[SLOT_RING]->getID() == ITEM_TIME_RING_IN_USE);
    if (timeRing != timeRingNow)
    {
        timeRing = timeRingNow;
        setNormalSpeed();
        hasteTicks = 0;
        sendChangeSpeed(this);
        sendIcons();
    }

    if(items[SLOT_BACKPACK] && items[SLOT_BACKPACK]->getID() == ITEM_MAGIC_BACKPACK)
    {
        mana += min(g_config.MAGIC_BACKPACK_MP, manamax - mana);
        health += min(g_config.MAGIC_BACKPACK_HP, healthmax - health);
    }
    if(items[SLOT_FEET] && items[SLOT_FEET]->getID() == ITEM_SOFT_BOOTS)
    {
        mana += min(g_config.SOFTMANA, manamax - mana);
        health += min(g_config.SOFTHEALTH, healthmax - health);
    }
        if(items[SLOT_FEET] && items[SLOT_FEET]->getID() == ITEM_KLAP_BOOTS)
    {
        mana += min(g_config.SOFTMANA, manamax - mana);
        health += min(g_config.SOFTHEALTH, healthmax - health);
    }

    bool energyRingNow = (items[SLOT_RING] && items[SLOT_RING]->getID() == ITEM_ENERGY_RING_IN_USE);
    if (energyRingNow)
        manaShieldTicks = items[SLOT_RING]->getTime();

    if (energyRing != energyRingNow)
    {
        energyRing = energyRingNow;
        if (!energyRing)
            manaShieldTicks = 0;
        sendIcons();
    }
    /*			bool dwarvenRingNow = (items[SLOT_RING] && items[SLOT_RING]->getID() == ITEM_DWARVEN_RING_IN_USE);
    	if (dwarvenRingNow)
    		dwarvenTicks = items[SLOT_RING]->getTime();

    	if (dwarvenRing != dwarvenRingNow)
    	{
    		dwarvenRing = dwarvenRingNow;
    		if (!dwarvenRing)
    			dwarvenTicks = 1;
    		sendIcons();
    	}*/
    bool dwarvenRingNow = (items[SLOT_RING] && items[SLOT_RING]->getID() == ITEM_DWARVEN_RING_IN_USE);
    if (dwarvenRingNow)
        immunities |= ATTACK_DRUNKNESS;

    if (dwarvenRing != dwarvenRingNow)
    {
        dwarvenRing = dwarvenRingNow;
        if (!dwarvenRing)
            immunities -= ATTACK_DRUNKNESS;
        sendIcons();
    }

    bool stealthRingNow = (items[SLOT_RING] && items[SLOT_RING]->getID() == ITEM_STEALTH_RING_IN_USE);
    if (stealthRingNow)
        invisibleTicks = items[SLOT_RING]->getTime();

    if (stealthRing != stealthRingNow)
    {
        stealthRing = stealthRingNow;
        if (!stealthRing)
            invisibleTicks = 0;
        g_game.creatureChangeOutfit(this);
    }
}
#endif //YUR_RINGS_AMULETS


#ifdef HUCZU_SKULLS
bool Player::checkSkull(int32_t thinkTicks)
{
    bool skullChanged = false;
    if (skullTicks > 0)
    {
        skullTicks -= thinkTicks;
        if (skullTicks <= 0)
        {
            skullTicks = 0;
            skullType = SKULL_NONE;
            skullChanged = true;

            for(AutoList<Player>::listiterator it = Player::listPlayer.list.begin(); it != Player::listPlayer.list.end(); ++it)
            {
                if(this->isYellowTo((*it).second))
                {
                    (*it).second->removeFromYellowList(this);
                }
                if((*it).second->hasAttacked(this))
                {
                    (*it).second->clearAttacked();
                }
            }
            clearAttacked();
            clearYellowList();
        }
    }
    if (skullKills > 0)
    {
        if (absolveTicks > 0)
        {
            absolveTicks -= thinkTicks;
            if (absolveTicks <= 0)
            {
                skullKills--;
                if (skullKills > 0)
                    absolveTicks = g_config.FRAG_TIME;
                else
                    absolveTicks = 0;
            }
        }
        else
            absolveTicks = g_config.FRAG_TIME;
    }
    return skullChanged;
}

void Player::onSkull(Player* player)
{
    client->sendSkull(player);
}

void Player::onPartyIcons(const Player *playa, int32_t icontype, bool skull, bool removeskull)
{
    client->sendPartyIcons(playa, icontype, skull, removeskull);
}

void Player::clearAttacked()
{
    attackedSet.clear();
}

void Player::clearYellowList()
{
    hasAsYellow.clear();
}

void Player::removeFromYellowList(Player* player)
{
    if(!player)
        return;
    if(!hasAsYellow.empty())
    {
        for(std::vector<Player*>::iterator yellowPlayer = hasAsYellow.begin(); yellowPlayer != hasAsYellow.end(); ++yellowPlayer)
        {
            if((*yellowPlayer) == player)//found!
            {
                hasAsYellow.erase(yellowPlayer);//erase him!
                break;
            }
        }
    }
}

bool Player::hasAttacked(const Player* attacked) const
{
    if(access >= 2)
        return false;
    if(!attacked)
        return false;

    AttackedSet::const_iterator it;
    uint32_t attacked_id = attacked->getID();
    it = attackedSet.find(attacked_id);
    if(it != attackedSet.end())
        return true;
    else
        return false;
}

void Player::addAttacked(const Player* attacked)
{
    if(access >= 2)
        return;

    if(!attacked || attacked == this)
        return;
    AttackedSet::iterator it;
    uint32_t attacked_id = attacked->getID();
    it = attackedSet.find(attacked_id);
    if(it == attackedSet.end())
        attackedSet.insert(attacked_id);
}

bool Player::isYellowTo(Player* player)
{
    if(!player || this == player)
        return false;
    if(!hasAsYellow.empty())
    {
        for(std::vector<Player*>::iterator yellowPlayer = hasAsYellow.begin(); yellowPlayer != hasAsYellow.end(); ++yellowPlayer)
        {
            if((*yellowPlayer) == player) //found
                return true;

        }
    }
    return false;
}
#endif //HUCZU_SKULLS

bool Player::isRookie() const
{
    return access < g_config.ACCESS_PROTECT && vocation == VOCATION_NONE;
}

void Player::setVocation(playervoc_t voc)
{
    if (vocation == VOCATION_NONE)
    {
        if (voc == VOCATION_DRUID || voc == VOCATION_KNIGHT || voc == VOCATION_PALADIN || voc == VOCATION_SORCERER)
            vocation = voc;
        else
            std::cout << "WARNING: invalid vocation for " << name << "!" << std::endl;
    }
    else
        std::cout << "WARNING: " << name << " already has vocation!" << std::endl;
}

bool Player::isUsingSpears() const
{
    return (items[SLOT_LEFT] && items[SLOT_LEFT]->getID() == ITEM_SPEAR) ||
           (items[SLOT_RIGHT] && items[SLOT_RIGHT]->getID() == ITEM_SPEAR);
}

bool Player::isUsingBurstArrows() const
{
    return ((items[SLOT_LEFT] && items[SLOT_LEFT]->getID() == ITEM_BOW) ||
            (items[SLOT_RIGHT] && items[SLOT_RIGHT]->getID() == ITEM_BOW)) &&
           (items[SLOT_AMMO] && items[SLOT_AMMO]->getID() == ITEM_BURST_ARROW);
}

void Player::checkLightItem(int32_t /*thinkTics*/)
{
#ifdef FIXY
    int32_t lightItemNow = items[SLOT_AMMO]? items[SLOT_AMMO]->getID() : 0 || items[SLOT_LEFT]? items[SLOT_LEFT]->getID() : 0 || items[SLOT_RIGHT]? items[SLOT_RIGHT]->getID() : 0;
#else
    int32_t lightItemNow = items[SLOT_AMMO]? items[SLOT_AMMO]->getID() : 0;
#endif //FIXY

    if (lightItemNow != lightItem)
    {
        if (Item::items[lightItem].lightLevel != Item::items[lightItemNow].lightLevel)
            g_game.creatureChangeLight(this, 0,
                                       Item::items[lightItemNow].lightLevel,
                                       Item::items[lightItemNow].lightColor);

        lightItem = lightItemNow;
    }
}

#ifdef YUR_PREMIUM_PROMOTION
void Player::checkPremium(int32_t thinkTics)
{
    if (premiumTicks > 0)
    {
        premiumTicks -= thinkTics;
        if (premiumTicks < 0)
            premiumTicks = 0;
    }
}
#endif //YUR_PREMIUM_PROMOTION

void Player::addDeath(const std::string& killer, int32_t level, time_t time)
{
    Death death = { killer, level, time };
    deathList.push_back(death);

    while (deathList.size() > g_config.MAX_DEATH_ENTRIES)
        deathList.pop_front();
}

#ifdef CVS_DAY_CYCLE
void Player::sendWorldLightLevel(unsigned char lightlevel, unsigned char color)
{
    if(last_worldlightlevel != lightlevel)
    {
        client->sendWorldLightLevel(lightlevel, color);
        last_worldlightlevel = lightlevel;
    }
}

void Player::sendPlayerLightLevel(Player* player)
{
    if(client)
        client->sendPlayerLightLevel(player);
}
#endif //CVS_DAY_CYCLE

int32_t Player::getWandId() const
{
    for (int32_t slot = SLOT_RIGHT; slot <= SLOT_LEFT; slot++)
    {
        int32_t id = items[slot]? items[slot]->getID() : 0;
        switch (id)
        {
        case ITEM_QUAGMIRE_ROD:
        case ITEM_SNAKEBITE_ROD:
        case ITEM_TEMPEST_ROD:
        case ITEM_VOLCANIC_ROD:
        case ITEM_MOONLIGHT_ROD:
        case ITEM_WAND_OF_INFERNO:
        case ITEM_WAND_OF_PLAGUE:
        case ITEM_WAND_OF_COSMIC_ENERGY:
        case ITEM_WAND_OF_VORTEX:
        case ITEM_WAND_OF_DRAGONBREATH:
            return id;
        }
    }
    return 0;
}

void Player::sendToSpecialChannel(SpeakClasses type, const std::string &text, uint16_t channelId, const std::string &info)
{
    client->sendToSpecialChannel(this, type, text, channelId, info);
}

void Player::mcCheck()
{
    std::stringstream info;
    unsigned char ip[4];

    for(AutoList<Player>::listiterator it = Player::listPlayer.list.begin(); it != Player::listPlayer.list.end(); ++it)
    {
        Player* lol = (*it).second;
        if(lol != this && this->getIP() == lol->getIP())
        {
            *(uint32_t*)&ip = (*it).second->getIP();
            info << lol->getName() << " oraz " << this->getName() << " IP: " <<
            (uint32_t)ip[0] << "." << (uint32_t)ip[1] << "." << (uint32_t)ip[2] << "." << (uint32_t)ip[3];
            g_game.creatureSendToSpecialChannel(this, SPEAK_CHANNEL_O, info.str().c_str(), 0x01, "MC Checker", false);
            break;
        }
    }
}

void Player::botMessage(BotType typBota)
{
    std::stringstream informacja;
    switch(typBota)
    {
    case BOT_CAVE:
        informacja << this->getName() << " uzywa cave bota.";
        break;
    case BOT_AUTOUH:
        informacja << this->getName() << " uzywa auto uh.";
        break;
    case BOT_YOURSELF_ATTACK:
        informacja << this->getName() << " atakuje siebie.";
        break;
    }
    g_game.creatureSendToSpecialChannel(this, SPEAK_CHANNEL_R1, informacja.str().c_str(), 0x01, "Bot Checker", false);
}

bool Player::hasTwoSquareItem() const
{
    for (int32_t slot = SLOT_RIGHT; slot <= SLOT_LEFT; slot++)
    {
        if (items[slot])
            switch(items[slot]->getID())
            {
            case ITEM_MLS:
            case ITEM_EPEE:
                return true;
                break;
            }
    }
    return false;
}

void Player::sendNetworkMessage(NetworkMessage *msg)
{
    client->sendNetworkMessage(msg);
}

#ifdef HUCZU_RRV
void Player::cancelPlayerViolation()
{
    client->cancelViolation(getName());
}
#endif

#ifdef HUCZU_SERVER_LOG
void Player::sendFromSystem(SpeakClasses type, const std::string &text)
{
    client->sendFromSystem(type, text);
}
#endif

bool Player::setGuildLevel(GuildLevel_t newLevel, uint32_t rank/* = 0*/)
{
    std::string name;
    if(!Guild::getInstance()->getRankEx(rank, name, guildId, newLevel))
        return false;

    guildStatus = newLevel;
    rankName = name;
    rankId = rank;
    return true;
}

bool Player::isGuildInvited(uint32_t guildId) const
{
    for(InvitedToGuildsList::const_iterator it = invitedToGuildsList.begin(); it != invitedToGuildsList.end(); ++it)
    {
        if((*it) == guildId)
            return true;
    }

    return false;
}

void Player::leaveGuild()
{
    //sendCloseChannel(CHANNEL_GUILD);
    g_chat.removeUserFromChannel(this,0x00);
    guildStatus = GUILD_NONE;
    guildId = rankId = 0;
    guildName = rankName = guildNick = "";
}

bool Player::isUsingGoldBolt() const
{
    return ((items[SLOT_LEFT] && items[SLOT_LEFT]->getID() == ITEM_CROSS_BOW) ||
            (items[SLOT_RIGHT] && items[SLOT_RIGHT]->getID() == ITEM_CROSS_BOW)) &&
           (items[SLOT_AMMO] && items[SLOT_AMMO]->getID() == ITEM_GOLD_BOLT);
}

bool Player::isUsingSilverWand() const
{
    bool ret = false;
    if(items[SLOT_LEFT] && items[SLOT_LEFT]->getID() == ITEM_SILVER_WAND)
    {
        if(items[SLOT_RIGHT] && items[SLOT_RIGHT]->getID() == ITEM_SPRITE_WAND)
            ret = false;
        else
            ret = true;
    }
    else if(items[SLOT_RIGHT] && items[SLOT_RIGHT]->getID() == ITEM_SILVER_WAND)
    {
        if(items[SLOT_LEFT] && items[SLOT_LEFT]->getID() == ITEM_SPRITE_WAND)
            ret = false;
        else
            ret = true;
    }
    return ret;
}
bool Player::removePunkty(uint32_t _punkty)
{
    if(!this || _punkty == 0)
        return false;

    if(getPunkty() > _punkty)
    {
        punkty -= _punkty;
        return true;
    }

    return false;
}
#ifdef __MIZIAK_SUPERMANAS__
Item* Player::getItemByID(int32_t itemid, int32_t count, bool actionId /*=false*/)
{
    for (int slot = 1; slot <= 10; slot++)
    {
        Item *item = items[slot];
        if (item)
        {
            Container *container = dynamic_cast<Container*>(item);
            if (item->getID() == itemid && item->getItemCountOrSubtype() == count)
            {
                if(actionId == true)
                {
                    if(item->getActionId() > 0)
                        return item;
                }
                else
                    return item;
            }
            else if (container)
                return getItemByIDContainer(container, itemid, count, actionId);
        }
    }
    return NULL;
}

Item* Player::getItemByIDContainer(Container* container, int32_t itemid, int32_t count, bool actionId/*=false*/)
{
    for(int32_t number = 0; number < container->size(); number++)
    {
        Item *item = container->getItem(number);
        Container *subcontainer = dynamic_cast<Container*>(item);
        if (item->getID() == itemid && item->getItemCountOrSubtype() == count)
        {
            if(actionId == true)
            {
                if(item->getActionId() > 0)
                    return item;
            }
            else
                return item;
        }
        else if(subcontainer)
            return getItemByIDContainer(subcontainer, itemid, count, actionId);

    }
    return NULL;
}
#endif
