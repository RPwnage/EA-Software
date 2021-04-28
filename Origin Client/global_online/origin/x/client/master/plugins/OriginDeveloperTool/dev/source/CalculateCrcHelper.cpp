//    CalculateCrcHelper.cpp
//    Copyright (c) 2013, Electronic Arts
//    All rights reserved.

#include "OriginDeveloperTool/CalculateCrcHelper.h"
#include "engine/utilities/ContentUtils.h"
#include "engine/content/ContentController.h"
#include "services/downloader/FilePath.h"
#include "services/log/LogService.h"
#include "services/downloader/StringHelpers.h"
#include "engine/debug/DownloadDebugDataManager.h"

#include "ContentProtocols.h"
#include "ContentProtocolPackage.h"
#include <QFileDialog>

namespace Origin
{
namespace Plugins
{
namespace DeveloperTool
{
    CalculateCrcHelper::CalculateCrcHelper(const Origin::Engine::Content::ContentConfigurationRef content) :
        mProtocol(NULL),
        mContent(content)
    {
    }

    CalculateCrcHelper::~CalculateCrcHelper()
    {
        cleanup();
    }

    void CalculateCrcHelper::cleanup()
    {
        if(mProtocol)
        {
            mProtocol->Cancel();
            mProtocol->deleteLater();
            mProtocol = NULL;
        }
    }

    bool CalculateCrcHelper::calculateCrc(const QString& downloadUrl)
    {
        // fail for NULL or non-dip content.
        if(mContent == NULL || !mContent->dip())
        {
            ORIGIN_LOG_WARNING << "Cannot calculate crc with NULL or non-dip content!";
            return false;
        }
        else if(downloadUrl.isEmpty())
        {
            ORIGIN_LOG_WARNING << "[" << mContent->productId() << "] Cannot calculate crc with empty download URL.";
            return false;
        }
        else
        {
            // create temporary directories to serve as the install and cache paths for the download protocol
            QString cachePath = QDir::tempPath() + QDir::separator() + "crcTempCache";
            if(!QDir().mkpath(cachePath))
            {
                ORIGIN_LOG_WARNING << "Could not create temp cache directory [" << cachePath << "] to calculate crc.";
                return false;
            }
            QString installPath = QDir::tempPath() + QDir::separator() + "crcTempInstall";
            if(!QDir().mkpath(installPath))
            {
                ORIGIN_LOG_WARNING << "Could not create temp install directory [" << installPath << "] to calculate crc.";
                return false;
            }

            QString crcMapPrefix;
            if (mContent->isPDLC())
                crcMapPrefix = mContent->installationDirectory();
            mProtocol = new Origin::Downloader::ContentProtocolPackage(mContent->productId(), cachePath, crcMapPrefix,
                Origin::Engine::Content::ContentController::currentUserContentController()->user()->getSession());
            mProtocol->setGameTitle(mContent->displayName());
            mProtocol->SetCrcMapKey(QString("%1").arg(qHash(mContent->installCheck())));
            mProtocol->setReportDebugInfo(true);

            ORIGIN_VERIFY_CONNECT_EX(mProtocol, SIGNAL(Initialized()), this, SLOT(onProtocolInitialized()), Qt::QueuedConnection);
            ORIGIN_VERIFY_CONNECT_EX(mProtocol, SIGNAL(IContentProtocol_Error(Origin::Downloader::DownloadErrorInfo)), this, SLOT(Protocol_Error(Origin::Downloader::DownloadErrorInfo)), Qt::QueuedConnection);
            
            mProtocol->Initialize(downloadUrl, installPath, false);
            return true;
        }
    }

    void CalculateCrcHelper::Protocol_Error(Origin::Downloader::DownloadErrorInfo errorInfo)
    {
        ORIGIN_LOG_WARNING << "[" << mContent->productId() << "] Protocol error while retrieving DiP manifest from local override: [errorCode:" << errorInfo.mErrorCode << ", errorType:" << errorInfo.mErrorType << "]";
        emit finished();
    }

    void CalculateCrcHelper::onProtocolInitialized()
    {  
        if(mProtocol->protocolState() == Origin::Downloader::kContentProtocolInitialized)
        {
            mProtocol->ComputeDownloadJobSizeBegin();
            ORIGIN_LOG_ACTION << "Processed package file entry map.";

            // Disconnect for all signals from protocol
            QObject::disconnect(this, NULL);

            QPointer<Engine::Debug::DownloadDebugDataCollector> collector = Engine::Debug::DownloadDebugDataManager::instance()->getDataCollector(mContent->productId()); 

            if(collector)
            {
                QString path = QFileDialog::getSaveFileName(NULL, QString(), Origin::Services::PlatformService::getStorageLocation(QStandardPaths::DesktopLocation), ".csv");
        
                if(!path.isEmpty())
                {
                    if(!path.contains('.'))
                    {
                        path.append(".csv");
                    }
                    QFile file(path);
                    if(!file.open(QIODevice::WriteOnly | QIODevice::Text))
                    {
                        ORIGIN_LOG_ERROR << "Error saving crc data to: " << path << ". Error: " << file.error();
                    }

                    QTextStream stream(&file);

                    QStringList headers;
                    headers.append("File Name");
                    headers.append("CRC");
                    stream << headers.join(",") << endl;

                    QStringList fields;
                    QMap<QString, Engine::Debug::DownloadFileMetadata> allFiles = collector->getDownloadFiles();

                    for(QMap<QString, Engine::Debug::DownloadFileMetadata>::ConstIterator it = allFiles.begin(); it != allFiles.constEnd(); ++it)
                    {
                        fields.clear();
                        fields.append(it.value().fileName());
                        fields.append(QString("0x%1").arg(it.value().packageFileCrc(), 8, 16));
                        stream << fields.join(",") << endl;
                    }

                    file.close();
                }
            }
        }
        else
        {
            ORIGIN_LOG_ERROR << "[" << mContent->productId() << "] Failed to initialize protocol with URL.";
        }
        emit finished();
    }
}
}
}
