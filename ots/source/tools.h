#ifndef __OTSERV_TOOLS_H
#define __OTSERV_TOOLS_H

#include <libxml/parser.h>
#include "otsystem.h"
typedef std::vector<std::string> StringVec;

bool fileExists(const char* filename);
//float box_muller(float m, float s);
int32_t random_range(int32_t lowest_number, int32_t highest_number);
uint32_t rand24b();
void hexdump(unsigned char *_data, int32_t _len);
char upchar(char c);
uint32_t getIPSocket(SOCKET s);
void upper(char *upstr, char *str);
void upper(char *upstr, char *str, int32_t n);
int32_t safe_atoi(const char* str);
double timer();
std::string article(const std::string& name);
std::string tickstr(int32_t ticks);
void toLowerCaseString(std::string& source);
bool readXMLInteger(xmlNodePtr node, const char* tag, int32_t &value);
bool readXMLString(xmlNodePtr node, const char* tag, std::string& value);
std::string trimString(std::string& str);
bool isLowercaseLetter(char character);
bool isUppercaseLetter(char character);
bool isValidName(std::string text, bool forceUppercaseOnFirstLetter = true);
StringVec explodeString(const std::string& string, const std::string& separator);
#endif
