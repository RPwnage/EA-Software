#include "engine/downloader/DynamicDownloadController.h"
#include "engine/content/ContentController.h"
#include "services/downloader/TransferStats.h"
#include "services/debug/DebugService.h"
#include "services/log/LogService.h"
#include "services/downloader/DownloaderErrors.h"

#include "ContentProtocolPackage.h"

#include <QSharedPointer>
#include <QFileInfo>
#ifdef _DEBUG
#include "PackageFile.h"
#endif

#include "DynamicDownloadControllerInternal.h"

namespace Origin
{
namespace Downloader
{
    static DynamicDownloadController *g_Instance = NULL;

    DynamicDownloadController::DynamicDownloadController() : mData(new DynamicDownloadControllerData(this)), mDataLock(QMutex::Recursive)
    {
        ORIGIN_LOG_EVENT << "[DDC] Controller created.";
    }

    DynamicDownloadController::~DynamicDownloadController()
    {
        delete mData;
    }

    DynamicDownloadController* DynamicDownloadController::GetInstance()
    {
        if (!g_Instance)
        {
            g_Instance = new DynamicDownloadController();
        }

        return g_Instance;
    }

    bool DynamicDownloadController::RegisterProtocol(const QString& offerId, const QString& packagePath, ContentProtocolPackage* packageProtocol, bool clearExisting)
    {
        QMutexLocker lock(&mDataLock);

        ORIGIN_LOG_EVENT << "[DDC] Protocol registered for content id = " << offerId;

        // Register the new package protocol and hook up signals
        if (!mData->HasInstanceData(offerId))
        {
            Internal::DynamicDownloadInstanceData* instanceData = new Internal::DynamicDownloadInstanceData(mData, offerId);
            // Ensure that this instance is assigned to the main DDC thread
            instanceData->moveToThread(this->thread());
            mData->SetInstanceData(offerId, QSharedPointer<Internal::DynamicDownloadInstanceData>(instanceData));
        }

        QSharedPointer<Internal::DynamicDownloadInstanceData> instanceData = mData->GetInstanceDataForOffer(offerId);
        if (!instanceData.isNull())
        {
            instanceData->RegisterProtocol(packagePath, packageProtocol, clearExisting);
            return true;
        }

        return false;
    }

    void DynamicDownloadController::ClearDynamicDownloadData(QString offerId)
    {
        // This must be run asynchronously such that it happens on the DDC thread
        ASYNC_INVOKE_GUARD_ARGS(Q_ARG(QString, offerId));

        QMutexLocker lock(&mDataLock);

        ORIGIN_LOG_EVENT << "[DDC] Clear dynamic download data for content id = " << offerId;

        // Clear the instance data
        if (mData->HasInstanceData(offerId))
        {
            mData->ClearInstanceData(offerId);
        }
    }

    bool DynamicDownloadController::IsProgressiveInstallationAvailable(const QString& offerId)
    {
        QMutexLocker lock(&mDataLock);

        ORIGIN_LOG_EVENT << "[DDC][ID=" << offerId << "] Is PI available?";

        QSharedPointer<Internal::DynamicDownloadInstanceData> instanceData = mData->GetInstanceDataForOffer(offerId);
        if (instanceData.isNull())
        {
            // Perhaps there is cached data we can load?
            if (mData->LoadInstanceData(offerId))
            {
                instanceData = mData->GetInstanceDataForOffer(offerId);
            }
        }

        if (!instanceData.isNull())
        {
            return instanceData->IsProgressiveInstallationAvailable();
        }

        ORIGIN_LOG_EVENT << "[DDC] Unable to find matching offer Id, PI unavailable.";

        // Assume it isn't available if we can't match the offer Id
        return false;
    }

    void DynamicDownloadController::SetChunkPriority(const QString& offerId, QVector<int> chunkIds)
    {
        QMutexLocker lock(&mDataLock);

        ORIGIN_LOG_EVENT << "[DDC][ID=" << offerId << "] Set chunk priority.";

        QSharedPointer<Internal::DynamicDownloadInstanceData> instanceData = mData->GetInstanceDataForOffer(offerId);
        if (!instanceData.isNull())
        {
            instanceData->SetChunkPriority(chunkIds);
        }
        else
        {
            ORIGIN_LOG_EVENT << "[DDC] Unable to find matching offer Id.";
        }
    }

    QVector<int> DynamicDownloadController::GetChunkPriority(const QString& offerId)
    {
        QMutexLocker lock(&mDataLock);

        ORIGIN_LOG_EVENT << "[DDC][ID=" << offerId << "] Query chunk priority order.";

        QSharedPointer<Internal::DynamicDownloadInstanceData> instanceData = mData->GetInstanceDataForOffer(offerId);
        if (!instanceData.isNull())
        {
            return instanceData->GetChunkPriority();
        }
        else
        {
            ORIGIN_LOG_EVENT << "[DDC] Unable to find matching offer Id. (" << offerId << ")";
            return QVector<int>();
        }
    }

    bool DynamicDownloadController::QueryChunkStatus(const QString& offerId, const int chunkId, Origin::Engine::Content::DynamicDownloadChunkRequirementT& outChunkRequirement, QString& outChunkInternalName, float& outChunkProgress, qint64 &outChunkSize, qint64& outChunkEta, qint64& outTotalEta, Origin::Engine::Content::DynamicDownloadChunkStateT& outChunkState)
    {
        QMutexLocker lock(&mDataLock);

        ORIGIN_LOG_EVENT << "[DDC][ID=" << offerId << "] Query chunk status, id = " << chunkId;

        QSharedPointer<Internal::DynamicDownloadInstanceData> instanceData = mData->GetInstanceDataForOffer(offerId);
        if (!instanceData.isNull())
        {
            return instanceData->QueryChunkStatus(chunkId, outChunkRequirement, outChunkInternalName, outChunkProgress, outChunkSize, outChunkEta, outTotalEta, outChunkState);
        }
        else
        {
            ORIGIN_LOG_EVENT << "[DDC] Unable to find matching offer Id. (" << offerId << ")";
            return false;
        }
    }

    bool DynamicDownloadController::QueryChunkFiles(const QString& offerId, const int chunkId, QSet<QString>& outFileList)
    {
        QMutexLocker lock(&mDataLock);

        ORIGIN_LOG_EVENT << "[DDC][ID=" << offerId << "] Query chunk files, id = " << chunkId;

        QSharedPointer<Internal::DynamicDownloadInstanceData> instanceData = mData->GetInstanceDataForOffer(offerId);
        if (!instanceData.isNull())
        {
            return instanceData->QueryChunkFiles(chunkId, outFileList);
        }
        else
        {
            ORIGIN_LOG_EVENT << "[DDC] Unable to find matching offer Id. (" << offerId << ")";
            return false;
        }
    }

    bool DynamicDownloadController::QueryAreChunksInstalled(const QString& offerId, const QSet<int>& chunkIds)
    {
        QMutexLocker lock(&mDataLock);

        ORIGIN_LOG_EVENT << "[DDC][ID=" << offerId << "] Query chunks are installed? ";

        QSharedPointer<Internal::DynamicDownloadInstanceData> instanceData = mData->GetInstanceDataForOffer(offerId);
        if (!instanceData.isNull())
        {
            return instanceData->QueryAreChunksInstalled(chunkIds);
        }
        else
        {
            ORIGIN_LOG_EVENT << "[DDC] Unable to find matching offer Id. (" << offerId << ")";
            return false;
        }
    }

    bool DynamicDownloadController::QueryIsFileDownloaded(const QString& offerId, const QString& filePath)
    {
        QMutexLocker lock(&mDataLock);

        ORIGIN_LOG_EVENT << "[DDC][ID=" << offerId << "] Is file downloaded? " << filePath;

        QSharedPointer<Internal::DynamicDownloadInstanceData> instanceData = mData->GetInstanceDataForOffer(offerId);
        if (!instanceData.isNull())
        {
            return instanceData->QueryIsFileDownloaded(filePath);
        }
        else
        {
            ORIGIN_LOG_EVENT << "[DDC] Unable to find matching offer Id. (" << offerId << ")";
            return false;
        }
    }

    bool DynamicDownloadController::CreateDynamicChunkFromFileList(const QString& offerId, const QSet<QString>& fileList, int& outChunkId)
    {
        QMutexLocker lock(&mDataLock);

        ORIGIN_LOG_EVENT << "[DDC][ID=" << offerId << "] Create dynamic chunk";

        QSharedPointer<Internal::DynamicDownloadInstanceData> instanceData = mData->GetInstanceDataForOffer(offerId);
        if (!instanceData.isNull())
        {
            return instanceData->CreateDynamicChunkFromFileList(fileList, outChunkId);
        }
        else
        {
            ORIGIN_LOG_EVENT << "[DDC] Unable to find matching offer Id. (" << offerId << ")";
            return false;
        }
    }

    bool DynamicDownloadController::StartDownload(const QString& offerId)
    {
        ORIGIN_LOG_EVENT << "[DDC] Start download, offer = " << offerId;

        if (offerId.isEmpty())
        {
            ORIGIN_LOG_ERROR << "[DDC] StartDownload Offer cannot be NULL";
            return false;
        }

        Origin::Engine::Content::ContentController *contentController = Origin::Engine::Content::ContentController::currentUserContentController();
        if(!contentController)
        {
            ORIGIN_LOG_ERROR << "[DDC] Unable to get content controller.  Origin may not be logged in.";
            return false;
        }
                
        // Search for the entitlement
        Origin::Engine::Content::EntitlementRef entitlement = contentController->entitlementByProductId(offerId);
        if (entitlement.isNull())
        {
            ORIGIN_LOG_ERROR << "[DDC] Unable to find specified entitlement.";
            return false;
        }

        if (!entitlement->localContent())
        {
            ORIGIN_LOG_ERROR << "[DDC] Internal error.";
            return false;
        }

        Origin::Downloader::IContentInstallFlow *installFlow = entitlement->localContent()->installFlow();

        if(installFlow == NULL)
        {
            ORIGIN_LOG_WARNING << "[DDC] Unexpected installFlow == NULL.";
            return false;
        }

        if(installFlow->isBusy())
        {
            // The content should already be downloading/installing.
            return true;
        }

        if (installFlow->canResume())
        {
            installFlow->resume();
            return true;
        }
        else if(installFlow->canBegin())
        {
             if(contentController->downloadById(offerId))
             {
                 return true;
             }
             else
             {
                ORIGIN_LOG_WARNING << "[DDC] Couldn't start the download.";
                return false;
             }
        }
        else
        {
           ORIGIN_LOG_WARNING << "[DDC] Content not in downloadable state.";
           return false;
        }
    }

    void DynamicDownloadController::SetDownloadUtilization(const QString& offerId, float utilization)
    {
        ORIGIN_LOG_EVENT << "[DDC][ID=" << offerId << "] Set Download Utilization = " << utilization;

        if (utilization < 0.0 || utilization > 2.0)
        {
            ORIGIN_LOG_WARNING << "[DDC] Invalid utilization value.  (0-2)";
            return;
        }

        DownloadService::SetGameRequestedDownloadUtilization(utilization);
    }

    void DynamicDownloadController::sendDynamicChunkUpdate(QString offerId, QString chunkName, int chunkId, Origin::Engine::Content::DynamicDownloadChunkRequirementT chunkRequirement, float chunkProgress, qint64 chunkSize, qint64 chunkEta, qint64 totalEta, Origin::Engine::Content::DynamicDownloadChunkStateT chunkState)
    {
        emit DynamicChunkUpdate(offerId, chunkName, chunkId, chunkRequirement, chunkProgress, chunkSize, chunkEta, totalEta, chunkState);
    }

#ifdef _DEBUG
    void DynamicDownloadController::DumpDebug(QString offerId, bool bDumpFileList)
    {
        QMutexLocker lock(&mDataLock);

        if (!mData)
        {
            ORIGIN_LOG_DEBUG << "[DDC] DumpDebug - mData is null.";
            return;
        }

        ORIGIN_LOG_EVENT << "[DDC][ID=" << offerId << "] DumpDebug; bDumpFileList = " << bDumpFileList;

        QSharedPointer<Internal::DynamicDownloadInstanceData> instanceData = mData->GetInstanceDataForOffer(offerId);
        if (!instanceData.isNull())
        {
            instanceData->DumpDebug(bDumpFileList);
        }
        else
        {
            ORIGIN_LOG_EVENT << "[DDC] Unable to find matching offer Id.";
        }
    }
#endif

}//Downloader
}//Origin