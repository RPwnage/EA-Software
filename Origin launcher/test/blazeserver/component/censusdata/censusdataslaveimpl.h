/*************************************************************************************************/
/*!
\file   censusdataslaveimpl.h


\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_CENSUSDATA_SLAVEIMPL_H
#define BLAZE_CENSUSDATA_SLAVEIMPL_H

/*** Include files ******************************************************************************/
#include "framework/usersessions/usersessionmanager.h" // for UserSessionSubscriber & UserSession
#include "censusdata/rpc/censusdataslave_stub.h"
#include "framework/replication/replicationcallback.h"
#include "censusdata/tdf/censusdata.h"
#include "EASTL/set.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/
namespace Blaze
{

    namespace CensusData
    {
        class CensusDataSlaveImpl :
            public CensusDataSlaveStub,
            private UserSessionSubscriber,
            private LocalUserSessionSubscriber,
            private Controller::RemoteServerInstanceListener,
            private Controller::DrainListener
        {
        public:
            CensusDataSlaveImpl();
            ~CensusDataSlaveImpl() override;

        public:

            ///////////////////////////////////////////////////////////////////////////
            // UserSessionSubscriber interface functions
            //
            void onUserSessionExistence(const UserSession& userSession) override;
            void onUserSessionMutation(const UserSession& userSession, const UserSessionExistenceData& newExistenceData) override;
            void onUserSessionExtinction(const UserSession& userSession) override; 

            ///////////////////////////////////////////////////////////////////////////
            // LocalUserSessionSubscriber interface functions
            //
            void onLocalUserSessionLogout(const UserSessionMaster& userSession) override;

            /* \brief Fetch remote census data */
            void fetchRemoteCensusData(Blaze::CensusDataByComponent &outCensusData);

        private:
            bool onConfigure() override;
            bool onResolve() override;
            bool onActivate() override;
            void onShutdown() override;
            bool onValidateConfig(CensusDataConfig& config, const CensusDataConfig* referenceConfigPtr, ConfigureValidationErrors& validationErrors) const override;

            void notificationLoop();

            // slave-only commands
            SubscribeToCensusDataError::Error processSubscribeToCensusData(const Message *message) override;
            SubscribeToCensusDataUpdatesError::Error processSubscribeToCensusDataUpdates(const Blaze::CensusData::SubscribeToCensusDataUpdatesRequest &request, Blaze::CensusData::SubscribeToCensusDataUpdatesResponse &response, const Message *message) override;
            UnsubscribeFromCensusDataError::Error processUnsubscribeFromCensusData(const Message *message) override;
            GetRegionCountsError::Error processGetRegionCounts(Blaze::CensusData::RegionCounts &response, const Message* message) override;

            bool subscribeToCensusDataInternal();

            // Controller::RemoteServerInstanceListener
            void onRemoteInstanceChanged(const RemoteServerInstance& instance) override;

            // Controller::DrainListener
            void onControllerDrain() override;

            void getRemoteCensusDataProvidersFiber(InstanceId instanceId);

        private:
            NotifyServerCensusDataPtr mComponentCensusData;

            TimerId mIdleTimerId;

            Blaze::TimeValue mLastNotificationTimestamp;

            TimerId mTimerId;
            Fiber::EventHandle mDrainEventHandle;

            typedef eastl::set<UserSessionId> UserSessionIdSet;
            UserSessionIdSet mUserSessionSet;
            UserSessionIdSet::iterator mLastNotifySessionIter;

            typedef eastl::set<ComponentId> ComponentIdSet;
            ComponentIdSet mRemoteProviderComponentIds;
        };

    } // CensusData
} // Blaze

#endif // BLAZE_CENSUSDATA_SLAVEIMPL_H

