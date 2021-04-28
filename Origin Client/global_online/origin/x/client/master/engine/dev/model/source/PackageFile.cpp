// PackageFile and PackageFileEntry implementations

#include "services/downloader/Parameterizer.h"
#include "services/log/LogService.h"
#include "ZipFileInfo.h"
#include "BigFileInfo.h"
#include "services/downloader/FilePath.h"
#include "services/downloader/StringHelpers.h"
#include "services/downloader/CRCMap.h"
#include "services/platform/envutils.h"
#include "TelemetryAPIDLL.h"
#include <services/debug/DebugService.h>
#include "PackageFile.h"

#include <QDir>

namespace Origin
{
namespace Downloader
{

/**
Write a string to a stdio file. Catch exceptions and print errors
*/
static bool SafeWriteString(QFile& file, const QString& line )
{
    bool result = false;

    if (file.write(reinterpret_cast<const char*>(line.utf16()), line.length()*sizeof(QChar)))
        result = true;
    else
        ORIGIN_LOG_ERROR << L"Unable to write to file " << file.fileName() << L".";

    return result;
}

static QString sizeToString( qint64 bytes )
{
    QString result;

    if ( bytes < 1000 )
    {
        result = QString("%1 bytes").arg(QString::number(bytes));
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

PackageFile::PackageFile()
{
}

PackageFile::~PackageFile()
{
    Clear();
}

bool PackageFile::LoadFromManifest( const QString& filename )
{
    QString sFullManifest;
    if (!StringHelpers::LoadQStringFromFile(filename, sFullManifest))
    {
        ORIGIN_LOG_ERROR << L"Unable to open " << filename << L".";
        return false;
    }

    Clear();

    quint32 packageFileCRC = 0;
    quint32 computedCRC = CRCMap::GetCRCInitialValue();

    qint32 nIndex = 0;
    while (nIndex >= 0 && nIndex < sFullManifest.length())
    {
        qint32 nStartLine = nIndex;
        if (nStartLine < 0)
            break;

        qint32 nEndLine = sFullManifest.indexOf("\n", nStartLine+1);
        if (nEndLine < 0)		// if there are no more new lines, then we're at the end of the file
            nEndLine = sFullManifest.length();

        // absorb all newline characters
        while (nEndLine < sFullManifest.length() && (sFullManifest.at(nEndLine) == '\r' || sFullManifest.at(nEndLine) == '\n'))
            nEndLine++;

        QString sLine = sFullManifest.mid(nStartLine, nEndLine-nStartLine);

        if (nIndex == 0)
        {
            // This is the file header
            //Read the header
            Parameterizer param;
            bool result = param.Parse(sLine);

            if ( result )
            {
                mName = param.GetString("name");
                mType = param.GetString("type");
                mDestDir = param.GetString("dest");
                if (param.IsParameterPresent("crc"))
                {
                    packageFileCRC = param.Get64BitInt("crc");
                }
            }
            else
            {
                ORIGIN_LOG_ERROR << L"Unable to read manifest header " << filename << L".";
                return false;
            }
        }
        else
        {
            // This is an entry
            PackageFileEntry* entry = new PackageFileEntry();

            // Adjust the CRC for each entry
            QByteArray entryData = sLine.toLocal8Bit();
            computedCRC = CRCMap::CalculateCRC(computedCRC, entryData.constData(), entryData.length());

            bool result = entry->fromString(sLine);

            if ( result )
                AddEntry(entry);
            else
            {
                ORIGIN_LOG_ERROR << L"Unable to read entry " << filename << L".";
                delete entry;
                return false;
            }
        }

        nIndex = nEndLine;	// advance the index
    }

    computedCRC = CRCMap::CalculateFinalCRC(computedCRC);

    if (packageFileCRC != 0 && packageFileCRC != computedCRC)
    {
        ORIGIN_LOG_ERROR << "CRC Mismatch on Manifest Load \"" << filename;

        Clear();

        return false;
    }
    else
    {
        ORIGIN_LOG_EVENT << "CRC Matched for Manifest Load \"" << filename;
    }

//	BalanceEntries();
    SortEntries();
    return true;
}

bool PackageFile::SaveManifest( const QString& filename ) const
{
    QFile file(filename);
    // EBIBUGS-19113, ensure that the file is gone before (re)creating it
    if ( file.exists() )
    {
        file.setPermissions(QFile::WriteOwner);

        if (file.remove() == false)
        {
            GetTelemetryInterface()->Metric_ERROR_DELETE_FILE("PackageFile_SaveManifest", filename.toUtf8().data(), file.error());
        }
    }
    if ( file.open(QIODevice::WriteOnly) )
        return SaveManifest( file );
    return false;
}

bool PackageFile::SaveManifest( QFile& file ) const
{
    
#ifdef ORIGIN_MAC
    
    // Explicitly set relaxed permissions to allow resuming downloads by other users.
    file.setPermissions(QFile::WriteOwner|QFile::ReadOwner|QFile::WriteUser|QFile::ReadUser|QFile::WriteGroup|QFile::ReadGroup|QFile::WriteOther|QFile::ReadOther);
    
#endif
    
    bool result = true;

    // Write Unicode Signature
    char sig[2];
    sig[0] = '\xFF';
    sig[1] = '\xFE';
    file.write(sig, 2);

    quint32 packageFileCRC = CRCMap::GetCRCInitialValue();

    QList<QString> packageFileEntries;

    //for each entry serialize and save a line
    PackageFileEntryList::const_iterator it = mEntries.begin();

    while ( it != mEntries.end() && result )
    {
        QString szEntry = (*it)->ToString();

        if ( !szEntry.isEmpty() )
        {
            szEntry.append("\n");

            // Adjust the CRC for each entry
            QByteArray entryData = szEntry.toLocal8Bit();
            packageFileCRC = CRCMap::CalculateCRC(packageFileCRC, entryData.constData(), entryData.length());

            packageFileEntries.push_back(szEntry);
        }
        else
        {
            result = false;
        }

        it++;
    }

    // Finalize the CRC
    packageFileCRC = CRCMap::CalculateFinalCRC(packageFileCRC);

    if (!result)
        return false;

    //Save the header
    Parameterizer param;
    param.SetString("name", mName);
    param.SetString("type", mType);
    param.SetString("dest", mDestDir);
    param.Set64BitInt("crc", packageFileCRC);

    QString szHeader;
    if ( !param.GetPackedParameterString(szHeader) )
    {
        return false;
    }

    szHeader.append("\n");
    result = SafeWriteString(file, szHeader);

    //for each entry serialize and save a line
    QList<QString>::Iterator entriesIter = packageFileEntries.begin();

    while ( entriesIter != packageFileEntries.end() && result )
    {
        QString szEntry = *entriesIter;

        result = SafeWriteString(file, szEntry);

        entriesIter++;
    }

    if (result)
    {
        ORIGIN_LOG_EVENT << "Save manifest \"" << file.fileName() << "\" succeeded.";
    }
    else
    {
        ORIGIN_LOG_ERROR << "Save manifest \"" << file.fileName() << "\" failed.";
    }

    return result;
}

bool PackageFile::LoadFromZipFile( const QString& filename )
{
    bool result = false;
    ZipFileInfo info;

    Clear();

    if ( info.Load(filename) )
    {
        result = LoadFromZipFile(info);
    }
    else
    {
        ORIGIN_LOG_ERROR << L"Unable to load " << filename;
    }

    return result;
}

bool PackageFile::LoadFromZipFile( const ZipFileInfo& info )
{
    bool result = true;

    Clear();

    ZipFileEntryList::const_iterator it = info.begin();

    while ( it != info.end() )
    {
        PackageFileEntry* entry = new PackageFileEntry;
        entry->fromZipEntry(**it);
        AddEntry(entry);
        it++;
    }

    if ( result )
    {
        mName = info.GetFileName();
        mType = "ZIP";
    }

    //BalanceEntries();
    SortEntries();

    return result;
}

bool PackageFile::LoadFromBigFile( const QString& filename )
{
    bool result = false;
    BigFileInfo info;

    Clear();

    if ( info.Load(filename) )
        result = LoadFromBigFile(info);
    else
        ORIGIN_LOG_ERROR << L"Unable to load " << filename;

    return result;
}

//	This function is used only in the VIV download flow.
bool PackageFile::LoadFromBigFile( const BigFileInfo& info )
{
    bool result = true;

    Clear();

    BigFileEntryList::const_iterator it = info.begin();

    while ( it != info.end() )
    {
        PackageFileEntry* entry = new PackageFileEntry;
        entry->fromBigEntry(**it);
        AddEntry(entry);
        it++;
    }

    if ( result )
    {
        mName = info.GetFileName();
        mType = "BIG";
    }

    SortEntriesByOffset();

    return result;
}

void PackageFile::Clear()
{
    mName.clear();
    mType.clear();

    PackageFileEntryList::iterator it = mEntries.begin();

    while ( it != mEntries.end() )
    {
        delete *it;
        it++;
    }

    mEntries.clear();
}

void PackageFile::SetDestinationDirectory( const QString& dir )
{
    if ( !dir.isEmpty() )
    {
        CFilePath path(dir);
        path.MakeAbsolute();
        mDestDir = path.GetDirectory();
    }
    else
    {
        mDestDir.clear();
    }
}

bool PackageFile::RecreateEntries() const
{
    // Disabling pre-allocation for DiP.
    // Disk full errors will still be handled in the middle of downloading if that occurs.
    // This can take exceptionally long for some packages with a lot of files, and there is no explicit state to communicate that to the user.
    return true;
}

// Changed to delete everything within and including the target unpack folder because users shouldn't be storing stuff there anyway.
bool PackageFile::EraseEntries() const
{
    bool result = true;

    QDir dir(mDestDir);

    if ( dir.exists(mDestDir) )
    {
        result = EnvUtils::DeleteFolderIfPresent(mDestDir, true, true);
        if (!result)
            ORIGIN_LOG_ERROR << L"Error deleting unpack-on-the-fly package contents. Path: " << mDestDir;
    }

    return result;
}

/*
CString PackageFile::Dump()
{
    CString result;
    //Dump standard data
    result.Format( L"Package archive type %s has %d entries (%u files and %u dirs).\n", (LPCTSTR) GetType(),
                                                                                        GetNumberOfEntries(),
                                                                                        GetNumberOfFiles(),
                                                                                        GetNumberOfDirectories() );

    PackageFileEntryList::iterator it = mEntries.begin();

    //Lets calculate the greatest filename length
    int max_namel = 0, max_sizel = 0;
    while ( it != mEntries.end() )
    {
        max_namel = max(max_namel, (*it)->GetFileName().GetLength());
        max_sizel = max(max_sizel, sizeToString((*it)->GetUncompressedSize()).GetLength());
        it++;
    }

    CString szFormat;
    szFormat.Format(L"%%-%d.%ds | %%%d.%ds | %%s\n", max_namel + 1, max_namel + 1,
                                                     max_sizel + 1, max_sizel + 1);

    it = mEntries.begin();

    while ( it != mEntries.end() )
    {
        CString line;
        PackageFileEntry* entry = *it;
        line.Format( (LPCTSTR) szFormat, (LPCTSTR) entry->GetFileName(), (LPCTSTR) sizeToString(entry->GetUncompressedSize()), entry->IsDirectory() ? L"dir" : L"file" );
        result.Append(line);
        it++;
    }

    return result;
}
*/

QString PackageFile::Dump()
{
    QString result;
    //Dump standard data
    result = QString("Package archive type %1 has %2 entries (%3 files and %4 dirs).\n").arg(GetType()).arg(GetNumberOfEntries()).arg(GetNumberOfFiles()).arg(GetNumberOfDirectories());

    PackageFileEntryList::iterator it = mEntries.begin();

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
        QString line;
        PackageFileEntry* entry = *it;
        line = QString("%1 | %2 | %3").arg(entry->GetFileName(), max_namel + 1).arg(sizeToString(entry->GetUncompressedSize()), max_sizel + 1).arg(entry->IsDirectory() ? "dir" : "file");
        result.append(line);
        it++;
    }

    return result;
}

quint32 PackageFile::GetNumberOfFiles() const
{
    quint32 count = 0;
    PackageFileEntryList::const_iterator it = mEntries.begin();

    while ( it != mEntries.end() )
    {
        if ( (*it)->IsFile() )
            count++;

        it++;
    }

    return count;
}

quint32 PackageFile::GetNumberOfDirectories() const
{
    quint32 count = 0;
    PackageFileEntryList::const_iterator it = mEntries.begin();

    while ( it != mEntries.end() )
    {
        if ( (*it)->IsDirectory() )
            count++;

        it++;
    }

    return count;
}

quint32 PackageFile::GetNumberOfIncludedFiles() const
{
    quint32 count = 0;
    PackageFileEntryList::const_iterator it = mEntries.begin();

    while ( it != mEntries.end() )
    {
        if ( (*it)->IsFile() && (*it)->IsIncluded() )
            count++;

        it++;
    }

    return count;
}


bool PackageFile::IsEmpty() const
{
    return mEntries.empty();
}

qint64 PackageFile::GetUnpackedSize() const
{
    qint64 size = 0;
    PackageFileEntryList::const_iterator it = mEntries.begin();

    while ( it != mEntries.end() )
    {
        size += (*it)->GetUncompressedSize();
        it++;
    }

    return size;
}

int PackageFile::GetEntriesInRange(PackageFileConstEntryList& list, qint64 offset, quint32 len, int currentDiskIndex) const
{
    int result = 0;
    PackageFileEntryList::const_iterator it = mEntries.begin();

    //Advance to the first file that matches on the expected disk
    while ( (it != mEntries.end() && (*it)->GetEndOffset() <= offset)  || ((*it)->GetDiskNoStart() != currentDiskIndex ))
    {
        it++;
    }

    //Insert until no files match (also stop if we hit the end of disk)
    while ( it != mEntries.end() && (*it)->GetOffsetToFileData() < offset + len && ((*it)->GetDiskNoStart() == currentDiskIndex ))
    {
        list.push_back(*it);
        result++;
        it++;
    }

    return result;
}

const PackageFileEntry* PackageFile::GetEntryByName(const QString& sFileName, bool caseSensitive) const
{
    QString sFilenameTemp = sFileName;

    sFilenameTemp.replace(L'\\', L'/');

    // Capture the fact that manifest filenames sometimes prepend the path separator
    if (sFilenameTemp.startsWith("\\") || sFilenameTemp.startsWith("/"))
    {
        sFilenameTemp.remove(0,1);
    }

    //Locate the manifest
    for(PackageFileEntryList::const_iterator it = begin(); it != end(); ++it)
    {
        const QString &filename = (*it)->GetFileName();

        if ((!caseSensitive && (filename.compare(sFilenameTemp, Qt::CaseInsensitive) == 0)) ||
             (caseSensitive && (filename.compare(sFilenameTemp) == 0)) )
        {
            return *it;
        }
    }
    return NULL;
}

bool PackageFile::AddEntry( PackageFileEntry* entry )
{
    mEntries.push_back(entry);
    return true;
}

/*
bool PackageFile::EntryExists( const PackageFileEntry* entry ) const
{
    CFilePath fullPath = CFilePath::Absolutize(mDestDir, entry->GetFileName());
    return fullPath.Exists();
}*/

// Return whether first element is before the second in the package file
static bool entry_less_predicate ( const PackageFileEntry* entry1, const PackageFileEntry* entry2 )
{
    //we use the header offset as the sort value here instead of the data offset, because the
    //data offset could be -1 if its the last file on a disk (due to the way zip info calcualtes the offset)
    // data offset = (next file offset) - (current file compressed size)

    qint64 off1 = entry1->GetOffsetToHeader();
    qint64 off2 = entry2->GetOffsetToHeader();

    int diskIndex1 = entry1->GetDiskNoStart();
    int diskIndex2 = entry2->GetDiskNoStart();

    //if the disks are the same see which is bigger and return out
    if(diskIndex1 != diskIndex2)
        return diskIndex1 < diskIndex2;

    if ( off1 == off2 )
    {
        //If the offsets match then at least one of the two has size == 0
        if ( entry1->GetCompressedSize() == 0 )
        {
            //If both equal zero then sort alphabetically
            if ( entry2->GetCompressedSize() == 0 )
            {
                // the "<" operator compares only Unicode values between QStrings, so use localeAwareCompare() to sort alphabetically.
                return QString::localeAwareCompare(entry1->GetFileName(), entry2->GetFileName()) < 0;
            }

            return true;
        }
    }

    return off1 < off2;
}

// Return whether first element is larger than second in the package file
// This should sort the files from largest to smallest
//static bool entry_larger_predicate ( const PackageFileEntry* entry1, const PackageFileEntry* entry2 )
//{
//	qint64 size1 = entry1->GetCompressedSize();
//	qint64 size2 = entry2->GetCompressedSize();
//
//	return size1 > size2;
//}

void PackageFile::SortEntries()
{
    mEntries.sort(entry_less_predicate);
}


// Return whether first element is larger than second in the package file
// This should sort the files from largest to smallest
static bool entry_offset_predicate ( const PackageFileEntry* entry1, const PackageFileEntry* entry2 )
{
    qint64 size1 = entry1->GetOffsetToFileData();
    qint64 size2 = entry2->GetOffsetToFileData();

    return size1 < size2;
}


/**
Sorts the entries to be most contiguous so downloader can optimally cluster
*/
void PackageFile::SortEntriesByOffset()
{
    mEntries.sort(entry_offset_predicate);
}

/*void PackageFile::BalanceEntries()
{
    mEntries.sort(entry_larger_predicate);

    PackageFileEntryList balancedList;
    while (!mEntries.empty())
    {
        // Grab the front item (the next largest item to consider)
        PackageFileEntryList::iterator it = mEntries.begin();
        const qint64 kFillerDivisor = 10;
        qint64 nFillSize = (*it)->GetUncompressedSize() / kFillerDivisor;		// fill in based on a percentage of the size of the large item

        // Add it to the balanced list
        balancedList.push_front( *it );
        mEntries.pop_front();

        // For the next N items, use small items until they add up to nFillSize calculated above
        qint64 nFilled = 0;
        while (!mEntries.empty() && nFilled < nFillSize)
        {
            PackageFileEntryList::reverse_iterator rIt = mEntries.rbegin();
            balancedList.push_front( *(rIt) );
            nFilled += (*rIt)->GetUncompressedSize();
            mEntries.pop_back();
        }
    }

    mEntries = balancedList;
}*/


PackageFileEntry::PackageFileEntry()
{
    mFileRedownloadCount = 0;
    Clear();
}

PackageFileEntry::~PackageFileEntry()
{
}

void PackageFileEntry::setIsDirectory( bool isDir )
{
    mIsDir = isDir;
    if ( mIsDir )
    {
        mCompressedSize = mUncompressedSize = 0;
    }
}

void PackageFileEntry::setIsVerified( bool ver )
{
    mVerified = ver;
}

void PackageFileEntry::SetIsIncluded( bool bIncluded )
{
    mbIncluded = bIncluded;
}

void PackageFileEntry::SetIsDiffUpdatable( bool bDiffUpdatable )
{
    mbDiffUpdatable = bDiffUpdatable;
}


void PackageFileEntry::setCompressionMethod( quint16 comp )
{
    mCompression = comp;
}

void PackageFileEntry::Clear()
{
    mFileName.clear();
    mFileModificationTime = QTime();
    mFileModificationDate = QDate();
    mOffsetToHeader = 0;
    mOffsetToFileData = 0;
    mCompressedSize = 0;
    mUncompressedSize = 0;
    mAttributes = 0;
    mIsDir = false;
    mVerified = false;
    mbIncluded = true;
    mbDiffUpdatable = true;
    mCompression = Uncompressed;
    mDiskNoStart = 0;
    mCRC = 0;
}

bool PackageFileEntry::IsValid() const
{
    return !mFileName.isEmpty();
}

bool PackageFileEntry::IsCompressed() const
{
    return (mCompression != Uncompressed);
}

quint16 PackageFileEntry::GetCompressionMethod() const
{
    return mCompression;
}

bool PackageFileEntry::fromString( const QString& str )
{
    bool result;
    Parameterizer params;

    result = params.Parse(str);

    if ( result )
    {
        result = fromParameters(params);
    }

    return result;
}

QString PackageFileEntry::ToString() const
{
    QString str;
    Parameterizer param;

    if ( toParameters(param) )
    {
        if ( !param.GetPackedParameterString(str) )
        {
            str.clear();
        }
    }

    return str;
}

bool PackageFileEntry::fromParameters( const Parameterizer& param )
{
    bool result;

    QString szValue;
    qint64 iValue = 0;

    Clear();

    result = param.GetString("name", szValue);
    setFileName( szValue );


    quint32 nDate		= param.GetULong("filedate");
    quint32 nTime		= param.GetULong("filetime");
    setFileModificationTime((quint16) nTime);
    setFileModificationDate((quint16) nDate);

    setFileCRC( param.GetULong("crc") );

    if ( result )
    {
        result = param.Get64BitInt("head", iValue);
        setOffsetToHeader(iValue);
    }

    if ( result )
    {
        result = param.Get64BitInt("data", iValue);
        setOffsetToFileData(iValue);
    }

    if ( result )
    {
        result = param.Get64BitInt("len", iValue);
        setCompressedSize(iValue);
    }

    if ( result )
    {
        result = param.Get64BitInt("ulen", iValue);
        setUncompressedSize(iValue);
    }

    if ( result )
    {
        bool isDir = false;
        result = param.GetBool("dir", isDir);
        setIsDirectory(isDir);
    }

    if ( result )
    {
        bool isVer = false;
        result = param.GetBool("ver", isVer);
        setIsVerified(isVer);
    }

    if ( result )
    {
        long diskNoStart = 0;
        result = param.GetLong("disc", diskNoStart);
        setDiskNoStart(diskNoStart);
    }

    if ( result )
    {
        unsigned long uComp = Uncompressed;
        result = param.GetULong("compression", uComp);
        setCompressionMethod((quint16) uComp);
    }
    
    if ( result )
    {
        // For backwards-compatibility, its OK if this fails
        unsigned long attrs = 0;
        param.GetULong("attrs", attrs);
        setFileAttributes(attrs);
    }

    return (result && IsValid());
}

bool PackageFileEntry::toParameters( Parameterizer& param ) const
{
    qint64 val;
    bool result = param.SetString("name", GetFileName());

/*	result &= param.SetLong("year",		mFileModificationTime.GetYear());
    result &= param.SetLong("month",	mFileModificationTime.GetMonth());
    result &= param.SetLong("day",		mFileModificationTime.GetDay());
    result &= param.SetLong("hour",		mFileModificationTime.GetHour());
    result &= param.SetLong("minute",	mFileModificationTime.GetMinute());
    result &= param.SetLong("second",	mFileModificationTime.GetSecond());*/

    result &= param.SetULong("filedate", (quint32) DateTime::QDateToMsDosDate(mFileModificationDate));
    result &= param.SetULong("filetime", (quint32) DateTime::QTimeToMsDosTime(mFileModificationTime));

    result &= param.SetULong("crc", mCRC);

    val = GetOffsetToHeader();
    result &= param.Set64BitInt("head", val);
    val = GetOffsetToFileData();
    result &= param.Set64BitInt("data", val);
    val = GetCompressedSize();
    result &= param.Set64BitInt("len", val);
    val = GetUncompressedSize();
    result &= param.Set64BitInt("ulen", val);
    result &= param.SetBool("dir", IsDirectory());
    result &= param.SetULong("compression", GetCompressionMethod());
    result &= param.SetBool("ver", IsVerified());
    result &= param.SetLong("disc", GetDiskNoStart());
    result &= param.SetLong("attrs", GetFileAttributes());
    return result;
}

void PackageFileEntry::fromZipEntry( const ZipFileEntry& e )
{
    setFileName( e.GetFileName() );
    setFileModificationTime( e.GetLastModifiedTime() );
    setFileModificationDate( e.GetLastModifiedDate() );
    setFileCRC( e.GetCrc32() );
    setOffsetToHeader( e.GetOffsetOfLocalHeader() );
    setOffsetToFileData( e.GetOffsetOfFileData() );
    setCompressedSize( e.GetCompressedSize() );		//We care about the packed file
    setUncompressedSize( e.GetUncompressedSize() );		//We care about the packed file
#if defined (ORIGIN_PC)
    setFileAttributes(e.GetDOSAttributes());
#else
    setFileAttributes(e.GetUnixAttributes());
#endif
    setIsDirectory( e.IsDirectory() );
    setCompressionMethod( e.GetCompressionMethod() );
    setIsVerified( mOffsetToFileData > mOffsetToHeader );
    setDiskNoStart( e.GetDiskNoStart() );
}

void PackageFileEntry::fromBigEntry( const BigFileEntry& e )
{
    setFileName( e.GetFileName() );
    setFileCRC( 0 ); // no CRC
    setOffsetToHeader( 0 ); //No header
    setOffsetToFileData( e.GetOffset() );
    setCompressedSize( e.GetSize() );
    setUncompressedSize( e.GetSize() );
    setIsDirectory( e.IsDirectory() );
    setCompressionMethod( Uncompressed );
    setIsVerified( false );
    setDiskNoStart( 0 );
}

} // namespace Downloader
} // namespace Origin

