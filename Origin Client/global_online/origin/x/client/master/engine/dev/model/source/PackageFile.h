///////////////////////////////////////////////////////////////////////////////
// PackageFile.h
//
// Copyright (c) 2012 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#ifndef _PACKAGEFILE_H_
#define _PACKAGEFILE_H_

#include <list>
#include <map>
#include <QDate>
#include <QString>
#include <QTime>
#include <QFile>
#include "DateTime.h"

namespace Origin
{
namespace Downloader
{
    class ZipFileInfo;
    class ZipFileEntry;
    class BigFileInfo;
    class BigFileEntry;

	class Parameterizer;
	class PackageFileEntry;

	typedef std::list<PackageFileEntry*> PackageFileEntryList;
	typedef std::list<const PackageFileEntry*> PackageFileConstEntryList;

	/// \brief Packed file info. A class to manage the content of any packed type file (BIG, Zip, Tar etc.).
	class PackageFile
	{
	public:
        /// \brief The PackageFile constructor.
		PackageFile();

        /// \brief The PackageFile destructor.
		~PackageFile();

        /// \brief Populates our list of entries by reading the manifest file.
        /// \param filename The path to the manifest file.
        /// \return True if the file was read successfully.
		bool LoadFromManifest( const QString& filename );
        
        /// \brief Populates our list of entries by reading the zip file.
        /// \param filename The path to the zip file.
        /// \return True if the file was read successfully.
		bool LoadFromZipFile( const QString& filename );
        
        /// \brief Populates our list of entries using a ZipFileInfo object.
        /// \param info The ZipFileInfo object to read from.
        /// \return True if we did not encounter any errors.
		bool LoadFromZipFile( const ZipFileInfo& info );
        
        /// \brief Populates our list of entries by reading the BIG file.
        /// \param filename The path to the BIG file.
        /// \return True if the file was read successfully.
		bool LoadFromBigFile( const QString& filename );
        
        /// \brief Populates our list of entries using a BigFileInfo object.
        /// \param info The BigFileInfo object to read from.
        /// \return True if we did not encounter any errors.
		bool LoadFromBigFile( const BigFileInfo& info );

		/// \brief Saves our list of entries to a definition file.
        /// \param filename The path to the save the file to.
        /// \return True if the file was saved successfully.
		bool SaveManifest( const QString& filename ) const;
        
		/// \brief Saves our list of entries to a definition file.
        /// \param file A QFile object representing the file we want to save to.
        /// \return True if the file was saved successfully.
		bool SaveManifest( QFile& file ) const;

		/// \brief Gets the name or identifier (may be original file name).
        /// \return The name or identifier.
		const QString& GetName() const  { return mName; }

		/// \brief Gets the product ID associated with this package (used for logging).
        /// \return The unique product ID.
		const QString& GetProductId() const  { return mProductId; }

		/// \brief Gets the type of this package (BIG, Zip, etc.).
        /// \return The package type, as a string.
		const QString& GetType() const	{ return mType; }

		/// \brief Gets the total number of entries (includes files and directories).
		/// \return The total number of entries.
		quint32 GetNumberOfEntries() const { return (quint32) mEntries.size(); }

		/// \brief Gets the file count (not including directories).
		/// \return The total number of files.
		quint32 GetNumberOfFiles() const;

	    /// \brief Gets the number of declared directories.
	    /// \note This does NOT include directories that should be created to output files, only explicitly declared directories. Directories are explicitly declared any way.
	    /// \return The total number of directories.
		quint32 GetNumberOfDirectories() const;

		/// \brief Gets the number of files marked as included.
        /// \return The number of files marked as included.
		quint32 GetNumberOfIncludedFiles() const;

		/// \brief Checks if our entry list is empty.
        /// \return True if the entry list is empty.
		bool IsEmpty() const;

		/// \brief Gets the unpacked size of all files.
		/// \return The unpacked size of all files.
		qint64 GetUnpackedSize() const;
        
	    /// \brief Gets a list of all entries.
        /// \return A list of all entries.
		const PackageFileEntryList& GetEntries() const { return mEntries; };

		/// \brief Gets files then overlap the given range.
        /// \param list This list is populated by this function.
        /// \param offset The offset from the start of the file.
        /// \param len The distance from the offset to search.
        /// \param currentDiskIndex The disk index to search in.
		/// \return The number of files in range, otherwise zero.
		int GetEntriesInRange(PackageFileConstEntryList& list, qint64 offset, quint32 len, int currentDiskIndex) const;

		/// \brief Gets an entry by its filename.
        /// \param sFileName The filename to compare against.
        /// \param caseSensitive True if we should ignore case.
		/// \return A pointer to the entry if one was found, otherwise NULL.
		const PackageFileEntry* GetEntryByName(const QString& sFileName, bool caseSensitive = false) const;
        
	    /// \brief Gets an iterator to the first entry.
        /// \return An iterator to the first entry.
		PackageFileEntryList::iterator begin() { return mEntries.begin(); }
        
	    /// \brief Gets an iterator to the first entry.
        /// \return An iterator to the first entry.
		PackageFileEntryList::const_iterator begin() const { return mEntries.begin(); }
        
	    /// \brief Gets an iterator to the last entry. This will not be a valid item.
        /// \return An iterator to the last entry.
		PackageFileEntryList::iterator end() { return mEntries.end(); }
        
	    /// \brief Gets an iterator to the last entry. This will not be a valid item.
        /// \return An iterator to the last entry.
		PackageFileEntryList::const_iterator end() const { return mEntries.end(); }
        
	    /// \brief Removes all entries and prepares for reuse.
		void Clear();
        
        /// \brief Sets the product ID associated with this package (used for logging).
		/// \param productId The product ID associated with this package.
        void SetProductId( const QString& productId ) { mProductId = productId; }

		/// \brief Sets the destination directory (where all files will be extracted).
		/// 
        /// If no destination directory is indicated, the current working directory is used.
        /// \param dir The directory to extract files to.
		/// \note Remember to append a slash at the end, otherwise it will be interpreted as a file and only the directory part will be used.
		void SetDestinationDirectory( const QString& dir );

		/// \brief Gets the destination directory.  May be empty if never set.
		/// \return The destination directory.
		const QString& GetDestinationDirectory() const { return mDestDir; }

		/// \brief Recreates all entries in the destination directory.
        /// 
		/// If any entry exists its contents are preserved (may be truncated or extended to the correct entry size),
		/// so it's safe to call this function several times (across sessions) to ensure every entry is there.
		/// \return True if all entries were created successfully.
		bool RecreateEntries() const;

		/// \brief Undoes RecreateEntries. Erases every file from this package deployed at destination directory.
		///
        /// Directories are also removed as long as they are empty after all entries are removed.
		/// This function will erase as much as it can and will not stop on error condition.
		/// If a directory cannot be removed because it is not empty, this method does not consider that an error condition.
		/// \return True if entries were deleted successfully.
		bool EraseEntries() const;

		/// \brief Dumps content info to a string in printable user-friendly format.
		/// \return A string containing all content info.
		QString Dump();

        /// \brief Distributes the entries for more uniform downloading.
        /// 
		// Because DiP downloads have the same overhead for small files as they do for large,
        // downloading a lot of small files will be slower overall than downloading few large files.
        // Because files packed into a ZIP file are packed alphabetically by folder, there is an unbalanced
        // packing of files, based on size.
        // This function is used for interspersing large and small files to make for more balanced transfers.
        //
        // The algorithm:
        // 1) Sort the files from largest to smallest
        // 2) For each large file, follow it by a number of small files to be downloaded while the large file is downloading.
        //
        /// \note The final list is reversed with all of the medium size files up front (hence working from the outside in and pushing front).
		//void BalanceEntries();

	private:
        /// \brief Adds an entry to the entry map.
        /// \param entry The entry to add to the list.
        /// \return True if the entry was added succesfully.
		bool AddEntry( PackageFileEntry* entry );

        /// \brief Checks if an entry exists on the hard drive.
        /// \param entry The entry to check for.
        /// \return True if the entry already exists.
		//bool EntryExists( const PackageFileEntry* entry ) const;
        
        /// \brief Sorts entries so that they are in the order described in the BIG/Zip/etc. file.
		void SortEntries();

		/// \brief Sorts the entries to be most contiguous so downloader can optimally cluster.
		void SortEntriesByOffset();

	private:
        /// \brief File name or identifier.
		QString mName;

        /// \brief Product ID associated with the package
        QString mProductId;

        /// \brief Package file type (Zip, BIG, etc.).
		QString mType;

        /// \brief Destination directory.
		QString mDestDir;

        /// \brief List of all entries in this package.
		PackageFileEntryList mEntries;
	};

	/// \brief Holds information about a single file within a package file.
	class PackageFileEntry
	{
	public:
        /// \brief The PackageFileEntry constructor.
		PackageFileEntry();

        /// \brief The PackageFileEntry destructor.
		~PackageFileEntry();

		/// \brief Gets the name of this file or directory.
		/// \return The name of this file or directory.
		const QString& GetFileName() const { return mFileName; }

		/// \brief Gets the file modification date.
        /// \return A QDate object representing the file modification date.
		const QDate& GetFileModificationDate() const { return mFileModificationDate; }
        
		/// \brief Gets the file modification time.
        /// \return A QTime object representing the file modification time.
		const QTime& GetFileModificationTime() const { return mFileModificationTime; }

		/// \brief Gets the CRC.
        /// \return The CRC.
		quint32 GetFileCRC() const { return mCRC; }

		/// \brief Gets the offset to the file header from the beginning of the packed file.
        /// \return The offset to the file header from the beginning of the packed file.
		qint64 GetOffsetToHeader() const { return mOffsetToHeader; }

		/// \brief Gets the offset to the file data from the beginning of the packed file.
		/// \return The offset to the file data from the beginning of the packed file.
		qint64 GetOffsetToFileData() const { return mOffsetToFileData; }

		//// \brief Gets the size of the header if it has been calculated.  
		/// 
        /// The header is variable size and so can only be calculated once it is retrieved.
		/// \return The size of the header if the header has been computed, otherwise value will be invalid (less than or equal to 0).
		qint64 GetHeaderSize() const { return mOffsetToFileData - mOffsetToHeader; }

		/// \brief Gets the offset to the end of this file from the beginning of the packed file.
		/// \return The offset to the end of this file from the beginning of the packed file.
		qint64 GetEndOffset() const { return mOffsetToFileData + mCompressedSize; }

		/// \brief Gets the compressed (packed) size.
		/// \return The compressed size.
		qint64 GetCompressedSize() const { return mCompressedSize; }

		/// \brief Gets the uncompressed (unpacked) size.
		/// \return The uncompressed size.
		qint64 GetUncompressedSize() const { return mUncompressedSize; }

		/// \brief Gets the disc/volume index where file is located.
        /// \return The disc/volume index where the file is located.
		quint16 GetDiskNoStart() const { return mDiskNoStart; }

        /// \brief Gets the file attributes (i.e. read-only, execute).
        /// \return The file attributes.
        quint32 GetFileAttributes() const { return mAttributes; }
        
		/// \brief Checks if this entry is a directory.
        /// \return True if this entry is a directory.
		bool IsDirectory() const { return mIsDir; }

        /// \brief Checks if this entry is a file.
        /// \return True if this entry is a file.
        bool IsFile() const { return !mIsDir; }

		/// \brief Checks if this entry has been verified.
        /// \return True if the file has been verified.
		bool IsVerified() const { return mVerified; }

		/// \brief Checks if the file is included in the package.
		/// \return True if the file is included in the package.
		bool IsIncluded() const { return mbIncluded; }

        /// \brief Checks if the file is able to be differentially updatable. (default: true)
        /// \return True if the file is able to be differentially updated.
        bool IsDiffUpdatable() const { return mbDiffUpdatable; }

		/// \brief Checks if the entry contains a valid filename.
        /// \return True if the entry contains a valid filename.
		bool IsValid() const;

		/// \brief Checks if the entry refers to a compressed file.
		/// \return True if the entry refers to a compressed file.
		bool IsCompressed() const;

        /// \brief The different algorithms used to compress a package file.
        ///
	    /// Unused/unsupported Zip compression methods:\n
		///    <b>2</b>. The file is Reduced with compression factor 1\n
		///    <b>3</b>. The file is Reduced with compression factor 2\n
		///    <b>4</b>. The file is Reduced with compression factor 3\n
		///    <b>5</b>. The file is Reduced with compression factor 4\n
		///    <b>7</b>. Reserved for Tokenizing compression algorithm\n
		///    <b>9</b>. Enhanced Deflating using Deflate64(tm)\n
		///    <b>10</b>. PKWARE Data Compression Library Imploding (old IBM TERSE)\n
		///    <b>18</b>. The file is compressed using IBM TERSE (new)\n
		///    <b>19</b>. IBM LZ77 z Architecture (PFS)\n
		///    <b>97</b>. WavPack compressed data\n
		///    <b>98</b>. PPMd version I, Rev 1\n
		enum CompressionMethod
		{
			Uncompressed = 0, ///< The file is stored without compression.
			Shrink = 1, ///< The file is Shrunk.
			Implode = 6, ///< The file is Imploded.
			Deflate = 8, ///< The file is Deflated.
			BZIP2 = 12, ///< The file is compressed using the BZIP2 algorithm.
			LZMA = 14 ///< The file is compressed using IBM TERSE (new)
		};

        /// \brief Gets the compression method used (see PackageFileEntry::CompressionMethod for available values).
		/// \return The compression method used, or zero if the entry is uncompressed.
		quint16 GetCompressionMethod() const;

		/// \brief Sets the file name.
		/// \param name The file name.
		void setFileName( const QString& name ) 
		{ 
			mFileName = name; 
			mFileName.replace("//", "/"); 
		}

		/// \brief Sets the file modification date.
		/// \param date The file modification date in MS DOS format.
		void setFileModificationDate( const quint16 date ) { mFileModificationDate = DateTime::MsDosDateToQDate(date); }
        
		/// \brief Sets the file modification time.
		/// \param time The file modification time in MS DOS format.
		void setFileModificationTime( const quint16 time ) { mFileModificationTime = DateTime::MsDosTimeToQTime(time); }

		/// \brief Sets the file CRC.
		/// \param crc The file CRC.
		void setFileCRC( quint32 crc ) { mCRC = crc; }
		
		/// \brief Sets the compressed file size.
		/// \param sz The compressed file size.
		void setCompressedSize( qint64 sz ) { mCompressedSize = sz; }

		/// \brief Sets the uncompressed file size.
		/// \param sz The uncompressed file size.
		void setUncompressedSize( qint64 sz ) { mUncompressedSize = sz; }

		/// \brief Sets the offset to file header (zero is expected if no header).
		/// \param off The offset to the file header.
		void setOffsetToHeader( qint64 off ) { mOffsetToHeader = off; }

		/// \brief Sets the offset to file data.
        ///
		/// Also makes the header size valid since it can be computed based on offset to header and this value
		/// \param off The offset to the file data.
		void setOffsetToFileData( qint64 off ) { mOffsetToFileData = off; }

        /// \brief Sets the attributes for the file (i.e. read-only, executable, etc.).
        /// \param attributes The attributes for the file.
        void setFileAttributes ( quint32 attributes ) { mAttributes = attributes; }
        
		/// \brief Sets whether or not this entry is a directory.
		/// \param isDir True if this entry is a directory.
		void setIsDirectory( bool isDir );

		/// \brief Marks this entry has been verified.
		/// \param ver True if this entry has been verified.
		void setIsVerified( bool ver );

		/// \brief Marks this entry as included.
		/// \param bIncluded True if this entry is included.
		void SetIsIncluded( bool bIncluded );

        /// \brief Marks this entry as differentially updatable (or not).
        /// \param bDiffUpdatable True if this entry is differentially updatable.
        void SetIsDiffUpdatable( bool bDiffUpdatable );

		/// \brief Sets the compression method (see PackageFileEntry::CompressionMethod for available values).
        /// \param comp The compression method.
		void setCompressionMethod( quint16 comp );

		/// \brief Sets the disc/volume index.
		/// \param disc The disc/volume index.
		void setDiskNoStart(quint16 disc) { mDiskNoStart = disc; }
        
        /// \brief Increments the count of files that need to be redownloaded.
		void addRedownloadCount() { mFileRedownloadCount++; }

        /// \brief Gets the number of files that need to be redownloaded.
        /// \return The number of files that need to be redownloaded.
		int getRedownloadCount() const { return mFileRedownloadCount; }

	protected:
		/// \brief Resets all data to default values.
		void Clear();
        
        /// \brief Populates this entry using an existing Parameterizer object.
        /// \param p The Parameterizer object to copy from.
        /// \return True if this entry was populated successfully.
		bool fromParameters( const Parameterizer& p );
        
        /// \brief Populates an existing Parameterizer object using data contained in this entry.
        /// \param p The Parameterizer object to populate.
        /// \return True if the object was populated successfully.
		bool toParameters( Parameterizer& p ) const;

        /// \brief Populates this entry using a string.
        /// \param str The string containing data about this entry.
        /// \return True if this entry was populated successfully.
		bool fromString( const QString& str );

        /// \brief Converts this entry to a string using the Parameterizer.
        /// \return The string representing this entry.
		QString ToString() const;

		/// \brief Populates this PackageFile using an existing Downloader::ZipFileEntry.
        /// \param e The existing Downloader::ZipFileEntry to copy from.
		void fromZipEntry( const ZipFileEntry& e );

		/// \brief Populates this PackageFile using an existing Downloader::BigFileEntry.
        /// \param e The existing Downloader::BigFileEntry to copy from.
		void fromBigEntry( const BigFileEntry& e );

	private:
        /// \brief Name or identifier of this file or directory.
		QString mFileName;

        /// \brief The date this file or directory was last modified.
		QDate mFileModificationDate;

        /// \brief The time this file or directory was last modified.
		QTime mFileModificationTime;

        /// \brief The CRC.
		quint32 mCRC;

        /// \brief The offset to file header from the start of the packed fileor directory.
		qint64 mOffsetToHeader;

        /// \brief The offset to the file data from the beginning of the packed fileor directory.
		qint64 mOffsetToFileData;

        /// \brief The unpacked file size.
		qint64 mUncompressedSize;

        /// \brief The compressed file size.
		qint64 mCompressedSize;
        
        /// \brief The file attributes (i.e. read-only, execute).
        quint32 mAttributes;

        /// \brief True if this entry represents a directory.
		bool mIsDir;

        /// \brief The compression method (see PackageFileEntry::CompressionMethod for available values).
		quint16 mCompression;

        /// \brief True if the file was verified.
		bool mVerified;

        /// \brief True if the file is included in the package (true by default).
		bool mbIncluded;

        /// \brief True if the file is able to be differentially updated.  (true by default)
        bool mbDiffUpdatable;

        /// \brief The disc number (for multi-part disc builds).
		quint16 mDiskNoStart;

        /// \brief Number of files that need to be redownloaded.
		int mFileRedownloadCount;

		friend class PackageFile;
	};

} // namespace Downloader
} // namespace Origin

#endif
