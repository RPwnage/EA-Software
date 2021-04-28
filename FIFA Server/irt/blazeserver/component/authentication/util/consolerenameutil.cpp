/*************************************************************************************************/
/*!
\file consolerenameutil.cpp


\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/
#include "framework/blaze.h"
#include "framework/usersessions/usersessionmanager.h"
#include "framework/usersessions/accountinfodb.h"

#include "authentication/authenticationimpl.h"
#include "authentication/util/consolerenameutil.h"

namespace Blaze
{
    namespace Authentication
    {

        ConsoleRenameUtil::ConsoleRenameUtil(const PeerInfo* peerInfo)
            : mGetPersonaUtil(peerInfo)
        {
        }

        BlazeRpcError ConsoleRenameUtil::doRename(const UserInfoData& userInfoData, UserInfoDbCalls::UpsertUserInfoResults& upsertUserInfoResults, bool updateCrossPlatformOpt, bool broadcastUpdateNotification)
        {
            BlazeRpcError error = ERR_OK;

            // Check if there are any name conflicts to resolve in the accountinfo db
            EaIdentifiers conflictingEaIds;
            conflictingEaIds.setOriginPersonaName(userInfoData.getPlatformInfo().getEaIds().getOriginPersonaName());
            error = AccountInfoDbCalls::filloutEaIdsByOriginPersonaName(conflictingEaIds);
            if (error == USER_ERR_USER_NOT_FOUND)
            {
                // This is unusual - at this point the accountinfo db should either have an entry for this user,
                // or it should have an entry for another user with the same originPersonaName.
                // It's possible there was a conflicting entry that's now been removed - try to upsert this user's
                // account info again.
                error = AccountInfoDbCalls::upsertAccountInfo(userInfoData);
                if (error == USER_ERR_EXISTS)
                {
                    // Either there's a conflict in a column that isn't being handled by AccountInfoDbCalls::resolve1stPartyIdConflicts, or
                    // somehow a conflicting entry was inserted during this user's login attempt. In either case, there's no graceful way to recover.
                    ERR_LOG("[ConsoleRenameUtil].doRename(): UpsertAccountInfo returned USER_ERR_EXISTS, but no entries were found for OriginPersonaName(" <<
                        userInfoData.getPlatformInfo().getEaIds().getOriginPersonaName() << "). BlazeId: " << userInfoData.getId() << " OriginPersonaId: " << userInfoData.getPlatformInfo().getEaIds().getOriginPersonaId());
                    error = ERR_SYSTEM;
                }
            }
            else if (error == ERR_OK)
            {
                if (conflictingEaIds.getOriginPersonaId() != userInfoData.getPlatformInfo().getEaIds().getOriginPersonaId())
                {
                    error = doRenameInternal(ACCOUNT, userInfoData);
                    if (error == ERR_OK)
                    {
                        // Now that we've resolved all name conflicts, we just need to upsert the original user's accountinfo
                        error = AccountInfoDbCalls::upsertAccountInfo(userInfoData);
                    }
                }
            }

            if (error != ERR_OK)
                return error;

            // Now resolve name conflicts in the userinfo table.
            // But first deactivate any old users with conflicting 1st party identifiers.
            error = UserInfoDbCalls::resolve1stPartyIdConflicts(userInfoData);
            if (error == USER_ERR_USER_NOT_FOUND)
                error = ERR_OK;

            if (error != ERR_OK)
                return error;

            error = gUserSessionManager->upsertUserInfo(userInfoData, false /*nullExtIds*/, true /*updateTimeFields*/, updateCrossPlatformOpt, upsertUserInfoResults.newRowInserted, upsertUserInfoResults.firstSetExternalData, upsertUserInfoResults.previousCountry, broadcastUpdateNotification);
            if (error != USER_ERR_EXISTS)
                return error;

            // There must be another user in the userinfo db with a conflicting persona name.
            // Update that user with the correct persona name from Nucleus.
            // But first, temporarily deactivate this user and set his persona name to nullptr,
            // to prevent any naming loops/deadlocks.
            bool unused1 = false, unused2 = false;
            uint32_t unused3 = 0;
            UserInfoData userInfoDataCopy;
            userInfoData.copyInto(userInfoDataCopy);
            userInfoDataCopy.setPersonaName(nullptr);
            userInfoDataCopy.setStatus(0);
            userInfoDataCopy.setIsPrimaryPersona(false);
            error = gUserSessionManager->upsertUserInfo(userInfoDataCopy, false /*nullExtIds*/, false /*updateTimeFields*/, false /*updateCrossPlatformOpt*/, unused1, unused2, unused3, false /*broadcastUpdateNotification*/);
            if (error == USER_ERR_EXISTS)
            {
                ERR_LOG("[ConsoleRenameUtil].doRename(): UpsertUserInfo returned USER_ERR_EXISTS, but we're setting persona name to nullptr. This should not happen. blazeId(" << userInfoData.getId() << ") personaNamespace(" << userInfoData.getPersonaNamespace() << ")");
                return ERR_SYSTEM;
            }
            else if (error != ERR_OK)
            {
                return error;
            }

            error = doRenameInternal(USER, userInfoData);
            if (error == ERR_OK)
            {
                // All name conflicts have been resolved; now just update the original user's userinfo
                error = gUserSessionManager->upsertUserInfo(userInfoData, false /*nullExtIds*/, true /*updateTimeFields*/, updateCrossPlatformOpt, upsertUserInfoResults.newRowInserted, upsertUserInfoResults.firstSetExternalData, upsertUserInfoResults.previousCountry, broadcastUpdateNotification);
            }

            return error;
        }

        BlazeRpcError ConsoleRenameUtil::doRenameInternal(RenameType renameType, const UserInfoData& userInfoData)
        {
            BlazeRpcError error = ERR_OK;
            UserInfoDeque userInfoDeque;
            const char8_t* newPersonaName = getPersonaName(renameType, userInfoData);
            const char8_t* personaNamespace = renameType == ACCOUNT ? NAMESPACE_ORIGIN : userInfoData.getPersonaNamespace();
            eastl::string nextUserToUpdate = newPersonaName;
            eastl::hash_set<eastl::string> conflictingPersonas;
            conflictingPersonas.insert(newPersonaName);
            while (error == ERR_OK && !nextUserToUpdate.empty())
            {
                eastl::string nextConflictingPersona;
                error = updateUserByPersonaName(renameType, nextUserToUpdate.c_str(), personaNamespace, userInfoData.getPlatformInfo().getClientPlatform(), userInfoDeque, nextConflictingPersona);
                if (error == ERR_OK)
                {
                    if (!nextConflictingPersona.empty())
                    {
                        eastl::hash_set<eastl::string>::iterator itr = conflictingPersonas.find(nextConflictingPersona);
                        if (itr != conflictingPersonas.end())
                        {
                            // We've hit a naming loop/deadlock. Since there can't be two users in Nucleus with the same persona name,
                            // this must mean that the user whose name we were originally changing to nextConflictingPersona now has a
                            // different persona name. Pop that user off the renaming queue (along with anyone added after him) and continue from there.
                            if (blaze_stricmp(nextConflictingPersona.c_str(), newPersonaName) == 0)
                            {
                                // The original user changed his persona name while logging in.
                                // At this point we should really just force the user to log in again.
                                ERR_LOG("[ConsoleRenameUtil].doRename(): User changed his persona name while logging in. Initial personaName("
                                    << newPersonaName << ") personaNamespace(" << personaNamespace << ")");
                                return ERR_SYSTEM;
                            }

                            conflictingPersonas.erase(itr);
                            userInfoDeque.pop_back();
                            while (!userInfoDeque.empty() && blaze_stricmp(nextConflictingPersona.c_str(), getPersonaName(renameType, *userInfoDeque.back())) != 0)
                            {
                                conflictingPersonas.erase(getPersonaName(renameType, *userInfoDeque.back()));
                                userInfoDeque.pop_back();
                            }
                            userInfoDeque.pop_back();

                            if (userInfoDeque.empty())
                                nextUserToUpdate = getPersonaName(renameType, userInfoData);
                            else
                                nextUserToUpdate = getPersonaName(renameType, *userInfoDeque.back());
                        }
                        else
                        {
                            conflictingPersonas.insert(nextConflictingPersona);
                            nextUserToUpdate = nextConflictingPersona;
                        }
                    }
                    else
                    {
                        break;
                    }
                }
            }

            // All conflicts should be resolved; now just do the upserts
            while (error == ERR_OK && !userInfoDeque.empty())
            {
                UserInfoPtr foundUserInfo = userInfoDeque.back();
                userInfoDeque.pop_back();
                if (renameType == ACCOUNT)
                {
                    error = gUserSessionManager->updateOriginPersonaName(foundUserInfo->getPlatformInfo().getEaIds().getNucleusAccountId(), foundUserInfo->getPlatformInfo().getEaIds().getOriginPersonaName(), true /*broadcastUpdateNotification*/);
                }
                else
                {
                    bool unused1 = false, unused2 = false;
                    uint32_t unused3 = 0;
                    error = gUserSessionManager->upsertUserInfo(*foundUserInfo, false /*nullExtIds*/, false /*updateTimeFields*/, false /*updateCrossPlatformOpt*/, unused1, unused2, unused3, true /*broadcastUpdateNotification*/);
                }
                if (error == USER_ERR_EXISTS)
                {
                    ERR_LOG("[ConsoleRenameUtil].doRename(): All conflicts should already have been resolved, but got error[" << ErrorHelp::getErrorName(error) << "] upserting info for user with personaName(" 
                        << getPersonaName(renameType, *foundUserInfo) << ") personaNamespace(" << personaNamespace << ")");
                    error = ERR_SYSTEM;
                }
            }

            return error;
        }

        BlazeRpcError ConsoleRenameUtil::updateUserByPersonaName(RenameType renameType, const char8_t* oldPersonaName, const char8_t* namespaceName, ClientPlatformType platform, UserInfoDeque& userInfoDeque, eastl::string& nextConflictingPersona)
        {
            UserInfoPtr userInfo;
            UserInfo userInfoToUpsert;
            BlazeRpcError error = ERR_OK;
            if (renameType == ACCOUNT)
            {
                EaIdentifiers conflictingEaIds;
                conflictingEaIds.setOriginPersonaName(oldPersonaName);
                error = AccountInfoDbCalls::filloutEaIdsByOriginPersonaName(conflictingEaIds);
                if (error == ERR_OK)
                {
                    userInfo = BLAZE_NEW UserInfo();
                    userInfo->setOriginPersonaId(conflictingEaIds.getOriginPersonaId());
                    userInfo->getPlatformInfo().getEaIds().setOriginPersonaId(conflictingEaIds.getOriginPersonaId());
                    userInfo->getPlatformInfo().getEaIds().setNucleusAccountId(conflictingEaIds.getNucleusAccountId());
                }
            }
            else
            {
                UserInfoPtrList userInfoList;
                PersonaNameVector personaNameList;
                personaNameList.push_back(oldPersonaName);
                error = UserInfoDbCalls::lookupUsersByPersonaNames(namespaceName, personaNameList, platform, userInfoList, true /*ignoreStatus*/);
                if (error == ERR_OK)
                {
                    if (userInfoList.empty())
                    {
                        error = USER_ERR_USER_NOT_FOUND;
                    }
                    else
                    {
                        userInfo = *(userInfoList.begin());
                    }
                }
            }

            if (error != ERR_OK)
            {
                ERR_LOG("[ConsoleRenameUtil].updateUserByPersonaName(): Error[" << ErrorHelp::getErrorName(error) << "] looking up " << (renameType == ACCOUNT ? "OriginPersonaId" : "UserInfo") << " by persona name '" << oldPersonaName << "'; personaNamespace(" << namespaceName << ")");

                if (error == USER_ERR_USER_NOT_FOUND)
                {
                    // The user isn't in the db
                    // This is unusual, since we wouldn't be attempting to update a user's persona name if their current name didn't conflict with an existing db entry.
                    // Possibly the offending entry has already been updated by some other means, so just continue.
                    return ERR_OK;
                }
                return error;
            }

            // Look up the user's up-to-date persona name in Nucleus
            PersonaInfo personaInfo;
            if (blaze_strcmp(namespaceName, NAMESPACE_ORIGIN) == 0)
            {
                // Look up the Origin persona name
                error = mGetPersonaUtil.getPersona(userInfo->getPlatformInfo().getEaIds().getOriginPersonaId(), &personaInfo);
            }
            else
            {
                // Look up the 1st-party persona name
                error = mGetPersonaUtil.getPersona(userInfo->getId(), &personaInfo);
            }

            if (error == ERR_OK)
            {
                if (renameType == ACCOUNT)
                    userInfo->getPlatformInfo().getEaIds().setOriginPersonaName(personaInfo.getDisplayName());
                else
                    userInfo->setPersonaName(personaInfo.getDisplayName());
            }

            bool nullExtId = false;
            userInfo->copyInto(userInfoToUpsert);
            if (error != ERR_OK)
            {
                WARN_LOG("[ConsoleRenameUtil].updateUserByPersonaName(): Error[" << ErrorHelp::getErrorName(error) << "] looking up persona info. User will be deactivated. personaName(" << getPersonaName(renameType, *userInfo) << ") personaNamespace(" << namespaceName << ")");

                userInfoToUpsert.setPersonaName(nullptr);
                userInfoToUpsert.getPlatformInfo().getEaIds().setOriginPersonaName(nullptr);
                userInfoToUpsert.setStatus(0);
                nullExtId = true;
            }

            if (renameType == ACCOUNT)
            {
                error = gUserSessionManager->updateOriginPersonaName(userInfoToUpsert.getPlatformInfo().getEaIds().getNucleusAccountId(), userInfoToUpsert.getPlatformInfo().getEaIds().getOriginPersonaName(), true /*broadcastUpdateNotification*/);
            }
            else
            {
                bool unused1 = false, unused2 = false;
                uint32_t unused3 = 0;
                error = gUserSessionManager->upsertUserInfo(userInfoToUpsert, nullExtId, false /*updateTimeFields*/, false /*updateCrossPlatformOpt*/, unused1, unused2, unused3, true /*broadcastUpdateNotification*/);
            }
            if (error == USER_ERR_EXISTS)
            {
                if (getPersonaName(renameType, userInfoToUpsert)[0] == '\0')
                {
                    ERR_LOG("[ConsoleRenameUtil].updateUserByPersonaName(): Update of " << (renameType == ACCOUNT ? "OriginPersonaName" : "UserInfo") << " returned USER_ERR_EXISTS, but we're setting persona name to nullptr. This should not happen. oldPersonaName(" << oldPersonaName << ") personaNamespace(" << namespaceName << ")");
                    return ERR_SYSTEM;
                }
                userInfoDeque.push_back(userInfo);
                nextConflictingPersona = getPersonaName(renameType, *userInfo);
                return ERR_OK;
            }

            return error;
        }

        const char8_t* ConsoleRenameUtil::getPersonaName(RenameType renameType, const UserInfoData& userInfo)
        {
            return (renameType == ACCOUNT ? userInfo.getPlatformInfo().getEaIds().getOriginPersonaName() : userInfo.getPersonaName());
        }

    } // namespace Authentication
} // namespace Blaze
