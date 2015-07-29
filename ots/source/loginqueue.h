//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// class representing the login queue by Yurez
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
#ifdef YUR_LOGIN_QUEUE

#ifndef LOGINQUEUE_H
#define LOGINQUEUE_H
#include <time.h>
#include <list>
#include <iterator>
#include <string>

struct LoginTry;
typedef std::list<LoginTry> LoginTryList;
typedef LoginTryList::iterator LoginTryListIterator;

enum qstate_t {
	LOGGED = 0,
	ACTIVE = 1,
	DEAD = 2
};

struct LoginTry
{
	int accountNumber;
	time_t tryTime;		///< time of last login try
	qstate_t state;
	LoginTry(int acc, qstate_t stat = ACTIVE): accountNumber(acc), tryTime(time(0)), state(stat) {}
};

class LoginQueue
{
private:
	static const int LOGGED_TIMEOUT = 30, ACTIVE_TIMEOUT = 60, DEAD_TIMEOUT = 15*60;
	LoginTryList lq;
	LoginTryListIterator findAccount(int account, int* realPos, int* effectivePos);
	void push(int account);
	void removeDeadEntries();

public:
	LoginQueue() {}
	bool load();
	bool save();
	bool login(int account, int playersOnline, int maxPlayers, int* placeInQueue);
	size_t size() const { return lq.size(); }
	void show();
};

#endif //LOGINQUEUE_H

#endif //YUR_LOGIN_QUEUE
