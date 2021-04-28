/*************************************************************************************************/
/*!
\file censusdataapi.h


\attention
(c) Electronic Arts. All Rights Reserved.
*************************************************************************************************/

#ifndef BLAZE_CENSUSSESSIOINAPI_H
#define BLAZE_CENSUSSESSIOINAPI_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

/*** Include Files *******************************************************************************/
#include "BlazeSDK/blazesdk.h"
#include "BlazeSDK/api.h"
#include "BlazeSDK/callback.h"
#include "BlazeSDK/dispatcher.h"
#include "BlazeSDK/blazeerrors.h"
#include "BlazeSDK/blaze_eastl/vector_map.h"
#include "BlazeSDK/component/censusdata/tdf/censusdata.h"
#include "BlazeSDK/component/censusdatacomponent.h"
#include "BlazeSDK/blaze_eastl/vector_map.h"
#include "BlazeSDK/usermanager/userlistener.h"

namespace Blaze
{
    class BlazeHub;

namespace CensusData
{
    class CensusDataAPIListener;

    /*! **************************************************************************************************************/
    /*! 
    \class CensusDataAPI
    \ingroup _mod_apis

    \brief
    CensusDataAPI is responsible for receiving server census information periodically. Clients who would like to get 
    census information needs to subscribe through subscribeToCensusData and unsubscribe through unsubscribeFromCensusData
 
    \details
    After receiving the data through onNotifyServerCensusData from server, CensusDataAPI will dispatch the information to
    below two listeners in order.
    a. CensusConsumerListener - Any component which has census data, eg. GamemanagerAPI, should add itself
                                to this listener, census data belongs to each component will be parsed and cached in each 
                                individual component
    b. CensusDataAPIListener -  After preceding dispatch, census information is ready in each individual component, client
                                who would like to get the parsed census information need to listen to this dispatch
    
    NOTE: CensusDataAPI does not directly provide census information to clients, it will pass the information sent from server to
    each individual component which registers as a listener of CensusConsumerListener, each component will be responsible for
    parsing and caching the census information belongs to the component, client will get detailed census infomation from each 
    individual component like GameManagerAPI after receiving CensusDataAPIListener dispatch.

    Since consumer component which has census data like GameManagerAPI need to listen to the CensusConsumerListener
    dispatch from CensusDataAPI, there is dependency between these components. To be able to retrieve census information from 
    consumer components, clients need to make sure the CensusDataAPI is valid.
    
    \section Capabilities

    CensusDataAPI provides these capabilities:
    \li Subscribe / Unsubscribe to CensusData
    \li Dispatch CensusConsumerListener to other components which has census information
    \li Dispatch CensusDataAPIListener to clients who would like to get census information
    ******************************************************************************************************************/
    class BLAZESDK_API CensusDataAPI : public SingletonAPI, public UserManager::PrimaryLocalUserListener
    {
    public:

            /*! ****************************************************************************/
            /*! \brief Creates the CensusDataAPI.

                \param hub The hub to create the API on.
                \param allocator pointer to the optional allocator to be used for the component; will use default if not specified. 
            ********************************************************************************/
            static void createAPI(BlazeHub &hub, EA::Allocator::ICoreAllocator* allocator = nullptr);

            /*! ************************************************************************************************/
            /*! \brief The callback functor dispatched on completion of a subscribeToCensusData request.
                
                \param[in] BlazeError the error code for the subscribeToCensusData attempt
            ***************************************************************************************************/
            typedef Functor2<BlazeError, JobId> SubscribeToCensusDataJobCb;
            typedef Functor1<BlazeError> SubscribeToCensusDataCb; // DEPRECATED

            /*! ****************************************************************************/
            /*! \brief subscribe to server census data, if succeeded, server will send
                 census data periodically 
            ********************************************************************************/
            JobId subscribeToCensusData(const SubscribeToCensusDataJobCb &resultFunctor);
            JobId subscribeToCensusData(const SubscribeToCensusDataCb &resultFunctor);     // DEPRECATED

            /*! ************************************************************************************************/
            /*! \brief The callback functor dispatched on completion of a subscribeToCensusData request.

                \param[in] BlazeError the error code for the subscribeToCensusData attempt
            ***************************************************************************************************/
            typedef Functor2<BlazeError, JobId> UnsubscribeFromCensusDataJobCb;
            typedef Functor1<BlazeError> UnsubscribeFromCensusDataCb; // DEPRECATED

            /*! ****************************************************************************/
            /*! \brief subscribe to server census data, if succeeded, server will send
                 census data periodically 
            ********************************************************************************/
            JobId unsubscribeFromCensusData(const UnsubscribeFromCensusDataJobCb &resultFunctor);
            JobId unsubscribeFromCensusData(const UnsubscribeFromCensusDataCb &resultFunctor);     // DEPRECATED

            /*! ****************************************************************************/
            /*! \brief get regions from server census data, if succeeded, server will send
                 a region map
            ********************************************************************************/
            JobId getRegionCounts(const CensusDataComponent::GetRegionCountsCb &callback);
    
        /*! ****************************************************************************/
        /*! \name CensusDataAPIListener methods
        ********************************************************************************/

            /*! ****************************************************************************/
            /*! \brief Adds a listener to receive async notifications from the CensusDataAPI.

                Once an object adds itself as a listener, CensusDataAPI will dispatch
                async events to the object via the methods in CensusDataAPIListener.

                \param listener The listener to add.
            ********************************************************************************/
            void addListener(CensusDataAPIListener *listener) 
            { 
                mDispatcher.addDispatchee( listener ); 
            }

            /*! ****************************************************************************/
            /*! \brief Removes a listener from the CensusDataAPI, preventing any further async
                notifications from being dispatched to the listener.

                \param listener The listener to remove.
            ********************************************************************************/
            void removeListener(CensusDataAPIListener *listener)
            { 
                mDispatcher.removeDispatchee( listener);
            }

            /*! ****************************************************************************/
            /*! \brief return the censusData

            \param id of the tdf in request
            ********************************************************************************/
            NotifyServerCensusDataItem* getCensusData(EA::TDF::TdfId id) { return mCensusDataByTdfId[id]; }

            /*! ****************************************************************************/
            /*! \brief get latest census data from server
            
            \param callback Callback for clients
            \return JobId of task.
            ********************************************************************************/
            JobId getLatestCensusData(const CensusDataComponent::GetLatestCensusDataCb &callback);

        private:
            NON_COPYABLE(CensusDataAPI);

            /*! ****************************************************************************/
            /*! \brief Constructor.

                Private as all construction should happen via the factory method.
            ********************************************************************************/
            CensusDataAPI(BlazeHub &blazeHub);
            virtual ~CensusDataAPI();

        private:
            /*! ****************************************************************************/
            /*! \brief register the listener to listen notification from server
            ********************************************************************************/
            void registerEventHandlers();

            /*! ****************************************************************************/
            /*! \brief clear the listener to listen notification from server
            ********************************************************************************/
            void clearEventHandlers();

            /*! ****************************************************************************/
            /*! \brief receive the census data from the server,dispatch the notification to
                consumer listener and api listener

                \param[in]notification - notification got from server
            ********************************************************************************/
            void onNotifyServerCensusData(const NotifyServerCensusData* notification, uint32_t userIndex);

            void helperCallback(BlazeError err, JobId jobId);

            void onSubscribeCensusDataCb(const SubscribeToCensusDataUpdatesResponse *subscribeToCensusDataUpdatesResponse, BlazeError err, JobId jobid, const SubscribeToCensusDataJobCb resultCb);
            void onUnsubscribeCensusDataCb(BlazeError err, JobId jobid, const UnsubscribeFromCensusDataJobCb resultCb);
            void onResubscribeCensusDataCb(const SubscribeToCensusDataUpdatesResponse *subscribeToCensusDataUpdatesResponse, BlazeError err, JobId jobid);

            void resubscribeToCensusData();

            //! \cond INTERNAL_DOCS
            /*! ****************************************************************************/
            /*! \brief Logs any initialization parameters for this API to the log.
            ********************************************************************************/
            void logStartupParameters() const;

            /*! ************************************************************************************************/
            /*! \brief Notification that a local user has been authenticated against the blaze server.  
                The provided user index indicates which local user authenticated.  If multiple local users
                authenticate, this will trigger once per user.
            
                \param[in] userIndex - the user index of the local user that authenticated.
            ***************************************************************************************************/
            virtual void onAuthenticated(uint32_t userIndex);

            /*! ************************************************************************************************/
            /*! \brief Notification that a local user has lost authentication (logged out) of the blaze server
                The provided user index indicates which local user  lost authentication.  If multiple local users
                loose authentication, this will trigger once per user.

                \param[in  userIndex - the user index of the local user that authenticated.
            ***************************************************************************************************/
            virtual void onDeAuthenticated(uint32_t userIndex);

            virtual void onPrimaryLocalUserChanged(uint32_t userIndex);

            typedef Dispatcher<CensusDataAPIListener> CensusDataAPIDispatcher;
            //! \endcond
  
        private:
            CensusDataComponent* mComponent;
            CensusDataAPIDispatcher mDispatcher;
            typedef Blaze::vector_map<EA::TDF::TdfId, NotifyServerCensusDataItemPtr> TdfByTdfId;
            TdfByTdfId mCensusDataByTdfId;
            TimeValue mResubscribeTimeout;
            JobId mResubscribeJobId;
            bool mAttemptResubscribe;
        };

        /*! ****************************************************************************/
        /*!
            \class CensusDataAPIListener

            \brief classes that wish to receive async notifications about CensusDataAPI
              events implement this interface, then add themselves as listeners.

            Listeners must add themselves to each CensusDataAPI that they wish to get notifications
            about.  See CensusDataAPI::addListener.
        ********************************************************************************/
        class CensusDataAPIListener
        {
        public:
            virtual ~CensusDataAPIListener() {};
            /*! ****************************************************************************/
            /*! \brief Notifies when a server's census data gets updated and is ready to be 
                 retrieved through component API accessors (e.g. getCensusData()) on each 
                 census data-providing component.
            ********************************************************************************/
            virtual void onNotifyCensusData() = 0;
        };
    } // namespace CensusData

} // namespace Blaze

#endif // BLAZE_CENSUSSESSIOINAPI_H

