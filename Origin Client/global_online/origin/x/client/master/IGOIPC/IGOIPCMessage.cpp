///////////////////////////////////////////////////////////////////////////////
// IGOIPCMessage.cpp
// 
// Copyright (c) 2012 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#include "IGOIPCMessage.h"
uint32_t gIPCidCounter=0;

#if defined(ORIGIN_MAC)
IGOIPCMessage::IGOIPCMessage(const char* msg)
{
    IGOIPC::MessageHeader* header = (IGOIPC::MessageHeader*)msg;
    mType = header->type;
    
    mReadPtr = 0;
    
    if (header->length > 0)
        add((const char*)(header + 1), header->length);

#ifdef _DEBUG_IPC_MESSAGES_
    gIPCidCounter++;
    wchar_t debugMessage[256];
    wsprintf(debugMessage, L"\nIPCMessage %i created\n", gIPCidCounter);
    OutputDebugString(debugMessage);
#endif
}
#endif

IGOIPCMessage::IGOIPCMessage(IGOIPC::MessageType type, const char *data, size_t size) :
    mType(type),
    mReadPtr(0)
{
    add(data, size);

#ifdef _DEBUG_IPC_MESSAGES_
    gIPCidCounter++;
    wchar_t debugMessage[256];
    wsprintf(debugMessage, L"\nIPCMessage %i created\n", gIPCidCounter);
    OutputDebugString(debugMessage);
#endif
}

IGOIPCMessage::~IGOIPCMessage()
{
#ifdef _DEBUG_IPC_MESSAGES_
    wchar_t debugMessage[256];
    wsprintf(debugMessage, L"\nIPCMessage %i destroyed\n", gIPCidCounter);
    OutputDebugString(debugMessage);
#endif
}

void IGOIPCMessage::add(const char *data, size_t size)
{
    if (size > 0 && data != NULL)
        mData.append(data, size);
}

void IGOIPCMessage::read(char *data, uint32_t size)
{
    if (size != 0)
    {
        IGO_ASSERT((mReadPtr < mData.size()));
    
        if (remainingSize() >= size)
        {
            memcpy(data, &mData[mReadPtr], size);
            mReadPtr += size;
        }
        else
        {
            size = remainingSize();
            memcpy(data, &mData[mReadPtr], size);
            mReadPtr += size;
        }
    }
}
