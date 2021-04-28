
#include "engine/debug/CalculateUpdateDetailsHelper.h"

#include "engine/utilities/ContentUtils.h"
#include "engine/content/ContentController.h"
#include "services/downloader/FilePath.h"
#include "services/log/LogService.h"
#include "services/downloader/StringHelpers.h"

#include "ContentProtocols.h"
#include "ContentProtocolPackage.h"

namespace Origin
{
namespace Engine
{
namespace Debug
{
    CalculateUpdateDetailsHelper::CalculateUpdateDetailsHelper(const QString& downloadUrl, const Origin::Engine::Content::ContentConfigurationRef content) :
        mProtocol(NULL),
        mDownloadUrl(downloadUrl),
        mContent(content)
    {
    
    }

    CalculateUpdateDetailsHelper::~CalculateUpdateDetailsHelper()
    {
        cleanup();
    }

    void CalculateUpdateDetailsHelper::cleanup()
    {
        if(mProtocol)
        {
            mProtocol->Cancel();
            mProtocol->deleteLater();
            mProtocol = NULL;
        }
    }

    bool CalculateUpdateDetailsHelper::calculateUpdateDetails(const QString& installPath, const QString& cachePath)
    {
        // fail for NULL or non-dip content.
        if(mContent == NULL || !mContent->dip())
        {
            ORIGIN_LOG_WARNING << "Cannot calculate update details with NULL or non-dip content!";
            return false;
        }
        else if(mDownloadUrl.isEmpty())
        {
            ORIGIN_LOG_WARNING << "[" << mContent->productId() << "] Cannot calculate update details with empty download URL.";
            return false;
        }
        else
        {
            QString crcMapPrefix;
            if (mContent->isPDLC())
                crcMapPrefix = mContent->installationDirectory();
            mProtocol = new Origin::Downloader::ContentProtocolPackage(mContent->productId(), cachePath, crcMapPrefix, 
                Origin::Engine::Content::ContentController::currentUserContentController()->user()->getSession());
            mProtocol->setGameTitle(mContent->displayName());
            mProtocol->SetUpdateFlag(true);
            mProtocol->SetRepairFlag(false);
            mProtocol->SetCrcMapKey(QString("%1").arg(qHash(mContent->installCheck())));

            ORIGIN_VERIFY_CONNECT_EX(mProtocol, SIGNAL(Initialized()), this, SLOT(onProtocolInitialized()), Qt::QueuedConnection);
            ORIGIN_VERIFY_CONNECT_EX(mProtocol, SIGNAL(IContentProtocol_Error(Origin::Downloader::DownloadErrorInfo)), this, SLOT(Protocol_Error(Origin::Downloader::DownloadErrorInfo)), Qt::QueuedConnection);
            mProtocol->Initialize(mDownloadUrl, installPath, false);
            return true;
        }
    }

    void CalculateUpdateDetailsHelper::Protocol_Error(Origin::Downloader::DownloadErrorInfo errorInfo)
    {
        ORIGIN_LOG_WARNING << "[" << mContent->productId() << "] Protocol error while retrieving DiP manifest from local override: [errorCode:" << errorInfo.mErrorCode << ", errorType:" << errorInfo.mErrorType << "]";
        emit protocolError();
    }

    void CalculateUpdateDetailsHelper::onProtocolInitialized()
    {  
        if(mProtocol->protocolState() == Origin::Downloader::kContentProtocolInitialized)
        {
            mProtocol->ComputeDownloadJobSizeBegin();
            ORIGIN_LOG_ACTION << "Processed package file entry map.";

            // Disconnect for all signals from protocol
            QObject::disconnect(this, NULL);
        }
        else
        {
            ORIGIN_LOG_ERROR << "[" << mContent->productId() << "] Failed to initialize protocol with URL.";
        }
    }

}
}
}
