#include <algorithm>
#include <functional>
#include <iostream>

#include "definitions.h"

#include "account.h"

Account::Account()
{
	accnumber = 0;
	accType = 0;
	premDays = 90;
}

Account::~Account()
{
	charList.clear();
}
