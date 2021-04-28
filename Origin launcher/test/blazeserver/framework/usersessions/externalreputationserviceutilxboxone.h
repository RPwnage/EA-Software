/*! ************************************************************************************************/
/*!
    \file externalreputationserviceutilxboxone.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef COMPONENT_EXTERNAL_REPUTATION_SERVICE_UTIL_H
#define COMPONENT_EXTERNAL_REPUTATION_SERVICE_UTIL_H

#include "blazerpcerrors.h"
#include "framework/usersessions/reputationserviceutil.h"
#include "framework/usersessions/usersessionmanager.h"
#include "framework/tdf/usersessiontypes_server.h"

namespace Blaze
{
    namespace ReputationService
    {

        class ExternalReputationServiceUtilXboxOne : public ReputationServiceUtil
        {
        public:
            ExternalReputationServiceUtilXboxOne() {};
            ~ExternalReputationServiceUtilXboxOne() override {};

            /*! ************************************************************************************************/
            /*!
                \brief updates reputation for the given user session list
    
                \param[in] userSessionIdList - list of user sessions to update reputation for
                \returns true if any user had a poor reputation value
            *************************************************************************************************/
            bool doReputationUpdate(const UserSessionIdList &userSessionIdList, const UserIdentificationList& externalUserList) const override;

        private:

            /*! ************************************************************************************************/
            /*!
                \brief retrieves the reputation and updates the UED of the given UserSession if the value has changed

                \param[in] userSession - the user session to run a reputation update on
                \param[out] hasPoorReputation - set to true if the user has poor reputation
                \return ERR_OK if the update succeeded
            *************************************************************************************************/
            BlazeRpcError updateReputation(const UserInfoData &userInfoData, const char8_t* serviceName, UserSessionExtendedData &userExtendedData, bool& hasPoorReputation) const override;

            struct ReputationUpdateStruct
            {
                ReputationUpdateStruct() :
                    blazeId(INVALID_BLAZE_ID),
                    oldRepValue(false),
                    newRepValue(false)
                {}

                BlazeId blazeId;
                bool oldRepValue;
                bool newRepValue;
            };
            typedef eastl::map<ExternalId, ReputationUpdateStruct> ReputationUpdateMap;

            BlazeRpcError getBatchReputationByExternalIds(ExternalId validExternalId, ReputationUpdateMap& repUpdateMap, bool& overallHasPoorReputation) const;
            void updateReputations(ReputationUpdateMap& repUpdateMap) const;
        };
    }
}

#endif //COMPONENT_EXTERNAL_REPUTATION_SERVICE_UTIL_H 
