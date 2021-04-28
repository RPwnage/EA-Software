//============================================================================
/// @file OriginStreamInstallBackend.cpp
///
/// Copyright (c) 2012 BioWare ULC
///
/// The source code included in this file is confidential,
/// secret, or proprietary information of BioWare ULC. It
/// may not be used in whole or in part without express
/// written permission from BioWare ULC.
///
/// Origin stream install backend -- coordinates communication with Origin
//============================================================================
#include "Pch.h"

#if defined (FB_ORIGIN_ENABLE)
#include "OriginStreamInstallBackend.h"

#include <Engine/Game/Client/Core/ClientGameContext.h> // For the core message manager

#define ORIGIN_DEFAULT_TIMEOUT	10000

namespace fb
{

	OriginStreamInstallBackend::OriginStreamInstallBackend()
	{
		// Nothing to do for now
	}

	OriginStreamInstallBackend::~OriginStreamInstallBackend()
	{
		// Nothing to do for now
	}

	bool OriginStreamInstallBackend::create()
	{
		g_coreMessageManager->registerMessageListener(MessageCategory_OriginSDKRequest, this);
		return true;
	}

	void OriginStreamInstallBackend::destroy()
	{
		g_coreMessageManager->unregisterMessageListener(MessageCategory_OriginSDKRequest, this);
	}

	void OriginStreamInstallBackend::registerOriginCallback()
	{
		OriginRegisterEventCallback(ORIGIN_EVENT_CHUNK_STATUS, OriginEventCallback, this);
	}

	void OriginStreamInstallBackend::onMessage(const Message& message)
	{
		switch (message.category)
		{
			case MessageCategory_OriginSDKRequest:
			{
				switch (message.type)
				{
					case MessageType_OriginSDKRequestIsProgressiveInstallAvailableRequest:
						{
							FB_INFO("[Origin SDK Request] IsProgressiveInstallationAvailable");
							OriginErrorT errCode;
							if ((errCode = OriginIsProgressiveInstallationAvailable(getOriginSDKItemId(), IsProgressiveInstallationAvailableCallback, this, ORIGIN_DEFAULT_TIMEOUT)) != ORIGIN_SUCCESS)
							{
								FB_WARNING_FORMAT("[Origin SDK Response] IsProgressiveInstallationAvailable Error - %u", (uint)errCode);
								
								// SDK failure, we can't proceed
								OriginStreamInstallOriginSDKErrorMessage* msg = new (FB_GLOBAL_ARENA) OriginStreamInstallOriginSDKErrorMessage();
								msg->setError((uint)errCode);
								g_coreMessageManager->queueMessage(msg);
							}
						}
						break;
					case MessageType_OriginSDKRequestAreChunksInstalledRequest:
						{
							FB_INFO("[Origin SDK Request] AreChunksInstalled");
							const OriginSDKRequestAreChunksInstalledRequestMessage* reqMsg = &message.as<OriginSDKRequestAreChunksInstalledRequestMessage>();
							const ArrayReader<uint> chunkIds = reqMsg->readChunkIds();

							// Populate the chunk IDs into an SDK-compatible data structure
							int32_t *sdkChunkIdList = allocArray<int32_t>(FB_GLOBAL_ARENA, chunkIds.size());
							for (unsigned int idx = 0; idx < chunkIds.size(); ++idx)
							{
								sdkChunkIdList[idx] = static_cast<int32_t>(chunkIds[idx]);
							}

							// Query the SDK
							OriginErrorT errCode;
							if ((errCode = OriginAreChunksInstalled(getOriginSDKItemId(), sdkChunkIdList, static_cast<OriginSizeT>(chunkIds.size()), AreChunksInstalledCallback, this, ORIGIN_DEFAULT_TIMEOUT)) != ORIGIN_SUCCESS)
							{
								FB_WARNING_FORMAT("[Origin SDK Response] AreChunksInstalled Error - %u", (uint)errCode);

								// SDK failure, we can't proceed
								OriginStreamInstallOriginSDKErrorMessage* msg = new (FB_GLOBAL_ARENA) OriginStreamInstallOriginSDKErrorMessage();
								msg->setError((uint)errCode);
								g_coreMessageManager->queueMessage(msg);
							}

							// Clean up
							freeArray<int32_t>(FB_GLOBAL_ARENA, sdkChunkIdList, chunkIds.size());
						}
						break;
					case MessageType_OriginSDKRequestQueryChunkStatusRequest:
						{
							FB_INFO("[Origin SDK Request] QueryChunkStatus");
							OriginErrorT errCode;
							if ((errCode = OriginQueryChunkStatus(getOriginSDKItemId(), QueryChunkStatusCallback, this, ORIGIN_DEFAULT_TIMEOUT, NULL)) != ORIGIN_SUCCESS)
							{
								FB_WARNING_FORMAT("[Origin SDK Response] QueryChunkStatus Error - %u", (uint)errCode);

								// SDK failure, we can't proceed
								OriginStreamInstallOriginSDKErrorMessage* msg = new (FB_GLOBAL_ARENA) OriginStreamInstallOriginSDKErrorMessage();
								msg->setError((uint)errCode);
								g_coreMessageManager->queueMessage(msg);
							}
						}
						break;
				}//switch type
			}
			break;
		}//switch category
	}

	const char * OriginStreamInstallBackend::getOriginSDKItemId()
	{
		// TODO: Maybe return our correct product ID?  The SDK will accept NULL for the base game.
		// To support DLC, we would need to pass an acceptable value here
		return NULL;
	}

	OriginErrorT OriginStreamInstallBackend::IsProgressiveInstallationAvailableCallback(void * pContext, OriginIsProgressiveInstallationAvailableT * availableInfo, OriginSizeT dummy, OriginErrorT error)
	{
		if (error == ORIGIN_SUCCESS)
		{
			// Notify our class
			FB_INFO("[Origin SDK Response] IsProgressiveInstallationAvailable");
			OriginStreamInstallIsProgressiveInstallAvailableResponseMessage* msg = new  (FB_GLOBAL_ARENA) OriginStreamInstallIsProgressiveInstallAvailableResponseMessage();
			msg->setavailable(availableInfo->bAvailable);
			g_coreMessageManager->queueMessage(msg);

			return ORIGIN_SUCCESS;
		}
		else
		{
			FB_WARNING_FORMAT("[Origin SDK Response] IsProgressiveInstallationAvailable Error - %u", (uint)error);

			// SDK failure, we can't proceed
			OriginStreamInstallOriginSDKErrorMessage* msg = new (FB_GLOBAL_ARENA) OriginStreamInstallOriginSDKErrorMessage();
			msg->setError((uint)error);
			g_coreMessageManager->queueMessage(msg);
		}

		return error;
	}

	OriginErrorT OriginStreamInstallBackend::AreChunksInstalledCallback(void * pContext, OriginAreChunksInstalledInfoT * installInfo, OriginSizeT count, OriginErrorT error)
	{
		if (error == ORIGIN_SUCCESS)
		{
			// Notify our class
			FB_INFO("[Origin SDK Response] AreChunksInstalled");
			OriginStreamInstallAreChunksInstalledResponseMessage* msg = new  (FB_GLOBAL_ARENA) OriginStreamInstallAreChunksInstalledResponseMessage();
			msg->setinstalled(installInfo->bInstalled);
			g_coreMessageManager->queueMessage(msg);			
		}
		else
		{
			FB_WARNING_FORMAT("[Origin SDK Response] AreChunksInstalled Error - %u", (uint)error);

			// SDK failure, we can't proceed
			OriginStreamInstallOriginSDKErrorMessage* msg = new (FB_GLOBAL_ARENA) OriginStreamInstallOriginSDKErrorMessage();
			msg->setError((uint)error);
			g_coreMessageManager->queueMessage(msg);
		}

		return error;
	}

	OriginErrorT OriginStreamInstallBackend::QueryChunkStatusCallback(void *pContext, OriginHandleT hHandle, OriginSizeT total, OriginSizeT available, OriginErrorT error)
	{
		if (error == ORIGIN_SUCCESS)    
		{   
			FB_INFO("[Origin SDK Response] QueryChunkStatus");
			OriginChunkStatusT * status = allocArray<OriginChunkStatusT>(FB_GLOBAL_ARENA, static_cast<fb::uint>(total));
			OriginSizeT read;    
			if (OriginReadEnumeration(hHandle, status, sizeof(OriginChunkStatusT)*total, 0, total, &read) == ORIGIN_SUCCESS)
			{
				// First, send back a query chunk status response with a list of all the chunk Ids
				// This is so that we can make "are all chunks done?" decisions with all the necessary info
				OriginStreamInstallQueryChunkStatusResponseMessage* queryMsg = new  (FB_GLOBAL_ARENA) OriginStreamInstallQueryChunkStatusResponseMessage();
				ArrayEditor<uint> allChunkIds = queryMsg->editChunkIds();
				for(unsigned int i=0; i<read; i++)        
				{
					allChunkIds.push_back((uint)status[i].ChunkId);
				}
				g_coreMessageManager->queueMessage(queryMsg);

				// For each chunk, create a chunk status event for it, similar to if we had received an SDK event about it
				for(unsigned int i=0; i<read; i++)        
				{            
					// Queue a chunk status event for our class
					OriginStreamInstallChunkUpdateEventMessage* msg = new  (FB_GLOBAL_ARENA) OriginStreamInstallChunkUpdateEventMessage();
					msg->setChunkId((uint)status[i].ChunkId);
					msg->setChunkState((uint)status[i].State);
					msg->setProgress(status[i].Progress);
					g_coreMessageManager->queueMessage(msg);	
				}	
			}
			else
			{
				FB_WARNING_FORMAT("Unable to read Origin SDK enumeration.");

				// SDK failure, we can't proceed
				OriginStreamInstallOriginSDKErrorMessage* msg = new (FB_GLOBAL_ARENA) OriginStreamInstallOriginSDKErrorMessage();
				msg->setError((uint)error);
				g_coreMessageManager->queueMessage(msg);
			}

			freeArray<OriginChunkStatusT>(FB_GLOBAL_ARENA, status, static_cast<fb::uint>(total));
			OriginDestroyHandle(hHandle);        
		}    
		else    
		{        
			FB_WARNING_FORMAT("[Origin SDK Response] QueryChunkStatus Error - %u", (uint)error);

			// SDK failure, we can't proceed
			OriginStreamInstallOriginSDKErrorMessage* msg = new (FB_GLOBAL_ARENA) OriginStreamInstallOriginSDKErrorMessage();
			msg->setError((uint)error);
			g_coreMessageManager->queueMessage(msg);
		}

		return error;
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//----------------------------------------------------------------------------
	/// @function OriginEventCallback
	//----------------------------------------------------------------------------
	///
	/// Central callback for all Origin events.
	/// Calls OriginStreamInstallBackend-specific handlers.
	///
	/// @param eventId id of the event
	/// @param pContext The OriginPresenceBackend context
	/// @param eventData event-specific data
	/// @param count event-specific count
	///
	/// @return origin error code
	///
	//----------------------------------------------------------------------------
	OriginErrorT OriginStreamInstallBackend::OriginEventCallback(int32_t eventId, void* pContext, void* eventData, OriginSizeT count)
	{
		OriginErrorT retCode(ORIGIN_SUCCESS);

		switch (eventId)
		{
		case ORIGIN_EVENT_CHUNK_STATUS:
			retCode = OriginStreamInstallBackend::ChunkUpdateEventCallback(eventId, pContext, eventData, count);
			break;
		}

		return retCode;
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//----------------------------------------------------------------------------
	/// @function OriginPresenceBackend::ChunkUpdateEventCallback
	//----------------------------------------------------------------------------
	///
	/// Callback to handle a Progressive Installation chunk update from Origin.
	/// See
	/// https://developer.origin.com/documentation/display/OriginSDK/Integrating+with+Chunk+Status+Events
	///
	/// @param eventId ORIGIN_EVENT_CHUNK_STATUS (Not used directly by this function)
	/// @param pContext an OriginPresenceBackend* (Not used directly by this function)
	/// @param eventData an (OriginChunkStatusT *)
	/// @param count unused
	///
	/// @return Origin return code
	///
	//----------------------------------------------------------------------------
	OriginErrorT OriginStreamInstallBackend::ChunkUpdateEventCallback(int32_t, void*, void* eventData, OriginSizeT)
	{
		OriginChunkStatusT *status = (OriginChunkStatusT *)eventData;
		if (status)
		{
			// Queue a chunk status event for the stream install class
			OriginStreamInstallChunkUpdateEventMessage* msg = new  (FB_GLOBAL_ARENA) OriginStreamInstallChunkUpdateEventMessage();
			msg->setChunkId((uint)status->ChunkId);
			msg->setChunkState((uint)status->State);
			msg->setProgress(status->Progress);
			g_coreMessageManager->queueMessage(msg);
		}

		return ORIGIN_SUCCESS;
	}
}//fb

#endif // FB_ORIGIN_ENABLE