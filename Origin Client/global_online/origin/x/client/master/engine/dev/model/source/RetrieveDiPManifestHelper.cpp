
#include "RetrieveDiPManifestHelper.h"

#include "engine/utilities/ContentUtils.h"
#include "engine/content/ContentController.h"
#include "services/downloader/FilePath.h"
#include "services/log/LogService.h"
#include "services/downloader/StringHelpers.h"

#include "engine/downloader/ContentServices.h"
#include "ContentProtocols.h"
#include "ContentProtocolPackage.h"

namespace Origin 
{
namespace Downloader
{
    QHash<QString, DiPManifest> RetrieveDiPManifestHelper::sDiPManifestCache;

    void RetrieveDiPManifestHelper::clearManifestCache()
    {
        sDiPManifestCache.clear();
    }

    RetrieveDiPManifestHelper::RetrieveDiPManifestHelper(const QString& downloadUrl, const Origin::Engine::Content::ContentConfigurationRef content, bool preserveManifestOnDisk) :
        mDownloadUrl(downloadUrl),
        mPreserveFile(preserveManifestOnDisk),
        mManifestDiskPath(""),
        mContent(content)
    {
              
    }

    bool RetrieveDiPManifestHelper::retrieveDiPManifestSync()
    {
        mManifest.Clear();

        // fail for NULL or non-dip content.
        if(mContent == NULL || !mContent->dip())
        {
            ORIGIN_LOG_WARNING << "Cannot retrieve DiP manifest with NULL or non-dip content!";
            return false;
        }
        else if(mDownloadUrl.isEmpty())
        {
            ORIGIN_LOG_WARNING << "[" << mContent->productId() << "] Cannot retrieve DiP manifest with empty download URL.";
            return false;
        }
        else if(!mPreserveFile && sDiPManifestCache.contains(mContent->productId()))
        {
            // :TODO: BUG - sDiPManifestCache is keyed by productId, not download URL!
            mManifest = sDiPManifestCache.value(mDownloadUrl);
            return !mManifest.IsEmpty();
        }
        else
        {
            bool result = false;

            Origin::Engine::Content::ContentController *contentController = Origin::Engine::Content::ContentController::currentUserContentController();
            if(contentController == NULL)
                return false;

            Origin::Engine::UserRef currentUser = contentController->user();
            if(currentUser == NULL)
                return false;

            Origin::Downloader::ContentProtocolPackage* packageProtocol = new Origin::Downloader::ContentProtocolPackage(mContent->productId(), ContentServices::GetTempFolderPath(), "", currentUser->getSession());
            packageProtocol->setGameTitle(mContent->displayName());

            QEventLoop eventLoop;
            ORIGIN_VERIFY_CONNECT_EX(packageProtocol, SIGNAL(Initialized()), &eventLoop, SLOT(quit()), Qt::QueuedConnection);
            ORIGIN_VERIFY_CONNECT_EX(packageProtocol, SIGNAL(IContentProtocol_Error(Origin::Downloader::DownloadErrorInfo)), this, SLOT(Protocol_Error(Origin::Downloader::DownloadErrorInfo)), Qt::QueuedConnection);
            ORIGIN_VERIFY_CONNECT(this, SIGNAL(protocolError()), &eventLoop, SLOT(quit()));

            packageProtocol->SetSkipCrcVerification(true);
            packageProtocol->Initialize(mDownloadUrl, "", false);

            eventLoop.exec();

            ORIGIN_VERIFY_DISCONNECT(packageProtocol, SIGNAL(Initialized()), &eventLoop, SLOT(quit()));

            if(packageProtocol->protocolState() == Origin::Downloader::kContentProtocolInitialized)
            {
                QString tempFileName = ContentServices::GetTempFilename(mContent->productId(), "man");
                ORIGIN_VERIFY_CONNECT_EX(packageProtocol, SIGNAL(FileDataReceived(QString, QString)), this, SLOT(ManifestFileReceived(QString, QString)), Qt::QueuedConnection);
                ORIGIN_VERIFY_CONNECT_EX(packageProtocol, SIGNAL(FileDataReceived(QString, QString)), &eventLoop, SLOT(quit()), Qt::QueuedConnection);
                packageProtocol->GetSingleFile(mContent->dipManifestPath(), tempFileName); 
                eventLoop.exec();

                result = !mManifest.IsEmpty();
            }
            else
            {
                ORIGIN_LOG_ERROR << "[" << mContent->productId() << "] Failed to initialize protocol with URL.";
            }

            sDiPManifestCache.insert(mContent->productId(), mManifest);

            delete packageProtocol;
            return result;
        }
    }

    DiPManifest& RetrieveDiPManifestHelper::dipManifest()
    {
        return mManifest;
    }

    void RetrieveDiPManifestHelper::Protocol_Error(Origin::Downloader::DownloadErrorInfo errorInfo)
    {
        ORIGIN_LOG_WARNING << "[" << mContent->productId() << "] Protocol error while retrieving DiP manifest from local override: [errorCode:" << errorInfo.mErrorCode << ", errorType:" << errorInfo.mErrorType << "]";
        emit protocolError();
    }

    QString& RetrieveDiPManifestHelper::manifestFilePath() 
    {
        return mManifestDiskPath;
    }

    void RetrieveDiPManifestHelper::ManifestFileReceived(const QString& archivePath, const QString& diskPath)
    {
        ORIGIN_LOG_DEBUG << "[" << mContent->productId() << "] Successfully retrieved " << archivePath << " parsing into DiPManifest.";
        
        QFile manifestFile(diskPath);
            
        if (manifestFile.exists())
        {
            if(!mManifest.Parse(manifestFile, mContent->isPDLC()))
            {
                ORIGIN_LOG_ERROR << "[" << mContent->productId() << "] Failed to parse manifest for product id: " << mContent->productId();
                mManifest.Clear();
            }
            manifestFile.close();
            if(mPreserveFile)
            {
                QString newFileName = diskPath;
                newFileName.replace(".tmp", ".xml");
                manifestFile.rename(diskPath, newFileName);
                mManifestDiskPath = newFileName;
            }
            else
            {
                manifestFile.remove(diskPath);
            }
        }
        else
        {
            ORIGIN_LOG_ERROR << "[" << mContent->productId() << "] Manifest file does not exist on disk.";
            mManifest.Clear();
        }
    }
}
}
