/////////////////////////////////////////////////////////////////////////////
// MCS.h
//
// Copyright (c) 2007 Criterion Software Limited and its licensors. All Rights Reserved.
// Created by Paul Pedriana
/////////////////////////////////////////////////////////////////////////////


#ifndef EAIO_WII_MCSFILE_H
#define EAIO_WII_MCSFILE_H


#include <EAIO/Wii/MCS.h>
#include <stdio.h>
#include <stdarg.h>


namespace EA
{
    namespace IO
    {
        namespace detail
        {
            const int FILEIO_COMMAND_SIZE_MAX = 2 * 1024;
            const int FILEIO_CHUNK_SIZE_MAX   = 2 * 1024;

            struct FileIOWork
            {
                u8 tempBuf[FILEIO_COMMAND_SIZE_MAX];
                u8 commBuf[FILEIO_CHUNK_SIZE_MAX * 32];
            };
        } 

        const int FILEIO_WORKBUFFER_SIZE = sizeof(detail::FileIOWork);
        const int FILEIO_PATH_MAX        = 260;  // Standard Windows max path length



        ///////////////////////////////////////////////////////////////////////
        // McsFileSystem
        //
        // This is the high level file system manager. It initializes and 
        // shuts down the file system. Unless you are directly using the 
        // low level functions (e.g. FileIO_Open) the only thing you need
        // from this module is this class. This class takes care of 
        // Mcs_Init, FileIO_Init, Mcs_CreateDeviceObject, FileIO_RegisterBuffer.
        //
        // The MCS file system is simply a remote Windows file system.
        // The file paths are Windows file paths, including 
        ///////////////////////////////////////////////////////////////////////

        class McsFileSystem
        {
        public:
            McsFileSystem();
           ~McsFileSystem();

            bool Init(const char* pWindowsRoot = NULL);
            void Shutdown();

        protected:
            uint64_t mWorkBuffer        [(FILEIO_WORKBUFFER_SIZE + (sizeof(uint64_t) - 1)) / sizeof(uint64_t)];
            uint64_t mDeviceObjectBuffer[(DEVICETYPE_NDEV_SIZE   + (sizeof(uint64_t) - 1)) / sizeof(uint64_t)];
            bool     mbInitialized;
        };


        ///////////////////////////////////////////////////////////////////////
        // Enumerations / data structs
        ///////////////////////////////////////////////////////////////////////

        enum FileIOError
        {
            FILEIO_ERROR_SUCCESS = 0,
            FILEIO_ERROR_COMERROR,
            FILEIO_ERROR_NOTCONNECT,

            FILEIO_ERROR_CONTINUE = 100,
            FILEIO_ERROR_PROTOCOLERROR,             // Protocol error due to version differences
            FILEIO_ERROR_FILENOTFOUND,
            FILEIO_ERROR_DIRECTORYNOTFOUND,
            FILEIO_ERROR_FILEALREADYEXIST,
            FILEIO_ERROR_SECURITYERROR,
            FILEIO_ERROR_UNAUTHORIZEDACCESS,
            FILEIO_ERROR_IOERROR,                   // Unclassified IO-related error
            FILEIO_ERROR_INVALIDFILEHANDLE,
            FILEIO_ERROR_NOMOREFILES,
            FILEIO_ERROR_CANCELED,
            
            FILEIO_ERROR_LAST = FILEIO_ERROR_CANCELED
        };

        enum FileIOFlags
        {
            FILEIO_FLAG_READ        =      1,
            FILEIO_FLAG_WRITE       =      2,
            FILEIO_FLAG_READWRITE   =      3,

            FILEIO_FLAG_NOOVERWRITE =      4,
            FILEIO_FLAG_NOCREATE    =      8,
            FILEIO_FLAG_INCENVVAR   =     16,   // It is undocumented what the MCS server means by this. Nintendo states: "Expand the PC's environment variables character string."
            FILEIO_FLAG_CREATEDIR   =     32,   // Create the directory for newly opened/created files if necessary.

            FILEIO_FLAG_SHAREREAD   =  65536,
            FILEIO_FLAG_SHAREWRITE  = 131072
        };

        enum FileIOSeek
        {
            FILEIO_SEEK_BEGIN,
            FILEIO_SEEK_CURRENT,
            FILEIO_SEEK_END
        };

        enum FileIOAttribute
        {
            FILEIO_ATTRIBUTE_READONLY             = 0x00000001,
            FILEIO_ATTRIBUTE_HIDDEN               = 0x00000002,
            FILEIO_ATTRIBUTE_SYSTEM               = 0x00000004,
            FILEIO_ATTRIBUTE_DIRECTORY            = 0x00000010,
            FILEIO_ATTRIBUTE_ARCHIVE              = 0x00000020,
            FILEIO_ATTRIBUTE_DEVICE               = 0x00000040,
            FILEIO_ATTRIBUTE_NORMAL               = 0x00000080,
            FILEIO_ATTRIBUTE_TEMPORARY            = 0x00000100,
            FILEIO_ATTRIBUTE_SPARSE_FILE          = 0x00000200,
            FILEIO_ATTRIBUTE_REPARSE_POINT        = 0x00000400,
            FILEIO_ATTRIBUTE_COMPRESSED           = 0x00000800,
            FILEIO_ATTRIBUTE_OFFLINE              = 0x00001000,
            FILEIO_ATTRIBUTE_NOT_CONTENT_INDEXED  = 0x00002000,
            FILEIO_ATTRIBUTE_ENCRYPTED            = 0x00004000
        };

        enum FileIOFileDialog
        {
            FILEIO_FILEDIALOG_OPEN                = 0,
            FILEIO_FILEDIALOG_SAVE                = 0x40000000,
            FILEIO_FILEDIALOG_NOOVERWRITEPROMPT   = 0x00000002,
            FILEIO_FILEDIALOG_CREATEPROMPT        = 0x00002000
        };

        struct FileInfo
        {
            u32  handle;
            u32  fileSize;
            u32  errorCode;
        };

        struct FindData
        {
            u32  attribute;
            u32  size;
            u32  date;
            u32  reserved1;
            u32  reserved2;
            char name[FILEIO_PATH_MAX];
        };


        ///////////////////////////////////////////////////////////////////////
        // FileIO
        ///////////////////////////////////////////////////////////////////////

        void FileIO_Init();
        void FileIO_SetWindowsRoot(const char* pDirectoryPath);
        void FileIO_RegisterBuffer(void* tempBuf);
        void FileIO_UnregisterBuffer();
        u32  FileIO_Open(FileInfo* pFileInfo, const char* fileName, u32 openFlag);
        u32  FileIO_Close(FileInfo* pFileInfo);
        u32  FileIO_Read(FileInfo* pFileInfo, void* buffer, u32 length, u32* pReadBytes);
        u32  FileIO_Write(FileInfo* pFileInfo, const void* buffer, u32 length);
        u32  FileIO_Seek(FileInfo* pFileInfo, s32 distanceToMove, u32 moveMethod, u32* pNewFilePointer = 0);
        u32  FileIO_GetOpenFileSize(const FileInfo* pFileInfo);
        u32  FileIO_FindFirst(FileInfo* pFileInfo, FindData* pFindData, const char* pattern);
        u32  FileIO_FindNext(FileInfo*  pFileInfo, FindData* pFindData);
        u32  FileIO_FindClose(FileInfo* pFileInfo);
        u32  FileIO_ShowFileDialog(char* fileNameBuf, u32 fileNameBufSize, u32 flag, const char* initialDirectory = 0, 
                                    const char* filter = 0, const char* defaultExt = 0, const char* title = 0);


        ///////////////////////////////////////////////////////////////////////
        // Standard C-style file IO
        ///////////////////////////////////////////////////////////////////////

        struct HFILE;
        extern HFILE* hstdin;
        extern HFILE* hstdout;
        extern HFILE* hstderr;

        bool    hfinit();
        void    hfshutdown();
        HFILE*  hfalloc();
        void    hffree(HFILE*);
        void    hclearerr(HFILE*);
        int     hfclose(HFILE*);
        int     hfeof(HFILE*);
        int     hferror(HFILE*);
        int     hfflush(HFILE*);
        int     hfgetc(HFILE*);
        int     hfgetpos(HFILE*, fpos_t* position);
        char*   hfgets(char* pString, int, HFILE*);
        HFILE*  hffopen(HFILE*, const char* pFilePath, const char* mode);
        HFILE*  hfopen(const char* pFilePath, const char* mode);
        int     hfileno(HFILE*);
        int     hfprintf(HFILE*, const char* pFormat, ...);
        int     hfputc(int, HFILE*);
        int     hfputs(const char* pString, HFILE*);
        size_t  hfread(void* data, size_t size, size_t count, HFILE*);
      //HFILE*  hfreopen(const char* pFilePath, const char* mode, HFILE*);
      //int     hfscanf(HFILE*, const char* pFormat, ...);
        int     hfseek(HFILE*, long offset, int origin);
        int     hfsetpos(HFILE*, const fpos_t* position);
        long    hftell(HFILE*);
        size_t  hfwrite(const void* data, size_t size, size_t count, HFILE*);
        char*   hgets(char* pString);
      //void    hperror(const char* pString);
        int     hprintf(const char* pFormat, ...);
        int     hputs(const char* pString);
      //int     hremove(const char* pFilePath);
      //int     hrename(const char* pFileNameOld, const char* pFileNameNew);
        void    hrewind(HFILE*);
      //int     hscanf(const char* pFormat, ...);
      //void    hsetbuf(HFILE*, char* buffer);
      //int     hsetvbuf(HFILE*, char* buffer, int mode, size_t size);
      //HFILE*  htmpfile();
      //char*   htmpnam(char* pFilePath);
      //int     hungetc(int character, HFILE*);
        int     hvfprintf(HFILE*, const char* pFormat, va_list);
        int     hvprintf(const char* pFormat, va_list);


        ///////////////////////////////////////////////////////////////////////
        // implementation detail
        ///////////////////////////////////////////////////////////////////////

        namespace detail
        {
            const ChannelType FILEIO_CHANNEL = 0xFF7F;

            enum
            {
                FILEIO_COMMAND_FILEOPEN  = 0,
                FILEIO_COMMAND_FILECLOSE,
                FILEIO_COMMAND_FILEREAD,
                FILEIO_COMMAND_FILEWRITE,
                FILEIO_COMMAND_FILESEEK,

                FILEIO_COMMAND_FINDFIRST = 16,
                FILEIO_COMMAND_FINDNEXT,
                FILEIO_COMMAND_FINDCLOSE,

                FILEIO_COMMAND_SHOWFILEDLG = 32
            };

            typedef void*     VoidPtr;
            typedef FileInfo* FileInfoPtr;
            typedef FindData* FindDataPtr;


            ///////////////////////////////////////////////////////////////////
            // Commands
            ///////////////////////////////////////////////////////////////////

            struct FileIOChunkHeader
            {
                u32 chunkID;
                u32 chunkSize;
            };

            struct FileIOCommand : FileIOChunkHeader
            {
                FileInfoPtr pFileInfo;
                u32         handle;
            };

            struct FileIOOpenCommand : FileIOChunkHeader
            {
                FileInfoPtr pFileInfo;
                u32         flag;
                u32         mode;                       // The meaning of this is unknown, as we don't have source code for the MCS server.
                char16_t    fileName[FILEIO_PATH_MAX];  // This is char16_t as per the Nintendo sample code, but it appears that the MCS server needs this to be char8_t
            };

            struct FileIOReadCommand : FileIOChunkHeader
            {
                FileInfoPtr pFileInfo;
                u32         handle;
                VoidPtr     pBuffer;
                u32         size;
            };

            struct FileIOWriteCommand : FileIOChunkHeader
            {
                FileInfoPtr pFileInfo;
                u32         handle;
                u32         size;
            };

            struct FileIOSeekCommand : FileIOChunkHeader
            {
                FileInfoPtr pFileInfo;
                u32         handle;
                s32         distanceToMove;
                u32         moveMethod;
            };

            struct FileIOFindFirstCommand : FileIOChunkHeader
            {
                FileInfoPtr pFileInfo;
                FindDataPtr pFindData;
                u32         mode;                       // The meaning of this is unknown, as we don't have source code for the MCS server.
                char16_t    fileName[FILEIO_PATH_MAX];  // This is char16_t as per the Nintendo sample code, but it appears that the MCS server needs this to be char8_t
            };

            struct FileIOFindNextCommand : FileIOChunkHeader
            {
                FileInfoPtr pFileInfo;
                u32         handle;
                FindDataPtr pFindData;
            };

            struct FileIOShowFileDlgCommand : FileIOChunkHeader
            {
                u32      flag;
                u32      mode;                        // The meaning of this is unknown, as we don't have source code for the MCS server.
                u32      reserved;
                char16_t initialDir[FILEIO_PATH_MAX]; // This is char16_t as per the Nintendo sample code, but it appears that the MCS server needs this to be char8_t
                char16_t filter[256];                 // Ditto
                char16_t defaultExt[16];              // Ditto
                char16_t title[256];                  // Ditto
            };


            ///////////////////////////////////////////////////////////////////
            // Responses
            ///////////////////////////////////////////////////////////////////

            struct FileIOResponse
            {
                u32         errorCode;
                FileInfoPtr pFileInfo;
            };

            struct FileIOOpenResponse
            {
                u32         errorCode;
                FileInfoPtr pFileInfo;
                u32         handle;
                u32         fileSize;
            };

            struct FileIOReadResponse
            {
                u32         errorCode;
                FileInfoPtr pFileInfo;
                VoidPtr     pBuffer;
                u32         size;
                u32         totalSize;
            };

            struct FileIOSeekResponse
            {
                u32         errorCode;
                FileInfoPtr pFileInfo;
                u32         filePointer;
            };

            struct FileIOFindResponse
            {
                u32         errorCode;
                FileInfoPtr pFileInfo;
                FindDataPtr pFindData;
                u32         handle;
                u32         fileSize;
                u32         fileAttribute;
                u32         fileDate;
                u32         reserved1;
                u32         reserved2;
                char16_t    fileName[FILEIO_PATH_MAX];
            };

            struct FileIOShowFileDlgResponse
            {
                u32 errorCode;
                u32 size;
            };

        }   // namespace detail

    }   // namespace IO

}   // namespace EA


#endif // Header include guard
