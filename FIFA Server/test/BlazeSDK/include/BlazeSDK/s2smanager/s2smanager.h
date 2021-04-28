/**************************************************************************************************/
/*! 
    \file s2smanager.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
***************************************************************************************************/

#ifndef S2SMANAGER_H
#define S2SMANAGER_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

#include "BlazeSDK/blazehub.h"
#include "BlazeSDK/component/framework/tdf/networkaddress.h"
#include "DirtySDK/proto/protohttp.h"

namespace Blaze
{
namespace S2SManager
{

#define S2S_PROTO_HTTP_BUFFER_SIZE (50000)

class BLAZESDK_API S2SManagerListener
{
public:
    virtual void onFetchedS2SToken(BlazeError err, const char8_t* token) = 0;
};

class BLAZESDK_API S2SManager: protected Idler
{
public:
    static S2SManager* create(BlazeHub* hub);
    BlazeError initialize(const char8_t* certData, const char8_t* keyData, const char8_t* configOverrideInstanceName = nullptr, const char8_t* identityConnectHost = nullptr);
    void getS2SToken();

    ~S2SManager();

    bool isInitialized() const { return mTokenReq != nullptr; }
    bool hasValidAccessToken() const { return mTokenExpiry > TimeValue::getTimeOfDay(); }
    void addListener(S2SManagerListener* listener) { mDispatcher.addDispatchee(listener); }
    void removeListener(S2SManagerListener* listener) { mDispatcher.removeDispatchee(listener); }

protected:
    virtual void idle(const uint32_t currentTime, const uint32_t elapsedTime);

private:
    BlazeHub *mHub;
    MemoryGroupId mMemGroup;
    ProtoHttpRefT* mTokenReq;
    eastl::string mAccessToken;
    eastl::string mTokenUrl;
    RawBuffer mData;
    TimeValue mTokenExpiry;
    bool mRequestInProgress;

    typedef Dispatcher<S2SManagerListener> S2SManagerDispatcher;
    S2SManagerDispatcher mDispatcher;

    BlazeError beginTokenRequest();

private:
    S2SManager(BlazeHub *hub, MemoryGroupId memGroupId = MEM_GROUP_FRAMEWORK_TEMP);
    NON_COPYABLE(S2SManager);
};

} // namespace S2SManager
} // namespace Blaze

#endif // S2SMANAGER_H
