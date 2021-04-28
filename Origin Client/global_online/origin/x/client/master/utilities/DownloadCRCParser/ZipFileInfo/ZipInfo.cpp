#include <QtCore/QCoreApplication>

#include <QDebug>
#include <QFile>
#include "ZipFileInfo.h"
#include "CorePackageFile.h"

namespace ZipInfo
{
    bool svc_requestBytes(QFile* file, qint64 start, qint64 end, MemBuffer& buffer)
    {
        qint64 len = end - start;
        if (len <= 0)
        {
            qDebug() << "ZipInfo::svc_requestBytes - invalid length";
            return false;
        }

        qDebug() << "ZipInfo::svc_requestBytes - start: " << start << " end: " << end << " size: " << len;

        file->reset();
        file->seek(start);
        if (file->read((char*)buffer.GetBufferPtr(), len) != len)
        {
            qDebug() << "ZipInfo::svc_requestBytes - read error";
            return false;
        }

        return true;
    }

    bool RetrieveOffsetToFileData(QFile* file, QMap<qint64, qint64>& headerOffsetTofileDataOffsetMap, PackageFileEntry *pEntry)
    {
        if (!pEntry)
            return false;

        //Skip already verified entries
        if (pEntry->IsVerified())
            return true;

        if (pEntry->IsDirectory())
            return true;

        //see if our map has an entry and use that offset 
        //when trying to RetrieveOffsetToFile data will fail, if the file you are trying
        //to get the offset for does not begin on the current disc
        if (headerOffsetTofileDataOffsetMap.contains(pEntry->GetOffsetToHeader()) && !pEntry->IsVerified())
        {
            pEntry->setOffsetToFileData(headerOffsetTofileDataOffsetMap[pEntry->GetOffsetToHeader()]);
            pEntry->setIsVerified(true);
            return true;
        }

        //Buffer used to fetch headers
        MemBuffer headerBuffer(ZipFileInfo::LocalFileHeaderFixedPartSize);

        qint64 startOffset = pEntry->GetOffsetToHeader();

        //Fetch the actual data
        bool result = svc_requestBytes(file, startOffset, startOffset + headerBuffer.TotalSize(), headerBuffer);
        if (result)
        {
            stLocalFileHeader localHeaderInfo;

            headerBuffer.Rewind();
            result = ZipFileInfo::readLocalFileHeader(localHeaderInfo, headerBuffer);

            if (result)
            {
                qint64 dataOffset = startOffset + ZipFileInfo::LocalFileHeaderFixedPartSize +
                    localHeaderInfo.filenameLength + localHeaderInfo.extraFieldLength;
                headerOffsetTofileDataOffsetMap[pEntry->GetOffsetToHeader()] = dataOffset;
                pEntry->setOffsetToFileData(dataOffset);
                pEntry->setIsVerified(true);
                return true;
            }

            //ORIGIN_LOG_ERROR << L"Could not load local file header for " << sLocalFilename << L".";
            return false;
        }
        else
        {
            
        }

        return false;
    }

    bool processZipFile(QFile* file, qint64 contentLength, PackageFile &packageFile)
    {
        ZipFileInfo zip;
        MemBuffer buffer;
        const int READBLOCK = 8 * 1024;
        const int MAX_CENTRAL_DIR_SCAN_SIZE = 5 * 1024 * 1024;	// there isn't any documented max size, but typically the central directory is a few hundred k max.  This is for crash prevention in the case of a badly formed zip file.
        bool result = true;

        qint64 offsetToRead = contentLength - READBLOCK;

        if (offsetToRead < 0)
            offsetToRead = contentLength / 2;

        while (result && offsetToRead >= 0 && buffer.TotalSize() < MAX_CENTRAL_DIR_SCAN_SIZE)
        {
            u_long thisRead = (u_long)(contentLength - offsetToRead - buffer.TotalSize());

            if (buffer.IsEmpty())
            {
                buffer.Resize(thisRead);
            }
            else
            {
                MemBuffer newBuffer(buffer.TotalSize() + thisRead);
                memcpy(newBuffer.GetBufferPtr() + thisRead, buffer.GetBufferPtr(), buffer.TotalSize());
                buffer.Assign(newBuffer);
            }

            qint64 nOffsetStart, nOffsetEnd;
            nOffsetStart = contentLength - buffer.TotalSize();
            nOffsetEnd = nOffsetStart + thisRead;

            //Fetch
            result = svc_requestBytes(file, nOffsetStart, nOffsetEnd, buffer);

            if (result)
            {
                //Clear any previous content
                zip.Clear();

                //Attempt to load with this data payload
                result = zip.TryLoad(buffer, contentLength, &offsetToRead);

                //Check: Ensure zip is not requesting something that is already there
                Q_ASSERT(result || (offsetToRead == 0) || (offsetToRead < contentLength - buffer.TotalSize()));

                if (result)
                {
                    //Save a pointer to the used data so we can later mark that chunk on the map
                    qDebug() << "ZipInfo - Central directory offset: " << offsetToRead;

                    //Done!!
                    break;
                }
                else if (offsetToRead >= 0 && offsetToRead < contentLength)
                {
                    //Try reading more
                    result = true;
                }
                else
                {
                    qDebug() << L"InitializerGetZipFileInfo - Could not read Zip file information";
                    //SetError(ERR_INVALID_FORMAT, NO_ERROR);
                }
            }
        }

        //if ( mbShutDownInitializer )
        //{
        //    result = false;
        //}

        if (result)
        {
            if (!packageFile.LoadFromZipFile(zip) || packageFile.IsEmpty())
            {
                qDebug() << L"InitializerGetZipFileInfo - Error reading ZIP file contents";
                //SetError(ERR_INVALID_FORMAT, NO_ERROR);
                result = false;
            }
            else
            {
                QMap<qint64, qint64> headerOffsetTofileDataOffsetMap;

                qDebug() << "ZipInfo - Archive contains files: " << packageFile.GetNumberOfFiles();

                PackageFileEntryList files = packageFile.GetEntries();
                for (PackageFileEntryList::iterator entry = files.begin(); entry != files.end(); entry++)
                {
                    PackageFileEntry *pkgfile = *entry;

                    RetrieveOffsetToFileData(file, headerOffsetTofileDataOffsetMap, pkgfile);

                    //if (pkgfile->GetFileName().toLower() == "__installer/installerdata.xml")
                    //if (pkgfile->GetOffsetToFileData() == 5509548003 || pkgfile->GetOffsetToHeader() == 5509548003)
                    //{
                    //    qDebug() << "Start: " << pkgfile->GetOffsetToHeader() << " End: " << pkgfile->GetEndOffset() << " File: " << pkgfile->GetFileName();

                        /*
                        if (pkgfile->GetCompressionMethod() != 0)
                        {
                            qDebug() << "installerdata.xml was compressed.";
                            break;
                        }

                        MemBuffer dipManifest(pkgfile->GetEndOffset() - pkgfile->GetOffsetToFileData());
                        if (svc_requestBytes(file, pkgfile->GetOffsetToFileData(), pkgfile->GetEndOffset(), dipManifest))
                        {
                            QString installerdata;
                            dipManifest.AsQString(installerdata);

                            qDebug() << installerdata;

                            //qDebug() << "Read bytes.";
                        }
                        */
                    //}
                }
            }

            //RunBalanceEntries();

            qDebug() << "ZipInfo - Archive has " << zip.GetNumberOfDiscs() << " disks.";
        }

        if (!result)
        {
            //mPackageFile.Clear();
        }

        return result;
    }

    PackageFile* processZipFile(QString filename)
    {
        
        //ZipFileInfo file;

        //if (!file.Load("K:\\Dev\\EA\\temp\\tiger_2012.zip"))
        //{
        //    qDebug() << "Error!";
        //}

        QFile file(filename);
        qint64 contentLength = file.size();

        file.open(QIODevice::ReadOnly);

        PackageFile *packageFile = new PackageFile;

        qDebug() << "ZipInfo - Processing " << filename;
        if (processZipFile(&file, contentLength, *packageFile))
        {
            qDebug() << "ZipInfo - Success!";

            return packageFile;
        }

        delete packageFile;

        return NULL;
    }
} //namespace ZipInfo