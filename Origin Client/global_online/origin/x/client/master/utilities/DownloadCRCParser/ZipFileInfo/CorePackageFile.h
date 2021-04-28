/**
Packed file info. A class to manage the content of any packed type file (BIG, Zip, Tar etc)
*/

#ifndef _PACKAGEFILE_H_
#define _PACKAGEFILE_H_

#include <list>
#include <map>
#include <QDate>
#include <QString>
#include <QTime>
#include "CoreDate.h"
#include "CoreFile.h"
#include "WinDefs.h"

class ZipFileInfo;
class ZipFileEntry;
class BigFileInfo;
class BigFileEntry;

//namespace Core
//{

	class Parameterizer;
	class PackageFileEntry;

	typedef std::list<PackageFileEntry*> PackageFileEntryList;
	typedef std::list<const PackageFileEntry*> PackageFileConstEntryList;

	/**
	This class holds information about a BIG file and its contents
	*/
	class PackageFile
	{
	public:
		PackageFile();
		~PackageFile();

		/**
		Load from a definition file
		*/
		bool LoadFromManifest( const QString& filename );
		bool LoadFromManifest( CoreFile& file );

		/**
		Load from a zip file
		*/
		bool LoadFromZipFile( const QString& filename );
		bool LoadFromZipFile( const ZipFileInfo& info );

		/**
		Load from a big file
		*/
		bool LoadFromBigFile( const QString& filename );
		bool LoadFromBigFile( const BigFileInfo& info );

		/**
		Save to a definition file
		*/
		bool SaveManifest( const QString& filename ) const;
		bool SaveManifest( CoreFile& file ) const;

		/**
		Get the name or identifier (may be original file name)
		*/
		const QString& GetName() const  { return mName; }

		/**
		Get the type (origin: zip, big etc)
		*/
		const QString& GetType() const	{ return mType; }

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
		Get the number of files marked as included
		*/
		u_long GetNumberOfIncludedFiles() const;

		/**
		Test if it contans entries
		*/
		bool IsEmpty() const;

		/**
		Get the unpacked size off all files
		*/
		int64_t GetUnpackedSize() const;

		/**
		Get all entries
		*/
		const PackageFileEntryList& GetEntries() const { return mEntries; };

		/**
		Get files then overlap a certain range.
		Return the number of files or zero
		*/
		int GetEntriesInRange(PackageFileConstEntryList& list, int64_t offset, u_long len, int currentDiskIndex) const;

		/**
		Locate an entry by its filename (optionally do it case sensitive)
		*/
		const PackageFileEntry* GetEntryByName(const QString& sFileName, bool caseSensitive = false) const;

		/**
		Access the first entry
		*/
		PackageFileEntryList::iterator begin() { return mEntries.begin(); }
		PackageFileEntryList::const_iterator begin() const { return mEntries.begin(); }

		/**
		Access the end of the contained collection (not a valid item but one pass last item)
		*/
		PackageFileEntryList::iterator end() { return mEntries.end(); }
		PackageFileEntryList::const_iterator end() const { return mEntries.end(); }

		/**
		Remove all entries. Prepare for reuse
		*/
		void Clear();

		/**
		Set the destination directory (where all files will be extracted)
		If no destinaton directory is indicated, the current working directory is used.
		Remember to append a slash at the end, otherwise it will be interpreted as a file
		and only the directory part will be used.
		*/
		void SetDestinationDirectory( const QString& dir );

		/**
		Get the destination directory. 
		May be empty if never set.
		*/
		const QString& GetDestinationDirectory() const { return mDestDir; }

		/**
		Recreate all entries in the destination directory.
		If any entry exists its contents are preserved (may be trucated or extended to the correct entry size)
		so its safe to call this function several times (across sessions) to ensure every entry is there.
		*/
		bool RecreateEntries() const;

		/**
		This method undoes RecreateEntries. Erases every file from this package deployed at destination dir.
		Directories are also removed as long as they are empty after all entries are removed.
		This function will erase as much as it can, it'll not stop on error condition.
		If a directory cannot be removed because it is not empty this method does not consider that an error condition
		*/
		bool EraseEntries() const;

		/**
		Dump content info to a string in printable user-friendly format
		*/
		QString Dump();

		/**
		Distributes the entries for more uniform downloading
		*/
		//void BalanceEntries();			

	private:
		bool AddEntry( PackageFileEntry* );
		bool EntryExists( const PackageFileEntry* ) const;
		void SortEntries();

	private:
		QString mName;			/* File name or identifier */
		QString mType;			/* Package file type (zip, big, etc) */

		QString mDestDir;		/* Destination directory */

		PackageFileEntryList mEntries;
	};

	/**
	This class holds information about a single file within a BIG file
	*/
	class PackageFileEntry
	{
	public:
		PackageFileEntry();
		~PackageFileEntry();

		/**
		Get the file name
		*/
		const QString& GetFileName() const { return mFileName; }

		/**
		Get the file modification time
		*/
		const QDate& GetFileModificationDate() const { return mFileModificationDate; }
		const QTime& GetFileModificationTime() const { return mFileModificationTime; }

		/**
		Get the CRC
		*/
		uint32_t GetFileCRC() const { return mCRC; }

		/**
		The offset to the file header from the beginning of the package file
		*/
		int64_t GetOffsetToHeader() const { return mOffsetToHeader; }

		/**
		The offset to the file data from the beginning of the package file
		*/
		int64_t GetOffsetToFileData() const { return mOffsetToFileData; }

		/**
		The size of the header if it has been calculated.  
		The header is variable size and so can only be calculated once it is retrieved
		This will return  <= 0 if the header has not yet been computed
		*/
		int64_t GetHeaderSize() const { return mOffsetToFileData - mOffsetToHeader; }

		/**
		Get the offset to the end of this file from the beginning of the package file
		*/
		int64_t GetEndOffset() const { return mOffsetToFileData + mCompressedSize; }

		/**
		The the compressed (packed) size
		*/
		int64_t GetCompressedSize() const { return mCompressedSize; }

		/**
		The unpacked size
		*/
		int64_t GetUncompressedSize() const { return mUncompressedSize; }

		/**
		The disc/volume index where file is located
		*/
		uint16_t GetDiskNoStart() const { return mDiskNoStart; }

		/**
		File attributes
		*/
		bool IsDirectory() const { return mIsDir; }

        /**
        File attributes
        */
        bool IsFile() const { return !mIsDir; }

		/**
		Returns true if the file is verified
		*/
		bool IsVerified() const { return mVerified; }

		/**
		Returns true if the file is included in the package
		*/
		bool IsIncluded() const { return mbIncluded; }


		/**
		Returns true if the entry contains a valid filename
		*/
		bool IsValid() const;

		/**
		Returns true if the entry refers to a compressed file
		*/
		bool IsCompressed() const;

		enum
		{
			Uncompressed = 0,
			Shrink = 1,
			Implode = 6,
			Deflate = 8,
			BZIP2 = 12,
			LZMA = 14
		};

		/**
		Returns the compression method used for the entry (see ZipFileEntry for available values)
		Returns zero if the entry is uncompressed
		*/
		u_short GetCompressionMethod() const;

		/**
		Set the file name
		*/
		void setFileName( const QString& name ) 
		{ 
			mFileName = name; 
			mFileName.replace("//", "/"); 
		}

		/**
		Set the file modification time
		*/
		void setFileModificationDate( const uint16_t date ) { mFileModificationDate = CoreDateTime::MsDosDateToQDate(date); }
		void setFileModificationTime( const uint16_t time ) { mFileModificationTime = CoreDateTime::MsDosTimeToQTime(time); }

		/**
		Set file CRC
		*/
		void setFileCRC( uint32_t crc ) { mCRC = crc; }
		
		/**
		Set the compressed file size
		*/
		void setCompressedSize( int64_t sz ) { mCompressedSize = sz; }

		/**
		Set the uncompressed file size
		*/
		void setUncompressedSize( int64_t sz ) { mUncompressedSize = sz; }

		/**
		Set the offset to file header (zero if no header)
		*/
		void setOffsetToHeader( int64_t off ) { mOffsetToHeader = off; }

		/**
		Set the offset to file data
		Also makes the header size valid since it can be computed based on offset to header and this value
		*/
		void setOffsetToFileData( int64_t off ) { mOffsetToFileData = off; }

		/**
		Set the file name
		*/
		void setIsDirectory( bool isDir );

		/**
		Mark as verified
		*/
		void setIsVerified( bool ver );

		/** 
		Mark the file for includion
		*/
		void SetIsIncluded( bool bIncluded );

		/**
		Mark as verified
		*/
		void setCompressionMethod( u_short comp );

		/**
		Set disc/volume index
		*/
		void setDiskNoStart(uint16_t disc) { mDiskNoStart = disc; }

		/**
		Redownload count
		*/
		void addRedownloadCount() 
		{
			mFileRedownloadCount++;
		}
		int getRedownloadCount() const
		{
			return mFileRedownloadCount;
		}

	protected:
		/** Set default values for all data **/
		void Clear();

		/** Serialization **/
		bool fromParameters( const Parameterizer& p );
		bool toParameters( Parameterizer& p ) const;
		bool fromString( const QString& str );
		QString ToString() const;

		/** Create from zip **/
		void fromZipEntry( const ZipFileEntry& e );

		/** Create from big **/
		void fromBigEntry( const BigFileEntry& e );

	private:
		QString mFileName;			//File name
		QDate mFileModificationDate;
		QTime mFileModificationTime;
		uint32_t mCRC;				
		int64_t mOffsetToHeader;	//Offset to file header from the start of the packed file
		int64_t mOffsetToFileData;	//Offset to file data from the start of the packed file
		int64_t mUncompressedSize;	//Unpacked file size
		int64_t mCompressedSize;	//Compressed file size
		bool mIsDir;				//Directory indicator
		u_short mCompression;		//Compression method (see ZipFileEntry for available values)
		bool mVerified;				//File was verified

		bool	mbIncluded;			// File is included in the package (true by default.)

		uint16_t mDiskNoStart;		// for multi-part disc builds

		int mFileRedownloadCount;

		friend class PackageFile;
	};

//}

#endif
