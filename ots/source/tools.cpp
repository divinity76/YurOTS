//#include "preheaders.h"
#include "definitions.h"
#include <string>
#include <cmath>
#include <sstream>
#include "tools.h"


bool fileExists(const char* filename)
{
#ifdef USING_VISUAL_2005
    FILE *f = NULL;
    fopen_s(&f, filename, "rb");
#else
    FILE *f = fopen(filename, "rb");
#endif //USING_VISUAL_2005

    bool exists = (f != NULL);
    if (f != NULL)
        fclose(f);

    return exists;
}

uint32_t rand24b()
{
    return ((rand() << 12) ^ (rand())) & (0xFFFFFF);
}
/*
float box_muller(float m, float s)
{
	// normal random variate generator
	// mean m, standard deviation s
	float x1, x2, w, y1;
	static float y2;

	static bool useLast = false;
	if(useLast) // use value from previous call
	{
		y1 = y2;
		useLast = false;
		return (m + y1 * s);
	}

	do
	{
		double r1 = (((float)(rand()) / RAND_MAX));
		double r2 = (((float)(rand()) / RAND_MAX));

		x1 = 2.0 * r1 - 1.0;
		x2 = 2.0 * r2 - 1.0;
		w = x1 * x1 + x2 * x2;
	}
	while(w >= 1.0);
	w = sqrt((-2.0 * log(w)) / w);

	y1 = x1 * w;
	y2 = x2 * w;

	useLast = true;
	return (m + y1 * s);
}*/

//////////////////////////////////////////////////
// get a random value between lowest_number and highest_number
int32_t random_range(int32_t lowestNumber, int32_t highestNumber)
{
    if(highestNumber == lowestNumber)
        return lowestNumber;

    if(lowestNumber > highestNumber)
        std::swap(lowestNumber, highestNumber);

    return (lowestNumber + ((int32_t)rand24b() % (highestNumber - lowestNumber + 1)));
}



/*
int32_t random_range(int32_t lowest_number, int32_t highest_number)
{
	if(lowest_number > highest_number){
		int32_t nTmp = highest_number;
		highest_number = lowest_number;
		lowest_number = nTmp;
    }

    double range = highest_number - lowest_number + 1;
    return lowest_number + int32_t(range * rand()/(RAND_MAX + 1.0));
}*/

//////////////////////////////////////////////////
// dump a part of the memory to stderr.
void hexdump(unsigned char *_data, int32_t _len)
{
    int32_t i;
    for (; _len > 0; _data += 16, _len -= 16)
    {
        for (i = 0; i < 16 && i < _len; i++)
            fprintf(stderr, "%02x ", _data[i]);
        for (; i < 16; i++)
            fprintf(stderr, "   ");
        fprintf(stderr, " ");
        for (i = 0; i < 16 && i < _len; i++)
            fprintf(stderr, "%c", (_data[i] & 0x70) < 32 ? '·' : _data[i]);
        fprintf(stderr, "\n");
    }
}

#if 0
//////////////////////////////////////////////////
// Enable asynchronous function calls.
// This function does not wait for return of the called function;
// instead, this function returns immediately.
// The called function must be of type void *fn(void *).
// You can use the pointer to the function for anything you want to.
// Return: a thread handle.
pthread_t *detach(void *(*_fn)(void *), void *_arg)
{
    pthread_t *thread = new pthread_t();
    if (pthread_create(thread, NULL, _fn, _arg))
        perror("pthread");
    return thread;
}
#endif

//////////////////////////////////////////////////
// Upcase a char.
char upchar(char c)
{
    if (c >= 'a' && c <= 'z')
        return c - 'a' + 'A';
    else if (c == 'à')
        return 'À';
    else if (c == 'á')
        return 'Á';
    else if (c == 'â')
        return 'Â';
    else if (c == 'ã')
        return 'Ã';
    else if (c == 'ä')
        return 'Ä';
    else if (c == 'å')
        return 'Å';
    else if (c == 'æ')
        return 'Æ';
    else if (c == 'ç')
        return 'Ç';
    else if (c == 'è')
        return 'È';
    else if (c == 'é')
        return 'É';
    else if (c == 'ê')
        return 'Ê';
    else if (c == 'ë')
        return 'Ë';
    else if (c == 'ì')
        return 'Ì';
    else if (c == 'í')
        return 'Í';
    else if (c == 'î')
        return 'Î';
    else if (c == 'ï')
        return 'Ï';
    else if (c == 'ð')
        return 'Ð';
    else if (c == 'ñ')
        return 'Ñ';
    else if (c == 'ò')
        return 'Ò';
    else if (c == 'ó')
        return 'Ó';
    else if (c == 'ô')
        return 'Ô';
    else if (c == 'õ')
        return 'Õ';
    else if (c == 'ö')
        return 'Ö';
    else if (c == 'ø')
        return 'Ø';
    else if (c == 'ù')
        return 'Ù';
    else if (c == 'ú')
        return 'Ú';
    else if (c == 'û')
        return 'Û';
    else if (c == 'ü')
        return 'Ü';
    else if (c == 'ý')
        return 'Ý';
    else if (c == 'þ')
        return 'Þ';
    else if (c == 'ÿ')
        return 'ß';
    else
        return c;
}

uint32_t getIPSocket(SOCKET s)
{
    sockaddr_in sain;
    socklen_t salen = sizeof(sockaddr_in);

    if(getpeername(s, (sockaddr*)&sain, &salen) == 0)
    {
#if defined WIN32 || defined __WINDOWS__
        return sain.sin_addr.S_un.S_addr;
#else
        return sain.sin_addr.s_addr;
#endif
    }

    return 0;
}

//////////////////////////////////////////////////
// Upcase a 0-terminated string.
void upper(char *upstr, char *str)
{
    for (; *str; str++, upstr++)
        *upstr = upchar(*str);
    *upstr = '\0';
}


//////////////////////////////////////////////////
// Upcase a 0-terminated string, but at most n chars.
void upper(char *upstr, char *str, int32_t n)
{
    for (; *str && n; str++, upstr++, n--)
        *upstr = upchar(*str);
    if (n) *upstr = '\0';
}

int32_t safe_atoi(const char* str)
{
    if (str)
        return atoi(str);
    else
        return 0;
}

double timer()
{
    static bool running = false;
    static _timeb start, end;

    if (!running)
    {
#ifdef USING_VISUAL_2005
        _ftime_s(&start);
#else
        _ftime(&start);
#endif //USING_VISUAL_2005
        running = true;
        return 0.0;
    }
    else
    {
#ifdef USING_VISUAL_2005
        _ftime_s(&end);
#else
        _ftime(&end);
#endif //USING_VISUAL_2005
        running = false;
        return (end.time-start.time)+(end.millitm-start.millitm)/1000.0;
    }
}

std::string article(const std::string& name)
{
    if (name.empty())
        return name;

    switch (upchar(name[0]))
    {
    case 'A':
    case 'E':
    case 'I':
    case 'O':
    case 'U':
        return std::string("an ") + name;
    default:
        return std::string("a ") + name;
    }
}

std::string tickstr(int32_t ticks)
{
    int32_t hours = (int32_t)floor(double(ticks)/(3600000.0));
    int32_t minutes = (int32_t)ceil((double(ticks) - double(hours)*3600000.0)/(60000.0));

    std::ostringstream info;
    info << hours << (hours==1? " hour " : " hours ") << minutes << (minutes==1? " minute" :" minutes");
    return info.str();
}

void toLowerCaseString(std::string& source)
{
    std::transform(source.begin(), source.end(), source.begin(), tolower);
}

bool readXMLInteger(xmlNodePtr node, const char* tag, int32_t &value)
{
    char* nodeValue = (char*)xmlGetProp(node, (xmlChar*)tag);
    if(!nodeValue)
        return false;

    value = atoi(nodeValue);
    xmlFreeOTSERV(nodeValue);
    return true;
}

bool readXMLString(xmlNodePtr node, const char* tag, std::string& value)
{
    char* nodeValue = (char*)xmlGetProp(node, (xmlChar*)tag);
    if(!nodeValue)
        return false;

    value = nodeValue;
    xmlFreeOTSERV(nodeValue);
    return true;
}

std::string trimString(std::string& str)
{
    str.erase(str.find_last_not_of(" ") + 1);
    return str.erase(0, str.find_first_not_of(" "));
}

StringVec explodeString(const std::string& string, const std::string& separator)
{
    StringVec returnVector;
    size_t start = 0, end = 0;
    while((end = string.find(separator, start)) != std::string::npos)
    {
        returnVector.push_back(string.substr(start, end - start));
        start = end + separator.size();
    }

    returnVector.push_back(string.substr(start));
    return returnVector;
}

bool isLowercaseLetter(char character)
{
    return (character >= 97 && character <= 122);
}

bool isUppercaseLetter(char character)
{
    return (character >= 65 && character <= 90);
}

bool isValidName(std::string text, bool forceUppercaseOnFirstLetter/* = true*/)
{
    uint32_t textLength = text.length(), lenBeforeSpace = 1, lenBeforeQuote = 1, lenBeforeDash = 1, repeatedCharacter = 0;
    char lastChar = 32;
    if(forceUppercaseOnFirstLetter)
    {
        if(!isUppercaseLetter(text[0]))
            return false;
    }
    else if(!isLowercaseLetter(text[0]) && !isUppercaseLetter(text[0]))
        return false;

    for(uint32_t size = 1; size < textLength; size++)
    {
        if(text[size] != 32)
        {
            lenBeforeSpace++;

            if(text[size] != 39)
                lenBeforeQuote++;
            else
            {
                if(lenBeforeQuote <= 1 || size == textLength - 1 || text[size + 1] == 32)
                    return false;

                lenBeforeQuote = 0;
            }

            if(text[size] != 45)
                lenBeforeDash++;
            else
            {
                if(lenBeforeDash <= 1 || size == textLength - 1 || text[size + 1] == 32)
                    return false;

                lenBeforeDash = 0;
            }

            if(text[size] == lastChar)
            {
                repeatedCharacter++;
                if(repeatedCharacter > 2)
                    return false;
            }
            else
                repeatedCharacter = 0;

            lastChar = text[size];
        }
        else
        {
            if(lenBeforeSpace <= 1 || size == textLength - 1 || text[size + 1] == 32)
                return false;

            lenBeforeSpace = lenBeforeQuote = lenBeforeDash = 0;
        }

        if(!(isLowercaseLetter(text[size]) || text[size] == 32 || text[size] == 39 || text[size] == 45
                || (isUppercaseLetter(text[size]) && text[size - 1] == 32)))
            return false;
    }

    return true;
}
