//============================================================================
/// @file OriginStreamInstallBackend.h
///
/// Copyright (c) 2014 BioWare ULC
///
/// The source code included in this file is confidential,
/// secret, or proprietary information of BioWare ULC. It
/// may not be used in whole or in part without express
/// written permission from BioWare ULC.
///
/// Origin stream install backend -- coordinates communication with Origin
//============================================================================
#pragma once

#include <Engine/Core/Message/CoreMessages.h>
#include <External/ea/OriginSDK/include/OriginSDK/OriginSDK.h>

namespace fb
{

class OriginStreamInstallBackend : private MessageListener
{
public:
	OriginStreamInstallBackend();
	virtual ~OriginStreamInstallBackend();

	bool create();
	void destroy();

	void registerOriginCallback();
private:
	virtual void onMessage(const Message& message) override;

	// TODO: Is there a better place to get this info?
	static const char * getOriginSDKItemId();

	static OriginErrorT IsProgressiveInstallationAvailableCallback(void * pContext, OriginIsProgressiveInstallationAvailableT * availableInfo, OriginSizeT dummy, OriginErrorT error);
	static OriginErrorT AreChunksInstalledCallback(void * pContext, OriginAreChunksInstalledInfoT * installInfo, OriginSizeT count, OriginErrorT error);
	static OriginErrorT QueryChunkStatusCallback(void *pContext, OriginHandleT hHandle, OriginSizeT total, OriginSizeT available, OriginErrorT error);

	static OriginErrorT OriginEventCallback(int32_t eventId, void* pContext, void* eventData, OriginSizeT count);
	static OriginErrorT ChunkUpdateEventCallback(int32_t eventId, void* pContext, void* eventData, OriginSizeT count);
};

}