#ifndef SAFE_ATOI_H
#define SAFE_ATOI_H

int safe_atoi(const char* str)
{
	if (str)
		return atoi(str);
	else
		return 0;
}

#endif