//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software Foundation,
// Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//////////////////////////////////////////////////////////////////////


#include "definitions.h"

#include <string>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <boost/tokenizer.hpp>

using namespace std;

#include <stdlib.h>
#include <time.h>       /* time_t, struct tm, difftime, time, mktime */

#include "protocol.h"
#include "player.h"
#include "luascript.h"
#include "const76.h"
#include "chat.h"

#ifdef ELEM_VIP_LIST
#include "networkmessage.h"
#endif //ELEM_VIP_LIST

extern LuaScript g_config;
extern Game g_game;
extern Chat g_chat;

AutoList<Player> Player::listPlayer;

#ifdef YUR_PREMIUM_PROMOTION
const int64_t Player::promotedGainManaVector[5][2] = {{6,1},{2,1},{2,1},{3,1},{6,1}};
const int64_t Player::promotedGainHealthVector[5][2] = {{6,1},{6,1},{6,1},{3,1},{2,1}};
#endif //YUR_PREMIUM_PROMOTION

const int64_t Player::gainManaVector[5][2] = {{6,1},{3,1},{3,1},{4,1},{6,1}};
const int64_t Player::gainHealthVector[5][2] = {{6,1},{6,1},{6,1},{4,1},{3,1}};

#ifdef CVS_GAINS_MULS
int Player::CapGain[5] = {10, 10, 10, 20, 25};
int64_t Player::ManaGain[5] = {5, 30, 30, 15, 5};
int64_t Player::HPGain[5] = {5, 5, 5, 10, 15};
#endif //CVS_GAINS_MULS

Player::Player(const std::string& name, Protocol *p) :
Creature()
{
	client     = p;
#ifdef YUR_BUILTIN_AAC
	if (client)
#endif //YUR_BUILTIN_AAC
		client->setPlayer(this);

	looktype   = PLAYER_MALE_1;
	vocation   = VOCATION_NONE;
	capacity = 300.00;
	mana       = 0;
	manamax    = 0;
	manaspent  = 0;
	this->name = name;
	food       = 0;
	guildId    = 0;

#ifdef CVS_DAY_CYCLE
	lightlevel = 0;
	last_worldlightlevel = -1;
#endif //CVS_DAY_CYCLE
#ifdef YUR_GUILD_SYSTEM
	guildStatus = GUILD_NONE;
#endif //YUR_GUILD_SYSTEM
#ifdef BD_HOUSE_WINDOW
	editedHouse = NULL;
	editedHouseRights = HOUSE_NONE;
#endif //BD_HOUSE_WINDOW
#ifdef TLM_HOUSE_SYSTEM
	houseRightsChanged = true;
#endif //TLM_HOUSE_SYSTEM
#ifdef TR_ANTI_AFK
	idleTime   = 0;
	warned     = false;
#endif //TR_ANTI_AFK
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
#ifdef TLM_SKULLS_PARTY
	party = 0;
	banned = false;
	skullTicks = 0;
	skullKills = 0;
	absolveTicks = 0;
#endif //TLM_SKULLS_PARTY
#ifdef YUR_RINGS_AMULETS
	energyRing = false;
 #ifdef YUR_INVISIBLE
	stealthRing = false;
 #endif //YUR_INVISIBLE
#endif //YUR_RINGS_AMULETS
#ifdef YUR_PREMIUM_PROMOTION
	premiumTicks = 0;
	promoted = false;
#endif //YUR_PREMIUM_PROMOTION
#ifdef YUR_LIGHT_ITEM
	lightItem = 0;
#endif //YUR_LIGHT_ITEM

	eventAutoWalk = 0;
	level      = 1;
	experience = 180;

	maglevel   = 20;

	access     = 0;
	lastlogin  = 0;
	lastLoginSaved = 0;
	SendBuffer = false;
	npings = 0;
	internal_ping = 0;
	fightMode = followMode = 0;

	tradePartner = 0;
	tradeState = TRADE_NONE;
	tradeItem = NULL;

	for(int i = 0; i < 7; i++)
	{
		skills[i][SKILL_LEVEL] = 1;
		skills[i][SKILL_TRIES] = 0;
		skills[i][SKILL_PERCENT] = 0;

		for(int j = 0; j < 2; j++){
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
	for(int i = 0; i < 11; i++)
		items[i] = NULL;

 	useCount = 0;

  	/*
 	CapGain[0]  = 10;     //for level advances
 	CapGain[1]  = 10;     //e.g. Sorcerers will get 10 Cap with each level up
 	CapGain[2]  = 10;
 	CapGain[3]  = 20;
	CapGain[4]  = 25;

 	ManaGain[0] = 5;      //for level advances
 	ManaGain[1] = 30;
 	ManaGain[2] = 30;
 	ManaGain[3] = 15;
 	ManaGain[4] = 5;

	HPGain[0]   = 5;      //for level advances
 	HPGain[1]   = 5;
 	HPGain[2]   = 5;
 	HPGain[3]   = 10;
 	HPGain[4]   = 15;
 	*/
#ifdef YUR_CVS_MODS
	max_depot_items = g_config.MAX_DEPOT_ITEMS;
#else
 	max_depot_items = 1000;
#endif //YUR_CVS_MODS

 	manaTick = 0;
 	healthTick = 0;
}


Player::~Player()
{
	for (int i = 0; i < 11; i++) {
		if (items[i]) {
			items[i]->releaseThing();
		}
	}

	DepotMap::iterator it;
	for(it = depots.begin();it != depots.end(); it++){
		it->second->releaseThing();
	}
	//std::cout << "Player destructor " << this->getID() << std::endl;
  delete client;
}

bool Player::isPushable() const {
	return ((getSleepTicks() <= 0) && access < g_config.ACCESS_PROTECT);
}

std::string Player::getDescription(bool self) const
{
	std::stringstream s;
	std::string str;

	if(self){
		s << "yourself.";

		if(vocation != VOCATION_NONE) {
#ifdef YUR_PREMIUM_PROMOTION
			if (isPromoted())
				s << " You are " << g_config.PROMOTED_VOCATIONS[(int)vocation] << ".";
			else
#endif //YUR_PREMIUM_PROMOTION
				s << " You are " << g_config.VOCATIONS[(int)vocation] << ".";
		}
	}
	else {
		s << name << " (Level " << level <<").";

		if(vocation != VOCATION_NONE){
			if(sex == PLAYERSEX_FEMALE)
				s << " She";
			else
				s << " He";

#ifdef YUR_PREMIUM_PROMOTION
			if (isPromoted())
				s << " is "<< g_config.PROMOTED_VOCATIONS[(int)vocation] << ".";
			else
#endif //YUR_PREMIUM_PROMOTION
				s << " is "<< g_config.VOCATIONS[(int)vocation] << ".";
		}
	}

#ifdef YUR_GUILD_SYSTEM
	if (guildStatus >= GUILD_MEMBER)
#else
	if(guildId)
#endif //YUR_GUILD_SYSTEM
	{
		if(self)
			s << " You are ";
		else
		{
			if(sex == PLAYERSEX_FEMALE)
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
	}

	str = s.str();
	return str;
}

Item* Player::getItem(int pos) const
{
	if(pos>0 && pos <11)
		return items[pos];
	return NULL;
}

int64_t Player::getWeaponDamage() const
{
	double mul = 1.0;
#ifdef YUR_FIGHT_MODE
	mul = (fightMode==1? 2.0 : (fightMode==2? 1.5 : 1.0));
#endif //YUR_FIGHT_MODE

	double damagemax = 0;
	//TODO:what happens with 2 weapons?
  for (int slot = SLOT_RIGHT; slot <= SLOT_LEFT; slot++)
	  if (items[slot]){
			if ((items[slot]->isWeapon())){
				// check which kind of skill we use...
				// and calculate the damage dealt
				Item *distitem;
				switch (items[slot]->getWeaponType()){
					case SWORD:
						//damagemax = 3*skills[SKILL_SWORD][SKILL_LEVEL] + 2*Item::items[items[slot]->getID()].attack;
						damagemax = mul*getSkill(SKILL_SWORD,SKILL_LEVEL)*Item::items[items[slot]->getID()].attack/20 + Item::items[items[slot]->getID()].attack;
						break;
					case CLUB:
						//damagemax = 3*skills[SKILL_CLUB][SKILL_LEVEL] + 2*Item::items[items[slot]->getID()].attack;
						damagemax = mul*getSkill(SKILL_CLUB,SKILL_LEVEL)*Item::items[items[slot]->getID()].attack/20 + Item::items[items[slot]->getID()].attack;
						break;
					case AXE:
						//damagemax = 3*skills[SKILL_AXE][SKILL_LEVEL] + 2*Item::items[items[slot]->getID()].attack;
						damagemax = mul*getSkill(SKILL_AXE,SKILL_LEVEL)*Item::items[items[slot]->getID()].attack/20 + Item::items[items[slot]->getID()].attack;
						break;
					case DIST:
						distitem = GetDistWeapon();
						if(distitem){
							//damagemax = 3*skills[SKILL_DIST][SKILL_LEVEL]+ 2*Item::items[distitem->getID()].attack;
							damagemax = mul*getSkill(SKILL_DIST,SKILL_LEVEL)*Item::items[distitem->getID()].attack/20 + Item::items[distitem->getID()].attack;

							//hit or miss
							int hitchance;
							if(distitem->getWeaponType() == AMO){//projectile
								hitchance = 90;
							}
							else{//thrown weapons
								hitchance = 50;
							}
							if(rand()%100 < hitchance){ //hit
								return 1+(int)(damagemax*rand()/(RAND_MAX+1.0));
							}
							else{	//miss
								return 0;
							}
						}
						break;
					case MAGIC:
						damagemax = (level*2+maglevel*3) * 1.25;
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
		damagemax = 2*mul*getSkill(SKILL_FIST,SKILL_LEVEL) + 5;

	// return it
	return 1+(int64_t)(damagemax*rand()/(RAND_MAX+1.0));
}

int Player::getArmor() const
{
	int armor=0;

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

int Player::getDefense() const
{
	int defense=0;

	if(items[SLOT_LEFT]){
		if(items[SLOT_LEFT]->getWeaponType() == SHIELD)
			defense += skills[SKILL_SHIELD][SKILL_LEVEL] + items[SLOT_LEFT]->getDefense();
		else
			defense += items[SLOT_LEFT]->getDefense();
	}
	if(items[SLOT_RIGHT]){
		if(items[SLOT_RIGHT]->getWeaponType() == SHIELD)
			defense += skills[SKILL_SHIELD][SKILL_LEVEL] + items[SLOT_RIGHT]->getDefense();
		else
			defense += items[SLOT_RIGHT]->getDefense();
	}
	//////////
	if(defense == 0)
		defense = (int)random_range(0,(int)skills[SKILL_SHIELD][SKILL_LEVEL]);
	else
		defense += (int)random_range(0,(int)skills[SKILL_SHIELD][SKILL_LEVEL]);

#ifdef YUR_FIGHT_MODE
	defense = int(double(defense) * (fightMode==1? 1.0 : (fightMode==2? 1.5 : 2.0)));
#endif //YUR_FIGHT_MODE

  	return random_range(int(defense*0.25), int(1+(int)(defense*rand())/(RAND_MAX+1.0)));
  	///////////
	//return defense;
}

unsigned long Player::getMoney()
{
	unsigned long money = 0;
	std::list<const Container*> stack;
	ContainerList::const_iterator cit;
	for(int i=0; i< 11;i++){
		if(items[i]){
			if(Container *tmpcontainer = dynamic_cast<Container*>(items[i])){
				stack.push_back(tmpcontainer);
			}
			else{
				money = money + items[i]->getWorth();
			}
		}
	}

	while(stack.size() > 0) {
		const Container *container = stack.front();
		stack.pop_front();
		for(cit = container->getItems(); cit != container->getEnd(); ++cit) {
			money = money + (*cit)->getWorth();
			Container *container = dynamic_cast<Container*>(*cit);
			if(container) {
				stack.push_back(container);
			}
		}
	}
	return money;
}

bool Player::substractMoney(unsigned long money)
{
	if(getMoney() < money)
		return false;

	std::list<Container*> stack;
	MoneyMap moneyMap;
	MoneyItem* tmp;

	ContainerList::iterator it;
	for(int i = 0; i < 11 && money > 0 ;i++){
		if(items[i]){
			if(Container *tmpcontainer = dynamic_cast<Container*>(items[i])){
				stack.push_back(tmpcontainer);
			}
			else{
				if(items[i]->getWorth() != 0){
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

	while(stack.size() > 0 && money > 0){
		Container *container = stack.front();
		stack.pop_front();
		for(int i = 0; i < container->size() && money > 0; i++){
			Item *item = container->getItem(i);
			if(item && item->getWorth() != 0){
				tmp = new MoneyItem;
				tmp->item = item;
				tmp->slot = 0;
				tmp->location = SLOT_TYPE_CONTAINER;
				tmp->parent = container;
				moneyMap.insert(moneymap_pair(item->getWorth(),tmp));
			}
			Container *containerItem = dynamic_cast<Container*>(item);
			if(containerItem){
				stack.push_back(containerItem);
			}
		}
	}

	MoneyMap::iterator it2;
	for(it2 = moneyMap.begin(); it2 != moneyMap.end() && money > 0; it2++){
		Item *item = it2->second->item;

		if(it2->second->location == SLOT_TYPE_INVENTORY){
			removeItemInventory(it2->second->slot);
		}
		else if(it2->second->location == SLOT_TYPE_CONTAINER){
			Container *container = it2->second->parent;
			unsigned char slot = container->getSlotNumberByItem(item);
			onItemRemoveContainer(container,slot);
			container->removeItem(item);
		}

		if((unsigned long)it2->first <= money){
			money = money - it2->first;
		}
		else{
			substractMoneyItem(item, money);
			money = 0;
		}
		g_game.FreeThing(item);
		item = NULL;
		delete it2->second;
		it2->second = NULL;
	}
	for(; it2 != moneyMap.end(); it2++){
		delete it2->second;
		it2->second = NULL;
	}

	moneyMap.clear();

	if(money != 0)
		return false;

	return true;
}

bool Player::substractMoneyItem(Item *item, unsigned long money)
{
	if(money >= (unsigned long)item->getWorth())
		return false;

	int remaind = item->getWorth() - money;
	int crys = remaind / 10000;
	remaind = remaind - crys * 10000;
	int plat = remaind / 100;
	remaind = remaind - plat * 100;
	int gold = remaind;
	if(crys != 0){
		Item *remaindItem = Item::CreateItem(ITEM_COINS_CRYSTAL, crys);
		if(!this->addItem(remaindItem))
			g_game.addThing(NULL,this->pos,remaindItem);

	}

	if(plat != 0){
		Item *remaindItem = Item::CreateItem(ITEM_COINS_PLATINUM, plat);
		if(!this->addItem(remaindItem))
			g_game.addThing(NULL,this->pos,remaindItem);
	}

	if(gold != 0){
		Item *remaindItem = Item::CreateItem(ITEM_COINS_GOLD, gold);
		if(!this->addItem(remaindItem))
			g_game.addThing(NULL,this->pos,remaindItem);
	}

	return true;
}

bool Player::removeItem(unsigned short id,long count)
{
	if(getItemCount(id) < count)
		return false;

	std::list<Container*> stack;

	ContainerList::iterator it;
	for(int i = 0; i < 11 && count > 0 ;i++){
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

	while(stack.size() > 0 && count > 0){
		Container *container = stack.front();
		stack.pop_front();
		for(int i = 0; i < container->size() && count > 0; i++){
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
}

int Player::getItemCount(unsigned short id)
{
	unsigned long counter = 0;
	std::list<const Container*> stack;
	ContainerList::const_iterator cit;
	for(int i=0; i< 11;i++){
		if(items[i]){
			if(items[i]->getID() == id){
				if(items[i]->isStackable()){
					counter = counter + items[i]->getItemCountOrSubtype();
				}
				else{
					counter++;
				}
			}
			if(Container *tmpcontainer = dynamic_cast<Container*>(items[i])){
				stack.push_back(tmpcontainer);
			}
		}
	}

	while(stack.size() > 0) {
		const Container *container = stack.front();
		stack.pop_front();
		for(cit = container->getItems(); cit != container->getEnd(); ++cit){
			if((*cit)->getID() == id){
				if((*cit)->isStackable()){
					counter = counter + (*cit)->getItemCountOrSubtype();
				}
				else{
					counter++;
				}
			}
			Container *container = dynamic_cast<Container*>(*cit);
			if(container) {
				stack.push_back(container);
			}
		}
	}
	return counter;
}

void Player::sendIcons()
{
	int icons = 0;
	if(inFightTicks >= 6000 || inFightTicks ==4000 || inFightTicks == 2000){
		icons |= ICON_SWORDS;
	}
	if(manaShieldTicks >= 1000){
		icons |= ICON_MANASHIELD;
	}
	if(speed > getNormalSpeed()){
		icons |= ICON_HASTE;
	}
	if(conditions.hasCondition(ATTACK_FIRE) /*burningTicks >= 1000*/){
		icons |= ICON_BURN | ICON_SWORDS;
	}
	if(conditions.hasCondition(ATTACK_ENERGY) /*energizedTicks >= 1000*/){
		icons |= ICON_ENERGY | ICON_SWORDS;
	}
	if(conditions.hasCondition(ATTACK_POISON)/*poisonedTicks >= 1000*/){
		icons |= ICON_POISON | ICON_SWORDS;
	}
	if(conditions.hasCondition(ATTACK_PARALYZE) /*speed < getNormalSpeed()*/ /*paralyzeTicks >= 1000*/) {
		icons |= ICON_PARALYZE | ICON_SWORDS;
	}

	client->sendIcons(icons);
}

void Player::updateInventoryWeigth()
{
	inventoryWeight = 0.00;
	if(access < g_config.ACCESS_PROTECT) {
		for(int slotid = 0; slotid < 11; ++slotid){
			if(getItem(slotid)) {
				inventoryWeight += getItem(slotid)->getWeight();
			}
		}
	}
}

int Player::sendInventory(unsigned char sl_id){
	client->sendInventory(sl_id);
	return true;
}

int Player::addItemInventory(Item* item, int pos, bool internal /*= false*/) {
#ifdef __DEBUG__
	std::cout << "Should add item at " << pos <<std::endl;
#endif
	if(pos > 0 && pos < 11)
  {
		if (items[pos]) {
			items[pos]->releaseThing();
		}

		items[pos] = item;
		if(items[pos]) {
			items[pos]->pos.x = 0xFFFF;
		}

		updateInventoryWeigth();

		if(!internal) {
			client->sendStats();
			client->sendInventory(pos);
		}
  }
	else
		return false;

	return true;
}

bool Player::addItem(Item *item, bool test /*=false*/){
	if(!item)
		return false;

	if(access < g_config.ACCESS_PROTECT && getFreeCapacity() < item->getWeight()) {
		return false;
	}

	Container *container;
	unsigned char slot;

	switch(getFreeSlot(&container,slot, item)){
	case SLOT_TYPE_NONE:
		return false;
		break;
	case SLOT_TYPE_INVENTORY:
		if(!test){
			addItemInventory(item,slot);
		}
		return true;
		break;
	case SLOT_TYPE_CONTAINER:
		if(container->isHoldingItem(item) == true){
			return false;
		}
		if(!test){
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
	if(!(item->getSlotPosition() & SLOTP_TWO_HAND) || (!items[SLOT_RIGHT] && !items[SLOT_LEFT])) {
		//first look free slot in inventory
		if(!items[SLOT_RIGHT]){
			if(!(items[SLOT_LEFT] && (items[SLOT_LEFT]->getSlotPosition() & SLOTP_TWO_HAND))){
				slot = SLOT_RIGHT;
				return SLOT_TYPE_INVENTORY;
			}
		}

		if(!items[SLOT_LEFT]){
			if(!(items[SLOT_RIGHT] && (items[SLOT_RIGHT]->getSlotPosition() & SLOTP_TWO_HAND))){
				slot = SLOT_LEFT;
				return SLOT_TYPE_INVENTORY;
			}
		}
	}

	if(!items[SLOT_AMMO]){
		slot = SLOT_AMMO;
		return SLOT_TYPE_INVENTORY;
	}

	//look in containers
	for(int i=0; i< 11;i++){
		Container *tmpcontainer = dynamic_cast<Container*>(items[i]);
		if(tmpcontainer){
			Container *container_freeslot = getFreeContainerSlot(tmpcontainer);
			if(container_freeslot){
				*container = container_freeslot;
				return SLOT_TYPE_CONTAINER;
			}
		}
	}
	return SLOT_TYPE_NONE;
}

Container* Player::getFreeContainerSlot(Container *parent)
{
	if(parent == getTradeItem()){
		return NULL;
	}
	//check if it is full
	if(parent->size() < parent->capacity()){
		return parent;
	}
	else{ //look for more containers inside
		for(ContainerList::const_iterator cit = parent->getItems(); cit != parent->getEnd(); ++cit){
			Container * temp_container = dynamic_cast<Container*>(*cit);
			if(temp_container){
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
	for(int i=0; i< 11;i++){
		if(item == items[i]){
			if(!test){
				removeItemInventory(i);
			}
			return true;
		}
		else if(tmpcontainer = dynamic_cast<Container*>(items[i])){
			if(internalRemoveItemContainer(tmpcontainer,item, test) == true){
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
		cit != parent->getEnd(); ++cit){
		if(*cit == item){
			unsigned char slot =  parent->getSlotNumberByItem(item);
			if(slot != 0xFF){
				if(!test){
					parent->removeItem(item);
					updateInventoryWeigth();
					client->sendStats();
					client->sendItemRemoveContainer(parent,slot);
				}
				return true;
			}
			else{
				return false;
			}
		}
		else if(temp_container = dynamic_cast<Container*>(*cit)){
			if(internalRemoveItemContainer(temp_container, item, test))
				return true;
		}
	}
	return false;
}

int Player::removeItemInventory(int pos, bool internal /*= false*/)
{
	if(pos > 0 && pos < 11) {

		if(items[pos]) {
			items[pos] = NULL;
		}

		updateInventoryWeigth();

		if(!internal) {
			client->sendStats();
			client->sendInventory(pos);
		}
	}
	else
		return false;

	return true;
}

unsigned int Player::getReqSkillTries (int skill, int level, playervoc_t voc) {
	//first find on cache
	for(int i=0;i<2;i++){
		if(SkillAdvanceCache[skill][i].level == level && SkillAdvanceCache[skill][i].vocation == voc){
#ifdef __DEBUG__
	std::cout << "Skill cache hit: " << this->name << " " << skill << " " << level << " " << voc <<std::endl;
#endif
			return SkillAdvanceCache[skill][i].tries;
		}
	}
	// follows the order of enum skills_t
	unsigned short int SkillBases[7] = { 50, 50, 50, 50, 30, 100, 20 };
	float SkillMultipliers[7][5] = {
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
	int j;
	if(SkillAdvanceCache[skill][0].level > SkillAdvanceCache[skill][1].level){
		j = 1;
	}
	else{
		j = 0;
	}
	SkillAdvanceCache[skill][j].level = level;
	SkillAdvanceCache[skill][j].vocation = voc;
	SkillAdvanceCache[skill][j].tries = (unsigned int) ( SkillBases[skill] * pow((float) SkillMultipliers[skill][voc], (float) ( level - 11) ) );
    return SkillAdvanceCache[skill][j].tries;
}

void Player::addSkillTry(int skilltry)
{
	int skill;
	bool foundSkill;
	foundSkill = false;
	std::string skillname;

	for (int slot = SLOT_RIGHT; slot <= SLOT_LEFT; slot++) {
		if (items[slot]) {
			if (items[slot]->isWeapon()) {
				switch (items[slot]->getWeaponType()) {
					case SWORD: skill = 2; break;
					case CLUB: skill = 1; break;
					case AXE: skill = 3; break;
					case DIST:
						if(GetDistWeapon())
							skill = 4;
						else
							skill = 0;
						break;
                    case SHIELD: continue; break;
                    case MAGIC: return; break;//TODO: should add skill try?
					default: skill = 0; break;
			 	}//switch

#ifdef YUR_MULTIPLIERS
				addSkillTryInternal(skilltry * (skill==4? g_config.DIST_MUL[vocation] : g_config.WEAPON_MUL[vocation]), skill);
#else
			 	addSkillTryInternal(skilltry,skill);
#endif //YUR_MULTIPLIERS
			 	foundSkill = true;
			 	break;
			}
		}
	}
	if(foundSkill == false)
#ifdef YUR_MULTIPLIERS
		addSkillTryInternal(skilltry * g_config.WEAPON_MUL[vocation],0);//add fist try
#else
		addSkillTryInternal(skilltry,0);//add fist try
#endif //YUR_MULTIPLIERS
}

void Player::addSkillShieldTry(int skilltry){

#ifdef YUR_MULTIPLIERS
	skilltry *= g_config.SHIELD_MUL[vocation];
#endif //YUR_MULTIPLIERS

	//look for a shield

	for (int slot = SLOT_RIGHT; slot <= SLOT_LEFT; slot++) {
		if (items[slot]) {
			if (items[slot]->isWeapon()) {
				if (items[slot]->getWeaponType() == SHIELD) {
					addSkillTryInternal(skilltry,5);
					break;
			 	}
			}
		}
	}
}

int64_t Player::getPlayerInfo(playerinfo_t playerinfo) const
{
	switch(playerinfo) {
		case PLAYERINFO_LEVEL: return level; break;
		case PLAYERINFO_LEVELPERCENT: return level_percent; break;
		case PLAYERINFO_MAGICLEVEL: return maglevel; break;
		case PLAYERINFO_MAGICLEVELPERCENT: return maglevel_percent; break;
		case PLAYERINFO_HEALTH: return health; break;
		case PLAYERINFO_MAXHEALTH: return healthmax; break;
		case PLAYERINFO_MANA: return mana; break;
		case PLAYERINFO_MAXMANA: return manamax; break;
		case PLAYERINFO_MANAPERCENT: return maglevel_percent; break;
		case PLAYERINFO_SOUL: return 100; break;
		default:
			return 0; break;
	}

	return 0;
}

int64_t Player::getSkill(skills_t skilltype, skillsid_t skillinfo) const
{
#ifdef YUR_RINGS_AMULETS
	if (skillinfo == SKILL_LEVEL && items[SLOT_RING])
	{
		int id = items[SLOT_RING]->getID();

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

std::string Player::getSkillName(int skillid){
	std::string skillname;
	switch(skillid){
	case 0:
		skillname = "fist fighting";
		break;
	case 1:
		skillname = "club fighting";
		break;
	case 2:
		skillname = "sword fighting";
		break;
	case 3:
		skillname = "axe fighting";
		break;
	case 4:
		skillname = "distance fighting";
		break;
	case 5:
		skillname = "shielding";
		break;
	case 6:
		skillname = "fishing";
		break;
	default:
		skillname = "unknown";
		break;
	}
	return skillname;
}

void Player::addSkillTryInternal(int skilltry,int skill){

	skills[skill][SKILL_TRIES] += skilltry;
	//for skill level advances
	//int reqTries = (int) ( SkillBases[skill] * pow((float) VocMultipliers[skill][voc], (float) ( skills[skill][SKILL_LEVEL] - 10) ) );
#if __DEBUG__
	//for debug
	cout << Creature::getName() << ", has the vocation: " << (int)vocation << " and is training his " << getSkillName(skill) << "(" << skill << "). Tries: " << skills[skill][SKILL_TRIES] << "(" << getReqSkillTries(skill, (skills[skill][SKILL_LEVEL] + 1), vocation) << ")" << std::endl;
	cout << "Current skill: " << skills[skill][SKILL_LEVEL] << std::endl;
#endif
	//Need skill up?
	if (skills[skill][SKILL_TRIES] >= getReqSkillTries(skill, (skills[skill][SKILL_LEVEL] + 1), vocation)) {
	 	skills[skill][SKILL_LEVEL]++;
	 	skills[skill][SKILL_TRIES] = 0;
		skills[skill][SKILL_PERCENT] = 0;
		std::stringstream advMsg;
		advMsg << "You advanced in " << getSkillName(skill) << ".";
		client->sendTextMessage(MSG_ADVANCE, advMsg.str().c_str());
		client->sendSkills();
	}
	else{
	 //update percent
	 int new_percent = (unsigned int)(100*(skills[skill][SKILL_TRIES])/(1.*getReqSkillTries(skill, (skills[skill][SKILL_LEVEL]+1), vocation)));

	 	if(skills[skill][SKILL_PERCENT] != new_percent){
			skills[skill][SKILL_PERCENT] = new_percent;
			client->sendSkills();
	 	}
	}
}


uint64_t Player::getReqMana(int64_t maglevel, playervoc_t voc) {
  //ATTENTION: MAKE SURE THAT CHARS HAVE REASONABLE MAGIC LEVELS. ESPECIALY KNIGHTS!!!!!!!!!!!
  double ManaMultiplier[5] = { 1.0, 1.1, 1.1, 1.4, 3.0};

	//will calculate required mana for a magic level
  uint64_t reqMana = (uint64_t)( 400 * std::pow(double(ManaMultiplier[voc]), double(maglevel-1)));
	if (reqMana % 20 < 10) //CIP must have been bored when they invented this odd rounding
    reqMana = reqMana - (reqMana % 20);
  else
    reqMana = reqMana - (reqMana % 20) + 20;

  return reqMana;
}

Container* Player::getContainer(unsigned char containerid)
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

	for(int i = 0; i < 11; i++){
		Container *container = dynamic_cast<Container*>(items[i]);
		if(container && topContainer == container){
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
#ifdef __DEBUG__
	cout << Creature::getName() << ", addContainer: " << (int)containerid << std::endl;
#endif
	if(containerid > 0xF)
		return;

	for(containerLayout::iterator cl = vcontainers.begin(); cl != vcontainers.end(); ++cl) {
		if(cl->first == containerid) {
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

#ifdef YUR_CLEAN_MAP
containerLayout::const_iterator Player::closeContainer(unsigned char containerid)
{
	for (containerLayout::iterator cl = vcontainers.begin(); cl != vcontainers.end(); ++cl)
		if (cl->first == containerid)
			return vcontainers.erase(cl);

	return vcontainers.end();
}
#else //YUR_CLEAN_MAP
void Player::closeContainer(unsigned char containerid)
{
  for(containerLayout::iterator cl = vcontainers.begin(); cl != vcontainers.end(); ++cl)
  {
	  if(cl->first == containerid)
	  {
		  vcontainers.erase(cl);
			break;
		}
	}

 #ifdef __DEBUG__
	cout << Creature::getName() << ", closeContainer: " << (int)containerid << std::endl;
 #endif
}
#endif //YUR_CLEAN_MAP

int Player::getLookCorpse(){
	if(sex != 0)
		return ITEM_MALE_CORPSE;
	else
		return ITEM_FEMALE_CORPSE;
}

void Player::dropLoot(Container *corpse)
{
#ifdef TLM_SKULLS_PARTY
	if (skullType == SKULL_RED)
	{
		for (int slot = 0; slot < 11; slot++)
		{
			if (items[slot])
			{
				corpse->addItem(items[slot]);
				items[slot] = NULL;
			}
		}
		return;
	}
#endif //TLM_SKULLS_PARTY

#ifdef YUR_AOL
	if (items[SLOT_NECKLACE] && items[SLOT_NECKLACE]->getID() == ITEM_AOL)
	{
		removeItemInventory(SLOT_NECKLACE);
		return;
	}
#endif //YUR_AOL

	for (int slot = 0; slot < 11; slot++)
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
  for (int slot = SLOT_RIGHT; slot <= SLOT_LEFT; slot++)
  {
    if (items[slot])
    {
		if ((items[slot]->isWeapon())) {
			Item *DistItem;
			switch (items[slot]->getWeaponType()){
			case DIST:
				DistItem = GetDistWeapon();
				if(DistItem){
					return FIGHT_DIST;
				}
				else{
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

void Player::removeDistItem(){
	Item *DistItem = GetDistWeapon();
	unsigned char sl_id;
	if(DistItem){
		if(DistItem->isStackable() == false)
			return;

		if(DistItem == getTradeItem())
			g_game.playerCloseTrade(this);

		//remove one dist item
		unsigned char n = (unsigned char)DistItem->getItemCountOrSubtype();
		if(DistItem == items[SLOT_RIGHT]){
			sl_id = SLOT_RIGHT;
		}
		else if(DistItem == items[SLOT_LEFT]){
			sl_id = SLOT_LEFT;
		}
		else if(DistItem == items[SLOT_AMMO]){
			sl_id = SLOT_AMMO;
		}

		if(n > 1){
			DistItem->setItemCountOrSubtype(n-1);
		}
		else{
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
	if(type == FIGHT_DIST) {
		Item *DistItem = GetDistWeapon();
		if(DistItem){
			return DistItem->getSubfightType();
		}
	}
    if(type == FIGHT_MAGICDIST) {
		Item *DistItem = GetDistWeapon();
		if(DistItem){
			return DistItem->getSubfightType();
		}
	}
	return DIST_NONE;
}

Item * Player::GetDistWeapon() const{
	for (int slot = SLOT_RIGHT; slot <= SLOT_LEFT; slot++){
	    if (items[slot]){
			if ((items[slot]->isWeapon())) {
				switch (items[slot]->getWeaponType()){
				case DIST:
					//find ammunition
					if(items[slot]->getAmuType() == AMU_NONE){
						return items[slot];
					}
					if(items[SLOT_AMMO]){
						//compare ammo types
						if(items[SLOT_AMMO]->getWeaponType() == AMO &&
							items[slot]->getAmuType() == items[SLOT_AMMO]->getAmuType()){
								return items[SLOT_AMMO];
						}
						else{
							return NULL;
						}

					}
					else{
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

void Player::addStorageValue(const unsigned long key, const long value){
	storageMap[key] = value;
}

bool Player::getStorageValue(unsigned long key, long &value) const{
	StorageMap::const_iterator it;
	it = storageMap.find(key);
	if(it != storageMap.end()){
		value = it->second;
		return true;
	}
	else{
		value = 0;
		return false;
	}
}

bool Player::CanSee(int x, int y, int z) const
{
  return client->CanSee(x, y, z);
}

void Player::setAcceptTrade(bool b)
{
	if(b) {
		tradeState = TRADE_ACCEPT;
		//acceptTrade = true;
	}
	else {
		tradeItem = NULL;
		tradePartner = 0;
		tradeState = TRADE_NONE;
		//acceptTrade = false;
	}
}

Container* Player::getDepot(unsigned long depotId){
	DepotMap::iterator it = depots.find(depotId);
	if (it != depots.end()){
      return it->second;
	}
	return NULL;
}

bool Player::addDepot(Container* depot,unsigned long depotId){
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
void Player::sendChangeSpeed(Creature* creature){
     client->sendChangeSpeed(creature);
}

void Player::sendToChannel(Creature *creature,SpeakClasses type, const std::string &text, unsigned short channelId){
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

void Player::sendStats(){
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

void Player::flushMsg(){
	client->flushOutputBuffer();
}
void Player::sendTextMessage(MessageClasses mclass, const char* message,const Position &pos, unsigned char type) const
{
	client->sendTextMessage(mclass,message,pos,type);
}

void Player::sendPing(){
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
}

void Player::receivePing(){
	if(npings > 0)
		npings--;
}

void Player::sendDistanceShoot(const Position &from, const Position &to, unsigned char type){
	client->sendDistanceShoot(from, to,type);
}

void Player::sendMagicEffect(const Position &pos, unsigned char type){
	client->sendMagicEffect(pos,type);
}
void Player::sendAnimatedText(const Position &pos, unsigned char color, std::string text){
	client->sendAnimatedText(pos,color,text);
}

void Player::sendCreatureHealth(const Creature *creature){
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

void Player::sendTextWindow(Item* item,const unsigned short maxlen, const bool canWrite){
	client->sendTextWindow(item,maxlen,canWrite);
}

void Player::sendCloseContainer(unsigned char containerid){
	client->sendCloseContainer(containerid);
}

void Player::sendContainer(unsigned char index, Container *container){
	client->sendContainer(index,container);
}

bool Player::NeedUpdateStats(){
	if(lastSentStats.health != this->health ||
		 lastSentStats.healthmax != this->healthmax ||
		 //lastSentStats.cap != this->cap ||
		 //(int)lastSentStats.capacity != (int)this->capacity ||
		 (int)lastSentStats.freeCapacity != (int)this->getFreeCapacity() ||
		 lastSentStats.experience != this->experience ||
		 lastSentStats.level != this->level ||
		 lastSentStats.mana != this->mana ||
		 lastSentStats.manamax != this->manamax ||
		 lastSentStats.manaspent != this->manaspent ||
		 lastSentStats.maglevel != this->maglevel){
		return true;
	}
	else{
		return false;
	}
}

void Player::onThingMove(const Creature *creature, const Thing *thing, const Position *oldPos,
	unsigned char oldstackpos, unsigned char oldcount, unsigned char count)
{
  client->sendThingMove(creature, thing, oldPos, oldstackpos, oldcount, count);
}

 //container to container
void Player::onThingMove(const Creature *creature, const Container *fromContainer, unsigned char from_slotid,
	const Item* fromItem, int oldFromCount, Container *toContainer, unsigned char to_slotid,
	const Item *toItem, int oldToCount, int count)
{
	client->sendThingMove(creature, fromContainer, from_slotid, fromItem, oldFromCount, toContainer, to_slotid, toItem, oldToCount, count);
}

//inventory to container
void Player::onThingMove(const Creature *creature, slots_t fromSlot, const Item* fromItem,
	int oldFromCount, const Container *toContainer, unsigned char to_slotid, const Item *toItem, int oldToCount, int count)
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
	int oldFromCount, slots_t toSlot, const Item* toItem, int oldToCount, int count)
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
void Player::onThingMove(const Creature *creature, const Container *fromContainer, unsigned char from_slotid,
	const Item* fromItem, int oldFromCount, slots_t toSlot, const Item *toItem, int oldToCount, int count)
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
	const Item* fromItem, int oldFromCount, const Position &toPos, const Item *toItem, int oldToCount, int count)
{
	client->sendThingMove(creature, fromContainer, from_slotid, fromItem, oldFromCount, toPos, toItem, oldToCount, count);
}

//inventory to ground
void Player::onThingMove(const Creature *creature, slots_t fromSlot,
	const Item* fromItem, int oldFromCount, const Position &toPos, const Item *toItem, int oldToCount, int count)
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
void Player::onThingMove(const Creature *creature, const Position &fromPos, int stackpos, const Item* fromItem,
	int oldFromCount, const Container *toContainer, unsigned char to_slotid, const Item *toItem, int oldToCount, int count)
{
	client->sendThingMove(creature, fromPos, stackpos, fromItem, oldFromCount, toContainer, to_slotid, toItem, oldToCount, count);
}

//ground to inventory
void Player::onThingMove(const Creature *creature, const Position &fromPos, int stackpos, const Item* fromItem,
	int oldFromCount, slots_t toSlot, const Item *toItem, int oldToCount, int count)
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
void Player::setAttackedCreature(unsigned long id)
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

void Player::onThingAppear(const Thing* thing){
	client->sendThingAppear(thing);
}

void Player::onThingTransform(const Thing* thing,int stackpos){
	client->sendThingTransform(thing,stackpos);
}

void Player::onThingDisappear(const Thing* thing, unsigned char stackPos){
	client->sendThingDisappear(thing, stackPos, false);
}
//auto-close containers
void Player::onThingRemove(const Thing* thing){
	client->sendThingRemove(thing);
}

void Player::onItemAddContainer(const Container* container,const Item* item){
	client->sendItemAddContainer(container,item);
}

void Player::onItemRemoveContainer(const Container* container,const unsigned char slot){
	client->sendItemRemoveContainer(container,slot);
}

void Player::onItemUpdateContainer(const Container* container,const Item* item,const unsigned char slot){
	client->sendItemUpdateContainer(container,item,slot);
}

void Player::onCreatureTurn(const Creature *creature, unsigned char stackPos)
{
  client->sendCreatureTurn(creature, stackPos);
}

void Player::onCreatureSay(const Creature *creature, SpeakClasses type, const std::string &text)
{
  client->sendCreatureSay(creature, type, text);
}

void Player::onCreatureChangeOutfit(const Creature* creature) {
		  client->sendSetOutfit(creature);
}

int Player::onThink(int& newThinkTicks)
{
	newThinkTicks = 1000;
	return 1000;
}

void Player::onTileUpdated(const Position &pos)
{
  client->sendTileUpdated(pos);
}

void Player::onTeleport(const Creature *creature, const Position *oldPos, unsigned char oldstackpos) {
  client->sendThingMove(creature, creature,oldPos, oldstackpos, true, 1, 1);
}

void Player::addManaSpent(uint64_t spent){
	if(spent == 0)
		return;

#ifdef YUR_MULTIPLIERS
	spent *= g_config.MANA_MUL[vocation];
#endif //YUR_MULTIPLIERS

	this->manaspent += spent;
	//Magic Level Advance
	int64_t reqMana = this->getReqMana(this->maglevel+1, this->vocation);
	if (this->access < g_config.ACCESS_PROTECT && this->manaspent >= reqMana) {
		this->manaspent -= reqMana;
		this->maglevel++;
		std::stringstream MaglvMsg;
		  time_t timer;
		  static time_t previous_timer=0;
          time(&timer);  /* get current time; same as: timer = time(NULL)  */
		MaglvMsg << timer-previous_timer << "You advanced from magic level " << (this->maglevel - 1) << " to magic level " << this->maglevel << ".";
		previous_timer=timer;
		this->sendTextMessage(MSG_ADVANCE, MaglvMsg.str().c_str());
		this->sendStats();
	}
	//End Magic Level Advance*/
}

void Player::addExp(exp_t exp)
{
	this->experience += exp;
	int lastLv = this->level;
	while (this->experience >= this->getExpForLv(this->level+1)) {
		this->level++;
		this->healthmax += g_config.HP_GAIN[(int)vocation];
		this->health += g_config.HP_GAIN[(int)vocation];
		this->manamax += g_config.MANA_GAIN[(int)vocation];
		this->mana += g_config.MANA_GAIN[(int)vocation];
		this->capacity += g_config.CAP_GAIN[(int)vocation];
	}
	if(lastLv != this->level)
	{
		this->setNormalSpeed();
		g_game.changeSpeed(this->getID(), this->getSpeed());
		std::stringstream lvMsg;
		lvMsg << "You advanced from level " << lastLv << " to level " << level << ".";
		this->sendTextMessage(MSG_ADVANCE,lvMsg.str().c_str());
		this->sendStats();
	}
}

unsigned long Player::getIP() const
{
	return client->getIP();
}

void Player::die()
{
	//Magic Level downgrade
	uint64_t sumMana = 0;
	int64_t lostMana = 0;
	for (int64_t i = 1; i <= maglevel; i++) {              //sum up all the mana
		sumMana += getReqMana(i, vocation);
	}

	sumMana += manaspent;

	lostMana = (int64_t)(sumMana * g_config.DIE_PERCENT_MANA/100.0);   //player loses 10% of all spent mana when he dies

    while(lostMana > manaspent){
		lostMana -= manaspent;
		manaspent = getReqMana(maglevel, vocation);
		maglevel--;
	}
	manaspent -= lostMana;
	//End Magic Level downgrade

	//Skill loss
	long lostSkillTries;
	unsigned long sumSkillTries;
	for (int i = 0; i <= 6; i++) {  //for each skill
		lostSkillTries = 0;         //reset to 0
		sumSkillTries = 0;

		for (unsigned c = 11; c <= skills[i][SKILL_LEVEL]; c++) { //sum up all required tries for all skill levels
			sumSkillTries += getReqSkillTries(i, c, vocation);
		}

		sumSkillTries += skills[i][SKILL_TRIES];

		lostSkillTries = (long) (sumSkillTries * g_config.DIE_PERCENT_SKILL/100.0);           //player loses 10% of his skill tries

		while(lostSkillTries > (long)skills[i][SKILL_TRIES]){
			lostSkillTries -= skills[i][SKILL_TRIES];
			skills[i][SKILL_TRIES] = getReqSkillTries(i, skills[i][SKILL_LEVEL], vocation);
			if(skills[i][SKILL_LEVEL] > 10){
				skills[i][SKILL_LEVEL]--;
			}
			else{
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
	long newLevel = level;
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
		lvMsg << "You were downgraded from level " << level << " to level " << newLevel << ".";
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

		experience -= getLostExperience(); //(int)(experience*0.1f);        //0.1f is also used in die().. maybe we make a little function for exp-loss?

		while(experience < getExpForLv(level))
		{
			if(level > 1)
				level--;
			else
				break;

			// This checks (but not the downgrade sentences) aren't really necesary cause if the
			// player has a "normal" hp,mana,etc when he gets level 1 he will not lose more
			// hp,mana,etc... but here they are :P
			if ((healthmax -= g_config.HP_GAIN[(int)vocation]) < 0) //This could be avoided with a proper use of unsigend int
				healthmax = 0;

			health = healthmax;

			if ((manamax -= g_config.MANA_GAIN[(int)vocation]) < 0) //This could be avoided with a proper use of unsigend int
				manamax = 0;

			mana = manamax;

			if ((capacity -= g_config.CAP_GAIN[(int)vocation]) < 0) //This could be avoided with a proper use of unsigend int
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
	int64_t add;
	manaTick++;
	if(vocation >= 0 && vocation < 5)
	{
#ifdef YUR_PREMIUM_PROMOTION
		if (promoted)
		{
			if(manaTick < promotedGainManaVector[vocation][0])
				return false;
			manaTick = 0;
			add = promotedGainManaVector[vocation][1];
		}
		else
#endif //YUR_PREMIUM_PROMOTION
		{
			if(manaTick < gainManaVector[vocation][0])
				return false;
			manaTick = 0;
			add = gainManaVector[vocation][1];
		}
	}
	else{
		add = 5;
	}

#ifdef YUR_MULTIPLIERS
	mana += min(add * g_config.MANA_TICK_MUL, manamax - mana);
#else
	mana += min(add, manamax - mana);
#endif //YUR_MULTIPLIERS

	return true;
}

bool Player::gainHealthTick()
{
	int add;
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
	else{
		add = 5;
	}

#ifdef YUR_MULTIPLIERS
	health += min(add * g_config.HEALTH_TICK_MUL, healthmax - health);
#else
	health += min(add, healthmax - health);
#endif //YUR_MULTIPLIERS

	return true;
}

void Player::removeList()
{
	listPlayer.removeList(getID());

	for(AutoList<Player>::listiterator it = Player::listPlayer.list.begin(); it != Player::listPlayer.list.end(); ++it)
	{
		(*it).second->notifyLogOut(this);
	}

}

void Player::addList()
{
	for(AutoList<Player>::listiterator it = Player::listPlayer.list.begin(); it != Player::listPlayer.list.end(); ++it)
	{
		(*it).second->notifyLogIn(this);
	}

	listPlayer.addList(this);
}

void Player::notifyLogIn(Player* login_player)
{
	VIPListSet::iterator it = VIPList.find(login_player->getGUID());
	if(it != VIPList.end()){
		client->sendVIPLogIn(login_player->getGUID());
		sendTextMessage(MSG_SMALLINFO, (login_player->getName() + " has logged in.").c_str());
	}
}

void Player::notifyLogOut(Player* logout_player)
{
	VIPListSet::iterator it = VIPList.find(logout_player->getGUID());
	if(it != VIPList.end()){
		client->sendVIPLogOut(logout_player->getGUID());
		sendTextMessage(MSG_SMALLINFO, (logout_player->getName() + " has logged out.").c_str());
	}
}

bool Player::removeVIP(unsigned long _guid)
{
	VIPListSet::iterator it = VIPList.find(_guid);
	if(it != VIPList.end()){
		VIPList.erase(it);
		return true;
	}
	return false;
}

bool Player::addVIP(unsigned long _guid, std::string &name, bool isOnline, bool internal /*=false*/)
{
	if(guid == _guid){
		if(!internal)
			sendTextMessage(MSG_SMALLINFO, "You cannot add yourself.");
		return false;
	}

	if(VIPList.size() > 50){
		if(!internal)
			sendTextMessage(MSG_SMALLINFO, "You cannot add more players.");
		return false;
	}

	VIPListSet::iterator it = VIPList.find(_guid);
	if(it != VIPList.end()){
		if(!internal)
			sendTextMessage(MSG_SMALLINFO, "You have already added this player.");
		return false;
	}

	VIPList.insert(_guid);

	if(!internal)
		client->sendVIP(_guid, name, isOnline);

	return true;
}


#ifdef TLM_BUY_SELL
bool Player::getCoins(unsigned long requiredcoins)
{
   unsigned long coins = 0;
   for(int slot = 1; slot <= 10; slot++){
       Item *item = items[slot];
       if (item){
           Container *container = dynamic_cast<Container*>(item);
           if (item->getID() == ITEM_COINS_GOLD){
               coins += item->getItemCountOrSubtype();
           }
           else if(item->getID() == ITEM_COINS_PLATINUM){
               coins += 100*item->getItemCountOrSubtype();
           }
           else if(item->getID() == ITEM_COINS_CRYSTAL){
               coins += 10000*item->getItemCountOrSubtype();
           }
           else if(container){
               coins = getContainerCoins(container, coins);
           }
       }
   }
   if (coins >= requiredcoins){
       return true;
   }
   else{
       return false;
   }
}

unsigned long Player::getContainerCoins(Container* container, unsigned long coins)
{
   for(int number = container->size()-1; number >= 0; number--){
       Container *subcontainer = dynamic_cast<Container*>(container->getItem(number));
       if (container->getItem(number)->getID() == ITEM_COINS_GOLD){
           coins += container->getItem(number)->getItemCountOrSubtype();
       }
       else if(container->getItem(number)->getID() == ITEM_COINS_PLATINUM){
           coins += 100*container->getItem(number)->getItemCountOrSubtype();
       }
       else if(container->getItem(number)->getID() == ITEM_COINS_CRYSTAL){
           coins += 10000*container->getItem(number)->getItemCountOrSubtype();
       }
       else if(subcontainer){
           coins = getContainerCoins(subcontainer, coins);
       }
   }
   return coins;
}

bool Player::getItem(int itemid, int count)
{
   for(int slot = 1; slot <= 10; slot++){
       if (count <= 0){
           return true;
       }
       Item *item = items[slot];
       if (item){
           Container *container = dynamic_cast<Container*>(item);
           if (item->getID() == itemid){
               if (item->isStackable()){
                   count -= item->getItemCountOrSubtype();
               }
               else{
                   return true;
               }
           }
           else if(container){
               count = getContainerItem(container, itemid, count);
           }
       }
   }
   return false;
}

signed long Player::getContainerItem(Container* container, int itemid, int count)
{
   for(int number = container->size()-1; number >= 0; number--){
       Container *subcontainer = dynamic_cast<Container*>(container->getItem(number));
       if (container->getItem(number)->getID() == itemid){
           if (container->getItem(number)->isStackable()){
               count -= container->getItem(number)->getItemCountOrSubtype();
           }
           else{
               count = 0;
               return count;
           }
       }
       else if(subcontainer){
           Container *subcontainer = dynamic_cast<Container*>(container->getItem(number));
           count = getContainerItem(subcontainer, itemid, count);
       }
   }
   return count;
}

bool Player::removeCoins(signed long cost)
{
   int value;
   for(int slot = 0; slot <= 10; slot++){
       Item *item = items[slot];
       if (item && cost > 0){
           Container *container = dynamic_cast<Container*>(item);
           if (item->getID() == ITEM_COINS_GOLD || item->getID() == ITEM_COINS_PLATINUM || item->getID() == ITEM_COINS_CRYSTAL){
               value = 1;
               if (item->getID() == ITEM_COINS_PLATINUM){
                   value = 100;
               }
               else if(item->getID() == ITEM_COINS_CRYSTAL){
                   value = 10000;
               }
               if (item->getItemCountOrSubtype()*value > cost){
                   int worth = (cost/value);
                   if((double)(cost/value) <= 1){
                       worth = 1;
                   }
                   signed long newcost = -1*(value*(item->getItemCountOrSubtype()-(item->getItemCountOrSubtype()-worth))-cost);
                   if (item->getItemCountOrSubtype()-worth > 0){
                       item->setItemCountOrSubtype(item->getItemCountOrSubtype()-worth);
                   }
                   else{
                       items[slot] = NULL;
                   }
                   cost = newcost;
                   if (cost < 0){
                       payBack(cost*-1);
                       cost = 0;
                   }
               }
               else if(item->getItemCountOrSubtype()*value <= cost){
                   cost -= item->getItemCountOrSubtype()*value;
                   items[slot] = NULL;
               }
               sendInventory(slot);
           }
           else if(container){
               cost = removeContainerCoins(container, cost);
           }
       }
   }
   if (cost > 0){
       if(!removeCoins(cost)){
           return false;
       }
   }
   return true;
}

signed long Player::removeContainerCoins(Container* container, signed long cost)
{
   int value;
   for(int number = 0; number < container->size(); number++){
       if (cost <= 0){
           return cost;
       }
       Item *item = container->getItem(number);
       Container *subcontainer = dynamic_cast<Container*>(item);
       if (item->getID() == ITEM_COINS_GOLD || item->getID() == ITEM_COINS_PLATINUM || item->getID() == ITEM_COINS_CRYSTAL){
           value = 1;
           if (item->getID() == ITEM_COINS_PLATINUM){
               value = 100;
           }
           else if(item->getID() == ITEM_COINS_CRYSTAL){
               value = 10000;
           }
           if (item->getItemCountOrSubtype()*value > cost){
               int worth = (cost/value);
               if((double)(cost/value) <= 1){
                   worth = 1;
               }
               signed long newcost = -1*(value*(item->getItemCountOrSubtype()-(item->getItemCountOrSubtype()-worth))-cost);
               if (item->getItemCountOrSubtype()-worth > 0){
                   item->setItemCountOrSubtype(item->getItemCountOrSubtype()-worth);
                   onItemUpdateContainer(container, item, number);
               }
               else{
                   container->removeItem(item);
                   onItemRemoveContainer(container, number);
                   number--;
               }
               cost = newcost;
               if (cost < 0){
                   payBack(cost*-1);
                   cost = 0;
               }
           }
           else if(item->getItemCountOrSubtype()*value <= cost){
               cost -= item->getItemCountOrSubtype()*value;
               container->removeItem(item);
               onItemRemoveContainer(container, number);
               number--;
           }
       }
       else if(subcontainer){
           cost = removeContainerCoins(subcontainer, cost);
       }
   }
   return cost;
}

bool Player::removeItem(int itemid, int count)
{
	for (int slot = 1; slot <= 10 && count > 0; slot++)
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

signed long Player::removeContainerItem(Container* container, int itemid, int count)
{
   for(int number = 0; number < container->size(); number++){
       if (count > 0){
           Item *item = container->getItem(number);
           Container *subcontainer = dynamic_cast<Container*>(item);
           if (item->getID() == itemid){
               if (item->isStackable() && item->getItemCountOrSubtype()-count > 0){
                   item->setItemCountOrSubtype(item->getItemCountOrSubtype()-count);
                   onItemUpdateContainer(container, item, number);
                   count = 0;
               }
               else if(item->isStackable() && item->getItemCountOrSubtype()-count <= 0){
                   count -= item->getItemCountOrSubtype();
                   container->removeItem(item);
                   onItemRemoveContainer(container, number);
                   number--;
               }
               else{
                   container->removeItem(item);
                   onItemRemoveContainer(container, number);
                   count = 0;
            }
           }
           else if(subcontainer){
               count = removeContainerItem(subcontainer, itemid, count);
           }
       }
       else{
           return count;
       }
   }
   return count;
}

void Player::payBack(unsigned long cost)
{
	if (cost/10000 > 100)
	{
		std::cout << "Player: payBack: too much money to pay back!" << std::endl;
		return;
	}

   if (cost/10000 > 0){
       TLMaddItem(ITEM_COINS_CRYSTAL, (unsigned char)(cost/10000));
       cost -= 10000*(cost/10000);
   }
   if (cost/100 > 0){
       TLMaddItem(ITEM_COINS_PLATINUM, (unsigned char)(cost/100));
       cost -= 100*(cost/100);
   }
   if (cost > 0 && cost < 100){
       TLMaddItem(ITEM_COINS_GOLD, (unsigned char)cost);
   }
}

void Player::TLMaddItem(int itemid, unsigned char count)
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
	else{
       for(int slot = 1; slot <= 10; slot++){
          Container *container = dynamic_cast<Container*>(items[slot]);
          if (container && container->size() < container->capacity()){
                 container->addItem(item);
                 onItemAddContainer(container,item);
                 return;
         }
     }
       Tile *playerTile = g_game.getTile(pos.x, pos.y, pos.z);
	   if (item->isStackable()){
           Item *toItem = dynamic_cast<Item*>(playerTile->getThingByStackPos(playerTile->getThingCount() - 1));
           if (toItem){
               if (item->getID() == toItem->getID()){
                   if (toItem->getItemCountOrSubtype()+item->getItemCountOrSubtype() <= 100){
                       toItem->setItemCountOrSubtype(toItem->getItemCountOrSubtype()+item->getItemCountOrSubtype());
                   }
                   else{
                       int oldcount = toItem->getItemCountOrSubtype();
                       toItem->setItemCountOrSubtype(100);
                       playerTile->addThing(Item::CreateItem(item->getID(), 100-oldcount));
                   }
               }
           }
           else{
               playerTile->addThing(item);
           }
       }
       else{
           playerTile->addThing(item);
	   }
	   item->pos =  pos;
	   g_game.sendAddThing(this, pos, item);
	}
}
#endif //TLM_BUY_SELL


#ifdef BD_HOUSE_WINDOW
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
		}
	}

	editedHouse = NULL;
	editedHouseRights = HOUSE_NONE;
}
#endif //BD_HOUSE_WINDOW


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


#ifdef YUR_GUILD_SYSTEM
void Player::setGuildInfo(gstat_t gstat, unsigned long gid, std::string gname, std::string rank, std::string nick)
{
	guildStatus = gstat;
	guildId = gid;
	guildName = gname;
	guildRank = rank;
	guildNick = nick;
}
#endif //YUR_GUILD_SYSTEM


#ifdef TR_ANTI_AFK
void Player::notAfk()
{
	idleTime = 0;
	warned = false;
}

void Player::checkAfk(int thinkTicks)
{
	if (idleTime < g_config.KICK_TIME)
		idleTime += thinkTicks;

	if (idleTime < g_config.KICK_TIME && idleTime > (g_config.KICK_TIME - 60000) && !warned)	// send warning for kick (1 min)
	{
		sendTextMessage(MSG_RED_TEXT,"Warning: You have been afk for too long. You will be kicked in a minute.");
		warned = true;
	}

	if (idleTime >= g_config.KICK_TIME && warned)	// player still afk time to kick
		kickPlayer();
}
#endif //TR_ANTI_AFK


#ifdef ELEM_VIP_LIST
void Player::sendVipLogin(std::string vipname)
{
	for (int i = 0; i < MAX_VIPS; i++)
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
	for (int i = 0; i < MAX_VIPS; i++)
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
#endif //ELEM_VIP_LIST


#ifdef YUR_CMD_EXT
exp_t Player::getExpForNextLevel()
{
	return getExpForLv(level + 1) - experience;
}

uint64_t Player::getManaForNextMLevel()
{
	return getReqMana(maglevel+1, vocation) - manaspent;
}
#endif //YUR_CMD_EXT


#ifdef YUR_RINGS_AMULETS
void Player::checkRing(int thinkTics)
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

 #ifdef YUR_INVISIBLE
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
 #endif //YUR_INVISIBLE
}
#endif //YUR_RINGS_AMULETS


#ifdef TLM_SKULLS_PARTY
bool Player::checkSkull(int thinkTicks)
{
	if (banned)
		kickPlayer();

	bool skullChanged = false;
	if (skullTicks > 0)
	{
		skullTicks -= thinkTicks;
		if (skullTicks <= 0)
		{
			skullTicks = 0;
			skullType = SKULL_NONE;
			skullChanged = true;
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

void Player::onPartyIcons(const Player *playa, int icontype, bool skull, bool removeskull)
{
	client->sendPartyIcons(playa, icontype, skull, removeskull);
}
#endif //TLM_SKULLS_PARTY


#ifdef YUR_ROOKGARD
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
#endif //YUR_ROOKGARD


#ifdef YUR_LEARN_SPELLS
void Player::learnSpell(const std::string& words)
{
	learnedSpells.push_back(words);
}

bool Player::knowsSpell(const std::string& words) const
{
	return std::find(learnedSpells.begin(), learnedSpells.end(), words) != learnedSpells.end();
}
#endif //YUR_LEARN_SPELLS


#ifdef BDB_REPLACE_SPEARS
bool Player::isUsingSpears() const
{
	return (items[SLOT_LEFT] && items[SLOT_LEFT]->getID() == ITEM_SPEAR) ||
		(items[SLOT_RIGHT] && items[SLOT_RIGHT]->getID() == ITEM_SPEAR);
}
#endif //BDB_REPLACE_SPEARS


#ifdef SD_BURST_ARROW
bool Player::isUsingBurstArrows() const
{
	return ((items[SLOT_LEFT] && items[SLOT_LEFT]->getID() == ITEM_BOW) ||
		(items[SLOT_RIGHT] && items[SLOT_RIGHT]->getID() == ITEM_BOW)) &&
		(items[SLOT_AMMO] && items[SLOT_AMMO]->getID() == ITEM_BURST_ARROW);
}
#endif //SD_BURST_ARROW


#ifdef YUR_LIGHT_ITEM
void Player::checkLightItem(int /*thinkTics*/)
{
	int lightItemNow = items[SLOT_AMMO]? items[SLOT_AMMO]->getID() : 0;

	if (lightItemNow != lightItem)
	{
		if (Item::items[lightItem].lightLevel != Item::items[lightItemNow].lightLevel)
			g_game.creatureChangeLight(this, 0,
				Item::items[lightItemNow].lightLevel,
				Item::items[lightItemNow].lightColor);

		lightItem = lightItemNow;
	}
}
#endif //YUR_LIGHT_ITEM


#ifdef YUR_PREMIUM_PROMOTION
void Player::checkPremium(int thinkTics)
{
	if (premiumTicks > 0)
	{
		premiumTicks -= thinkTics;
		if (premiumTicks < 0)
			premiumTicks = 0;
	}
}
#endif //YUR_PREMIUM_PROMOTION


#ifdef JD_DEATH_LIST
void Player::addDeath(const std::string& killer, int level, time_t time)
{
	Death death = { killer, level, time };
	deathList.push_back(death);

	while (deathList.size() > g_config.MAX_DEATH_ENTRIES)
		deathList.pop_front();
}
#endif //JD_DEATH_LIST


#ifdef CVS_DAY_CYCLE
void Player::sendWorldLightLevel(unsigned char lightlevel, unsigned char color)
{
	if(last_worldlightlevel != lightlevel){
		client->sendWorldLightLevel(lightlevel, color);
		last_worldlightlevel = lightlevel;
	}
}

void Player::sendPlayerLightLevel(Player* player)
{
	client->sendPlayerLightLevel(player);
}
#endif //CVS_DAY_CYCLE


#ifdef JD_WANDS
int Player::getWandId() const
{
	for (int slot = SLOT_RIGHT; slot <= SLOT_LEFT; slot++)
	{
		int id = items[slot]? items[slot]->getID() : 0;
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
#endif //JD_WANDS

