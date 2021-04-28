///////////////////////////////////////////////////////////////////////////////
// DynamicDownloadControllerInternal.h
//
// Private implementation data structures for the DDC
//
// Copyright (c) 2014 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#ifndef DYNAMICDOWNLOADCONTROLLERINTERNAL_H
#define DYNAMICDOWNLOADCONTROLLERINTERNAL_H

#include "engine/downloader/DynamicDownloadController.h"
#include "services/downloader/TransferStats.h"
#include "services/debug/DebugService.h"
#include "services/log/LogService.h"
#include "services/downloader/DownloaderErrors.h"

#include "ContentProtocolPackage.h"

#include <QSharedPointer>
#include <QJsonDocument>
#include <QFileInfo>
#ifdef _DEBUG
#include "PackageFile.h"
#endif

// Dynamic chunks start and count up from here
#define DYNAMIC_CHUNK_START 100000

namespace Origin
{
namespace Downloader
{
    // Private implementation data
    // This cannot appear in the main header since QPointer requires full object definitions
    // and we are trying to avoid creating include dependencies
    namespace Internal
    {
        struct ChunkData
        {
            ChunkData();

            void read(const QJsonObject& json);
            void write(QJsonObject& json);

            int chunkId;
            Origin::Engine::Content::DynamicDownloadChunkRequirementT chunkRequirement;
            Origin::Engine::Content::DynamicDownloadChunkStateT chunkState;
            int chunkErrorCode;
            QString internalName;
            float chunkProgress;
            qint64 bytesRead;
            qint64 bytesTotal;
            QSet<QString> chunkFiles;
            bool etaInit;
            qint64 chunkEta;
            QSharedPointer<Origin::Downloader::TransferStats> transferStats; // This thing is not copy-constructable for some stupid reason, so we'll pass around a heap pointer
        };

        class DynamicDownloadInstanceData : public QObject
        {
            Q_OBJECT
        public:
            DynamicDownloadInstanceData(DynamicDownloadControllerData* ddc, QString offerId);

            QString GetProductId();

            void RegisterProtocol(const QString& packagePath, ContentProtocolPackage* packageProtocol, bool clearExisting);

            bool IsProgressiveInstallationAvailable();
            void SetChunkPriority(QVector<int> chunkIds);
            QVector<int> GetChunkPriority();
            void UpdatePriorityGroupOrder(QVector<int> queue);
            bool QueryChunkStatus(const int chunkId, Origin::Engine::Content::DynamicDownloadChunkRequirementT& outChunkRequirement, QString& outChunkInternalName, float& outChunkProgress, qint64& outChunkSize, qint64& outChunkEta, qint64& outTotalEta, Origin::Engine::Content::DynamicDownloadChunkStateT& outChunkState); // TODO: Add ETA stuff
            bool QueryChunkFiles(const int chunkId, QSet<QString>& outFileList);
            bool QueryAreChunksInstalled(const QSet<int>& chunkIds);
            bool QueryIsFileDownloaded(const QString& filePath);
            bool CreateDynamicChunkFromFileList(const QSet<QString>& fileList, int& outChunkId);

#ifdef _DEBUG
		    void DumpDebug(bool bDumpFileList);		// output chunk information
#endif

            void read(const QJsonObject& json);
            void write(QJsonObject& json);

        private slots:
            void onProtocol_TransferProgress(qint64 bytesDownloaded, qint64 totalBytes, qint64 bytesPerSecond, qint64 estimatedSecondsRemaining);
            void onProtocol_DynamicChunkDefined(int chunkId, Origin::Engine::Content::DynamicDownloadChunkRequirementT requirement, QString internalName, QSet<QString> fileList);
            void onProtocol_DynamicChunkUpdate(int chunkId, float chunkProgress, qint64 chunkRead, qint64 chunkSize, Origin::Engine::Content::DynamicDownloadChunkStateT chunkState);
            void onProtocol_DynamicChunkError(int chunkId, qint32 errCode);
            void onProtocol_DynamicChunkOrderUpdated(QVector<int> chunkIds);
            void onProtocol_destroyed(QObject* obj);

        private:
            DynamicDownloadControllerData* mDDC;
            QPointer<ContentProtocolPackage> mProtocol;
            QString mProductId;
            QString mPackageRootPath;
            QVector<int> mChunkPriorityQueue;
            QMap<int, ChunkData> mChunkList;
            qint64 mGlobalBps;
            qint64 mGlobalEta;
            int mLastDynamicChunkId;
        };
    }

    class DynamicDownloadControllerData
    {
        friend class Internal::DynamicDownloadInstanceData;
        
    public:
        DynamicDownloadControllerData(DynamicDownloadController* ddc) : mDDC(ddc) { }

        bool HasInstanceData(const QString& offerId);
        void SetInstanceData(const QString& offerId, QSharedPointer<Internal::DynamicDownloadInstanceData> instanceData);
        QSharedPointer<Internal::DynamicDownloadInstanceData> GetInstanceDataForOffer(const QString& offerId);
        void ClearInstanceData(const QString& offerId);
        bool LoadInstanceData(const QString& offerId);

        void sendDynamicChunkUpdate(QString offerId, QString chunkName, int chunkId, Origin::Engine::Content::DynamicDownloadChunkRequirementT chunkRequirement, float chunkProgress, qint64 chunkSize, qint64 chunkEta, qint64 totalEta, Origin::Engine::Content::DynamicDownloadChunkStateT chunkState);
    private:
        bool GetCacheFilePath(const QString& offerId, QString& outPath);
        bool SaveInstanceData(Internal::DynamicDownloadInstanceData* instanceData);

    private:
        DynamicDownloadController *mDDC;
        QMap<QString, QSharedPointer<Internal::DynamicDownloadInstanceData> > mDynamicDownloads;
    };

}//Downloader
}//Origin

#endif //DYNAMICDOWNLOADCONTROLLERINTERNAL_H