#ifndef __NETWORK_MESSAGE_H__
#define __NETWORK_MESSAGE_H__

////#include "preheaders.h"
#include "otsystem.h"
#include "const76.h"

class Creature;
class Player;
class Item;
class Position;


class NetworkMessage
{
public:
    // constructor/destructor
    NetworkMessage();
    virtual ~NetworkMessage();


    // resets the internal buffer to an empty message
    void Reset();


    // socket functions
    bool ReadFromSocket(SOCKET socket);
    bool WriteToSocket(SOCKET socket);


    // simply read functions for incoming message
    unsigned char  GetByte();
    uint16_t GetU16();
    uint16_t GetItemId();
    uint32_t   GetU32();
    std::string    GetString();
    std::string	 GetRaw();
    Position       GetPosition();


    // skips count unknown/unused bytes in an incoming message
    void SkipBytes(int32_t count);


    // simply write functions for outgoing message
    void AddByte(unsigned char  value);
    void AddU16 (uint16_t value);
    void AddU32 (uint32_t   value);

    void AddString(const std::string &value);
    void AddString(const char* value);


    // write functions for complex types
    void AddPosition(const Position &pos);
    void AddItem(uint16_t id, unsigned char count);
    void AddItem(const Item *item);
    void AddItemId(const Item *item);
    void AddCreature(const Creature *creature, bool known, uint32_t remove);

    int32_t getMessageLength()
    {
        return m_MsgSize;
    }

    void JoinMessages(NetworkMessage &add);


protected:
    inline bool canAdd(int32_t size)
    {
        return (size + m_ReadPos < NETWORKMESSAGE_MAXSIZE - 16);
    };
    int32_t m_MsgSize;
    int32_t m_ReadPos;

    unsigned char m_MsgBuf[NETWORKMESSAGE_MAXSIZE];
};
#endif // #ifndef __NETWORK_MESSAGE_H__
