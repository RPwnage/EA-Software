///////////////////////////////////////////////////////////////////////////////
// ZipFileInfo.h
//
// Copyright (c) 2012 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#ifndef _ZIPFILEINFO_H_
#define _ZIPFILEINFO_H_

#include <memory.h>
#include <QString>

#include "services/downloader/MemBuffer.h"
#include <list>
#include <QFile>

namespace Origin
{
namespace Downloader
{
//Forward declarations
class ZipFileEntry;
class MemBuffer;

/// \brief Contains data about the end of the central directory.  Used internally.
class EndOfCentralDir
{
public:
    /// \brief EndOfCentralDir constructor.
    EndOfCentralDir()
    {
        memset(this, 0, sizeof(*this));
    }

    /// \brief End of central dir signature.
    quint32 signature;

    /// \brief Number of this disc.
    quint32 diskNo;

    /// \brief Number of the disc with the start of the central directory.
    quint32 diskWithCentralDir;

    /// \brief Total number of entries in the central directory on this disc.
    qint64 entriesThisDisk;

    /// \brief Total number of entries in the central directory.
    qint64 entriesTotal;

    /// \brief Size of the central directory.
    qint64 centralDirSize;

    /// \brief Offset of start of central directory with respect to the starting disc number.
    qint64 offsetToCentralDir;

    /// \brief Zip file comment length.
    quint16	commentLength;
};


/// \brief Locates the end of central directory for Zip64.  Used internally.
class Zip64EndOfCentralDirLocator
{
public:
    /// \brief The Zip64EndOfCentralDirLocator constructor.
    Zip64EndOfCentralDirLocator()
    {
        memset(this, 0, sizeof(*this));
    }

    /// \brief End of central directory locator signature.
    quint32 signature;
    
    /// \brief Number of the disc with start of Zip64 EOCD.
    quint32 diskWithEndOfCentralDir;
    
    /// \brief Offset of start of Zip64 EOCD with respect to the starting disc number.
    qint64 offsetToZip64EndOfCentralDir;
    
    /// \brief Total number of discs.
    quint32 numDisks;
};

/// \brief Contains data about the a file header within central directory.  Used internally.
class FileHeader
{
public:
    /// \brief The FileHeader constructor.
    FileHeader()
    {
        memset(this, 0, sizeof(*this));
    }

    /// \brief Central file header signature (see ZipFileInfo::FileHeaderSignature). 4 bytes.
    quint32 signature;

    /// \brief Version made by. 2 bytes.
    quint16 versionMade;

    /// \brief Version needed to extract. 2 bytes.
    quint16 versionNeeded;

    /// \brief General purpose bit flag. 2 bytes.
    quint16 bitFlags;

    /// \brief Compression method (see PackageFileEntry for available values). 2 bytes.
    quint16 compressionMethod;

    /// \brief Last file modification time. 2 bytes.
    quint16 lastModTime;

    /// \brief Last file modification date.  2 bytes.
    quint16 lastModDate;

    /// \brief 32-bit CRC. 4 bytes.
    quint32 crc32;

    /// \brief Compressed size. 4 bytes.
    quint32 compressedSize;

    /// \brief Uncompressed size. 4 bytes.
    quint32 uncompressedSize;

    /// \brief File name length. 2 bytes.
    quint16 filenameLength;

    /// \brief Extra field length. 2 bytes.
    quint16 extraFieldLength;

    /// \brief File comment length. 2 bytes.
    quint16 fileCommentLength;

    /// \brief Disk number start. 2 bytes.
    quint16 diskNoStart;

    /// \brief Internal file attributes. 2 bytes.
    quint16 internalAttribs;

    /// \brief External file attributes. 4 bytes.
    quint32 externalAttribs;

    /// \brief Relative offset of local header. 4 bytes.
    quint32 offsetLocalHeader;
};

/// \brief Contains data about the file header immediately before the file data.
class LocalFileHeader
{
public:
    /// \brief The LocalFileHeader constructor.
    LocalFileHeader()
    {
        memset(this, 0, sizeof(*this));
    }

    /// \brief Central file header signature (see ZipFileInfo::LocalFileHeaderSignature). 4 bytes.
    quint32 signature;

    /// \brief Version needed to extract. 2 bytes.
    quint16 versionNeeded;

    /// \brief General purpose bit flag. 2 bytes.
    quint16 bitFlags;

    /// \brief Compression method (see PackageFileEntry for available values). 2 bytes.
    quint16 compressionMethod;
    
    /// \brief Last file modification time. 2 bytes.
    quint16 lastModTime;
    
    /// \brief Last file modification date. 2 bytes.
    quint16 lastModDate;
    
    /// \brief 32-bit CRC. 4 bytes.
    quint32 crc32;

    /// \brief Compressed size. 4 bytes.
    quint32 compressedSize;

    /// \brief Uncompressed size. 4 bytes.
    quint32 uncompressedSize;

    /// \brief File name length. 2 bytes.
    quint16 filenameLength;

    /// \brief Extra field length. 2 bytes.
    quint16 extraFieldLength;
};

typedef std::list<ZipFileEntry*> ZipFileEntryList;

/// \brief Holds information about a Zip file and its contents.
class ZipFileInfo
{
public:
    /// \brief The ZipFileInfo constructor.
	ZipFileInfo();

    /// \brief The ZipFileInfo destructor.
	~ZipFileInfo();
    
	/// \brief Attempts to load a Zip file.
    /// \param filename A string path to the file to load.
    /// \return True if the file was loaded successfully.
	bool Load( const QString& filename );
    
	/// \brief Attempts to load a Zip file.
    /// \param file A QFile object created from the Zip file.
    /// \return True if the file was loaded successfully.
	bool Load( QFile& file );
    
	/// \brief Removes all entries and prepares for reuse.
	void Clear();
    
	/// \brief Dumps contents to a debug stream.
	void Dump();

	/// \brief Retrieves the file name (if loaded from a file).
    /// \return A string representing the file name.
	const QString& GetFileName() const { return mFileName; }

	/// \brief Gets the global comment.
	/// \return The global comment.
	const QString& GetGlobalComment() const { return mComment; }
    
	/// \brief Gets the total number of entries, including files and directories.
    /// \return The number of entries including files and directories.
	quint32 GetNumberOfEntries() const { return (quint32) mEntries.size(); }
    
	/// \brief Gets the file count, excluding directories.
	/// \return The number of files excluding directories.
	quint32 GetNumberOfFiles() const;
    
	/// \brief Gets the number of declared directories
	/// \note This does NOT include directories that should be created to output files, only explicitly declared directories. Directories are explicitly declared any way.
	/// \return The number of directories.
	quint32 GetNumberOfDirectories() const;

	/// \brief Gets the number of discs in the archive.
	/// \return The number of discs in the archive.
	quint16 GetNumberOfDiscs() { return mNumOfDiscs; }
    
	/// \brief Gets the uncompressed size of all files.
    /// \return The uncompressed size of all files.
	qint64 GetUncompressedSize() const;
    
	/// \brief Gets a list of all entries.
    /// \return A list of all entries.
	const ZipFileEntryList& GetEntries() const { return mEntries; };
    
	/// \brief Gets an iterator to the first entry.
    /// \return An iterator to the first entry.
	ZipFileEntryList::const_iterator begin() const { return mEntries.begin(); }
    
	/// \brief Gets an iterator to the last entry. This will not be a valid item.
    /// \return An iterator to the last entry.
	ZipFileEntryList::const_iterator end() const { return mEntries.end(); }

	/// \brief The EOCD signature for Zip files.
	static const quint32 ZipEndOfCentralDirSignature;
    
	/// \brief The EOCD signature for Zip64 files.
	static const quint32 Zip64EndOfCentralDirSignature;
    
	/// \brief The EOCD locator signature for Zip64 files.
	static const quint32 Zip64EndOfCentralDirLocatorSignature;

    /// \brief TBD.
	static const quint32 ZipEndOfCentralDirFixedPartSize;
    
    /// \brief TBD.
	static const quint32 Zip64EndOfCentralDirFixedPartSize;
    
    /// \brief TBD.
	static const quint32 Zip64EndOfCentralDirLocatorSize;
    
    /// \brief The file header signature.
	static const quint32 FileHeaderSignature;

    /// \brief TBD.
	static const quint32 FileHeaderFixedPartSize;
    
    /// \brief The local file header signature.
	static const quint32 LocalFileHeaderSignature;
    
    /// \brief TBD.
	static const quint32 LocalFileHeaderFixedPartSize;

	/// \brief Tries to load the central directory from the tail of the file.
	/// 
	/// If it fails it can be because of two conditions:\n
	/// 1. Not enough data, in which case offsetToStart will point to a required offset\n
	/// 2. Invalid file, unknown error, in which case offsetToStart will be set to zero\n
	/// \param buffer Buffer containing the last xxx bytes of the file.
    /// \param fileSize The total file size.
    /// \param offsetToStart This function will set this value to the offset to the start of the central directory.
    /// \return True if the file was loaded successfully.
	bool TryLoad( MemBuffer& buffer, qint64 fileSize, qint64* offsetToStart );

	/// \brief Validates the Zip file.  Does nothing in Release.
    ///
    /// This function runs a series of sanity checks on the Zip file.
	/// \return True if all sanity checks pass. Always returns true in Release.
	bool ValidationTest();

	/// \brief Static helper funtion that reads a local file header.
    /// \param info A LocalFileHeader object to populate.
    /// \param data A MemBuffer object containing the raw data to read.
    /// \return True if info was populated successfully.
	static bool readLocalFileHeader( LocalFileHeader& info, MemBuffer& data );

private:
    /// \brief Populates our list of ZipFileEntry objects by loading the central directory.
    /// \param info An EndOfCentralDir object that contains data about the central directory.
    /// \param f A QFile object representing the file containing the central directory.
    /// \return True if the central directory was loaded successfully.
	bool loadCentralDir( const EndOfCentralDir* info, QFile& f );
    
    /// \brief Populates our list of ZipFileEntry objects by loading the central directory.
    /// \param info An EndOfCentralDir object that contains data about the central directory.
    /// \param buffer A MemBuffer object containing the raw central directory data.
    /// \return True if the central directory was loaded successfully.
	bool loadCentralDir( const EndOfCentralDir* info, MemBuffer& buffer );

    /// \brief Populates an EndOfCentralDir object using the raw data.
    /// \param info An EndOfCentralDir object that will be populated.
    /// \param data A MemBuffer object containing the raw data.
    /// \return True if the EndOfCentralDir object was populated successfully.
	static bool readEndOfCentralDir( EndOfCentralDir& info, MemBuffer& data );
    
    /// \brief Populates a Zip64EndOfCentralDirLocator object using the raw data.
    /// \param locator A Zip64EndOfCentralDirLocator object that will be populated.
    /// \param data A MemBuffer object containing the raw data.
    /// \return True if the Zip64EndOfCentralDirLocator object was populated successfully.
	static bool readZip64EndOfCentralDirLocator( Zip64EndOfCentralDirLocator& locator, MemBuffer& data );

    /// \brief Populates a FileHeader object using the raw data.
    /// \param info A FileHeader object that will be populated.
    /// \param data A MemBuffer object containing the raw data.
    /// \return True if the FileHeader object was populated successfully.
	static bool readFileHeader( FileHeader& info, MemBuffer& data );

    /// \brief TBD.
	static QString loadString(QFile& file, int len, MemBuffer::CodePage codepage = MemBuffer::kCodePageANSI);
    
    /// \brief TBD.
	static QString loadString(const QString &str, int len, MemBuffer::CodePage codepage = MemBuffer::kCodePageANSI);
    
    /// \brief Adds a ZipFileEntry to our internal list of all entries.
    /// \param entry The ZipFileEntry to add.
    /// \return True if the entry was added successfully.
	bool AddEntry( ZipFileEntry* entry );

private:
    /// \brief Zip file name if loaded from disk.
	QString mFileName;

    /// \brief Offset of start of central directory.
	qint64 mOffsetToCentralDir;

    /// \brief Size of the central directory.
    qint64 mSizeCentralDir;

    /// \brief Zip file comment.
	QString mComment;

    /// \brief Number of discs the archive is broken into.
	quint16 mNumOfDiscs;

	/// \brief List of ZipFileEntry objects.
	ZipFileEntryList mEntries;
};
       
/// \brief Holds information about a single file within a Zip file.
class ZipFileEntry
{
public: 
    /// \brief The ZipFileEntry constructor.
	ZipFileEntry();

    /// \brief The ZipFileEntry destructor.
	~ZipFileEntry();

	/// \brief Gets the file or directory name.
	const QString& GetFileName() const { return mFileName; }

	/// \brief Gets the file comment, if any.
	const QString& GetFileComment() const { return mFileComment; }

	/// \brief Gets the compression method (see PackageFileEntry::CompressionMethod for available values).
	quint16 GetCompressionMethod() const { return mCompressionMethod; }
    
	/// \brief Checks if the entry refers to a compressed file.
	/// \return True if the entry refers to a compressed file.
	bool IsCompressed() const { return mCompressionMethod != 0; }

	/// \brief Gets a human-readable string representing a compression method.
    /// \return A string representing the compression method.
	QString GetCompressionMethodName() const;
    
	/// \brief Gets the file modification date.
    /// \return The file modification date in MS DOS format.
	const quint16 GetLastModifiedDate() const  { return mLastModifiedDate; }
    
	/// \brief Gets the file modification time.
    /// \return The file modification time in MS DOS format.
	const quint16 GetLastModifiedTime() const  { return mLastModifiedTime; }

	/// \brief Gets the 32-bit CRC for this file.
    /// \return The 32-bit CRC for this file.
	quint32 GetCrc32() const { return mCrc32; }
	
	/// \brief Gets the compressed size.
	/// \return The compressed size.
	qint64 GetCompressedSize() const { return mCompressedSize; }
    
	/// \brief Gets the uncompressed size.
	/// \return The uncompressed size.
	qint64 GetUncompressedSize() const { return mUncompressedSize; }
    
	/// \brief Checks if this entry is a directory.
    /// \return True if this entry is a directory.
	bool IsDirectory() const;
    
    /// \brief Checks if this entry is a file.
    /// \return True if this entry is a file.
    bool IsFile() const { return !IsDirectory(); }

    /// \brief Gets the DOS attributes.
    /// 
    /// If the attribute format is ZipFileEntry::ZIP_ATTRFORMAT_UNIX, the low-order byte 
    /// of the file attributes (read-only, hidden, system, directory) is compatible.
    /// \return The MSDOS attributes.
	quint32 GetDOSAttributes() const;

    /// \brief Gets the Unix attributes.
    ///
    /// If this is not a Unix-compatible archive, this function will synthesize 
    /// the attributes from the DOS attributes.
    /// \return The Unix attributes.
    quint32 GetUnixAttributes() const;

    /// \brief Gets the attribute format (see ZipFileEntry::ZIP_ATTRFORMAT_UNIX and ZipFileEntry::ZIP_ATTRFORMAT_DOS).
    /// \return The attribute format.
    quint8 GetAttributeFormat() const;
    
    /// \brief Checks if this file is encrypted.
    ///
    /// Checks bit 1 of the general purpose flag.
    /// \return True if this file is encrypted.
	bool IsEncrypted() const;
    
    /// \brief Checks if this file has a CRC and sizes.
    ///
    /// Checks bit 3: if set, then no crc or sizes are supplied with the file header.
    /// \return True if the file header contains a CRC and sizes.
	bool HasCrcAndSizes() const;
    
    /// \brief Checks if this file is encoded in UTF-8.
    ///
    /// Checks bit 11: if set, filename and comment are UTF-8.
    /// \return True if the filename and comment are UTF-8.
	bool HasUtf8Encoding() const;

	/// \brief Gets any extra fields.
    /// \return MemBuffer object containing the data from the extra fields.
	const MemBuffer& GetExtraFields() const { return mExtraField; }

	/// \brief Gets the content of an extra field, if present.
	/// \param id ID of the extra field.
    /// \param buffer MemBuffer object to populate with data from the extra field.
    /// \return True if a field with the given ID was found and buffer was populated with its contents.
	bool GetExtraField(int id, MemBuffer& buffer) const;
	
	/// \brief Gets the offset of the local file header.
    /// \return The offset of the local file header.
	qint64 GetOffsetOfLocalHeader() const;
    
	/// \brief Gets the offset of the local file data.
    /// \return The offset of the local file data.
	qint64 GetOffsetOfFileData() const;

    /// \brief Gets the disc number that this entry starts on.
    /// \return The disc number this entry starts on.
	quint16 GetDiskNoStart() const { return mDiskNoStart; }

protected:
    /// \brief The ZipFileEntry constructor.
    /// \param info The file header describing this file or directory.
	ZipFileEntry(const FileHeader* info);

    /// \brief Sets the file name of this entry.
    /// \param name The file name of this entry.
	void setFileName( const QString& name ) { mFileName = name; }

    /// \brief Adds data the extra fields of this entry.
    /// \param buffer Buffer containing data to add to the extra fields.
	void setExtraField( MemBuffer& buffer );

    /// \brief Adds a file comment to this entry.
    /// \param cmnt The comment to add.
	void setFileComment( const QString& cmnt ) { mFileComment = cmnt; }
    
	/// \brief Resets all data to default values.
	void Clear();

private:
    /// \brief Name or identifier of this file or directory.
	QString mFileName;

    /// \brief Comment associated with this file or directory.
	QString mFileComment;

    /// \brief Version this file was made by.
	quint16 mVersionMade;

    /// \brief Version needed to extract this file.
	quint16 mVersionNeeded;

    /// \brief Method used to compress this file (see PackageFileEntry for available values).
	quint16 mCompressionMethod;

    /// \brief General purpose flags.
	quint16 mFlags;
    
    /// \brief The file modification date in MS DOS format.
	quint16 mLastModifiedDate;
    
    /// \brief The file modification time in MS DOS format.
	quint16 mLastModifiedTime;

    /// \brief The 32-bit CRC.
	quint32 mCrc32;

    /// \brief The compressed size.
	qint64 mCompressedSize;

    /// \brief The uncompressed size.
	qint64 mUncompressedSize;
    
    /// \brief Internal file attributes.
	quint16 mInternalAttribs;

    /// \brief External file attributes.
	quint32 mExternalAttribs;

    /// \brief Buffer containing any extra fields.
	MemBuffer mExtraField;

    /// \brief Relative offset of local header.
	qint64 mOffsetOfLocalHeader;

    /// \brief Relative offset of local file data.
	qint64 mOffsetOfLocalFileData;
    
    /// \brief The disc number this entry starts on.
	quint16 mDiskNoStart;
    
    /// \brief Hiword of 0 for the file attribute represents MS DOS format.
    static const quint8 ZIP_ATTRFORMAT_DOS = 0;
    
    /// \brief Hiword of 3 for the file attribute represents Unix format.
    static const quint8 ZIP_ATTRFORMAT_UNIX = 3;

	friend class ZipFileInfo;
};

} // namespace Downloader
} // namespace Origin

#endif
