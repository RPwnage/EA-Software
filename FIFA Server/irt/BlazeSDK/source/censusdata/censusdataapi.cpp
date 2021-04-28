/*************************************************************************************************/
/*!
\file censusdataapi.cpp


\attention
(c) Electronic Arts. All Rights Reserved.
*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "BlazeSDK/internal/internal.h"
#include "BlazeSDK/componentmanager.h"
#include "BlazeSDK/blazehub.h"
#include "BlazeSDK/connectionmanager/connectionmanager.h"
#include "BlazeSDK/censusdata/censusdataapi.h"
#include "BlazeSDK/usermanager/usermanager.h"
#include "BlazeSDK/blazesdkdefs.h"


namespace Blaze
{
namespace CensusData
{   
    void CensusDataAPI::createAPI(BlazeHub &hub, EA::Allocator::ICoreAllocator* allocator)
    {
        if (hub.getCensusDataAPI() != nullptr)
        {
            BLAZE_SDK_DEBUGF("[CensusDataAPI] Warning: Ignoring attempt to recreate API");
            return;
        }

        CensusDataComponent::createComponent(&hub);
        if (Blaze::Allocator::getAllocator(MEM_GROUP_CENSUSDATA) == nullptr)
            Blaze::Allocator::setAllocator(MEM_GROUP_CENSUSDATA, allocator!=nullptr? allocator : Blaze::Allocator::getAllocator());
        hub.createAPI(CENSUSDATA_API, BLAZE_NEW(MEM_GROUP_CENSUSDATA, "createAPI") CensusDataAPI(hub));
    }

    CensusDataAPI::CensusDataAPI(BlazeHub &blazeHub)
        : SingletonAPI(blazeHub),
          mComponent(nullptr),
          mCensusDataByTdfId(MEM_GROUP_CENSUSDATA, "CensusDataByTdfId"),
          mResubscribeTimeout(2000*1000),
          mResubscribeJobId(INVALID_JOB_ID),
          mAttemptResubscribe(false)
    {
        mComponent = getBlazeHub()->getComponentManager()->getCensusDataComponent();
        if (EA_UNLIKELY(!mComponent))
        {
            BlazeAssert(false && "CensusDataAPI - the census data component is invalid.");
            return;
        }
        registerEventHandlers();

        getBlazeHub()->getUserManager()->addPrimaryUserListener(this);
    }


    CensusDataAPI::~CensusDataAPI()
    {
        mAttemptResubscribe = false;
        if (mResubscribeJobId != INVALID_JOB_ID)
            getBlazeHub()->getScheduler()->cancelJob(mResubscribeJobId);

        clearEventHandlers();
        getBlazeHub()->getUserManager()->removePrimaryUserListener(this);

        // Remove any outstanding txns.  Canceling here can be dangerous here as it will still attempt
        // to call the callback.  Since the object is being deleted, we go with the remove.
        getBlazeHub()->getScheduler()->removeByAssociatedObject(this);
        mComponent = nullptr;
    }

    
    void CensusDataAPI::helperCallback(BlazeError err, JobId jobId)
    {
        Job* job = getBlazeHub()->getScheduler()->getJob(jobId);
        if (job != nullptr)
        {
            const SubscribeToCensusDataCb* titleCb = (const SubscribeToCensusDataCb*)&job->getAssociatedTitleCb();
            if (titleCb->isValid())
                (*titleCb)(err);
        }
    }

    /*! ****************************************************************************/
    /*! \brief subscribe to server census data, if succeeded, server will send
            census data periodically 

        \return false if the job failed to run - no execution to server occurred.
    ********************************************************************************/
    JobId CensusDataAPI::subscribeToCensusData(const SubscribeToCensusDataCb &resultFunctor)
    {
        SubscribeToCensusDataJobCb helperCb = MakeFunctor(this, &CensusDataAPI::helperCallback);
        JobId jobId = subscribeToCensusData(helperCb);
        Job::addTitleCbAssociatedObject(getBlazeHub()->getScheduler(), jobId, resultFunctor);  // Override the job with the real title cb
        return jobId;
    }
    JobId CensusDataAPI::subscribeToCensusData(const SubscribeToCensusDataJobCb &resultFunctor)
    {
        BLAZE_SDK_DEBUGF("[CensusDataComponent] CensusDataAPI::subscribeToCensusData()...\n");

        mAttemptResubscribe = true;
        if (mResubscribeJobId != INVALID_JOB_ID)
        {
            getBlazeHub()->getScheduler()->cancelJob(mResubscribeJobId);
            mResubscribeJobId = INVALID_JOB_ID;
        }

        // Send off request to server; stuffing the title's cb functor into the rpc payload
        SubscribeToCensusDataUpdatesRequest request;
        request.setResubscribe(false);
        JobId jobId = mComponent->subscribeToCensusDataUpdates(request, MakeFunctor(this, &CensusDataAPI::onSubscribeCensusDataCb), resultFunctor);
        Job::addTitleCbAssociatedObject(getBlazeHub()->getScheduler(), jobId, resultFunctor);
        return jobId;
    }

    /*! ****************************************************************************/
    /*! \brief subscribe to server census data, if succeeded, server will send
            census data periodically 

        \return false if the job failed to run - no execution to server occurred.
    ********************************************************************************/
    JobId CensusDataAPI::unsubscribeFromCensusData(const UnsubscribeFromCensusDataCb &resultFunctor)
    {
        SubscribeToCensusDataJobCb helperCb = MakeFunctor(this, &CensusDataAPI::helperCallback);
        JobId jobId = unsubscribeFromCensusData(helperCb);
        Job::addTitleCbAssociatedObject(getBlazeHub()->getScheduler(), jobId, resultFunctor); // Override the job with the real title cb
        return jobId;
    }
    JobId CensusDataAPI::unsubscribeFromCensusData(const UnsubscribeFromCensusDataJobCb &resultFunctor)
    {
        BLAZE_SDK_DEBUGF("[CensusDataComponent] CensusDataAPI::unsubscribeFromCensusData()...\n");

        mAttemptResubscribe = false;
        if (mResubscribeJobId != INVALID_JOB_ID)
        {
            getBlazeHub()->getScheduler()->cancelJob(mResubscribeJobId);
            mResubscribeJobId = INVALID_JOB_ID;
        }

        // Send off request to server; stuffing the title's cb functor into the rpc payload
        JobId jobId = mComponent->unsubscribeFromCensusData(MakeFunctor(this, &CensusDataAPI::onUnsubscribeCensusDataCb), resultFunctor);
        Job::addTitleCbAssociatedObject(getBlazeHub()->getScheduler(), jobId, resultFunctor);
        return jobId;
    } 

    /*! ****************************************************************************/
    /*! \brief get regions from server census data, if succeeded, server will send
         a region map
    ********************************************************************************/
    JobId CensusDataAPI::getRegionCounts(const CensusDataComponent::GetRegionCountsCb &callback)
    {
        JobId jobId = mComponent->getRegionCounts(callback);
        Job::addTitleCbAssociatedObject(getBlazeHub()->getScheduler(), jobId, callback);
        return jobId;
    }

    /*! ****************************************************************************/
    /*! \brief get latest census data from server
    ********************************************************************************/
    JobId CensusDataAPI::getLatestCensusData(const CensusDataComponent::GetLatestCensusDataCb &callback)
    {
        JobId jobId = mComponent->getLatestCensusData(callback);
        Job::addTitleCbAssociatedObject(getBlazeHub()->getScheduler(), jobId, callback);
        return jobId;
    }

    /*! ****************************************************************************/
    /*! \brief receive the census data from the server,dispatch the notification to
         consumer listener and api listener

         \param[in]notification - notification got from server
    ********************************************************************************/
    void CensusDataAPI::onNotifyServerCensusData(const NotifyServerCensusData* notification, uint32_t userIndex)
    {
        if (mResubscribeJobId != INVALID_JOB_ID)
            getBlazeHub()->getScheduler()->cancelJob(mResubscribeJobId);

        // Parse the notification and store Tdf's in a map by TdfId for easy retrieval.
        // We take over the ownership of individual Tdfs in the notification. If an old 
        // value is already in the table, we free it. Values will also need to be cleaned 
        // up at destruction time (see dtor).
        const NotifyServerCensusDataList& cdList = notification->getCensusDataList();
        for (NotifyServerCensusDataList::const_iterator it = cdList.begin(), endit = cdList.end();
             it != endit; ++it)
        {
            if ((*it)->getTdf() != nullptr)
            {
                // Manually clone the data item, it is currently owned by *it which contains it.
                NotifyServerCensusDataItemPtr newItem = BLAZE_NEW(Blaze::MEM_GROUP_CENSUSDATA, "NotifyServerCensusDataItem") NotifyServerCensusDataItem;
                newItem->setTdf(*(*it)->getTdf()->clone(*Allocator::getAllocator(Blaze::MEM_GROUP_CENSUSDATA)));
                const EA::TDF::TdfId id = newItem->getTdf()->getTdfId();
                mCensusDataByTdfId[id] = newItem;
            }
        }

        if (mAttemptResubscribe)
        {
            TimeValue notificationTimeout = notification->getCensusNotificationPeriod() + notification->getNotificationTimeout();
            mResubscribeTimeout = notification->getResubscribeTimeout();
            mResubscribeJobId = getBlazeHub()->getScheduler()->scheduleMethod("resubscribeToCensusData", this, &CensusDataAPI::resubscribeToCensusData, this, (uint32_t)notificationTimeout.getMillis());
        }

        mDispatcher.dispatch("onNotifyCensusData", &CensusDataAPIListener::onNotifyCensusData);
    }


    void CensusDataAPI::onAuthenticated(uint32_t userIndex)
    {
    }


    void CensusDataAPI::onDeAuthenticated(uint32_t userIndex) 
    {
        // This isn't technically correct for MLU situations where we may want to maintain a resubscription when an MLU player leaves the Game.  (Maintains existing functionality.)
        // (Code should check if ALL players are deauthenticated, or track who made original subscription.)
        mAttemptResubscribe = false;
        if (mResubscribeJobId != INVALID_JOB_ID)
        {
            getBlazeHub()->getScheduler()->cancelJob(mResubscribeJobId);
            mResubscribeJobId = INVALID_JOB_ID;
        }
    }

    void UnsubscribeFromCensusDataCallback(BlazeError err)
    {
        // Error message is ignored
    }

    void CensusDataAPI::onPrimaryLocalUserChanged(uint32_t userIndex)
    {
        BLAZE_SDK_DEBUGF("[CensusDataAPI:%p].onPrimaryLocalUserChanged : User index %u is now primary user\n", this, userIndex);

        if (getBlazeHub()->getConnectionManager()->isConnected())
        {
            // Unsubscribe from any previous census updates:
            if (EA_LIKELY(mComponent))
                unsubscribeFromCensusData(UnsubscribeFromCensusDataCb(UnsubscribeFromCensusDataCallback));
        }

        mComponent = getBlazeHub()->getComponentManager(userIndex)->getCensusDataComponent();
        if (EA_UNLIKELY(!mComponent))
        {
            BlazeAssert(false && "CensusDataAPI - the census data component is invalid.");
            return;
        }
    }

    void CensusDataAPI::registerEventHandlers()
    {
        for (uint32_t userIndex = 0; userIndex < getBlazeHub()->getNumUsers(); ++userIndex)
        {
            CensusDataComponent* component = getBlazeHub()->getComponentManager(userIndex)->getCensusDataComponent();
            component->setNotifyServerCensusDataHandler(CensusDataComponent::NotifyServerCensusDataCb(this, &CensusDataAPI::onNotifyServerCensusData));
        }
    }

    void CensusDataAPI::clearEventHandlers()
    {
        for (uint32_t userIndex = 0; userIndex < getBlazeHub()->getNumUsers(); ++userIndex)
        {
            CensusDataComponent* component = getBlazeHub()->getComponentManager(userIndex)->getCensusDataComponent();
            component->clearNotifyServerCensusDataHandler();
        }
    }

    void CensusDataAPI::onSubscribeCensusDataCb(const SubscribeToCensusDataUpdatesResponse *subscribeToCensusDataUpdatesResponse, BlazeError err, JobId jobid, const SubscribeToCensusDataJobCb resultCb)
    {
        if (err == ERR_OK)
        {
            TimeValue notificationTimeout = subscribeToCensusDataUpdatesResponse->getCensusNotificationPeriod() + subscribeToCensusDataUpdatesResponse->getNotificationTimeout();
            mResubscribeTimeout = subscribeToCensusDataUpdatesResponse->getResubscribeTimeout();

            if (mResubscribeJobId != INVALID_JOB_ID)
                getBlazeHub()->getScheduler()->cancelJob(mResubscribeJobId);

            if (mAttemptResubscribe)
            {
                mResubscribeJobId = getBlazeHub()->getScheduler()->scheduleMethod("resubscribeToCensusData", this, &CensusDataAPI::resubscribeToCensusData, this, (uint32_t)notificationTimeout.getMillis());
            }
        }

        resultCb(err, jobid);
    }  /*lint !e1746 !e1762 - Avoid lint to check whether parameter or function could be made const reference or const*/

    void CensusDataAPI::onUnsubscribeCensusDataCb(BlazeError err, JobId jobid, const UnsubscribeFromCensusDataJobCb resultCb)
    {
        resultCb(err, jobid);
    }  /*lint !e1746 !e1762 - Avoid lint to check whether parameter or function could be made const reference or const*/

    void CensusDataAPI::onResubscribeCensusDataCb(const SubscribeToCensusDataUpdatesResponse *subscribeToCensusDataUpdatesResponse, BlazeError err, JobId jobid)
    {
        // We may have received a ServerCensusData notification after making the resubscribe request but before entering this callback.
        // If this is the case, then we've already scheduled our next resubscribe attempt and we shouldn't do anything here.
        if (mResubscribeJobId != INVALID_JOB_ID)
            return;

        if (mAttemptResubscribe)
        {
            if (err == ERR_OK)
            {
                TimeValue notificationTimeout = subscribeToCensusDataUpdatesResponse->getCensusNotificationPeriod() + subscribeToCensusDataUpdatesResponse->getNotificationTimeout();
                mResubscribeTimeout = subscribeToCensusDataUpdatesResponse->getResubscribeTimeout();

                mResubscribeJobId = getBlazeHub()->getScheduler()->scheduleMethod("resubscribeToCensusData", this, &CensusDataAPI::resubscribeToCensusData, this, (uint32_t)notificationTimeout.getMillis());
            }
            else
            {
                mResubscribeJobId = getBlazeHub()->getScheduler()->scheduleMethod("resubscribeToCensusData", this, &CensusDataAPI::resubscribeToCensusData, this, (uint32_t)mResubscribeTimeout.getMillis());
            }
        }
    }  /*lint !e1746 !e1762 - Avoid lint to check whether parameter or function could be made const reference or const*/

    void CensusDataAPI::resubscribeToCensusData()
    {
        BLAZE_SDK_DEBUGF("[CensusDataComponent] CensusDataAPI::resubscribeToCensusData()...\n");

        mAttemptResubscribe = true;
        mResubscribeJobId = INVALID_JOB_ID;
        SubscribeToCensusDataUpdatesRequest request;
        request.setResubscribe(true);
        mComponent->subscribeToCensusDataUpdates(request, MakeFunctor(this, &CensusDataAPI::onResubscribeCensusDataCb));
    }

    void CensusDataAPI::logStartupParameters() const
    {
        //  NO_IMPL
    }

} // CensusData
} // Blaze
