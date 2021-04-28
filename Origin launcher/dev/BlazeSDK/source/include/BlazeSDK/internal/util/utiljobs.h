/*! ************************************************************************************************/
/*!
    \file utiljobs.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/
#ifndef BLAZE_UTIL_JOBS_H
#define BLAZE_UTIL_JOBS_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

#include "BlazeSDK/blazesdk.h"
#include "BlazeSDK/util/utilapi.h"
#include "BlazeSDK/job.h"
#include "BlazeSDK/component/util/tdf/utiltypes.h"

#ifdef EA_PLATFORM_XBOXONE
#include "BlazeSDK/component/xblsystemconfigs/tdf/xblsystemconfigs.h"
#endif

struct ProtoHttpRefT;
#define PROTO_HTTP_BUFFER_SIZE (4096)

namespace Blaze
{

#ifdef EA_PLATFORM_XBOXONE
    namespace XBLSystem { class ValidateStringsResponse; }
#endif

namespace Util
{
    class FilterUserTextJobBase : public Job
    {
        NON_COPYABLE(FilterUserTextJobBase);

    protected:
        FilterUserTextJobBase(const UtilAPI::FilterUserTextCb &titleCb, Blaze::Util::UtilAPI *utilApi);
    public:
        virtual ~FilterUserTextJobBase();
        virtual void execute() = 0;
        virtual void cancel(BlazeError err) = 0;
        void dispatchTitleCallback(BlazeError blazeError, const FilterUserTextResponse *response);

    protected:
        UtilAPI::FilterUserTextCb mTitleCb;
        Blaze::Util::UtilAPI *mUtilApi;
    };

#if defined(EA_PLATFORM_XBOXONE)
    class XboxOneValidateString;

    class FilterUserTextJobXboxOne : public FilterUserTextJobBase
    {
        NON_COPYABLE(FilterUserTextJobXboxOne);
    public:
        FilterUserTextJobXboxOne(const UtilAPI::FilterUserTextCb &titleCb, Blaze::Util::UtilAPI *utilApi, 
                const XboxOneValidateString &xboxOneValidateString);
        virtual ~FilterUserTextJobXboxOne();
        virtual void execute();
        virtual void cancel(BlazeError err);

    private:
        JobId mXboxLiveServicesJobId;
    };
#else
    /*! ************************************************************************************************/
    /*! \brief internal job class used for text filtering.

        If we're only doing client-side filtering, the job is essentially a no-op 
            (we just return the unfiltered text back to the user on the next idle).

        If we're doing server-side filtering, the job spans the blaze filter text RPC.
    ***************************************************************************************************/
    class FilterUserTextJob : public FilterUserTextJobBase
    {
        NON_COPYABLE(FilterUserTextJob);

    public:

        FilterUserTextJob(const UserStringList& userStringList, const UtilAPI::FilterUserTextCb &titleCb, Blaze::Util::UtilAPI *utilApi,
                bool isClientOnly);
        virtual ~FilterUserTextJob();
        virtual void execute();
        virtual void cancel(BlazeError err);

    private:
        bool mIsClientOnly;
        FilterUserTextResponse mResponse;
    };
    
#endif

#ifdef EA_PLATFORM_XBOXONE
    class XboxOneValidateString : public Job, private Idler
    {
    public:
        XboxOneValidateString(JobId filterUserTextJobId, Blaze::Util::UtilComponent* component, Blaze::Util::UtilAPI* api,
            bool clientOnly, MemoryGroupId memGroupId = MEM_GROUP_FRAMEWORK_TEMP, uint32_t timeoutMS = UtilAPI::PROFANITY_FILTER_TIMEOUT_MS);
        virtual ~XboxOneValidateString();
        virtual void execute();
        virtual void cancel(BlazeError err);

        bool start(const UserStringList& userStringList);

    private:
        virtual void idle(const uint32_t currentTime, const uint32_t elapsedTime);
        void onVerifyStringResult(const XBLSystem::ValidateStringsResponse* response, BlazeError error, JobId jobId);
        void dispatchTitleCallback(BlazeError blazeError, const FilterUserTextResponse *response, bool removeOwningJob);

    private:
        JobId mFilterUserTextJobId;
        Blaze::Util::UtilAPI *mUtilApi;
        Blaze::Util::UtilComponent *mUtilComponent;
        bool mClientOnly;
        FilterUserTextResponse mResponse;
        MemoryGroupId mMemGroup;   //!< memory group to be used by this class, initialized with parameter passed to constructor};
        const char8_t* mValidateStringUri;

        ProtoHttpRefT *mValidateStringReq;
        XBLSystem::ValidateStringsRequest mReq;
        JobId mJobId;
        uint32_t mTimeout;
    };
#endif

} // namespace Util
} // namespace Blaze

#endif // BLAZE_UTIL_JOBS_H
