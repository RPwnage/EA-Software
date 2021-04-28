/*! ************************************************************************************************/
/*!
    \file reputationserviceutil.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef COMPONENT_REPUTATION_SERVICE_UTIL_H
#define COMPONENT_REPUTATION_SERVICE_UTIL_H

#include "blazerpcerrors.h"
#include "framework/usersessions/extendeddataprovider.h"
#include "framework/util/expression.h"
#include "framework/tdf/usersessiontypes_server.h"

namespace Blaze
{
    namespace ReputationService
    {
        const uint16_t EXTENDED_DATA_REPUTATION_VALUE_KEY = 1;

        class ReputationServiceUtil : public ExtendedDataProvider
        {
        public:
            ReputationServiceUtil() : mRefCount(0) {};
            ~ReputationServiceUtil() override {};

            /*! ************************************************************************************************/
            /*!
                \brief updates reputation for the given user session (calls updateReputation internally)

                \param[in] userSessionId - id of the user session to update reputation for
                \returns true if any user had a poor reputation value
            *************************************************************************************************/
            bool doSessionReputationUpdate(UserSessionId userSessionId) const;

            /*! ************************************************************************************************/
            /*!
                \brief updates reputation for the given user session list

                \param[in] userSessionIdList - list of user sessions to update reputation for
                \returns true if any user had a poor reputation value
            *************************************************************************************************/
            virtual bool doReputationUpdate(const UserSessionIdList &userSessionIdList, const UserIdentificationList& externalUserList) const;

            // returns true if the given user is a has a good reputation, based on UED and the configured goodReputationValidationExpression and reputationUEDName
            static bool userHasPoorReputation(const UserSessionExtendedData& userExtendedData);
            static bool userHasPoorReputation(const UserExtendedDataMap& userExtendedDataMap);

            // extended data provider interface
            BlazeRpcError onLoadExtendedData(const UserInfoData& data, UserSessionExtendedData &extendedData) override;
            BlazeRpcError onRefreshExtendedData(const UserInfoData& data, UserSessionExtendedData &extendedData) override;

            bool isMock() const;
        protected:

            static bool evaluateReputation(const UserExtendedDataValue &value);

        private:

            struct ResolveReputationInfo
            {
                const UserExtendedDataValue *reputationValue;
                bool* success; // using ptr to get around const void* param because this indicates if the resolve succeeded or not
            };

            /*! ************************************************************************************************/
            /*!
                \brief resolves criteria variable during an evaluateReputation.

                \param[in] nameSpace Currently unused but defined to match the ResolveVariableCb definition
                \param[in] name The name of the variable, should always be "reputationValue"
                \param[in] type The value type, ie. int.
                \param[in] context Entry Criteria specific context, which resolves to ResolveCriteriaInfo*.
                \param[out] val The reputation value to evaluate.
            *************************************************************************************************/
            static void resolveReputationVariable(const char8_t* nameSpace, const char8_t* name, Blaze::ExpressionValueType type, const void* context, Blaze::Expression::ExpressionVariableVal& val);


            /*! ************************************************************************************************/
            /*!
                \brief retrieves the reputation and updates the UED of the given UserSession if the value has changed

                \param[in] userInfoData - id information for the target user
                \param[out] userExtendedData - extended data for the target user
                \param[out] hasPoorReputation - set to true if the user has poor reputation
                \return ERR_OK if the update succeeded
            *************************************************************************************************/
            virtual BlazeRpcError updateReputation(const UserInfoData &userInfoData, const char8_t* serviceName, UserSessionExtendedData &userExtendedData, bool& hasPoorReputation) const;

            uint32_t mRefCount;

            friend void intrusive_ptr_add_ref(ReputationServiceUtil* ptr);
            friend void intrusive_ptr_release(ReputationServiceUtil* ptr);
        };

        inline void intrusive_ptr_add_ref(ReputationServiceUtil* ptr)
        {
            ptr->mRefCount++;
        }

        inline void intrusive_ptr_release(ReputationServiceUtil* ptr)
        {
            if (--ptr->mRefCount == 0)
                delete ptr;
        }
    }
}

#endif
