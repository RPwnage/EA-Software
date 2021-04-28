#include "ContentProtocols.h"

#include <QDebug>
#include <QMetaType>
#include <QDir>
#include <QFileInfo>
#include <QElapsedTimer>

#include <set>

#include "services/debug/DebugService.h"

#include "services/downloader/File.h"
#include "PackageFile.h"
#include "services/downloader/FilePath.h"
#include "services/downloader/StringHelpers.h"
#include "services/downloader/Timer.h"
#include "services/downloader/DownloadService.h"

#include "services/escalation/IEscalationClient.h"

#include "engine/downloader/IContentInstallFlow.h"

#include "engine/content/ContentController.h"
#include "engine/utilities/ContentUtils.h"
#include "ZipFileInfo.h"

#include "services/log/LogService.h"
#include "TelemetryAPIDLL.h"

#define PKGLOG_PREFIX "[Game Title: " << getGameTitle() << "][ProductID:" << mProductId << "]"
#define UTILLOG_PREFIX "[Game Title: " << gameTitle << "][ProductID:" << productId << "]"
#define TRANSFER_PROGRESS_THROTTLE_MS 1000

#ifdef WIN32
#include <Windows.h>
#endif

namespace Origin
{
namespace Downloader
{
    namespace Util
    {
        static bool DeleteDirectoryAndContents(const QString &dirName)
        {
            bool result = true;
            QDir dir(dirName);
            
            if (dir.exists(dirName)) {
                Q_FOREACH(QFileInfo info, dir.entryInfoList(QDir::NoDotAndDotDot | QDir::System | QDir::Hidden  | QDir::AllDirs | QDir::Files, QDir::DirsFirst)) {
                    if (info.isDir()) {
                        result = DeleteDirectoryAndContents(info.absoluteFilePath());
                    }
                    else {
                        result = QFile::remove(info.absoluteFilePath());
                    }
                    
                    if (!result) {
                        return result;
                    }
                }
                result = dir.rmdir(dirName);
            }
            
            return result;
        }

    }

    static QString g_protocolStateStrings[] =
    {
        "ContentProtocolUnknown",
        "ContentProtocolInitializing",
        "ContentProtocolInitialized",
        "ContentProtocolStarting",
        "ContentProtocolResuming",
        "ContentProtocolRunning",
        "ContentProtocolPausing",
        "ContentProtocolPaused",
        "ContentProtocolCanceling",
        "ContentProtocolCanceled",
        "ContentProtocolFinished",
        "ContentProtocolError",
        "Unrecognized State"
    };
    
    const QString& IContentProtocol::protocolStateString(ContentProtocolState state)
    {
        if(state >= kContentProtocolUnknown && state <= kContentProtocolError)
        {
            return g_protocolStateStrings[state];
        }
        else
        {
            return g_protocolStateStrings[kContentProtocolError+1];
        }
    }

    static ContentProtocols *g_instProtocols = NULL;

    ContentProtocols* ContentProtocols::GetInstance()
    {
        // WARNING / DANGER / WARNING : This is NOT threadsafe right now
        // TODO: FIXME FIXME

        if (NULL == g_instProtocols)
            g_instProtocols = new ContentProtocols();

        return g_instProtocols;
    }

    void ContentProtocols::RegisterProtocol(IContentProtocol *protocol)
    {
        // Move the protocol object to the protocol thread
        protocol->moveToThread(&(GetInstance()->_protocolThread));
    }

    void ContentProtocols::RegisterWithProtocolThread(QObject *obj)
    {
        // Move the protocol object to the protocol thread
        obj->moveToThread(&(GetInstance()->_protocolThread));
    }

    void ContentProtocols::RegisterWithInstallFlowThread(QObject *flow)
    {
        // Move the protocol object to the protocol thread
        flow->moveToThread(&(GetInstance()->_flowThread));
    }

    ContentProtocols::ContentProtocols()
    {
        // Start the protocol thread to its event loop
        _protocolThread.start();
        Services::ThreadNameSetter::setThreadName(&_protocolThread, "ContentProtocol Thread");

        _flowThread.start();
        Services::ThreadNameSetter::setThreadName(&_flowThread, "InstallFlow Thread");
    }

    IContentProtocol::IContentProtocol(const QString& productId, Services::Session::SessionWRef session) :
        mSession(session),
        mProductId(productId),
        mTimeOfLastTransferProgress(0),
        mReportDebugInfo(false),
        mProtocolState(kContentProtocolUnknown)
    {

    }

    void IContentProtocol::setReportDebugInfo(bool reportDebugInfo)
    {
        mReportDebugInfo = reportDebugInfo;
    }

    void IContentProtocol::setProtocolState(ContentProtocolState newState)
    {
        ORIGIN_LOG_EVENT << PKGLOG_PREFIX << "Transitioning from " << protocolStateString(mProtocolState) << " to " << protocolStateString(newState);
        mProtocolState = newState;
    }

    ContentProtocolState IContentProtocol::protocolState()
    {
        return mProtocolState;
    }

    void IContentProtocol::setJobId(const QString& uuid)
    {
        mJobId = uuid;
    }

    QString IContentProtocol::getJobId()
    {
        return mJobId;
    }

    bool IContentProtocol::checkToSendThrottledUpdate()
    {
        if(GetTick() - mTimeOfLastTransferProgress >= TRANSFER_PROGRESS_THROTTLE_MS)
        {
            mTimeOfLastTransferProgress = GetTick();
            return true;
        }
        else
        {
            return false;
        }
    }

    int DownloadErrorFromEscalationError(int escalationError, int commandFailureError)
    {
        if (escalationError == Origin::Escalation::kCommandErrorNone)
        {
            return ContentDownloadError::OK;
        }
        else if (escalationError == Origin::Escalation::kCommandErrorUserCancelled)
        {
            return ProtocolError::EscalationFailureUserCancelled;
        }
        else if(escalationError == Origin::Escalation::kCommandErrorUACTimedOut)
        {
            return ProtocolError::EscalationFailureUACTimedOut;
        }
        else if(escalationError == Origin::Escalation::kCommandErrorNotElevatedUser)
        {
            return ProtocolError::EscalationFailureAdminRequired;
        }
        else if(escalationError == Origin::Escalation::kCommandErrorUACRequestAlreadyPending)
        {
            return ProtocolError::EscalationFailureUACRequestAlreadyPending;
        }
        else if (escalationError == Origin::Escalation::kCommandErrorProcessExecutionFailure)
        {
            return ProtocolError::EscalationFailureUnknown;
        }
        else if (escalationError == Origin::Escalation::kCommandErrorCommandFailed)
        {
            return commandFailureError;
        }
        return ProtocolError::EscalationFailureUnknown;
    }

    ContentDownloadError::type ContentProtocolUtilities::CreateDirectoryAllAccess(const QString& sDir, int* escalationErrorOut, int* escalationSysErrorOut)
    {
        if (escalationErrorOut)
            *escalationErrorOut = 0;
        if (escalationSysErrorOut)
            *escalationSysErrorOut = 0;

        ORIGIN_ASSERT(!sDir.isEmpty());
        if (sDir.isEmpty())
        {
            ORIGIN_LOG_WARNING << "CreateDirectoryAllAccess called with empty directory.";
            return (ContentDownloadError::type)ProtocolError::ContentFolderError;
        }

        QString escalationReasonStr = "CreateDirectoryAllAccess (" + sDir + ")";
        int escalationError = 0;
        QSharedPointer<Origin::Escalation::IEscalationClient> escalationClient;
        if (!Origin::Escalation::IEscalationClient::quickEscalate(escalationReasonStr, escalationError, escalationClient))
        {
            if(escalationErrorOut)
                *escalationErrorOut = escalationError;

            return (ContentDownloadError::type)DownloadErrorFromEscalationError(escalationError);
        }

        //We don't want default behavior, we will handle failures
        QString szSD("D:(A;OICI;GA;;;WD)");
        int err = escalationClient->createDirectory(sDir, szSD);

        if(escalationErrorOut)
            *escalationErrorOut = err;

        if (!Origin::Escalation::IEscalationClient::evaluateEscalationResult(escalationReasonStr, "createDirectory", err, escalationClient->getSystemError()))
        {
            if (escalationSysErrorOut)
                *escalationSysErrorOut = escalationClient->getSystemError();

            return (ContentDownloadError::type)DownloadErrorFromEscalationError(err, ProtocolError::ContentFolderError);
        }
          
        return (ContentDownloadError::type)ProtocolError::OK;
    }

    static bool string_longer_predicate( const QString& dir1, const QString& dir2 )
    {
        return dir1.length() > dir2.length();
    }

    bool ContentProtocolUtilities::PreallocateFiles(const PackageFile& packageFile)
    {
        bool result = true;
        QStringList createdDirs;
        QString sDestDir = packageFile.GetDestinationDirectory();

        // If this is a network path, replace the forward "\\" with the unicode path identifier
        // HACK FIXME HACK FIXME HACK - Unicode Path support is broken in QFile!!!
        //if (sDestDir.startsWith("\\\\"))
        //    sDestDir = QString("\\\\?\\UNC\\%1").arg(sDestDir.right( sDestDir.size() - 2));
        //else
        //    sDestDir.prepend("\\\\?\\");

        // Build a list of unique folder names from the files/folders in the package
        std::set<QString> uniqueFolders;
        std::list<QString> folders;

        QDir dir;

        for ( PackageFileEntryList::const_iterator it = packageFile.begin(); it != packageFile.end() && result; it++ )
        {
            if ((*it)->IsIncluded())
            {
                CFilePath fullPath = CFilePath::Absolutize( sDestDir, (*it)->GetFileName() );
                QString fullDir = fullPath.GetDirectory();

                //First create the directory if necessary
                if ( !dir.exists(fullDir) )
                {
                    uniqueFolders.insert(fullDir);
                }
            }
        }

        // Move the list of unique folders into a sortable list
        for (std::set<QString>::iterator it = uniqueFolders.begin(); it != uniqueFolders.end(); it++)
        {
            //QDEBUGEX << "Unique Folder \"" << *it << "\"";
            folders.push_back(*it);
        }

        // We'll sort the list of folders from longest to shortest (as that will let us delete children before parents.)
        folders.sort(string_longer_predicate);

        // If there are folders that need to be created
        if (!folders.empty())
        {
            // We need the escalation client in order to create our folders
            QString escalationReasonStr = "PreallocateFiles";
            int escalationError = 0;
            QSharedPointer<Origin::Escalation::IEscalationClient> escalationClient;
            if (!Origin::Escalation::IEscalationClient::quickEscalate(escalationReasonStr, escalationError, escalationClient))
                result = false;

            // Iterate all the files (unless result == false)
            for (std::list<QString>::iterator it = folders.begin(); (result && it != folders.end()); it++)
            {
                QString sFolder(*it);
                if (!dir.exists(sFolder))
                {
                    ORIGIN_LOG_EVENT << "Attempting to create: " << sFolder;
                    
                    escalationReasonStr = "PreallocateFiles (" + sFolder + ")";
                    if ( !Origin::Escalation::IEscalationClient::evaluateEscalationResult(escalationReasonStr, "createDirectory", escalationClient->createDirectory(sFolder), escalationClient->getSystemError()) )
                    {
                        ORIGIN_LOG_ERROR << "Unable to create directory \"" << sFolder << "\"";
                        
                        // Strip out extraneous folder info... we don't need to know the user's full directory structure.
                        CFilePath fullDir(sFolder);
                        QString strippedDir = sFolder;
                        strippedDir.remove(packageFile.GetDestinationDirectory());
                        strippedDir.prepend(fullDir.GetVolume() + "__");
                        int err = 0;
#ifdef ORIGIN_PC
#pragma message ("TODO: Create a means for the escalation client to either set this metric or return the error code.")
                        err = GetLastError();
#else
                        // TODO: Create a means for the escalation client to either set this metric or return the error code.
#endif
                        QString offerId = packageFile.GetProductId();
                        GetTelemetryInterface()->Metric_DL_ERROR_PREALLOCATE(offerId.toLocal8Bit().data(), "dir", strippedDir.toLocal8Bit().constData(), err, 0);
                        break;
                    }
                    else
                    {
                        createdDirs.push_back( sFolder );
                    }
                }
            }
        }

        // If we're still ok, go ahead and preallocate the 0-byte files
        if (result)
        {
            for ( PackageFileEntryList::const_iterator it = packageFile.begin(); it != packageFile.end() && result; it++ )
            {
                if ((*it)->IsIncluded())
                {
                    CFilePath fullPath = CFilePath::Absolutize( sDestDir, (*it)->GetFileName() );
                    CFilePath fullDir = fullPath.GetDirectory();

                    // Not preallocating files anymore.......  But still need to create 0 byte entries
                    if ( result && fullPath.IsFile() && (*it)->GetUncompressedSize() == 0 )
                    {
                        // Ensure that if the file is already on disk that it is not read-only

#ifdef ORIGIN_PC
                        
                        QFile::setPermissions(fullPath.ToString(), QFile::permissions(fullPath.ToString())|QFile::WriteOwner|QFile::WriteGroup|QFile::WriteUser);

#else
                        // Explicitly set relaxed permissions to allow resuming downloads by other users.
                        QFile::setPermissions(fullPath.ToString(), QFile::permissions(fullPath.ToString())|QFile::WriteOwner|QFile::WriteGroup|QFile::WriteUser|QFile::WriteOther);
                        
#endif
                        
                        //Now create the file
                        File file(fullPath.ToString());
                        if (!file.open(QIODevice::ReadWrite))
                        {
                            ORIGIN_LOG_ERROR << "Unable to set file size of \"" << fullPath.ToString() << "\" to " << (*it)->GetUncompressedSize() << ". (" << file.errorString() << ")";
                        
                            // Strip out extraneous folder info... we don't need to know the user's full directory structure.
                            QString strippedPath = fullPath.ToString();
                            strippedPath.remove(packageFile.GetDestinationDirectory());
                            strippedPath.prepend(fullDir.GetVolume() + "__");

                            QString offerId = packageFile.GetProductId();
                            GetTelemetryInterface()->Metric_DL_ERROR_PREALLOCATE(offerId.toLocal8Bit().data(), "file", strippedPath.toLocal8Bit().constData(), file.error(), (*it)->GetUncompressedSize());
                            return false;
                        }
                        else
                        {

#ifdef ORIGIN_MAC
                            
                            // Explicitly set relaxed permissions to allow resuming downloads by other users.
                            file.setPermissions(file.permissions(fullPath.ToString())|QFile::WriteOwner|QFile::WriteGroup|QFile::WriteUser|QFile::WriteOther);
                            
#endif
                            
                            file.close();
                        }
                    }

                }
            }
        }

        //Undo if failed
        if ( !result )
        {
            while ( !createdDirs.isEmpty() )
            {
                //Remove in reverse order as some
                //may be subdirs of other created dirs
                Util::DeleteDirectoryAndContents( createdDirs.takeLast() );
            }
        }

        return result;
    }

    bool ContentProtocolUtilities::ReadZipCentralDirectory(IDownloadSession *downloadSession, qint64 contentLength, const QString& gameTitle, const QString& productId, PackageFile &packageFile, int &numDiscs, DownloadErrorInfo& errInfo, QAtomicInt *syncRequestCount, QWaitCondition *waitCondition)
    {
        ZipFileInfo zip;
        MemBuffer buffer;
        const int READBLOCK = 8 * 1024;
        const quint32 MAX_CENTRAL_DIR_SCAN_SIZE = 5 * 1024 * 1024;	// there isn't any documented max size, but typically the central directory is a few hundred k max.  This is for crash prevention in the case of a badly formed zip file.
        bool result = true;

        qint64 offsetToRead = contentLength - READBLOCK;

        if ( offsetToRead < 0 )
            offsetToRead = contentLength / 2;

        if (syncRequestCount)
        {
            // Increment active sync requests to delay cancel requests from the client
            syncRequestCount->ref();
        }

        while ( result && offsetToRead >= 0 && buffer.TotalSize() < MAX_CENTRAL_DIR_SCAN_SIZE)
        {
            quint32 thisRead = (quint32) (contentLength - offsetToRead - buffer.TotalSize());

            if ( buffer.IsEmpty() )
            {
                buffer.Resize( thisRead );
            }
            else
            {
                MemBuffer newBuffer( buffer.TotalSize() + thisRead );
                memcpy( newBuffer.GetBufferPtr() + thisRead, buffer.GetBufferPtr(), buffer.TotalSize() );
                buffer.Assign( newBuffer );
            }

            qint64 nOffsetStart, nOffsetEnd;
            nOffsetStart = contentLength - buffer.TotalSize();
            nOffsetEnd = nOffsetStart + thisRead;

            if ((nOffsetEnd - nOffsetStart) == 0)
            {
                GetTelemetryInterface()->Metric_DL_IMMEDIATE_ERROR(productId.toLocal8Bit().constData(), "ReadZipCentralDirectory", "CouldNotFindZipInfo", __FILE__, __LINE__);

                ORIGIN_LOG_ERROR << UTILLOG_PREFIX << "ReadZipCentralDirectory - Could not read Zip file information.  No central directory found.  Content length: " << contentLength;
                POPULATE_ERROR_INFO_TELEMETRY(errInfo, ProtocolError::ZipContentInvalid, 0, productId.toLocal8Bit().constData());

                // If the content is less than 2k (likely not an actual zip file), we can just bulk send the data in telemetry, this could prove to be interesting to see what data comes back
                if (contentLength <= 2048)
                {
                    QByteArray contentBuffer((const char*)buffer.GetBufferPtr(), buffer.TotalSize());
                    QByteArray contentBufferEncoded = contentBuffer.toBase64();

                    ORIGIN_LOG_ERROR << UTILLOG_PREFIX << "Zip Directory Raw data: [" << contentLength << "][" << QString(contentBufferEncoded) << "]";

                    // Make this easily parseable from the telemetry TSVs.  Surround the critical values by delimieters '$' which are safe for telemetry
                    QString telemetrySafeStr = QString("ZipDirectoryRawData$%1$%2$").arg(contentLength).arg(QString(contentBufferEncoded));
                    telemetrySafeStr.replace("/", "_"); // Telemetry doesn't allow / characters, which appear in some Base64 implementations (not sure about Qt)

                    GetTelemetryInterface()->Metric_DL_IMMEDIATE_ERROR(productId.toLocal8Bit().constData(), "ZipDirectoryRawData", qPrintable(telemetrySafeStr), __FILE__, __LINE__);
                }

                return false;
            }

            //Fetch the actual data
            qint64 reqId = downloadSession->createRequest(nOffsetStart, nOffsetEnd);
            IDownloadRequest *request = downloadSession->getRequestById(reqId);

            result = downloadSession->requestAndWait(reqId);

            if ( result && request->chunkIsAvailable() && request->getErrorState() == 0 )
            {
                // Copy the data
                MemBuffer *reqBuffer = request->getDataBuffer();
                memcpy((char*)buffer.GetBufferPtr(), (char*)reqBuffer->GetBufferPtr(), thisRead);
                request->chunkMarkAsRead();

                //Clear any previous content
                zip.Clear();

                //Attempt to load with this data payload
                result = zip.TryLoad( buffer, contentLength, &offsetToRead );

                //Check: Ensure zip is not requesting something that is already there
                Q_ASSERT(result || (offsetToRead == 0) || (offsetToRead < contentLength - buffer.TotalSize()));
                
                if ( result )
                {
                    //Save a pointer to the used data so we can later mark that chunk on the map
                    //qDebug() << "central dir offset: " << offsetToRead;
                    //Done!!
                    // We're done with the request
                    downloadSession->closeRequest(reqId);
                    break;
                }
                else if ( offsetToRead >= 0 && offsetToRead < contentLength )
                {
                    //Try reading more
                    result = true;
                }
                else
                {
                    GetTelemetryInterface()->Metric_DL_IMMEDIATE_ERROR(productId.toLocal8Bit().constData(), "ReadZipCentralDirectory", "CouldNotReadZipInfo", __FILE__, __LINE__);

                    ORIGIN_LOG_ERROR << UTILLOG_PREFIX << "InitializerGetZipFileInfo - Could not read Zip file information";
                    POPULATE_ERROR_INFO_TELEMETRY(errInfo, ProtocolError::ContentInvalid, 0, productId.toLocal8Bit().constData());
                }
            }

            if(!result && request->getErrorState() != 0)
            {
                //  Here we are seeing the generic (-1) error, which shouldn't happen.
                POPULATE_ERROR_INFO_TELEMETRY(errInfo, request->getErrorState(), request->getErrorCode(), productId.toLocal8Bit().constData());
            }

            // We're done with the request
            downloadSession->closeRequest(reqId);
        }

        //Wake cancel requests that were delayed because of an sync network request
        if (syncRequestCount)
        {
            syncRequestCount->deref();
            if (syncRequestCount <= 0 && waitCondition)
                waitCondition->wakeAll();
        }

        if ( result )
        {
            //  TELEMETRY: Need to figure out what case is failing
            bool loadResult = packageFile.LoadFromZipFile(zip);
            if ( loadResult == false )
            {
                GetTelemetryInterface()->Metric_DL_IMMEDIATE_ERROR(productId.toLocal8Bit().constData(), "ReadZipCentralDirectory", "CouldNotLoadFromZipFile", __FILE__, __LINE__);
            }
            bool isEmptyResult = packageFile.IsEmpty();
            if ( isEmptyResult == true )
            {
                GetTelemetryInterface()->Metric_DL_IMMEDIATE_ERROR(productId.toLocal8Bit().constData(), "ReadZipCentralDirectory", "PackageFileIsEmpty", __FILE__, __LINE__);
            }

            if  ( !loadResult || isEmptyResult )
            {
                ORIGIN_LOG_ERROR << UTILLOG_PREFIX << "InitializerGetZipFileInfo - Error reading ZIP file contents";
                POPULATE_ERROR_INFO_TELEMETRY(errInfo, ProtocolError::ZipContentInvalid, 0, productId.toLocal8Bit().constData());
                result = false;
            }
            else
            {
                ORIGIN_LOG_DEBUG << UTILLOG_PREFIX << "zip contains files: " << packageFile.GetNumberOfFiles();
            }

            //RunBalanceEntries();

            numDiscs = zip.GetNumberOfDiscs();
            ORIGIN_LOG_DEBUG << UTILLOG_PREFIX << "Zip num disks: " << zip.GetNumberOfDiscs();
        }

        return result;
    }

    bool ContentProtocolUtilities::VerifyPackageFile(QString unpackPath, qint64 contentLength, const QString& gameTitle, const QString& productId, PackageFile &packageFile)
    {
        bool result = true;

        CFilePath baseDir = unpackPath; 

        PackageFileEntryList::iterator it;

        for ( it = packageFile.begin(); it != packageFile.end(); it++ )
        {
            const PackageFileEntry* entry = *it;

            CFilePath fullPath = CFilePath::Absolutize( baseDir, entry->GetFileName() );
            
            //We are testing if the file name is either absolute or a relative of the form ../../
            if ( !fullPath.IsSubdirOf(baseDir) && (baseDir != fullPath.GetDirectory()) )
            {
                ORIGIN_LOG_ERROR << UTILLOG_PREFIX << "VerifyPackageFile - File \"" << entry->GetFileName() << "\" is invalid (not a subdir of the unpack dir)";
                result = false;
                break;
            }

            // Skip unconfirmed entries so we don't break multi-disc/ITO
            if (!entry->IsVerified())
                continue;

            //Now we check the offsets, files should not overlap.
            PackageFileEntryList::iterator itNext = it;
            itNext++;

            qint64 nextOffset = 0;

            if ( itNext != packageFile.end() )
            {
                const PackageFileEntry* nextEntry = *itNext;
                
                nextOffset = nextEntry->GetOffsetToHeader();

                if ( nextOffset == 0 )
                {
                    nextOffset = nextEntry->GetOffsetToFileData();
                }
            }
            else
            {
                nextOffset = contentLength;
            }

            if ( entry->IsFile() && (nextOffset != 0) && (entry->GetOffsetToFileData() + entry->GetCompressedSize() > nextOffset) )
            {			
                ORIGIN_LOG_ERROR << UTILLOG_PREFIX << "VerifyPackageFile - File \"" << entry->GetFileName() << "\" has invalid offsets";
                result = false;
                break;
            }
        }

        return result;
    }

    void IContentProtocol::setGameTitle(const QString &gameTitle)
    {
        mGameTitle = gameTitle;
    }

    QString IContentProtocol::getGameTitle() const
    {
        return mGameTitle;
    }

} // namespace Downloader
} // namespace Origin

