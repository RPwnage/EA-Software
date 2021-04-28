/////////////////////////////////////////////////////////////////////////////
// MCS.cpp
//
// Copyright (c) 2007 Criterion Software Limited and its licensors. All Rights Reserved.
// Created by Paul Pedriana
/////////////////////////////////////////////////////////////////////////////
// Some of this library is based on Nintendo sample code.
// Copyright (C)2005-2007 Nintendo  All rights reserved.
// These coded instructions, statements, and computer programs contain
// proprietary information of Nintendo of America Inc. and/or Nintendo
// Company Ltd., and are protected by Federal copyright law.  They may
// not be disclosed to third parties or copied or duplicated in any form,
// in whole or in part, without the prior written consent of Nintendo.
/////////////////////////////////////////////////////////////////////////////


#include <EAIO/internal/Config.h>
#include <EAIO/Wii/MCS.h>
#include <revolution.h>
#include <revolution/os.h>
#include <revolution/hio2.h>
#include <new>
#include <string.h>
#include <stdarg.h>
#include EA_ASSERT_HEADER


namespace
{
    using namespace EA::IO;
    using namespace EA::IO::detail;


    bool        sbMcsInitialized = false;
    OSMutex     sMcsMutex;
    CommDevice* spCommDevice = NULL;


    class HIO2CommDeviceEnumerate : public IHIO2CommDeviceEnumerate
    {
    public:
        HIO2CommDeviceEnumerate(IDeviceEnumerate* pEnumerate)
          : mpEnumerate(pEnumerate), mbContinue(false)
        { }

        virtual bool Find(int type);
        bool         IsContinue() const { return mbContinue; }

    private:
        IDeviceEnumerate* mpEnumerate;
        bool              mbContinue;
    };


    bool HIO2CommDeviceEnumerate::Find(int devType)
    {
        DeviceInfo deviceInfo;

        deviceInfo.deviceType = DEVICETYPE_NDEV;
        deviceInfo.param1     = u32(devType);
        deviceInfo.param2     = 0;
        deviceInfo.param3     = 0;

        mbContinue = mpEnumerate->Find(deviceInfo);

        return mbContinue;
    }


} // namespace



namespace EA
{
namespace IO
{


///////////////////////////////////////////////////////////////////
// MCS
///////////////////////////////////////////////////////////////////

void Mcs_Init()
{
    if(!sbMcsInitialized)
    {
        sbMcsInitialized = true;

        // OSRegisterVersion(NW4R_MCS_Version_);
        HIO2Init();
        OSInitMutex(&sMcsMutex);
    }
}


bool Mcs_IsInitialized()
{
    return sbMcsInitialized;
}


void Mcs_EnumDevices(IDeviceEnumerate* pEnumerate, u32 /*flag*/)
{
    HIO2CommDeviceEnumerate hio2CommDeviceEnumerate(pEnumerate);

    HIO2CommDevice::EnumerateDevice(&hio2CommDeviceEnumerate);

    if(hio2CommDeviceEnumerate.IsContinue())
    {
        // Proceed to next enumeration of devices
    }
}


// Returns the memory size required to create a communication device object.
//
// deviceInfo: Object storing communication device information.
//             This value can be obtained by calling the Mcs_EnumDevices function.
//
// Returns the memory size required to create a communication device object.
//
u32 Mcs_GetDeviceObjectMemSize(const DeviceInfo& deviceInfo)
{
    switch (deviceInfo.deviceType)
    {
        case DEVICETYPE_NDEV:
            return DEVICETYPE_NDEV_SIZE; // return sizeof(HIO2CommDevice) + Hio2RingBuffer::CopyAlignment + HIO2CommDevice::TEMP_BUF_SIZE_MIN;

        case DEVICETYPE_USBADAPTER:
            return DEVICETYPE_USBADAPTER_SIZE;
    }

    return 0; // Returns 0 when the type is unknown or unsupported.
}



// Creates a communication device object.
//
// deviceInfo: Object storing communication device information. This value can be obtained by calling the Mcs_EnumDevices function.
// buf:        Pointer to memory required by the communication device object.
// bufSize:    Size of the memory buffer pointed to by buf.
//
// Returns true if creation of the communications device object succeeded; returns false if it failed.
//
bool Mcs_CreateDeviceObject(const DeviceInfo& deviceInfo, void* buf, u32 bufSize)
{
    EA_ASSERT(sbMcsInitialized && !spCommDevice);

    CommDevice* pCommDevice = NULL;

    switch (deviceInfo.deviceType)
    {
        case DEVICETYPE_NDEV:
        {
            EA_ASSERT(bufSize >= Mcs_GetDeviceObjectMemSize(deviceInfo));

            void* const tempBuf     = static_cast<u8*>(buf) + sizeof(HIO2CommDevice);
            const u32   tempBufSize = bufSize - sizeof(HIO2CommDevice);

            // Calls a constructor with layout new
            pCommDevice = new(buf) HIO2CommDevice(s32(deviceInfo.param1), tempBuf, tempBufSize);
            break;
        }
    }

    if(pCommDevice)
    {
        AutoMutexLock autoLock(sMcsMutex);
        spCommDevice = pCommDevice;
        return true;
    }

    return false;
}


void Mcs_DestroyDeviceObject()
{
    EA_ASSERT(sbMcsInitialized && spCommDevice);

    AutoMutexLock autoLock(sMcsMutex);
    spCommDevice = NULL;
}


bool Mcs_IsDeviceObjectCreated()
{
    EA_ASSERT(sbMcsInitialized);

    AutoMutexLock autoLock(sMcsMutex);
    return spCommDevice != NULL;
}


// Registers the buffer for data reception.
//
// If the reception buffer becomes full of received data, and there is 
// no space available to store newly-received data, that received data 
// will be thrown away. Be sure the buffer size is large enough to hold 
// the amount of data used in communication.
//
// channel: Value for uniquely identifying the communication partner when sending and receiving data.
// buf:     The reception buffer to register
// bufSize: The size of the reception buffer to register
//
void Mcs_RegisterBuffer(ChannelType channel, void* buf, u32 bufSize)
{
    EA_ASSERT(sbMcsInitialized && spCommDevice);

    AutoMutexLock autoLock(sMcsMutex);
    spCommDevice->RegisterBuffer(channel, buf, bufSize);
}


// Deletes the registration of the receive callback function registered with 
// Mcs_RegisterRecvCallback() or the receive buffer registered with Mcs_RegisterStreamRecvBuffer().
//
// channel: Value for uniquely identifying the communication partner when sending and receiving data.
//
void Mcs_UnregisterBuffer(ChannelType channel)
{
    EA_ASSERT(sbMcsInitialized && spCommDevice);

    AutoMutexLock autoLock(sMcsMutex);
    spCommDevice->UnregisterBuffer(channel);
}


// Open communication devices.
//
// Returns 0 if the function was successful.
// Returns an error code (non-zero value) if the function fails.
//
u32 Mcs_Open()
{
    EA_ASSERT(sbMcsInitialized && spCommDevice);

    AutoMutexLock autoLock(sMcsMutex);
    return spCommDevice->Open();
}


void Mcs_Close()
{
    EA_ASSERT(sbMcsInitialized && spCommDevice);

    AutoMutexLock autoLock(sMcsMutex);
    spCommDevice->Close();
}


// You should call this function periodically from your main loop.
//
// Returns 0 if the function was successful.
// Returns an error code (non-zero value) if the function fails.
//
u32 Mcs_Polling()
{
    EA_ASSERT(sbMcsInitialized && spCommDevice);

    AutoMutexLock autoLock(sMcsMutex);
    return spCommDevice->Polling();
}


// Gets the size of data that can be read without blocking by the Mcs_Read function.
// channel: Value for uniquely identifying the communication partner when sending and receiving data.
// Returns the readable data size.
//
u32 Mcs_GetReadableBytes(ChannelType channel)
{
    EA_ASSERT(sbMcsInitialized && spCommDevice);

    AutoMutexLock autoLock(sMcsMutex);
    return spCommDevice->GetReadableBytes(channel);
}


// Reads in data. The read data is not deleted from the receive buffer.
// If the size specified in size is bigger than the data size stored in the 
// receive buffer at that time, only the size stored in the receive buffer 
// will be stored.
//
// channel: Value for uniquely identifying the communication partner when sending and receiving data.
// data:    Pointer to the buffer that stores the data to be read.
// size:    Size of the buffer that stores the data to be read.
// 
// Returns the number of bytes read.
//
u32 Mcs_Peek(ChannelType channel, void* data, u32 size)
{
    EA_ASSERT(sbMcsInitialized && spCommDevice);

    AutoMutexLock autoLock(sMcsMutex);
    return spCommDevice->Peek(channel, data, size);
}


// Receives data. Does not return control until the entire size specified by 
// size is received. Function calls are blocked when reading an amount of 
// data larger than the data size stored in the receive buffer at this time.
//
// channel:    Value used to identify data reception. Assigned by the user.
// data:       Pointer to the buffer that stores the data to be read.
// size:       Size of the buffer that stores the data to be read.
//
// Returns 0 if the function was successful. Returns an error code (non-zero value) 
// if the function fails.
//
u32 Mcs_Read(ChannelType channel, void* data, u32 size)
{
    EA_ASSERT(sbMcsInitialized && spCommDevice);

    AutoMutexLock autoLock(sMcsMutex);
    return spCommDevice->Read(channel, data, size);
}


// Skips received data.
// Does not return control until the entire size specified by size is skipped.
// Function calls are blocked when skipping an amount of data larger than the 
// data size stored in the receive buffer at this time.
//
// channel: Value used to identify data reception. Assigned by the user.
// size:    The size of the data to skip.
//
// Returns 0 if the function was successful. Returns an error code (non-zero value) 
// if the function fails.
//
u32 Mcs_Skip(ChannelType channel, u32 size)
{
    EA_ASSERT(sbMcsInitialized && spCommDevice);

    AutoMutexLock autoLock(sMcsMutex);
    u8 buf[32];

    while(size > 0)
    {
        const u32 readSize  = detail::Min(sizeof(buf), size);
        const u32 errorCode = spCommDevice->Read(channel, buf, readSize);

        if(errorCode != 0)
            return errorCode;

        size -= readSize;
    }

    return MCS_ERROR_SUCCESS;
}


// Gets the number of bytes that can be sent at once.
//
// channel: Value for uniquely identifying the communication partner when sending and receiving data.
// pWritableBytes: Pointer to variable storing the number of transferable bytes.
//
// Returns 0 if the function was successful. Returns an error code (non-zero value) if the function fails.
//
u32 Mcs_GetWritableBytes(ChannelType channel, u32* pWritableBytes)
{
    EA_ASSERT(sbMcsInitialized && spCommDevice);

    AutoMutexLock autoLock(sMcsMutex);
    return spCommDevice->GetWritableBytes(channel, pWritableBytes);
}


// Sends data.
// Does not return control until the entire size specified by size is sent.
// Function calls are blocked when sending data of a size larger than that 
// returned by the GetWritableBytes method.
//
// channel: Value for uniquely identifying the communication partner when sending and receiving data.
// data:    Pointer to the buffer that stores the data to send.
// size:    The size of the data to send.
//
// Returns 0 if the function was successful. Returns an error code (non-zero value) 
// if the function fails.
//
u32 Mcs_Write(ChannelType channel, const void* data, u32 size)
{
    EA_ASSERT(sbMcsInitialized && spCommDevice);

    AutoMutexLock autoLock(sMcsMutex);
    return spCommDevice->Write(channel, data, size);
}


// Gets a value indicating the connection status with the mcs server.
// Returns true if connected to the server; otherwise, returns false.
//
bool Mcs_IsServerConnect()
{
    EA_ASSERT(sbMcsInitialized && spCommDevice);

    AutoMutexLock autoLock(sMcsMutex);
    return spCommDevice->IsServerConnect();
}




///////////////////////////////////////////////////////////////////
// MCS
///////////////////////////////////////////////////////////////////

namespace
{
    u32 GetReadableBytes(const SimpleDataRange& range, u32 bufSize)
    {
        if(range.start <= range.end)
            return range.end - range.start;
        else
            return range.end + bufSize - range.start;
    }


    u32 GetSequenceReadableBytes(const SimpleDataRange& range, u32 bufSize)
    {
        if(range.start <= range.end)
            return range.end - range.start;
        else
            return bufSize - range.start;
    }


    u32 GetWritableBytes(const SimpleDataRange& range, u32 bufSize)
    {
        const u32 start = (range.start + bufSize - 4) % bufSize;

        if(start >= range.end)
            return start - range.end;
        else
            return start + bufSize - range.end;
    }

    u32 GetSequenceWritableBytes(const SimpleDataRange& range, u32 bufSize)
    {
        const u32 start = (range.start + bufSize - 4) % bufSize;

        if(start >= range.end)
            return start - range.end;
        else
            return bufSize - range.end;
    }

} // namespace



namespace detail
{


void SimpleRingBuffer::Init(void* buf, u32 size)
{
    u32 bufStart = reinterpret_cast<u32>(buf);
    u32 bufEnd   = bufStart + size;

    bufStart = RoundUp(bufStart, 4);    // 4-byte alignment
    bufEnd   = RoundDown(bufEnd, 4);    // 4-byte alignment

    EA_ASSERT(bufStart < bufEnd);
    EA_ASSERT((bufEnd - bufStart) > (sizeof(SimpleRingBufferHeader) + 4)); // headersize overrun

    mpHeader = reinterpret_cast<SimpleRingBufferHeader*>(bufStart);
    mpHeader->bufSize = bufEnd - bufStart - sizeof(SimpleRingBufferHeader);

    mpHeader->dataRange.start = 0;
    mpHeader->dataRange.end   = 0;
    OSInitMutex(&mpHeader->mutex);
}


void SimpleRingBuffer::Discard()
{
    AutoMutexLock lock(mpHeader->mutex);

    mpHeader->dataRange.start = 0;
    mpHeader->dataRange.end   = 0;
}


u32 SimpleRingBuffer::GetReadableBytes()
{
    SimpleDataRange dataRange;

    {
        AutoMutexLock lock(mpHeader->mutex);

        dataRange = mpHeader->dataRange;
    }

    return ::GetReadableBytes(dataRange, mpHeader->bufSize);
}


u32 SimpleRingBuffer::GetWritableBytes()
{
    SimpleDataRange dataRange;

    {
        AutoMutexLock lock(mpHeader->mutex);

        dataRange = mpHeader->dataRange;
    }

    return ::GetWritableBytes(dataRange, mpHeader->bufSize);
}

u32 SimpleRingBuffer::Read(void* buf, u32 size)
{
    SimpleDataRange dataRange;

    BeginRead(&dataRange);

    const u32 ret = ContinueRead(&dataRange, buf, size);

    EndRead(dataRange);

    return ret;
}


void SimpleRingBuffer::BeginRead(SimpleDataRange* pDataRange)
{
    // Get data start and end positions
    AutoMutexLock lock(mpHeader->mutex);

    *pDataRange = mpHeader->dataRange;
}


u32 SimpleRingBuffer::ContinueRead(SimpleDataRange* pDataRange, void* buf, u32 size)
{
    const u32 readableBytes = ::GetReadableBytes(*pDataRange, mpHeader->bufSize);

    if(readableBytes == 0)
        return 0;

    u8*             pDst = static_cast<u8*>(buf);
    const u8* const pSrc = reinterpret_cast<u8*>(mpHeader) + sizeof(SimpleRingBufferHeader);

    u32 restSize = Min(readableBytes, size);
    u32 ret      = restSize;

    while(restSize > 0)
    {
        const u32 readByte = Min(GetSequenceReadableBytes(*pDataRange, mpHeader->bufSize), restSize);

        std::memcpy(pDst, pSrc + pDataRange->start, readByte);
        pDataRange->start = (pDataRange->start + readByte) % mpHeader->bufSize;
        pDst     += readByte;
        restSize -= readByte;
    }

    return ret;
}


void SimpleRingBuffer::EndRead(const SimpleDataRange& dataRange)
{
    // Update read start position
    AutoMutexLock lock(mpHeader->mutex);

    mpHeader->dataRange.start = dataRange.start;
}


u32 SimpleRingBuffer::Write(const void* buf, u32 size)
{
    SimpleDataRange dataRange;

    BeginWrite(&dataRange);

    const u32 ret = ContinueWrite(&dataRange, buf, size);

    EndWrite(dataRange);

    return ret;
}


void SimpleRingBuffer::BeginWrite(SimpleDataRange* pDataRange)
{
    // Get data start and end positions
    AutoMutexLock lock(mpHeader->mutex);

    *pDataRange = mpHeader->dataRange;
}


// Make sure that end point is not buf + bufSize. In this case, indicate buf.
// When writing, end should have a different value than start. There should 
// always be a space of 4 bytes between start and end.
//
u32 SimpleRingBuffer::ContinueWrite(SimpleDataRange* pDataRange, const void* buf, u32 size)
{
    const u32 writableBytes = ::GetWritableBytes(*pDataRange, mpHeader->bufSize);

    if(writableBytes == 0)
        return 0;

    u8* const pDst = reinterpret_cast<u8*>(mpHeader) + sizeof(SimpleRingBufferHeader);
    const u8* pSrc = static_cast<const u8*>(buf);

    u32 restSize = Min(writableBytes, size);
    u32 ret      = restSize;

    while(restSize > 0)
    {
        const u32 writeByte = Min(GetSequenceWritableBytes(*pDataRange, mpHeader->bufSize), restSize);

        std::memcpy(pDst + pDataRange->end, pSrc, writeByte);
        pDataRange->end = (pDataRange->end + writeByte) % mpHeader->bufSize;
        pSrc     += writeByte;
        restSize -= writeByte;
    }

    return ret;
}

void SimpleRingBuffer::EndWrite(const SimpleDataRange& dataRange)
{
    // Update write start position
    AutoMutexLock lock(mpHeader->mutex);

    mpHeader->dataRange.end = dataRange.end;
}

}   // namespace detail





///////////////////////////////////////////////////////////////////
// Hio2RingBuffer
///////////////////////////////////////////////////////////////////

#define MCS_HIO2READ            HIO2Read
#define MCS_HIO2WRITE           HIO2Write
#define MCS_HIO2GetLastError    HIO2GetLastError


namespace
{
    const u32 CheckCode = ('M' << 24) + ('C' << 16) + ('H' << 8) + 'I'; // Check code written to the start of the buffer (also used for the startup check)

    struct DataPointer
    {
        u32 pointer;
        u8  reserved[24];
        u32 pointerCopy;
    };

    struct BufferHeader
    {
        u32         signature;
        u8          reserved[28];
        DataPointer read;
        DataPointer write;
    };

    struct MessageHeader
    {
        u32 channel;
        u32 size;
        u32 reserve1;
        u32 reserve2;
    };

    const u16 ReadInfoOffset  = 0x20;                   // Offset to the location of the read point
    const u16 WriteInfoOffset = 0x40;                   // Offset to the location of the write point
    const u16 HeaderSize      = sizeof(BufferHeader);   // Header : Size of all headers


    #if defined(ENABLE_MCS_REPORT)
        #define MCS_REPORT0(str)                    OSReport(str)
        #define MCS_REPORT1(fmt, arg1)              OSReport(fmt, arg1)
        #define MCS_REPORT2(fmt, arg1, arg2)        OSReport(fmt, arg1, arg2)
        #define MCS_REPORT3(fmt, arg1, arg2, arg3)  OSReport(fmt, arg1, arg2, arg3)
    #else
        #define MCS_REPORT0(str)                    ((void) 0)
        #define MCS_REPORT1(fmt, arg1)              ((void) 0)
        #define MCS_REPORT2(fmt, arg1, arg2)        ((void) 0)
        #define MCS_REPORT3(fmt, arg1, arg2, arg3)  ((void) 0)
    #endif

    u32 HostToNet(u32 val)
    {
        #if defined(EA_SYSTEM_LITTLE_ENDIAN)
            return htonl(val);
        #else
            return val;
        #endif
    }

    u32 NetToHost(u32 val)
    {
        #if defined(EA_SYSTEM_LITTLE_ENDIAN)
            return ntohl(val);
        #else
            return val;
        #endif
    }

    void DataPointer_Init(DataPointer* pPt, u32 pointer)
    {
        pPt->pointer = HostToNet(pointer);
        std::memset(pPt->reserved, 0, sizeof(pPt->reserved));
        pPt->pointerCopy = pPt->pointer;
    }


    // Made to go through with const void*
    void DCFlushRange(const void* addr, u32 nBytes)
    {
        ::DCFlushRange(const_cast<void*>(addr), nBytes);
    }

}   // namespace



namespace detail
{


///////////////////////////////////////////////////////////////////////////////
// Hio2RingBuffer
///////////////////////////////////////////////////////////////////////////////


// Initializes Hio2RingBuffer objects.
//
// baseOffset: Shared memory start offset.
// size:       Shared memory size.
// tempBuf:    Work memory used by the Hio2RingBuffer object
//
void Hio2RingBuffer::Init(HIO2Handle devHandle, u32 baseOffset, u32 size, void* tempBuf)
{
    EA_ASSERT(RoundDown(baseOffset, CopyAlignment) == baseOffset);
    EA_ASSERT(RoundDown(size,       CopyAlignment) == size);
    EA_ASSERT(RoundDown(tempBuf,    CopyAlignment) == tempBuf);

    mDevHandle  = devHandle;
    mBaseOffset = baseOffset;
    mBufferSize = size - HeaderSize;
    mTempBuf    = tempBuf;
    mReadPoint  = mWritePoint = 0;
    mEnablePort = false;
}


void Hio2RingBuffer::InitBuffer()
{
    BufferHeader* const bufHeader = static_cast<BufferHeader*>(mTempBuf);

    bufHeader->signature = HostToNet(CheckCode);
    std::memset(bufHeader->reserved, 0, sizeof(bufHeader->reserved));

    DataPointer_Init(&bufHeader->read, 0);
    DataPointer_Init(&bufHeader->write, 0);

    if(WriteSection_(mBaseOffset + 0, bufHeader, sizeof(*bufHeader)))
    {
        mReadPoint = mWritePoint = 0;

        MCS_REPORT2("DEBUG: Write to Offset:%05x , Size: %d\n" , 0, 0x60);
    }
}


// Reads from shared memory.
// Data cannot be read from memory while straddling a MemoryBlockSize boundary.
//
// offset:  Shared memory read start address.
// data:    Pointer to the buffer that stores the data to be read.
// size:    Load data size.
//
// Returns true if the function succeeds; otherwise, returns false.
//
bool Hio2RingBuffer::ReadSection_(u32 offset, void* data, u32 size)
{
    EA_ASSERT(RoundDown(offset, 4) == offset);  // Ensure 4-byte alignment

    #if !defined(WIN32)
        EA_ASSERT(RoundDown(data, 32) == data);     // Ensure 32-byte alignment
    #endif

    EA_ASSERT(RoundUp(size, 32) == size);       // Ensure 32-byte alignment

    MCS_REPORT3("DEBUG:      Hio2RingBuffer::ReadSection_(%06x, %08x, %d);\n", offset, data, size );

    if(IsPort())
    {
        #if !defined(WIN32)
            DCInvalidateRange(data, size);
        #endif

        for(int retry_count = 0; retry_count < HIO2FuncRetryCountMax; ++retry_count)
        {
            if(MCS_HIO2READ(mDevHandle, offset, data, s32(size)))
                return true;

            if(MCS_HIO2GetLastError() != HIO2_ERROR_INTERNAL)
                break;

            // In the case of HIO2_ERROR_INTERNAL, retry several times.
            OSSleepMilliseconds(HIO2FuncRetrySleepTime);
        }

        MCS_REPORT1("HIO2Read() error. - %d\n", MCS_HIO2GetLastError());
        DisablePort();
    }

    return false;
}


// Writes to shared memory.
// Data cannot be written to memory while straddling a MemoryBlockSize boundary.
//
// offset: Shared memory write start address.
// data:   Pointer to the buffer that stores the data to be written.
// size:   Write data size.
//
//Returns true if the function succeeds; otherwise, returns false.
//
bool Hio2RingBuffer::WriteSection_(u32 offset, const void* data, u32 size)
{
    EA_ASSERT(RoundDown(offset, 4) == offset);  // Ensure 4-byte alignment

    #if !defined(WIN32)
        EA_ASSERT(RoundDown(data, CopyAlignment) == data);  // Ensure 32-byte alignment
    #endif

    EA_ASSERT(RoundUp(size, CopyAlignment) == size);        // Ensure 32-byte alignment

    MCS_REPORT3("DEBUG:      Hio2RingBuffer::WriteSection_(0x%06x, %08x, %d);\n", offset, data, size );

    if(IsPort())
    {
        #if !defined(WIN32)
            DCFlushRange(data, size);
        #endif

        for(int retryCount = 0; retryCount < HIO2FuncRetryCountMax; ++retryCount)
        {
            if(MCS_HIO2WRITE(mDevHandle, offset, data, s32(size)))
                return true;

            if(MCS_HIO2GetLastError() != HIO2_ERROR_INTERNAL)
                break;

            // In the case of HIO2_ERROR_INTERNAL, retry several times.
            OSSleepMilliseconds(HIO2FuncRetrySleepTime);
        }

        MCS_REPORT1("HIO2Write() error. - %d\n", MCS_HIO2GetLastError());
        DisablePort();
    }

    return false;
}


// Updates the read/write position to the latest status.
// Returns true if the function succeeds; otherwise, returns false.
//
bool Hio2RingBuffer::UpdateReadWritePoint_()
{
    if(UpdateReadPoint_())
        return UpdateWritePoint_();

    return false;
}


// Updates the read point to the latest status.
// Returns true if the function succeeds; otherwise, returns false.
//
bool Hio2RingBuffer::UpdateReadPoint_()
{
    if(IsPort())
    {
        DataPointer* const dataPtr = static_cast<DataPointer*>(mTempBuf);

        const int MAX_RETRY = 30;

        int retryCount = 0;
        for(; retryCount < MAX_RETRY; ++retryCount)
        {
            if(!ReadSection_(mBaseOffset + ReadInfoOffset, dataPtr, sizeof(*dataPtr)))
                return false;

            if(dataPtr->pointer == dataPtr->pointerCopy)
                break;
        }

        if(retryCount >= MAX_RETRY)
        {
            MCS_REPORT0("ReadPoint read fail.\n");
            return false;
        }

        mReadPoint = NetToHost(dataPtr->pointer);

        if(RoundDown(mReadPoint, 32) != mReadPoint)
            MCS_REPORT1("mReadPoint %08X\n", mReadPoint);

        return true;
    }

    return false;
}


// Updates the write point to the latest status.
// Returns true if the function succeeds; otherwise, returns false.
//
bool Hio2RingBuffer::UpdateWritePoint_()
{
    if(IsPort())
    {
        DataPointer* const dataPtr = static_cast<DataPointer*>(mTempBuf);

        const int MAX_RETRY = 30;

        int retryCount = 0;
        for(; retryCount < MAX_RETRY; ++retryCount)
        {
            if(!ReadSection_(mBaseOffset + WriteInfoOffset, dataPtr, sizeof(*dataPtr)))
                return false;

            if(dataPtr->pointer == dataPtr->pointerCopy)
                break;
        }

        if(retryCount >= MAX_RETRY)
        {
            MCS_REPORT0("WritePoint read fail.\n");
            return false;
        }

        mWritePoint = NetToHost(dataPtr->pointer);

        if(RoundDown(mWritePoint, 32) != mWritePoint)
            MCS_REPORT1("mWritePoint %08X\n", mWritePoint);

        return true;
    }

    return false;
}


// Loads data from shared memory.
// Returns true if the function succeeds; otherwise, returns false.
//
bool Hio2RingBuffer::Read(bool* pbAvailable)
{
    EA_ASSERT(pbAvailable != NULL);

    if(UpdateWritePoint_())
    {
        *pbAvailable = mReadPoint != mWritePoint;

        if(*pbAvailable)
        {
            mProcBytes = 0;
            mTransBytes = (mWritePoint + mBufferSize - mReadPoint) % mBufferSize;

            u32       currReadPoint = mReadPoint;
            u8* const msgBuf        = static_cast<u8*>(mTempBuf) + HeaderSize;
            u32       tmpOfs        = 0;

            for(u32 restBytes = mTransBytes; restBytes > 0; )
            {
                const u32 readBytes = mWritePoint < currReadPoint ? mBufferSize - currReadPoint: restBytes;

                if(ReadSection_(mBaseOffset + HeaderSize + currReadPoint, msgBuf + tmpOfs, readBytes))
                {
                    currReadPoint = (currReadPoint + readBytes) % mBufferSize;
                    tmpOfs    += readBytes;
                    restBytes -= readBytes;
                }
                else
                    return false;
            }

            EA_ASSERT(currReadPoint == mWritePoint);

            DataPointer* const dataPtr = static_cast<DataPointer*>(mTempBuf);
            DataPointer_Init(dataPtr, currReadPoint);

            if(WriteSection_(mBaseOffset + ReadInfoOffset, dataPtr, sizeof(*dataPtr)))
            {
                mReadPoint = currReadPoint;
                MCS_REPORT1("DEBUG: Read: Point:%08x\n" , mReadPoint);
            }
            else
                return false;
        }

        return true;
    }

    return false;
}


// Gets message data.
// Moves to the next message position each time a message is obtained.
//
// pChannel:   Pointer to the variable storing the channel value of the message.
// pTotalSize: Pointer to the variable storing the total size of the message.
//
// Returns NULL if there is no message to get;
// Otherwise returns a pointer to the message data.
//
void* Hio2RingBuffer::GetMessage(u32* pChannel, u32* pTotalSize)
{
    if(mProcBytes < mTransBytes)
    {
        u8* const msgStart = static_cast<u8*>(mTempBuf) + HeaderSize + mProcBytes;
        const MessageHeader *const pMsgHeader = reinterpret_cast<MessageHeader*>(msgStart);

        *pChannel   = NetToHost(pMsgHeader->channel);
        *pTotalSize = NetToHost(pMsgHeader->size);

        mProcBytes += RoundUp(sizeof(*pMsgHeader) + *pTotalSize, CopyAlignment);
        return msgStart + sizeof(*pMsgHeader);
    }

    return NULL;
}


// Gets the number of bytes that can be written.
//
// pBytes:     Pointer to the variable used to get the number of bytes that can be written.
// withUpdate: Updates if TRUE.
//
// Returns true if the function succeeds; otherwise, returns false.
//
bool Hio2RingBuffer::GetWritableBytes(u32* pBytes, bool withUpdate)
{
    if(withUpdate)
    {
        if(!UpdateReadPoint_())
            return false;
    }

    u32 writeEndPoint = (mReadPoint + mBufferSize - CopyAlignment) % mBufferSize;
    u32 writableBytes = (writeEndPoint + mBufferSize - mWritePoint) % mBufferSize;

    *pBytes = writableBytes >= sizeof(MessageHeader) ? writableBytes - sizeof(MessageHeader) : 0;
    return true;
}


// Writes data to shared memory.
//
// channel: Channel value.
// buf:     Data to be written.
// size:    Total size of data to be written.
//
// Returns true if the function succeeds; otherwise, returns false.
//
bool Hio2RingBuffer::Write(u32 channel, const void* buf, u32 size)
{
    if(!IsPort())
        return false;

    {
        u32 writableBytes;
        // Gets the writable bytes without updates
        if(!GetWritableBytes(&writableBytes, false))
            return false;

        if(size > writableBytes)
        {
            // Gets the writable bytes with updates
            if(!GetWritableBytes(&writableBytes, true))
                return false;

            if(size > writableBytes)
            {
                // Deny writing of data if the amount to be written exceeds the amount that can be written.
                // The amount of data to be written must be adjusted by the code that calls this function.
                return false;
            }
        }
    }

    u8* const tempBuf           = static_cast<u8*>(mTempBuf);
    bool      bOutMessageHeader = false;
    const u8* pSrc              = static_cast<const u8*>(buf);
    const u32 writeEndPoint     = (mReadPoint + mBufferSize - CopyAlignment) % mBufferSize;

    u32 currWritePoint = mWritePoint;

    for(u32 restBytes = size; restBytes > 0; )
    {
        u32 tmpOfs = 0;

        // Size not including message header
        u32 writableBytes = writeEndPoint >= currWritePoint ? writeEndPoint - currWritePoint: mBufferSize - currWritePoint;

        if(!bOutMessageHeader)
        {
            MessageHeader* const pHeader = reinterpret_cast<MessageHeader*>(tempBuf + tmpOfs);

            pHeader->channel  = HostToNet(channel);
            pHeader->size     = HostToNet(size);
            pHeader->reserve1 = 0;
            pHeader->reserve2 = 0;

            tmpOfs        += sizeof(MessageHeader);
            writableBytes -= sizeof(MessageHeader);

            bOutMessageHeader = true;
        }

        const u32 msgWriteBytes = Min(restBytes, writableBytes);

        std::memcpy(tempBuf + tmpOfs, pSrc, msgWriteBytes); // Transfer to temporary buffer
        tmpOfs    += msgWriteBytes;
        pSrc      += msgWriteBytes;
        restBytes -= msgWriteBytes;

        if(restBytes == 0)
        {
            const u32 oldTempOfs = tmpOfs;
            tmpOfs = RoundUp(tmpOfs, CopyAlignment);

            if(tmpOfs > oldTempOfs)
                std::memset(tempBuf + oldTempOfs, 0, tmpOfs - oldTempOfs);
        }

        if(!WriteSection_(mBaseOffset + HeaderSize + currWritePoint, tempBuf, tmpOfs))
            return false;

        currWritePoint = (currWritePoint + tmpOfs) % mBufferSize;
    }

    DataPointer* const dataPtr = static_cast<DataPointer*>(mTempBuf);
    DataPointer_Init(dataPtr, currWritePoint);

    MCS_REPORT1("DEBUG: EndWrite: Point:%08x\n" , currWritePoint);
    if(WriteSection_(mBaseOffset + WriteInfoOffset, dataPtr, sizeof(*dataPtr)))
    {
        mWritePoint = currWritePoint;
        return true;
    }

    return false;
}


} // namespace detail




///////////////////////////////////////////////////////////////////////////////
// Hio2RingBuffer
///////////////////////////////////////////////////////////////////////////////

namespace
{

    enum
    {
        NEGOPHASE_NOATTACH,
        NEGOPHASE_ATTACHED,
        NEGOPHASE_REBOOT,
        NEGOPHASE_CONNECT,
        NEGOPHASE_ERROR         // An error occurred after the open function succeeded
    };

    IHIO2CommDeviceEnumerate* spEnumerate = NULL;

    BOOL EnumCallback(HIO2DeviceType type)
    {
        return spEnumerate->Find(type);
    }

    BOOL HIO_EnumDevices(HIO2EnumCallback callback)
    {
        for(int retryCount = 0; retryCount < HIO2FuncRetryCountMax; ++retryCount)
        {
            if(HIO2EnumDevices(callback))
                return true;

            if(HIO2GetLastError() != HIO2_ERROR_INTERNAL)
                break;

            // In the case of HIO2_ERROR_INTERNAL, retry several times.
            OSSleepMilliseconds(HIO2FuncRetrySleepTime);
        }

        EA_ASSERT_FORMATTED(false, ("HIO2EnumDevices() error. - %dX\n", HIO2GetLastError()));
        return false;
    }

    HIO2Handle HIO_Open(HIO2DeviceType type, HIO2ReceiveCallback callback, HIO2DisconnectCallback disconnect)
    {
        for(int retryCount = 0; retryCount < HIO2FuncRetryCountMax; ++retryCount)
        {
            const HIO2Handle handle = HIO2Open(type, callback, disconnect);

            if(HIO2_INVALID_HANDLE_VALUE != handle)
                return handle;

            if(HIO2GetLastError() != HIO2_ERROR_INTERNAL)
                break;

            // In the case of HIO2_ERROR_INTERNAL, retry several times.
            OSSleepMilliseconds(HIO2FuncRetrySleepTime);
        }

        EA_ASSERT_FORMATTED(false, ("HIO2Open() error. device type %d, error code %d.\n", type, HIO2GetLastError()));
        return HIO2_INVALID_HANDLE_VALUE;
    }

    BOOL HIO_Close(HIO2Handle  handle)
    {
        for(int retryCount = 0; retryCount < HIO2FuncRetryCountMax; ++retryCount)
        {
            if(HIO2Close(handle))
                return true;

            if(HIO2GetLastError() != HIO2_ERROR_INTERNAL)
                break;

            // In the case of HIO2_ERROR_INTERNAL, retry several times.
            OSSleepMilliseconds(HIO2FuncRetrySleepTime);
        }

        EA_ASSERT_FORMATTED(false, ("HIO2Close() error. - %dX\n", HIO2GetLastError()));
        return false;
    }

    BOOL HIO_ReadMailbox(HIO2Handle handle, u32* pMail)
    {
        for(int retryCount = 0; retryCount < HIO2FuncRetryCountMax; ++retryCount)
        {
            if(HIO2ReadMailbox(handle, pMail))
                return true;

            if(HIO2GetLastError() != HIO2_ERROR_INTERNAL)
                break;

            // In the case of HIO2_ERROR_INTERNAL, retry several times.
            OSSleepMilliseconds(HIO2FuncRetrySleepTime);
        }

        EA_ASSERT_FORMATTED(false, ("HIO2ReadMailbox() error. - %dX\n", HIO2GetLastError()));
        return false;
    }

    BOOL HIO_WriteMailbox(HIO2Handle handle, u32 mail)
    {
        for(int retryCount = 0; retryCount < HIO2FuncRetryCountMax; ++retryCount)
        {
            if(HIO2WriteMailbox(handle, mail))
                return true;

            if(HIO2GetLastError() != HIO2_ERROR_INTERNAL)
                break;

            // In the case of HIO2_ERROR_INTERNAL, retry several times.
            OSSleepMilliseconds(HIO2FuncRetrySleepTime);
        }

        EA_ASSERT_FORMATTED(false, ("HIO2WriteMailbox() error. - %dX\n", HIO2GetLastError()));
        return false;
    }

    BOOL HIO_ReadStatus(HIO2Handle handle, u32* pStatus)
    {
        for(int retryCount = 0; retryCount < HIO2FuncRetryCountMax; ++retryCount)
        {
            if(HIO2ReadStatus(handle, pStatus))
                return true;

            if(HIO2GetLastError() != HIO2_ERROR_INTERNAL)
                break;

            // In the case of HIO2_ERROR_INTERNAL, retry several times.
            OSSleepMilliseconds(HIO2FuncRetrySleepTime);
        }

        EA_ASSERT_FORMATTED(false, ("HIO2ReadStatus() error. - %dX\n", HIO2GetLastError()));
        return false;
    }

}   // namespace



namespace detail
{


// Enumerates the communication devices.
// When a communication device is found, the pEnumerate Find method is called.
//
// pEnumerate:  Object that implements the Find method called when a communication device is found.
//
// Return TRUE if successfully enumerated.  Return FALSE if enumeration fails.
//
bool HIO2CommDevice::EnumerateDevice(IHIO2CommDeviceEnumerate* pEnumerate)
{
    EA_ASSERT(spEnumerate == 0);

    spEnumerate    = pEnumerate;
    const BOOL ret = HIO_EnumDevices(EnumCallback);
    spEnumerate    = 0;

    return ret == TRUE;
}


// This is a constructor for HIO2CommDevice.
//
// exiChanNo:    EXI channel number.
// exiDevNo:     EXI device number.
// tempBuf:      Pointer to the work memory used by HIO2CommDevice.
// tempBufSize:  Size of the memory buffer pointed to by tempBuf.
//
HIO2CommDevice::HIO2CommDevice(int devType, void* tempBuf, u32 tempBufSize)
:   mDevType(devType),
    mHioHandle(HIO2_INVALID_HANDLE_VALUE),
    mRBReader(),
    mRBWriter(),
  //mTempBuf(NULL),
  //mTempBufSize(0),
    mbReadInitCodeMail(false),
    mNegoPhase(NEGOPHASE_NOATTACH),
    mMutex(),
    mbOpen(false),
    mChannelInfoList()
{
    EA_ASSERT(tempBufSize >= Hio2RingBuffer::CopyAlignment + TEMP_BUF_SIZE_MIN);

    mTempBuf     = RoundUp(tempBuf, Hio2RingBuffer::CopyAlignment);
    mTempBufSize = RoundDown(reinterpret_cast<u32>(tempBuf) + tempBufSize, Hio2RingBuffer::CopyAlignment) - reinterpret_cast<u32>(mTempBuf);

    OSInitMutex(&mMutex);
}


HIO2CommDevice::~HIO2CommDevice()
{
    // Empty
}


// Registers the buffer for data reception.
//
// When the receive buffer is full and there is no free space to
// store newly received data, the received data filling the buffer is discarded.
// Be sure the buffer size is large enough to hold the
// amount of data used in communication.
// 
// channel:   Value for uniquely identifying the communication partner when sending and receiving data.
// buf:       The reception buffer to register
// bufSize :  The size of the reception buffer to register
//
void HIO2CommDevice::RegisterBuffer(ChannelType channel, void* buf, u32 bufSize)
{
    AutoMutexLock lockObj(mMutex);

    EA_ASSERT(NULL == GetChannelInfo(channel));   // Channel should be unused

    u32 start = reinterpret_cast<u32>(buf);
    u32 end   = start + bufSize;

    start = RoundUp(start, 4);
    end   = RoundDown(end, 4);

    EA_ASSERT(start < end);
    EA_ASSERT(end - start >= sizeof(ChannelInfo) + sizeof(SimpleRingBufferHeader) + sizeof(u32) + 4 + 4);

    ChannelInfo* const pInfo = reinterpret_cast<ChannelInfo*>(start);

    pInfo->channel = channel;
    const u32 ringBufStart = start + sizeof(ChannelInfo);
    pInfo->recvBuf.Init(reinterpret_cast<void*>(ringBufStart), end - ringBufStart);

    mChannelInfoList.push_back(*pInfo);
}


// The receive buffer registered by the RegisterBuffer method is Mcs_RegisterStreamRecvBuffer().
//
// channel: Value for uniquely identifying the communication partner when sending and receiving data.
//
void HIO2CommDevice::UnregisterBuffer(ChannelType channel)
{
    AutoMutexLock lockObj(mMutex);

    ChannelInfo* const pInfo = GetChannelInfo(channel);
    EA_ASSERT(pInfo);

    mChannelInfoList.remove(*pInfo);
}


// Open communication devices.
//
// Returns 0 if the function was successful.
// Returns an error code (non-zero value) if the function fails.
//
u32 HIO2CommDevice::Open()
{
    AutoMutexLock lockObj(mMutex);

    if(mbOpen)
        return MCS_ERROR_SUCCESS;

    if(mNegoPhase == NEGOPHASE_NOATTACH)
    {
        mHioHandle = HIO_Open(HIO2DeviceType(mDevType), NULL, NULL);
        if(mHioHandle == HIO2_INVALID_HANDLE_VALUE)
            return MCS_ERROR_COMERROR;

        mNegoPhase = NEGOPHASE_ATTACHED;
    }

    // Reads out mail that may remain from the previous communication.

    u32 hioStatus;
    if(!HIO_ReadStatus(mHioHandle, &hioStatus))
        goto HioError;

    if(hioStatus & HIO2_STATUS_TX)
    {
        u32 mailWord;
        if(!HIO_ReadMailbox(mHioHandle, &mailWord))
            goto HioError;

        // NW4R_DB_LOG("clear mail box. drop mail %08X\n", mailWord);
    }

    // Notify regarding startup
    if(!HIO_WriteMailbox(mHioHandle, HioNegotiate::RebootCode))
        goto HioError;

    // NW4R_DB_LOG("Write reboot mail.\n");

    mNegoPhase = NEGOPHASE_REBOOT;

    mbOpen = true;              // Open function succeeded
    return MCS_ERROR_SUCCESS;

    // Error generated by HIO function
    HioError:
    mNegoPhase = NEGOPHASE_NOATTACH;

    if(mHioHandle != HIO2_INVALID_HANDLE_VALUE)
    {
        HIO_Close(mHioHandle);
        mHioHandle = HIO2_INVALID_HANDLE_VALUE;
    }

    return MCS_ERROR_COMERROR;
}


void HIO2CommDevice::Close()
{
    AutoMutexLock lockObj(mMutex);

    if(!mbOpen)
        return;

    mRBReader.DisablePort();
    mRBWriter.DisablePort();

    HIO_Close(mHioHandle);
    mHioHandle = HIO2_INVALID_HANDLE_VALUE;

    mbOpen = false;

    #if HIOINIT_CALL_ONETIME
        mNegoPhase = NEGOPHASE_ATTACHED;
    #else
        mNegoPhase = NEGOPHASE_NOATTACH;
    #endif

    return;
}


// Do idle processing. 
// You should call this function inside the main loop.
// 
// Returns 0 if the function was successful.
// Returns an error code (non-zero value) if the function fails.
//
u32 HIO2CommDevice::Polling()
{
    AutoMutexLock lockObj(mMutex);

    if(!mbOpen)   // Does nothing if not open
        return MCS_ERROR_SUCCESS;

    if(IsError())
        return MCS_ERROR_COMERROR;

    const u32 errorCode = Negotiate();
    if(errorCode)
        return errorCode;

    switch (mNegoPhase)
    {
        case NEGOPHASE_CONNECT:
        {
            bool bDataAvailable = false;

            if(!mRBReader.Read(&bDataAvailable))
                goto HioError;

            if(!bDataAvailable)
                break;

            u32   channel;
            u32   messageSize;
            void* msgPtr = NULL;

            while((msgPtr = mRBReader.GetMessage(&channel, &messageSize)) != NULL)    // while data is present...
            {
                ChannelInfo* const pInfo = GetChannelInfo(static_cast<ChannelType>(channel));
                bool bWritable = true;

                if(!pInfo)
                {
                    EA_ASSERT_FORMATTED(false, ("NL mcs warning : channel %d not found.\n", channel));
                    bWritable = false;
                }
                else if(pInfo->recvBuf.GetWritableBytes() < messageSize)
                {
                    // ERROR -  the buffer has been exhausted
                    EA_ASSERT_FORMATTED(false, ("NL mcs error : buffer is not enough. writable %d, data size %d\n", pInfo->recvBuf.GetWritableBytes(), messageSize));
                    bWritable = false;
                }

                if(bWritable)
                {
                    SimpleDataRange dataRange;

                    pInfo->recvBuf.BeginWrite(&dataRange);
                    u32 writtenBytes = pInfo->recvBuf.ContinueWrite(&dataRange, msgPtr, messageSize);
                    EA_ASSERT(writtenBytes == messageSize);
                    pInfo->recvBuf.EndWrite(dataRange);
                }
            }
        }

        break;
    }

    return MCS_ERROR_SUCCESS;

    // Error generated by HIO function
    HioError:
    mNegoPhase = NEGOPHASE_ERROR;

    return MCS_ERROR_COMERROR;
}


// Gets the size of data that can be read without blocking by the Read method.
// 
// channel: Value for uniquely identifying the communication partner when sending and receiving data.
// 
// Returns the readable data size.
//
u32 HIO2CommDevice::GetReadableBytes(ChannelType channel)
{
    AutoMutexLock lockObj(mMutex);

    EA_ASSERT(mbOpen);

    ChannelInfo* const pInfo = GetChannelInfo(channel);
    EA_ASSERT(pInfo);

    return pInfo->recvBuf.GetReadableBytes();
}


// Reads in data. The read data is not deleted from the receive buffer.
// 
// If the size specified by size is larger than the data size being stored in the 
// receive buffer at this time, only data up to the amount specified by size is read.
// Only the data amount stored in the receive buffer is read.
// 
// channel: Value for uniquely identifying the communication partner when sending and receiving data.
// data:    Pointer to the buffer that stores the data to be read.
// size:    Size of the buffer that stores the data to be read.
// 
// Returns the number of bytes read.
//
u32 HIO2CommDevice::Peek(ChannelType channel, void* data, u32 size)
{
    AutoMutexLock lockObj(mMutex);

    EA_ASSERT(mbOpen);

    ChannelInfo* const pInfo = GetChannelInfo(channel);
    EA_ASSERT(pInfo);

    SimpleDataRange dataRange;

    pInfo->recvBuf.BeginRead(&dataRange);

    return pInfo->recvBuf.ContinueRead(&dataRange, data, size);
}


// Receives data.
// Does not return control until the entire size specified by size is received.
// Function calls are blocked when skipping an amount of data larger than the 
// data size stored in the receive buffer at this time.
// 
// channel: Value used to indentify data reception. Assigned by the user.
// data:    Pointer to the buffer that stores the data to be read.
// size:    Size of the buffer that stores the data to be read.
// 
// Returns 0 if the function was successful.
// Returns an error code (non-zero value) if the function fails.
//
u32 HIO2CommDevice::Read(ChannelType channel, void* data, u32 size)
{
    AutoMutexLock lockObj(mMutex);

    EA_ASSERT(mbOpen);

    ChannelInfo* const pInfo = GetChannelInfo(channel);
    EA_ASSERT(pInfo);

    u8* const pDst     = static_cast<u8*>(data);
    int       retryCnt = 0;
    u32       offset   = 0;

    while(offset < size)
    {
        const u32 readableBytes = pInfo->recvBuf.GetReadableBytes();

        if(readableBytes == 0)
        {
            if(IsError())
                return MCS_ERROR_COMERROR;

            if(!IsServerConnect())
                return MCS_ERROR_NOTCONNECT;

            if(u32 errorCode = WaitRecvData()) // Wait until data can be read
                return errorCode;
        }
        else
        {
            u32 readByte = Min(size - offset, pInfo->recvBuf.GetReadableBytes());

            readByte = pInfo->recvBuf.Read(pDst + offset, readByte);
            offset  += readByte;
            retryCnt = 0;
        }
    }

    return MCS_ERROR_SUCCESS;
}


// Gets the number of bytes that can be sent at once.
// 
// channel:        Not used at the current time.
// pWritableBytes: Pointer to variable storing the number of transferable bytes.
// 
// Returns 0 if the function was successful.
// Returns an error code (non-zero value) if the function fails.
//
u32 HIO2CommDevice::GetWritableBytes(ChannelType /*channel*/, u32* pWritableBytes)
{
    AutoMutexLock lockObj(mMutex);

    EA_ASSERT(mbOpen);

    if(IsError())
        return MCS_ERROR_COMERROR;

    if(!IsServerConnect())
        return MCS_ERROR_NOTCONNECT;

    u32 writableBytes;

    if(mRBWriter.GetWritableBytes(&writableBytes))
    {
        *pWritableBytes = writableBytes;
        return MCS_ERROR_SUCCESS;
    }

    mNegoPhase = NEGOPHASE_ERROR;
    return MCS_ERROR_COMERROR;
}


// Sends data.
// Does not return control until the entire size specified by size is sent.
// Function calls are blocked when sending data of a size larger than that 
// returned by the GetWritableBytes method.
// 
// channel: Value for uniquely identifying the communication partner when sending and receiving data.
// data:    Pointer to the buffer that stores the data to send.
// size:    The size of the data to send.
// 
// Returns 0 if the function was successful.
// Returns an error code (non-zero value) if the function fails.
//
u32 HIO2CommDevice::Write(ChannelType channel, const void* data, u32 size)
{
    AutoMutexLock lockObj(mMutex);

    EA_ASSERT(mbOpen);

    if(IsError())
        return MCS_ERROR_COMERROR;

    if(!IsServerConnect())
        return MCS_ERROR_NOTCONNECT;

    const u8* const pSrc     = static_cast<const u8*>(data);
    int             retryCnt = 0;
    u32             offset   = 0;

    while(offset < size)
    {
        const u32 restSize = size - offset;
        u32 writeBytes = 0;

        if(u32 errorCode = GetWritableBytes(channel, &writeBytes)) // Obtains the amount of data that can be written at once
            return errorCode;

        const u32 SendDataSizeMin = (mRBWriter.GetBufferSize() / 2);

        if(restSize > writeBytes && writeBytes < SendDataSizeMin)
        {
            const u32 errorCode = WaitSendData();

            if(errorCode) // Wait until it can be written
                return errorCode;
        }
        else
        {
            writeBytes = Min(restSize, writeBytes);

            if(!mRBWriter.Write(channel, pSrc + offset, writeBytes))
            {
                EA_ASSERT_MSG(false, "NL Mcs error: send error\n");
                goto HioError;
            }

            offset += writeBytes;
            retryCnt = 0;
        }
    }

    return MCS_ERROR_SUCCESS;   // Returns success if data can be written to the end.

    // Error generated by HIO function
    HioError:
    mNegoPhase = NEGOPHASE_ERROR;
    return MCS_ERROR_COMERROR;
}


// Gets a value indicating the connection status with the mcs server.
// 
// Returns true if connected to the server; otherwise, returns false.
//
bool HIO2CommDevice::IsServerConnect()
{
    AutoMutexLock lockObj(mMutex);

    return (mNegoPhase == NEGOPHASE_CONNECT);
}




// Gets whether or not an error has occurred.
// 
// Returns true if there was an error; otherwise, returns false.
//
bool HIO2CommDevice::IsError()
{
    return mNegoPhase == NEGOPHASE_ERROR;
}


// Negotiates synchronized communications with the mcs server.
// 
// Returns 0 if the function was successful.
// Returns an error code (non-zero value) if the function fails.
//
u32 HIO2CommDevice::Negotiate()
{
    if(IsError())
        return MCS_ERROR_COMERROR;

    u32 hioStatus;

    if(!HIO_ReadStatus(mHioHandle, &hioStatus))
        goto HioError;

    if(hioStatus & HIO2_STATUS_TX)
    {
        u32 mailWord;
        if(!HIO_ReadMailbox(mHioHandle, &mailWord))
            goto HioError;

        // NW4R_DB_LOG("read mail %08X\n", mailWord);

        switch (mailWord)
        {
            case HioNegotiate::RebootCode:
                // Sends reboot mail again because the server has just started.
                mNegoPhase = NEGOPHASE_ATTACHED;
                break;

            case HioNegotiate::InitBufferCode:
                if(!HIO_WriteMailbox(mHioHandle, HioNegotiate::AckCode))
                    goto HioError;

                // NW4R_DB_LOG("Write ack mail.\n");

                InitHioRingBuffer();
                mRBReader.EnablePort();
                mRBWriter.EnablePort();

                InitRegisteredBuffer();     // Destroy the contents of the registered receive buffer

                mNegoPhase = NEGOPHASE_CONNECT;
                break;

            case HioNegotiate::Disconnect:
                // Disconnected by server. Try sending reboot mail
                // NW4R_DB_LOG("server disconnect.\n");
                mNegoPhase = NEGOPHASE_ATTACHED;
                break;

            default:
                // Unknown mail. Try sending reboot mail again
                EA_ASSERT_FORMATTED(false, ("Unknown mail %08X, Write retry reboot mail.\n", mailWord));
                mNegoPhase = NEGOPHASE_ATTACHED;
                break;
        }
    }

    if(mNegoPhase == NEGOPHASE_ATTACHED)
    {
        if(!HIO_WriteMailbox(mHioHandle, HioNegotiate::RebootCode))
            goto HioError;

        // NW4R_DB_LOG("Write reboot mail.\n");
        mNegoPhase = NEGOPHASE_REBOOT;
    }

    return MCS_ERROR_SUCCESS;

    // Error generated by HIO function
    HioError:
    mNegoPhase = NEGOPHASE_ERROR;
    return MCS_ERROR_COMERROR;
}


// Initialize shared memory information used to carry out communications with the mcs server.
//
void HIO2CommDevice::InitHioRingBuffer()
{
    mRBWriter.Init(mHioHandle, 0                  , WRITE_TRANSBUF_SIZE, mTempBuf);
    mRBReader.Init(mHioHandle, WRITE_TRANSBUF_SIZE, READ_TRANSBUF_SIZE , AddOffsetToPtr(mTempBuf, WRITE_TRANSBUF_SIZE));
}


// Clears the contents of the receive buffers registered for each channel.
//
void HIO2CommDevice::InitRegisteredBuffer()
{
    for(ChannelInfoList::iterator it = mChannelInfoList.begin(); it != mChannelInfoList.end(); ++it)
    {
        ChannelInfo& channelInfo = *it;
        channelInfo.recvBuf.Discard();
    }
}


// Gets a pointer to ChannelInfo for the specified channel.
// 
// channel: Value for uniquely identifying the communication partner when sending and receiving data.
// 
// If ChannelInfo exists for the specified channel, a pointer to that variable is returned. Otherwise, returns NULL.
//
ChannelInfo* HIO2CommDevice::GetChannelInfo(ChannelType channel)
{
    for(ChannelInfoList::iterator it = mChannelInfoList.begin(); it != mChannelInfoList.end(); ++it)
    {
        ChannelInfo& channelInfo = *it;

        if(channelInfo.channel == channel)
            return &channelInfo;
    }

    return NULL;
}


// Temporarily wait to send data.
// 
// Returns 0 if the function was successful.
// Returns an error code (non-zero value) if the function fails.
//
u32 HIO2CommDevice::WaitSendData()
{
    const u32 errorCode = Polling();

    if(errorCode)  // Set so that the PC-side is not blocked by the write wait
        return errorCode;

    OSSleepMilliseconds(8);

    return MCS_ERROR_SUCCESS;
}


// Temporarily wait to receive data.
// 
// Returns 0 if the function was successful.
// Returns an error code (non-zero value) if the function fails.
//
u32 HIO2CommDevice::WaitRecvData()
{
    const u32 errorCode = Polling();

    if(errorCode)  // Set so that the PC-side is not blocked by the write wait
        return errorCode;

    OSSleepMilliseconds(8);

    return MCS_ERROR_SUCCESS;
}


} // namespace detail



} // namespace IO

} // namespace EA




