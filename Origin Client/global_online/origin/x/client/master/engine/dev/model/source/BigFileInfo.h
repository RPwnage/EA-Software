#ifndef _BIGFILEINFO_H_
#define _BIGFILEINFO_H_

#include "services/downloader/MemBuffer.h"
#include <QString>
#include <QFile>

#include <list>

namespace Origin
{
namespace Downloader
{

//Forward declarations
class stBIGFileHeader;
class BigFileEntry;
class MemBuffer;

typedef std::list<BigFileEntry*> BigFileEntryList;

/// \brief Holds information about a BIG file (VIV) and its contents.
class BigFileInfo
{
public:
    /// \brief The BigFileInfo constructor.
	BigFileInfo();

    /// \brief The BigFileInfo destructor.
	~BigFileInfo();

	/// \brief Attempts to load a BIG file.
    /// \param filename A string path to the file to load.
    /// \return True if the file was loaded successfully.
	bool Load( const QString& filename );
    
	/// \brief Attempts to load a BIG file.
    /// \param file A QFile object created from the BIG file.
    /// \return True if the file was loaded successfully.
	bool Load( QFile& file );

	/// \brief Attemps to load a BIG file from a buffer containing header and TOC.
    ///
	/// To know the size of the header for a given BIG file, use GetHeaderSize().
	/// \param buffer A MemBuffer object containing the header and TOC.
    /// \return True if the file was loaded successfully.
	bool Load( MemBuffer& buffer );

	/// \brief Removes all entries and prepares for reuse.
	void Clear();

	/// \brief Dumps contents to a debug stream.
	void Dump();

	/// \brief Retrieves the file name (if loaded from a file).
    /// \return A string representing the file name.
	const QString& GetFileName() const { return mFileName; }

	/// \brief Gets the full header size (including TOC) by reading the beginning of the file.
	///
    /// This function validates the BIG file type.
    /// \param bof A pointer to a buffer of AT LEAST FileHeaderSize bytes.
	/// \return 0 if the function fails, the size of the header otherwise.
	static quint32 GetHeaderSize( quint8* bof );

	/// \brief Gets the BIG file type.
    /// \return A constant representing the BIG file type (see BigFileInfo::BIG2Type, BigFileInfo::BIG3Type, BigFileInfo::BIG4Type, BigFileInfo::BIG5Type, BigFileInfo::BIGFType, BigFileInfo::c0fbType).
	quint32 GetBIGFileType() const { return mFileType; } 
    
	/// \brief Checks if the file is of the BIG2 type.
    /// \return True if the file type is BigFileInfo::BIG2Type.
	bool IsBIG2() const { return mFileType == BIG2Type; }

	/// \brief Checks if the file is of the BIG3 type.
    /// \return True if the file type is BigFileInfo::BIG3Type.
	bool IsBIG3() const { return mFileType == BIG3Type; }
    
	/// \brief Checks if the file is of the BIG4 type.
    /// \return True if the file type is BigFileInfo::BIG4Type or BigFileInfo::BIGFType.
	bool IsBIG4() const { return mFileType == BIG4Type || mFileType == BIGFType; }
    
	/// \brief Checks if the file is of the BIG5 type.
    /// \return True if the file type is BigFileInfo::BIG5Type.
	bool IsBIG5() const { return mFileType == BIG5Type; }
    
	/// \brief Checks if the file is of the c0fb type.
    /// \return True if the file type is BigFileInfo::c0fbType.
	bool Isc0fb() const { return mFileType == c0fbType; }

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

	/// \brief Gets the unpacked size of all files.
    /// \return The unpacked size of all files.
	qint64 GetUnpackedSize() const;

	/// \brief Gets a list of all entries.
    /// \return A list of all entries.
	const BigFileEntryList& GetEntries() const { return mEntries; };

	/// \brief Gets an iterator to the first entry.
    /// \return An iterator to the first entry.
	BigFileEntryList::const_iterator begin() const { return mEntries.begin(); }

	/// \brief Gets an iterator to the last entry. This will not be a valid item.
    /// \return An iterator to the last entry.
	BigFileEntryList::const_iterator end() const { return mEntries.end(); }

    /// \brief Constant representing the BIG2 type.
	static const quint32 BIG2Type;
    
    /// \brief Constant representing the BIG3 type.
	static const quint32 BIG3Type;
    
    /// \brief Constant representing the BIG4 type.
	static const quint32 BIG4Type;
    
    /// \brief Constant representing the BIG5 type.
	static const quint32 BIG5Type;

    /// \brief Constant representing the c0fb type.
	static const quint32 c0fbType;

    /// \brief Constant representing the BIGF type.
	static const quint32 BIGFType;

    /// \brief The size of the file header.
	static const quint32 FileHeaderSize;

private:
    /// \brief Reads the BIG file header and populates the info parameter.
    /// \param info An stBIGFileHeader object that will be populated by this function.
    /// \param data A MemBuffer object containing the BIG file header.
    /// \return True if the header was successfully read.
	static bool readBIGFileHeader( stBIGFileHeader& info, MemBuffer& data );

    /// \brief Gets a human-readable string that represents a BIG file type.
    /// \return A string representing the BIG file type (i.e. "BIG2" for BigFileInfo::BIG2Type).
	static QString GetBIGFileTypeDescription( quint32 type );

	/// \brief Reads and returns a newly allocated BigFileEntry from a data buffer.
    ///
	/// The data buffer pointer will end up pointing to the byte immediately after the header ending byte.
	/// \param data A MemBuffer object containing the BIG file.
    /// \param error The function will set this to true if there is an error in the input data.
    /// \return NULL if the function failed, a pointer to a new BigFileEntry object otherwise.
    /// \note If the buffer is insufficient, the function returns NULL but error is false.
	/// If there is an error in the input data, the function returns NULL but error is true.
	BigFileEntry* readTOCFileHeader( MemBuffer& data, bool& error ) const;

    /// \brief Adds a BigFileEntry to our internal list of all entries.
    /// \param entry The BigFileEntry to add.
    /// \return True if the entry was added successfully.
	bool AddEntry( BigFileEntry* entry );

private:
    /// \brief The BIG file name if loaded from disk.
	QString mFileName;

    /// \brief The BIG file type.
	quint32 mFileType;

    /// \brief The number of entries in the BIG file.
	quint32 mNumEntries;

    /// \brief The length of the header.  Also represents the offset to the start of TOC.
	qint64 mHeaderLen;

    /// \brief The size of the TOC.
    qint64 mSizeTOC;

    /// \brief The size of the BIG file.
	qint64 mSizeFile;

    /// \brief A list of all entries in the BIG file.
	BigFileEntryList mEntries;
};

/// \brief Holds information about a single file or directory within a BIG (VIV) file.
class BigFileEntry
{
public:
    /// \brief The BigFileEntry constructor.
	BigFileEntry();

    /// \brief The BigFileEntry destructor.
	~BigFileEntry();

	/// \brief Gets the name of the file or directory.
    /// \return A string representing the name of the file.
	const QString& GetFileName() const { return mFileName; }

	/// \brief Gets the unpacked size of the file.
    /// \return The unpacked size of the file.
	quint32 GetSize() const { return mSize; }

	/// \brief Checks whether or not this entry is a directory.
    /// \return True if this entry is a directory.
	bool IsDirectory() const;

	/// \brief Gets the offset of the TOC header from the start of the BIG file.
    /// \return The offset of the TOC header from the start of the BIG file.
	quint32 GetOffsetOfLocalHeader() const { return mOffsetOfTOCHeader; }
    
	/// \brief Gets the offset of the entry from the start of the BIG file.
    /// \return The offset of the entry from the start of the BIG file.
	qint64 GetOffset() const { return mOffset; }

	/// \brief Checks that the entry contains a valid file or directory name.
    /// \return True if the entry contains a valid file or directory name.
	bool IsValid() const;

protected:
    /// \brief Sets the file or directory name of the entry.
    /// \param name The name of the file or directory.
	void setFileName( const QString& name ) { mFileName = name; }

    /// \brief Sets the offset to this entry from the start of the BIG file.
    /// \param off The offset to this entry from the start of the BIG file.
	void setOffset( qint64 off ) { mOffset = off; }

    /// \brief Sets the size of this entry.
    /// \param sz The size of this entry.
	void setSize( quint32 sz ) { mSize = sz; }

    /// \brief Clears all information about this entry.
	void Clear();

private:
    /// \brief The name of the file or directory that this entry represents.
	QString mFileName;

    /// \brief The size of the file or directory that this entry represents.
	quint32 mSize;

    /// \brief The offset to this entry from the start of the BIG file.
	qint64 mOffset;

    /// \brief Offset of TOC header from the start of the BIG file.
	quint32 mOffsetOfTOCHeader;
	

	friend class BigFileInfo;
};

} // namespace Downloader
} // namespace Origin

#endif
