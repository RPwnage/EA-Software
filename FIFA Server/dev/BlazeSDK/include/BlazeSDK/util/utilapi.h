/*! ************************************************************************************************/
/*!
    \file utilapi.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/
#ifndef BLAZE_UTIL_API
#define BLAZE_UTIL_API
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

#include "BlazeSDK/blazesdk.h"
#include "BlazeSDK/api.h"
#include "BlazeSDK/jobid.h"
#include "BlazeSDK/blazetypes.h"
#include "BlazeSDK/usermanager/userlistener.h"

#include "DirtySDK/voip/voip.h"

//forward declarations
struct ConnApiRefT;
struct ProtoTunnelRefT;
struct VoipTunnelRefT;
struct DirtySessionManagerRefT;
struct QosClientRefT;
struct ProtoHttpRefT;
struct ProtoSSLRefT;

namespace Blaze
{
    namespace BlazeNetworkAdapter
    {
        class ConnApiAdapterConfig;
    }

    namespace Util
    {
        class FilterUserTextResponse;
        class UserStringList;
        class UtilComponent;

        enum ProfanityFilterType
        {
            FILTER_CLIENT_SERVER = 0, /// filter will be performed on the client first if available, then the server
            FILTER_CLIENT_ONLY = 1 /// filter will be performed on the client only, if not available input copied to output
        };

        /*! ****************************************************************************/
        /*! \class UtilAPI

            \ingroup _mod_apis
            \brief API for miscellaneous helpers and utilities                
            ********************************************************************************/
        class BLAZESDK_API UtilAPI: public SingletonAPI, public UserManager::PrimaryLocalUserListener
        {
        private:
            NON_COPYABLE(UtilAPI);
        public:
            static const uint32_t PROFANITY_FILTER_TIMEOUT_MS = 5000;
            static const uint32_t DEFAULT_CLIENT_USER_METRICS_RATE = 60000;
            static const unsigned int PROFANITY_FILTER_MAX_STRINGS_PER_BATCH = 10;
            static const unsigned int PROFANITY_FILTER_MAX_NUMBER_OF_BATCHES = 10;
            static const unsigned int PROFANITY_FILTER_MAX_NUMBER_OF_FILTER_STRINGS = PROFANITY_FILTER_MAX_STRINGS_PER_BATCH * PROFANITY_FILTER_MAX_NUMBER_OF_BATCHES;

            /*! ****************************************************************************/
            /*! \name API types
            ********************************************************************************/

            /*! ****************************************************************************/
            /*! \brief Creates the API

                \param hub The hub to create the API on.
                \param allocator pointer to the optional allocator to be used for the component; will use default if not specified. 
            ********************************************************************************/
            static void createAPI(BlazeHub &hub, EA::Allocator::ICoreAllocator* allocator = nullptr);

            //! \cond INTERNAL_DOCS
            /*! ****************************************************************************/
            /*! \brief Logs any initialization parameters for this API to the log.
            ********************************************************************************/
            virtual void logStartupParameters() const;
            //! \endcond INTERNAL_DOCS

            typedef Functor3<BlazeError /*error*/, JobId /*jobId*/, const FilterUserTextResponse* > FilterUserTextCb;

            /*! ****************************************************************************/
            /*! \brief Filter user supplied text for profanity

                \param textlist     the text(s) to be filtered (Xbox only: list size must by < 10)
                \param filterCb     filtering status for each of the text supplied and sanitized text (*** out)

                \param filterType   
                            FILTER_CLIENT_SERVER: 
                            filter will be performed on the client first if available, then the server
                            FILTER_CLIENT_ONLY:
                            filter will be performed on the client only, if not available input copied to output
                            (currently only available to XOne, no-op on others). For use by component writer 
                            which further filtering will be handled in the server side component.

                \param timeoutMS    client side timeout in milliseconds. Defaults to PROFANITY_FILTER_TIMEOUT_MS. 
                \return             JobId - for use when canceling join request
            ********************************************************************************/
            JobId filterUserText(const UserStringList& textlist, const FilterUserTextCb& filterCb, ProfanityFilterType filterType = FILTER_CLIENT_SERVER, uint32_t timeoutMS = PROFANITY_FILTER_TIMEOUT_MS);

            /*! ****************************************************************************/
            /*! \brief Overrides the settings of the passed in struct, based on settings found in util.cfg

                \param dest target struct to receive the setting overrides
                \param instanceName identifier to distinguish between two different setting sets, for instances of the same object type. 
            ********************************************************************************/
            void OverrideConfigs(ProtoHttpRefT * dest, const char8_t * instanceName);
            void OverrideConfigs(ProtoSSLRefT * dest);
            void OverrideConfigs(Blaze::BlazeNetworkAdapter::ConnApiAdapterConfig * dest);
            void OverrideConfigs(ConnApiRefT * dest);
            void OverrideConfigs(ProtoTunnelRefT * dest);
            void OverrideConfigs(VoipTunnelRefT * dest);
            void OverrideConfigs(DirtySessionManagerRefT * dest);
            void OverrideConfigs(QosClientRefT * dest);
            
        protected:            
            
            /*! ************************************************************************************************/
            /*! \brief The BlazeSDK has lost connection to the blaze server.  This is user independent, as
                all local users share the same connection.
            ****************************************************************************************************/
            virtual void onAuthenticated(uint32_t userIndex);
            virtual void onDisconnected(BlazeError errorCode);

        private:
            friend class XboxOneValidateString;
            friend class FilterUserTextJob;


            //! tts/stt state per user
            typedef struct
            {
                VoipSpeechToTextMetricsT PreviousSentSTTMetrics;
                VoipTextToSpeechMetricsT PreviousSentTTSMetrics;
            } UserAccessabilityStateT;

            //filter text
            void internalFilterUserTextCb(const FilterUserTextResponse *res, BlazeError err, JobId rpcJobId, JobId jobId);
            static void initFilterUserTextRequest(const UserStringList &userStringList, FilterUserTextResponse &request);
            void clientUserMetricsJob();
            VoipSpeechToTextMetricsT getVoipSpeechToTextMetrics(uint32_t userIndex);
            VoipTextToSpeechMetricsT getVoipTextToSpeechMetrics(uint32_t userIndex);
            
            // config override
            bool parseConfigOverride(const char * moduleTag, const char8_t * instanceName, const char8_t *key, char8_t *value, int32_t *selector, uint32_t *tokenCount, char8_t **token0, char8_t **token1, char8_t **token2);

            virtual void onPrimaryLocalUserChanged(uint32_t userIndex);

            UtilAPI(BlazeHub &blazeHub);
            virtual ~UtilAPI();

            Blaze::Util::UtilComponent* mComponent;
            JobId mClientUserMetricsJobId;
            UserAccessabilityStateT mUsersAccessabilityState[VOIP_MAXLOCALUSERS];
        };
    }   // Util
}
// Blaze
#endif
