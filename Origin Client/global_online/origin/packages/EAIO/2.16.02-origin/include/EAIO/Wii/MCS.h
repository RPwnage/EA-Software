/////////////////////////////////////////////////////////////////////////////
// MCS.h
//
// Copyright (c) 2007 Criterion Software Limited and its licensors. All Rights Reserved.
// Created by Paul Pedriana
/////////////////////////////////////////////////////////////////////////////


#ifndef EAIO_WII_MCS_H
#define EAIO_WII_MCS_H


#include <revolution/types.h>
#include <revolution/hio2.h>
#include <revolution/os.h>
#include <EASTL/intrusive_list.h>


namespace EA
{
    namespace IO
    {
        typedef uint16_t ChannelType;


        const int HIO2FuncRetryCountMax   = 30;
        const int HIO2FuncRetrySleepTime  = 8;   // Milliseconds


        enum MCSResult
        {
            MCS_ERROR_SUCCESS = 0,
            MCS_ERROR_COMERROR,
            MCS_ERROR_NOTCONNECT
        };


        namespace detail
        {
            struct ChannelInfo;
        }


        class AutoMutexLock
        {
        public:
            AutoMutexLock(OSMutex& lockObj) : mLockObj(lockObj)
              { OSLockMutex(&lockObj); }
           ~AutoMutexLock()
              { OSUnlockMutex(&mLockObj); }
            
        protected:
            OSMutex& mLockObj;
        };


        ///////////////////////////////////////////////////////////////////
        // CommDevice
        ///////////////////////////////////////////////////////////////////

        struct DeviceInfo;


        class IDeviceEnumerate
        {
        public:
            virtual bool Find(const DeviceInfo& deviceInfo) = 0;
        };


        class CommDevice
        {
        public:
            //CommDevice(){}
            virtual ~CommDevice(){}

            virtual u32  Open() = 0;
            virtual void Close() = 0;
            virtual void RegisterBuffer(ChannelType channel, void* buf, u32 bufSize) = 0;
            virtual void UnregisterBuffer(ChannelType channel) = 0;
            virtual u32  Polling() = 0;
            virtual u32  GetReadableBytes(ChannelType channel) = 0;
            virtual u32  Peek(ChannelType channel, void* data, u32 size) = 0;
            virtual u32  Read(ChannelType channel, void* data, u32 size) = 0;
            virtual u32  GetWritableBytes( ChannelType channel, u32* pWritableBytes) = 0;
            virtual u32  Write(ChannelType channel, const void* data, u32 size) = 0;
            virtual bool IsServerConnect() = 0;
        };


        ///////////////////////////////////////////////////////////////////
        // MCS
        ///////////////////////////////////////////////////////////////////

        void Mcs_Init();
        bool Mcs_IsInitialized();
        void Mcs_EnumDevices(IDeviceEnumerate* pEnumerate, u32 flag = 0);
        u32  Mcs_GetDeviceObjectMemSize(const DeviceInfo& deviceInfo);
        bool Mcs_CreateDeviceObject(const DeviceInfo& deviceInfo, void* buf, u32 bufSize);
        void Mcs_DestroyDeviceObject();
        bool Mcs_IsDeviceObjectCreated();
        void Mcs_RegisterBuffer(ChannelType channel, void* buf, u32 bufSize);
        void Mcs_UnregisterBuffer(ChannelType channel);
        u32  Mcs_Open();
        void Mcs_Close();
        u32  Mcs_Polling();
        u32  Mcs_GetReadableBytes(ChannelType channel);
        u32  Mcs_Peek(ChannelType channel, void* data, u32 size);
        u32  Mcs_Read(ChannelType channel, void* data, u32 size);
        u32  Mcs_Skip(ChannelType channel, u32 size);
        u32  Mcs_GetWritableBytes(ChannelType channel, u32* pWritableBytes);
        u32  Mcs_Write(ChannelType channel, const void* data, u32 size);
        bool Mcs_IsServerConnect();



        namespace detail
        {
            ///////////////////////////////////////////////////////////////////
            // SimpleRingBuffer
            ///////////////////////////////////////////////////////////////////

            struct SimpleDataRange
            {
                u32 start;
                u32 end;
            };


            struct SimpleRingBufferHeader
            {
                u32             bufSize;
                SimpleDataRange dataRange;
                OSMutex         mutex;
            };


            class SimpleRingBuffer
            {
            public:
                SimpleRingBuffer() {}

                void Init(void* buf, u32 size);
                void Discard();
                u32  GetReadableBytes();
                u32  GetWritableBytes();
                u32  Read(void* buf, u32 size);
                void BeginRead(SimpleDataRange* pDataRange);
                u32  ContinueRead(SimpleDataRange* pDataRange, void* buf, u32 size);
                void EndRead(const SimpleDataRange& dataRange);
                u32  Write(const void* buf, u32 size);
                void BeginWrite(SimpleDataRange* pDataRange);
                u32  ContinueWrite( SimpleDataRange* pDataRange, const void* buf, u32 size);
                void EndWrite(const SimpleDataRange& dataRange);

            protected:
                SimpleRingBufferHeader* mpHeader;
            };


            // Information recorded for each channel
            struct ChannelInfo : public eastl::intrusive_list_node
            {
                u32              channel;
                SimpleRingBuffer recvBuf;
            };

            typedef eastl::intrusive_list<ChannelInfo> ChannelInfoList;


            ///////////////////////////////////////////////////////////////////
            // Hio2RingBuffer
            ///////////////////////////////////////////////////////////////////

            class Hio2RingBuffer
            {
            public:
                static const int CopyAlignment = 32;

            public:
                Hio2RingBuffer() {}
                Hio2RingBuffer(HIO2Handle devHandle, u32 bufOffset, u32 size, void* tempBuf)
                   { Init(devHandle, bufOffset, size, tempBuf); }

                void Init(HIO2Handle devHandle, u32 baseOffset, u32 size, void* tempBuf);
                void InitBuffer();

                void  EnablePort()            { mEnablePort = true;  }
                void  DisablePort()           { mEnablePort = false; }
                bool  IsPort()                { return mEnablePort; }
                u32   GetBufferSize() const   { return mBufferSize; }
                bool  Read(bool* pbAvailable);
                void* GetMessage(u32* pChannel, u32* pTotalSize);
                bool  GetWritableBytes(u32* pBytes, bool withUpdate = true);
                bool  Write(u32 channel, const void* buf, u32 size);

            protected:
                bool ReadSection_(u32 offset, void* data, u32 size);
                bool WriteSection_(u32 offset, const void* data, u32 size);
                bool UpdateReadWritePoint_();
                bool UpdateReadPoint_();
                bool UpdateWritePoint_();

            protected:
                HIO2Handle mDevHandle;         // 
                u32        mBaseOffset;        // Offset for the buffer to be used
                u32        mBufferSize;        // Size of the buffer that can be used (excluding the header)
                void*      mTempBuf;           // Temporary read/write buffer
                u32        mReadPoint;         // Point to read at this time
                u32        mWritePoint;        // Point to write at this time
                u32        mProcBytes;         // Number of bytes processed in temporary buffer
                u32        mTransBytes;        // Number of bytes transferred to temporary buffer
                bool       mEnablePort;        // Can USB2EXI be used without error?
                u8         padding[3];         // warning handling
            };



            ///////////////////////////////////////////////////////////////////
            // HioNegotiate
            ///////////////////////////////////////////////////////////////////

            class HioNegotiate
            {
            public:
                static const u32 RebootCode     = 0x1FFFFFFF & ('M' << 24 | 'C' << 16 | 'S' << 8 | '0');
                static const u32 InitBufferCode = 0x1FFFFFFF & ('I' << 24 | 'N' << 16 | 'B' << 8 | 'F');
                static const u32 AckCode        = 0x1FFFFFFF & ('A' << 24 | 'C' << 16 | 'K' << 8 | 'N');
                static const u32 Disconnect     = 0x1FFFFFFF & ('D' << 24 | 'I' << 16 | 'S' << 8 | 'C');
            };


            ///////////////////////////////////////////////////////////////////
            // HIO2CommDevice
            ///////////////////////////////////////////////////////////////////

            class IHIO2CommDeviceEnumerate
            {
            public:
                virtual bool Find(int devType) = 0;
            };


            class HIO2CommDevice : public CommDevice
            {
            public:
                static const u32 READ_TRANSBUF_SIZE  = 0x00001000;
                static const u32 WRITE_TRANSBUF_SIZE = 0x00001000;
                static const u32 TEMP_BUF_SIZE_MIN   = READ_TRANSBUF_SIZE + WRITE_TRANSBUF_SIZE;

            public:
                HIO2CommDevice(int devType, void* tempBuf, u32 tempBufSize);
                virtual ~HIO2CommDevice();

                virtual void RegisterBuffer(ChannelType channel, void* buf, u32 bufSize);
                virtual void UnregisterBuffer(ChannelType channel);
                virtual u32  Open();
                virtual void Close();
                virtual u32  Polling();
                virtual u32  GetReadableBytes(ChannelType channel);
                virtual u32  Peek(ChannelType channel, void* data, u32 size);
                virtual u32  Read(ChannelType channel, void* data, u32 size);
                virtual u32  GetWritableBytes(ChannelType channel, u32* pWritableBytes);
                virtual u32  Write(ChannelType channel, const void* data, u32 size);
                virtual bool IsServerConnect();

                static bool EnumerateDevice(IHIO2CommDeviceEnumerate* pEnumerate);

            protected:
                bool           IsError();
                u32            Negotiate();
                void           InitHioRingBuffer();
                void           InitRegisteredBuffer();
                ChannelInfo*   GetChannelInfo(ChannelType channel);
                u32            WaitSendData();
                u32            WaitRecvData();

            protected:
                const int       mDevType;
                HIO2Handle      mHioHandle;
                Hio2RingBuffer  mRBReader;
                Hio2RingBuffer  mRBWriter;
                void*           mTempBuf;
                u32             mTempBufSize;
                bool            mbReadInitCodeMail;
                u32             mNegoPhase;
                OSMutex         mMutex;
                bool            mbOpen;
                ChannelInfoList mChannelInfoList;
            };

        } // namespace detail


        enum MCSDeviceType
        {
            DEVICETYPE_UNKNOWN,
            DEVICETYPE_USBADAPTER,
            DEVICETYPE_NDEV
        };

        enum MCSDeviceObjectSize
        {
            DEVICETYPE_UNKNOWN_SIZE    = 0,
            DEVICETYPE_USBADAPTER_SIZE = 0,
            DEVICETYPE_NDEV_SIZE       = sizeof(detail::HIO2CommDevice) + detail::Hio2RingBuffer::CopyAlignment + detail::HIO2CommDevice::TEMP_BUF_SIZE_MIN
        };

        struct DeviceInfo
        {
            u32 deviceType; // One of enum MCSDeviceType
            u32 param1;
            u32 param2;
            u32 param3;
        };

    } // namespace IO

} // namespace EA



///////////////////////////////////////////////////////////////////////////////
// Iinlines
///////////////////////////////////////////////////////////////////////////////


namespace EA
{
    namespace IO
    {
        namespace detail
        {
            template<typename Type>
            inline const Type& Min(const Type& a, const Type& b)
                { return a < b ? a: b; }


            template<typename Type>
            inline Type RoundUp(Type value, u32 alignment)
                { return Type((u32(value) + alignment -1) & ~(alignment -1)); }


            template<typename Type>
            inline Type RoundDown(Type value, u32 alignment)
                { return Type(u32(value) & ~(alignment -1)); }


            template <typename T>
            inline T* AddOffsetToPtr(T* ptr, intptr_t offset)
            {
                return (T*)((uintptr_t)ptr + offset);
            }

        } // namespace detail

    } // namespace IO

} // namespace EA


#endif // Header include guard

