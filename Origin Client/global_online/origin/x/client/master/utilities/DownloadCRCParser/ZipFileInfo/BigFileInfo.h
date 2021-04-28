/**
BIG file information (VIV
*/

#ifndef _BIGFILEINFO_H_
#define _BIGFILEINFO_H_

#include "MemBuffer.h"
#include "CoreFile.h"
#include <QString>

#include <list>

//using namespace Core;

//Forward declarations
class stBIGFileHeader;
class BigFileEntry;
class MemBuffer;

typedef std::list<BigFileEntry*> BigFileEntryList;

/**
This class holds information about a BIG file and its contents
*/
class BigFileInfo
{
public:
	BigFileInfo();
	~BigFileInfo();

	/**
	Read from a file
	*/
	bool Load( const QString& filename );
	bool Load( CoreFile& file );
	
	/**
	Load from a buffer containing header + TOC
	To know the size of the header for a given BIG file use GetHeaderSize()
	*/
	bool Load( MemBuffer& HeaderAndTOC );

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
	Get the full header size (including TOC) by reading the beginning of the file
	bof should point to a buffer of AT LEAST FileHeaderSize bytes.
	If the function fails it will return zero.
	This function validates the BIG file type.
	*/
	static u_long GetHeaderSize( u_char* bof );	
	
	/**
	Get the BIG file type
	*/
	u_long GetBIGFileType() const { return mFileType; }
	
	/** Commodities **/
	bool IsBIG2() const { return mFileType == BIG2Type; }
	bool IsBIG3() const { return mFileType == BIG3Type; }
	bool IsBIG4() const { return mFileType == BIG4Type || mFileType == BIGFType; }
	bool IsBIG5() const { return mFileType == BIG5Type; }
	bool Isc0fb() const { return mFileType == c0fbType; }

	/**
	Get the total number of entries (includes files and directories)
	*/
	u_long GetNumberOfEntries() const { return (u_long) mEntries.size(); }

	/**
	Get the file count (not including dirs)
	*/
	u_long GetNumberOfFiles() const;

	/**
	Get the numbre of declared directories
	This does NOT include dirs that should be created to output files 
	but only explicitely declared dirs.
	Dirs are explicitely declared any way.
	*/
	u_long GetNumberOfDirectories() const;

	/**
	Get the unpacked size off all files
	*/
	__int64 GetUnpackedSize() const;

	/**
	Get all entries
	*/
	const BigFileEntryList& GetEntries() const { return mEntries; };

	/**
	Access the first entry
	*/
	BigFileEntryList::const_iterator begin() const { return mEntries.begin(); }

	/**
	Access the end of the contained collection (not a valid item but one pass last item)
	*/
	BigFileEntryList::const_iterator end() const { return mEntries.end(); }

	//Some constants
	static const u_long BIG2Type;
	static const u_long BIG3Type;
	static const u_long BIG4Type;
	static const u_long BIG5Type;
	static const u_long c0fbType;
	static const u_long BIGFType;
	static const u_long FileHeaderSize;

private:
	bool loadTOC( CoreFile& f );
	bool loadTOC( MemBuffer& buffer );

	static bool readBIGFileHeader( stBIGFileHeader& info, MemBuffer& data );

	static QString GetBIGFileTypeDescription( u_long type );
	
	/**
	Reads and returns a newly allocated BigFileEntry from a data buffer
	The data buffer pointer will end up pointing one after the header ending byte.
	If the buffer is insufficient the function returns 'null' but error is false
	If there is an error in the input data the function returns 'null' but error is true
	*/
	BigFileEntry* readTOCFileHeader( MemBuffer& data, bool& error ) const;

	bool AddEntry( BigFileEntry* );

private:
	QString mFileName;			/* BIG file name if loaded from disk */
	u_long mFileType;			/* BIG file type */
	u_long mNumEntries;			/* Number of entries in BIG file */
	__int64 mHeaderLen;			/* Offset of start of TOC */
    __int64 mSizeTOC;			/* Size of TOC */
	__int64 mSizeFile;			/* Size of BIG file */

	BigFileEntryList mEntries;
};

/**
This class holds information about a single file within a BIG file
*/
class BigFileEntry
{
public:
	BigFileEntry();
	~BigFileEntry();

	/**
	Get the file name
	*/
	const QString& GetFileName() const { return mFileName; }

	/**
	The unpacked size
	*/
	u_long GetSize() const { return mSize; }

	/**
	File attributes
	*/
	bool IsDirectory() const;

	/**
	Location
	*/
	u_long GetOffsetOfLocalHeader() const { return mOffsetOfTOCHeader; }

	/**
	The offset from the begining of the BIG file
	*/
	__int64 GetOffset() const { return mOffset; }

	/**
	Returns true if the entry contains a valid filename
	*/
	bool IsValid() const;

protected:
	void setFileName( const QString& name ) { mFileName = name; }
	void setOffset( __int64 off ) { mOffset = off; }
	void setSize( u_long sz ) { mSize = sz; }
	void Clear();

private:
	QString mFileName;			//file name
	u_long mSize;				//file size
	__int64 mOffset;			//Offset to file from the start of the BIG file

	u_long mOffsetOfTOCHeader;	//Offset of TOC header from the start of the BIG file
	

	friend class BigFileInfo;
};

#endif
