// BigFileInfo and BigFileEntry implementations

#include "BigFileInfo.h"
#include "services/log/LogService.h"

#include <QString>

namespace Origin
{
namespace Downloader
{

/** Struct used internally **/
class stBIGFileHeader
{
public:
	stBIGFileHeader()
	{
		memset(this, 0, sizeof(*this));
	}

	quint32 type;	//type of BIG file (BIG2..BIG5 etc)
	quint32 len;		//len of the file in bytes, may overflow for BIG5 > 4 GB
	quint32 num;		//number of files in BIG file
	quint32 hlen;	//length of the header including this field
};

/**
Read from a file, optionally seeking to a specific place. Catch exceptions.
*/
static bool SafeReadFile(QFile& f, void* lpBuf, quint32 nCount, qint64 offsetFromBegin = -1)
{
	bool result = false;

	//Seek
	if ( offsetFromBegin >= 0 )
	{
		if (!f.seek(offsetFromBegin))
			return false;
	}

	//Read
	result = (f.read(reinterpret_cast<char*>(lpBuf), nCount) == nCount);

	return result;
}

static QString sizeToString( qint64 bytes )
{
	QString result;

	if ( bytes < 1000 )
	{
		result.sprintf( "%d bytes", (int) bytes );
	}
	else if ( bytes < 1024 * 1000 )
	{
		result.sprintf( "%.2f KB", double(bytes) / 1024 );
	}
	else if ( bytes < 1024 * 1024 * 1000 )
	{
		result.sprintf( "%.2f MB", double(bytes) / (1024 * 1024) );
	}
	else
	{
		result.sprintf( "%.2f GB", double(bytes) / (1024 * 1024 * 1024) );
	}

	return result;
}

const quint32 BigFileInfo::BIG2Type = 0x42494732;
const quint32 BigFileInfo::BIG3Type = 0x42494733;
const quint32 BigFileInfo::BIG4Type = 0x42494734;
const quint32 BigFileInfo::BIG5Type = 0x42494735;
const quint32 BigFileInfo::c0fbType = 0x63306662;
const quint32 BigFileInfo::BIGFType = 0x42494746;
const quint32 BigFileInfo::FileHeaderSize = 16;

BigFileInfo::BigFileInfo()
{
	mHeaderLen = 0;
	mFileType = 0;
    mSizeTOC = 0;
	mSizeFile = 0;
}

BigFileInfo::~BigFileInfo()
{
	Clear();
}

void BigFileInfo::Clear()
{
	mFileName.clear();
	mNumEntries = 0;
	mFileType = 0;
    mHeaderLen = 0;
    mSizeTOC = 0;
	mSizeFile = 0;

	BigFileEntryList::iterator it = mEntries.begin();

	while ( it != mEntries.end() )
	{
		delete *it;
		it++;
	}

	mEntries.clear();
}

void BigFileInfo::Dump()
{
	//Dump standard data
	ORIGIN_LOG_DEBUG << "BIG archive type " << GetBIGFileTypeDescription(GetBIGFileType()) << " has "
			  << GetNumberOfEntries() << " entries (" << GetNumberOfFiles() << " files and " << GetNumberOfDirectories() << " dirs).\n";

	BigFileEntryList::iterator it = mEntries.begin();

	//Lets calculate the greatest filename length
	int max_namel = 0, max_sizel = 0;
	while ( it != mEntries.end() )
	{
		max_namel = qMax(max_namel, (*it)->GetFileName().length());
		max_sizel = qMax(max_sizel, sizeToString((*it)->GetSize()).length());
		it++;
	}

	QString szFormat;
	szFormat.sprintf("%%-%d.%ds | %%%d.%ds | %%s\n", max_namel + 1, max_namel + 1,
												     max_sizel + 1, max_sizel + 1);

	it = mEntries.begin();

	while ( it != mEntries.end() )
	{
		BigFileEntry* entry = *it;
		QString logStr;
		
		logStr.sprintf( qPrintable(szFormat), qPrintable(entry->GetFileName()), qPrintable(sizeToString(entry->GetSize())), entry->IsDirectory() ? "dir" : "file" );

		ORIGIN_LOG_DEBUG << logStr;

		it++;
	}
}

bool BigFileInfo::Load( const QString& filename )
{
	bool result = false;

	Clear();

	QFile file(filename);

	if ( file.exists() && file.open(QIODevice::ReadOnly | QIODevice::Append) )
		result = Load( file );
	else
		ORIGIN_LOG_ERROR << "Unable to open " << filename << ".";

	return result;
}

bool BigFileInfo::Load( QFile& file )
{
	bool result;
	stBIGFileHeader header;

	MemBuffer buffer(FileHeaderSize);

	//Read the main header
	result = SafeReadFile(file, buffer.GetBufferPtr(), FileHeaderSize);

	if ( result )
	{
		//Read the file header
		result = readBIGFileHeader(header, buffer);

		if ( result )
		{
			qint64 available = 0;
			quint32 required = GetHeaderSize(buffer.GetBufferPtr());

			available = file.size();

			if ( required > 0 && required <= available )
			{
				//Read the whole deal
				buffer.Create(required);
				
				result = SafeReadFile(file, buffer.GetBufferPtr(), required, 0);

				if ( result )
				{
					result = Load(buffer);
				}
			}
			else
			{
				result = false;
			}
		}
	}

	if ( result )
	{
		mFileName = file.fileName();
	}
	else
	{
		Clear();
	}

	return result;
}

bool BigFileInfo::Load( MemBuffer& buffer )
{
	if ( buffer.Remaining() < FileHeaderSize )
		return false;

	//Read the file header
	stBIGFileHeader header;
	
	bool result = readBIGFileHeader(header, buffer);

	//Check we have enough data
	result = result && (buffer.Remaining() >= (header.hlen - FileHeaderSize));

	if ( result )
	{
		//Save values
		mFileType = header.type;
		mSizeFile = header.len;			//Warning, this could overflow for BIG5 > 4GIG
		mNumEntries = header.num;
		mHeaderLen = header.hlen;

		//Header parsing loop
		while ( result && (mEntries.size() < mNumEntries) )
		{
			bool error = false;
			BigFileEntry* entry = readTOCFileHeader(buffer, error);

			if ( entry )
			{
				AddEntry(entry);
			}
			else
			{
				//may be error or
				//insufficient data, but we have the entire header (malformed?)
				result = false;
			}
		}
	}

	if ( !result )
	{
		Clear();
	}

	return result;
}

quint32 BigFileInfo::GetHeaderSize( quint8* bof )
{
	stBIGFileHeader header;
	MemBuffer buffer(bof, FileHeaderSize, false);

	//Read the file header
	bool result = readBIGFileHeader(header, buffer);

	return result ? header.hlen : 0; 
}

quint32 BigFileInfo::GetNumberOfFiles() const
{
	quint32 count = 0;
	BigFileEntryList::const_iterator it = mEntries.begin();

	while ( it != mEntries.end() )
	{
		if ( !(*it)->IsDirectory() )
			count++;

		it++;
	}

	return count;
}

quint32 BigFileInfo::GetNumberOfDirectories() const
{
	quint32 count = 0;
	BigFileEntryList::const_iterator it = mEntries.begin();

	while ( it != mEntries.end() )
	{
		if ( (*it)->IsDirectory() )
			count++;
		
		it++;
	}

	return count;
}

qint64 BigFileInfo::GetUnpackedSize() const
{
	qint64 size = 0;
	BigFileEntryList::const_iterator it = mEntries.begin();

	while ( it != mEntries.end() )
	{
		size += (*it)->GetSize();
		it++;
	}

	return size;
}

bool BigFileInfo::readBIGFileHeader( stBIGFileHeader& info, MemBuffer& data )
{
	bool ok = false;

	//Zero out the struct
	memset(&info, 0, sizeof(info));

	if ( data.Remaining() >= FileHeaderSize )
	{
		data.SetByteOrder( MemBuffer::BigEndian );

		//Get BIG file type
		info.type = data.getULong();

		//Check type is valid (recognized)
		ok = info.type == BIG2Type || info.type == BIG3Type ||
			 info.type == BIG4Type || info.type == BIG5Type ||
			 info.type == BIGFType || info.type == c0fbType;

		if ( ok )
		{
			info.len = data.getULong();
			info.num = data.getULong();
			info.hlen = data.getULong();
		}
	}

	return ok;
}

BigFileEntry* BigFileInfo::readTOCFileHeader( MemBuffer& data, bool& error ) const
{
	//bool ok = false;
	error = false;

	BigFileEntry* entry = NULL;

	if ( data.Remaining() >= 5 )
	{
		quint32 bufPos = data.Tell();

		data.SetByteOrder( MemBuffer::BigEndian );

		QString name;
		qint64 off = 0;
		quint32 size = 0;
		
		//TODO take type into account
		if ( IsBIG2() )
		{
			off = data.getUShort();
			size = data.getUShort();
		}
		else if ( (IsBIG3() || Isc0fb()) && data.Remaining() >= 6 )
		{
			quint8* buf = data.GetReadPtr();
			off = qint64(buf[2]) + (qint64(buf[1]) << 8) + (qint64(buf[0]) << 16);
			data.Seek(3, SEEK_CUR);
			buf = data.GetReadPtr();
			size = quint32(buf[2]) + (quint32(buf[1]) << 8) + (quint32(buf[0]) << 16);
			data.Seek(3, SEEK_CUR);
		}
		else if ( IsBIG4() && data.Remaining() >= 8 )
		{
			off = data.getULong();
			size = data.getULong();
		}
		else if ( IsBIG5() && data.Remaining() >= 9 )
		{
			//40 bit
			quint8* buf = data.GetReadPtr();
			off = qint64(buf[4]) + (qint64(buf[3]) << 8) + 
					(qint64(buf[2]) << 16) + (qint64(buf[1]) << 24) + 
					(qint64(buf[0]) << 32);
			data.Seek(5, SEEK_CUR);
			size = data.getULong();
		}

		//We have to have at least one character and the null terminator
		if ( off > 0 && data.Remaining() > 1 )
		{
			name = data.ReadString(-1, MemBuffer::kCodePageUTF8);

			if ( name.isEmpty() )
			{
				//This is an error condition, empty name, fishy
				error = true;
			}
		}

		if ( !name.isEmpty() && name.length() <= 512 )
		{
			entry = new BigFileEntry;
			
			entry->setOffset( off );
			entry->setSize( size );
			entry->setFileName( name );
		}
		else
		{
			if ( name.length() > 512 )
			{
				//This is an error condition. We should stop here
				error = true;
			}
		}

		if ( !entry )
		{
			//Restore the buffer pointer if we failed
			data.Seek(bufPos, SEEK_SET);
		}
	}

	return entry;
}

bool BigFileInfo::AddEntry( BigFileEntry* entry )
{
	mEntries.push_back(entry);
	return true;
}

QString BigFileInfo::GetBIGFileTypeDescription( quint32 type )
{
	QString szType;

	switch ( type )
	{
	case BIG2Type: 
		szType  = "BIG2";
		break;
	case BIG3Type: 
		szType  = "BIG3";
		break;
	case BIG4Type: 
		szType  = "BIG4";
		break;
	case BIG5Type: 
		szType  = "BIG5";
		break;
	case c0fbType: 
		szType  = "c0fb";
		break;
	case BIGFType: 
		szType  = "BIGF";
		break;
	default:
		szType = "Unknown";
	}

	return szType;
}

BigFileEntry::BigFileEntry()
{
	Clear();
}

BigFileEntry::~BigFileEntry()
{
}

void BigFileEntry::Clear()
{
	mFileName.clear();
	mSize = 0;
	mOffset = 0;
	mOffsetOfTOCHeader = 0;
}

bool BigFileEntry::IsValid() const
{
	return !mFileName.isEmpty();
}

bool BigFileEntry::IsDirectory() const
{
	//Investigated with Glump and it doesn't store empty directories.
	//It does however storre empty files, so size == 0 doesn't mean a directory
	return false;
}

} // namespace Downloader
} // namespace Origin

