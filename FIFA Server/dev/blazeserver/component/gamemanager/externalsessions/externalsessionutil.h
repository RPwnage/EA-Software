/*! ************************************************************************************************/
/*!
    \file externalsessionutil.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_GAMEMANAGER_EXTERNAL_SESSION_UTIL_H
#define BLAZE_GAMEMANAGER_EXTERNAL_SESSION_UTIL_H

#include "framework/tdf/userdefines.h"
#include "EASTL/string.h"
#include "gamemanager/tdf/externalsessiontypes_server.h" 
#include "gamemanager/tdf/externalsessionconfig_server.h" 
#include "component/gamemanager/rpc/gamemanager_defines.h"
#include "framework/controller/controller.h"
#include "gamemanager/externalsessions/externalsessionmemberstrackinghelper.h"
#include "gamemanager/externalsessions/externalsessionscommoninfo.h"

namespace Blaze
{
    class ExternalMemberInfo;
    class UserSessionExistenceData;
    class UserIdentification;
    class UserSessionInfo;

    namespace GameManager
    {
        class GameSessionMaster; 
        class GameManagerSlaveImpl; 
        class GameManagerMasterImpl;
        class CreateGameRequest;
        class ExternalSessionUtilManager;


        class ExternalSessionUtil
        {
            NON_COPYABLE(ExternalSessionUtil);
            public:
                ExternalSessionUtil(const ExternalSessionServerConfig& config, const TimeValue& reservationTimeout) :
                    mReservationTimeout(reservationTimeout)
                {
                    config.copyInto(mConfig);
                }
                virtual ~ExternalSessionUtil(){}

                /*! ************************************************************************************************/
                /*! \brief Gets and stores a user authentication token to buffer, for making the external session requests
                    \param[in] serviceName the user session's service name if available, or else the default service name.
                ***************************************************************************************************/
                virtual Blaze::BlazeRpcError getAuthToken(const UserIdentification& ident, const char8_t* serviceName, eastl::string& buf)
                {
                    return ERR_OK;
                }

                /*! ************************************************************************************************/
                /*! \brief (DEPRECATED: future implns use updatePresenceForUser()) creates the external session.
                    Pre: input user has been checked for external sessions update permissions as needed
                    \param[in] commit - if supported, whether to actually commit the changes to the external session service.
                ***************************************************************************************************/
                virtual Blaze::BlazeRpcError create(const CreateExternalSessionParameters& createExternalSessionParameters, Blaze::ExternalSessionResult* result, bool commit)
                {
                    return ERR_OK;
                }

                /*! ************************************************************************************************/
                /*! \brief (DEPRECATED: future implns use updatePresenceForUser()) join or update the members in the external session. creates external session if it does not yet exist.
                    Pre: input users have been checked for external sessions update permissions as needed
                ***************************************************************************************************/
                virtual Blaze::BlazeRpcError join  (const JoinExternalSessionParameters& joinExternalSessionParameters, Blaze::ExternalSessionResult* result, bool commit)
                {
                    return ERR_OK;
                }

                /*!************************************************************************************************/
                /*! \brief (DEPRECATED: future implns use updatePresenceForUser()) leaves a group of players from the external session.
                ***************************************************************************************************/
                virtual Blaze::BlazeRpcError leave(const LeaveGroupExternalSessionParameters& params)
                {
                    return ERR_OK;
                }

                /*!************************************************************************************************/
                /*! \brief get list of the external session's members.
                ***************************************************************************************************/
                virtual Blaze::BlazeRpcError getMembers(const ExternalSessionIdentification& sessionIdentification, ExternalIdList& memberExternalIds)
                {
                    return ERR_OK;
                }

                /*! ************************************************************************************************/
                /*! \brief return whether user has permission to be joined into external sessions
                ***************************************************************************************************/
                static bool hasJoinExternalSessionPermission(const Blaze::UserSessionExistenceData& userSession);

                /*! ************************************************************************************************/
                /*! \brief return whether there are changes to be propagated to the external session.
                ***************************************************************************************************/
                virtual bool isUpdateRequired(const ExternalSessionUpdateInfo& origValues, const ExternalSessionUpdateInfo& newValues, const ExternalSessionUpdateEventContext& context)
                {
                    return false;
                }

                /*!************************************************************************************************/
                /*! \brief updates the external session's properties.
                ***************************************************************************************************/
                virtual Blaze::BlazeRpcError update(const UpdateExternalSessionPropertiesParameters& params)
                {
                    return ERR_OK;
                }

                /*!************************************************************************************************/
                /*! \brief updates the external session's image. side: done separate from update() for efficiency.
                ***************************************************************************************************/
                virtual Blaze::BlazeRpcError updateImage(const UpdateExternalSessionImageParameters& params, UpdateExternalSessionImageErrorResult& errorResult)
                {
                    return ERR_OK;
                }

                typedef eastl::hash_set<BlazeId> BlazeIdSet;

                /*! ************************************************************************************************/
                /*! \brief If update requires a specific member to be the caller, pick the member. Otherwise, no op.
                    \param[in] updatePropertiesParams Update properties parameters, which may be used for determining updater, and for efficiency whether update still needed.
                    \return GAMEMANAGER_ERR_PLAYER_NOT_FOUND on failure or if update not required, otherwise ERR_OK.
                ***************************************************************************************************/
                virtual Blaze::BlazeRpcError getUpdater(const ExternalMemberInfoListByActivityTypeInfo& possibleUpdaters, ExternalMemberInfo& updater, BlazeIdSet& avoid, const UpdateExternalSessionPropertiesParameters* updatePropertiesParams = nullptr)
                {
                    return ERR_OK;
                }

                /*! ************************************************************************************************/
                /*! \brief Helper providing basic implementation getUpdater() overrides may call. Finds next member with S2S user token from possibleUpdaters, not in avoid.
                ***************************************************************************************************/
                static BlazeRpcError getUpdaterWithUserToken(ClientPlatformType platform, const ExternalMemberInfoList& possibleUpdaters, ExternalMemberInfo& updater, BlazeIdSet& avoid, bool addToAvoid = true);


                /*!************************************************************************************************/
                /*! \brief return whether post-creation PresenceMode updating is supported for the platform.
                ***************************************************************************************************/
                virtual bool isUpdatePresenceModeSupported()
                {
                    return true;
                }

                /*! ************************************************************************************************/
                /*! \brief checks whether the members in the request pass the external session's join restrictions.
                ***************************************************************************************************/
                virtual Blaze::BlazeRpcError checkRestrictions(const CheckExternalSessionRestrictionsParameters& params)
                {
                    return ERR_OK;
                }

                /*! ************************************************************************************************/
                /*! \brief return whether friends only join restrictions for external sessions is checked by checkRestrictions
                ***************************************************************************************************/
                virtual bool isFriendsOnlyRestrictionsChecked()
                {
                    // implementers should return true only if checkRestrictions checks friends restrictions
                    return false;
                }

                /*!************************************************************************************************/
                /*! \brief return whether the game's external session should be treated as full.
                    \param[out] maxUsers The capacity shown for the external session in the shell UX
                ***************************************************************************************************/
                bool isFullForExternalSession(const GameSessionMaster& gameSession, uint16_t& maxUsers);


                //// Primary External Sessions

                /*!************************************************************************************************/
                /*! \brief sets the external session as the primary one for the user.
                ***************************************************************************************************/
                virtual Blaze::BlazeRpcError setPrimary(const SetPrimaryExternalSessionParameters& params, SetPrimaryExternalSessionResult& result, SetPrimaryExternalSessionErrorResult* errorResult)
                {
                    return ERR_OK;
                }

                /*!************************************************************************************************/
                /*! \brief clears the external session from being the primary for the user.
                ***************************************************************************************************/
                virtual Blaze::BlazeRpcError clearPrimary(const ClearPrimaryExternalSessionParameters& params, ClearPrimaryExternalSessionErrorResult* errorResult)
                {
                    return ERR_OK;
                }

                /*!************************************************************************************************/
                /*! \brief updates user's game presence. Default implementation calls:
                        updatePrimaryPresence(), followed by updateNonPrimaryPresence()
                ***************************************************************************************************/
                virtual Blaze::BlazeRpcError updatePresenceForUser(const UpdateExternalSessionPresenceForUserParameters& params, UpdateExternalSessionPresenceForUserResponse& result, UpdateExternalSessionPresenceForUserErrorInfo& errorResult);

                /*!************************************************************************************************/
                /*! \brief determines and updates the primary external session for the user.
                ***************************************************************************************************/
                Blaze::BlazeRpcError updatePrimaryPresence(const UpdatePrimaryExternalSessionForUserParameters& params, UpdatePrimaryExternalSessionForUserResult& result, UpdatePrimaryExternalSessionForUserErrorResult& errorResult, const UserSessionPtr userSession);

                /*!************************************************************************************************/
                /*! \brief Used to handle presence for activity types not done in updatePrimaryPresence, for the user.
                    \param[in,out] result On success, this method should update result.getUpdatedSession() as appropriate.
                    \param[in,out] errorResult On failure, this method should update errorResult.getErrorInfo() as appropriate.
                ***************************************************************************************************/
                virtual Blaze::BlazeRpcError updateNonPrimaryPresence(const UpdateExternalSessionPresenceForUserParameters& params, UpdateExternalSessionPresenceForUserResponse& result, UpdateExternalSessionPresenceForUserErrorInfo& errorResult)
                {
                    return ERR_OK;
                }

                /*!************************************************************************************************/
                /*! \brief return whether user session should update the primary external session for user.
                ***************************************************************************************************/
                virtual bool hasUpdatePrimaryExternalSessionPermission(const UserSessionExistenceData& userSession)
                {
                    return false;
                }

                /*! ************************************************************************************************/
                /*! \brief (non blocking) clear/prep external session ids, tracked member lists etc. on the GameSessionMaster,
                        for the next game replay round.
                ***************************************************************************************************/
                virtual void prepForReplay(GameSessionMaster& gameSession)
                {
                }

                /*! ************************************************************************************************/
                /*! \brief Do any external session specific updates and validations for the request to create or join game.
                    \param[in,out] request Request to validate/update. Pre: template attributes resolved and populated.
                ***************************************************************************************************/
                virtual Blaze::BlazeRpcError prepForCreateOrJoin(CreateGameRequest& request)
                {
                    return ERR_OK;
                }


                /*! ************************************************************************************************/
                /*! \brief creates the external session utils. Returns false on error.
                    \param[out] utilOut - The util filled in by this function
                    \param[in] reservationTimeout - external session member reservations timeout.
                    \param[in] inactivityTimeout - external session member inactivity timeout.
                    \param[in] readyTimeout - external session member ready timeout
                    \param[in] sessionEmptyTimeout - external session empty timeout
                    \param[in] maxMemberCount - cap on number of members in external sessions. UINT16_MAX omits specifying.
                    \param[in] teamNameByTeamIdMap - external sessions' team names to set by Blaze TeamIds.
                    \param[in] gameManagerSlave - owning GameManagerSlave, if the util is owned by one. nullptr otherwise.
                ***************************************************************************************************/
                static bool createExternalSessionServices(ExternalSessionUtilManager& utilOut, const ExternalSessionServerConfigMap& config, const TimeValue& reservationTimeout, uint16_t maxMemberCount = UINT16_MAX, const TeamNameByTeamIdMap* teamNameByTeamIdMap = nullptr, GameManagerSlaveImpl* gameManagerSlave = nullptr);


                /*!************************************************************************************************/
                /*! \brief validate the external session config before reconfiguration
                ***************************************************************************************************/
                virtual bool onPrepareForReconfigure(const ExternalSessionServerConfig& config) { return true; }

                /*!************************************************************************************************/
                /*! \brief reconfigure the external session util.
                ***************************************************************************************************/
                void onReconfigure(const ExternalSessionServerConfig& config, const TimeValue reservationTimeout);

                bool isMockPlatformEnabled() const { return (mConfig.getUseMock() || gController->getUseMockConsoleServices()); }

                /*! \brief return the external session name for the given game id */
                BlazeRpcError getNextGameExternalSessionName(eastl::string& result) const;

                /*! \brief return the external session name for the given matchmaking session id */
                void getExternalMatchmakingSessionName(eastl::string& result, Blaze::GameManager::MatchmakingSessionId mmSessionId) const;

                const ExternalSessionServerConfig& getConfig() const { return mConfig; }
                

                struct ExternalUserProfile
                {
                    ExternalId mExternalId;
                    eastl::string mUserName;
                };
                typedef eastl::list<ExternalUserProfile> ExternalUserProfiles;

                /*! ************************************************************************************************/
                /*! \brief gets user profile data from external profile service.
                ***************************************************************************************************/
                virtual Blaze::BlazeRpcError getProfiles(const ExternalIdList& externalIds, ExternalUserProfiles& buf, const char8_t* groupAuthToken)
                {
                    return ERR_OK;
                }

                virtual Blaze::ClientPlatformType getPlatform() { return ClientPlatformType::INVALID; };
                virtual const ExternalSessionActivityTypeInfo& getPrimaryActivityType();
                
                static const char8_t* getValidJsonUtf8(const char8_t* value, size_t maxLen);

            protected:
                virtual void setReservationTimeout(const TimeValue& value) { mReservationTimeout = value; }
                virtual void setInactivityTimeout(const TimeValue& value) { mConfig.setExternalSessionInactivityTimeout(value); }
                virtual void setReadyTimeout(const TimeValue& value) { mConfig.setExternalSessionReadyTimeout(value); }
                virtual void setSessionEmptyTimeout(const TimeValue& value) { mConfig.setExternalSessionEmptyTimeout(value); }
                virtual void setImageMaxSize(uint32_t value) { mConfig.setExternalSessionImageMaxSize(value); }
                virtual void setImageDefault(const ExternalSessionImageHandle& value) { mConfig.setExternalSessionImageDefault(value); }
                virtual void setStatusValidLocales(const char8_t* value) { mConfig.setExternalSessionStatusValidLocales(value); }
                virtual void setStatusMaxLocales(uint16_t value) { mConfig.setExternalSessionStatusMaxLocales(value); }

                static BlazeRpcError waitSeconds(uint64_t seconds, const char8_t* context, size_t logRetryNumber);
                static BlazeRpcError waitTime(const TimeValue& time, const char8_t* context, size_t logRetryNumber);
            private:
                bool doesExternalSessionFullnessExcludeReservationsForGame(const GameSessionMaster& gameSession) const;
                bool isFullForExternalSessionExcludingReservations(const GameSessionMaster& gameSession) const;
                bool isFullForExternalSessionRole(const GameSessionMaster& gameSession) const;
                const GameActivity* choosePrimary(const GameActivityList& candidates) const;
                bool isHigherPriority(const GameActivity& a, const GameActivity& b) const;
                size_t getGameTypePriority(GameType gameType) const;
                static ExternalSessionUtil* createExternalSessionService(ClientPlatformType platform, const ExternalSessionServerConfig& config, const TimeValue& reservationTimeout, uint16_t maxMemberCount = UINT16_MAX, const TeamNameByTeamIdMap* teamNameByTeamIdMap = nullptr, GameManagerSlaveImpl* gameManagerSlave = nullptr);
            protected:
                ExternalSessionServerConfig mConfig;
                TimeValue mReservationTimeout;//external session reservation timeout. if 0 ignores. E.g. for MM/PG there are no reservations.
                mutable ExternalSessionActivityTypeInfo mPrimaryActivityTypeInfo;
        };
    }
}

#endif
