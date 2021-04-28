/*************************************************************************************************/
/*!
\file   censusdataslaveimpl.cpp


\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
\class CensusDataSlaveImpl

CensusData Slave implementation.

\note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// main include
#include "framework/blaze.h"
#include "framework/connection/selector.h"
#include "censusdata/tdf/censusdata.h"
#include "censusdata/tdf/censusdata_server.h"
#include "censusdataslaveimpl.h"
#include "framework/component/censusdata.h" // for CensusDataManager & CensusDataProvider

// censusdata includes

namespace Blaze
{
namespace CensusData
{
    // static
    CensusDataSlave* CensusDataSlave::createImpl()
    {
        return BLAZE_NEW_NAMED("CensusDataSlaveImpl") CensusDataSlaveImpl();
    }


    /*** Public Methods ******************************************************************************/
    CensusDataSlaveImpl::CensusDataSlaveImpl()
        :  CensusDataSlaveStub(),
           mComponentCensusData(nullptr),
           mIdleTimerId(INVALID_TIMER_ID),
           mTimerId(INVALID_TIMER_ID)
    {
        mLastNotifySessionIter = mUserSessionSet.end();
    }

    CensusDataSlaveImpl::~CensusDataSlaveImpl()
    {
    }

    bool CensusDataSlaveImpl::onValidateConfig(CensusDataConfig& config, const CensusDataConfig* referenceConfigPtr, ConfigureValidationErrors& validationErrors) const
    {
        if (config.getCensusNotificationPeriodMS() == 0)
        {
            eastl::string msg;
            msg.sprintf("[CensusDataSlaveImpl] Config censusNotificationPeriodMS must be Non-Zero");
            EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
            str.set(msg.c_str());
        }
        if (config.getIdlePeriodMS() == 0)
        {
            eastl::string msg;
            msg.sprintf("[CensusDataSlaveImpl] Config idlePeriodMS must be Non-Zero");
            EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
            str.set(msg.c_str());
        }
        if (config.getNotificationTimeout().getMicroSeconds() == 0)
        {
            eastl::string msg;
            msg.sprintf("[CensusDataSlaveImpl] Config notificationTimeout must be Non-Zero");
            EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
            str.set(msg.c_str());
        }
        if (config.getResubscribeTimeout().getMicroSeconds() == 0)
        {
            eastl::string msg;
            msg.sprintf("[CensusDataSlaveImpl] Config resubscribeTimeout must be Non-Zero");
            EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
            str.set(msg.c_str());
        }
        return validationErrors.getErrorMessages().empty();
    }
    
    bool CensusDataSlaveImpl::onConfigure()
    {
        return true;
    }  /*lint !e1961 - Avoid lint to check whether virtual member function could be made const*/

    bool CensusDataSlaveImpl::onResolve()
    {
        if (gUserSessionMaster != nullptr)
            gUserSessionMaster->addLocalUserSessionSubscriber(*this);

        return true;
    }

    bool CensusDataSlaveImpl::onActivate()
    {
        gController->addDrainListener(*this);

        //First we need to scan any remote instances we already know about and pretend like they're new
        gController->syncRemoteInstanceListener(*this);

        // now register a listener to pick up other remote instances as they come up
        gController->addRemoteInstanceListener(*this);

        gUserSessionManager->addSubscriber(*this);

        // at this point all the usersessions are already sync'ed, 
        // we need to iterate them in a nonblocking fashion and synchronize the census counts
        gUserSessionManager->addUserRegionForCensus();

        BlazeRpcError rc;
        // Need to ensure user manager is active and replicating. Otherwise when gamemanager initializes replication
        // after resolving, the slave will spew errors due to no user sessions.
        rc = gUserSessionManager->waitForState(ACTIVE, "CensusDataSlaveImpl::onActivate");
        if (rc != ERR_OK)
        {
            FATAL_LOG("[CensusDataSlave] Unable to activate, user session manager not active.");
            return false;
        }

        TimeValue startTime = TimeValue::getTimeOfDay();
        mTimerId = gSelector->scheduleFiberTimerCall((startTime + getConfig().getCensusNotificationPeriodMS() * 1000), this, &CensusDataSlaveImpl::notificationLoop,
            "CensusDataSlaveImpl::notificationLoop");
        return (INVALID_TIMER_ID != mTimerId);
    }

    // Note: components are shut down in a specific (safe) order, so unregister from maps/session manager here
    void CensusDataSlaveImpl::onShutdown()
    {
        // cancel notificationLoop timer
        if (mTimerId != INVALID_TIMER_ID)
        {
            gSelector->cancelTimer(mTimerId);
            mTimerId = INVALID_TIMER_ID;
        }

        if (gUserSessionMaster != nullptr)
            gUserSessionMaster->removeLocalUserSessionSubscriber(*this);

        // remove itself from user session manager
        gUserSessionManager->removeSubscriber(*this);

        gController->removeRemoteInstanceListener(*this);

        gController->removeDrainListener(*this);
      
        mComponentCensusData = nullptr;        
    }

    /*! ************************************************************************************************/
    /*! \brief add the current user session in current context to the user session list in census data
        manager, when the period time is due, server census data will be sent to the user

        \return SubscribeToCensusDataError
    ***************************************************************************************************/
    SubscribeToCensusDataError::Error CensusDataSlaveImpl::processSubscribeToCensusData(const Message *message)
    {
        if (subscribeToCensusDataInternal())
            return SubscribeToCensusDataError::ERR_OK;
        else
            return SubscribeToCensusDataError::CENSUSDATA_ERR_PLAYER_ALREADY_SUBSCRIBED;
    }

    SubscribeToCensusDataUpdatesError::Error CensusDataSlaveImpl::processSubscribeToCensusDataUpdates(const Blaze::CensusData::SubscribeToCensusDataUpdatesRequest &request, Blaze::CensusData::SubscribeToCensusDataUpdatesResponse &response, const Message *message)
    {
        response.setCensusNotificationPeriod(getConfig().getCensusNotificationPeriodMS()*1000);
        response.setNotificationTimeout(getConfig().getNotificationTimeout());
        response.setResubscribeTimeout(getConfig().getResubscribeTimeout());

        if (subscribeToCensusDataInternal())
            return SubscribeToCensusDataUpdatesError::ERR_OK;
        else
            return request.getResubscribe() ? SubscribeToCensusDataUpdatesError::ERR_OK : SubscribeToCensusDataUpdatesError::CENSUSDATA_ERR_PLAYER_ALREADY_SUBSCRIBED;
    }

    bool CensusDataSlaveImpl::subscribeToCensusDataInternal()
    {
        if (!gCurrentUserSession)
            return false;

        UserSessionId sessionId = gCurrentUserSession->getUserSessionId();
       
        if (mUserSessionSet.find(sessionId) == mUserSessionSet.end())
        {
            mUserSessionSet.insert(sessionId);
            return true;
        }
       
        return false;
    }

    /*! ************************************************************************************************/
    /*! \brief remove the current user session in current context to the user session list in census data
            manager, the user won't get any server census data after this
    
        \return UnsubscribeFromCensusDataError
    ***************************************************************************************************/
    UnsubscribeFromCensusDataError::Error CensusDataSlaveImpl::processUnsubscribeFromCensusData(const Message *message)
    {
        UserSessionId userSession = gCurrentUserSession->getUserSessionId();


        UserSessionIdSet::iterator foundSession = mUserSessionSet.find(userSession);
        if (mUserSessionSet.find(userSession) != mUserSessionSet.end())
        {
            if (mLastNotifySessionIter == foundSession)
            {
                mLastNotifySessionIter = mUserSessionSet.erase(foundSession);
            }
            else
                mUserSessionSet.erase(foundSession);

            return UnsubscribeFromCensusDataError::ERR_OK;
        }

        return UnsubscribeFromCensusDataError::CENSUSDATA_ERR_PLAYER_NOT_SUBSCRIBED;
    }

    /*! ************************************************************************************************/
    /*! \brief return the number of user keyed by region map collected after user successfully login
        
        note: region here is account locale

        \return GetRegionCountsError
    ***************************************************************************************************/
    GetRegionCountsError::Error CensusDataSlaveImpl::processGetRegionCounts(Blaze::CensusData::RegionCounts &response, const Message* message)
    {
        gCensusDataManager->getNumOfUsersByRegionMap().copyInto(response.getNumOfUsersByRegion());

        if (IS_LOGGING_ENABLED(Logging::SPAM))
        {
            NumOfUsersByRegionMap::const_iterator it = response.getNumOfUsersByRegion().begin();    
            NumOfUsersByRegionMap::const_iterator itend = response.getNumOfUsersByRegion().end();
            for (; it != itend; it++)
            {
                SPAM_LOG("Region Name: " << it->first.c_str() << ", count: " << it->second << ". \n");
            }
        }
        
        return GetRegionCountsError::ERR_OK;
    }

    /////////////////  UserSessionSubscriber interface functions  /////////////////////////////
    void CensusDataSlaveImpl::onUserSessionExistence(const UserSession& userSession)
    {
        // we only count console user, ignore web user and dedicated server user
        if (userSession.isGameplayUser())
            gCensusDataManager->addUserRegion(userSession.getBlazeId(), userSession.getSessionCountry());
    }

    void CensusDataSlaveImpl::onUserSessionMutation(const UserSession& userSession, const UserSessionExistenceData& newExistenceData)
    {
        if (userSession.getSessionCountry() != newExistenceData.getSessionCountry())
        {
            if (gUserSessionManager->getClientTypeDescription(newExistenceData.getClientType()).getAllowAsGamePlayer())
            {
                gCensusDataManager->removeUserRegion(userSession.getBlazeId(), userSession.getSessionCountry());
                gCensusDataManager->addUserRegion(userSession.getBlazeId(), newExistenceData.getSessionCountry());
            }
        }
    }

    void CensusDataSlaveImpl::onUserSessionExtinction(const UserSession& userSession)
    {
        // we only count console user, ignore web suer and dedicated server user
        if (userSession.isGameplayUser())
            gCensusDataManager->removeUserRegion(userSession.getBlazeId(), userSession.getSessionCountry());
    }

    void CensusDataSlaveImpl::onLocalUserSessionLogout(const UserSessionMaster& userSession)
    {
        UserSessionIdSet::iterator foundSessionIt = mUserSessionSet.find(userSession.getUserSessionId());
        if (foundSessionIt != mUserSessionSet.end())
        {
            if (mLastNotifySessionIter == foundSessionIt)
            {
                mLastNotifySessionIter = mUserSessionSet.erase(foundSessionIt);
            }
            else
            {
                mUserSessionSet.erase(foundSessionIt);
            }
        }
    }
    
    void CensusDataSlaveImpl::fetchRemoteCensusData(Blaze::CensusDataByComponent &outCensusData)
    {
        // collect information from remote instances
        ComponentIdSet remoteProviderComponentIds(mRemoteProviderComponentIds);
        while (!remoteProviderComponentIds.empty())
        {
            GetCensusDataForComponentsRequest req;
            GetCensusDataForComponentsResponse resp;
            bool reqLocal = false;
            bool reqActive = true;

            for (ComponentId componentId : remoteProviderComponentIds)
                req.getComponentIds().push_back(componentId);
                         
            Component *component = gController->getComponent(*remoteProviderComponentIds.begin(), reqLocal, reqActive);
            if (component == nullptr)
            {
                WARN_LOG("[CensusDataSlaveImpl].fetchRemoteCensusData(): Could not get remote component instance for component id "
                    << *remoteProviderComponentIds.begin());
                break;
            }

            RpcCallOptions opts;
            opts.routeTo.setInstanceId(component->getAnyInstanceId());
                    
            BlazeRpcError err = gController->getCensusDataForComponents(req, resp, opts);
            if (err != ERR_OK)
            {
                WARN_LOG("[CensusDataSlaveImpl].fetchRemoteCensusData(): populateCensusData for remote components got error " << (ErrorHelp::getErrorName(err)) 
                            << ", line:" << __LINE__ << "!");
                break;
            }

            // remove components for which we just got results
            for (auto censusData : resp.getCensusDataByComponent())
            {
                ComponentId componentId = censusData.first;
                remoteProviderComponentIds.erase(componentId);

                // Add the data to the response:
                if (outCensusData.find(componentId) == outCensusData.end())
                {
                    outCensusData[componentId] = outCensusData.allocate_element();
                    censusData.second->copyInto(*outCensusData[componentId]);
                }
            }
        }
    }

    void CensusDataSlaveImpl::notificationLoop()
    {
        mDrainEventHandle = Fiber::getNextEventHandle();
        

        while (!gController->isDraining())
        {
            
            uint32_t numOfPeriod(0);
            if (mUserSessionSet.empty())
            {
                TRACE_LOG("[CensusDataSlaveImpl]notificationLoop: currently no subscriber. skip processing.");
            }
            else
            {
                Blaze::CensusDataByComponent censusDataByComponent;

                // Get the latest Census Data:        
                fetchRemoteCensusData(censusDataByComponent);
                BlazeRpcError err = gCensusDataManager->populateCensusData(censusDataByComponent);
                if (err != ERR_OK)
                {
                    WARN_LOG("[CensusDataSlaveImpl].notificationLoop(): populateCensusData got error " << (ErrorHelp::getErrorName(err))
                        << ", line:" << __LINE__ << "!");
                }


                // We create the tdf on heap instead of stack since we have sleep code down in the 
                // function. It's possible that while the thread is sleeping the component is shutdown and
                // the tdf that holds the map is still on the stack if we use local tdf. 
                // So we create tdf on heap and delete the tdf before sleep to avoid possible leak
                mComponentCensusData = BLAZE_NEW NotifyServerCensusData();
                mComponentCensusData->setCensusNotificationPeriod(1000 * getConfig().getCensusNotificationPeriodMS());
                mComponentCensusData->setNotificationTimeout(getConfig().getNotificationTimeout());
                mComponentCensusData->setResubscribeTimeout(getConfig().getResubscribeTimeout());
                NotifyServerCensusDataList* nsList = &mComponentCensusData->getCensusDataList();
                nsList->reserve(censusDataByComponent.size());

                for (auto curCensusByComp : censusDataByComponent)
                {
                    NotifyServerCensusDataItemPtr nsCensusData = nsList->pull_back();
                    nsCensusData->setTdf(*curCensusByComp.second->get());
                }

                uint32_t numOfUsers = mUserSessionSet.size();
                uint32_t idlePeriod = getConfig().getCensusNotificationPeriodMS() / getConfig().getIdlePeriodMS();
                uint32_t userCount = numOfUsers / idlePeriod;

                if (userCount == 0)
                {
                    // if usercount is 0, we will use 1 as advance step
                    userCount = 1;
                }
                else if ((numOfUsers % idlePeriod) != 0)
                {
                    // in case some session is missed, we always make sure advance
                    // step will cover all the sessions
                    userCount++;
                }

                if (mLastNotifySessionIter == mUserSessionSet.end())
                {
                    mLastNotifySessionIter = mUserSessionSet.begin();
                }
                for (; (numOfPeriod < idlePeriod) && !gController->isDraining(); numOfPeriod++)
                {
                    if (mLastNotifySessionIter == mUserSessionSet.end())
                        break;

                    UserSessionIdSet::iterator updatedLastIterator = mLastNotifySessionIter;

                    // advance the update iterator
                    for (uint32_t step = 0; step < userCount; ++step)
                    {
                        updatedLastIterator++;
                        if (updatedLastIterator == mUserSessionSet.end())
                            break;
                    }

                    typedef eastl::vector<UserSessionId> UserSessionIdVector;
                    UserSessionIdVector targetIds;
                    targetIds.reserve(eastl::distance(mLastNotifySessionIter, updatedLastIterator));
                    targetIds.assign(mLastNotifySessionIter, updatedLastIterator);

                    sendNotifyServerCensusDataToUserSessionsById(
                        targetIds.begin(),
                        targetIds.end(),
                        UserSessionIdIdentityFunc(),
                        mComponentCensusData);

                    mLastNotifySessionIter = updatedLastIterator;
                    if (mLastNotifySessionIter == mUserSessionSet.end())
                    {
                        break;
                    }
                    BlazeRpcError error = gSelector->sleep(getConfig().getIdlePeriodMS() * 1000, &mTimerId, &mDrainEventHandle);
                    if (error != ERR_OK)
                    {
                        WARN_LOG("[CensusDataSlaveImpl].notificationLoop(): sleep error is " << (ErrorHelp::getErrorName(error))
                            << ", line:" << __LINE__ << "!");
                    }

                    // we have to double check if the mLastNotifySessionIter is valid or not after the sleep
                    // since client could unsubscribe during the sleep and turns the mLastNotifySessionIter to be invalid
                    //
                    if (mLastNotifySessionIter == mUserSessionSet.end())
                    {
                        break;
                    }
                }

                mComponentCensusData = nullptr;
            }

            if (!gController->isDraining())
            {
                uint32_t extraTime = getConfig().getCensusNotificationPeriodMS() - (numOfPeriod * getConfig().getIdlePeriodMS());
                BlazeRpcError err = gSelector->sleep(extraTime * 1000, &mTimerId, &mDrainEventHandle);
                if (err != ERR_OK)
                {
                    WARN_LOG("[CensusDataSlaveImpl].notificationLoop(): sleep error is " << (ErrorHelp::getErrorName(err))
                        << ", line:" << __LINE__ << "!");
                }
            }
        }
    

        mDrainEventHandle = Fiber::INVALID_EVENT_ID;
    }

    // Controller::RemoteServerInstanceListener
    void CensusDataSlaveImpl::onRemoteInstanceChanged(const RemoteServerInstance& instance)
    {
        if (instance.isActive() && instance.getInstanceType() == ServerConfig::AUX_SLAVE)
        {
            // remote census data providers can be located only in aux slaves
            gSelector->scheduleFiberCall(this, &CensusDataSlaveImpl::getRemoteCensusDataProvidersFiber, instance.getInstanceId(), "CensusDataSlaveImpl::getRemoteCensusDataProvidersFiber");
        }
    }

    void CensusDataSlaveImpl::onControllerDrain()
    {
        if (mDrainEventHandle.isValid())
            Fiber::signal(mDrainEventHandle, ERR_OK);
    }

    void CensusDataSlaveImpl::getRemoteCensusDataProvidersFiber(InstanceId instanceId)
    {
        // query remote instance for list of census data providers
        CensusDataComponentIds localProviderComponentIds;
        gCensusDataManager->getCensusDataProviderComponentIds(localProviderComponentIds);

        GetCensusDataProvidersResponse resp;
        RpcCallOptions opts;
        opts.routeTo.setInstanceId(instanceId);
        
        BlazeRpcError err = gController->getCensusDataProviders(resp, opts);
        if (err != Blaze::ERR_OK)
        {
            WARN_LOG("[CensusDataSlaveImpl].getRemoteCensusDataProvidersFiber(): could not get remote census data provider component ids: " << (ErrorHelp::getErrorName(err))
                << " for instance with id " << instanceId);
            return;
        }

        for (CensusDataComponentIds::const_iterator it = resp.getComponentIds().begin(), itEnd = resp.getComponentIds().end();
             it != itEnd;
             it++)
        {
            if (eastl::find(localProviderComponentIds.begin(), localProviderComponentIds.end(), *it) == localProviderComponentIds.end())
            {
                mRemoteProviderComponentIds.insert(*it);
            }
        }
    }

} // CensusData
} // Blaze
