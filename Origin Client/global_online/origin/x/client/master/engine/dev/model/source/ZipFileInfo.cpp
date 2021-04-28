/**
ZipFileInfo and ZipFileEntry implementations
*/

#include "ZipFileInfo.h"
#include <limits>
#include "services/debug/DebugService.h" //debug_assert
#include "services/log/LogService.h"

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

#define LOWORD(l)   ((quint16)(((quint32)(l)) & 0xffff))
#define HIWORD(l)   ((quint16)((((quint32)(l)) >> 16) & 0xffff))
#define LOBYTE(w)   ((uchar)(((quint32)(w)) & 0xff))
#define HIBYTE(w)   ((uchar)((((quint32)(w)) >> 8) & 0xff))

// from WinNT.h
#define FILE_ATTRIBUTE_READONLY             0x00000001  
#define FILE_ATTRIBUTE_HIDDEN               0x00000002  
#define FILE_ATTRIBUTE_SYSTEM               0x00000004  
#define FILE_ATTRIBUTE_DIRECTORY            0x00000010  
#define FILE_ATTRIBUTE_ARCHIVE              0x00000020  
#define FILE_ATTRIBUTE_DEVICE               0x00000040  
#define FILE_ATTRIBUTE_NORMAL               0x00000080 

#define COUNT_ELEMENTS(arr) (sizeof(arr) / sizeof((arr)[0]))


// from sys/stat.h
/* File type */
#define	S_IFMT		0170000		/* [XSI] type of file mask */
#define	S_IFDIR		0040000		/* [XSI] directory */
#define	S_IFREG		0100000		/* [XSI] regular */
#define	S_IFLNK		0120000		/* [XSI] symbolic link */

/* File mode */
/* Read, write, execute/search by owner */
#define	S_IRWXU		0000700		/* [XSI] RWX mask for owner */
#define	S_IRUSR		0000400		/* [XSI] R for owner */
#define	S_IWUSR		0000200		/* [XSI] W for owner */
#define	S_IXUSR		0000100		/* [XSI] X for owner */
/* Read, write, execute/search by group */
#define	S_IRWXG		0000070		/* [XSI] RWX mask for group */
#define	S_IRGRP		0000040		/* [XSI] R for group */
#define	S_IWGRP		0000020		/* [XSI] W for group */
#define	S_IXGRP		0000010		/* [XSI] X for group */
/* Read, write, execute/search by others */
#define	S_IRWXO		0000007		/* [XSI] RWX mask for other */
#define	S_IROTH		0000004		/* [XSI] R for other */
#define	S_IWOTH		0000002		/* [XSI] W for other */
#define	S_IXOTH		0000001		/* [XSI] X for other */

namespace Origin
{
namespace Downloader
{

/**
Read from a file, optionally seeking to a specific place. Catch exceptions.;
*/
static bool SafeReadFile(QFile& f, void* lpBuf, quint32 nCount,qint64 offsetFromBegin = -1)
{
	bool result = false;

	//Seek
	/*if ( offsetFromBegin >= 0 )
	{
		if ( f.Seek(offsetFromBegin, QFile::kSeekStart) != offsetFromBegin )
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

static QString sizeToString(qint64 bytes )
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
	ORIGIN_LOG_DEBUG << line;
}

//Forward declarations
static QString getCompressionMethodDescription( quint16 ID );
#ifdef _DEBUG
static QString getExtraFieldsDescription( const MemBuffer& extraField );
static QString getExtraFieldDescription( quint16 ID );
#endif

//Class static const members
const quint32 ZipFileInfo::ZipEndOfCentralDirSignature = 0x06054b50;
const quint32 ZipFileInfo::Zip64EndOfCentralDirSignature = 0x06064b50;
const quint32 ZipFileInfo::Zip64EndOfCentralDirLocatorSignature = 0x07064b50;
const quint32 ZipFileInfo::ZipEndOfCentralDirFixedPartSize = 22;
const quint32 ZipFileInfo::Zip64EndOfCentralDirFixedPartSize = 56;
const quint32 ZipFileInfo::Zip64EndOfCentralDirLocatorSize = 20;
const quint32 ZipFileInfo::FileHeaderSignature = 0x02014b50;
const quint32 ZipFileInfo::FileHeaderFixedPartSize = 46;
const quint32 ZipFileInfo::LocalFileHeaderSignature = 0x04034b50;
const quint32 ZipFileInfo::LocalFileHeaderFixedPartSize = 30;

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
		max_namel = qMax(max_namel, (*it)->GetFileName().length());
		max_sizel = qMax(max_sizel, sizeToString((*it)->GetUncompressedSize()).length());
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

	/*File file;
	if (file.Open(filename, File::kOpenRead, File::kCDOpenExisting))
		result = Load( file );
	else
		Output( QString("Unable to open %1.").arg(filename) );

	return result;*/

	QFile file(filename);
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
static qint64 findSignature( QFile& file, quint32 signature, quint32 backStart = 0, quint32 uMaxBack = ULONG_MAX )
{
	const int READBLOCK = 1024;

	uchar buf[READBLOCK + 4];
	qint64 uSizeFile = file.size();
	qint64 uBackRead;
	qint64 uPosFound = 0;
	qint64 uReadPos;
	quint32 uReadSize;

	//Walk backwards and try to find the signature
    
	uBackRead = backStart + 4;

	uchar sig[4] = { LOBYTE(LOWORD(signature)), HIBYTE(LOWORD(signature)),
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
		uReadSize = (quint32) READBLOCK + 4;

		//Clamp size if we are reaching the beginning (shouldn't happen)
		if ( uSizeFile - uReadPos < uReadSize )
			uReadSize = (quint32) (uSizeFile - uReadPos);
		
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
static qint64 findSignature( const uchar* buf, quint32 size, quint32 signature )
{
	qint64 uPosFound = -1;

	const uchar sig[4] = { LOBYTE(LOWORD(signature)), HIBYTE(LOWORD(signature)),
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
static qint64 findSignature( MemBuffer& memBuffer, quint32 signature, quint32 backStart = 0, quint32 uMaxBack = ULONG_MAX )
{
	qint64 uPosFound = -1;

	if ( memBuffer.TotalSize() < 4 )
		return uPosFound;

	if ( backStart >= memBuffer.TotalSize() - 4 )
		return uPosFound;

	if ( uMaxBack > memBuffer.TotalSize() )
		uMaxBack = memBuffer.TotalSize();

	if ( backStart > uMaxBack )
		return uPosFound;
	
	//Get the starting pointer
	quint32 offset = memBuffer.TotalSize() - uMaxBack;

	const uchar* ptr = memBuffer.GetBufferPtr() + offset;

	uPosFound = findSignature( ptr, uMaxBack - backStart, signature );

	if ( uPosFound >= 0 )
	{
		uPosFound += offset;
	}

	return uPosFound;
}

bool ZipFileInfo::Load( QFile& file )
{
	bool result = false;

	//Walk backwards and try to find the end of central directory
	//signature 4 bytes: 0x06054b50
	qint64 uPosFound = findSignature(file, ZipEndOfCentralDirSignature, 0, 0xffff /* maximum size of global comment */);

	//We have the end of central dir
	if ( uPosFound != 0 )
	{
		uchar headBuf[ZipEndOfCentralDirFixedPartSize];
		EndOfCentralDir info;

		MemBuffer endOfCentralBuffer(headBuf, sizeof(headBuf));
		if ( SafeReadFile(file, headBuf, COUNT_ELEMENTS(headBuf), uPosFound) &&
		     readEndOfCentralDir(info, endOfCentralBuffer))
		{
			// set number of discs from EndOfCentralDir
			mNumOfDiscs = info.diskNo + 1;

			//Read the file comment
			if ( info.commentLength > 0 )
			{
				mComment = loadString(file, info.commentLength, MemBuffer::kCodePageANSI);
			}

			//Now check if we need to find a Zip64 end of central dir instead
			if ( info.offsetToCentralDir == 0xffffffff )
			{
				//Ok find the locator and then the Zip64 EOCD
				qint64 fileLength = file.size();
				ORIGIN_ASSERT(fileLength>=0);
				if (fileLength == -1)	// file error
					fileLength = 0;

				uPosFound = findSignature(file, Zip64EndOfCentralDirLocatorSignature, (quint32) (fileLength - uPosFound));

				if ( uPosFound != 0 )
				{
					uchar locatorBuf[Zip64EndOfCentralDirLocatorSize];
					Zip64EndOfCentralDirLocator locator;

					//Read the Zip64 end of central directory locator
					MemBuffer zip64Buffer(locatorBuf, sizeof(locatorBuf));
					if ( SafeReadFile(file, locatorBuf, COUNT_ELEMENTS(locatorBuf), uPosFound) &&
						 readZip64EndOfCentralDirLocator(locator, zip64Buffer)) 
					{
						uchar head64Buf[Zip64EndOfCentralDirFixedPartSize];

						//Read the Zip64 end of central directory
						MemBuffer stZip64Buffer(head64Buf, sizeof(head64Buf));
						if ( SafeReadFile(file, head64Buf, COUNT_ELEMENTS(head64Buf), locator.offsetToZip64EndOfCentralDir) &&
							 readEndOfCentralDir(info, stZip64Buffer))
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

bool ZipFileInfo::TryLoad( MemBuffer& buffer,qint64 fileSize,qint64* offsetToStart )
{
	const quint32 MAX_CENTRAL_DIR_SCAN_SIZE = 5 * 1024 * 1024;	// there isn't any documented max size, but typically the central directory is a few hundred k max.  This is for crash prevention in the case of a badly formed zip file.

	bool result = false;
	qint64 dummy = 0;
	
	//Point to dummy stack var if unavailable
	if ( offsetToStart == NULL )
		offsetToStart = &dummy;

	*offsetToStart = 0;

	qint64 uPosFound = findSignature(buffer, ZipEndOfCentralDirSignature, 0, 0xffff /* maximum size of global comment */);

	//We have the end of central dir
	if ( uPosFound >= 0 )
	{
		EndOfCentralDir eocdInfo;
		
		buffer.Seek((qint32) uPosFound, SEEK_SET);

		if ( readEndOfCentralDir(eocdInfo, buffer) )
		{
			// set number of discs from EndOfCentralDir
			mNumOfDiscs = eocdInfo.diskNo + 1;

			//Read the file comment, if any
			if ( eocdInfo.commentLength > 0 )
			{
				//buffer.Seek((qint32) uPosFound + ZipEndOfCentralDirFixedPartSize, SEEK_SET);
				mComment = buffer.ReadString(eocdInfo.commentLength, MemBuffer::kCodePageANSI);
			}

			//Now check if we need to find a Zip64 end of central dir instead
			if ( eocdInfo.offsetToCentralDir == 0xffffffff )
			{
				//Ok find the locator and then the Zip64 EOCD, starting at where we found the end of central dir
				uPosFound = findSignature(buffer, Zip64EndOfCentralDirLocatorSignature, (quint32)(buffer.TotalSize() - uPosFound));

				if ( uPosFound != 0 )
				{
					Zip64EndOfCentralDirLocator locator;
					MemBuffer newBuffer(buffer.GetBufferPtr() + uPosFound, Zip64EndOfCentralDirLocatorSize);
					//Read the Zip64 end of central directory locator
					if ( readZip64EndOfCentralDirLocator(locator, newBuffer))
					{
						qint64 pos = locator.offsetToZip64EndOfCentralDir - (fileSize - buffer.TotalSize());

						if ( pos >= 0 )
						{ 
							MemBuffer tempMemBuffer(buffer.GetBufferPtr() + pos, Zip64EndOfCentralDirFixedPartSize);
							if ( !readEndOfCentralDir(eocdInfo, tempMemBuffer) )
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
						qint64 wantMore = qMin(fileSize - buffer.TotalSize(), (qint64)1024);
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
					qint64 pos = eocdInfo.offsetToCentralDir - (fileSize - buffer.TotalSize());
					MemBuffer tmpBuffer( buffer.GetBufferPtr() + pos, (quint32) eocdInfo.centralDirSize, false );

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
			qint64 wantMore = qMin(fileSize - buffer.TotalSize(), (qint64)1024);
			*offsetToStart = fileSize - buffer.TotalSize() - wantMore;
		}
	}

	return result;
}

quint32 ZipFileInfo::GetNumberOfFiles() const
{
	quint32 count = 0;
	ZipFileEntryList::const_iterator it = mEntries.begin();

	while ( it != mEntries.end() )
	{
		if ( !(*it)->IsDirectory() )
			count++;
		
		it++;
	}

	return count;
}

quint32 ZipFileInfo::GetNumberOfDirectories() const
{
	quint32 count = 0;
	ZipFileEntryList::const_iterator it = mEntries.begin();

	while ( it != mEntries.end() )
	{
		if ( (*it)->IsDirectory() )
			count++;
		
		it++;
	}

	return count;
}

qint64 ZipFileInfo::GetUncompressedSize() const
{
	qint64 size = 0;
	ZipFileEntryList::const_iterator it = mEntries.begin();

	while ( it != mEntries.end() )
	{
		size += (*it)->GetUncompressedSize();
		it++;
	}

	return size;
}

bool ZipFileInfo::loadCentralDir( const EndOfCentralDir* info, QFile& f )
{
	bool result = false;

	MemBuffer buffer((quint32) info->centralDirSize);

	if ( !buffer.IsEmpty() &&
		 SafeReadFile(f, buffer.GetBufferPtr(), buffer.TotalSize(), info->offsetToCentralDir) )
	{
		result = loadCentralDir(info, buffer);
	}
	
	return result;
}

QString ZipFileInfo::loadString(QFile& file, int len, MemBuffer::CodePage codepage)
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

QString ZipFileInfo::loadString(const QString& str, int len, MemBuffer::CodePage codepage)
{
	QString dst;

	if ( !str.isEmpty() && len > 0 )
	{
		MemBuffer tmpBuffer((uchar*)str.constData(), len, false); //The cast is safe, we will only be reading
		dst = tmpBuffer.ReadString(len, codepage);
	}

	return dst;
}

bool ZipFileInfo::loadCentralDir( const EndOfCentralDir* info, MemBuffer& buffer )
{
	bool result = true;

	for ( int i = 0; i < info->entriesTotal && result; i++ )
	{
		//We don't just 'point' the pointer because of the endian compatibility
		FileHeader header;
		
		result = readFileHeader(header, buffer);
		
		if ( result )
		{
			mOffsetToCentralDir = info->offsetToCentralDir;

			ZipFileEntry* entry = new ZipFileEntry(&header);

			quint32 variableSize = header.filenameLength + 
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
				MemBuffer::CodePage encoding = (entry->HasUtf8Encoding()) ? MemBuffer::kCodePageUTF8 : MemBuffer::kCodePageOEM;

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
							qint64 val = buffer.getInt64();
							entry->mUncompressedSize = val;
						}

						if ( entry->GetCompressedSize() == 0xffffffff )
						{
							qint64 val = buffer.getInt64();
							entry->mCompressedSize = val;
						}

						if ( entry->GetOffsetOfLocalHeader() == 0xffffffff )
						{
							qint64 val = buffer.getInt64();
							entry->mOffsetOfLocalHeader = val;
						}

						//Disk number follows but we don't support that
					}

					//Unicode filename
					if ( entry->GetExtraField(0x7075, buffer) )
					{
						uchar version = buffer.getUChar();
						
						if ( version == 1 )
						{							
							quint32 nameCRC32 = buffer.getULong();(void)nameCRC32; //suppress warning

							QString szName = buffer.ReadString( buffer.Remaining(), MemBuffer::kCodePageUTF8 );
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

		qint64 nOffsetToFileData;
		// special case for last entry
		if (itNext == mEntries.end())
		{
			nOffsetToFileData = mOffsetToCentralDir - (*it)->mCompressedSize;

			// for ITO builds the central directory is its own file, and mOffsetToCentral dir is 0
			// so the nOffsetToFileData will be less than zero. We don't want it to assert in this case
			//we will calculate the proper offset in CoreDownloadJob_Zip::RetrieveOffsetToFileData in this case

			if(mOffsetToCentralDir)
			{
				ORIGIN_ASSERT(nOffsetToFileData > 0);
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
			ORIGIN_ASSERT(nOffsetToFileData > 0);
		}

		(*it)->mOffsetOfLocalFileData = nOffsetToFileData;

	}

	return result;
}

bool ZipFileInfo::readEndOfCentralDir( EndOfCentralDir& info, MemBuffer& data )
{
	bool ok = false;

	//Zero out the struct
	memset(&info, 0, sizeof(info));

	//Check
	if ( data.Remaining() < qMin(ZipEndOfCentralDirFixedPartSize, Zip64EndOfCentralDirFixedPartSize) )
		return false;

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
		info.commentLength = data.getUShort();
	}
	else if ( info.signature == ZipFileInfo::Zip64EndOfCentralDirSignature ) //Zip64
	{
		//Skip size of this record as we are not using the extensible data sector
		qint64 sizeOfRecord = data.getInt64();

		//As per the documentation this value 
		//should not include the leading 12 bytes
		//SizeOfFixedFields + SizeOfVariableData - 12

		//Should be at least the fixed size, assuming no variable data is present
		if ( sizeOfRecord >= (Zip64EndOfCentralDirFixedPartSize - 12) ) 
		{
			quint16 versionMadeBy = data.getUShort();(void)versionMadeBy; //suppress warning
			quint16 versionNeededToExtract = data.getUShort();(void)versionNeededToExtract; //suppress warning
			info.diskNo = data.getULong();
			info.diskWithCentralDir = data.getULong();
			info.entriesThisDisk = data.getInt64();
			info.entriesTotal = data.getInt64();
			info.centralDirSize = data.getInt64();
			ok = data.Remaining() >= 8;
			info.offsetToCentralDir = data.getInt64();
			info.commentLength = 0;
		}
	}
	else
	{
		//unrecognized
	}

	return ok;
}

bool ZipFileInfo::readZip64EndOfCentralDirLocator( Zip64EndOfCentralDirLocator& locator, MemBuffer& data )
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

bool ZipFileInfo::readFileHeader( FileHeader& info, MemBuffer& data )
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

bool ZipFileInfo::readLocalFileHeader( LocalFileHeader& info, MemBuffer& data )
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

ZipFileEntry::ZipFileEntry(const FileHeader* info)
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
    if (GetAttributeFormat() == ZIP_ATTRFORMAT_DOS)
    {
        return (GetDOSAttributes() & FILE_ATTRIBUTE_DIRECTORY);
    }
    else if (GetAttributeFormat() == ZIP_ATTRFORMAT_UNIX)
    {
        return (GetUnixAttributes() & S_IFDIR);
    }
    
    // This is a fallback, there is usually DOS-compatible attribute information
	return (GetDOSAttributes() & FILE_ATTRIBUTE_DIRECTORY) != 0;
}

	/*
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
}*/

quint32 ZipFileEntry::GetDOSAttributes() const
{
	//If this is compatible with MS-DOS then the upper byte should be 0
	//For UNIX (3) the low-order byte of the file attributes (read-only, hidden, system, directory) 
	//is compatible
    quint8 attrFormat = GetAttributeFormat();
    
	if ( attrFormat == ZIP_ATTRFORMAT_DOS )
	{
		return mExternalAttribs;
	}
	else if ( attrFormat == ZIP_ATTRFORMAT_UNIX )
	{
		return (quint32) LOWORD(mExternalAttribs);
	}

	return FILE_ATTRIBUTE_ARCHIVE | FILE_ATTRIBUTE_NORMAL;
}
    
quint32 ZipFileEntry::GetUnixAttributes() const
{
    //If this is compatible with MS-DOS then the upper byte should be 0
	//For UNIX (3) the low-order byte of the file attributes (read-only, hidden, system, directory) 
	//is compatible
    quint8 attrFormat = GetAttributeFormat();
    
    if ( attrFormat == ZIP_ATTRFORMAT_UNIX )
	{
		return (quint32) HIWORD(mExternalAttribs);
	}
    
    // This is not a UNIX-compatible archive, so we'll synthesize the attributes from the DOS attributes
    quint32 unixAttrs = 0;
    
    if (GetDOSAttributes() & FILE_ATTRIBUTE_DIRECTORY)
    {
        unixAttrs |= S_IFDIR;
    }
    else
    {
        unixAttrs |= S_IFREG;
    }
    
    // Read-write-execute for everyone
    unixAttrs |= (S_IRWXU | S_IRWXG | S_IRWXO);
    
    return unixAttrs;
}
    
quint8 ZipFileEntry::GetAttributeFormat() const
{
    return HIBYTE(mVersionMade);
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

	MemBuffer buffer((uchar*) mExtraField.GetBufferPtr(), mExtraField.TotalSize(), false);
	buffer.SetByteOrder( MemBuffer::LittleEndian );

	do 
	{
		quint16 id = buffer.getUShort();
		quint16 size = buffer.getUShort();

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

qint64 ZipFileEntry::GetOffsetOfLocalHeader() const
{
	return mOffsetOfLocalHeader;
}

qint64 ZipFileEntry::GetOffsetOfFileData() const
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
	quint16 ID;
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

#ifdef _DEBUG
static QString getExtraFieldDescription( quint16 ID )
{
	for ( size_t i = 0; i < COUNT_ELEMENTS(ExtraFieldDescriptionList); i++ )
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
#endif

static QString getCompressionMethodDescription( quint16 ID )
{
	for ( size_t i = 0; i < COUNT_ELEMENTS(CompressionMethodDescription); i++ )
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

#ifdef _DEBUG
static QString getExtraFieldsDescription( const MemBuffer& extraField )
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
	MemBuffer buffer((uchar*) extraField.GetReadPtr(), extraField.Remaining(), false);
	buffer.SetByteOrder( MemBuffer::LittleEndian );

	do 
	{
		quint16 id = buffer.getUShort();
		quint16 size = buffer.getUShort();

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
#endif

bool ZipFileInfo::ValidationTest()
{
#ifdef _DEBUG

	bool result = true;

	//We have our central dir loaded
	//Verify the information is redundant
	QFile file(mFileName);
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
			LocalFileHeader info;

			//Ok we have the buffer then read it!
			if ( readLocalFileHeader(info, buffer) )
			{
				MemBuffer::CodePage encoding = ((info.bitFlags & 0x800) != 0) ? MemBuffer::kCodePageUTF8 : MemBuffer::kCodePageOEM;

				QString fileName = loadString(file, info.filenameLength, encoding);

				MemBuffer extraFieldBuffer(info.extraFieldLength);
				SafeReadFile(file, extraFieldBuffer.GetBufferPtr(), extraFieldBuffer.TotalSize());

				bool outExtra = false;

				//We have the local file header, compare with existing data
				if ( info.extraFieldLength != entry->GetExtraFields().TotalSize() )
				{
					//Ok sizes don't match but there may be the case that ZipFileInfo already knows this, 
					//as for the Zip64 extended information extra field. Check again
					quint32 expectedHeaderSize = (quint32) (entry->GetOffsetOfFileData() - entry->GetOffsetOfLocalHeader());
					quint32 actualHeaderSize = ZipFileInfo::LocalFileHeaderFixedPartSize + info.filenameLength + info.extraFieldLength;

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

} // namespace Downloader
} // namespace Origin

