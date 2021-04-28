/**
ZIP file information
*/

#ifndef _ZIPFILEINFO_H_
#define _ZIPFILEINFO_H_

//#include "platform.h"
#include <stdint.h>
#include <memory.h>
#include <QString>

#include "WinDefs.h"

//Forward declarations
//class stEndOfCentralDir;
//class stZip64EndOfCentralDirLocator;
//class stFileHeader;
//class stLocalFileHeader;
class ZipFileEntry;

/*****************************/
/** Structs used internally **/
/*****************************/

/**
End of central directory
*/
class stEndOfCentralDir
{
public:
    stEndOfCentralDir()
    {
        memset(this, 0, sizeof(*this));
    }

    uint32_t signature;				//end of central dir signature
    uint32_t diskNo;					//number of this disk
    uint32_t diskWithCentralDir;		//number of the disk with the start of the central directory
    __int64 entriesThisDisk;		//total number of entries in the central directory on this disk  
    __int64 entriesTotal;			//total number of entries in the central directory
    __int64 centralDirSize;			//size of the central directory   
    __int64 offsetToCentralDir;     //offset of start of central directory with respect to the starting disk number
    uint16_t	commentLenght;			//.ZIP file comment length
};


/** 
Zip64 end of central directory locator
*/
class stZip64EndOfCentralDirLocator
{
public:
    stZip64EndOfCentralDirLocator()
    {
        memset(this, 0, sizeof(*this));
    }

    uint32_t  signature;						//end of central dir locator signature
    uint32_t  diskWithEndOfCentralDir;		//number of the disk with start of Zip64 EOCD
    __int64 offsetToZip64EndOfCentralDir;   //offset of start of Zip64 EOCD with respect to the starting disk number
    uint32_t  numDisks;						//Total numbr of disks
};

/** File header within central dir **/
class stFileHeader
{
public:
    stFileHeader()
    {
        memset(this, 0, sizeof(*this));
    }

    uint32_t signature;			//central file header signature   4 bytes  (0x02014b50)
    uint16_t versionMade;		//version made by                 2 bytes
    uint16_t versionNeeded;		//version needed to extract       2 bytes
    uint16_t bitFlags;			//general purpose bit flag        2 bytes
    uint16_t compressionMethod;	//compression method              2 bytes
    uint16_t lastModTime;		//last mod file time              2 bytes
    uint16_t lastModDate;		//last mod file date              2 bytes
    uint32_t crc32;				//crc-32                          4 bytes
    uint32_t compressedSize;		//compressed size                 4 bytes
    uint32_t uncompressedSize;    //uncompressed size               4 bytes
    uint16_t filenameLength;		//file name length                2 bytes
    uint16_t extraFieldLength;	//extra field length              2 bytes
    uint16_t fileCommentLength;	//file comment length             2 bytes
    uint16_t diskNoStart;		//disk number start               2 bytes
    uint16_t internalAttribs;    //internal file attributes        2 bytes
    uint32_t externalAttribs;    //external file attributes        4 bytes
    uint32_t offsetLocalHeader;	//relative offset of local header 4 bytes
};

/** File header immediately before the file data **/
class stLocalFileHeader
{
public:
    stLocalFileHeader()
    {
        memset(this, 0, sizeof(*this));
    }

    uint32_t signature;			//central file header signature   4 bytes  (0x04034b50)
    uint16_t versionNeeded;		//version needed to extract       2 bytes
    uint16_t bitFlags;			//general purpose bit flag        2 bytes
    uint16_t compressionMethod;	//compression method              2 bytes
    uint16_t lastModTime;		//last mod file time              2 bytes
    uint16_t lastModDate;		//last mod file date              2 bytes
    uint32_t crc32;				//crc-32                          4 bytes
    uint32_t compressedSize;		//compressed size                 4 bytes
    uint32_t uncompressedSize;    //uncompressed size               4 bytes
    uint16_t filenameLength;		//file name length                2 bytes
    uint16_t extraFieldLength;	//extra field length              2 bytes
};

class MemBuffer;

#include "MemBuffer.h"
#include "CoreFile.h"
//#include "atltime.h"
#include <list>

typedef std::list<ZipFileEntry*> ZipFileEntryList;

//using namespace Core;

/**
This class holds information about a zip file and its contents
*/
class ZipFileInfo
{
public:
	ZipFileInfo();
	~ZipFileInfo();

	/**
	Read from a file
	*/
	bool Load( const QString& filename );
	bool Load( CoreFile& file );

	/**
	Remove all entries. Prepare for reuse
	*/
	void Clear();

	/**
	Dump contents to debug stream
	*/
	void Dump();

	/**
	Get the file name (if loaded from a file)
	*/
	const QString& GetFileName() const { return mFileName; }

	/**
	Get the global comment
	*/
	const QString& GetGlobalComment() const { return mComment; }

	/**
	Get the total number of entries (includes files and directories)
	*/
	uint32_t GetNumberOfEntries() const { return (uint32_t) mEntries.size(); }

	/**
	Get the file count (not including dirs)
	*/
	uint32_t GetNumberOfFiles() const;

	/**
	Get the number of declared directories
	This does NOT include dirs that should be created to output files 
	but only explicitly declared dirs.
	Dirs are explicitly declared any way.
	*/
	uint32_t GetNumberOfDirectories() const;

	/**
	Get the number of discs in the archive
	*/
	uint16_t GetNumberOfDiscs() { return mNumOfDiscs; }

	/**
	Get the uncompressed size off all files
	*/
	__int64 GetUncompressedSize() const;

	/**
	Get all entries
	*/
	const ZipFileEntryList& GetEntries() const { return mEntries; };

	/**
	Access the first entry
	*/
	ZipFileEntryList::const_iterator begin() const { return mEntries.begin(); }

	/**
	Access the end of the contained collection (not a valid item but one pass last item)
	*/
	ZipFileEntryList::const_iterator end() const { return mEntries.end(); }

	//Some constants
	static const uint32_t ZipEndOfCentralDirSignature;
	static const uint32_t Zip64EndOfCentralDirSignature;
	static const uint32_t Zip64EndOfCentralDirLocatorSignature;
	static const uint32_t ZipEndOfCentralDirFixedPartSize;
	static const uint32_t Zip64EndOfCentralDirFixedPartSize;
	static const uint32_t Zip64EndOfCentralDirLocatorSize;
	static const uint32_t FileHeaderSignature;
	static const uint32_t FileHeaderFixedPartSize;
	static const uint32_t LocalFileHeaderSignature;
	static const uint32_t LocalFileHeaderFixedPartSize;


	/**
	Try to load the central directory from the tail of the file.
	buffer should be the last xxx bytes of the file. 
	fileSize is the total file size
	offsetToStart will contain on output the offset to the start of the central dir
	If it fails it can be because of two conditions
	1. Not enough data, in which case offsetToStart will point to a required offset
	2. Invalid file, unknown error, in which case offsetToStart will be set to zero
	*/
	bool TryLoad( MemBuffer& buffer, __int64 fileSize, __int64* offsetToStart );

	/**
	Internal test, DO-NOTHING in release
	*/
	bool ValidationTest();

	//Static helper. Read a local file header
	static bool readLocalFileHeader( stLocalFileHeader& info, MemBuffer& data );

private:
	bool loadCentralDir( const stEndOfCentralDir* info, CoreFile& f );
	bool loadCentralDir( const stEndOfCentralDir* info, MemBuffer& buffer );

	//Some static helpers
        static bool readEndOfCentralDir( stEndOfCentralDir& info, MemBuffer& data );
	static bool readZip64EndOfCentralDirLocator( stZip64EndOfCentralDirLocator& locator, MemBuffer& data );

	static bool readFileHeader( stFileHeader& info, MemBuffer& data );

	static QString loadString(CoreFile& file, int len, u_int codepage = CP_ACP);
	static QString loadString(const QString &str, int len, u_int codepage = CP_ACP);

	bool AddEntry( ZipFileEntry* );

private:
	QString mFileName;			   /* Zip file name if loaded from disk */
	__int64 mOffsetToCentralDir;   /* Offset of start of central directory */
    __int64 mSizeCentralDir;       /* Size of the central directory  */
	QString mComment;			   /* Zip file comment */

	uint16_t mNumOfDiscs;			/* Number of discs archive is broken into */

	//ZipFileEntryMap 
	ZipFileEntryList mEntries;
};

/**
This class holds information about a single file within a zip file
*/
class ZipFileEntry
{
public:
	ZipFileEntry();
	~ZipFileEntry();

	/**
	Get the file name
	*/
	const QString& GetFileName() const { return mFileName; }

	/**
	Get the file comment, if any
	*/
	const QString& GetFileComment() const { return mFileComment; }

	/**
	Compression methods:
		0 - The file is stored (no compression)
		1 - The file is Shrunk
		2 - The file is Reduced with compression factor 1
		3 - The file is Reduced with compression factor 2
		4 - The file is Reduced with compression factor 3
		5 - The file is Reduced with compression factor 4
		6 - The file is Imploded
		7 - Reserved for Tokenizing compression algorithm
		8 - The file is Deflated
		9 - Enhanced Deflating using Deflate64(tm)
		10 - PKWARE Data Compression Library Imploding (old IBM TERSE)
		12 - File is compressed using BZIP2 algorithm
		14 - LZMA (EFS)
		18 - File is compressed using IBM TERSE (new)
		19 - IBM LZ77 z Architecture (PFS)
		97 - WavPack compressed data
		98 - PPMd version I, Rev 1
	*/
	uint16_t GetCompressionMethod() const { return mCompressionMethod; }

	/**
	Test if the file is compressed or just stored
	*/
	bool IsCompressed() const { return mCompressionMethod != 0; }

	/**
	Get the compression method name
	*/
	QString GetCompressionMethodName() const;

	/**
	Get the last modification date-time
	*/
	const uint16_t GetLastModifiedDate() const  { return mLastModifiedDate; }
	const uint16_t GetLastModifiedTime() const  { return mLastModifiedTime; }

	/**
	Get the crc-32 for this file
	*/
	uint32_t GetCrc32() const { return mCrc32; }
	
	/**
	The compressed size
	*/
	__int64 GetCompressedSize() const { return mCompressedSize; }

	/**
	The size when uncompressed
	*/
	__int64 GetUncompressedSize() const { return mUncompressedSize; }

	/**
	File attributes
	*/
	bool IsDirectory() const;
    bool IsFile() const { return !IsDirectory(); }
	bool IsHidden() const;
	bool IsSystem() const;
	bool IsReadOnly() const;
	DWORD GetAttributes() const;

	/**
	Other attributes
	*/
	bool IsEncrypted() const;       //Bit 1 of the general purpose flag
	bool HasCrcAndSizes() const;	//Bit 3: If set, then no crc or sizes is supplied with the file header
	bool HasUtf8Encoding() const;	//Bit 11: Filename and comment are UTF8

	/**
	Optional extra field
	*/
	const MemBuffer& GetExtraFields() const { return mExtraField; }

	/**
	Get the content of an extra field if present
	*/
	bool GetExtraField(int id, MemBuffer& buffer) const;
	
	/**
	Location
	*/
	__int64 GetOffsetOfLocalHeader() const;
	__int64 GetOffsetOfFileData() const;

	uint16_t GetDiskNoStart() const { return mDiskNoStart; }

protected:
	ZipFileEntry(const stFileHeader*);
	void setFileName( const QString& name ) { mFileName = name; }
	void setExtraField( MemBuffer& );
	void setFileComment( const QString& cmnt ) { mFileComment = cmnt; }
	void Clear();

private:
	QString mFileName;
	QString mFileComment;
	uint16_t mVersionMade;
	uint16_t mVersionNeeded;
	uint16_t mCompressionMethod;
	uint16_t mFlags;
	//CTime mLastModified;
	uint16_t mLastModifiedDate;	
	uint16_t mLastModifiedTime;	
	uint32_t mCrc32;
	__int64 mCompressedSize;
	__int64 mUncompressedSize;

	uint16_t mInternalAttribs;
	uint32_t mExternalAttribs;

	MemBuffer mExtraField;

	__int64 mOffsetOfLocalHeader;
	__int64 mOffsetOfLocalFileData;

	uint16_t mDiskNoStart;

	friend class ZipFileInfo;
};

#endif
