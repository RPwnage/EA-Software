///////////////////////////////////////////////////////////////////////////////
// DynamicDownloadController.h
//
// Copyright (c) 2014 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#ifndef DYNAMICDOWNLOADCONTROLLER_H
#define DYNAMICDOWNLOADCONTROLLER_H

#include <QObject>
#include <QString>
#include <QMutex>

#include "engine/content/DynamicContentTypes.h"

namespace Origin
{
namespace Downloader
{
    class ContentProtocolPackage; // Forward declare

    class DynamicDownloadControllerData; // Hide our member variables so that we don't need to create an include dependency on the downloader internals
    class DynamicDownloadController : public QObject
    {
        Q_OBJECT

        DynamicDownloadController();
        ~DynamicDownloadController();

        friend class DynamicDownloadControllerData;
    public:
        static DynamicDownloadController* GetInstance();

        bool RegisterProtocol(const QString& offerId, const QString& packagePath, ContentProtocolPackage* packageProtocol, bool clearExisting);

        Q_INVOKABLE void ClearDynamicDownloadData(QString offerId);

        bool IsProgressiveInstallationAvailable(const QString& offerId);

        void SetChunkPriority(const QString& offerId, QVector<int> chunkIds);
        QVector<int> GetChunkPriority(const QString& offerId);
        bool QueryChunkStatus(const QString& offerId, const int chunkId, Origin::Engine::Content::DynamicDownloadChunkRequirementT& outChunkRequirement, QString& outChunkInternalName, float& outChunkProgress, qint64& outChunkSize, qint64& outChunkEta, qint64& outTotalEta, Origin::Engine::Content::DynamicDownloadChunkStateT& outChunkState); // TODO: Add ETA stuff
        bool QueryChunkFiles(const QString& offerId, const int chunkId, QSet<QString>& outFileList);
        bool QueryAreChunksInstalled(const QString& offerId, const QSet<int>& chunkIds);
        bool QueryIsFileDownloaded(const QString& offerId, const QString& filePath);

        bool CreateDynamicChunkFromFileList(const QString& offerId, const QSet<QString>& fileList, int& outChunkId);

        // Global/Non-DD-specific APIs
        bool StartDownload(const QString& offerId);
        void SetDownloadUtilization(const QString& offerId, float utilization);

#ifdef _DEBUG
		Q_INVOKABLE void DumpDebug(QString offerId, bool bDumpFileList = false);		// output chunk information
#endif
    private:
        void UpdatePriorityGroupOrder(QVector<int> queue);

        void sendDynamicChunkUpdate(QString offerId, QString chunkName, int chunkId, Origin::Engine::Content::DynamicDownloadChunkRequirementT chunkRequirement, float chunkProgress, qint64 chunkSize, qint64 chunkEta, qint64 totalEta, Origin::Engine::Content::DynamicDownloadChunkStateT chunkState);
    signals:
        void DynamicChunkUpdate(QString offerId, QString chunkName, int chunkId, Origin::Engine::Content::DynamicDownloadChunkRequirementT chunkRequirement, float chunkProgress, qint64 chunkSize, qint64 chunkEta, qint64 totalEta, Origin::Engine::Content::DynamicDownloadChunkStateT chunkState);

    private:
        DynamicDownloadControllerData* mData;
        QMutex mDataLock;
    };
}//Downloader
}//Origin

#endif