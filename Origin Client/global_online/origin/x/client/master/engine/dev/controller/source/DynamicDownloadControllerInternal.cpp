#include "DynamicDownloadControllerInternal.h"

#include "engine/content/ContentController.h"
#include "services/downloader/StringHelpers.h"

namespace Origin
{
namespace Downloader
{
    ///////////////////////////////////////////////////////////////////////////
    // DynamicDownloadControllerData
    //
    //    Maintains the internal state data for the DDC.  Contains the
    //    map of offerID->DD data.
    //
    ///////////////////////////////////////////////////////////////////////////

    bool DynamicDownloadControllerData::HasInstanceData(const QString& offerId)
    {
        if (offerId.isEmpty())
            return false;
        return mDynamicDownloads.contains(offerId.toUpper());
    }

    void DynamicDownloadControllerData::SetInstanceData(const QString& offerId, QSharedPointer<Internal::DynamicDownloadInstanceData> instanceData)
    {
        if (offerId.isEmpty())
            return;

        mDynamicDownloads[offerId.toUpper()] = instanceData;
    }

    QSharedPointer<Internal::DynamicDownloadInstanceData> DynamicDownloadControllerData::GetInstanceDataForOffer(const QString& offerId)
    {
        // We don't know what game they are talking about!
        if (offerId.isEmpty())
            return QSharedPointer<Internal::DynamicDownloadInstanceData>(NULL);

        if (mDynamicDownloads.contains(offerId.toUpper()))
        {
            return mDynamicDownloads[offerId.toUpper()];
        }

        ORIGIN_LOG_EVENT << "[DDC] Unable to find instance data for offerId = " << offerId;

        return QSharedPointer<Internal::DynamicDownloadInstanceData>(NULL);
    }

    void DynamicDownloadControllerData::ClearInstanceData(const QString& offerId)
    {
        if (mDynamicDownloads.contains(offerId.toUpper()))
        {
            mDynamicDownloads.remove(offerId.toUpper());
        }

        // Try to remove the cache file if possible
        QString cachePath;
        if (GetCacheFilePath(offerId, cachePath))
        {
            if (QFile::exists(cachePath) && !QFile::remove(cachePath))
            {
                ORIGIN_LOG_ERROR << "[DDC] ClearInstanceData - Unable to clear cached data for offer: " << offerId << " at: " << cachePath;
            }
        }
    }

    bool DynamicDownloadControllerData::GetCacheFilePath(const QString& offerId, QString& outPath)
    {
        if (offerId.isEmpty())
        {
            ORIGIN_LOG_ERROR << "[DDC] GetCacheFilePath - No offer ID specified.";
            return false;
        }

        QString cacheFileName = QString("%1.ddc").arg(offerId);

        // Clean the filename (offerIDs typically contain ':' characters, etc)
        Downloader::StringHelpers::StripReservedCharactersFromFilename(cacheFileName);

        Origin::Engine::Content::ContentController *contentController = Origin::Engine::Content::ContentController::currentUserContentController();
        if(!contentController)
        {
            ORIGIN_LOG_ERROR << "[DDC] GetCacheFilePath - Unable to get content controller.  Origin may not be logged in.";
            return false;
        }
                
        // Search for the entitlement
        Origin::Engine::Content::EntitlementRef entitlement = contentController->entitlementByProductId(offerId);
        if (entitlement.isNull())
        {
            ORIGIN_LOG_ERROR << "[DDC] GetCacheFilePath - Unable to find specified entitlement.";
            return false;
        }

        if (!entitlement->localContent())
        {
            ORIGIN_LOG_ERROR << "[DDC] GetCacheFilePath - Internal error.";
            return false;
        }

        QString cachePath = entitlement->localContent()->cacheFolderPath();
        QDir cacheDir(cachePath);

        outPath = QString("%1/%2").arg(cacheDir.absolutePath()).arg(cacheFileName);

        return true;
    }

    bool DynamicDownloadControllerData::LoadInstanceData(const QString& offerId)
    {
        ORIGIN_LOG_EVENT << "[DDC] LoadInstanceData - Trying to load cache data for offer: " << offerId;

        QString path;
        if (!GetCacheFilePath(offerId, path))
        {
            ORIGIN_LOG_ERROR << "[DDC] LoadInstanceData unable to get cache data path.";
            return false;
        }

        if (!QFile::exists(path))
        {
            ORIGIN_LOG_WARNING << "[DDC] LoadInstanceData - Cache data does not exist for offer: " << offerId << " at: " << path;
            return false;
        }

        QFile loadFile(path);
        if (!loadFile.open(QIODevice::ReadOnly))
        {
            ORIGIN_LOG_WARNING << "[DDC] LoadInstanceData - Unable to open cache data file for offer: " << offerId << " at: " << path;
            return false;
        }

        // Read the cache data
        QByteArray cacheData = loadFile.readAll();
        QJsonParseError errorData;
        QJsonDocument loadDoc(QJsonDocument::fromJson(cacheData, &errorData));
        
        if (errorData.error != QJsonParseError::NoError)
        {
            ORIGIN_LOG_WARNING << "[DDC] LoadInstanceData - Unable to parse cache data for offer: " << offerId << " at: " << path;
            ORIGIN_LOG_WARNING << "[DDC] JSON Parse Error: " << errorData.errorString() << " At offset: " << errorData.offset;
            return false;
        }

        QJsonObject root = loadDoc.object();

        QSharedPointer<Internal::DynamicDownloadInstanceData> instanceData = QSharedPointer<Internal::DynamicDownloadInstanceData>(new Internal::DynamicDownloadInstanceData(this, offerId));
        instanceData->moveToThread(mDDC->thread());
        instanceData->read(root["ddInstanceData"].toObject());

        SetInstanceData(offerId, instanceData);

        ORIGIN_LOG_EVENT << "[DDC] LoadInstanceData - Loaded DDC cache data for offer: " << offerId << " at: " << path;

        return true;
    }

    bool DynamicDownloadControllerData::SaveInstanceData(Internal::DynamicDownloadInstanceData* instanceData)
    {
        if (!instanceData)
        {
            ORIGIN_LOG_ERROR << "[DDC] SaveInstanceData - instance data was null.";
            return false;
        }

        QString offerId = instanceData->GetProductId();
        ORIGIN_LOG_EVENT << "[DDC] SaveInstanceData - Trying to save cache data for offer: " << offerId;

        QString cachePath;
        if (!GetCacheFilePath(offerId, cachePath))
        {
            ORIGIN_LOG_ERROR << "[DDC] SaveInstanceData - unable to get cache data path.";
            return false;
        }

        QFile saveFile(cachePath);
        if (!saveFile.open(QIODevice::WriteOnly))
        {
            ORIGIN_LOG_ERROR << "[DDC] SaveInstanceData - unable to open cache file for writing: " << cachePath;
            return false;
        }

        QJsonObject root;
        QJsonObject instanceObj;
        instanceData->write(instanceObj);
        root["ddInstanceData"] = instanceObj;

        QJsonDocument saveDoc(root);
        saveFile.write(saveDoc.toJson(QJsonDocument::Indented));

        ORIGIN_LOG_EVENT << "[DDC] SaveInstanceData - Saved DDC cache data for offer: " << offerId << " at: " << cachePath;

        return true;
    }

    void DynamicDownloadControllerData::sendDynamicChunkUpdate(QString offerId, QString chunkName, int chunkId, Origin::Engine::Content::DynamicDownloadChunkRequirementT chunkRequirement, float chunkProgress, qint64 chunkSize, qint64 chunkEta, qint64 totalEta, Origin::Engine::Content::DynamicDownloadChunkStateT chunkState)
    {
        mDDC->sendDynamicChunkUpdate(offerId, chunkName, chunkId, chunkRequirement, chunkProgress, chunkSize, chunkEta, totalEta, chunkState);
    }

    ///////////////////////////////////////////////////////////////////////////
    // Internal::ChunkData
    //
    //    Represents the internal state data for one chunk.
    //
    ///////////////////////////////////////////////////////////////////////////

    Internal::ChunkData::ChunkData() : 
        chunkId(-1), 
        chunkRequirement(Origin::Engine::Content::kDynamicDownloadChunkRequirementUnknown), 
        chunkState(Origin::Engine::Content::kDynamicDownloadChunkState_Unknown), 
        chunkErrorCode(0),
        chunkProgress(0.0), 
        bytesRead(0), 
        bytesTotal(0), 
        etaInit(false), 
        chunkEta(0)
    {
        // We have to do this hacky garbage because TransferStats is not copy-constructable and doesn't play well with all our Qt containers
        if (transferStats.isNull())
            transferStats.reset(new Origin::Downloader::TransferStats("DDC","Chunk"));
    }

    void Internal::ChunkData::read(const QJsonObject& json)
    {
        // Clear internal state
        chunkId = -1;
        chunkRequirement = Origin::Engine::Content::kDynamicDownloadChunkRequirementUnknown;
        chunkState = Origin::Engine::Content::kDynamicDownloadChunkState_Unknown;
        chunkErrorCode = 0;
        chunkProgress = 0.0;
        bytesRead = 0;
        bytesTotal = 0;
        chunkEta = 0;
        etaInit = false;
        chunkFiles.clear();

        chunkId = static_cast<int>(json["chunkId"].toDouble());
        internalName = json["internalName"].toString();
        chunkRequirement = Engine::Content::DynamicDownloadChunkRequirementFromString(json["requirement"].toString());
        chunkState = Engine::Content::DynamicDownloadChunkStateFromString(json["state"].toString());
        chunkErrorCode = static_cast<int>(json["errCode"].toDouble());
        chunkProgress = static_cast<float>(json["progress"].toDouble());
        bytesRead = static_cast<qint64>(json["bytesRead"].toString().toLongLong());
        bytesTotal = static_cast<qint64>(json["bytesTotal"].toString().toLongLong());

        // Parse the file list
        QJsonArray fileList = json["files"].toArray();
        for (QJsonArray::iterator fileIter = fileList.begin(); fileIter != fileList.end(); ++fileIter)
        {
            chunkFiles.insert((*fileIter).toString());
        }
    }

    void Internal::ChunkData::write(QJsonObject& json)
    {
        /*int chunkId;
        Origin::Engine::Content::DynamicDownloadChunkRequirementT chunkRequirement;
        Origin::Engine::Content::DynamicDownloadChunkStateT chunkState;
        int chunkErrorCode;
        QString internalName;
        float chunkProgress;
        qint64 bytesRead;
        qint64 bytesTotal;
        QSet<QString> chunkFiles;*/

        json["chunkId"] = chunkId;
        json["internalName"] = internalName;
        json["requirement"] = Engine::Content::DynamicDownloadChunkRequirementToString(chunkRequirement);
        json["state"] = Engine::Content::DynamicDownloadChunkStateToString(chunkState);
        json["errCode"] = chunkErrorCode;
        json["progress"] = chunkProgress;
        json["bytesRead"] = QString("%1").arg(bytesRead); // No built-in JSON qint64 type
        json["bytesTotal"] = QString("%1").arg(bytesTotal); // No built-in JSON qint64 type

        QJsonArray fileList;
        for (QSet<QString>::iterator fileIter = chunkFiles.begin(); fileIter != chunkFiles.end(); ++fileIter)
        {
            QJsonValue fileEntry;
            fileEntry = *fileIter;
            fileList.append(fileEntry);
        }

        json["files"] = fileList;
    }

    ///////////////////////////////////////////////////////////////////////////
    // Internal::DynamicDownloadInstanceData
    //
    //    Maintains the internal chunk state data for a single DD-enabled 
    //    game.  Listens to signals from the protocol.
    //
    ///////////////////////////////////////////////////////////////////////////

    Internal::DynamicDownloadInstanceData::DynamicDownloadInstanceData(DynamicDownloadControllerData* ddc, QString offerId) :
        mDDC(ddc),
        mProductId(offerId),
        mGlobalBps(0),
        mGlobalEta(0),
        mLastDynamicChunkId(DYNAMIC_CHUNK_START)
    {
        // Nothing to do here at the moment
    }

    QString Internal::DynamicDownloadInstanceData::GetProductId()
    {
        return mProductId;
    }

    void Internal::DynamicDownloadInstanceData::RegisterProtocol(const QString& packagePath, ContentProtocolPackage* packageProtocol, bool clearExisting)
    {
        if (clearExisting)
        {
            // Clear the data we had
            mChunkList.clear();
            mChunkPriorityQueue.clear();
        }

        mPackageRootPath = packagePath;

        // Clear out the ETA/rate info
        mGlobalBps = 0;
        mGlobalEta = 0;

        mProtocol = packageProtocol;
        ORIGIN_VERIFY_CONNECT_EX(mProtocol, SIGNAL(TransferProgress(qint64, qint64, qint64, qint64)), this, SLOT(onProtocol_TransferProgress(qint64, qint64, qint64, qint64)), Qt::QueuedConnection);
        ORIGIN_VERIFY_CONNECT_EX(mProtocol, SIGNAL(DynamicChunkDefined(int, Origin::Engine::Content::DynamicDownloadChunkRequirementT, QString, QSet<QString>)), this, SLOT(onProtocol_DynamicChunkDefined(int, Origin::Engine::Content::DynamicDownloadChunkRequirementT, QString, QSet<QString>)), Qt::QueuedConnection);
        ORIGIN_VERIFY_CONNECT_EX(mProtocol, SIGNAL(DynamicChunkUpdate(int, float, qint64, qint64, Origin::Engine::Content::DynamicDownloadChunkStateT)), this, SLOT(onProtocol_DynamicChunkUpdate(int, float, qint64, qint64, Origin::Engine::Content::DynamicDownloadChunkStateT)), Qt::QueuedConnection);
        ORIGIN_VERIFY_CONNECT_EX(mProtocol, SIGNAL(DynamicChunkError(int, qint32)), this, SLOT(onProtocol_DynamicChunkError(int, qint32)), Qt::QueuedConnection);
        ORIGIN_VERIFY_CONNECT_EX(mProtocol, SIGNAL(DynamicChunkOrderUpdated(QVector<int>)), this, SLOT(onProtocol_DynamicChunkOrderUpdated(QVector<int>)), Qt::QueuedConnection);
        ORIGIN_VERIFY_CONNECT_EX(mProtocol, SIGNAL(destroyed(QObject *)), this, SLOT(onProtocol_destroyed(QObject*)), Qt::QueuedConnection);
    }

    bool Internal::DynamicDownloadInstanceData::IsProgressiveInstallationAvailable()
    {
        return true;
    }

    void Internal::DynamicDownloadInstanceData::SetChunkPriority(QVector<int> chunkIds)
    {
        if (mProtocol.isNull())
            return;

        mProtocol->SetPriorityGroupOrder(chunkIds);
    }

    QVector<int> Internal::DynamicDownloadInstanceData::GetChunkPriority()
    {
        if (mProtocol.isNull())
            return mChunkPriorityQueue;

        // If we don't have a valid cached queue order
        if (mChunkPriorityQueue.count() == 0)
        {
            // Fetch an updated queue order
            QVector<int> priorityGroupOrder = mProtocol->GetPriorityGroupOrder();
            UpdatePriorityGroupOrder(priorityGroupOrder);
        }

        return mChunkPriorityQueue;
    }

    void Internal::DynamicDownloadInstanceData::UpdatePriorityGroupOrder(QVector<int> queue)
    {
        // The queue returned by the protocol may be missing groups, so we need to tack them on the end
        // This can happen because already-installed groups don't necessarily appear in the queue if they
        // were downloaded in a previous session.
        // Go back to front on the key list so that we insert things in correct order, at the beginning
        for (int idx = mChunkList.keys().count() - 1; idx >= 0; idx--)
        {
            int chunkId = mChunkList.keys().at(idx);
            if (!queue.contains(chunkId))
                queue.prepend(chunkId);
        }
        
        // Save it
        mChunkPriorityQueue = queue;
    }

    bool Internal::DynamicDownloadInstanceData::QueryChunkStatus(const int chunkId, Origin::Engine::Content::DynamicDownloadChunkRequirementT& outChunkRequirement, QString& outChunkInternalName, float& outChunkProgress, qint64& outChunkSize, qint64& outChunkEta, qint64& outTotalEta, Origin::Engine::Content::DynamicDownloadChunkStateT& outChunkState)
    {
        if (!mChunkList.contains(chunkId))
            return false;

        const Internal::ChunkData chunkMetadata = mChunkList[chunkId];

        outChunkRequirement = chunkMetadata.chunkRequirement;
        outChunkInternalName = chunkMetadata.internalName;
        outChunkProgress = chunkMetadata.chunkProgress;
		outChunkSize = chunkMetadata.bytesTotal;
        outChunkEta = chunkMetadata.chunkEta;
        outTotalEta = mGlobalEta;
        outChunkState = chunkMetadata.chunkState;

        return true;
    }

    bool Internal::DynamicDownloadInstanceData::QueryChunkFiles(const int chunkId, QSet<QString>& outFileList)
    {
        if (!mChunkList.contains(chunkId))
            return false;

        const Internal::ChunkData chunkMetadata = mChunkList[chunkId];

        QString packageRootPath = QDir::cleanPath(mPackageRootPath);
        
        // Append the package root path to each
        for (QSet<QString>::const_iterator fileIter = chunkMetadata.chunkFiles.cbegin(); fileIter != chunkMetadata.chunkFiles.cend(); ++fileIter)
        {
            QString fullPath = packageRootPath + "/" + *fileIter;

            outFileList.insert(QFileInfo(fullPath).absoluteFilePath());
        }

        return true;
    }

    bool Internal::DynamicDownloadInstanceData::QueryAreChunksInstalled(const QSet<int>& chunkIds)
    {
        for (QSet<int>::const_iterator chunkIter = chunkIds.cbegin(); chunkIter != chunkIds.cend(); ++chunkIter)
        {
            int chunkId = *chunkIter;

            // If we can't find the chunk data, it isn't installed
            if (!mChunkList.contains(chunkId))
                return false;

            const Internal::ChunkData chunkMetadata = mChunkList[chunkId];

            if (chunkMetadata.chunkState != Origin::Engine::Content::kDynamicDownloadChunkState_Installed)
                return false;
        }

        return true;
    }

    bool Internal::DynamicDownloadInstanceData::QueryIsFileDownloaded(const QString& filePath)
    {
        QString packageRootPath = QDir::cleanPath(mPackageRootPath);

        // Clean up and remove the base path (if present) from the query path
        QString queryPath = QFileInfo(filePath).absoluteFilePath();
        queryPath.remove(packageRootPath, Qt::CaseInsensitive);
        if (queryPath.startsWith("/"))
            queryPath.remove(0,1);

        ORIGIN_LOG_EVENT << "[DDC][ID=" << mProductId << "] Query Is File Downloaded? " << filePath << " -> Cleaned Archive Path: " << queryPath;

        for (QMap<int, Internal::ChunkData>::Iterator chunkIter = mChunkList.begin(); chunkIter != mChunkList.end(); ++chunkIter)
        {
            if (chunkIter.value().chunkFiles.contains(queryPath))
                return true;
        }
        
        return false;
    }

    bool Internal::DynamicDownloadInstanceData::CreateDynamicChunkFromFileList(const QSet<QString>& fileList, int& outChunkId)
    {
        // Can't create dynamic chunks if the protocol isn't hooked up!  (Downloader not running)
        if (mProtocol.isNull())
            return false;

        // Empty file list is pointless
        if (fileList.empty())
            return false;

        // Grab (and clean) the package root path
        QString packageRootPath = QDir::cleanPath(mPackageRootPath);

        QStringList packageFileList;
        for (QSet<QString>::const_iterator fileIter = fileList.cbegin(); fileIter != fileList.cend(); ++fileIter)
        {
            // Clean up and remove the base path (if present) from the query path
            QString queryPath = QFileInfo(*fileIter).absoluteFilePath();
            queryPath.remove(packageRootPath, Qt::CaseInsensitive);
            if (queryPath.startsWith("/"))
                queryPath.remove(0,1);

            packageFileList.append(queryPath);
        }

        int newChunkId = ++mLastDynamicChunkId;
        QString dynamicChunkName = QString("dynamicChunk_%1").arg(newChunkId);

        ORIGIN_LOG_EVENT << "[DDC] Create runtime dynamic chunk ID #" << newChunkId << " internal name: " << dynamicChunkName << " with " << packageFileList.count() << " file(s).";

        // Tell the protocol to create a new dynamic chunk
        mProtocol->CreateRuntimeDynamicChunk(newChunkId, dynamicChunkName, packageFileList);

        outChunkId = newChunkId;
        return true;
    }

#ifdef _DEBUG
    void Internal::DynamicDownloadInstanceData::DumpDebug(bool bDumpFileList)
    {
        ORIGIN_LOG_DEBUG << "*************************************";
        ORIGIN_LOG_DEBUG << "**         DDC - CHUNK LIST        **";
        ORIGIN_LOG_DEBUG << "*************************************";

        for (QMap<int, Internal::ChunkData>::Iterator chunkIter = mChunkList.begin(); chunkIter != mChunkList.end(); ++chunkIter)
        {
            Internal::ChunkData& data = *chunkIter;

            QString sRequirement;
            switch (data.chunkRequirement)
            {
            case Origin::Engine::Content::kDynamicDownloadChunkRequirementUnknown:      sRequirement = "Unknown";      break;
            case Origin::Engine::Content::kDynamicDownloadChunkRequirementLowPriority:  sRequirement = "Low";          break;
            case Origin::Engine::Content::kDynamicDownloadChunkRequirementRecommended:  sRequirement = "Recommended";  break;	
            case Origin::Engine::Content::kDynamicDownloadChunkRequirementRequired:     sRequirement = "Required";     break;
            case Origin::Engine::Content::kDynamicDownloadChunkRequirementDemandOnly:   sRequirement = "DemandOnly";   break;
            case Origin::Engine::Content::kDynamicDownloadChunkRequirementDynamic:      sRequirement = "Dynamic";      break;

            }


            QString sState;
            switch (data.chunkState)
            {
            case Origin::Engine::Content::kDynamicDownloadChunkState_Unknown:       sState = "Unknown";      break;
            case Origin::Engine::Content::kDynamicDownloadChunkState_Paused:        sState = "Paused";       break;
            case Origin::Engine::Content::kDynamicDownloadChunkState_Queued:        sState = "Queued";       break;
            case Origin::Engine::Content::kDynamicDownloadChunkState_Downloading:   sState = "Downloading";  break;
            case Origin::Engine::Content::kDynamicDownloadChunkState_Installing:    sState = "Installing";   break;
            case Origin::Engine::Content::kDynamicDownloadChunkState_Installed:     sState = "Installed";    break;
            case Origin::Engine::Content::kDynamicDownloadChunkState_Busy:          sState = "Busy";         break;
            case Origin::Engine::Content::kDynamicDownloadChunkState_Error:         sState = "Error";        break;
            }

            ORIGIN_LOG_DEBUG << QString("id:%1 name:%2 req:%3 state:%4 filesTotal: %5 bytesTotal:%6 bytesRead:%7 progress:%8").arg(data.chunkId).arg(data.internalName).arg(sRequirement).arg(sState).arg(data.chunkFiles.size()).arg(data.bytesTotal).arg(data.bytesRead).arg(data.chunkProgress);
        }

        if (bDumpFileList)
        {
            ORIGIN_LOG_DEBUG << "******** DDC FILE CHUNK ASSIGNMENT ********";
            QSet<QString> totalFiles;

            for (QMap<int, Internal::ChunkData>::Iterator chunkIter = mChunkList.begin(); chunkIter != mChunkList.end(); ++chunkIter)
            {
                Internal::ChunkData& data = *chunkIter;

                ORIGIN_LOG_DEBUG << "*CHUNK id: " << data.chunkId << "* filesTotal:" << data.chunkFiles.size();

                // Append the package root path to each
                for (QSet<QString>::const_iterator fileIter = data.chunkFiles.cbegin(); fileIter != data.chunkFiles.cend(); ++fileIter)
                {
                    QString fileName(*fileIter);
                    fileName = fileName.toLower();

                    ORIGIN_LOG_DEBUG << "id:" << data.chunkId << " file:\"" << *fileIter << "\"";
                    totalFiles.insert(fileName);
                }				

            }


            if (mProtocol && mProtocol->_packageFileDirectory)
            {
                int32_t nUnassigned = 0;
                for (std::list<PackageFileEntry*>::iterator it = mProtocol->_packageFileDirectory->begin();  it != mProtocol->_packageFileDirectory->end(); it++)
                {
                    if (!(*it)->IsIncluded()|| !(*it)->IsFile())
                        continue;

                    QString fileName = (*it)->GetFileName();
                    fileName = fileName.toLower();

                    if (totalFiles.find(fileName) == totalFiles.end())
                    {
                        ORIGIN_LOG_DEBUG << "id:UNASSIGNED file:\"" << (*it)->GetFileName() << "\"";
                        nUnassigned++;
                    }
                }
                ORIGIN_LOG_DEBUG << "Total Files Unassigned:" << nUnassigned;
            }
            else
            {
                ORIGIN_LOG_DEBUG << "mData->mProtocol is NULL or mData->mProtocol->_packageFileDirectory is NULL....... can't enumerate unassigned files.";
            }

            ORIGIN_LOG_DEBUG << "Total Files   Assigned:" << totalFiles.size();
        }
        ORIGIN_LOG_DEBUG << "********* DDC END OF LINE ***********";
    }
#endif

    void Internal::DynamicDownloadInstanceData::read(const QJsonObject& json)
    {
        // Make sure our list is cleared first
        mChunkList.clear();

        mProductId = json["productId"].toString();
        mPackageRootPath = json["packageRootPath"].toString();

        QJsonArray chunkArray = json["chunks"].toArray();
        for (QJsonArray::iterator chunkIter = chunkArray.begin(); chunkIter != chunkArray.end(); ++chunkIter)
        {
            QJsonObject chunkObj = (*chunkIter).toObject();
            Internal::ChunkData chunkData;
            chunkData.read(chunkObj);
            mChunkList[chunkData.chunkId] = chunkData;
        }

        // Update the priority order with an empty list, since we don't persist the queue order
        // This will simply return the chunk list in arbitrary order until we get attached to a protocol
        UpdatePriorityGroupOrder(QVector<int>());
    }

    void Internal::DynamicDownloadInstanceData::write(QJsonObject& json)
    {
        //QString mProductId;
        //QString mPackageRootPath;
        //QVector<int> mChunkPriorityQueue;

        json["productId"] = mProductId;
        json["packageRootPath"] = mPackageRootPath;

        QJsonArray chunkArray;
        for (QMap<int, ChunkData>::iterator chunkIter = mChunkList.begin(); chunkIter != mChunkList.end(); ++chunkIter)
        {
            QJsonObject chunkObj;
            chunkIter->write(chunkObj);
            chunkArray.append(chunkObj);
        }
        json["chunks"] = chunkArray;
    }

    void Internal::DynamicDownloadInstanceData::onProtocol_TransferProgress(qint64 bytesDownloaded, qint64 totalBytes, qint64 bytesPerSecond, qint64 estimatedSecondsRemaining)
    {
        Q_UNUSED(bytesDownloaded); // Not interested in these
        Q_UNUSED(totalBytes);

        // Save the whole-download stats for use in other events (specifically chunk ETA calculation)
        mGlobalBps = bytesPerSecond;
        mGlobalEta = estimatedSecondsRemaining;
    }

    void Internal::DynamicDownloadInstanceData::onProtocol_DynamicChunkDefined(int chunkId, Origin::Engine::Content::DynamicDownloadChunkRequirementT requirement, QString internalName, QSet<QString> fileList)
    {
        ORIGIN_LOG_EVENT << "[DDC][ID=" << mProductId << "] Chunk defined for chunk ID #" << chunkId << ", requirement: " << Engine::Content::DynamicDownloadChunkRequirementToString(requirement) << " with " << fileList.count() << " files.";

        // Update our internal data structrues
        Internal::ChunkData& chunkMetadata = mChunkList[chunkId];
        chunkMetadata.chunkId = chunkId;
        chunkMetadata.chunkRequirement = requirement;
        chunkMetadata.internalName = internalName;
        chunkMetadata.chunkFiles = fileList;
    }

    void Internal::DynamicDownloadInstanceData::onProtocol_DynamicChunkUpdate(int chunkId, float chunkProgress, qint64 chunkRead, qint64 chunkSize, Origin::Engine::Content::DynamicDownloadChunkStateT chunkState)
    {
        ORIGIN_LOG_EVENT << "[DDC][ID=" << mProductId << "] Chunk update for chunk ID #" << chunkId << " progress " << (chunkProgress*100) << "% state - " << Engine::Content::DynamicDownloadChunkStateToString(chunkState);

        // Update our internal data structrues
        Internal::ChunkData& chunkMetadata = mChunkList[chunkId];
        chunkMetadata.chunkId = chunkId;

        qint64 lastRead = chunkMetadata.bytesRead;
        qint64 lastTotal = chunkMetadata.bytesTotal;
        Origin::Engine::Content::DynamicDownloadChunkStateT lastState = chunkMetadata.chunkState;

        chunkMetadata.chunkProgress = chunkProgress;
        chunkMetadata.chunkState = chunkState;
        chunkMetadata.bytesRead = chunkRead;
        chunkMetadata.bytesTotal = chunkSize;

        // Reset our stats if the byte delta goes negative or if the total chunk size changes
        qint64 byteDelta = chunkRead - lastRead;
        if (byteDelta < 0 || (lastTotal != 0 && lastTotal != chunkSize))
        {
            chunkMetadata.etaInit = false;

            // Assume the progress was reset, starting from 0.  Chunk read will now be the new 'progress' delta
            lastRead = 0;
            byteDelta = chunkRead;
        }

        if (chunkState == Origin::Engine::Content::kDynamicDownloadChunkState_Error)
        {
            ORIGIN_LOG_EVENT << "[DDC][ID=" << mProductId << "] Chunk ID #" << chunkId << " is in ERROR state.";

            // Kill the ETA counters
            if (chunkMetadata.etaInit)
            {
                chunkMetadata.transferStats->onFinished();
            }

            chunkMetadata.etaInit = false;
            chunkMetadata.chunkEta = 0;
        }
        else if (chunkState == Origin::Engine::Content::kDynamicDownloadChunkState_Downloading)
        {
            // Restart the stats if we're switching from another state to downloading, or if the byte delta was negative (redownload)
            if (!chunkMetadata.etaInit || lastState != Origin::Engine::Content::kDynamicDownloadChunkState_Downloading)
            {
                // Start the transfer stats
                chunkMetadata.transferStats->setLogId(QString("Chunk %1").arg(chunkId));
                chunkMetadata.transferStats->onStart(chunkSize, lastRead);
                chunkMetadata.etaInit = true;
                chunkMetadata.chunkEta = 0;
            }

            chunkMetadata.transferStats->onDataReceived(byteDelta);

            // If the ETA is valid (not -1), save it
            qint64 eta = chunkMetadata.transferStats->estimatedTimeToCompletion();
            if (eta >= 0)
            {
                chunkMetadata.chunkEta = eta;
                ORIGIN_LOG_EVENT << "[DDC][ID=" << mProductId << "] ETA for chunk ID #" << chunkId << " seconds: " << chunkMetadata.transferStats->estimatedTimeToCompletion();
            }
        }
        else if (chunkState == Origin::Engine::Content::kDynamicDownloadChunkState_Installed || chunkState == Origin::Engine::Content::kDynamicDownloadChunkState_Installing)
        {
            if (lastState == Origin::Engine::Content::kDynamicDownloadChunkState_Downloading)
            {
                if (byteDelta > 0)
                {
                    chunkMetadata.transferStats->onDataReceived(byteDelta);
                }

                chunkMetadata.transferStats->onFinished();
            }
            else if (lastState == Origin::Engine::Content::kDynamicDownloadChunkState_Queued)
            {
                // We went from queued directly to installed.... so, don't worry about the transfer stats anymore
            }
        }

        // Forward to listeners (SDK)
        mDDC->sendDynamicChunkUpdate(mProductId, chunkMetadata.internalName, chunkId, chunkMetadata.chunkRequirement, chunkProgress, chunkSize, chunkMetadata.chunkEta, mGlobalEta, chunkState);
    }

    void Internal::DynamicDownloadInstanceData::onProtocol_DynamicChunkError(int chunkId, qint32 errCode)
    {
        // Update our internal data structrues
        Internal::ChunkData& chunkMetadata = mChunkList[chunkId];
        chunkMetadata.chunkId = chunkId;
        chunkMetadata.chunkErrorCode = errCode;

        ORIGIN_LOG_EVENT << "[DDC][ID=" << mProductId << "] Chunk ID #" << chunkId << " ERROR: " << errCode << " - " << Downloader::ErrorTranslator::ErrorString((Origin::Downloader::ContentDownloadError::type)errCode);
    }

    void Internal::DynamicDownloadInstanceData::onProtocol_DynamicChunkOrderUpdated(QVector<int> chunkIds)
    {
        ORIGIN_LOG_EVENT << "[DDC][ID=" << mProductId << "] Chunk order update received.";

        // Store the updated chunk ordering
        UpdatePriorityGroupOrder(chunkIds);
        
        // TODO: Forward to SDK ?
    }

    void Internal::DynamicDownloadInstanceData::onProtocol_destroyed(QObject* obj)
    {
        ORIGIN_LOG_EVENT << "[DDC][ID=" << mProductId << "] Package protocol destroyed.";

        // The protocol went away, so we should assume that none of our chunks are downloading anymore
        for (QMap<int, Internal::ChunkData>::Iterator chunkIter = mChunkList.begin(); chunkIter != mChunkList.end(); ++chunkIter)
        {
            Internal::ChunkData& chunkMetadata = chunkIter.value();

            // Non-installed chunks are now paused, as long as they aren't in an error state
            if (chunkMetadata.chunkState != Origin::Engine::Content::kDynamicDownloadChunkState_Installed && chunkMetadata.chunkState != Origin::Engine::Content::kDynamicDownloadChunkState_Error)
            {
                chunkMetadata.chunkState = Origin::Engine::Content::kDynamicDownloadChunkState_Paused;
                chunkMetadata.chunkEta = 0;
                chunkMetadata.etaInit = false;

                // Forward to listeners (SDK)
                mDDC->sendDynamicChunkUpdate(mProductId, chunkMetadata.internalName, chunkMetadata.chunkId, chunkMetadata.chunkRequirement, chunkMetadata.chunkProgress, chunkMetadata.bytesTotal, 0, 0, chunkMetadata.chunkState);
            }
        }

        // Save our instance data
        mDDC->SaveInstanceData(this);
    }
}
}