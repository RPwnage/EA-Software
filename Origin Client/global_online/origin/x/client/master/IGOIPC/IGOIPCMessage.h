///////////////////////////////////////////////////////////////////////////////
// IGOIPCMessage.h
// 
// Copyright (c) 2012 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#ifndef IGOIPCMESSAGE_H
#define IGOIPCMESSAGE_H

#include "EASTL/string.h"

#include "IGOIPC.h"

class IGOIPCMessage
{
public:

#if defined(ORIGIN_MAC)
    IGOIPCMessage(const char* msg);
#endif

    IGOIPCMessage(IGOIPC::MessageType type, const char *data = 0, size_t size = 0);
    //IGOIPCMessage(IGOIPCMessage &msg);
    virtual ~IGOIPCMessage();

    void add(const char *data, size_t size);
    void add(int64_t data) { add((const char *)&data, sizeof(data)); }
    void add(uint64_t data) { add((const char *)&data, sizeof(data)); }
    void add(uint32_t data) { add((const char *)&data, sizeof(data)); }
    void add(uint16_t data) { add((const char *)&data, sizeof(data)); }
    void add(uint8_t data) { add((const char *)&data, sizeof(data)); }
    void add(int32_t data) { add((const char *)&data, sizeof(data)); }
    void add(int16_t data) { add((const char *)&data, sizeof(data)); }
    void add(int8_t data) { add((const char *)&data, sizeof(data)); }
    void add(bool data) { add((const char *)&data, sizeof(data)); }
    void add(float data) { add((const char *)&data, sizeof(data)); }

#if defined(ORIGIN_MAC)
    void add(ssize_t data) { add((const char *)&data, sizeof(data)); }
#endif

    void read(char *data, uint32_t size); // be careful using this
    void read(int64_t &data) { read((char *)&data, sizeof(data)); }
    void read(uint64_t &data) { read((char *)&data, sizeof(data)); }
    void read(uint32_t &data) { read((char *)&data, sizeof(data)); }
    void read(uint16_t &data) { read((char *)&data, sizeof(data)); }
    void read(uint8_t &data) { read((char *)&data, sizeof(data)); }
    void read(int32_t &data) { read((char *)&data, sizeof(data)); }
    void read(int16_t &data) { read((char *)&data, sizeof(data)); }
    void read(int8_t &data) { read((char *)&data, sizeof(data)); }
    void read(bool &data) { read((char *)&data, sizeof(data)); }
    void read(float &data) { read((char *)&data, sizeof(data)); }

#if defined(ORIGIN_MAC)
    void read(ssize_t &data) { read((char *)&data, sizeof(data)); }
#endif

    const char* readPtr() { return mData.c_str() + mReadPtr; }
    uint32_t remainingSize() { return (uint32_t)mData.size() -  mReadPtr; }

    IGOIPC::MessageType type() { return mType; }
    const char *data() { return mData.c_str(); }
    uint32_t size() { return (uint32_t)mData.size(); }
    void reset() { mReadPtr = 0; }

    void setType(IGOIPC::MessageType type) { mType = type; }
    void setData(const char *data, size_t size) { mData.clear(); if (size) add(data, size); }

private:
    IGOIPC::MessageType mType;
    eastl::string mData;
    uint32_t mReadPtr;
};

#endif
