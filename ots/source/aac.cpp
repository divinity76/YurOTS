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
#ifdef YUR_BUILTIN_AAC

#include "aac.h"
#include "ioaccount.h"
#include "ioplayer.h"
#include "player.h"
#include <fstream>
#include <sstream>
#include <boost/regex.hpp>

std::string AccountCreator::process(std::string request)
{
	if (!g_config.ACCMAKER)
		return "Sorry, account creator disabled";

	boost::regex pattern("^([a-zA-Z ]{3,20}),([a-zA-Z0-9]{3,20}),([01]),([1-4])$");
	boost::smatch part;
	if (!boost::regex_match(request, part, pattern))
		return "Sorry, invalid request format";

		// gm check
	if (tolower(request[0]) == 'g' && tolower(request[1]) == 'm')
		return "Sorry, name has GM prefix";

		// get data
	std::string name(part[1]);
	std::string pass(part[2]);
	int sex = std::string(part[3]).at(0) - '0';
	int voc = std::string(part[4]).at(0) - '0';

		// set rook mode
	if (g_config.ACCMAKER_ROOK)
		voc = 0;

		// check if name exists
	std::ifstream file((g_config.DATA_DIR + "players/" + name + ".xml").c_str());
	if (file)
		return "Sorry, name already in use";

		// create account
	Account account;
	account.accnumber = uniqueAccountNumber();
	account.password = pass;
	account.charList.push_back(name);

	if (account.accnumber < 0)
		return "Sorry, not enough free account numbers";

		// load player template
	Player* player = new Player(name, NULL);
	if (!IOPlayer::instance()->loadPlayer(player, str(voc)))
		return "Sorry, vocation not available";

		// fill with provided data
	player->setName(name);
	player->setAccountNumber(account.accnumber);
	player->setSex((playersex_t)sex);
	player->looktype = sex? PLAYER_MALE_1 : PLAYER_FEMALE_1;
	player->lookmaster = player->looktype;

		// save all to new files
	if (!IOAccount::instance()->saveAccount(account))
		return "Sorry, could not create account";

	if (!IOPlayer::instance()->savePlayer(player)) 
	{
		delete player;
		return "Sorry, could not create player";
	}

	delete player;
	return std::string("Your account number is ") + str(account.accnumber);
}

long AccountCreator::uniqueAccountNumber()
{
	std::ostringstream filename;
	static const long ACCNUM_MIN = 100000L, ACCNUM_MAX = 999999L, ACCNUM_RANGE = ACCNUM_MAX-ACCNUM_MIN+1;
	int maxAttempts = 10000;

	while (maxAttempts-- > 0)
	{
		long a = rand(), b = rand();
		long num = (a + b*RAND_MAX) % ACCNUM_RANGE + ACCNUM_MIN;

		filename.str("");
		filename << g_config.DATA_DIR << "accounts/" << num << ".xml" << std::ends;
		std::ifstream file(filename.str().c_str());

		if (file)	// account already exists
			file.close();
		else
			return num;
	}

	return -1;
}

#endif //YUR_BUILTIN_AAC
