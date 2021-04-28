/*************************************************************************************************/
/*!
\file   fetchUsersAuditState_command.cpp


\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
\class FetchUsersAuditStateCommand

Check audit states of users

\note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// global includes
#include "framework/blaze.h"
//user includes
#include "framework/usersessions/usersessionmanager.h"
#include "framework/rpc/usersessionsslave/fetchusersauditstate_stub.h"

namespace Blaze
{

    class FetchUsersAuditStateCommand : public FetchUsersAuditStateCommandStub
    {
    public:

        FetchUsersAuditStateCommand(Message* message, Blaze::FetchUsersAuditStateRequest* request, UserSessionsSlave* componentImpl)
            :FetchUsersAuditStateCommandStub(message, request),
            mComponent(componentImpl)
        {
        }

        ~FetchUsersAuditStateCommand() override
        {
        }

        /* Private methods *******************************************************************************/
    private:

        FetchUsersAuditStateCommandStub::Errors execute() override
        {
            BLAZE_TRACE_LOG(Log::USER, "[FetchUsersAuditStateCommand].execute()");
            
            BlazeRpcError err = ERR_OK;
            AuditStateByBlazeIdsMap &auditStateByBlazeIdsMap = mResponse.getAuditStateByBlazeIdsMap();
            const BlazeIdList &blazeIdList = mRequest.getBlazeIdList();
            if (!blazeIdList.empty())
            {
                for (auto id : blazeIdList)
                {
                    AuditIdSet audits;
                    Logger::getAuditIds(id, "", "", "", INVALID_ACCOUNT_ID, INVALID, audits);
                    auditStateByBlazeIdsMap[id] = !audits.empty();
                }
            }
            else
            {
                // Get all users who are currently audited.
                // This is not a precise search; it simply returns all users listed in the AuditLogEntryList by BlazeId, persona name, and/or NucleusAccountId.
                // For more accurate results, use the getAudits RPC to fetch all configured audit log entries.
                AuditLogEntryList audits;
                Logger::getAudits(audits);

                typedef eastl::hash_map<AccountId, ClientPlatformSet> PlatformsByAccountIdMap;
                typedef eastl::map<ClientPlatformType, PersonaNameVector> PersonaNamesByPlatformMap;
                PlatformsByAccountIdMap platformsByAccountId;
                PersonaNamesByPlatformMap personaNamesByPlatform;
                AccountIdVector allAccountIds;

                for (auto& entry : audits)
                {
                    if (entry->getBlazeId() != INVALID_BLAZE_ID)
                        auditStateByBlazeIdsMap[entry->getBlazeId()] = true;
                    else if (entry->getPersona()[0] != '\0')
                        personaNamesByPlatform[entry->getPlatform()].push_back(entry->getPersona());
                    else if (entry->getNucleusAccountId() != INVALID_ACCOUNT_ID)
                    {
                        auto inserted = platformsByAccountId.insert(entry->getNucleusAccountId());
                        if (inserted.second)
                            allAccountIds.push_back(entry->getNucleusAccountId());

                        if (entry->getPlatform() == INVALID)
                        {
                            // This NucleusAccountId is audited on all platforms
                            const ClientPlatformSet& allPlatforms = gController->getHostedPlatforms();
                            inserted.first->second.insert(allPlatforms.begin(), allPlatforms.end());
                        }
                        else
                            inserted.first->second.insert(entry->getPlatform());
                    }
                }

                for (const auto& personaItr : personaNamesByPlatform)
                {
                    BlazeRpcError lookupErr = ERR_OK;
                    if (personaItr.first == INVALID)
                    {
                        // This persona name is audited on all platforms
                        UserInfoListByPersonaNameByNamespaceMap results;
                        lookupErr = gUserSessionManager->lookupUserInfoByPersonaNamesMultiNamespace(personaItr.second, results, true /*primaryNamespaceOnly*/, false /*restrictCrossPlatformResults*/);
                        if (lookupErr == ERR_OK)
                        {
                            for (const auto& namespaceItr : results)
                            {
                                for (const auto& nameItr : namespaceItr.second)
                                {
                                    for (const auto& userItr : nameItr.second)
                                        auditStateByBlazeIdsMap[userItr->getId()] = true;
                                }
                            }
                        }
                    }
                    else
                    {
                        PersonaNameToUserInfoMap results;
                        lookupErr = gUserSessionManager->lookupUserInfoByPersonaNames(personaItr.second, personaItr.first, results, gController->getNamespaceFromPlatform(personaItr.first));
                        if (lookupErr == ERR_OK)
                        {
                            for (const auto& nameItr : results)
                                auditStateByBlazeIdsMap[nameItr.second->getId()] = true;
                        }
                    }
                    if (lookupErr != ERR_OK)
                    {
                        BLAZE_TRACE_LOG(Log::USER, "[FetchUsersAuditStateCommand].execute: Unable to look up user info for " << personaItr.second.size() << " persona names on " 
                            << (personaItr.first == INVALID ? "any" : ClientPlatformTypeToString(personaItr.first)) << " platform.");
                    }
                }

                if (!allAccountIds.empty())
                {
                    UserInfoPtrList results;
                    BlazeRpcError lookupErr = gUserSessionManager->lookupUserInfoByAccountIds(allAccountIds, results);
                    if (lookupErr == ERR_OK)
                    {
                        for (const auto& userItr : results)
                        {
                            const ClientPlatformSet& platformSet = platformsByAccountId[userItr->getPlatformInfo().getEaIds().getNucleusAccountId()];
                            if (platformSet.find(userItr->getPlatformInfo().getClientPlatform()) != platformSet.end())
                                auditStateByBlazeIdsMap[userItr->getId()] = true;
                        }
                    }
                    else
                    {
                        BLAZE_TRACE_LOG(Log::USER, "[FetchUsersAuditStateCommand].execute: Unable to look up user info for " << allAccountIds.size() << " NucleusAccountIds.");
                    }
                }
            }

            mResponse.setAuditLoggingSuppressed(Logger::isAuditLoggingSuppressed());

            return commandErrorFromBlazeError(err);
        }

    private:
        UserSessionsSlave* mComponent;  // memory owned by creator, don't free
    };

    DEFINE_FETCHUSERSAUDITSTATE_CREATE_COMPNAME(UserSessionManager)

} // Blaze
