///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2011 Electronic Arts. All rights reserved.
//
///////////////////////////////////////////////////////////////////////////////
//
//  File: ProgressiveInstallation_ServiceArea.cpp
//    SDK Service Area
//    
//    Author: Hans van Veenendaal

#include "lsx.h"
#include "lsxEnumStrings.h"
#include "lsxreader.h"
#include "lsxwriter.h"
#include "NodeDocument.h"
#include "ReaderCommon.h"
#include "EbisuError.h"

#ifdef min
#undef min
#endif

#include "Service/ProgressiveInstallationService/ProgressiveInstallation_ServiceArea.h"

#include "engine/downloader/DynamicDownloadController.h"
#include "services/debug/DebugService.h"
#include "services/settings/SettingsManager.h"
#include <QSet>

// I don't know why I have to include this down here to avoid errors?
#include "LSX/LsxConnection.h"

using namespace Origin::Downloader;


namespace Origin
{
	namespace SDK
	{
		// Singleton functions
		static ProgressiveInstallation_ServiceArea* mpSingleton = NULL;

		ProgressiveInstallation_ServiceArea* ProgressiveInstallation_ServiceArea::create(Lsx::LSX_Handler* handler)
		{
			if (mpSingleton == NULL)
			{
				mpSingleton = new ProgressiveInstallation_ServiceArea(handler);
			}
			return mpSingleton;
		}

		ProgressiveInstallation_ServiceArea* ProgressiveInstallation_ServiceArea::instance()
		{
			return mpSingleton;
		}

		void ProgressiveInstallation_ServiceArea::destroy()
		{
			delete mpSingleton;
			mpSingleton = NULL;
		}

		ProgressiveInstallation_ServiceArea::ProgressiveInstallation_ServiceArea( Lsx::LSX_Handler* handler )
			: BaseService( handler, "PI" )
		{
			registerHandler("IsProgressiveInstallationAvailable", ( BaseService::RequestHandler ) &ProgressiveInstallation_ServiceArea::IsProgressiveInstallationAvailableHandler);
			registerHandler("GetChunkPriority", ( BaseService::RequestHandler ) &ProgressiveInstallation_ServiceArea::GetChunkPriorityHandler);
			registerHandler("SetChunkPriority", ( BaseService::RequestHandler ) &ProgressiveInstallation_ServiceArea::SetChunkPriorityHandler);
			registerHandler("AreChunksInstalled", ( BaseService::RequestHandler ) &ProgressiveInstallation_ServiceArea::AreChunksInstalledHandler);
			registerHandler("QueryChunkStatus", ( BaseService::RequestHandler ) &ProgressiveInstallation_ServiceArea::QueryChunkStatusHandler);
			registerHandler("QueryChunkFiles", ( BaseService::RequestHandler ) &ProgressiveInstallation_ServiceArea::QueryChunkFilesHandler);
			registerHandler("IsFileDownloaded", ( BaseService::RequestHandler ) &ProgressiveInstallation_ServiceArea::IsFileDownloadedHandler);
			registerHandler("CreateChunk", ( BaseService::RequestHandler ) &ProgressiveInstallation_ServiceArea::CreateChunkHandler);
			registerHandler("StartDownload", ( BaseService::RequestHandler ) &ProgressiveInstallation_ServiceArea::StartDownloadHandler);
			registerHandler("SetDownloaderUtilization", ( BaseService::RequestHandler ) &ProgressiveInstallation_ServiceArea::SetDownloadUtilizationHandler);

#ifdef INCLUDE_DEBUG_PI_SIMULATOR_CODE
			bUseSimulator = Origin::Services::readSetting(Origin::Services::SETTING_ENABLE_PROGRESSIVE_INSTALLER_SIMULATOR).toQVariant().toBool();

			if(bUseSimulator)
			{
                prototypeProgressTimer.setInterval(1000);
                prototypeProgressTimer.start();
			}
			else
#endif
			{
            ORIGIN_VERIFY_CONNECT_EX(DynamicDownloadController::GetInstance(), SIGNAL(DynamicChunkUpdate(QString, QString, int, Origin::Engine::Content::DynamicDownloadChunkRequirementT, float, qint64, qint64, qint64, Origin::Engine::Content::DynamicDownloadChunkStateT)), this, SLOT(onDDC_DynamicChunkUpdate(QString, QString, int, Origin::Engine::Content::DynamicDownloadChunkRequirementT, float, qint64, qint64, qint64, Origin::Engine::Content::DynamicDownloadChunkStateT)), Qt::QueuedConnection);
			}
        }											

		ProgressiveInstallation_ServiceArea::~ProgressiveInstallation_ServiceArea()
		{

		}

        lsx::ChunkStateT LSXfromEngine_ChunkState(Origin::Engine::Content::DynamicDownloadChunkStateT state)
        {
            switch (state)
            {
                case Origin::Engine::Content::kDynamicDownloadChunkState_Paused:
                    return lsx::CHUNKSTATE_PAUSED;
                case Origin::Engine::Content::kDynamicDownloadChunkState_Queued:
                    return lsx::CHUNKSTATE_QUEUED;
                case Origin::Engine::Content::kDynamicDownloadChunkState_Downloading:
                    return lsx::CHUNKSTATE_DOWNLOADING;
                case Origin::Engine::Content::kDynamicDownloadChunkState_Installing:
                    return lsx::CHUNKSTATE_INSTALLING;
                case Origin::Engine::Content::kDynamicDownloadChunkState_Installed:
                    return lsx::CHUNKSTATE_INSTALLED;
                case Origin::Engine::Content::kDynamicDownloadChunkState_Busy:
                    return lsx::CHUNKSTATE_BUSY;
                case Origin::Engine::Content::kDynamicDownloadChunkState_Error:
                    return lsx::CHUNKSTATE_ERROR;
                case Origin::Engine::Content::kDynamicDownloadChunkState_Unknown:
                default:
                    return lsx::CHUNKSTATE_UNKNOWN;
            }
        }
                
        lsx::ChunkTypeT LSXfromEngine_ChunkRequirement(Origin::Engine::Content::DynamicDownloadChunkRequirementT requirement)
        {
            switch (requirement)
            {
                case Origin::Engine::Content::kDynamicDownloadChunkRequirementLowPriority:
                    return lsx::CHUNKTYPE_NORMAL;
                case Origin::Engine::Content::kDynamicDownloadChunkRequirementRecommended:
                    return lsx::CHUNKTYPE_RECOMMENDED;
                case Origin::Engine::Content::kDynamicDownloadChunkRequirementRequired: 
                    return lsx::CHUNKTYPE_REQUIRED;
                case Origin::Engine::Content::kDynamicDownloadChunkRequirementDemandOnly:
                    return lsx::CHUNKTYPE_ONDEMAND;
                case Origin::Engine::Content::kDynamicDownloadChunkRequirementUnknown:
                default:
                    return lsx::CHUNKTYPE_UNKNOWN;
            }
        }

        QString GetItemId(Lsx::Request& request, QString requestItemId)
        {
            QString connectionOfferId;
            if (request.connection() && request.connection()->entitlement() && request.connection()->entitlement()->contentConfiguration())
            {
                connectionOfferId = request.connection()->entitlement()->contentConfiguration()->productId();
            }
            return (!requestItemId.isEmpty()) ? requestItemId : connectionOfferId;
        }

#ifdef INCLUDE_DEBUG_PI_SIMULATOR_CODE
		void ProgressiveInstallation_ServiceArea::chunkUpdateEvent(const QString &offerId, const QString &name, int chunkId, float progress, long long size, int type, int state)
		{
			lsx::ChunkStatusT status;

			status.ItemId = offerId;
			status.ChunkId = chunkId;
            status.Name = name;
			status.Progress = progress;
			status.Size = size;
			status.Type = (lsx::ChunkTypeT)type;
			status.State = (lsx::ChunkStateT)state;
            status.ChunkETA = status.State == lsx::CHUNKSTATE_DOWNLOADING ? size * (1.0f - progress) * 1000/(downloadSpeed * downloadUtilization) : -1;
            status.TotalETA = totalETA(status.ItemId);

			sendEvent(status);
		}
#endif


        void ProgressiveInstallation_ServiceArea::onDDC_DynamicChunkUpdate(QString offerId, QString chunkName, int chunkId, Origin::Engine::Content::DynamicDownloadChunkRequirementT chunkRequirement, float chunkProgress,  qint64 chunkSize, qint64 chunkEta, qint64 totalEta, Origin::Engine::Content::DynamicDownloadChunkStateT chunkState)
        {
            lsx::ChunkStatusT status;

			status.ItemId = offerId; 
			status.ChunkId = chunkId;
			status.Name = chunkName;
			status.Progress = chunkProgress;
			status.Size = chunkSize;
			status.Type = LSXfromEngine_ChunkRequirement(chunkRequirement);
			status.State = LSXfromEngine_ChunkState(chunkState);
            status.ChunkETA = static_cast<int32_t>(chunkEta*1000);
            status.TotalETA = static_cast<int32_t>(totalEta*1000);

			sendEvent(status);
        }

		void ProgressiveInstallation_ServiceArea::IsProgressiveInstallationAvailableHandler(Lsx::Request& request, Lsx::Response& response )
		{
			lsx::IsProgressiveInstallationAvailableT r;

			if(lsx::Read(request.document(), r))
			{
				lsx::IsProgressiveInstallationAvailableResponseT resp;

				resp.ItemId = GetItemId(request, r.ItemId);

#ifdef INCLUDE_DEBUG_PI_SIMULATOR_CODE
                if (bUseSimulator)
                {
                    if(!prototypeInprocessChunkList.contains(resp.ItemId) && !prototypeDownloadedChunkList.contains(resp.ItemId))
                    {
                        resetChunks(resp.ItemId);
                        StartDownload(resp.ItemId);
                    }
                    resp.Available = prototypeInprocessChunkList.contains(resp.ItemId) || prototypeDownloadedChunkList.contains(resp.ItemId);
                }
                else
#endif
                {
					resp.Available = DynamicDownloadController::GetInstance()->IsProgressiveInstallationAvailable(GetItemId(request, r.ItemId));
                }

				lsx::Write(response.document(), resp);
			}
			else
			{
				CreateErrorResponse(response, EBISU_ERROR_LSX_INVALID_REQUEST, "Couldn't decode message");
			}

		}

		void ProgressiveInstallation_ServiceArea::GetChunkPriorityHandler(Lsx::Request& request, Lsx::Response& response )
		{
			lsx::GetChunkPriorityT r;

			if(lsx::Read(request.document(), r))
			{
				lsx::GetChunkPriorityResponseT resp;
                
#ifdef INCLUDE_DEBUG_PI_SIMULATOR_CODE
				if(bUseSimulator)
				{
                    ChunkCollectionMap::iterator chunkInfo = prototypeDownloadedChunkList.find(r.ItemId);

                    if(chunkInfo != prototypeDownloadedChunkList.end())
                    {
                        for(int chunk = 0; chunk<chunkInfo.value().size(); chunk++)
                        {
                            resp.ChunkIds.push_back(chunkInfo.value()[chunk].chunkId);
                        }
                    }

                    chunkInfo = prototypeInprocessChunkList.find(r.ItemId.toUtf8().data());

                    if(chunkInfo != prototypeInprocessChunkList.end())
                    {
                        resp.ItemId = r.ItemId;

                        for(int chunk = 0; chunk<chunkInfo.value().size(); chunk++)
                        {
                            resp.ChunkIds.push_back(chunkInfo.value()[chunk].chunkId);
                        }
                    }
				}
				else
#endif
				{
                    // Ask the DDC for the current chunk priority queue
                    QVector<int> queue = DynamicDownloadController::GetInstance()->GetChunkPriority(GetItemId(request, r.ItemId));

                    if (queue.count() > 0)
                    {
                        resp.ItemId = GetItemId(request, r.ItemId);
                        for (QVector<int>::iterator chunkIdIter = queue.begin(); chunkIdIter != queue.end(); ++chunkIdIter)
                        {
                            resp.ChunkIds.push_back(*chunkIdIter);
                        }
                    }
                    else
                    {
                        // Just pass a 0 chunk back for normal content.
                        resp.ItemId = GetItemId(request, r.ItemId);
                        resp.ChunkIds.push_back(0);
                    }
				}
				lsx::Write(response.document(), resp);
			}
            else
            {
				CreateErrorResponse(response, EBISU_ERROR_LSX_INVALID_REQUEST, "Couldn't decode message");
            }
		}
		void ProgressiveInstallation_ServiceArea::SetChunkPriorityHandler(Lsx::Request& request, Lsx::Response& response )
		{
			lsx::SetChunkPriorityT r;

			if(lsx::Read(request.document(), r))
			{
                // We are only interested in chunks that have not been downloaded yet.

#ifdef INCLUDE_DEBUG_PI_SIMULATOR_CODE
                ChunkCollectionMap::iterator chunkInfo = prototypeInprocessChunkList.find(r.ItemId.toUtf8().data());

                if(chunkInfo != prototypeInprocessChunkList.end())
                {
                    ChunkInfoCollection newOrdering;

					ChunkInfo downloadingChunk = chunkInfo.value()[0];

                    for(unsigned int i=0; i<r.ChunkIds.size(); i++)
                    {
                        int chunkId = r.ChunkIds[i];

                        for(ChunkInfoCollection::iterator chunk = chunkInfo.value().begin(); chunk != chunkInfo.value().end(); chunk++)
                        {
                            if(chunkId == chunk->chunkId)
                            {
								// Make sure that the chunk is moved from on demand to normal.
								chunk->type = lsx::CHUNKTYPE_NORMAL;
                                newOrdering.push_back(*chunk);
                                chunkInfo.value().erase(chunk);
                                break;
                            }
                        }
                    }

                    // Copy the remaining chunks
                    for(ChunkInfoCollection::iterator chunk = chunkInfo.value().begin(); chunk != chunkInfo.value().end(); chunk++)
                    {
                        newOrdering.push_back(*chunk);
                    }

					if(downloadingChunk.chunkId != newOrdering[0].chunkId)
					{
						chunkUpdateEvent(downloadingChunk.itemId, downloadingChunk.name, downloadingChunk.chunkId, downloadingChunk.progress, downloadingChunk.size, (int)lsx::CHUNKTYPE_NORMAL, lsx::CHUNKSTATE_QUEUED);
					}

                    chunkInfo.value() = newOrdering;
                }
				else
#endif
				{
                // Build a ordered list
                QVector<int> queueOrder;
                for (size_t ix = 0; ix < r.ChunkIds.size(); ++ix)
                {
                    queueOrder.push_back(r.ChunkIds[ix]);
                }

                DynamicDownloadController::GetInstance()->SetChunkPriority(GetItemId(request, r.ItemId), queueOrder);
				}

                // We may want to return the new ordering here.

				lsx::ErrorSuccessT resp;

                resp.Code = EBISU_SUCCESS;
                resp.Description = "ChunkIds updated.";

				lsx::Write(response.document(), resp);
			}
            else
            {
				CreateErrorResponse(response, EBISU_ERROR_LSX_INVALID_REQUEST, "Couldn't decode message");
            }
		}

		void ProgressiveInstallation_ServiceArea::AreChunksInstalledHandler(Lsx::Request& request, Lsx::Response& response )
		{
			lsx::AreChunksInstalledT r;
			if(lsx::Read(request.document(), r))
			{
				lsx::AreChunksInstalledResponseT resp;
                resp.ChunkIds = r.ChunkIds;
                resp.ItemId = GetItemId(request, r.ItemId);
                resp.Installed = true;

#ifdef INCLUDE_DEBUG_PI_SIMULATOR_CODE
				if(bUseSimulator)
				{
                    /// check whether all chunks are in the downloaded collection
                    ChunkCollectionMap::iterator chunkInfo = prototypeDownloadedChunkList.find(r.ItemId.toUtf8().data());

                    if(chunkInfo != prototypeDownloadedChunkList.end())
                    {
                        for(unsigned int i=0; i<r.ChunkIds.size(); i++)
                        {
                            int chunkId = r.ChunkIds[i];

                            bool found = false;
                            for(ChunkInfoCollection::iterator chunk = chunkInfo.value().begin(); chunk != chunkInfo.value().end(); chunk++)
                            {
                                if(chunkId == chunk->chunkId)
                                {
                                    found = true;
                                    break;
                                }
                            }

                            if(!found)
                            {
                                resp.Installed = false;
                                break;
                            }
                        }
                    }
                    else
                    {
                        ChunkCollectionMap::iterator chunkInfo = prototypeInprocessChunkList.find(r.ItemId.toUtf8().data());

                        if(chunkInfo != prototypeInprocessChunkList.end())
                        {
                            resp.Installed = false;
                        }
                        else
                        {
                            // not a progressive installation session
                        }
                    }
				}
				else
#endif
				{
                    // Build a chunk set to query the DDC
                    QSet<int> chunkList;
                    for (size_t ix = 0; ix < r.ChunkIds.size(); ++ix)
                    {
                        chunkList.insert(r.ChunkIds[ix]);
                    }

                    resp.Installed = DynamicDownloadController::GetInstance()->QueryAreChunksInstalled(GetItemId(request, r.ItemId), chunkList);
				}


				lsx::Write(response.document(), resp);
			}
            else
            {
				CreateErrorResponse(response, EBISU_ERROR_LSX_INVALID_REQUEST, "Couldn't decode message");
            }
		}

		void ProgressiveInstallation_ServiceArea::QueryChunkStatusHandler(Lsx::Request& request, Lsx::Response& response )
		{
			lsx::QueryChunkStatusT r;

			if(lsx::Read(request.document(), r))
			{
				lsx::QueryChunkStatusResponseT resp;

#ifdef INCLUDE_DEBUG_PI_SIMULATOR_CODE
				if(bUseSimulator)
				{
                    QString itemId = GetItemId(request, r.ItemId);

                    /// check whether all chunks are in the downloaded collection
                    ChunkCollectionMap::iterator chunkInfo = prototypeDownloadedChunkList.find(itemId);

                    if(chunkInfo != prototypeDownloadedChunkList.end())
                    {
                        ChunkInfoCollection &chunks = chunkInfo.value();

                        for(int i=0; i<chunks.size(); i++)
                        {
                            lsx::ChunkStatusT status;

                            status.ItemId = itemId;
                            status.ChunkId = chunks[i].chunkId;
						    status.Name = chunks[i].name;
						    status.Type = chunks[i].type;
                            status.State = lsx::CHUNKSTATE_INSTALLED;
                            status.ChunkETA = 0;
                            status.TotalETA = totalETA(r.ItemId);
                            status.Progress = 1.0f;
						    status.Size = chunks[i].size;

                            resp.ChunkStatus.push_back(status);
                        }
                    }

                    chunkInfo = prototypeInprocessChunkList.find(itemId);

                    if(chunkInfo != prototypeInprocessChunkList.end())
                    {
                        ChunkInfoCollection &chunks = chunkInfo.value();

                        for(int i=0; i<chunks.size(); i++)
                        {
                            lsx::ChunkStatusT status;

                            bool isItemDownloading = prototypeInprocessChunkList[prototypeDownloadOrder[0]][0].itemId == itemId;

                            status.ItemId = itemId;
                            status.ChunkId = chunks[i].chunkId;
						    status.Name = QString("Name %1").arg(chunks[i].chunkId);
						    status.Type = chunks[i].type;
                            if(status.Type == lsx::CHUNKTYPE_ONDEMAND)
                            {
                                status.State = lsx::CHUNKSTATE_PAUSED;
                            }
                            else
                            {
                                status.State = (i == 0 && isItemDownloading) ? lsx::CHUNKSTATE_DOWNLOADING : lsx::CHUNKSTATE_QUEUED;
                            }
                            status.ChunkETA = chunkETA(r.ItemId, status.ChunkId);
                            status.TotalETA = totalETA(r.ItemId);
                            status.Progress = chunks[i].progress;
						    status.Size = chunks[i].size;

                            resp.ChunkStatus.push_back(status);
                        }
                    }
                    else
                    {
                        // normal download.
                    }
				}
				else
#endif
				{
                    DynamicDownloadController *ddc = DynamicDownloadController::GetInstance();

                    QString itemId = GetItemId(request, r.ItemId);
                    QVector<int> chunkList = ddc->GetChunkPriority(itemId);

                    for(int i=0; i<chunkList.size(); i++)
                    {
                        lsx::ChunkStatusT status;

                        Origin::Engine::Content::DynamicDownloadChunkRequirementT chunkRequirement = Origin::Engine::Content::kDynamicDownloadChunkRequirementUnknown;
                        QString chunkInternalName;
                        float chunkProgress = 0.0;
					    qint64 chunkSize = 0;
                        qint64 chunkEta = 0;
                        qint64 totalEta = 0;
                        Origin::Engine::Content::DynamicDownloadChunkStateT chunkState = Origin::Engine::Content::kDynamicDownloadChunkState_Unknown;

                        if (ddc->QueryChunkStatus(itemId, chunkList[i], chunkRequirement, chunkInternalName, chunkProgress, chunkSize, chunkEta, totalEta, chunkState))
                        {
                            status.ItemId = itemId;
                            status.ChunkId = chunkList[i];
						    status.Name = chunkInternalName;
						    status.Type = LSXfromEngine_ChunkRequirement(chunkRequirement);
                            status.State = LSXfromEngine_ChunkState(chunkState);
                            status.ChunkETA = static_cast<int32_t>(chunkEta*1000);
                            status.TotalETA = static_cast<int32_t>(totalEta*1000);
                            status.Progress = chunkProgress;
						    status.Size = chunkSize;

                            resp.ChunkStatus.push_back(status);
                        }
                    }
				}

				lsx::Write(response.document(), resp);
			}
            else
            {
				CreateErrorResponse(response, EBISU_ERROR_LSX_INVALID_REQUEST, "Couldn't decode message");
            }
		}


#ifdef INCLUDE_DEBUG_PI_SIMULATOR_CODE
        void AddFiles(ProgressiveInstallation_ServiceArea::ChunkCollectionMap::iterator i, int chunkId, lsx::QueryChunkFilesResponseT &resp)
        {
            for(int j = 0; j < i->size(); j++)
            {
                if((*i)[j].chunkId == chunkId)
                {
                    for(int f = 0; f < (*i)[j].files.size(); f++)
                    {
                        resp.Files.push_back((*i)[j].files[f]);
                    }
                }
            }
        }
#endif 

        void ProgressiveInstallation_ServiceArea::QueryChunkFilesHandler(Lsx::Request& request, Lsx::Response& response)
		{
			lsx::QueryChunkFilesT r;

			if(lsx::Read(request.document(), r))
			{
				lsx::QueryChunkFilesResponseT resp;

#ifdef INCLUDE_DEBUG_PI_SIMULATOR_CODE
				if(bUseSimulator)
				{
                    QString itemId = GetItemId(request, r.ItemId);
                    ChunkCollectionMap::iterator i = prototypeDownloadedChunkList.find(itemId);

                    if(i != prototypeDownloadedChunkList.end())
                    {
                        AddFiles(i, r.ChunkId, resp);
                    }
                    
                    i = prototypeInprocessChunkList.find(itemId);

                    if(i != prototypeInprocessChunkList.end())
                    {
                        AddFiles(i, r.ChunkId, resp);
                    }
				}
				else
#endif
				{
                    QSet<QString> fileList;
                    if (DynamicDownloadController::GetInstance()->QueryChunkFiles(GetItemId(request, r.ItemId), r.ChunkId, fileList))
                    {
                        for (QSet<QString>::iterator fileIter = fileList.begin(); fileIter != fileList.end(); ++fileIter)
                        {
                            resp.Files.push_back(*fileIter);
                        }
                    }
				}

				lsx::Write(response.document(), resp);
			}
            else
            {
				CreateErrorResponse(response, EBISU_ERROR_LSX_INVALID_REQUEST, "Couldn't decode message");
            }
		}

		void ProgressiveInstallation_ServiceArea::IsFileDownloadedHandler(Lsx::Request& request, Lsx::Response& response )
		{
			lsx::IsFileDownloadedT r;

			if(lsx::Read(request.document(), r))
			{
				lsx::IsFileDownloadedResponseT resp;

#ifdef INCLUDE_DEBUG_PI_SIMULATOR_CODE
				if(bUseSimulator)
				{
                    resp.Filepath = r.Filepath;
                    resp.ItemId = GetItemId(request, r.ItemId);
                    resp.Downloaded = false;

                    ChunkCollectionMap::iterator i = prototypeDownloadedChunkList.find(resp.ItemId);

                    if(i != prototypeDownloadedChunkList.end())
                    {
                        for(ChunkInfoCollection::iterator j = (*i).begin(); j != (*i).end(); j++)
                        {
                            for(int f = 0; f < j->files.size(); f++)
                            {
                                if(j->files[f] == r.Filepath)
                                {
                                    resp.Downloaded = true;
                                    goto found;
                                }
                            }
                        }
                    }
                found:;
				}
				else
#endif
				{
                    resp.Filepath = r.Filepath;
                    resp.ItemId = GetItemId(request, r.ItemId);
                    resp.Downloaded = DynamicDownloadController::GetInstance()->QueryIsFileDownloaded(GetItemId(request, r.ItemId), r.Filepath);
				}

				lsx::Write(response.document(), resp);
			}
            else
            {
				CreateErrorResponse(response, EBISU_ERROR_LSX_INVALID_REQUEST, "Couldn't decode message");
            }
		}

		void ProgressiveInstallation_ServiceArea::CreateChunkHandler(Lsx::Request& request, Lsx::Response& response )
		{
			lsx::CreateChunkT r;

			if(lsx::Read(request.document(), r))
			{
				lsx::CreateChunkResponseT res;

#ifdef INCLUDE_DEBUG_PI_SIMULATOR_CODE
				if(bUseSimulator)
				{
                    int id = newChunkId(r.ItemId);
                    ChunkInfo info(r.ItemId, QString("Chunk_%1").arg(id), id, 0.0, 100000, lsx::CHUNKTYPE_NORMAL);

                    for(int i = 0; i<r.Files.size(); i++)
                    {
                        info.files.push_back(r.Files[i]);
                    }

				    if(prototypeDownloadOrder.size()>0)
				    {
					    ChunkCollectionMap::iterator chunkInfo = prototypeInprocessChunkList.find(prototypeDownloadOrder[0]);

					    // Check if the offer indeed exist.
					    if(chunkInfo != prototypeInprocessChunkList.end())
					    {
						    // Get all the chunks.
						    ChunkInfoCollection &chunks = chunkInfo.value();

						    // Check if the first element is the current chunk
						    if(chunks.size()>0)
						    {
							    // Queue the currently downloading chunk.
							    ChunkInfo &info = chunks[0];
							    chunkUpdateEvent(prototypeDownloadOrder[0], info.name, info.chunkId, info.progress, info.size, info.type, (int)lsx::CHUNKSTATE_QUEUED);
						    }
					    }
				    }

				    prototypeInprocessChunkList[r.ItemId].push_front(info);

                    res.ChunkId = info.chunkId;
				}
				else
#endif
				{
                // Build a set of files for the DDC
                QSet<QString> fileList;
                for (size_t idx = 0; idx < r.Files.size(); ++idx)
                {
                    fileList.insert(r.Files[idx]);
                }

                int chunkId = 0;

                if (!DynamicDownloadController::GetInstance()->CreateDynamicChunkFromFileList(GetItemId(request, r.ItemId), fileList, chunkId))
                {
                    chunkId = -1;
                    // TODO: Maybe do something else?
                }

                res.ChunkId = chunkId;
				}

				lsx::Write(response.document(), res);
			}
			else
			{
				CreateErrorResponse(response, EBISU_ERROR_LSX_INVALID_REQUEST, "Couldn't decode message");
			}
		}

		void ProgressiveInstallation_ServiceArea::StartDownloadHandler(Lsx::Request& request, Lsx::Response& response )
		{
			lsx::StartDownloadT r;
            bool success = false;

			if(lsx::Read(request.document(), r))
			{
#ifdef INCLUDE_DEBUG_PI_SIMULATOR_CODE
				if(bUseSimulator)
				{
                    QString itemId = GetItemId(request, r.ItemId); 

                    // Make sure there is something to download
                    if(!prototypeInprocessChunkList.contains(itemId) && !prototypeDownloadedChunkList.contains(itemId))
                    {
                        resetChunks(itemId);
                    }
                    
                    success = StartDownload(itemId);
				}
				else
#endif
				{
                    success = DynamicDownloadController::GetInstance()->StartDownload(r.ItemId);
				}

				lsx::ErrorSuccessT res;

				res.Code = success ? EBISU_SUCCESS : EBISU_ERROR_INVALID_ARGUMENT;
				res.Description = "Requested a download of a DLC offer.";

				lsx::Write(response.document(), res);
			}
			else
			{
				CreateErrorResponse(response, EBISU_ERROR_LSX_INVALID_REQUEST, "Couldn't decode message");
			}
		}

		void ProgressiveInstallation_ServiceArea::SetDownloadUtilizationHandler(Lsx::Request& request, Lsx::Response& response )
		{
			lsx::SetDownloaderUtilizationT r;

			if(lsx::Read(request.document(), r))
			{
				if(r.Utilization>=0.0f && r.Utilization<=2.0f)
				{
#ifdef INCLUDE_DEBUG_PI_SIMULATOR_CODE
					if(bUseSimulator)
					{
					    downloadUtilization = r.Utilization;
					}
					else
#endif
					{
						DynamicDownloadController::GetInstance()->SetDownloadUtilization(GetItemId(request, ""), r.Utilization);
					}

					CreateErrorResponse(response, EBISU_SUCCESS, "");
				}
				else
				{
					CreateErrorResponse(response, EBISU_ERROR_INVALID_ARGUMENT, "Utilization should be between [0,2]");
				}
			}
			else
			{
				CreateErrorResponse(response, EBISU_ERROR_LSX_INVALID_REQUEST, "Couldn't decode message");
			}
		}

		void ProgressiveInstallation_ServiceArea::CreateErrorResponse(Lsx::Response &response, int code, const char *description )
		{
			lsx::ErrorSuccessT ret;

			ret.Code = code;
			ret.Description = description;

			lsx::Write(response.document(), ret);
		}


#ifdef INCLUDE_DEBUG_PI_SIMULATOR_CODE

        void AddFiles(const lsx::ChunkT &chunk, ProgressiveInstallation_ServiceArea::ChunkInfo &chunkInfo)
        {
            quint64 size = 0;

            for(int f = 0; f < chunk.Files.size(); f++)
            {
                size += chunk.Files[f].size;
                chunkInfo.files.append(chunk.Files[f].name);
            }	

            chunkInfo.size = size;
        }

        void ProgressiveInstallation_ServiceArea::resetChunks(const QString &baseGameOfferId)
        {
            QString filename = Origin::Services::readSetting(Origin::Services::SETTING_PROGRESSIVE_INSTALLER_SIMULATOR_CONFIG).toString();
            QFile PISimulator(filename);
            
            if(!PISimulator.open(QIODevice::ReadOnly))
                return;

            INodeDocument * doc = CreateXmlDocument();
            doc->Parse(PISimulator.readAll());

            lsx::PISimulatorT chunkInformation;

            prototypeDownloadedChunkList.clear();
            prototypeInprocessChunkList.clear();
            prototypeDownloadOrder.clear();

            if(lsx::Read(doc, chunkInformation))
            {
                for(int p = 0; p < chunkInformation.PIOffers.size(); p++)
                {
                    lsx::PIOfferT &offer = chunkInformation.PIOffers[p];

                    for(int c = 0; c < offer.Chunks.Chunk.size(); c++)
                    {
                        const lsx::ChunkT &chunk = offer.Chunks.Chunk[c];

                        if(chunk.type == lsx::CHUNKTYPE_REQUIRED)
                        {
                            ChunkInfo chunkInfo(offer.offerId, chunk.name, chunk.id, 1.0, 0, chunk.type);
                            AddFiles(chunk, chunkInfo);
                            prototypeDownloadedChunkList[offer.offerId].push_back(chunkInfo);
                        }
                        else
                        {
                            ChunkInfo chunkInfo(offer.offerId, chunk.name, chunk.id, 0.0, 0, chunk.type);
                            AddFiles(chunk, chunkInfo);
                            prototypeInprocessChunkList[offer.offerId].push_back(chunkInfo);
                        }
                    }
                    prototypeDownloadOrder.push_back(offer.offerId);
                }
            }

            downloadSpeed = chunkInformation.downloadSpeed;
			downloadUtilization = chunkInformation.downloadUtilization;

			ORIGIN_VERIFY_DISCONNECT(&prototypeProgressTimer, SIGNAL(timeout()), this, SLOT(updateChunks()));
        }

        void ProgressiveInstallation_ServiceArea::updateChunks()
        {
            if(prototypeDownloadOrder.size() > 0)
            {
                // Get the chunks to download for the first offer.
                ChunkCollectionMap::iterator chunkInfo = prototypeInprocessChunkList.find(prototypeDownloadOrder[0]);

                // Check if the offer indeed exist.
                if(chunkInfo != prototypeInprocessChunkList.end())
                {
                    // Get all the chunks.
                    ChunkInfoCollection &chunks = chunkInfo.value();

                    // Check if the first element is the current chunk
                    if(chunks.size()>0)
                    {
                        // Get the first element.
                        ChunkInfo &chunk = chunks[0];

						if(chunk.type != lsx::CHUNKTYPE_ONDEMAND)
						{
							// Update its progress.
							long long downloadedSize = chunk.size * chunk.progress;

							chunk.progress = (double)(downloadedSize + (downloadSpeed * downloadUtilization))/(double)chunk.size;

							if(chunk.progress>1.0)
							{
								chunk.progress = 1.0;

								// send an event that this chunk is downloaded.

								chunkUpdateEvent(chunk.itemId, chunk.name, chunk.chunkId, chunk.progress, chunk.size, (int)lsx::CHUNKTYPE_NORMAL, (int)lsx::CHUNKSTATE_INSTALLED);

								// Move the chunk to the downloaded list.
								prototypeDownloadedChunkList[chunk.itemId].push_back(chunk);
								chunks.erase(chunks.begin());
							}
							else
							{
								chunkUpdateEvent(chunk.itemId, chunk.name, chunk.chunkId, chunk.progress, chunk.size, (int)lsx::CHUNKTYPE_NORMAL, (int)lsx::CHUNKSTATE_DOWNLOADING);
							}
						}
						else
						{
                            // Check whether there are other !ONDEMAND chunks that we could be processing?
                            if(chunks.size() > 1)
                            {
                                for(ChunkInfoCollection::iterator ci = chunks.begin(); ci != chunks.end(); ci++)
                                {
                                    if(ci->type != lsx::CHUNKTYPE_ONDEMAND)
                                    {
                                        // Move to first
                                        ChunkInfo temp = *ci;
                                        chunks.erase(ci);
                                        chunks.push_front(temp);
                                        return;
                                    }
                                }
                            }

                            /// push the front of the download list to the back.
                            QString itemId = prototypeDownloadOrder.front();
                            prototypeDownloadOrder.pop_front();
                            prototypeDownloadOrder.push_back(itemId);
						}
                    }
                    else
                    {
                        // Nothing more to download for this offer.
                        prototypeDownloadOrder.erase(prototypeDownloadOrder.begin());
                    }
                }
            }
            else
            {
                // Downloaded all chunks, lets reset them to the beginning.
                resetChunks("");
            }
        }


		int ProgressiveInstallation_ServiceArea::chunkETA( QString ItemId, int ChunkId )
		{
			return 0;
		}

        int ProgressiveInstallation_ServiceArea::totalETA( QString ItemId )
        {
            long long totalData = 0;
            
            ChunkCollectionMap::iterator chunkInfo = prototypeInprocessChunkList.find(ItemId.toUtf8().data());

            if(chunkInfo != prototypeInprocessChunkList.end())
            {
                ChunkInfoCollection &chunks = chunkInfo.value();

                for(int i=0; i<chunks.size(); i++)
                {
                    totalData += chunks[i].size * (1.0f - chunks[i].progress);
                }
            }

            return totalData * 1000/(downloadSpeed * downloadUtilization);
        }

		int ProgressiveInstallation_ServiceArea::newChunkId( QString itemId )
		{
			int id=1;

			ChunkInfoCollection &downloadCollection = prototypeDownloadedChunkList[itemId];
			ChunkInfoCollection &inprogressCollection = prototypeInprocessChunkList[itemId];

			for(ChunkInfoCollection::iterator i=downloadCollection.begin(); i!=downloadCollection.end(); i++)
			{
				if(id <= i->chunkId)
				{
					id = i->chunkId + 1;
				}
			}
			for(ChunkInfoCollection::iterator i=inprogressCollection.begin(); i!=inprogressCollection.end(); i++)
			{
				if(id <= i->chunkId)
				{
					id = i->chunkId + 1;
				}
			}

			return id;
		}

        bool ProgressiveInstallation_ServiceArea::StartDownload( const QString &itemId )
        {
            // If the download order contains the item we want to move forward in the download and it is not already in the first spot.
            if(prototypeDownloadOrder.contains(itemId))
            {
                if(prototypeDownloadOrder[0] != itemId)
                {
                    ChunkCollectionMap::iterator chunkInfo = prototypeInprocessChunkList.find(prototypeDownloadOrder[0]);

                    // Check if the offer indeed exist.
                    if(chunkInfo != prototypeInprocessChunkList.end())
                    {
                        // Get all the chunks.
                        ChunkInfoCollection &chunks = chunkInfo.value();

                        // Check if the first element is the current chunk
                        if(chunks.size() > 0)
                        {
                            // Queue the currently downloading chunk.
                            ChunkInfo &info = chunks[0];
                            chunkUpdateEvent(prototypeDownloadOrder[0], info.name, info.chunkId, info.progress, info.size, info.type, (int)lsx::CHUNKSTATE_QUEUED);
                        }
                    }
                    prototypeDownloadOrder.removeAll(itemId);
                    prototypeDownloadOrder.push_front(itemId);
                }

                connect(&prototypeProgressTimer, SIGNAL(timeout()), this, SLOT(updateChunks()), Qt::UniqueConnection);

                return true;
            }
            return false;
        }

#endif


    }
}