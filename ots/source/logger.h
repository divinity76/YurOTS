#ifndef __LOGGER_H
#define __LOGGER_H

#ifdef __GNUC__
#define __OTSERV_PRETTY_FUNCTION__ __PRETTY_FUNCTION__
#endif
#ifdef _MSC_VER
#define __OTSERV_PRETTY_FUNCTION__ __FUNCDNAME__
#endif

#define LOG_MESSAGE(channel, type, level, message) \
	Logger::getInstance()->logMessage(channel, type, level, message, __OTSERV_PRETTY_FUNCTION__, __LINE__, __FILE__);

#include <string>
#include <map>



enum eLogType
{
    EVENT,
    WARNING,
    BLAD
};

class Logger
{
public:
    static Logger* getInstance();
    void logMessage(std::string channel, eLogType type, int32_t level,
                    std::string message, std::string func,
                    int32_t line, std::string file);
private:
    static Logger* instance;
    Logger();
};

#endif
