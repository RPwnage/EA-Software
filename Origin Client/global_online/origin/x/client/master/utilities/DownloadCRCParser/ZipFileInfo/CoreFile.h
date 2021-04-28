//
// Copyright: Electronic Arts 2008
//
// Author: Craig Nielsen
//
// Hugely based on existing code in CoreFile and CoreFile
//
// Reason:
//	   We lost CFIle and it's variants in a move away from MFC for Core Service so I
//	   replaced these with wrappers around some open source replacements in case we
//     need to replace these due to licensing or usage issues later.
//
#pragma once

//#include "CorePlatform.h"
#include <vector>
#include <stdint.h>

#include <QFile>
#include <QString>

//namespace Core
//{

	/////////////////////////////////////////////////////////////////////
	// File/path special characters
	//
	const wchar_t			kFilePathSeparator            = '\\';
	const wchar_t* const	kFilePathSeparatorString      = L"\\";
	const wchar_t			kFilePathDot                  = '.';
	const wchar_t* const	kFilePathDotString            = L".";
	const wchar_t			kFilePathDriveSeparator       = ':';
	const wchar_t* const	kFilePathDriveSeparatorString = L":";
	const unsigned			kMaxPathLength                = 260;
	const unsigned			kMaxDriveLength               = 3;
	const unsigned			kMaxDirectoryLength           = 256;
	const unsigned			kMaxFileNameLength            = 256;
	const unsigned			kMaxDirectoryNameLength       = 256;
	const unsigned			kMaxExtensionLength           = 256;
	const char* const		kTextFileNewLineString        = "\r\n";

	/*class CoreFile
	{
	public:
		// Seek methods
		enum eSeekMethod { 
			kSeekStart,    // Seek offset from beginning of file
			kSeekCurrent,  // Seek offset from current position in file
			kSeekEnd       // Seek offset from end of file
		};

		enum eAttribute {
			kReadable  = 0x0001, //The file is readable
			kWritable  = 0x0002, //The file is writable
			kExecutable= 0x0004, //The file is executable
			kDirectory = 0x0008, //The file is a directory
			kAlias     = 0x0010, //The file is an alias (a.k.a. "symbolic link", or "shortcut")
			kArchive   = 0x0020, //The file has the archive attribute set.
			kHidden    = 0x0040, //The file is hidden. This is somewhat Win32-specific.
			kSystem    = 0x0080, //The file is a system file. This is somewhat Win32-specific.
		};

		enum eOpenMode {                                    // Open mode
			kOpenQueryAttributes = 0x0000,						// Query attributes only
			kOpenRead            = 0x0001,						// Open for reading
			kOpenWrite           = 0x0002,						// Open for writing
			kOpenReadWrite       = kOpenRead | kOpenWrite   // Open for reading and writing
		};

		enum eCreationDisposition {
			kCDCreateNew,            // Fails if file already exists
			kCDCreateAlways,         // Never fails, always creates and truncates to 0
			kCDOpenExisting,         // Fails if file doesn't exist, keeps contents
			kCDOpenAlways,           // Never fails, creates if doesn't exist, keeps contents
			kCDTruncateExisting      // Fails if file doesn't exist, truncates to 0
		};

		// Sharing mode
		enum eShareMode {         
			kShareNone   = 0x0000,    // No sharing
			kShareRead   = 0x0001,    // Allow sharing for reading
			kShareWrite  = 0x0002,    // Allow sharing for writing
			kShareDelete = 0x0004     // Allow sharing for deletion
		};

		enum eBuffering{
			kUseReadBufferingDefault  = -1,  //Pass this into 'SetBuffering' to tell it to use the default value.
			kUseWriteBufferingDefault = -1   //Pass this into 'SetBuffering' to tell it to use the default value.
		};

		enum InitialBufferSize{
			kInitialReadBufferSize  = 512, //By default, CoreFile makes a read buffer of this size. 
			kInitialWriteBufferSize = 512  //You can disable buffering by calling 'SetBuffering' with 0 values.
		};

	public:
		CoreFile(
			uint32_t ReadBufferSize = kInitialReadBufferSize, 
			uint32_t WriteBufferSize = kInitialWriteBufferSize  );

		explicit CoreFile(const char *path, 
			uint32_t ReadBufferSize = kInitialReadBufferSize, 
			uint32_t WriteBufferSize = kInitialWriteBufferSize  );

		explicit CoreFile(const wchar_t *path, 
			uint32_t ReadBufferSize = kInitialReadBufferSize, 
			uint32_t WriteBufferSize = kInitialWriteBufferSize  );

		explicit CoreFile(const QString& path, 
			uint32_t ReadBufferSize = kInitialReadBufferSize, 
			uint32_t WriteBufferSize = kInitialWriteBufferSize  );

		~CoreFile();

	public:
		// Open and close
		bool IsOpen() const {return mbOpen;}
		bool Open(const char *path, 
			int nOpenMode   = kOpenRead, 
			int nCreationMode = kCDOpenExisting, 
			int nSharingMode  = kShareRead);
		bool Open(const wchar_t *path, 
			int nOpenMode   = kOpenRead, 
			int nCreationMode = kCDOpenExisting, 
			int nSharingMode  = kShareRead);
		bool Open(const QString& path,
			int nOpenMode = kOpenRead,
			int nCreationMode = kCDOpenExisting,
			int nSharingMode  = kShareRead);
		bool Open(int nOpenMode   = kOpenRead, 
			int nCreationMode = kCDOpenExisting, 
			int nSharingMode  = kShareRead);
		bool Close();

	public:
		// Position manipulation
		int64_t Position();
		int64_t Length();
		int64_t SeekToBegin();
		int64_t SeekToEnd();

		bool SetLength(int64_t nNewLength);
		int64_t SeekToRelativePosition(int64_t nRelativePosition);
		int64_t SeekToPosition(int64_t nPosition);
		int64_t Seek(int64_t nOffset, int nSeekMethod = kSeekCurrent);

		bool	IsEOF() { return (IsOpen() ? (Position() == Length()) : true); }

		const wchar_t* GetPath() { return msPath.c_str(); }

	public:// I/O Double buffered read and write
		uint32_t Read(void* pBuffer, uint32_t nBytes);
		bool ReadWithCount(void *pBuffer, uint32_t& nBytes);
		bool Write(const void *pBuffer, uint32_t nBytes);   //Fails if actual written != numbytes
		bool WriteWithCount(const void *pBuffer, uint32_t& nBytes);
		bool Flush();

		// CStdioFile replacements
		BOOL WriteString(QString& rString);
		BOOL ReadString(QString& rString);

	public:// I/O Single buffered write
		bool ResizeWriteBuffer2(uint32_t nBytes);
		void *WriteBufferPointer2();
		bool UpdateWriteBufferSize2(uint32_t nBytes);

	public: // Other
		bool Exists();
		bool Remove();
		bool Rename( const char *newPath );
		bool Rename( const wchar_t *newPath );

	public:
		// Static functions 
		static bool Remove( const char *path );
		static bool Remove( const wchar_t *path );
		static bool Remove( const QString& path );
		static bool Rename( const char *oldPath, const char *newPath );
		static bool Rename( const wchar_t *oldPath, const wchar_t *newPath );

	protected:
		void SaveLastError();
		void ClearReadBuffer();
		bool FillReadBuffer();
		int RawRead(void *pData, int nBytes);
		void ClearWriteBuffer();
		bool FlushWriteBuffer();
		bool FillWriteBuffer(uint8_t* pData, int nSize);
		bool ValidInternalPosition();
		void SetCurrentPositionInternal();

	protected:
		enum Error{
			kErrorNone              = (int)0x20000000,
			kErrorFileAlreadyExists = (int)0x20000001,
			kErrorAccessDenied      = (int)0x20000002
		};

		//Data
		std::wstring	msPath;          // Full path to file
		bool			mbOpen;           // Whether or not the file is open
		void*			mhFile;           // File handle
		int				mnOpenMode;
		int				mnCreationMode;
		int				mnSharingMode;
		uint32_t		mReferenceCount; 
		uint32_t		mnLastError;

		//Stuff related to buffering
		int64_t			mnCurrentPositionExternal;    //This is the position of the the file pointer as the the user sees it. It is where the next byte read or write will come from or go to.
		int64_t			mnCurrentPositionInternal;    //This is the position of the the file pointer as the OS sees it.
		int				mnReadBufferSize;             //This is the size of the read buffer available for our usage.
		std::vector<uint8_t> mReadBuffer;            //This is the read buffer.
		int64_t			mnReadBufferStartPosition;    //This is where in the file the beginning the read buffer corresponds to. 
		int				mnReadBufferLength;           //This is the count of bytes in the read buffer that are valid.
		int				mnWriteBufferSize;            //This is the size of the write buffer available for our usage.
		std::vector<uint8_t> mWriteBuffer;           //This is the write buffer.
		int64_t			mnWriteBufferStartPosition;   //This is where in the file the beginning the write buffer corresponds to.
		int				mnWriteBufferLength;          //This is the count of bytes in the write buffer that are valid.
	};

	CoreFile *CreateCoreFile(const char *path);
	CoreFile *CreateCoreFile(const wchar_t *path);

	*/

	class CoreFile : public QFile
	{
		public:

			static bool moveDirectory(const QString& newLocation, const QString& oldLocation);

			enum eFilePosition { 
					kStart,    // beginning of file
					kCurrent,  // current position in file
					kEnd       // end of file
			};

			CoreFile(const QString& name) : QFile(name) {}
			CoreFile(QObject* parent) : QFile(parent) {}
			CoreFile(const QString& name, QObject* parent) : QFile(name, parent) {}

			bool seek(qint64 numBytes, enum eFilePosition referencePosition = kStart);
			bool readString(QString& rString);
	};

//}
