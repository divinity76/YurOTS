
#include "itemloader.h"

int32_t ItemLoader::setProps(attribute_t attr, void* data, datasize_t size)
{
    //attribute
    if(!writeData(&attr, sizeof(attribute_t), true))
        return getError();

    //size
    if(!writeData(&size, sizeof(datasize_t), true))
        return getError();

    //data
    if(!writeData(data, size, true))
        return getError();

    return ERROR_NONE;
}

int32_t ItemLoader::setFlags(flags_t flags)
{
    //data
    if(!writeData(&flags, sizeof(flags_t), true))
        return getError();

    return ERROR_NONE;
}

