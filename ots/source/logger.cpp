
#include "logger.h"
#include <iostream>

Logger* Logger::instance = NULL;

Logger::Logger() {}

Logger* Logger::getInstance()
{
    if(!instance)
        instance = new Logger();
    return instance;
}

void Logger::logMessage(std::string channel, eLogType type, int32_t level,
                        std::string message, std::string func,
                        int32_t line, std::string file)
{
    std::string sType;
    switch(type)
    {
    case BLAD:
        sType = "error";
        break;
    case EVENT:
        sType = "event";
        break;
    case WARNING:
        sType = "warning";
    }
    std::cout << "Channel: " << channel << std::endl;
    std::cout << "Type: " << sType << std::endl;
    std::cout << "Level: " << level << std::endl;
    std::cout << "Messafe: " << message << std::endl;
    std::cout << "Func: " << func << std::endl;
    std::cout << "Line: " << line << std::endl;
    std::cout << "File: " << file << std::endl;
}
