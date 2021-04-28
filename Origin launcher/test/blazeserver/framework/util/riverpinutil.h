/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_RIVERPINUTIL_H
#define BLAZE_RIVERPINUTIL_H

/*** Include files *******************************************************************************/
#include "framework/tdf/riverposter.h"

#include "EASTL/bonus/ring_buffer.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{       

class RiverPinUtil : private BlazeThread
{
    NON_COPYABLE(RiverPinUtil);
    public:
        RiverPinUtil();

        bool configure(const PINConfig& pinConfig);

        EA::Thread::ThreadId start() override WARN_UNUSED_RESULT;
        void stop() override;

        void sendPinData(RawBufferPtr pinDataBuffer);

    private:
        void sendPinDataInternal(RawBufferPtr pinDataBuffer);
        void checkCpuThresholds();

        void fillRequestQueue();
        void awaitFillRequestQueue();

        void drainRequestQueue();
        void awaitDrainRequestQueue();

        void run() override;

        bool openLogFile(const char8_t* filename);
        void closeLogFile();

    private:
        HttpConnectionManagerPtr mRiverConnManager;
        FiberJobQueue mRequestJobQueue;

        EA::Thread::ThreadId mThreadId;
        bool mStopping;

        EA::Thread::Mutex mMutex;
        EA::Thread::Condition mCond;
        FiberManager* mFiberManager;

        typedef eastl::vector<RawBufferPtr> RawBufferPtrList;
        RawBufferPtrList mRawBufferPtrList;

        Fiber::EventHandle mRequestJobQueueWorkerAvailableEventHandle;
        Fiber::EventHandle mThreadStoppedEventHandle;

        int mFileDescriptor;

        eastl::string mHttpHeader_x_ea_lint_level;
        eastl::string mHttpHeader_application_id;
        eastl::string mHttpHeader_x_ea_env;
        eastl::string mHttpHeader_x_ea_taxv;
        eastl::string mHttpHeader_x_ea_game_id;
        eastl::string mHttpHeader_x_ea_game_id_type;
        eastl::fixed_vector<const char8_t*, 10> mHttpHeaders;

        eastl::ring_buffer<int64_t> mResponseTimes;
        int64_t mAverageResponseTime;

        int32_t mMaxRequestsPerSecond;
        int32_t mCurrentRequestsPerSecond;
        TimeValue mPreferredNextRequestTime;
        TimeValue mLastCheckCpuThresholds;
};

} // Blaze

#endif // BLAZE_RIVERPINUTIL_H

