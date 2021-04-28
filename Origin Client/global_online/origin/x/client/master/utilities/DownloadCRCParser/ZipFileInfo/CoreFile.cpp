//
// Copyright Electronic Arts 2008
//
// Author: Craig Nielsen
//
#include "CoreFile.h"
#include <io.h>
#include <sys/types.h> // usage of stat
#include <sys/stat.h>  // usage of stat
#include "patch_util.h"
#ifdef EADM
#include "Logger.h"
#else
#define CoreLogDebug	
#define CoreLogWarning	
#define CoreLogError	
#endif

#include <QDir>
#include <QRegExp>

//namespace Core
//{
	// A few define to clean up code
#define DECLARE_LIZERO LARGE_INTEGER liZERO; liZERO.QuadPart = 0
#define DECLARE_LIPOS  LARGE_INTEGER liPos; liPos.QuadPart = 0

	///////////////////////////////////////////////////////////////////////////////
	// MIN/MAX
	//
	// We define these because the standard C++ or STL definitions of min/max are
	// virtually useless, since they generate compiler errors for the lamest reasons.
	//
	#define LOCAL_MIN(a,b)            (((a) < (b)) ? (a) : (b))
	#define LOCAL_MAX(a,b)            (((a) > (b)) ? (a) : (b))
	///////////////////////////////////////////////////////////////////////////////
/*
	CoreFile *CreateCoreFile(const char *path)
	{
		return new CoreFile(path);
	}

	CoreFile *CreateCoreFile(const wchar_t *path)
	{
		return new CoreFile(path);
	}

	///////////////////////////////////////////////////////////////////////////////
	// CoreFile::CoreFile
	//
	CoreFile::CoreFile(uint32_t ReadBufferSize, uint32_t WriteBufferSize  )
		:  mbOpen(false),
		mhFile(INVALID_HANDLE_VALUE),
		mnOpenMode(0),
		mnCreationMode(0),
		mnSharingMode(0),
		mReferenceCount(0),
		mnLastError(kErrorNone),
		//Stuff related to buffering
		mnCurrentPositionExternal(0),
		mnCurrentPositionInternal(0),
		mnReadBufferSize(ReadBufferSize),
		mnReadBufferStartPosition(0),
		mnReadBufferLength(0),
		mnWriteBufferSize(WriteBufferSize),
		mnWriteBufferStartPosition(0),
		mnWriteBufferLength(0),
		msPath(L"")
	{
	}

	CoreFile::CoreFile(const char *path, uint32_t ReadBufferSize, uint32_t WriteBufferSize  )
			:  mbOpen(false),
			mhFile(INVALID_HANDLE_VALUE),
			mnOpenMode(0),
			mnCreationMode(0),
			mnSharingMode(0),
			mReferenceCount(0),
			mnLastError(kErrorNone),
			//Stuff related to buffering
			mnCurrentPositionExternal(0),
			mnCurrentPositionInternal(0),
			mnReadBufferSize(ReadBufferSize),
			mnReadBufferStartPosition(0),
			mnReadBufferLength(0),
			mnWriteBufferSize(WriteBufferSize),
			mnWriteBufferStartPosition(0),
			mnWriteBufferLength(0)
	{
		msPath = Util::UTF8ToWChar(path);
	}



	CoreFile::CoreFile(const wchar_t *path, uint32_t ReadBufferSize, uint32_t WriteBufferSize  )
		:  mbOpen(false),
		mhFile(INVALID_HANDLE_VALUE),
		mnOpenMode(0),
		mnCreationMode(0),
		mnSharingMode(0),
		mReferenceCount(0),
		mnLastError(kErrorNone),

		//Stuff related to buffering
		mnCurrentPositionExternal(0),
		mnCurrentPositionInternal(0),
		mnReadBufferSize(ReadBufferSize),
		mnReadBufferStartPosition(0),
		mnReadBufferLength(0),
		mnWriteBufferSize(WriteBufferSize),
		mnWriteBufferStartPosition(0),
		mnWriteBufferLength(0)
	{
		msPath = path;
	}

	CoreFile::CoreFile(const QString& path, uint32_t ReadBufferSize, uint32_t WriteBufferSize  )
		:  mbOpen(false),
		mhFile(INVALID_HANDLE_VALUE),
		mnOpenMode(0),
		mnCreationMode(0),
		mnSharingMode(0),
		mReferenceCount(0),
		mnLastError(kErrorNone),

		//Stuff related to buffering
		mnCurrentPositionExternal(0),
		mnCurrentPositionInternal(0),
		mnReadBufferSize(ReadBufferSize),
		mnReadBufferStartPosition(0),
		mnReadBufferLength(0),
		mnWriteBufferSize(WriteBufferSize),
		mnWriteBufferStartPosition(0),
		mnWriteBufferLength(0)
	{
		msPath = path.utf16();
	}

	///////////////////////////////////////////////////////////////////////////////
	// CoreFile::~CoreFile
	//
	CoreFile::~CoreFile()
	{
		if(IsOpen())
			Close();
	}


	///////////////////////////////////////////////////////////////////////////////
	// CoreFile::Open
	//
	bool CoreFile::Open(const char *path, int nOpenMode, int nCreationMode, int nSharingMode)
	{
		msPath = Util::UTF8ToWChar(path);
		return Open(nOpenMode, nCreationMode, nSharingMode);
	}
	bool CoreFile::Open(const wchar_t *path, int nOpenMode, int nCreationMode, int nSharingMode)
	{
		msPath = path;
		return Open(nOpenMode, nCreationMode, nSharingMode);
	}
	bool CoreFile::Open(const QString& path, int nOpenMode, int nCreationMode, int nSharingMode)
	{
		msPath = path.utf16();
		return Open(nOpenMode, nCreationMode, nSharingMode);
	}

	bool CoreFile::Open(int nOpenMode, int nCreationMode, int nSharingMode)
	{
		if(!IsOpen())
		{
			DWORD nAccess(0);
			DWORD nShare(0);
			DWORD nCreate(0);

			if(nOpenMode & kOpenRead)
				nAccess |= GENERIC_READ;
			if(nOpenMode & kOpenWrite)
				nAccess |= GENERIC_WRITE;
			if(nSharingMode & kShareRead)
				nShare |= FILE_SHARE_READ;
			if(nSharingMode & kShareWrite)
			{
				nShare |= FILE_SHARE_READ;
				nShare |= FILE_SHARE_WRITE;
			}

			switch(nCreationMode)
			{
			case kCDCreateNew :
				nCreate = CREATE_NEW;
				break;
			case kCDCreateAlways :
				nCreate = CREATE_ALWAYS;
				break;
			case kCDOpenExisting :
				nCreate = OPEN_EXISTING;
				break;
			case kCDOpenAlways :
				nCreate = OPEN_ALWAYS;
				break;
			case kCDTruncateExisting :
				nCreate = TRUNCATE_EXISTING;
				break;
			default:
				CORE_ASSERT(false);
				return false;
			}


			// Open the file, allowing foreign characters in the path
			// Use native Unicode as the filename
			mhFile = ::CreateFileW(msPath.c_str(), nAccess, nShare, NULL, nCreate, 0, NULL);

			if(mhFile != INVALID_HANDLE_VALUE)
			{
				mbOpen = true;

				// From Win32's CreateFile() documentation:
				//    "If the specified file exists before the function call and
				//     dwCreationDisposition is CREATE_ALWAYS or OPEN_ALWAYS,
				//     a call to GetLastError returns ERROR_ALREADY_EXISTS
				//     (even though the function has succeeded)."
				// So we'll save this error code if it applies.
				if((nCreate == CREATE_ALWAYS) || (nCreate == OPEN_ALWAYS))
					SaveLastError();

				//Set these values for query while the file is open.
				mnOpenMode     = nOpenMode;
				mnCreationMode = nCreationMode;
				mnSharingMode  = nSharingMode;

				// Set the internal position thru win32 call and set external position
				SetCurrentPositionInternal();
				mnCurrentPositionExternal = mnCurrentPositionInternal;

				ClearReadBuffer();
				ClearWriteBuffer();

				return true;
			}

			SaveLastError();
			return false;
		}
		return true;
	}



	///////////////////////////////////////////////////////////////////////////////
	// CoreFile::Close
	//
	bool CoreFile::Close()
	{
		if(mbOpen)
		{
			//Possibly flush the write buffer.
			FlushWriteBuffer();
			ClearReadBuffer();
			ClearWriteBuffer();

			if(::CloseHandle(mhFile) == 0)
				SaveLastError();

			mnOpenMode     = 0;
			mnCreationMode = 0;
			mnSharingMode  = 0;

			mnCurrentPositionInternal = 0;

			mhFile = INVALID_HANDLE_VALUE;
			mbOpen = false;

			return true;
		}

#ifdef EADM
		EBILOGERROR << L"CoreFile::Close(): File isn't open.\n";
#endif
		return false;
	}

	///////////////////////////////////////////////////////////////////////////////
	// CoreFile::SaveLastError
	//
	// This is an internal function.
	//
	// In order to provide access to error codes/messages returned by the
	// OS, this method should be called by every CoreFile method that calls
	// an OS API method and can return "false"(indicating failure of the
	// method), so that the OS's error code and message are saved in data
	// members.
	//
	void CoreFile::SaveLastError()
	{
		DWORD lastError = ::GetLastError();

		// Translate Win32 API error numbers to CoreFile error enums
		switch(lastError)
		{
		case ERROR_ALREADY_EXISTS:  // 183: "Cannot create a file when that file already exists."
			mnLastError = kErrorFileAlreadyExists;
			break;

		case ERROR_ACCESS_DENIED:   //   5: "Access is denied."
			mnLastError = kErrorAccessDenied;
			break;

		default:
			mnLastError = lastError;
			break;
		}
	}

	///////////////////////////////////////////////////////////////////////////////
	// CoreFile::FillReadBuffer
	//
	// This is an internal function.
	//
	// This function erases anything that is in the read buffer and fills it
	// completely with data from the current actual file position.
	//
	bool CoreFile::FillReadBuffer(){
		bool bReturnValue(false);

		//Here we test to see if the read buffer hasn't been allocated yet.
		//It will generally only be non-allocated the first time we get here.
		if(mReadBuffer.empty())
			mReadBuffer.resize(mnReadBufferSize);

		mnReadBufferStartPosition  = mnCurrentPositionInternal;

		int nBytes = RawRead(&mReadBuffer[0], (int)mnReadBufferSize);

		if(nBytes >= 0){
			mnReadBufferLength         = nBytes;
			bReturnValue               = true;
		}
		else{
			mnReadBufferStartPosition = 0;
			mnReadBufferLength        = 0;
		}

		return bReturnValue;
	}


	///////////////////////////////////////////////////////////////////////////////
	// CoreFile::RawRead
	//
	// Performs a raw read from the underlying file handle.
	//
	int CoreFile::RawRead(void *pBuffer, int nBytes){
		const BOOL bResult = ::ReadFile(mhFile, pBuffer, nBytes, (DWORD*)&nBytes, NULL);
		if(bResult){   // success
			mnCurrentPositionInternal += nBytes;
			return nBytes;
		}
		else{          // failure
			SetCurrentPositionInternal();
			return -1;
		}
	}

	///////////////////////////////////////////////////////////////////////////////
	// CoreFile::ClearReadBuffer
	//
	// This is an internal function.
	//
	// This function does not resize the read buffer to zero but rather simply
	// sets it to be empty.
	//
	void CoreFile::ClearReadBuffer()
	{
		mnReadBufferStartPosition = 0;
		mnReadBufferLength        = 0;
	}


	///////////////////////////////////////////////////////////////////////////////
	// CoreFile::ClearWriteBuffer
	//
	// This is an internal function.
	//
	// This function does not resize the buffer to zero but rather simply
	// sets it to be empty.
	//
	void CoreFile::ClearWriteBuffer()
	{
		mnWriteBufferStartPosition = 0;
		mnWriteBufferLength        = 0;
	}

	///////////////////////////////////////////////////////////////////////////////
	// CoreFile::FlushWriteBuffer
	//
	// This is an internal function.
	//
	// Returns true if the buffer could be flushed OK. The return value of this
	// function is much like the return value of our member Write function.
	//
	bool CoreFile::FlushWriteBuffer()
	{
		bool bReturnValue(true);

		//We leave 'mnCurrentPositionExternal' alone. However, upon exit, the internal
		//and external positions should (usually) be equal. Actually, there is one case
		//where they won't be equal and that is when the Write function calls FillWriteBuffer
		//and the buffer needs to be flushed before returning from FillWriteBuffer. In that
		//case the Write function won't update the 'mnCurrentPositionExternal' variable
		//until after the FillWriteBuffer function returns. Then things will become
		//aligned properly.

		if(mnWriteBufferLength) //If there is anything to write...
		{ 
			uint32_t nBytes(mnWriteBufferLength);
			const BOOL bResult = ::WriteFile(mhFile, &mWriteBuffer[0], nBytes, (DWORD*)&nBytes, NULL);
			if(bResult)
			{
				mnCurrentPositionInternal += nBytes;
				mnWriteBufferStartPosition = mnCurrentPositionInternal;
				mnWriteBufferLength        = 0;
			}
			else
			{					
				//Else we have a severe problem.
				SetCurrentPositionInternal();
				mnWriteBufferStartPosition = mnCurrentPositionInternal;
				mnWriteBufferLength        = 0;
				bReturnValue               = false;
			}

		}

		return bReturnValue;
	}

	///////////////////////////////////////////////////////////////////////////////
	// CoreFile::Exists
	//
	bool CoreFile::Exists(){
		struct _stat tempStat;

		const int result = ::_wstat(msPath.c_str(), &tempStat);
		return(result == 0);
	}

	///////////////////////////////////////////////////////////////////////////////
	// CoreFile::Position
	//
	int64_t CoreFile::Position()
	{
		int64_t nReturnValue = -1;

		if(mbOpen)
		{
			nReturnValue = mnCurrentPositionExternal; //::tell(*((int*)mhFile)); We don't call 'tell' because we want to return the position as the user sees it.
			if(nReturnValue == -1)
			{
				mnLastError  = errno;
			}
		}
		else
		{
#ifdef EADM
			EBILOGWARNING << L"CoreFile::Position(): File isn't open.\n";
#endif
		}

		return nReturnValue;
	}

	///////////////////////////////////////////////////////////////////////////////
	// CoreFile::Length
	//
	int64_t CoreFile::Length(){
		int64_t nReturnValue = -1;

		if(mbOpen){
			DECLARE_LIPOS;

			if (!::GetFileSizeEx( mhFile, &liPos ))
			{
				SaveLastError();
			}
			else{
				nReturnValue = liPos.QuadPart;

				if(mnWriteBufferLength){ //If there is any write buffer...
					//If the write buffer extends beyond the current
					//file end, adjust the reported file end.
					if(nReturnValue < mnCurrentPositionExternal)
						nReturnValue = mnCurrentPositionExternal;
				}
			}
		}
		else
		{
#ifdef EADM
			EBILOGERROR << L"CoreFile::Length(): File isn't open.\n";
#endif
		}

		return nReturnValue;
	}

	///////////////////////////////////////////////////////////////////////////////
	// CoreFile::SetLength
	//
	bool CoreFile::SetLength(int64_t nNewLength){
		bool bReturnValue(false);

		if(mbOpen){
			DECLARE_LIPOS;

			LARGE_INTEGER liDistanceToMove;
			liDistanceToMove.QuadPart = nNewLength;

			//Possibly clear/flush buffers.
			ClearReadBuffer();
			FlushWriteBuffer(); //What do we do if this fails?

			//Under Win32, you set the length of a file by calling SetFilePointer
			//to set the pointer to the given position, then SetEndOFFile (or Write)
			//to cement the action.
			if (!::SetFilePointerEx(mhFile, liDistanceToMove, NULL, FILE_BEGIN))
			{
				SaveLastError();
#ifdef EADM
				EBILOGERROR << L"CoreFile::SetLength(): failure.\n";
#endif
				bReturnValue = false;
			}
			else{
				if (!::SetEndOfFile(mhFile))
				{
					SaveLastError();
#ifdef EADM
					EBILOGERROR << L"CoreFile::SetLength(): failure.\n";
#endif
					bReturnValue = false;
				}
				else
					bReturnValue = true;
			}

			SetCurrentPositionInternal();
			mnCurrentPositionExternal = mnCurrentPositionInternal;
		}
		else
		{
#ifdef EADM
			EBILOGERROR << L"CoreFile::SetLength(): File isn't open.\n";
#endif
		}

		return bReturnValue;
	}

	///////////////////////////////////////////////////////////////////////////////
	// CoreFile::SeekToBegin
	//
	int64_t CoreFile::SeekToBegin(){
		return Seek(0, kSeekStart);
	}

	///////////////////////////////////////////////////////////////////////////////
	// CoreFile::SeekToEnd
	//
	int64_t CoreFile::SeekToEnd(){
		return Seek(0, kSeekEnd);
	}

	///////////////////////////////////////////////////////////////////////////////
	// CoreFile::SeekToPosition
	//
	int64_t CoreFile::SeekToRelativePosition(int64_t nRelativePosition){
		return Seek(nRelativePosition, kSeekCurrent);
	}

	///////////////////////////////////////////////////////////////////////////////
	// CoreFile::SeekToPosition
	//
	int64_t CoreFile::SeekToPosition(int64_t nPosition){
		return Seek(nPosition, kSeekStart);
	}

	//////////////////////////////////////////////////////////////////////////////
	// CoreFile::Seek
	//
	int64_t CoreFile::Seek(int64_t nOffset, int nSeekMethod)
	{
		if(mbOpen)
		{
			//DECLARE_LIZERO;
			DWORD nMethod;

			switch(nSeekMethod)
			{
			case kSeekStart:
				nMethod = FILE_BEGIN;
				break;
			case kSeekCurrent:
				nMethod = FILE_CURRENT;
				if(mnCurrentPositionInternal != mnCurrentPositionExternal)
				{ //If we have buffering in place and the user does a relative
					nMethod = FILE_BEGIN;                                    //seek, the user wants to seek relative to where the user thinks
					nOffset = mnCurrentPositionExternal + nOffset;           //the file pointer is (mnCurrentPositionInternal) not where
				}                                                           //the OS thinks it is (mnCurrentPositionExternal).
				break;
			case kSeekEnd:
				nMethod = FILE_END;
				break;
			default :
				CORE_ASSERT(false); // "CoreFile::Seek(): Invalid argument"
				return Position();
			}

			//Possibly flush the write buffer. There's no reason to clear the read buffer.
			FlushWriteBuffer();

			//Do the actual seek.
			int64_t nReturnValue = -1;		
			DECLARE_LIPOS;
			LARGE_INTEGER liOffset; liOffset.QuadPart = nOffset;

			if ( !::SetFilePointerEx(mhFile, liOffset, &liPos, nMethod) )
			{
				SetCurrentPositionInternal();
				mnCurrentPositionExternal = mnCurrentPositionInternal;
				SaveLastError();
			}
			else
			{
				nReturnValue = liPos.QuadPart;
				mnCurrentPositionInternal = nReturnValue;
				mnCurrentPositionExternal = mnCurrentPositionInternal;
			}
			return nReturnValue;
		}
		else
		{
#ifdef EADM
			EBILOGERROR << L"CoreFile::Seek(): File isn't open.\n";
#endif
		}

		return -1;
	}


	///////////////////////////////////////////////////////////////////////////////
	// CoreFile::Read
	//
	uint32_t CoreFile::Read(void* pBuffer, uint32_t nBytes)
	{
		uint32_t nBytesActual(nBytes);
		ReadWithCount(pBuffer, nBytesActual);
		return nBytesActual;
	}



	///////////////////////////////////////////////////////////////////////////////
	// CoreFile::ReadWithCount
	//
	bool CoreFile::ReadWithCount(void* pBuffer, uint32_t& nBytes)
	{
		if(mbOpen)
		{
			if(nBytes)
			{
				//Possibly flush the write buffer.
				if(mnWriteBufferLength)
					FlushWriteBuffer();

				//Possibly get data from the read buffer.
				if(mnReadBufferSize)
				{	//If buffering is enabled...
					uint8_t* pBuffer8 = (uint8_t*)pBuffer;
					bool   bReadStartIsWithinCache(false);
					uint32_t nBytesRemaining(nBytes);
					bool   bResult(true);

					if(mnCurrentPositionExternal >= mnReadBufferStartPosition &&
						mnCurrentPositionExternal < mnReadBufferStartPosition+mnReadBufferLength)
					{
						bReadStartIsWithinCache = true;
					}

					if(bReadStartIsWithinCache)
					{
						// intentionally using int because we are only doing int size reads...
						const int nOffsetWithinCacheBuffer = static_cast<int>(mnCurrentPositionExternal - mnReadBufferStartPosition);
						const int nBytesToGetFromCacheBuffer = static_cast<int>(LOCAL_MIN(mnReadBufferLength-nOffsetWithinCacheBuffer, static_cast<int>(nBytesRemaining)));

						::memcpy(pBuffer8, &mReadBuffer[nOffsetWithinCacheBuffer], nBytesToGetFromCacheBuffer);
						nBytesRemaining           -= nBytesToGetFromCacheBuffer;
						pBuffer8                  += nBytesToGetFromCacheBuffer;
						mnCurrentPositionExternal += nBytesToGetFromCacheBuffer;
					}

					while(nBytesRemaining){ //If there is anything else to read...
						//We need to clear the read buffer, move the current internal file pointer to be where
						//we left off above, and start filling the cache from that position on.
						ClearReadBuffer();
						if(mnCurrentPositionInternal != mnCurrentPositionExternal)
						{
							bResult = (SeekToPosition(mnCurrentPositionExternal) == mnCurrentPositionExternal);
						}
						if(bResult)
							bResult = FillReadBuffer();

						if(bResult && mnReadBufferLength)
						{
							const int nBytesToGetFromCacheBuffer(LOCAL_MIN(mnReadBufferLength, static_cast<int>(nBytesRemaining)));

							::memcpy(pBuffer8, &mReadBuffer[0], nBytesToGetFromCacheBuffer);
							nBytesRemaining           -= nBytesToGetFromCacheBuffer;
							pBuffer8                  += nBytesToGetFromCacheBuffer;
							mnCurrentPositionExternal += nBytesToGetFromCacheBuffer;
						}
						else //Else we hit the end of the file.
							break;
					}
					nBytes -= nBytesRemaining; //Normally, 'nBytesRemaining' will be zero due to being able to read all requested data.

					return bResult; //Return true if all the bytes were readable.
				}
				else
				{ //Else non-buffered behaviour
					CORE_ASSERT(!mnReadBufferLength && !mnWriteBufferLength && (mnCurrentPositionExternal == mnCurrentPositionInternal));

					CORE_ASSERT(sizeof(uint32_t) == sizeof(DWORD));
					const BOOL bResult = ::ReadFile(mhFile, pBuffer, nBytes, (DWORD*)&nBytes, NULL);
					if(bResult)
					{
						mnCurrentPositionInternal += nBytes;
					}
					else
					{
						SetCurrentPositionInternal();
						SaveLastError();
					}
					mnCurrentPositionExternal = mnCurrentPositionInternal;

					return (bResult != 0);
				}
			}
			else
			{ //Else the user requested a read of zero bytes.
				return true;
			}
		}
		else
		{
#ifdef EADM
			EBILOGERROR << L"CoreFile::ReadWithCount(): File isn't open.\n";
#endif
		}

		return false;
	}

	///////////////////////////////////////////////////////////////////////////////
	// CoreFile::FillWriteBuffer
	//
	// This is an internal function.
	//
	bool CoreFile::FillWriteBuffer(uint8_t *pData, int nSize)
	{
		bool bReturnValue(true);

		CORE_ASSERT(nSize >= 0);
		if(nSize > 0)
		{
			//Here we test to see if the write buffer hasn't been allocated yet.
			//It will generally only be non-allocated the first time we get here.
			if(mWriteBuffer.empty())
				mWriteBuffer.resize(mnWriteBufferSize);
			CORE_ASSERT(mWriteBuffer.size() == mnWriteBufferSize);

			if(mnWriteBufferLength == 0)
			{	//If this is our first write to the buffer since it was last purged...
//				CORE_ASSERT(mnCurrentPositionInternal == mnCurrentPositionExternal);
				mnWriteBufferStartPosition = mnCurrentPositionInternal;
			}

			if(mnWriteBufferLength + nSize <= mnWriteBufferSize)
			{
				//We simply append the data to the write buffer. This should be the most
				//common pathway. If we are finding that the write buffer is often too
				//small for the amounts of writes happening, then the write buffer needs
				//to be enlarged or discarded altogether.
				::memcpy(&mWriteBuffer[mnWriteBufferLength], pData, nSize);
				mnWriteBufferLength += nSize;
			}
			//else if(nSize > mnWriteBufferSize){
			//   FlushWriteBuffer();
			//   ClearWriteBuffer();
			//   bReturnValue = (Write the data directly to disk, bypassing the cache.)
			//}
			else
			{	//Else the input data overflows the write buffer.
				//In this case we fill the write buffer as much as possible,
				//flush it, clear it, and fill with new data. This would be
				//faster if we detected large input data sizes and simply
				//wrote them directly to disk instead of copying them to
				//the buffer and then copying the buffer to disk.
				while(nSize && bReturnValue)
				{
					const int nSizeToBuffer(LOCAL_MIN(mnWriteBufferSize-mnWriteBufferLength, nSize));

					if(nSizeToBuffer){
						::memcpy(&mWriteBuffer[mnWriteBufferLength], pData, nSizeToBuffer);
						mnWriteBufferLength += nSizeToBuffer;
						pData               += nSizeToBuffer;
						nSize               -= nSizeToBuffer;
					}
					CORE_ASSERT(mnWriteBufferLength <= mnWriteBufferSize);
					if(mnWriteBufferLength == mnWriteBufferSize)
						bReturnValue = FlushWriteBuffer();
					CORE_ASSERT(nSize >= 0);
				}
			}
		}
		return bReturnValue;
	}

	///////////////////////////////////////////////////////////////////////////////
	// CoreFile::Write
	//
	bool CoreFile::Write(const void* pBuffer, uint32_t nBytes)
	{
		uint32_t nBytesActual(nBytes);

		if(WriteWithCount(pBuffer, nBytesActual))
			return (nBytes == nBytesActual);
		return false;
	}

	///////////////////////////////////////////////////////////////////////////////
	// CoreFile::WriteWithCount
	//
	bool CoreFile::WriteWithCount(const void* pBuffer, uint32_t& nBytes)
	{
		if(mbOpen){
			//Possibly clear the read buffer.
			if(mnReadBufferLength){
				//Our buffer design stipulates that we only have at most one buffer
				//active at a time. This isn't much of a big deal, as 95+% of the time
				//we only open files for either reading or writing.
				ClearReadBuffer();

				//If the position as the user sees it is different from the position
				//as the OS sees it, we need to align these, because otherwise writes
				//will go to the place the OS sees it and not where the user expects.
				if(mnCurrentPositionExternal != mnCurrentPositionInternal)
					SeekToPosition(mnCurrentPositionExternal);
			}

			//Possibly use the write buffer.
			if(mnWriteBufferSize){ //If buffering is enabled...
				CORE_ASSERT(!mnWriteBufferLength || (mnCurrentPositionExternal - mnCurrentPositionInternal) == mnWriteBufferLength); //With write buffers, this should always be true.
				CORE_ASSERT(!mnWriteBufferLength || (mnCurrentPositionExternal == (mnWriteBufferStartPosition + mnWriteBufferLength)));
				const bool bResult = FillWriteBuffer((uint8_t*)pBuffer, nBytes);
				mnCurrentPositionExternal += nBytes; //Not sure what to do if 'bResult' is false. '+= nBytes' seems to be best.
				CORE_ASSERT(!mnWriteBufferLength || (mnCurrentPositionExternal - mnCurrentPositionInternal) == mnWriteBufferLength); //With write buffers, this should always be true.
				CORE_ASSERT(!mnWriteBufferLength || (mnCurrentPositionExternal == (mnWriteBufferStartPosition + mnWriteBufferLength)));
				return bResult;
			}
			else{ //Else non-buffered behaviour
				CORE_ASSERT(!mnReadBufferLength && !mnWriteBufferLength && (mnCurrentPositionExternal == mnCurrentPositionInternal));

				uint32_t nActual(nBytes);
				const BOOL bResult = ::WriteFile(mhFile, pBuffer, nActual, (DWORD*)&nActual, NULL);
				if(bResult)
				{
					mnCurrentPositionInternal += nActual;
					mnCurrentPositionExternal = mnCurrentPositionInternal;
					nBytes = nActual;
				}
				else
				{					
					//Else we have a severe problem.
					SetCurrentPositionInternal();
				}

				return bResult ? true : false; // otherwise we get a stupid compiler warning (BOOL vs bool)
			}
		}
		else
		{
#ifdef EADM
			EBILOGERROR << L"CoreFile::WriteWithCount(): File isn't open.\n";
#endif
		}

		return false;
	}

	///////////////////////////////////////////////////////////////////////////////
	// CoreFile::Flush
	//
	bool CoreFile::Flush(){
		if(mbOpen){
			bool bFlushFileSys = (mnWriteBufferLength > 0); // bpt 9/12/02 // flushing to the filesystem is expensive. do it if and only if there is something to flush.

			//Possibly flush the write buffer. No reason to clear the read buffer.
			FlushWriteBuffer();

			BOOL bResult = 1;
			if(bFlushFileSys){
				//Do the actual flush.
				bResult = ::FlushFileBuffers(mhFile);
				if(bResult == 0)   //If the function failed...
					SaveLastError();
			}

			return (bResult != 0);
		}
		else
		{
#ifdef EADM
			EBILOGERROR << L"CoreFile::Flush(): File isn't open.\n";
#endif
		}

		return false;
	}

	///////////////////////////////////////////////////////////////////////////////
	// CoreFile::ResizeWriteBuffer2
	//
	bool CoreFile::ResizeWriteBuffer2(uint32_t nBytes)
	{
		if(mWriteBuffer.size() != nBytes)
		{
			// If resizeing the cache is necessary flush the previous cache 
			if ( FlushWriteBuffer() == false ) 
				return false;

			mnWriteBufferSize = nBytes;
			mWriteBuffer.resize(mnWriteBufferSize);
		}

		return true;
	}

	///////////////////////////////////////////////////////////////////////////////
	// CoreFile::WriteBufferPointer2
	//
	void *CoreFile::WriteBufferPointer2()
	{
		//Here we test to see if the write buffer hasn't been allocated yet.
		//It will generally only be non-allocated the first time we get here.
		CORE_ASSERT(mWriteBuffer.size() == mnWriteBufferSize);

		if(mnWriteBufferLength == 0)
		{	//If this is our first write to the buffer since it was last purged...
			CORE_ASSERT(mnCurrentPositionInternal == mnCurrentPositionExternal);
			mnWriteBufferStartPosition = mnCurrentPositionInternal;
		}

		return &mWriteBuffer[mnWriteBufferLength];
	}

	///////////////////////////////////////////////////////////////////////////////
	// CoreFile::UpdateWriteBufferSize2
	//
	bool CoreFile::UpdateWriteBufferSize2(uint32_t nBytes)
	{
		mnWriteBufferLength += nBytes;	
		mnCurrentPositionExternal += nBytes;

		if ( mnWriteBufferLength == mnWriteBufferSize )
		{
			// flush the write buffer when buffer is filled			
			FlushWriteBuffer();	

			return true;
		}

		return true;
	}

	bool CoreFile::Remove()
	{
		if (!msPath.empty())
			return Remove( msPath.c_str() );

		return false;
	}

	bool CoreFile::Rename( const char *newPath )
	{
		if ((newPath) && (!msPath.empty()))
			return Rename( msPath.c_str(), Util::UTF8ToWChar(newPath).c_str() );

		return false;
	}

	bool CoreFile::Rename( const wchar_t *newPath )
	{
		if ((newPath) && (!msPath.empty()))
			return Rename( msPath.c_str(), newPath );

		return false;
	}

	bool CoreFile::Remove( const char *path )
	{
		CORE_ASSERT(path);

		return (::DeleteFileA( path ) == S_OK);
	}

	bool CoreFile::Remove( const wchar_t *path )
	{
		CORE_ASSERT(path);

		return (::DeleteFileW( path ) == S_OK);
	}

	bool CoreFile::Remove( const QString& path )
	{
		CORE_ASSERT(!path.isEmpty());

		return (::DeleteFileW( path.utf16() ) == S_OK);
	}


	bool CoreFile::Rename( const char *oldPath, const char *newPath )
	{
		CORE_ASSERT(oldPath);
		CORE_ASSERT(newPath);

		return (::MoveFileA( oldPath, newPath ) == S_OK);
	}

	bool CoreFile::Rename( const wchar_t *oldPath, const wchar_t *newPath )
	{
		CORE_ASSERT(oldPath);
		CORE_ASSERT(newPath);

		return (::MoveFileW( oldPath, newPath ) == S_OK);
	}

	bool CoreFile::ValidInternalPosition()
	{
		DECLARE_LIZERO;
		DECLARE_LIPOS;

		if ( (::SetFilePointerEx(mhFile, liZERO, &liPos, FILE_CURRENT))
			&& (mnCurrentPositionInternal == liPos.QuadPart) )
			return true;

		return false;
	}

	void CoreFile::SetCurrentPositionInternal()
	{
		//Get the current position. This is how you do it under Win32.
		DECLARE_LIZERO;
		DECLARE_LIPOS;
		::SetFilePointerEx(mhFile, liZERO, &liPos, FILE_CURRENT);
		mnCurrentPositionInternal = liPos.QuadPart;

	}

	BOOL CoreFile::WriteString(QString& rString)
	{
		if (!IsOpen()) return false;

		// TODO: Should we always move to end of file or allow insertion/termination at current offset?

		// String already terminated with CRLF pair?
		if (rString.right(2) == "\r\n")
			if (Write(rString.constData(), rString.size() * sizeof(wchar_t)))  // No additional termination
				return false;
		// If either CR or LF is found at end of string
			else if (!rString.right(1).contains(QRegExp("[\r\n]")))
			{
				int iSizeInBytes = (rString.size()-1) * sizeof(wchar_t);
				if (iSizeInBytes)
					if ( Write( rString.constData(), iSizeInBytes) )
						return false;
				iSizeInBytes = 2*sizeof(wchar_t);
				if (Write(L"\r\n", iSizeInBytes))
					return false;
			}
			return true;
	}

	BOOL CoreFile::ReadString(QString& rString)
	{
		rString.clear();

		// End of file?
		if (!IsOpen() || IsEOF())
		{
			return false;
		}

		// Remember current offset
		ULONGLONG ullOffset = Position();

		long lCharsConsumed = 0;  // Includes any extra string CR/LFs consumed but not returned in string.
		while(1)
		{
			wchar_t pBuffer[256];
			memset(pBuffer, 0, 256 * sizeof(wchar_t));

			// Get another chunk
			long lBytesRead = Read(pBuffer, 256);
			long lCharsRead = lBytesRead / sizeof(wchar_t);

			long lLineCROrLFOffset = -1;

			// No more data?
			if (lBytesRead <= 0) 
				break;
			else  
			{
				// Truncate string at current chunk length if partial
				//if (lCharsRead < 256)
					//sChunk.ReleaseBuffer( lCharsRead );
				QString sChunk = QString::fromWCharArray(pBuffer, lBytesRead);

				// Search for first of possible CRLF pair 
				lLineCROrLFOffset = sChunk.indexOf(QRegExp("[\r\n]"));
				if (lLineCROrLFOffset >= 0)
				{
					// Part of a pair?
					if ( sChunk.mid(lLineCROrLFOffset+1, 1).indexOf(QRegExp("[\r\n]")) >= 0 )
						lCharsConsumed += lLineCROrLFOffset+2;
					else
						lCharsConsumed += lLineCROrLFOffset+3;

					// Terminate chunk before line end char/pair
					sChunk = sChunk.left(lLineCROrLFOffset + 2);

					// Done since we found a line terminator
					rString.append(sChunk);
					break;
				}

				rString.append(sChunk);

				// Stop now if we got back and partial chunk so we know we're out of data
				if (lCharsRead < 256)
					break;
			}
		}

		// Place offset at end of retrieved string
		Seek(ullOffset + (lCharsConsumed * sizeof(wchar_t)));

		return true;
	}
*/	

	bool CoreFile::moveDirectory(const QString& newLocation, const QString& oldLocation)
	{
		QString source = QDir::toNativeSeparators(oldLocation);
		QString target = QDir::toNativeSeparators(newLocation);

		QDir oldDir(source);
		if (!oldDir.exists())
		{
			//EBILOGERROR << "Directory does not exist: " << oldLocation;
			return false;
		}

		QDir newDir(target);
		if (newDir.exists())
		{
			//EBILOGERROR << "Cannot overwrite an existing directory: " << newLocation;
			return false;
		}

		newDir.mkdir(newLocation);
		QStringList filesToMove = oldDir.entryList(QDir::Files);
		foreach (const QString& filename, filesToMove)
		{
			QString sourceName = QString("%s/%s").arg(source).arg(filename);
			QString targetName = QString("%s/%s").arg(target).arg(filename);
			if (!QFile::copy(sourceName, targetName))
			{
				//EBILOGERROR << "Unable to copy file " << sourceName << " to " << targetName;
				return false;
			}
			if (!QFile::remove(sourceName))
			{
				//EBILOGERROR << "Unable to delete file: " << sourceName;
				return false;
			}

		}

		QStringList subdirectoriesToMove = oldDir.entryList(QDir::AllDirs | QDir::NoDotAndDotDot);
		foreach (const QString& subdirectoryName, subdirectoriesToMove)
		{
			QString sourceName = QString("%s/%s").arg(source).arg(subdirectoryName);
			QString targetName = QString("%s/%s").arg(target).arg(subdirectoryName);
			if (!moveDirectory(targetName, sourceName))
			{
				//EBILOGERROR << "Unable to move directory " << sourceName << " to" << targetName;
				return false;
			}
		}

		if (!oldDir.rmdir(source))
		{
			//EBILOGERROR << "Unable to delete directory: " << source;
			return false;
		}


		return true;
	}

	bool CoreFile::readString(QString& rString)
	{
		rString.clear();

		if (!isOpen() || atEnd())
		{
			return false;
		}

		// Remember current offset
		qint64 ullOffset = pos();

		long lCharsConsumed = 0;  // Includes any extra string CR/LFs consumed but not returned in string.
		while(1)
		{
			wchar_t pBuffer[256];
			memset(pBuffer, 0, 256 * sizeof(wchar_t));

			// Get another chunk, make sure last wchar is null 
			qint64 lBytesRead = read(reinterpret_cast<char*>(pBuffer), 256 * sizeof(wchar_t));
			qint64 lCharsRead = lBytesRead / sizeof(wchar_t);

			long lLineCROrLFOffset = -1;

			// No more data?
			if (lBytesRead <= 0) 
				break;
			else  
			{
				// Truncate string at current chunk length if partial
				QString sChunk = QString::fromWCharArray(pBuffer, lCharsRead);

				// Search for first of possible CRLF pair 
				lLineCROrLFOffset = sChunk.indexOf(QRegExp("[\r\n]"));
				if (lLineCROrLFOffset >= 0)
				{
					// Part of a pair?
					if ( sChunk.mid(lLineCROrLFOffset+1, 1).indexOf(QRegExp("[\r\n]")) >= 0 )
						lCharsConsumed += lLineCROrLFOffset+2;
					else
						lCharsConsumed += lLineCROrLFOffset+3;

					// Terminate chunk before line end char/pair
					sChunk = sChunk.left(lLineCROrLFOffset + 2);

					// Done since we found a line terminator
					rString.append(sChunk);
					break;
				}

				rString.append(sChunk);

				// Stop now if we got back and partial chunk so we know we're out of data
				if (lCharsRead < 256)
					break;
			}
		}

		// Place offset at end of retrieved string
		seek(ullOffset + (lCharsConsumed * sizeof(wchar_t)));

		return true;
	}

	bool CoreFile::seek(qint64 numBytes, enum eFilePosition referencePosition)
	{
		qint64 offsetFromBegin = numBytes;

		switch (referencePosition)
		{
		case CoreFile::kCurrent:
			offsetFromBegin = pos() + numBytes;
			break;
		case CoreFile::kEnd:
			numBytes = abs(numBytes);
			offsetFromBegin = size() - numBytes;
			break;
		default:;
		}

		return QFile::seek(offsetFromBegin);
	}
//}
