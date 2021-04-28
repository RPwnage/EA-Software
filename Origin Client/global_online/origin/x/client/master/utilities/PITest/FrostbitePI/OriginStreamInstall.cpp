// (c) Electronic Arts.  All Rights Reserved.
#include "Pch.h"
//#include <BF/Client/Install/StreamInstall.h>
#include <Engine/Core/StreamInstall/StreamInstall.h>
#if FB_USING(FB_STREAMINSTALL_ENABLED)

//#include <collection.h>
#include <eastl/vector.h>
#include <eastl/string.h>
#include <eastl/set.h>

#include <Engine/Core/Buffers/FileInBuffer.h>
#include <Engine/Core/Diagnostics/ExecutionContext.h>
#include <Engine/Core/File/SuperBundleBackend.h>
#include <Engine/Core/File/VirtualFileSystem.h>
#include <Engine/Core/Message/CoreMessages.h>
#include <Engine/Core/Message/Message.h>
#include <Engine/Core/Message/MessageManager.h>
#include <Engine/Core/Memory/MallocScope.h>
#include <Engine/Core/Resource/IResourceManager.h>
#include <Engine/Core/String/StringBuilder.h>
#include <Engine/Core/String/StringUtil.h>
#include <Engine/Core/StreamInstall/Common/StreamInstallCommon.h>
#include <Engine/Core/Types.h>

// Origin-specific include
#include <External/ea/OriginSDK/include/OriginSDK/OriginSDK.h>

#ifndef FB_RETAIL
#include <Engine/Core/Time/PerfCounter.h>
#endif

//==================================================================================================================
// These are the messages that Origin Streaming Installation depends on.
// These should be added in CoreMessages.ddf
//==================================================================================================================
//message StreamInstall.InstallFailed { uint ErrCode; }
//
//message OriginSDKRequest.IsProgressiveInstallAvailableRequest;
//message OriginSDKRequest.AreChunksInstalledRequest { uint ChunkIds[]; }
//message OriginSDKRequest.QueryChunkStatusRequest;
//
//message OriginStreamInstall.OriginAvailable;
//message OriginStreamInstall.OriginUnavailable;
//message OriginStreamInstall.OriginSDKError { uint Error; };
//message OriginStreamInstall.IsProgressiveInstallAvailableResponse { bool available; }
//message OriginStreamInstall.AreChunksInstalledResponse { bool installed; }
//message OriginStreamInstall.QueryChunkStatusResponse { uint ChunkIds[]; }
//message OriginStreamInstall.ChunkUpdateEvent { uint ChunkId; u64 ChunkSize; float Progress; uint ChunkState; }
//==================================================================================================================

namespace fb 
{
	#if defined(FB_STREAMINSTALL_DEBUGGING)
		#define FB_INSTALLDEBUG_MOUNT(name) {FB_INFO_FORMAT("Mounting %s at %d", name, PerfCounter::getCount());}
		#define FB_INSTALLDEBUG_QUEUE(name) {FB_INFO_FORMAT("Queue %s at %d", name, PerfCounter::getCount());}
		#define FB_INSTALLDEBUG_UNMOUNT(name) {FB_INFO_FORMAT("Unmounting %s at %d", name, PerfCounter::getCount());}
		#define FB_INSTALLDEBUG_UNQUEUE(name) {FB_INFO_FORMAT("Removed %s from queue at %d", name, PerfCounter::getCount());}
	#else
		#define FB_INSTALLDEBUG_MOUNT(name) {}
		#define FB_INSTALLDEBUG_QUEUE(name) {}
		#define FB_INSTALLDEBUG_UNMOUNT(name) {}
		#define FB_INSTALLDEBUG_UNQUEUE(name) {}
	 #endif


	class StreamInstallInternalOrigin: public StreamInstallCommon, public MessageListener 
	{
	public:
		StreamInstallInternalOrigin() : m_init(false), m_originSDKAvailable(false), m_streamInstallEnabled(false) {};
		void setup();
		void cleanup();
		virtual void onMessage(const Message& message) override;

	private:
		void onOriginAvailable();
		void onProgressiveInstallAvailable();
		void onAreChunksInstalledResponse(bool installed);
		void onQueryChunkStatusResponse(eastl::vector<uint> chunkIds);
		void onChunkUpdateEvent(uint chunkId, uint64_t chunkSize, float progress, uint chunkStatus);

	public:
		ManifestContent m_content;
	private:
		bool m_init;
		bool m_originSDKAvailable;
		bool m_streamInstallEnabled;

		struct TrackingChunkMetadata
		{
			TrackingChunkMetadata() : chunkId(0), progress(0.0), state(ORIGIN_PROGRESSIVE_INSTALLATION_UNKNOWN) { }
			uint chunkId;
			uint64_t chunkSize;
			float progress;
			enumChunkState state;
		};

		struct TrackingGroupMetadata
		{
			eastl::string groupName;
			eastl::set<uint> chunkIds;
		};

		eastl::map<eastl::string, TrackingGroupMetadata> m_trackingGroups;
		eastl::map<uint, TrackingChunkMetadata> m_trackingChunks;
	};

	static StreamInstallInternalOrigin s_siInstance;


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void StreamInstallInternalOrigin::setup()
	{
		MallocScope();

		FB_INFO("[Stream Install] Initial setup");
			
		if (!StreamInstallCommon::manifestExist())
		{
			FB_WARNING_FORMAT("Unable to open streaminstall manifest @ '%s'. StreamInstall disabled.", StreamInstallCommon::getManifestFilename());
			g_coreMessageManager->executeMessage(StreamInstallGameInstalledMessage());
			return;
		}

		StreamInstallCommon::parseManifest(m_content);
	
		eastl::vector<uint> allChunkIds = m_content.m_chunkIds;
		if(allChunkIds.size() == 0)
		{
			FB_WARNING_FORMAT("No chunks present in the manifest @ '%s'. StreamInstall disabled.", StreamInstallCommon::getManifestFilename());
			g_coreMessageManager->executeMessage(StreamInstallGameInstalledMessage());
			return;
		}

		// Stream Install init
		m_init = true;

		if (m_originSDKAvailable)
		{
			onOriginAvailable();
		}
		else
		{
			// Now we will wait to see if the SDK stuff comes online
			FB_INFO("[Stream Install] Manifest definitions loaded, waiting for Origin SDK");
		}
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void StreamInstallInternalOrigin::cleanup()
	{
		cleanupData();
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void StreamInstallInternalOrigin::onMessage(const Message& message) 
	{
		switch (message.category)
		{
		case MessageCategory_OriginStreamInstall:
			{
				// All internal Origin SDK events
				switch (message.type)
				{
					case MessageType_OriginStreamInstallOriginAvailable:
						{
							onOriginAvailable();
						}
						break;
					case MessageType_OriginStreamInstallOriginUnavailable:
						{
							// Stream Install no longer available
							m_streamInstallEnabled = false;

							// SDK failure, we can't proceed
							StreamInstallInstallFailedMessage msg;
							msg.setErrCode(0); // TODO: Do something useful with this code
							g_coreMessageManager->executeMessage(msg);
						}
						break;
					case MessageType_OriginStreamInstallOriginSDKError:
						{
							const OriginStreamInstallOriginSDKErrorMessage* sdkMsg = static_cast<const OriginStreamInstallOriginSDKErrorMessage*>(&message);

							FB_WARNING_FORMAT("[Stream Install] Unable to query Origin SDK about Progressive Installation status.");

							// SDK failure, we can't proceed
							StreamInstallInstallFailedMessage msg;
							msg.setErrCode(sdkMsg->getError());
							g_coreMessageManager->executeMessage(msg);

							// TODO: Call cleanup?
						}
						break;
					case MessageType_OriginStreamInstallIsProgressiveInstallAvailableResponse:
						{
							const OriginStreamInstallIsProgressiveInstallAvailableResponseMessage* msg = static_cast<const OriginStreamInstallIsProgressiveInstallAvailableResponseMessage*>(&message);

							if (msg->getavailable())
							{
								// Call our handler
								onProgressiveInstallAvailable();
							}
							else
							{
								FB_WARNING_FORMAT("Origin SDK reports Progressive Installation is not available.  Game fully installed.");
								g_coreMessageManager->executeMessage(StreamInstallGameInstalledMessage());
							}
						}
						break;
					case MessageType_OriginStreamInstallAreChunksInstalledResponse:
						{
							const OriginStreamInstallAreChunksInstalledResponseMessage* msg = static_cast<const OriginStreamInstallAreChunksInstalledResponseMessage*>(&message);
							onAreChunksInstalledResponse(msg->getinstalled());
						}
						break;
					case MessageType_OriginStreamInstallQueryChunkStatusResponse:
						{
							const OriginStreamInstallQueryChunkStatusResponseMessage* msg = static_cast<const OriginStreamInstallQueryChunkStatusResponseMessage*>(&message);
							ArrayReader<uint> chunkIds = msg->readChunkIds();
							eastl::vector<uint> allChunks;
							for (unsigned int i = 0; i < chunkIds.size(); ++i)
							{
								allChunks.push_back(chunkIds[i]);
							}
							onQueryChunkStatusResponse(allChunks);
						}
						break;
					case MessageType_OriginStreamInstallChunkUpdateEvent:
						{
							const OriginStreamInstallChunkUpdateEventMessage* msg = static_cast<const OriginStreamInstallChunkUpdateEventMessage*>(&message);
							onChunkUpdateEvent(msg->getChunkId(), msg->getChunkSize(), msg->getProgress(), msg->getChunkState());
						}
						break;
				}
			}break;
		case MessageCategory_StreamInstall:
			{
				switch (message.type)
				{
				case MessageType_StreamInstallGameInstalled:
					{
						cleanup();
					}
					break;

				case MessageType_StreamInstallChunkInstalled:
					{
						const StreamInstallChunkInstalledMessage* msg = static_cast<const StreamInstallChunkInstalledMessage*>(&message);
						chunkInstalled(msg->getChunkId());
					}
					break;

				case MessageType_StreamInstallInstallDone:
					{
						const StreamInstallInstallDoneMessage* msg = static_cast<const StreamInstallInstallDoneMessage*>(&message);
						groupInstalled(msg->getInstallGroup().c_str());
					}
					break;
				}
			}break;
		}
	}

	void StreamInstallInternalOrigin::onOriginAvailable()
	{
		if (!m_originSDKAvailable)
		{
			m_originSDKAvailable = true;

			FB_INFO("[Stream Install] Origin SDK Online");
		}

		// If Stream Install hasn't been initialized, nothing to do here
		if (!m_init)
		{
			FB_WARNING("[Stream Install] Skip PI init, since the engine doesn't have the proper chunk definitions loaded.");
			return;
		}

		// Origin specifics

		// Ask the Origin SDK if Progressive Installation is available
		//    This will return as a message
		//    If not available, the game is fully installed
		FB_INFO("[Stream Install] SDK Query IsProgressiveInstallAvailable");
		OriginSDKRequestIsProgressiveInstallAvailableRequestMessage* msg = new (FB_GLOBAL_ARENA) OriginSDKRequestIsProgressiveInstallAvailableRequestMessage();
		g_coreMessageManager->queueMessage(msg);
	}

	void StreamInstallInternalOrigin::onProgressiveInstallAvailable()
	{
		// We're available
		m_streamInstallEnabled = true;

		// PI is available, now get chunk status
		// Ask the Origin SDK if all chunks are installed
		//    This will return as a message.
		eastl::vector<uint> allChunkIds = m_content.m_chunkIds;

		// Query the SDK
		FB_INFO("[Stream Install] SDK Query AreChunksInstalled");
		OriginSDKRequestAreChunksInstalledRequestMessage* msg = new (FB_GLOBAL_ARENA) OriginSDKRequestAreChunksInstalledRequestMessage();
		ArrayEditor<uint> editChunkIds = msg->editChunkIds();
		for (eastl::vector<uint>::size_type idx = 0; idx < allChunkIds.size(); ++idx)
		{
			FB_INFO_FORMAT("[Stream Install] Adding Chunk To AreChunksInstalled Query: %u", allChunkIds[idx]);
			editChunkIds.push_back(allChunkIds[idx]);
		}
		g_coreMessageManager->queueMessage(msg);
	}

	void StreamInstallInternalOrigin::onAreChunksInstalledResponse(bool installed)
	{
		// If not all chunks are installed...
		//    Send installing message
		//    Get chunk status (verify all the chunks we expect to be there are there)
		//       This will return as a message

		if (!installed)
		{
			FB_INFO("[Stream Install] SDK Reports not all chunks are installed.");

			// Setup base class data
			setupData(m_content);

			// Populate our internal tracking group metadata structures
			//typedef eastl::map<eastl::string, eastl::vector<uint> > GroupMapping;
			for (GroupMapping::iterator it = m_content.m_groups.begin(); it != m_content.m_groups.end(); ++it)
			{
				FB_INFO_FORMAT("[Stream Install] Setup Tracking Group: %s", it->first.c_str());

				TrackingGroupMetadata groupMetadata;
				groupMetadata.groupName = it->first;
				groupMetadata.chunkIds.clear();
				groupMetadata.chunkIds.insert(it->second.begin(), it->second.end());  // Insert all the chunk Ids
				m_trackingGroups[it->first] = groupMetadata;
			}

			// Notify everyone that we are installing
			StreamInstallInstallingMessage* msg = new (FB_GLOBAL_ARENA) StreamInstallInstallingMessage();
			g_coreMessageManager->queueMessage(msg);

			// Query the SDK for more detailed chunk status
			FB_INFO("[Stream Install] SDK Query QueryChunkStatus");
			OriginSDKRequestQueryChunkStatusRequestMessage* queryMsg = new (FB_GLOBAL_ARENA) OriginSDKRequestQueryChunkStatusRequestMessage();
			g_coreMessageManager->queueMessage(queryMsg);
		}
		else
		{
			FB_INFO("[Stream Install] SDK Reports all chunks are already installed.");

			// All the chunks we care about are already installed!
			//FB_WARNING_FORMAT("Origin SDK reports all chunks installed.  Game fully installed.");
			g_coreMessageManager->executeMessage(StreamInstallGameInstalledMessage());
		}
	}

	void StreamInstallInternalOrigin::onQueryChunkStatusResponse(eastl::vector<uint> chunkIds)
	{
		uint launchChunks = 0;
		for (eastl::vector<uint>::size_type idx = 0; idx < chunkIds.size(); ++idx)
		{
			uint chunkId = chunkIds[idx];

			if (chunkId < m_content.m_intialChunkCount)
			{
				++launchChunks;
				continue;
			}

			// Update the internal chunk tracking data structures
			m_trackingChunks[chunkId].chunkId = chunkId;
		}

		FB_INFO_FORMAT("[Stream Install] Received Chunk Status for %u Total Chunks, ignoring %u launch chunks.", chunkIds.size(), launchChunks);
	}

	void StreamInstallInternalOrigin::onChunkUpdateEvent(uint chunkId, uint64_t chunkSize, float progress, uint chunkStatus)
	{
		// On chunk update event:
		//    Match chunk from SDK to internal chunk tracking structures, add it if necessary
		//    If chunk is installed, send the chunkInstalled message
		//    If all chunks are installed, send the gameInstalled message
		FB_INFO_FORMAT("[Stream Install] Received Chunk Update for Chunk ID: %u", chunkId);

		if (chunkId < m_content.m_intialChunkCount)
		{
			FB_INFO_FORMAT("[Stream Install] Ignoring Chunk Update for already-completed start chunk (ID: %u)", chunkId);
			return;
		}

		// Update the internal chunk tracking data structures
		m_trackingChunks[chunkId].chunkId = chunkId;
		m_trackingChunks[chunkId].chunkSize = chunkSize;
		m_trackingChunks[chunkId].progress = progress;
		m_trackingChunks[chunkId].state = static_cast<enumChunkState>(chunkStatus); // Just do a direct cast, since the SDK is returning this but we're moving it around as a uint

		// Go over every tracking group and send progress for those
		for (eastl::map<eastl::string, TrackingGroupMetadata>::iterator it = m_trackingGroups.begin(); it != m_trackingGroups.end(); ++it)
		{
			// If the tracking group contains this chunk ID
			if ((it->second).chunkIds.find(chunkId) != (it->second).chunkIds.end())
			{
				float totalProgress = 0.0;
				uint64_t totalSize = 0;
				bool groupCompleted = true;
				// Refresh group progress
				for (eastl::set<uint>::iterator chunkIter = (it->second).chunkIds.begin(); chunkIter != (it->second).chunkIds.end(); ++chunkIter)
				{
					uint curChunk = (*chunkIter);
					// If we know about this chunk
					if (m_trackingChunks.find(curChunk) != m_trackingChunks.end())
					{
						totalSize += m_trackingChunks[curChunk].chunkSize;
						totalProgress += m_trackingChunks[curChunk].progress;

						// If any chunks in the tracking group are not complete, the group is not complete
						if (m_trackingChunks[curChunk].state != ORIGIN_PROGRESSIVE_INSTALLATION_INSTALLED)
						{
							groupCompleted = false;
						}
					}
					else
					{
						// TODO: What to do if we didn't receive info on it?
						FB_WARNING_FORMAT("[Stream Install] No chunk data was ever received for Chunk ID %u.", curChunk);
					}
				}

				// Send out the updated group progress
				float groupProgress = totalProgress / totalSize;
				StreamInstallInstallProgressMessage* msg = new (FB_GLOBAL_ARENA) StreamInstallInstallProgressMessage();
				msg->setProgress((uint)groupProgress);
				msg->setInstallGroup((it->second).groupName);
				g_coreMessageManager->queueMessage(msg);

				if (groupCompleted)
				{
					FB_INFO("[Stream Install] Group Complete");
					StreamInstallInstallDoneMessage* msg = new (FB_GLOBAL_ARENA) StreamInstallInstallDoneMessage();
					msg->setInstallGroup((it->second).groupName);
					g_coreMessageManager->queueMessage(msg);
				}
			}
		}

		if (chunkStatus == ORIGIN_PROGRESSIVE_INSTALLATION_INSTALLED)
		{
			// Notify everyone that a chunk completed
			FB_INFO("[Stream Install] Chunk Complete");
			StreamInstallChunkInstalledMessage* msg = new (FB_GLOBAL_ARENA) StreamInstallChunkInstalledMessage();
			msg->setChunkId(chunkId);
			g_coreMessageManager->queueMessage(msg);
		}

		bool allChunksComplete = true;

		for (eastl::map<uint, TrackingChunkMetadata>::iterator chunkIter = m_trackingChunks.begin(); chunkIter != m_trackingChunks.end(); ++chunkIter)
		{
			if (chunkIter->second.state != ORIGIN_PROGRESSIVE_INSTALLATION_INSTALLED)
			{
				allChunksComplete = false;
				break;
			}
		}

		if (allChunksComplete)
		{
			// Notify everyone
			FB_INFO("[Stream Install] All Chunks Complete");
			StreamInstallGameInstalledMessage* msg = new  (FB_GLOBAL_ARENA) StreamInstallGameInstalledMessage();
			g_coreMessageManager->queueMessage(msg);
		}
	}


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void StreamInstall::init()
	{
		g_coreMessageManager->registerMessageListener(MessageCategory_StreamInstall, &s_siInstance);
		g_coreMessageManager->registerMessageListener(MessageCategory_OriginStreamInstall, &s_siInstance);
		s_siInstance.setup();
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void StreamInstall::deinit()
	{
		g_coreMessageManager->unregisterMessageListener(MessageCategory_StreamInstall, &s_siInstance);
		g_coreMessageManager->unregisterMessageListener(MessageCategory_OriginStreamInstall, &s_siInstance);
		s_siInstance.cleanup();
	}

	// BWDA_START - March 21, 2014 - Christou, Chris - [SHIPJIRA] Add Superbundle Exists check
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	bool StreamInstall::superbundleExists(const char* superbundle, MediaHint hint)
	{
		return s_siInstance.superbundleExists(superbundle, hint);
	}
	// BWDA_END

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void StreamInstall::mountSuperbundle(const char* superbundle, MediaHint hint, bool optional)
	{
		s_siInstance.mountSuperbundle(superbundle, hint, optional);
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void StreamInstall::unmountSuperbundle(const char* superbundle)
	{
		s_siInstance.unmountSuperbundle(superbundle);
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	bool StreamInstall::isInstalling()
	{
		return s_siInstance.isInstalling();
	}	

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	bool StreamInstall::isSuperbundleInstalled(const char* fileName)
	{
		return s_siInstance.isSuperbundleInstalled(fileName);
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	bool StreamInstall::isGroupInstalled(const char* groupName)
	{
		return s_siInstance.isGroupInstalled(groupName);
	}
}

#endif
