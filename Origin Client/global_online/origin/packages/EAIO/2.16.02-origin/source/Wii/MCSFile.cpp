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
#include <EAIO/Wii/MCSFile.h>
#include <EAIO/Wii/MCS.h>
#include <EASTL/fixed_list.h>
#include <revolution.h>
#include <cstring>
#include EA_ASSERT_HEADER


namespace
{
    using namespace EA::IO;
    using namespace EA::IO::detail;

    bool        sbMcsFileInitialized = false;
    char        sWindowsRootDirectory[260] = { 0 };  // All paths are appended to this.
    OSMutex     sMcsFileMutex;
    FileIOWork* spFileIOWork = NULL;


    const int SEND_RETRY_MAX       = 10;
    const int LOOP_RETRY_MAX       = 1000;
    const int LOOP_RETRY_NOTIMEOUT = -1;


    // Check the connection status with the mcs server.
    // 
    // Returns true if connected to the mcs server; otherwise, returns FALSE.
    // 
    bool CheckConnect()
    {
        if(Mcs_IsServerConnect())
            return true;

        // EA_FAIL_MSG("Mcs file I/O: not sever connect\n"); Disabled; we let the user handle failure.
        return false;
    }


    // Copies the path string.
    // dst:  Copy destination buffer for the string.
    // src:  Copy source string.
    void CopyUserPathString(char16_t* dst, const char* src)
    {
        char* dst8 = reinterpret_cast<char*>(dst);

        std::strcpy(dst8, sWindowsRootDirectory); // To consider: implement checks.
        std::strcat(dst8, src);
    }


    void CopyWindowsPathString(char* dst, const char16_t* src)
    {
        const char* src8 = reinterpret_cast<const char*>(src);

        std::strcpy(dst, src8 + strlen(sWindowsRootDirectory)); // To consider: implement checks.
    }


    char* StrNCpy(char* dst, const char* src, size_t n)
    {
        if(src)
            std::strncpy(dst, src, n);
        else
            std::memset(dst, '\0', n);

        return dst;
    }


    // Sends data.
    // Retries several times until transmission succeeds.
    // 
    // pFindInfo: Not used at the current time.
    // buf:       Pointer to the buffer that stores the data to send.
    // size:      The size of the data to send.              
    // 
    // Returns 0 if the function was successful.
    // Returns an error code (non-zero value) if the function fails.
    // 
    u32 WriteStream(FileInfo* /*pFileInfo*/, const void* buf, u32 size)
    {
        for(int retryCount = 0; retryCount < SEND_RETRY_MAX; ++retryCount)
        {
            u32 writableBytes;
            u32 errorCode = Mcs_GetWritableBytes(FILEIO_CHANNEL, &writableBytes);

            if(errorCode != MCS_ERROR_SUCCESS)
                return errorCode;

            if(size <= writableBytes)
                return Mcs_Write(FILEIO_CHANNEL, buf, size);
            else
                OSSleepMilliseconds(16);
        }

        EA_FAIL_MSG("NW4R Mcs File I/O: send time out\n");
        return FILEIO_ERROR_COMERROR;
    }


    // Wait for transmission.
    // 
    // pFindInfo:  Not used at the current time.
    // 
    // Returns 0 if the function was successful.
    // Returns an error code (non-zero value) if the function fails.
    //
    u32 ReceiveResponse(FileInfo* /*pFindInfo*/, int retryCnt = LOOP_RETRY_MAX)
    {
        FileIOChunkHeader resChunkHead;

        int loopCnt = 0;

        for(; (LOOP_RETRY_NOTIMEOUT == retryCnt) || (loopCnt < retryCnt); ++loopCnt)
        {
            const u32 errorCode = Mcs_Polling();

            if(errorCode)
                return errorCode;

            if(Mcs_GetReadableBytes(FILEIO_CHANNEL) >= sizeof(resChunkHead))
                break;

            OSSleepMilliseconds(8);
        }

        if((LOOP_RETRY_NOTIMEOUT != retryCnt) && (loopCnt >= retryCnt))
        {
            EA_FAIL_MSG("NW4R Mcs File I/O: polling time out\n");
            return FILEIO_ERROR_COMERROR;
        }

        Mcs_Peek(FILEIO_CHANNEL, &resChunkHead, sizeof(resChunkHead));

        loopCnt = 0;

        for(; (LOOP_RETRY_NOTIMEOUT == retryCnt) || (loopCnt < retryCnt); ++loopCnt)
        {
            const u32 errorCode = Mcs_Polling();

            if(errorCode)
                return errorCode;

            if((Mcs_GetReadableBytes(FILEIO_CHANNEL) + resChunkHead.chunkSize) >= sizeof(FileIOChunkHeader))
                break;

            OSSleepMilliseconds(8);
        }

        if((LOOP_RETRY_NOTIMEOUT != retryCnt) && (loopCnt >= retryCnt))
        {
            EA_FAIL_MSG("NW4R Mcs File I/O: polling time out\n");
            return FILEIO_ERROR_COMERROR;
        }

        return MCS_ERROR_SUCCESS;
    }

}   // namespace



namespace EA
{
namespace IO
{


///////////////////////////////////////////////////////////////////////////////
// McsFileSystem
///////////////////////////////////////////////////////////////////////////////

struct DeviceEnumerate : public IDeviceEnumerate
{
    DeviceInfo mDeviceInfo;

    DeviceEnumerate()
        { mDeviceInfo.deviceType = 0xffffffff; }

    bool Find(const DeviceInfo& deviceInfo)
        { mDeviceInfo = deviceInfo; return false; } // False means to stop the iteration/search.
};


McsFileSystem::McsFileSystem()
  : mbInitialized(false)
{
    #if defined(EA_DEBUG)
        memset(mWorkBuffer, 0, sizeof(mWorkBuffer));
        memset(mDeviceObjectBuffer, 0, sizeof(mDeviceObjectBuffer));
    #endif
}


McsFileSystem::~McsFileSystem()
{
    Shutdown();
}


bool McsFileSystem::Init(const char* pWindowsRoot)
{
    VIInit();       // Wii Video Interface init
    Mcs_Init();
    FileIO_Init();
    FileIO_SetWindowsRoot(pWindowsRoot);

    DeviceEnumerate deviceEnumerate;
    Mcs_EnumDevices(&deviceEnumerate);

    if(!mbInitialized)
    {
        // Create a communication device object.
        const u32 deviceObjBufSize = Mcs_GetDeviceObjectMemSize(deviceEnumerate.mDeviceInfo);
        EA_ASSERT((deviceObjBufSize > 0) && (deviceObjBufSize <= DEVICETYPE_NDEV_SIZE)); (void)deviceObjBufSize;

        bool bResult = Mcs_CreateDeviceObject(deviceEnumerate.mDeviceInfo, mDeviceObjectBuffer, DEVICETYPE_NDEV_SIZE);
        EA_ASSERT(bResult);

        // Register a buffer for the file I/O function.
        FileIO_RegisterBuffer(mWorkBuffer);

        // Open the device
        const u32 errorCode = Mcs_Open();

        if(errorCode == MCS_ERROR_SUCCESS)
        {
            // Wait for mcs server to connect.
            bool bConnected = Mcs_IsServerConnect();

            if(!bConnected)
            {
                const u32 giveupTime = OSGetTick() + OSMillisecondsToTicks(10000);

                do{
                    VIWaitForRetrace();
                    Mcs_Polling();
                }while(!(bConnected = Mcs_IsServerConnect()) && (OSGetTick() < giveupTime));
            }

            if(bConnected)
            {
                mbInitialized = true;
                return true;
            }
            else
            {
                #ifdef EA_DEBUG
                    OSReport("McsFileSystem::Init: Unable to connect to MCS server. Have you started the NintendoWare MCS server on your PC?\n");
                #endif
            }
        }

        Shutdown();
    }

    return false;
}


void McsFileSystem::Shutdown()
{
    if(mbInitialized)
    {
        // To consider: close any existing files and call hfshutdown (our C FILE system).

        FileIO_UnregisterBuffer();
        Mcs_Close();
        Mcs_DestroyDeviceObject();

        mbInitialized = false;
    }
}



///////////////////////////////////////////////////////////////////////////////
// FileIO
///////////////////////////////////////////////////////////////////////////////

void FileIO_Init()
{
    Mcs_Init();

    if(!sbMcsFileInitialized)
    {
        sbMcsFileInitialized = true;
        OSInitMutex(&sMcsFileMutex);
    }
}


// Allows the user to specify a root directory on the host Windows system.
// This allows the user to work with files that have Unix-like paths without
// drive letters.
// 
// pDirectoryPath: Specifies a directory path on the Windows system.
//                 Can end with a trailing path separator or not.
//
void FileIO_SetWindowsRoot(const char* pDirectoryPath)
{
    if(pDirectoryPath && *pDirectoryPath)
    {
        strncpy(sWindowsRootDirectory, pDirectoryPath, sizeof(sWindowsRootDirectory));
        sWindowsRootDirectory[sizeof(sWindowsRootDirectory) - 1] = 0;

        // Erase a trailing path separator if present. We do this so that the user
        // can use paths like "/abc/def.txt" and they are simply appended to the root.
        const size_t lastPos = strlen(sWindowsRootDirectory) - 1;

        if((sWindowsRootDirectory[lastPos] == '\\') || (sWindowsRootDirectory[lastPos] == '/'))
            sWindowsRootDirectory[lastPos] = 0;
    }
}


// Registers a work area in memory to be used by the file I/O API.
// 
// tempBuf: Pointer to the work area in memory to be used by the file I/O API.
// 
void FileIO_RegisterBuffer(void* tempBuf)
{
    AutoMutexLock lockObj(sMcsFileMutex);

    EA_ASSERT(spFileIOWork == 0);

    spFileIOWork = static_cast<FileIOWork*>(tempBuf);
    Mcs_RegisterBuffer(FILEIO_CHANNEL, spFileIOWork->commBuf, sizeof(spFileIOWork->commBuf));
}


// Unregisters the work area in memory registered using the FileIO_RegisterBuffer function.
// 
void FileIO_UnregisterBuffer()
{
    AutoMutexLock lockObj(sMcsFileMutex);

    EA_ASSERT(spFileIOWork);

    Mcs_UnregisterBuffer(FILEIO_CHANNEL);
    spFileIOWork = 0;
}


// Opens an existing file, or creates and opens a new file.
// 
// pFileInfo: Pointer to the file information structure
// fileName:  File name
// openFlag:  Designates method to create/open file and load type
// 
// Returns 0 if the function was successful.
// Returns an error code (non-zero value) if the function fails.
// 
u32 FileIO_Open(FileInfo* pFileInfo, const char* fileName, u32 openFlag)
{
    AutoMutexLock lockObj(sMcsFileMutex);

    EA_ASSERT(pFileInfo && fileName && spFileIOWork);

    if(!CheckConnect())
        return (pFileInfo->errorCode = FILEIO_ERROR_NOTCONNECT);

    {
        FileIOOpenCommand& cmd = *reinterpret_cast<FileIOOpenCommand*>(spFileIOWork->tempBuf);

        cmd.chunkID     = FILEIO_COMMAND_FILEOPEN;
        cmd.chunkSize   = sizeof(cmd) - sizeof(FileIOChunkHeader);
        cmd.pFileInfo   = pFileInfo;
        cmd.flag        = openFlag;     // This has read/write combined with what Microsoft calls "creation disposition".
        cmd.mode        = 0;            // The meaning of this is unknown, as we don't have source code for the MCS server.

        CopyUserPathString(cmd.fileName, fileName);

        pFileInfo->errorCode = WriteStream(pFileInfo, &cmd, sizeof(cmd));

        if(pFileInfo->errorCode != FILEIO_ERROR_SUCCESS)
            return pFileInfo->errorCode;
    }

    pFileInfo->errorCode = ReceiveResponse(pFileInfo);
    if(pFileInfo->errorCode != FILEIO_ERROR_SUCCESS)
        return pFileInfo->errorCode;

    FileIOChunkHeader chunkHead;

    pFileInfo->errorCode = Mcs_Read(FILEIO_CHANNEL, &chunkHead, sizeof(chunkHead));
    if(pFileInfo->errorCode != FILEIO_ERROR_SUCCESS)
        return pFileInfo->errorCode;

    FileIOOpenResponse res;

    if(sizeof(res) != chunkHead.chunkSize)
        return (pFileInfo->errorCode = FILEIO_ERROR_PROTOCOLERROR);

    pFileInfo->errorCode = Mcs_Read(FILEIO_CHANNEL, &res, sizeof(res));
    if(pFileInfo->errorCode != FILEIO_ERROR_SUCCESS)
        return pFileInfo->errorCode;

    pFileInfo->handle    = res.handle;
    pFileInfo->fileSize  = res.fileSize;
    pFileInfo->errorCode = res.errorCode;

    return pFileInfo->errorCode;
}


// Closes files.
// 
// pFileInfo: Pointer to the file information structure
// 
// Returns 0 if the function was successful.
// Returns an error code (non-zero value) if the function fails.
// 
u32 FileIO_Close(FileInfo* pFileInfo)
{
    AutoMutexLock lockObj(sMcsFileMutex);

    EA_ASSERT(pFileInfo && spFileIOWork);

    if(!CheckConnect())
        return (pFileInfo->errorCode = FILEIO_ERROR_NOTCONNECT);

    {
        FileIOCommand& cmd = *reinterpret_cast<FileIOCommand*>(spFileIOWork->tempBuf);

        cmd.chunkID   = FILEIO_COMMAND_FILECLOSE;
        cmd.chunkSize = sizeof(cmd) - sizeof(FileIOChunkHeader);
        cmd.pFileInfo = pFileInfo;
        cmd.handle    = pFileInfo->handle;

        pFileInfo->errorCode = WriteStream(pFileInfo, &cmd, sizeof(cmd));
        if(pFileInfo->errorCode != FILEIO_ERROR_SUCCESS)
            return pFileInfo->errorCode;
    }

    pFileInfo->errorCode = ReceiveResponse(pFileInfo);
    if(pFileInfo->errorCode != FILEIO_ERROR_SUCCESS)
        return pFileInfo->errorCode;

    FileIOChunkHeader chunkHead;

    pFileInfo->errorCode = Mcs_Read(FILEIO_CHANNEL, &chunkHead, sizeof(chunkHead));
    if(pFileInfo->errorCode != FILEIO_ERROR_SUCCESS)
        return pFileInfo->errorCode;

    FileIOResponse res;

    if(sizeof(res) != chunkHead.chunkSize)
        return pFileInfo->errorCode = FILEIO_ERROR_PROTOCOLERROR;

    pFileInfo->errorCode = Mcs_Read(FILEIO_CHANNEL, &res, sizeof(res));
    if(pFileInfo->errorCode != FILEIO_ERROR_SUCCESS)
        return pFileInfo->errorCode;

    return (pFileInfo->errorCode = res.errorCode);
}


// Loads from a file.
// 
// pFileInfo:   Pointer to the file information structure
// buffer:      Buffer used for loading
// length:      Number of bytes to load
// pReadBytes:  Pointer to a variable that returns the number of bytes actually loaded
// 
// Returns 0 if the function was successful.
// Returns an error code (non-zero value) if the function fails.
// 
u32 FileIO_Read(FileInfo* pFileInfo, void* buffer, u32 length, u32* pReadBytes)
{
    AutoMutexLock lockObj(sMcsFileMutex);

    EA_ASSERT(pFileInfo && spFileIOWork);

    if(!CheckConnect())
        return pFileInfo->errorCode = FILEIO_ERROR_NOTCONNECT;

    {
        FileIOReadCommand& cmd = *reinterpret_cast<FileIOReadCommand*>(spFileIOWork->tempBuf);

        cmd.chunkID   = FILEIO_COMMAND_FILEREAD;
        cmd.chunkSize = sizeof(cmd) - sizeof(FileIOChunkHeader);
        cmd.pFileInfo = pFileInfo;
        cmd.handle    = pFileInfo->handle;
        cmd.pBuffer   = buffer;
        cmd.size      = length;

        // Send command
        pFileInfo->errorCode = WriteStream(pFileInfo, &cmd, sizeof(cmd));
        if(pFileInfo->errorCode != FILEIO_ERROR_SUCCESS)
            return pFileInfo->errorCode;
    }

    while(true)
    {
        // Wait for command results.
        pFileInfo->errorCode = ReceiveResponse(pFileInfo);
        if(pFileInfo->errorCode != FILEIO_ERROR_SUCCESS)
            return pFileInfo->errorCode;

        FileIOChunkHeader chunkHead;

        pFileInfo->errorCode = Mcs_Read(FILEIO_CHANNEL, &chunkHead, sizeof(chunkHead));
        if(pFileInfo->errorCode != FILEIO_ERROR_SUCCESS)
            return pFileInfo->errorCode;

        FileIOReadResponse res;

        if(sizeof(res) > chunkHead.chunkSize)
            return pFileInfo->errorCode = FILEIO_ERROR_PROTOCOLERROR;

        pFileInfo->errorCode = Mcs_Read(FILEIO_CHANNEL, &res, sizeof(res));
        if(pFileInfo->errorCode != FILEIO_ERROR_SUCCESS)
            return pFileInfo->errorCode;

        if(res.errorCode != FILEIO_ERROR_CONTINUE && res.errorCode != FILEIO_ERROR_SUCCESS)
            return pFileInfo->errorCode = res.errorCode;

        if(sizeof(res) + res.size != chunkHead.chunkSize)
            return pFileInfo->errorCode = FILEIO_ERROR_PROTOCOLERROR;

        pFileInfo->errorCode = Mcs_Read(FILEIO_CHANNEL, res.pBuffer, res.size);
        if(pFileInfo->errorCode != FILEIO_ERROR_SUCCESS)
            return pFileInfo->errorCode;

        if(res.errorCode == FILEIO_ERROR_SUCCESS)
        {
            pFileInfo->errorCode = res.errorCode;
            *pReadBytes = res.totalSize;   // Number of bytes to load
            break;
        }
    }

    return pFileInfo->errorCode;
}


// Writes to a file.
// 
// pFileInfo:  Pointer to the file information structure
// buffer:     Pointer to the buffer that stores data to be written
// length:     Number of bytes to write
// 
// Returns 0 if the function was successful.
// Returns an error code (non-zero value) if the function fails.
// 
u32 FileIO_Write(FileInfo* pFileInfo, const void* buffer, u32 length)
{
    AutoMutexLock lockObj(sMcsFileMutex);

    EA_ASSERT(pFileInfo && spFileIOWork);

    if(!CheckConnect())
        return pFileInfo->errorCode = FILEIO_ERROR_NOTCONNECT;

    const u8* fromBuf = static_cast<const u8*>(buffer);
    const u32 maxSize = FILEIO_CHUNK_SIZE_MAX - sizeof(FileIOWriteCommand);

    while (length > 0)
    {
        const u32 sendSize = Min(length, maxSize);

        {
            FileIOWriteCommand& cmd = *reinterpret_cast<FileIOWriteCommand*>(spFileIOWork->tempBuf);

            cmd.chunkID     = FILEIO_COMMAND_FILEWRITE;
            cmd.chunkSize   = sizeof(cmd) - sizeof(FileIOChunkHeader) + sendSize;
            cmd.pFileInfo   = pFileInfo;
            cmd.handle      = pFileInfo->handle;
            cmd.size        = sendSize;

            // Send command
            pFileInfo->errorCode = WriteStream(pFileInfo, &cmd, sizeof(cmd));
            if(pFileInfo->errorCode != FILEIO_ERROR_SUCCESS)
                return pFileInfo->errorCode;
        }

        pFileInfo->errorCode = WriteStream(pFileInfo, fromBuf, sendSize);
        if(pFileInfo->errorCode != FILEIO_ERROR_SUCCESS)
            return pFileInfo->errorCode;

        pFileInfo->errorCode = ReceiveResponse(pFileInfo);
        if(pFileInfo->errorCode != FILEIO_ERROR_SUCCESS)
            return pFileInfo->errorCode;

        FileIOChunkHeader chunkHead;

        pFileInfo->errorCode = Mcs_Read(FILEIO_CHANNEL, &chunkHead, sizeof(chunkHead));
        if(pFileInfo->errorCode != FILEIO_ERROR_SUCCESS)
            return pFileInfo->errorCode;

        FileIOResponse res;

        if(sizeof(res) != chunkHead.chunkSize)
            return pFileInfo->errorCode = FILEIO_ERROR_PROTOCOLERROR;

        pFileInfo->errorCode = Mcs_Read(FILEIO_CHANNEL, &res, sizeof(res));
        if(pFileInfo->errorCode != FILEIO_ERROR_SUCCESS)
            return pFileInfo->errorCode;

        pFileInfo->errorCode = res.errorCode;

        // Escape if an error occurred.
        if(pFileInfo->errorCode != FILEIO_ERROR_SUCCESS)
            break;

        length  -= sendSize;
        fromBuf += sendSize;
    }

    return pFileInfo->errorCode;
}


// Moves the file pointer to the specified position.
// 
// pFileInfo:        Pointer to the file information structure
// distanceToMove:   Number of bytes to move the file pointer
// moveMethod:       Position to start the file pointer move
// pNewFilePointer:  Pointer to the variable that holds the new file pointer
// 
// Returns 0 if the function was successful.
// Returns an error code (non-zero value) if the function fails.
// 
u32 FileIO_Seek(FileInfo* pFileInfo, s32 distanceToMove, u32 moveMethod, u32* pNewFilePointer)
{
    AutoMutexLock lockObj(sMcsFileMutex);

    EA_ASSERT(pFileInfo && spFileIOWork);

    if(!CheckConnect())
        return pFileInfo->errorCode = FILEIO_ERROR_NOTCONNECT;

    {
        FileIOSeekCommand& cmd = *reinterpret_cast<FileIOSeekCommand*>(spFileIOWork->tempBuf);

        cmd.chunkID         = FILEIO_COMMAND_FILESEEK;
        cmd.chunkSize       = sizeof(cmd) - sizeof(FileIOChunkHeader);
        cmd.pFileInfo       = pFileInfo;
        cmd.handle          = pFileInfo->handle;
        cmd.distanceToMove  = distanceToMove;
        cmd.moveMethod      = moveMethod;

        pFileInfo->errorCode = WriteStream(pFileInfo, &cmd, sizeof(cmd));
        if(pFileInfo->errorCode != FILEIO_ERROR_SUCCESS)
            return pFileInfo->errorCode;
    }

    pFileInfo->errorCode = ReceiveResponse(pFileInfo);
    if(pFileInfo->errorCode != FILEIO_ERROR_SUCCESS)
        return pFileInfo->errorCode;

    FileIOChunkHeader chunkHead;

    pFileInfo->errorCode = Mcs_Read(FILEIO_CHANNEL, &chunkHead, sizeof(chunkHead));
    if(pFileInfo->errorCode != FILEIO_ERROR_SUCCESS)
        return pFileInfo->errorCode;

    FileIOSeekResponse res;

    if(sizeof(res) != chunkHead.chunkSize)
        return pFileInfo->errorCode = FILEIO_ERROR_PROTOCOLERROR;

    pFileInfo->errorCode = Mcs_Read(FILEIO_CHANNEL, &res, sizeof(res));
    if(pFileInfo->errorCode != FILEIO_ERROR_SUCCESS)
        return pFileInfo->errorCode;

    pFileInfo->errorCode = res.errorCode;

    if(pFileInfo->errorCode == FILEIO_ERROR_SUCCESS && pNewFilePointer)
        *pNewFilePointer = res.filePointer;

    return pFileInfo->errorCode;
}


u32 FileIO_GetOpenFileSize(const FileInfo* pFileInfo) 
{
    return pFileInfo->fileSize;
}


// Searches the directory for a file with the same name as the one specified.
// 
// pFileInfo: Pointer to the file information structure.
// pFindData: Pointer to the structure that stores information about the found file.
// fileName:  File name pattern to search for.
// 
// Returns 0 if the function was successful.
// Returns an error code (non-zero value) if the function fails.
// 
u32 FileIO_FindFirst(FileInfo* pFileInfo, FindData* pFindData, const char* fileName)
{
    AutoMutexLock lockObj(sMcsFileMutex);

    EA_ASSERT(pFileInfo && pFindData && fileName && spFileIOWork);

    if(!CheckConnect())
        return pFileInfo->errorCode = FILEIO_ERROR_NOTCONNECT;

    {
        FileIOFindFirstCommand& cmd = *reinterpret_cast<FileIOFindFirstCommand*>(spFileIOWork->tempBuf);

        cmd.chunkID     = FILEIO_COMMAND_FINDFIRST;
        cmd.chunkSize   = sizeof(cmd) - sizeof(FileIOChunkHeader);
        cmd.pFileInfo   = pFileInfo;
        cmd.pFindData   = pFindData;
        cmd.mode        = 0;            // The meaning of this is unknown, as we don't have source code for the MCS server.

        CopyUserPathString(cmd.fileName, fileName);

        pFileInfo->errorCode = WriteStream(pFileInfo, &cmd, sizeof(cmd));
        if(pFileInfo->errorCode != FILEIO_ERROR_SUCCESS)
            return pFileInfo->errorCode;
    }

    pFileInfo->errorCode = ReceiveResponse(pFileInfo);
    if(pFileInfo->errorCode != FILEIO_ERROR_SUCCESS)
        return pFileInfo->errorCode;

    FileIOChunkHeader chunkHead;

    pFileInfo->errorCode = Mcs_Read(FILEIO_CHANNEL, &chunkHead, sizeof(chunkHead));
    if(pFileInfo->errorCode != FILEIO_ERROR_SUCCESS)
        return pFileInfo->errorCode;

    FileIOFindResponse res;

    if(sizeof(res) != chunkHead.chunkSize)
        return pFileInfo->errorCode = FILEIO_ERROR_PROTOCOLERROR;

    pFileInfo->errorCode = Mcs_Read(FILEIO_CHANNEL, &res, sizeof(res));
    if(pFileInfo->errorCode != FILEIO_ERROR_SUCCESS)
        return pFileInfo->errorCode;

    pFileInfo->handle       = res.handle;
    pFileInfo->fileSize     = 0;
    pFileInfo->errorCode    = res.errorCode;

    if(pFileInfo->errorCode == FILEIO_ERROR_SUCCESS)
    {
        // If successful, set the members of FindData
        pFindData->size         = res.fileSize;
        pFindData->attribute    = res.fileAttribute;
        pFindData->date         = res.fileDate;
        pFindData->reserved1    = 0;
        pFindData->reserved2    = 0;

        CopyWindowsPathString(pFindData->name, res.fileName);
    }

    return pFileInfo->errorCode;
}


// Searches for the next file matching the specified pattern using the FileIO_FindFirst function.
// 
// pFileInfo: Pointer to the file information structure
// pFindData: Pointer to the structure that stores information about the found file.
// 
// Returns 0 if the function was successful.
// Returns an error code (non-zero value) if the function fails.
// 
u32 FileIO_FindNext(FileInfo* pFileInfo, FindData* pFindData)
{
    AutoMutexLock lockObj(sMcsFileMutex);

    EA_ASSERT(pFileInfo && pFindData && spFileIOWork);

    if(!CheckConnect())
        return pFileInfo->errorCode = FILEIO_ERROR_NOTCONNECT;

    {
        FileIOFindNextCommand& cmd = *reinterpret_cast<FileIOFindNextCommand*>(spFileIOWork->tempBuf);

        cmd.chunkID     = FILEIO_COMMAND_FINDNEXT;
        cmd.chunkSize   = sizeof(cmd) - sizeof(FileIOChunkHeader);
        cmd.pFileInfo   = pFileInfo;
        cmd.handle      = pFileInfo->handle;
        cmd.pFindData   = pFindData;

        pFileInfo->errorCode = WriteStream(pFileInfo, &cmd, sizeof(cmd));
        if(pFileInfo->errorCode != FILEIO_ERROR_SUCCESS)
            return pFileInfo->errorCode;
    }

    pFileInfo->errorCode = ReceiveResponse(pFileInfo);
    if(pFileInfo->errorCode != FILEIO_ERROR_SUCCESS)
        return pFileInfo->errorCode;

    FileIOChunkHeader chunkHead;

    pFileInfo->errorCode = Mcs_Read(FILEIO_CHANNEL, &chunkHead, sizeof(chunkHead));
    if(pFileInfo->errorCode != FILEIO_ERROR_SUCCESS)
        return pFileInfo->errorCode;

    FileIOFindResponse res;

    if(sizeof(res) != chunkHead.chunkSize)
        return pFileInfo->errorCode = FILEIO_ERROR_PROTOCOLERROR;

    pFileInfo->errorCode = Mcs_Read(FILEIO_CHANNEL, &res, sizeof(res));
    if(pFileInfo->errorCode != FILEIO_ERROR_SUCCESS)
        return pFileInfo->errorCode;

    pFileInfo->errorCode = res.errorCode;

    if(pFileInfo->errorCode == FILEIO_ERROR_SUCCESS)
    {
        // If successful, set the members of FindData
        pFindData->size         = res.fileSize;
        pFindData->attribute    = res.fileAttribute;
        pFindData->date         = res.fileDate;
        pFindData->reserved1    = 0;
        pFindData->reserved2    = 0;

        CopyWindowsPathString(pFindData->name, res.fileName);
    }

    return pFileInfo->errorCode;
}


// Ends the search started by the FileIO_FindFirst function.
// 
// pFileInfo: Pointer to the file information structure
// 
// Returns 0 if the function was successful.
// Returns an error code (non-zero value) if the function fails.
// 
u32 FileIO_FindClose(FileInfo* pFileInfo)
{
    AutoMutexLock lockObj(sMcsFileMutex);

    EA_ASSERT(pFileInfo && spFileIOWork);

    if(!CheckConnect())
        return pFileInfo->errorCode = FILEIO_ERROR_NOTCONNECT;

    {
        FileIOCommand& cmd = *reinterpret_cast<FileIOCommand*>(spFileIOWork->tempBuf);

        cmd.chunkID     = FILEIO_COMMAND_FINDCLOSE;
        cmd.chunkSize   = sizeof(cmd) - sizeof(FileIOChunkHeader);
        cmd.pFileInfo   = pFileInfo;
        cmd.handle      = pFileInfo->handle;

        pFileInfo->errorCode = WriteStream(pFileInfo, &cmd, sizeof(cmd));
        if(pFileInfo->errorCode != FILEIO_ERROR_SUCCESS)
            return pFileInfo->errorCode;
    }

    pFileInfo->errorCode = ReceiveResponse(pFileInfo);
    if(pFileInfo->errorCode != FILEIO_ERROR_SUCCESS)
        return pFileInfo->errorCode;

    FileIOChunkHeader chunkHead;

    pFileInfo->errorCode = Mcs_Read(FILEIO_CHANNEL, &chunkHead, sizeof(chunkHead));
    if(pFileInfo->errorCode != FILEIO_ERROR_SUCCESS)
        return pFileInfo->errorCode;

    FileIOResponse res;

    if(sizeof(res) != chunkHead.chunkSize)
        return pFileInfo->errorCode = FILEIO_ERROR_PROTOCOLERROR;

    pFileInfo->errorCode = Mcs_Read(FILEIO_CHANNEL, &res, sizeof(res));
    if(pFileInfo->errorCode != FILEIO_ERROR_SUCCESS)
        return pFileInfo->errorCode;

    return pFileInfo->errorCode = res.errorCode;
}


// Displays a file dialog box.
// 
// fileNameBuf:       Buffer storing the file name.
// fileNameBufSize:   Size of fileNameBuf
// flag:              Flag.
//                    FILEIO_FILEDIALOG_OPEN ... Opens the dialog box for opening a file.
// initialDirectory:  Initial directory.
//                    Specify the null character string "" or NULL when there is no particular specification.
// filter:            Filter string. The filter description and filter are specified in combination and separated by a vertical bar (|).
//                    Specify the null character string "" or NULL when there is no particular specification.
//                    Ex.) "Text files (*.txt)|*.txt|All files (*.*)|*.*"
// defaultExt:        Default extension.
//                    Specify the null character string "" or NULL when there is no particular specification.
// title:             Title string for the file dialog box.
//                    Specify the null character string "" or NULL when there is no particular specification.
// 
// Returns 0 if the function was successful.
// Returns an error code (non-zero value) if the function fails.
// 
u32 FileIO_ShowFileDialog(char* fileNameBuf, u32 fileNameBufSize, u32 flag, const char* initialDirectory, 
                          const char* filter, const char* defaultExt, const char* title)
{
    AutoMutexLock lockObj(sMcsFileMutex);

    EA_ASSERT(fileNameBuf && (fileNameBufSize >= 1) && spFileIOWork);

    if(!CheckConnect())
        return FILEIO_ERROR_NOTCONNECT;

    u32 errorCode;

    {
        FileIOShowFileDlgCommand& cmd = *reinterpret_cast<FileIOShowFileDlgCommand*>(spFileIOWork->tempBuf);

        cmd.chunkID     = FILEIO_COMMAND_SHOWFILEDLG;
        cmd.chunkSize   = sizeof(cmd) - sizeof(FileIOChunkHeader);
        cmd.flag        = flag;
        cmd.mode        = 0;        // The meaning of this is unknown, as we don't have source code for the MCS server.
        cmd.reserved    = 0;

        if(initialDirectory)
            CopyUserPathString(cmd.initialDir, initialDirectory);
        else // An empty string is used when NULL is specified.
            std::memset(cmd.initialDir, '\0', sizeof(cmd.initialDir));

        StrNCpy(reinterpret_cast<char*>(cmd.filter),     filter,     sizeof(cmd.filter));
        StrNCpy(reinterpret_cast<char*>(cmd.defaultExt), defaultExt, sizeof(cmd.defaultExt));
        StrNCpy(reinterpret_cast<char*>(cmd.title),      title,      sizeof(cmd.title));

        errorCode = WriteStream(NULL, &cmd, sizeof(cmd));
        if(errorCode != FILEIO_ERROR_SUCCESS)
            return errorCode;
    }

    errorCode = ReceiveResponse(NULL, LOOP_RETRY_NOTIMEOUT);    // There will be no timeout because a dialog will be displayed.
    if(errorCode != FILEIO_ERROR_SUCCESS)
        return errorCode;

    FileIOChunkHeader chunkHead;

    errorCode = Mcs_Read(FILEIO_CHANNEL, &chunkHead, sizeof(chunkHead));
    if(errorCode != FILEIO_ERROR_SUCCESS)
        return errorCode;

    FileIOShowFileDlgResponse res;

    if(sizeof(res) > chunkHead.chunkSize)
        return FILEIO_ERROR_PROTOCOLERROR;

    errorCode = Mcs_Read(FILEIO_CHANNEL, &res, sizeof(res));
    if(errorCode != FILEIO_ERROR_SUCCESS)
        return errorCode;

    if(res.errorCode != FILEIO_ERROR_SUCCESS)
        return res.errorCode;

    if(sizeof(res) + res.size != chunkHead.chunkSize)
        return FILEIO_ERROR_PROTOCOLERROR;

    // Reads the file string
    errorCode = Mcs_Read(FILEIO_CHANNEL, fileNameBuf, Min(fileNameBufSize, res.size));
    if(errorCode != FILEIO_ERROR_SUCCESS)
        return errorCode;

    if(fileNameBufSize < res.size)
    {
        fileNameBuf[fileNameBufSize - 1] = '\0';

        // Skips remaining contents
        errorCode = Mcs_Skip(FILEIO_CHANNEL, res.size - fileNameBufSize);
        if(errorCode != FILEIO_ERROR_SUCCESS)
            return errorCode;
    }
    else
        std::memset(fileNameBuf + res.size, '\0', fileNameBufSize - res.size);

    return FILEIO_ERROR_SUCCESS;
}






///////////////////////////////////////////////////////////////////////
// Standard C-style file IO
///////////////////////////////////////////////////////////////////////


struct HFILE : public FileInfo
{
    HFILE() { handle = 0; }
};

typedef eastl::fixed_list<HFILE, 8, true> HFILEList;

HFILEList gFileList;
HFILE     gStdInOutErr;
HFILE*    hstdin  = &gStdInOutErr;
HFILE*    hstdout = &gStdInOutErr;
HFILE*    hstderr = &gStdInOutErr;


bool hfinit()
{
    FileIO_Init();

    hstdin  = &gStdInOutErr;
    hstdout = &gStdInOutErr;
    hstderr = &gStdInOutErr;

    if(hffopen(&gStdInOutErr, "stdout", "w"))
        return true;
    return false;
}


void hfshutdown()
{
    EA_ASSERT(spFileIOWork || (gStdInOutErr.handle == 0)); // If this fails then it means we are calling hfshutdown *after* calling McsFileSystem::Shutdown.

    if(spFileIOWork) // If the MCS system is alive...
    {
        if(gStdInOutErr.handle != 0)
            hfclose(&gStdInOutErr);
    }
}


// hffree
//
// Allocates an HFILE
//
HFILE* hfalloc()
{
    gFileList.push_back();
    return &gFileList.back();
}


// hffree
//
// Frees an HFILE.
//
void hffree(HFILE* pFile)
{
    // This search is O(n) and thus is not fast for the case of many open files.
    for(HFILEList::iterator it = gFileList.begin(), itEnd = gFileList.end(); it != itEnd; ++it)
    {
        HFILE* const pFileCurrent = &*it;

        if(pFileCurrent == pFile)
        {
            gFileList.erase(it);
            break;
        }
    }
}


// hclearerr
//
// Resets both the error and the EOF indicators of the stream.
// When a stream function fails either because of an error or because the end of the 
// file has been reached, one of these internal indicators may be set. These indicators 
// remain set until either this, rewind, fseek or fsetpos is called.
//
void hclearerr(HFILE* pFile)
{
    pFile->errorCode = FILEIO_ERROR_SUCCESS;
}


// hfclose
// 
// Closes the file associated with the stream and disassociates it.
// All internal buffers associated with the stream are flushed: the content of any 
// unwritten buffer is written and the content of any unread buffer is discarded.
// Even if the call fails, the stream passed as parameter will no longer be associated 
// with the file.
// 
// stream: Pointer to a FILE object that specifies the stream to be closed. 
// 
// If the stream is successfully closed, a zero value is returned.
// On failure, EOF is returned.
// 
int hfclose(HFILE* pFile)
{
    const int errorCode = (int)(unsigned)FileIO_Close(pFile);

    hffree(pFile);

    return errorCode;
}


// hfeof
//
// Checks if the End-of-File indicator associated with stream is set, 
// returning a value different from zero if it is.
// This indicator is generally set by a previous operation on the stream that 
// reached the End-of-File. Further operations on the stream once the End-of-File 
// has been reached will fail until either rewind, fseek or fsetpos is successfully 
// called to set the position indicator to a new value.
// 
// A non-zero value is returned in the case that the End-of-File indicator associated 
// with the stream is set. Otherwise, a zero value is returned.
//
int hfeof(HFILE* pFile)
{
    u32 positionCurrent;
    u32 result = FileIO_Seek(pFile, 0, FILEIO_SEEK_CURRENT, &positionCurrent);

    if(result == FILEIO_ERROR_SUCCESS)
    {
        u32 positionEnd;
        result = FileIO_Seek(pFile, 0, FILEIO_SEEK_END, &positionEnd);

        FileIO_Seek(pFile, 0, FILEIO_SEEK_BEGIN, &positionCurrent);

        if(positionCurrent != positionEnd)
            return 0;
    }

    return EOF;
}


// hferror
//
// Checks if the error indicator associated with stream is set, returning a 
// value different from zero if it is. This indicator is generaly set by a previous 
// operation on the stream that failed.
//
// If the error indicator associated with the stream was set, the function returns 
// a nonzero value. Otherwise, it returns a zero value.
//
int hferror(HFILE* pFile)
{
    return (int)(unsigned)pFile->errorCode;
}


// hfflush
//
// If the given stream was open for writing and the last i/o operation was an output 
// operation, any unwritten data in the output buffer is written to the file.
// If it was open for reading and the last operation was an input operation, the behavior 
// depends on the specific library implementation. In some implementations this causes the 
// input buffer to be cleared, but this is not standard behavior.
// If the argument is a null pointer, all open files are flushed.
// The stream remains open after this call.
// When a file is closed, either because of a call to fclose or because the program terminates, 
// all the buffers associated with it are automatically flushed.
// 
// A zero value indicates success. If an error occurs, EOF is returned and the error 
// indicator is set (see feof).
// 
int hfflush(HFILE*)
{
    return 0;
}


// hfgetc
//
// Returns the character currently pointed by the internal file position indicator of 
// the specified stream. The internal file position indicator is then advanced by one 
// character to point to the next character.
// fgetc and getc are equivalent, except that getc may be implemented as a macro.
// 
// The character read is returned as an int value.
// If the End-of-File is reached or a reading error happens, the function returns EOF and 
// the corresponding error or eof indicator is set. You can use either ferror or feof to 
// determine whether an error happened or the End-Of-File was reached.
// 
int hfgetc(HFILE* pFile)
{
    u32           readSizeResult;
    unsigned char c;

    pFile->errorCode = FileIO_Read(pFile, &c, 1, &readSizeResult);

    if(pFile->errorCode == FILEIO_ERROR_SUCCESS)
    {
        if(readSizeResult == 1)
            return (int)c;
        // Otherwise we were at the end of the file.
    }

    return EOF;
}


// hfgetpos
//
// Gets the information needed to uniquely identify the current value of the stream's 
// position indicator and stores it in the location pointed by position.
// The parameter position should point to an already allocated object of the type fpos_t, 
// which is only intended to be used as a paremeter in future calls to fsetpos.
// To retrieve the value of the internal file position indicator as an integer value, 
// use ftell function instead.
// 
// pFile:    Pointer to an HFILE object that identifies the stream.
// position: Pointer to a fpos_t object.
// 
// The functions returns zero on success and non-zero in case of error.
//
int hfgetpos(HFILE* pFile, fpos_t* position)
{
    u32 positionCurrent;
    pFile->errorCode = FileIO_Seek(pFile, 0, FILEIO_SEEK_CURRENT, &positionCurrent);

    if(pFile->errorCode == FILEIO_ERROR_SUCCESS)
    {
        *position = (fpos_t)positionCurrent;
        return 0;
    }

    return -1;
}


// hfgets
//
// Reads characters from stream and stores them as a C string into str until (num-1) 
// characters have been read or either a newline or a the End-of-File is reached, 
// whichever comes first.
// A newline character makes fgets stop reading, but it is considered a valid character 
// and therefore it is included in the string copied to str.
// A null character is automatically appended in str after the characters read to signal 
// the end of the C string.
// 
// pString: Pointer to an array of chars where the string read is stored.
// count:   Maximum number of characters to be read (including the final null-character). 
//          Usually, the length of the array passed as str is used.
// 
// On success, the function returns the same str parameter.
// If the End-of-File is encountered and no characters have been read, the contents of 
// str remain unchanged and a null pointer is returned.
// If an error occurs, a null pointer is returned.
// Use either ferror or feof to check whether an error happened or the End-of-File was reached.
//
char* hfgets(char* pString, int count, HFILE* pFile)
{
    int   c  = 0;
    char* cs = pString;

    while((--count > 0) && ((c = hfgetc(pFile)) != EOF))
    {
        *cs++ = c;
        if(c == '\n')
            break;
    }

    if((c == EOF) && (cs == pString))
        return NULL;

    *cs++ = 0;
    return pString;
}


// hfopen
//
// This is identical to hfopen except in this case the user provides the FILE object.
//
HFILE* hffopen(HFILE* pFile, const char* pFilePath, const char* mode)
{
    u32 openFlags = 0;

    if(std::strcmp("a+", mode) == 0)
        openFlags = FILEIO_FLAG_READWRITE | FILEIO_FLAG_SHAREREAD;
    else if(std::strcmp("a", mode) == 0)
        openFlags = FILEIO_FLAG_WRITE | FILEIO_FLAG_SHAREREAD;
    else if(std::strcmp("r+", mode) == 0)
        openFlags = FILEIO_FLAG_READWRITE | FILEIO_FLAG_NOCREATE | FILEIO_FLAG_SHAREREAD;
    else if(std::strcmp("w", mode) == 0)
        openFlags = FILEIO_FLAG_WRITE | FILEIO_FLAG_SHAREREAD;
    else if(std::strcmp("w+", mode) == 0)
        openFlags = FILEIO_FLAG_READWRITE | FILEIO_FLAG_SHAREREAD;
    else if(std::strcmp("r", mode) == 0)
        openFlags = FILEIO_FLAG_READ | FILEIO_FLAG_NOCREATE | FILEIO_FLAG_SHAREREAD;
    else
        return NULL;

    u32 errorCode = FileIO_Open(pFile, pFilePath, openFlags);

    if(errorCode == FILEIO_ERROR_SUCCESS)
    {
        if(mode[0] == 'w')
        {
            // In this case we need to truncate the file.
            // However, we don't have a means to do that via the current MCS protocol.
        }

        return pFile;
    }

    return NULL;
}


// hfopen
//
// Opens the file whose name is specified in the parameter filename and associates it 
// with a stream that can be identified in future operations by the FILE object whose 
// pointer is returned. The operations that are allowed on the stream and how these are 
// performed are defined by the mode parameter.
// The running environment supports at least FOPEN_MAX files open simultaneously; 
// FOPEN_MAX is a macro constant defined in <cstdio>.
// 
// pFilePath: C string containing the name of the file to be opened. This paramenter must follow the file name specifications of the running environment and can include a path if the system supports it.
// mode:      C string containing a file access modes. It can be:
//              "r"    Open a file for reading. The file must exist.
//              "w"    Create an empty file for writing. If a file with the same name already exists its content is erased and the file is treated as a new empty file.
//              "a"    Append to a file. Writing operations append data at the end of the file. The file is created if it does not exist.
//              "r+"   Open a file for update both reading and writing. The file must exist.
//              "w+"   Create an empty file for both reading and writing. If a file with the same name already exists its content is erased and the file is treated as a new empty file.
//              "a+"   Open a file for reading and appending. All writing operations are performed at the end of the file, protecting the previous content to be overwritten. You can reposition (fseek, rewind) the internal pointer to anywhere in the file for reading, but writing operations will move it back to the end of file. The file is created if it does not exist.
// 
// With the mode specifiers above the file is open as a text file. In order to open a 
// file as a binary file, a "b" character has to be included in the mode string. 
// This additional "b" character can either be appended at the end of the string (thus 
// making the following compound modes: "rb", "wb", "ab", "r+b", "w+b", "a+b") or be 
// inserted between the letter and the "+" sign for the mixed modes ("rb+", "wb+", "ab+").
// Additional characters may follow the sequence, although they should have no effect. 
// For example, "t" is sometimes appended to make explicit the file is a text file.
// In the case of text files, depending on the environment where the application runs, 
// some special character conversion may occur in input/output operations to adapt them to 
// a system-specific text file format. In many environments, such as most UNIX-based systems, 
// it makes no difference to open a file as a text file or a binary file; Both are treated 
// exactly the same way, but differentiation is recommended for a better portability.
// For the modes where both read and writing (or appending) are allowed (those which include 
// a "+" sign), the stream should be flushed (fflush) or repositioned (fseek, fsetpos, rewind) 
// between either a reading operation followed by a writing operation or a writing operation 
// followed by a reading operation.
// 
// If the file has been succesfully opened the function will return a pointer to an HFILE object 
// that is used to identify the stream on all further operations involving it.
// Otherwise, a null pointer is returned.
//
HFILE* hfopen(const char* pFilePath, const char* mode)
{
    HFILE* pFile = hfalloc();

    if(pFile)
    {
        if(!hffopen(pFile, pFilePath, mode))
        {
            hffree(pFile);
            pFile = NULL;
        }
    }

    return pFile;   
}


// hfileno
//
// Returns the integer file handle. 
// For us this means returning the Windows HFILE handle for the file.
//
int hfileno(HFILE* pFile)
{
    return (int)(unsigned)pFile->handle;
}


// hfprintf
//
// Acts the same as printf to hstdout.
//
int hfprintf(HFILE* pFile, const char* pFormat, ...)
{
    va_list arguments;

    va_start(arguments, pFormat);
    int result = hvfprintf(pFile, pFormat, arguments);
    va_end(arguments);

    return result;
}


// hfputc
//
// Writes a character to the stream and advances the position indicator.
// The character is written at the current position of the stream as indicated by the 
// internal position indicator, which is then advanced one character.
// 
// character: Character to be written. The character is passed as its int promotion.
// pFile:     Pointer to an HFILE object that identifies the stream where the character is to be written. 
// 
// If there are no errors, the same character that has been written is returned.
// If an error occurs, EOF is returned and the error indicator is set (see ferror).
// 
int hfputc(int character, HFILE* pFile)
{
    const unsigned char c = (unsigned char)(unsigned)character;
    pFile->errorCode = FileIO_Write(pFile, &c, 1);

    if(pFile->errorCode == FILEIO_ERROR_SUCCESS)
        return character;

    return EOF;
}


// hfputs
//
// Writes the string pointed by str to the stream.
// The function begins copying from pString until it reaches the terminating null character ('\0'). 
// This final null character is not copied to the stream.
// 
// pString: An array containing the null-terminated sequence of characters to be written.
// pFile:   Pointer to a HFILE object that identifies the stream where the string is to be written.
// 
// On success, a non-negative value is returned.
// On error, the function returns EOF.
// 
int hfputs(const char* pString, HFILE* pFile)
{
    const size_t len = strlen(pString);

    pFile->errorCode = FileIO_Write(pFile, pString, (u32)len);
    if(pFile->errorCode == FILEIO_ERROR_SUCCESS)
        return (len + 1);

    return EOF;
}


// hfread
//
// Reads an array of count elements, each one with a size of size bytes, 
// from the stream and stores them in the block of memory specified by ptr.
// The postion indicator of the stream is advanced by the total amount of bytes read.
// The total amount of bytes read if successful is (size * count).
// 
// data:  Pointer to a block of memory with a minimum size of (size*count) bytes.
// size:  Size in bytes of each element to be read.
// count: Number of elements, each one with a size of size bytes.
// pFile: Pointer to an HFILE object that specifies an input stream.
// 
// Returns the total number of elements successfully read, 
// If this number differs from the count parameter, either an error occured or the 
// End Of File was reached. You can use either ferror or feof to check whether an 
// error happened or the End-of-File was reached.
//
size_t hfread(void* data, size_t size, size_t count, HFILE* pFile)
{
    const size_t readSize = size * count; // To consider: check for overflow.
    u32 readSizeResult;

    pFile->errorCode = FileIO_Read(pFile, data, (u32)readSize, &readSizeResult);

    if(pFile->errorCode == FILEIO_ERROR_SUCCESS)
        return (readSizeResult / size);

    return 0;
}


// hfseek
//
// Sets the position indicator associated with the stream to a new position defined by 
// adding offset to a reference position specified by origin. The End-of-File internal 
// indicator of the stream is cleared after a call to this function, and all effects from 
// previous calls to ungetc are dropped. When using fseek on text files with offset values 
// other than zero or values retrieved with ftell, bear in mind that on some platforms some 
// format transformations occur with text files which can lead to unexpected repositioning.
// On streams open for update (read+write), a call to fseek allows to switch between reading and writing.
// 
// pFile:  Pointer to an HFILE object that identifies the stream. 
// offset: Number of bytes to offset from origin.
// origin: Position from where offset is added. It is specified by one of the following constants defined in <cstdio>:
//             SEEK_SET    Beginning of file
//             SEEK_CUR    Current position of the file pointer
//             SEEK_END    End of file
// 
// If successful, the function returns a zero value.
// Otherwise, it returns nonzero value.
//
int hfseek(HFILE* pFile, long offset, int origin)
{
    EA_COMPILETIME_ASSERT((SEEK_SET == FILEIO_SEEK_BEGIN) && (SEEK_CUR == FILEIO_SEEK_CURRENT) && (SEEK_END == FILEIO_SEEK_END));

    u32 position;
    pFile->errorCode = FileIO_Seek(pFile, (s32)offset, (u32)(unsigned)origin, &position);

    if(pFile->errorCode == FILEIO_ERROR_SUCCESS)
        return (int)(unsigned)position;

    return -1;
}


// hfsetpos
//
// Changes the internal file position indicator associated with stream to a new position. 
// The position parameter is a pointer to an fpos_t object whose value shall have been 
// previously obtained with a call to fgetpos. The End-of-File internal indicator of the 
// stream is cleared after a call to this function, and all effects from previous calls to 
// ungetc are dropped. On streams open for update (read+write), a call to fsetpos allows to 
// switch between reading and writing.
// 
// stream:   pFile Pointer to an HFILE object that identifies the stream.
// position: Pointer to a fpos_t object containing a position previously obtained with fgetpos.
// 
// If successful, the function returns a zero value.
// Otherwise, it returns a nonzero value and sets the global variable errno to a positive value, 
// which can be interpreted with perror.
//
int hfsetpos(HFILE* pFile, const fpos_t* position)
{
    if(pFile && position)
    {
        if(hfseek(pFile, (long)(*position), SEEK_SET) == 0)
            return 0;
    }
    else
        pFile->errorCode = -1;

    return -1;
}


// hftell
//
// Returns the current value of the position indicator of the stream.
//
// pFile: Pointer to a HFILE object that identifies the stream.
//
// On success, the current value of the position indicator is returned.
// If an error occurs, -1L is returned, and the global variable errno is set to 
// a positive value. This value can be interpreted by perror.
//
long hftell(HFILE* pFile)
{
    u32 position;
    pFile->errorCode = FileIO_Seek(pFile, 0, FILEIO_SEEK_CURRENT, &position);
    if(pFile->errorCode == FILEIO_ERROR_SUCCESS)
        return (long)(s32)position;
    return -1;
}


// hfwrite
//
// Writes an array of count elements, each one with a size of size bytes, 
// from the block of memory pointed by ptr to the current position in the stream.
// The postion indicator of the stream is advanced by the total amount of bytes written.
// The total amount of bytes written is (size * count).
// 
// ptr:    Pointer to the array of elements to be written.
// size:   Size in bytes of each element to be written.
// count:  Number of elements, each one with a size of size bytes.
// stream: Pointer to a HFILE object that specifies an output stream.
// 
// The total number of elements successfully written is returned as a size_t 
// object, which is an integral data type. If this number differs from the count 
// parameter, it indicates an error.
// 
size_t hfwrite(const void* data, size_t size, size_t count, HFILE* pFile)
{
    const size_t readSize = size * count; // To consider: check for overflow.

    pFile->errorCode = FileIO_Write(pFile, data, (u32)readSize);

    if(pFile->errorCode == FILEIO_ERROR_SUCCESS)
        return count;

    return 0;
}


// fgets
//
// Reads characters from stdin and stores them as a string into pString until a 
// newline character ('\n') or the end of the file is reached.
// The ending newline character ('\n') is not included in the string.
// A null character ('\0') is automatically appended after the last character 
// copied to str to signal the end of the C string.
// Note that gets does not behave exactly as fgets does with stdin as argument: 
//    - The ending newline character is not included with gets while with fgets it is. 
//    - gets does not let you specify a limit on how many characters are to be read, 
//      so you must be careful with the size of the array pointed by str to avoid buffer overflows.
// 
// str: Pointer to an array of chars where the C string is stored. 
// 
// On success, the function returns the same str parameter.
// If the End-of-File is encountered and no characters have been read, the contents of 
// str remain unchanged and a null pointer is returnes.
// If an error occurs, a null pointer is returned.
// Use either ferror or feof to check whether an error happened or the End-of-File was reached.
//
char* hgets(char* pString)
{
    int   c;
    char* cs = pString;

    while(((c = getchar()) != '\n') && (c != EOF))
        *cs++ = c;

    if(c == EOF && (cs == pString))
        return NULL;

    *cs++ = 0;
    return pString;
}


// hprintf
//
// Acts the same as printf to hstdout.
//
int hprintf(const char* pFormat, ...)
{
    va_list arguments;

    va_start(arguments, pFormat);
    int result = hvfprintf(hstdout, pFormat, arguments);
    va_end(arguments);

    return result;
}


// hputs
//
// Writes the C string pointed by str to stdout and appends a newline character ('\n').
// The function begins copying from the address specified (str) until it reaches the 
// terminating null character ('\0'). This final null-character is not copied to stdout.
// Using fputs(pString, stdout) instead, performs the same operation as puts(str) but 
// without appending the newline character at the end.
//
// pString: C string to be written.
// 
// On success, a non-negative value is returned.
// On error, the function returns EOF.
// 
int hputs(const char* pString)
{
    // We don't simply call fputs(hstdout, pString) here because the two functions are slightly different.

    const size_t len = strlen(pString);

    hstdout->errorCode = FileIO_Write(hstdout, pString, (u32)len);

    if(hstdout->errorCode == FILEIO_ERROR_SUCCESS)
    {
        const char newline = '\n';
        hstdout->errorCode = FileIO_Write(hstdout, &newline, 1);

        if(hstdout->errorCode == FILEIO_ERROR_SUCCESS)
            return (len + 1);
    }

    return EOF;
}


// hrewind
//
// Sets the position indicator associated with stream to the beginning of the file.
// A call to rewind is equivalent to: fseek(pFile, 0, SEEK_SET), except that unlike fseek, 
// rewind clears the error indicator. On streams open for update (read+write), a call to 
// rewind allows to switch between reading and writing.
//
void hrewind(HFILE* pFile)
{
    hfflush(pFile);
    hfseek(pFile, 0, SEEK_SET);
    hclearerr(pFile);
}


// hvfprintf
//
// Acts the same as vprintf to the given pFile.
//
int hvfprintf(HFILE* pFile, const char* pFormat, va_list arguments)
{
    int result = 0;

    if(pFormat)
    {
        char buffer[512];

        result = vsnprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), pFormat, arguments);

        if(result >= 0)
        {
            if(result < 512)
                result = hfputs(buffer, pFile);
            else
                result = -1;
        }
    }

    return result;
}


// hvprintf
//
// Acts the same as vprintf to hstdout.
//
int hvprintf(const char* pFormat, va_list arguments)
{
    return hvfprintf(hstdout, pFormat, arguments);
}


}   // namespace IO

}   // namespace EA



