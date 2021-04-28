/**
ZipFileInfo and ZipFileEntry implementations
*/

#include "ZipFileInfo.h"
#include <limits>
#include <assert.h>

#include <QDebug>

#ifdef _DEBUG
// int testMe()
// {
// 	ZipFileInfo finfo;
// 	if ( finfo.Load( "C:\\downloads\\qa_non_ascii2.zip" ) )
// 	{
// 		finfo.ValidationTest();
// 	}
// 	return 1;
// }
// 
// static int test = testMe();
#endif


/**
Read from a file, optionally seeking to a specific place. Catch exceptions.;
*/
static bool SafeReadFile(CoreFile& f, void* lpBuf, UINT nCount, __int64 offsetFromBegin = -1)
{
	bool result = false;

	//Seek
	/*if ( offsetFromBegin >= 0 )
	{
		if ( f.Seek(offsetFromBegin, CoreFile::kSeekStart) != offsetFromBegin )
			return false;
	}
	
	//Read
	result = (f.Read(lpBuf, nCount) == nCount);*/

	if (offsetFromBegin > 0)
	{
		if (!f.seek(offsetFromBegin))
		{
			return false;
		}
	}

	result = f.read(reinterpret_cast<char*>(lpBuf), nCount) == nCount;

	return result;
}

static QString sizeToString( __int64 bytes )
{
	QString result;

	if ( bytes < 1000 )
	{
		result = QString("%1 bytes").arg((int) bytes);
	}
	else if ( bytes < 1024 * 1000 )
	{
		result = QString("%1 KB").arg(QString::number(double(bytes) / 1024, 'f', 2));
	}
	else if ( bytes < 1024 * 1024 * 1000 )
	{
		result = QString("%1 MB").arg(QString::number(double(bytes) / (1024 * 1024), 'f', 2));
	}
	else
	{
		result = QString("%1 GB").arg(QString::number(double(bytes) / (1024 * 1024 * 1024), 'f', 2));
	}

	return result;
}

static void Output( QString line )
{
        OutputDebugString( (const WCHAR*) line.utf16() );
}

//Forward declarations
QString getCompressionMethodDescription( unsigned short ID );
QString getExtraFieldsDescription( const MemBuffer& extraField );
QString getExtraFieldDescription( unsigned short ID );

//Class static const members
const uint32_t ZipFileInfo::ZipEndOfCentralDirSignature = 0x06054b50;
const uint32_t ZipFileInfo::Zip64EndOfCentralDirSignature = 0x06064b50;
const uint32_t ZipFileInfo::Zip64EndOfCentralDirLocatorSignature = 0x07064b50;
const uint32_t ZipFileInfo::ZipEndOfCentralDirFixedPartSize = 22;
const uint32_t ZipFileInfo::Zip64EndOfCentralDirFixedPartSize = 56;
const uint32_t ZipFileInfo::Zip64EndOfCentralDirLocatorSize = 20;
const uint32_t ZipFileInfo::FileHeaderSignature = 0x02014b50;
const uint32_t ZipFileInfo::FileHeaderFixedPartSize = 46;
const uint32_t ZipFileInfo::LocalFileHeaderSignature = 0x04034b50;
const uint32_t ZipFileInfo::LocalFileHeaderFixedPartSize = 30;

ZipFileInfo::ZipFileInfo()
{
	mOffsetToCentralDir = 0;
    mSizeCentralDir = 0;
	mNumOfDiscs = 1;
}

ZipFileInfo::~ZipFileInfo()
{
	Clear();
}

void ZipFileInfo::Clear()
{
    mFileName.clear();
	mComment.clear();
	mSizeCentralDir = 0;
    mOffsetToCentralDir = 0;

	ZipFileEntryList::iterator it = mEntries.begin();

	while ( it != mEntries.end() )
	{
		delete *it;
		it++;
	}

	mEntries.clear();
}

void ZipFileInfo::Dump()
{
	//Dump standard data
	Output( QString("Zip archive has %1 entries (%2 files and %3 dirs).\n").arg(GetNumberOfEntries()).arg(GetNumberOfFiles()).arg(GetNumberOfDirectories()) );

	ZipFileEntryList::iterator it = mEntries.begin();

	//Lets calculate the greatest filename length
	int max_namel = 0, max_sizel = 0;
	while ( it != mEntries.end() )
	{
                max_namel = MAX(max_namel, (*it)->GetFileName().length());
                max_sizel = MAX(max_sizel, sizeToString((*it)->GetUncompressedSize()).length());
		it++;
	}

	it = mEntries.begin();

	while ( it != mEntries.end() )
	{
		ZipFileEntry* entry = *it;
		Output( QString("%1 | %2 | %3").arg(entry->GetFileName(), max_namel + 1).arg(sizeToString(entry->GetUncompressedSize()), max_sizel + 1).arg(entry->IsDirectory() ? "dir" : "file") );
		it++;
	}
}

bool ZipFileInfo::Load( const QString& filename )
{
	bool result = false;

	/*CoreFile file;
	if (file.Open(filename, CoreFile::kOpenRead, CoreFile::kCDOpenExisting))
		result = Load( file );
	else
		Output( QString("Unable to open %1.").arg(filename) );

	return result;*/

	CoreFile file(filename);
	if (file.exists() && file.open(QIODevice::ReadOnly))
	{
		result = Load(file);
		file.close();
	}
	else
	{
		Output( QString("Unable to open %1.").arg(filename) );
	}

	return result;
}

/**
Find backwards a signature within a file. 
sig: The signature to look for (stored as little endian)
backStart: Offset from end to start from
uMaxBack: If this value is reached aborts the search
*/
static __int64 findSignature( CoreFile& file, uint32_t signature, uint32_t backStart = 0, uint32_t uMaxBack = ULONG_MAX )
{
	const int READBLOCK = 1024;

	unsigned char buf[READBLOCK + 4];
	__int64 uSizeFile = file.size();
	__int64 uBackRead;
	__int64 uPosFound = 0;
	__int64 uReadPos;
	uint32_t uReadSize;

	//Walk backwards and try to find the signature
    
	uBackRead = backStart + 4;

	u_char sig[4] = { LOBYTE(LOWORD(signature)), HIBYTE(LOWORD(signature)),
					  LOBYTE(HIWORD(signature)), HIBYTE(HIWORD(signature)) };

	while ( uBackRead < uMaxBack )
	{
		if ( uBackRead + READBLOCK > uMaxBack )
			uBackRead = uMaxBack;
		else
			uBackRead += READBLOCK;

		//Clamp starting pos if we are reaching the beginning (shouldn't happen)
		if ( uBackRead > uSizeFile )
			uBackRead = uSizeFile;
			
		//Starting position
		uReadPos = uSizeFile - uBackRead;

		//Read chunk size
		uReadSize = (uint32_t) READBLOCK + 4;

		//Clamp size if we are reaching the beginning (shouldn't happen)
		if ( uSizeFile - uReadPos < uReadSize )
			uReadSize = (uint32_t) (uSizeFile - uReadPos);
		
		//Do the seek and read
		if ( !SafeReadFile(file, buf, uReadSize, uReadPos) )
			break;

		//Search backwards for the marker
		for ( int i = (int)uReadSize-3; (i--)>0; )
		{
			if (((*(buf+i))==sig[0]) && ((*(buf+i+1))==sig[1]) &&
			    ((*(buf+i+2))==sig[2]) && ((*(buf+i+3))==sig[3]))
			{
				uPosFound = uReadPos + i;
				break;
			}
		}

		if ( uPosFound != 0 )
			break;
	}

	return uPosFound;
}

/**
Find backwards a signature within a buffer
Returns the index from the beginning of the buffer where the signature was found or -1
*/
static __int64 findSignature( const u_char* buf, uint32_t size, uint32_t signature )
{
	__int64 uPosFound = -1;

	const u_char sig[4] = { LOBYTE(LOWORD(signature)), HIBYTE(LOWORD(signature)),
							LOBYTE(HIWORD(signature)), HIBYTE(HIWORD(signature)) };

	if ( size < 4 )
		return uPosFound;

	//Search backwards for the marker
	for ( int i = (int)size-3; (i--)>0; )
	{
		if (((*(buf+i))==sig[0]) && ((*(buf+i+1))==sig[1]) &&
			((*(buf+i+2))==sig[2]) && ((*(buf+i+3))==sig[3]))
		{
			uPosFound = i;
			break;
		}
	}

	return uPosFound;
}

/**
Find backwards a signature within a buffer. 
sig: The signature to look for (stored as little endian)
backStart: Offset from end to starting point
uMaxBack: If this value is reached aborts the search
Returns the index from the beginning of the buffer where the signature was found or -1
*/
static __int64 findSignature( MemBuffer& memBuffer, uint32_t signature, uint32_t backStart = 0, uint32_t uMaxBack = ULONG_MAX )
{
	__int64 uPosFound = -1;

	if ( memBuffer.TotalSize() < 4 )
		return uPosFound;

	if ( backStart >= memBuffer.TotalSize() - 4 )
		return uPosFound;

	if ( uMaxBack > memBuffer.TotalSize() )
		uMaxBack = memBuffer.TotalSize();

	if ( backStart > uMaxBack )
		return uPosFound;
	
	//Get the starting pointer
	uint32_t offset = memBuffer.TotalSize() - uMaxBack;

	const u_char* ptr = memBuffer.GetBufferPtr() + offset;

	uPosFound = findSignature( ptr, uMaxBack - backStart, signature );

	if ( uPosFound >= 0 )
	{
		uPosFound += offset;
	}

	return uPosFound;
}

bool ZipFileInfo::Load( CoreFile& file )
{
	bool result = false;

	//Walk backwards and try to find the end of central directory
	//signature 4 bytes: 0x06054b50
	__int64 uPosFound = findSignature(file, ZipEndOfCentralDirSignature, 0, 0xffff /* maximum size of global comment */);

	//We have the end of central dir
	if ( uPosFound != 0 )
	{
		u_char headBuf[ZipEndOfCentralDirFixedPartSize];
		stEndOfCentralDir info;

                MemBuffer tempbuf1(headBuf, sizeof(headBuf));
		if ( SafeReadFile(file, headBuf, _countof(headBuf), uPosFound) &&
                     readEndOfCentralDir(info, tempbuf1) )
		{
			// set number of discs from EndOfCentralDir
			mNumOfDiscs = info.diskNo + 1;

			//Read the file comment
			if ( info.commentLenght > 0 )
			{
				mComment = loadString(file, info.commentLenght, CP_ACP);
			}

			//Now check if we need to find a Zip64 end of central dir instead
			if ( info.offsetToCentralDir == 0xffffffff )
			{
				//Ok find the locator and then the Zip64 EOCD
				__int64 fileLength = file.size();
				assert(fileLength>=0);
				if (fileLength == -1)	// file error
					fileLength = 0;

				uPosFound = findSignature(file, Zip64EndOfCentralDirLocatorSignature, (uint32_t) (fileLength - uPosFound));

				if ( uPosFound != 0 )
				{
					u_char locatorBuf[Zip64EndOfCentralDirLocatorSize];
					stZip64EndOfCentralDirLocator locator;

					//Read the Zip64 end of central directory locator
                                        MemBuffer tempbuf2(locatorBuf, sizeof(locatorBuf));
					if ( SafeReadFile(file, locatorBuf, _countof(locatorBuf), uPosFound) &&
                                                 readZip64EndOfCentralDirLocator(locator, tempbuf2) )
					{
						u_char head64Buf[Zip64EndOfCentralDirFixedPartSize];

						//Read the Zip64 end of central directory
                                                MemBuffer tempbuf3(head64Buf, sizeof(head64Buf));
						if ( SafeReadFile(file, head64Buf, _countof(head64Buf), locator.offsetToZip64EndOfCentralDir) &&
                                                         readEndOfCentralDir(info, tempbuf3) )
						{
							// set number of discs from EndOfCentralDir
							mNumOfDiscs = info.diskNo + 1;

							//Now load the central dir
							result = loadCentralDir(&info, file);
						}
					}
				}
			}
			else
			{
				//Now load the central dir
				result = loadCentralDir(&info, file);
			}
		}
	}

	if ( result )
	{
		mFileName = file.fileName();
	}
	else
	{
		mFileName.clear();
	}

	return result;
}

bool ZipFileInfo::TryLoad( MemBuffer& buffer, __int64 fileSize, __int64* offsetToStart )
{
	const int MAX_CENTRAL_DIR_SCAN_SIZE = 5 * 1024 * 1024;	// there isn't any documented max size, but typically the central directory is a few hundred k max.  This is for crash prevention in the case of a badly formed zip file.

	bool result = false;
	__int64 dummy = 0;
	
	//Point to dummy stack var if unavailable
	if ( offsetToStart == NULL )
		offsetToStart = &dummy;

	*offsetToStart = 0;

	__int64 uPosFound = findSignature(buffer, ZipEndOfCentralDirSignature, 0, 0xffff /* maximum size of global comment */);

	//We have the end of central dir
	if ( uPosFound >= 0 )
	{
		stEndOfCentralDir eocdInfo;
		
		buffer.Seek((long) uPosFound, SEEK_SET);

		if ( readEndOfCentralDir(eocdInfo, buffer) )
		{
			// set number of discs from EndOfCentralDir
			mNumOfDiscs = eocdInfo.diskNo + 1;

			//Read the file comment, if any
			if ( eocdInfo.commentLenght > 0 )
			{
				//buffer.Seek((long) uPosFound + ZipEndOfCentralDirFixedPartSize, SEEK_SET);
				mComment = buffer.ReadString(eocdInfo.commentLenght, CP_ACP);
			}

			//Now check if we need to find a Zip64 end of central dir instead
			if ( eocdInfo.offsetToCentralDir == 0xffffffff )
			{
				//Ok find the locator and then the Zip64 EOCD, starting at where we found the end of central dir
				uPosFound = findSignature(buffer, Zip64EndOfCentralDirLocatorSignature, (uint32_t)(buffer.TotalSize() - uPosFound));

				if ( uPosFound != 0 )
				{
					stZip64EndOfCentralDirLocator locator;

					//Read the Zip64 end of central directory locator
                                        MemBuffer tempbuf1(buffer.GetBufferPtr() + uPosFound, Zip64EndOfCentralDirLocatorSize);
                                        if ( readZip64EndOfCentralDirLocator(locator, tempbuf1) )
					{
						__int64 pos = locator.offsetToZip64EndOfCentralDir - (fileSize - buffer.TotalSize());

						if ( pos >= 0 )
						{
                                                        MemBuffer tempbuf2(buffer.GetBufferPtr() + pos, Zip64EndOfCentralDirFixedPartSize);
                                                        if ( !readEndOfCentralDir(eocdInfo, tempbuf2) )
							{
								eocdInfo.offsetToCentralDir = 0;

								//Failed
								Output("Failed to read Zip64 End Of Central Dir");
							}
							else
							{
								// set number of discs from EndOfCentralDir
								mNumOfDiscs = eocdInfo.diskNo + 1;
							}
						}
						else
						{
							//Ask for more
							*offsetToStart = locator.offsetToZip64EndOfCentralDir;
						}
					}
					else
					{
						//Error
						Output("Failed to read Zip64 End Of Central Dir Locator");
					}
				}
				else
				{
					//Need more
					if ( buffer.TotalSize() < MAX_CENTRAL_DIR_SCAN_SIZE  )
					{
                                                __int64 wantMore = MIN(fileSize - buffer.TotalSize(), 1024);
						*offsetToStart = fileSize - buffer.TotalSize() - wantMore;
					}
				}
			}

			//If we have the end of central dir info, try to load the central dir
			if ( eocdInfo.offsetToCentralDir >= 0 && eocdInfo.offsetToCentralDir != 0xffffffff ) // modified to allow for a offsetToCentralDir == 0 as multi-part zips can have this (mfong: 5/17/11)
			{
				//Check to see if we have enough data
				if ( buffer.TotalSize() >= fileSize - eocdInfo.offsetToCentralDir )
				{
					__int64 pos = eocdInfo.offsetToCentralDir - (fileSize - buffer.TotalSize());
					MemBuffer tmpBuffer( buffer.GetBufferPtr() + pos, (uint32_t) eocdInfo.centralDirSize, false );

					//Finally load the central directory
					result = loadCentralDir( &eocdInfo, tmpBuffer );

					if ( result )
					{
						//Let the caller know how much we used
						*offsetToStart = eocdInfo.offsetToCentralDir;
					}
				}
				else
				{
					//Ask for more, check error condition
					if ( eocdInfo.offsetToCentralDir < fileSize )
					{
						*offsetToStart = eocdInfo.offsetToCentralDir;
					}
				}
			}
		}
		else
		{
			//format error
			Output("Could not read End of Central Dir");
		}
	}
	else
	{
		//End of central dir not found ask for more
		if ( buffer.TotalSize() < MAX_CENTRAL_DIR_SCAN_SIZE  )
		{
                        __int64 wantMore = MIN(fileSize - buffer.TotalSize(), 1024);
			*offsetToStart = fileSize - buffer.TotalSize() - wantMore;
		}
	}

	return result;
}

uint32_t ZipFileInfo::GetNumberOfFiles() const
{
	uint32_t count = 0;
	ZipFileEntryList::const_iterator it = mEntries.begin();

	while ( it != mEntries.end() )
	{
		if ( !(*it)->IsDirectory() )
			count++;
		
		it++;
	}

	return count;
}

uint32_t ZipFileInfo::GetNumberOfDirectories() const
{
	uint32_t count = 0;
	ZipFileEntryList::const_iterator it = mEntries.begin();

	while ( it != mEntries.end() )
	{
		if ( (*it)->IsDirectory() )
			count++;
		
		it++;
	}

	return count;
}

__int64 ZipFileInfo::GetUncompressedSize() const
{
	__int64 size = 0;
	ZipFileEntryList::const_iterator it = mEntries.begin();

	while ( it != mEntries.end() )
	{
		size += (*it)->GetUncompressedSize();
		it++;
	}

	return size;
}

bool ZipFileInfo::loadCentralDir( const stEndOfCentralDir* info, CoreFile& f )
{
	bool result = false;

	MemBuffer buffer((uint32_t) info->centralDirSize);

	if ( !buffer.IsEmpty() &&
		 SafeReadFile(f, buffer.GetBufferPtr(), buffer.TotalSize(), info->offsetToCentralDir) )
	{
		result = loadCentralDir(info, buffer);
	}
	
	return result;
}

QString ZipFileInfo::loadString(CoreFile& file, int len, u_int codepage)
{
	QString dst;

	if ( len > 0 )
	{
		MemBuffer tmpBuffer(len);

		if ( SafeReadFile(file, tmpBuffer.GetBufferPtr(), len) )
		{
			dst = tmpBuffer.ReadString(len, codepage);
		}
	}

	return dst;
}

QString ZipFileInfo::loadString(const QString& str, int len, u_int codepage)
{
	QString dst;

	if ( !str.isEmpty() && len > 0 )
	{
		MemBuffer tmpBuffer((u_char*)str.constData(), len, false); //The cast is safe, we will only be reading
		dst = tmpBuffer.ReadString(len, codepage);
	}

	return dst;
}

bool ZipFileInfo::loadCentralDir( const stEndOfCentralDir* info, MemBuffer& buffer )
{
	bool result = true;

	for ( int i = 0; i < info->entriesTotal && result; i++ )
	{
		//We don't just 'point' the pointer because of the endian compatibility
		stFileHeader header;
		
		result = readFileHeader(header, buffer);
		
		if ( result )
		{
			mOffsetToCentralDir = info->offsetToCentralDir;

			ZipFileEntry* entry = new ZipFileEntry(&header);

			uint32_t variableSize = header.filenameLength + 
								  header.extraFieldLength + 
								  header.fileCommentLength;
			
			//As per specs the three combined variable 
			//fields should not exceed 65,535 bytes, but 
			if ( header.filenameLength + 
				 header.extraFieldLength + 
				 header.fileCommentLength > 65535 )
			{
				//this is not a requirement, uncomment to be safe
				//result = false;
			}

			if ( result && (buffer.Remaining() >= variableSize) )
			{
				int encoding = entry->HasUtf8Encoding() ? CP_UTF8 : CP_OEMCP;

				if ( header.filenameLength > 0 )
				{
					QString name = buffer.ReadString(header.filenameLength, encoding);
					entry->setFileName(name);
				}
				
				if ( header.extraFieldLength > 0 )
				{
					MemBuffer extraFieldBuffer(header.extraFieldLength);
					buffer.Read(extraFieldBuffer.GetBufferPtr(), extraFieldBuffer.TotalSize());
					entry->setExtraField(extraFieldBuffer);

					//zip64 extended information extra field
					//If the file offset is past the 4 bytes 4Gb limit then
					//the zip64 extended information extra field contains the 
					//proper 8 bytes values. 
					//Refer to the ZIP format documentation
					MemBuffer buffer;
					if ( entry->GetExtraField(0x01, buffer) )
					{
						if ( entry->GetUncompressedSize() == 0xffffffff )
						{
							__int64 val = buffer.getInt64();
							entry->mUncompressedSize = val;
						}

						if ( entry->GetCompressedSize() == 0xffffffff )
						{
							__int64 val = buffer.getInt64();
							entry->mCompressedSize = val;
						}

						if ( entry->GetOffsetOfLocalHeader() == 0xffffffff )
						{
							__int64 val = buffer.getInt64();
							entry->mOffsetOfLocalHeader = val;
						}

						//Disk number follows but we don't support that
					}

					//Unicode filename
					if ( entry->GetExtraField(0x7075, buffer) )
					{
						u_char version = buffer.getUChar();
						
						if ( version == 1 )
						{
							uint32_t nameCRC32 = buffer.getULong();
							QString szName = buffer.ReadString( buffer.Remaining(), CP_UTF8 );
							if ( !szName.isEmpty() )
							{
								entry->setFileName( szName );
							}
						}
					}
				}

				if ( header.fileCommentLength > 0 )
				{
					QString comment = buffer.ReadString(header.fileCommentLength, encoding);
					entry->setFileComment(comment);
				}

				AddEntry(entry);
			}
			else
			{
				delete entry;
			}
		}
	}

	for (ZipFileEntryList::iterator it = mEntries.begin(); it != mEntries.end(); it++)
	{
		ZipFileEntryList::iterator itNext = it;
		itNext++;

		__int64 nOffsetToFileData;
		// special case for last entry
		if (itNext == mEntries.end())
		{
			nOffsetToFileData = mOffsetToCentralDir - (*it)->mCompressedSize;

			// for ITO builds the central directory is its own file, and mOffsetToCentral dir is 0
			// so the nOffsetToFileData will be less than zero. We don't want it to assert in this case
			//we will calculate the proper offset in CoreDownloadJob_Zip::RetrieveOffsetToFileData in this case

			if(mOffsetToCentralDir)
			{
				assert(nOffsetToFileData > 0);
			}
		}
		else
		if ((*itNext)->GetDiskNoStart() != (*it)->GetDiskNoStart())
		{
			//if the files aren't on the same disk, we cannot calculate the file data offset now
			//we will calculate the proper offset in CoreDownloadJob_Zip::RetrieveOffsetToFileData
			nOffsetToFileData = -1;
		}
		else
		{
			nOffsetToFileData = (*itNext)->mOffsetOfLocalHeader - (*it)->mCompressedSize;
			assert(nOffsetToFileData > 0);
		}

		(*it)->mOffsetOfLocalFileData = nOffsetToFileData;

	}

	return result;
}

bool ZipFileInfo::readEndOfCentralDir( stEndOfCentralDir& info, MemBuffer& data )
{
	bool ok = false;

	//Zero out the struct
	memset(&info, 0, sizeof(info));

	//Check
        if ( data.Remaining() < MIN(ZipEndOfCentralDirFixedPartSize, Zip64EndOfCentralDirFixedPartSize) )
        {
		return false;
        }

	data.SetByteOrder( MemBuffer::LittleEndian );

	//The signature will tell us the version (Zip or Zip64)
	info.signature = data.getULong();

	if ( info.signature == ZipFileInfo::ZipEndOfCentralDirSignature )		//Zip
	{
		info.diskNo = data.getUShort();
		info.diskWithCentralDir = data.getUShort();
		info.entriesThisDisk = data.getUShort();
		info.entriesTotal = data.getUShort();
		info.centralDirSize = data.getULong();
		info.offsetToCentralDir = data.getULong();
		ok = data.Remaining() >= 2;
		info.commentLenght = data.getUShort();
	}
	else if ( info.signature == ZipFileInfo::Zip64EndOfCentralDirSignature ) //Zip64
	{
		//Skip size of this record as we are not using the extensible data sector
		__int64 sizeOfRecord = data.getInt64();

		//As per the documentation this value 
		//should not include the leading 12 bytes
		//SizeOfFixedFields + SizeOfVariableData - 12

		//Should be at least the fixed size, assuming no variable data is present
		if ( sizeOfRecord >= (Zip64EndOfCentralDirFixedPartSize - 12) ) 
		{
			uint16_t versionMadeBy = data.getUShort();
			uint16_t versionNeededToExtract = data.getUShort();
			info.diskNo = data.getULong();
			info.diskWithCentralDir = data.getULong();
			info.entriesThisDisk = data.getInt64();
			info.entriesTotal = data.getInt64();
			info.centralDirSize = data.getInt64();
			ok = data.Remaining() >= 8;
			info.offsetToCentralDir = data.getInt64();
			info.commentLenght = 0;
		}
	}
	else
	{
		//unrecognized
	}

	return ok;
}

bool ZipFileInfo::readZip64EndOfCentralDirLocator( stZip64EndOfCentralDirLocator& locator, MemBuffer& data )
{
	bool ok = false;

	//Zero out the struct
	memset(&locator, 0, sizeof(locator));

	if ( data.Remaining() < Zip64EndOfCentralDirLocatorSize )
		return false;

	data.SetByteOrder( MemBuffer::LittleEndian );

	//Read signature
	locator.signature = data.getULong();

	//Verify signature
	if ( locator.signature == Zip64EndOfCentralDirLocatorSignature )
	{
		locator.diskWithEndOfCentralDir = data.getULong();
		locator.offsetToZip64EndOfCentralDir = data.getInt64();
		locator.numDisks = data.getULong();
		ok = true;
	}

	return ok;

}

bool ZipFileInfo::readFileHeader( stFileHeader& info, MemBuffer& data )
{
	bool ok = false;

	//Zero out the struct
	memset(&info, 0, sizeof(info));

	if ( data.Remaining() >= FileHeaderFixedPartSize )
	{
		data.SetByteOrder( MemBuffer::LittleEndian );

		//The signature
		info.signature = data.getULong();

		if ( info.signature == ZipFileInfo::FileHeaderSignature ) //Central directory file header
		{
			info.versionMade = data.getUShort();
			info.versionNeeded = data.getUShort();
			info.bitFlags = data.getUShort();
			info.compressionMethod = data.getUShort();
			info.lastModTime = data.getUShort();
			info.lastModDate = data.getUShort();
			info.crc32 = data.getULong();
			info.compressedSize = data.getULong();
			info.uncompressedSize = data.getULong();
			info.filenameLength = data.getUShort();
			info.extraFieldLength = data.getUShort();
			info.fileCommentLength = data.getUShort();
			info.diskNoStart = data.getUShort();
			info.internalAttribs = data.getUShort();
			info.externalAttribs = data.getULong();
			info.offsetLocalHeader = data.getULong();

			ok = true;
		}
	}

	return ok;
}

bool ZipFileInfo::readLocalFileHeader( stLocalFileHeader& info, MemBuffer& data )
{
	bool ok = false;

	//Zero out the struct
	memset(&info, 0, sizeof(info));

	if ( data.Remaining() >= LocalFileHeaderFixedPartSize )
	{
		data.SetByteOrder( MemBuffer::LittleEndian );

		//The signature
		info.signature = data.getULong();

		if ( info.signature == ZipFileInfo::LocalFileHeaderSignature ) //Local directory file header
		{
			info.versionNeeded = data.getUShort();
			info.bitFlags = data.getUShort();
			info.compressionMethod = data.getUShort();
			info.lastModTime = data.getUShort();
			info.lastModDate = data.getUShort();
			info.crc32 = data.getULong();
			info.compressedSize = data.getULong();
			info.uncompressedSize = data.getULong();
			info.filenameLength = data.getUShort();
			info.extraFieldLength = data.getUShort();

			ok = true;
		}
	}

	return ok;
}

bool ZipFileInfo::AddEntry( ZipFileEntry* entry )
{
	mEntries.push_back(entry);
	return true;
}

ZipFileEntry::ZipFileEntry()
{
	Clear();
}

ZipFileEntry::ZipFileEntry(const stFileHeader* info)
{
	if ( info )
	{
		mVersionMade = info->versionMade;
		mVersionNeeded = info->versionNeeded;
		mCompressionMethod = info->compressionMethod;
		mFlags = info->bitFlags;
		mLastModifiedDate = info->lastModDate;
		mLastModifiedTime = info->lastModTime;
		mCrc32 = info->crc32;
		mCompressedSize = info->compressedSize;
		mUncompressedSize = info->uncompressedSize;
		mInternalAttribs = info->internalAttribs;
		mExternalAttribs = info->externalAttribs;
		mOffsetOfLocalHeader = info->offsetLocalHeader;
		mOffsetOfLocalFileData = -1;	// needs to be calculated later
		mDiskNoStart = info->diskNoStart;
	}
	else
	{
		Clear();
	}
}

ZipFileEntry::~ZipFileEntry()
{
}

void ZipFileEntry::Clear()
{
	mFileName.clear();
	mFileComment.clear();
	mLastModifiedDate = 0;
	mLastModifiedTime = 0;

	mVersionMade = 0;
	mVersionNeeded = 0;
	mCompressionMethod = 0;
	mFlags = 0;
	mCrc32 = 0;
	mCompressedSize = 0;
	mUncompressedSize = 0;
	mInternalAttribs = 0;
	mExternalAttribs = 0;
	mOffsetOfLocalHeader = 0;
	mOffsetOfLocalFileData = 0;
	mDiskNoStart = 0;
}

QString ZipFileEntry::GetCompressionMethodName() const
{
	return getCompressionMethodDescription( GetCompressionMethod() );
}

bool ZipFileEntry::IsDirectory() const
{
	return (GetAttributes() & FILE_ATTRIBUTE_DIRECTORY) != 0;
}
	
bool ZipFileEntry::IsHidden() const
{
	return (GetAttributes() & FILE_ATTRIBUTE_HIDDEN) != 0;
}

bool ZipFileEntry::IsSystem() const
{
	return (GetAttributes() & FILE_ATTRIBUTE_SYSTEM) != 0;
}

bool ZipFileEntry::IsReadOnly() const
{
	return (GetAttributes() & FILE_ATTRIBUTE_READONLY) != 0;
}

DWORD ZipFileEntry::GetAttributes() const
{
	//If this is compatible with MS-DOS then the upper byte should be 0
	//For UNIX (3) the low-order byte of the file attributes (read-only, hidden, system, directory) 
	//is compatible
	if ( HIBYTE(mVersionMade) == 0 )
	{
		return mExternalAttribs;
	}
	else if ( HIBYTE(mVersionMade) == 3 )
	{
		return (DWORD) LOWORD(mExternalAttribs);
	}

	return FILE_ATTRIBUTE_ARCHIVE | FILE_ATTRIBUTE_NORMAL;
}

bool ZipFileEntry::IsEncrypted() const
{ 
	//Bit 1 of the general purpose flag
	return mFlags & 0x1;
}

bool ZipFileEntry::HasCrcAndSizes() const
{
	//Bit 3 of the general purpose flag	
	return (mFlags & 0x8) == 0;
}

bool ZipFileEntry::HasUtf8Encoding() const
{
	//Bit 11 of the general purpose flag	
	return (mFlags & 0x800) != 0;
}

bool ZipFileEntry::GetExtraField(int fieldID, MemBuffer& fieldBuffer) const
{
	bool result = false;
	fieldBuffer.Destroy();
	fieldBuffer.SetByteOrder(MemBuffer::LittleEndian);

	if ( mExtraField.TotalSize() < 0 )
	{
		return false;
	}
	
	MemBuffer buffer((u_char*) mExtraField.GetBufferPtr(), mExtraField.TotalSize(), false);
	buffer.SetByteOrder( MemBuffer::LittleEndian );

	do 
	{
		uint16_t id = buffer.getUShort();
		uint16_t size = buffer.getUShort();

		if ( id == fieldID )
		{
			if ( buffer.Remaining() >= size )
			{
				fieldBuffer.Assign( buffer.GetReadPtr(), size, false );
				result = true;
			}

			//Done				
			break;
		}

		if ( !buffer.Seek(size, SEEK_CUR) )
		{
			//Ooops.. invalid format
			break;
		}

	} while ( buffer.Remaining() > 0 );

	return result;
}

__int64 ZipFileEntry::GetOffsetOfLocalHeader() const
{
	return mOffsetOfLocalHeader;
}

__int64 ZipFileEntry::GetOffsetOfFileData() const
{
	return mOffsetOfLocalFileData;
}

void ZipFileEntry::setExtraField( MemBuffer& buffer )
{
	//Used by ZipFileInfo, take ownership
	mExtraField.Assign( buffer.GetBufferPtr(), buffer.TotalSize(), true );
	buffer.Release();
}

struct stIdToDescription
{
	unsigned short ID;
	const char* description;
};

stIdToDescription ExtraFieldDescriptionList[] =
{
	//-- PKWARE mappings --//

	{0x0001, "Zip64 extended information extra field"},
	{0x0007, "AV Info"},
	{0x0008, "Reserved for extended language encoding data (PFS)"},
	{0x0009, "OS/2"},
	{0x000a, "NTFS "},
	{0x000c, "OpenVMS"},
	{0x000d, "UNIX"},
	{0x000e, "Reserved for file stream and fork descriptors"},
	{0x000f, "Patch Descriptor"},
	{0x0014, "PKCS#7 Store for X.509 Certificates"},
	{0x0015, "X.509 Certificate ID and Signature for individual file"},
	{0x0016, "X.509 Certificate ID for Central Directory"},
	{0x0017, "Strong Encryption Header"},
	{0x0018, "Record Management Controls"},
	{0x0019, "PKCS#7 Encryption Recipient Certificate List"},
	{0x0065, "IBM S/390 (Z390), AS/400 (I400) attributes - uncompressed"},
	{0x0066, "Reserved for IBM S/390 (Z390), AS/400 (I400) attributes - compressed"},
	{0x4690, "POSZIP 4690 (reserved)"},

	//-- Third party mappings --//

    {0x07c8, "Macintosh"},
    {0x2605, "ZipIt Macintosh"},
    {0x2705, "ZipIt Macintosh 1.3.5+"},
    {0x2805, "ZipIt Macintosh 1.3.5+"},
    {0x334d, "Info-ZIP Macintosh"},
    {0x4341, "Acorn/SparkFS"},
    {0x4453, "Windows NT security descriptor (binary ACL)"},
    {0x4704, "VM/CMS"},
    {0x470f, "MVS"},
    {0x4b46, "FWKCS MD5 (see below)"},
    {0x4c41, "OS/2 access control list (text ACL)"},
    {0x4d49, "Info-ZIP OpenVMS"},
    {0x4f4c, "Xceed original location extra field"},
    {0x5356, "AOS/VS (ACL)"},
    {0x5455, "Extended timestamp"},
    {0x554e, "Xceed unicode extra field"},
    {0x5855, "Info-ZIP UNIX (original, also OS/2, NT, etc)"},
    {0x6375, "Info-ZIP Unicode Comment Extra Field"},
    {0x6542, "BeOS/BeBox"},
    {0x7075, "Info-ZIP Unicode Path Extra Field"},
    {0x756e, "ASi UNIX"},
    {0x7855, "Info-ZIP UNIX (new)"},
    {0xa220, "Microsoft Open Packaging Growth Hint"},
    {0xfd4a, "SMS/QDOS"}
};

stIdToDescription CompressionMethodDescription[] =
{
	{ 0, "Stored (no compression)"},
    { 1, "Shrunk"},
    { 2, "Reduced with factor 1"},
    { 3, "Reduced with factor 2"},
    { 4, "Reduced with factor 3"},
    { 5, "Reduced with factor 4"},
    { 6, "Imploded"},
    { 7, "Tokenizing compression algorithm"},
    { 8, "Deflated"},
    { 9, "Enhanced Deflating using Deflate64(tm)"},
    { 10, "PKWARE Data Compression Library Imploding (old IBM TERSE)"},
    //{ 11, "Reserved by PKWARE"},
    { 12, "File is compressed using BZIP2 algorithm"},
    //{ 13, "Reserved by PKWARE"},
    { 14, "LZMA (EFS)"},
    //{ 15, "Reserved by PKWARE"},
    //{ 16, "Reserved by PKWARE"},
    //{ 17, "Reserved by PKWARE"},
    { 18, "IBM TERSE (new)"},
    { 19, "IBM LZ77 z Architecture (PFS)"},
    { 97, "WavPack"},
    { 98, "PPMd version I, Rev 1"}
};

QString getExtraFieldDescription( unsigned short ID )
{
	for ( int i = 0; i < _countof(ExtraFieldDescriptionList); i++ )
	{
		stIdToDescription* desc = &ExtraFieldDescriptionList[i];
		if ( desc->ID == ID )
		{
			return QString(desc->description);
		}
	}

	QString res;
	res = QString("Extra field ID 0x%1").arg(QString::number(ID, 16));
	return res;
}

QString getCompressionMethodDescription( unsigned short ID )
{
	for ( int i = 0; i < _countof(CompressionMethodDescription); i++ )
	{
		stIdToDescription* desc = &CompressionMethodDescription[i];
		if ( desc->ID == ID )
		{
			return QString(desc->description);
		}
	}

	QString res;
	res = QString("Compression method %1").arg(ID);
	return res;
}

QString getExtraFieldsDescription( const MemBuffer& extraField )
{
	if ( extraField.Remaining() == 0 )
	{
		return "No extra field";
	}
	else if ( extraField.Remaining() < 4 )
	{
		return "Invalid extra field";
	}

	QString result;
	MemBuffer buffer((u_char*) extraField.GetReadPtr(), extraField.Remaining(), false);
	buffer.SetByteOrder( MemBuffer::LittleEndian );

	do 
	{
		uint16_t id = buffer.getUShort();
		uint16_t size = buffer.getUShort();

		QString extra;
		extra = QString("0x%1 (%2) %3").arg(QString::number(id, 16)).arg(sizeToString(size)).arg(getExtraFieldDescription(id));

		if ( !result.isEmpty() )
			result.append("\n");

		result.append(extra);

		if ( !buffer.Seek(size, SEEK_CUR) )
		{

			//Ooops.. invalid format
			break;
		}
	
	} while ( buffer.Remaining() > 0 );

	return result;
}

bool ZipFileInfo::ValidationTest()
{
#ifdef _DEBUG

	bool result = true;

	//We have our central dir loaded
	//Verify the information is redundant
	CoreFile file(mFileName);
	if (!file.exists() || !file.open(QIODevice::ReadOnly | QIODevice::Append))
		return false;

	//Ok we are done
	ZipFileEntryList::iterator it = mEntries.begin();

	while ( it != mEntries.end() )
	{
		MemBuffer buffer(LocalFileHeaderFixedPartSize);

		const ZipFileEntry* entry = *it;

		//No compression
		if ( entry->IsCompressed() )
		{
			Output( QString("File %1 is compressed. %2 (%3 / %4).\n").arg(entry->GetFileName()).arg(getCompressionMethodDescription(entry->GetCompressionMethod()))
											   			  .arg(sizeToString(entry->GetCompressedSize()))
														  .arg(sizeToString(entry->GetUncompressedSize())) );
			result = false;
		}

		//Load the files local headers
		if ( SafeReadFile(file, buffer.GetBufferPtr(), LocalFileHeaderFixedPartSize, entry->GetOffsetOfLocalHeader()) )
		{
			stLocalFileHeader info;

			//Ok we have the buffer then read it!
			if ( readLocalFileHeader(info, buffer) )
			{
				int encoding = ((info.bitFlags & 0x800) != 0) ? CP_UTF8 : CP_OEMCP;

				QString fileName = loadString(file, info.filenameLength, encoding);

				MemBuffer extraFieldBuffer(info.extraFieldLength);
				SafeReadFile(file, extraFieldBuffer.GetBufferPtr(), extraFieldBuffer.TotalSize());

				bool outExtra = false;

				//We have the local file header, compare with existing data
				if ( info.extraFieldLength != entry->GetExtraFields().TotalSize() )
				{
					//Ok sizes don't match but there may be the case that ZipFileInfo already knows this, 
					//as for the Zip64 extended information extra field. Check again
					uint32_t expectedHeaderSize = (uint32_t) (entry->GetOffsetOfFileData() - entry->GetOffsetOfLocalHeader());
					uint32_t actualHeaderSize = ZipFileInfo::LocalFileHeaderFixedPartSize + info.filenameLength + info.extraFieldLength;

					if ( actualHeaderSize != expectedHeaderSize )
					{
						Output( QString("Extra info size mismatch for %1.\n").arg(entry->GetFileName()) );
						Output( QString("%1 on central dir and %2 on local header\n.").arg(sizeToString(entry->GetExtraFields().TotalSize()))  
																			   .arg(sizeToString(info.extraFieldLength)) );
						result = false;
						outExtra = true;
					}
				}
				else if ( memcmp(extraFieldBuffer.GetBufferPtr(), entry->GetExtraFields().GetBufferPtr(), entry->GetExtraFields().TotalSize()) != 0 )
				{
					Output( QString("Extra info data mismatch for %1.\n").arg(entry->GetFileName()) );
					result = false;
					outExtra = true;
				}

				if ( outExtra )
				{
					QString central = getExtraFieldsDescription(entry->GetExtraFields());
					QString local = getExtraFieldsDescription(extraFieldBuffer);

					Output( QString("Extra field on central dir:\n%1\n").arg(central) );
					Output( QString("Extra field on local header:\n%1\n").arg(local) );
				}

				if ( info.filenameLength != entry->GetFileName().length() ||
					 fileName != entry->GetFileName() )
				{
					Output( QString("Filename mismatch for %1. Local header says: %2\n").arg(entry->GetFileName()).arg(fileName) );
					result = false;
				}
			}
			else
			{
				Output( QString("Could not read file header for %1\n").arg(entry->GetFileName()) );
				result = false;
			}
		}
		else
		{
			Output( QString("Could not read %1\n").arg(entry->GetFileName()) );
			result = false;
		}

		it++;
	}

	if ( result )
	{
		Output( QString("ZIP file valid!!\n") );
	}

	return result;

#else
	return true;
#endif
}

