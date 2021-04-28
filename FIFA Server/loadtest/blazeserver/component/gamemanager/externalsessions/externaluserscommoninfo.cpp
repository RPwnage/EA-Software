/*! ************************************************************************************************/
/*!
    \file   externaluserscommoninfo.cpp

    \brief Utils for external user info

    \attention
    (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "gamemanager/externalsessions/externaluserscommoninfo.h"
#include "framework/usersessions/usersessionmanager.h" // for gUserSessionManager in lookupUserByExternalUserIdentification() and user defines


namespace Blaze
{
namespace GameManager
{
    const char8_t* externalUserIdentificationToString(const UserIdentification& ident, char8_t* buf, size_t bufLen)
    {
        blaze_snzprintf(buf, bufLen, "psnId:(%" PRIu64 "), xblId:(%" PRIu64 "), steamId:(%" PRIu64 "), switchId:(%s)", 
            ident.getPlatformInfo().getExternalIds().getPsnAccountId(), ident.getPlatformInfo().getExternalIds().getXblAccountId(), ident.getPlatformInfo().getExternalIds().getSteamAccountId(), ident.getPlatformInfo().getExternalIds().getSwitchId());
        return buf;
    }

    const char8_t* externalUserIdentificationToString(const UserIdentification &ident, eastl::string& buf)
    {
        return buf.sprintf("%s, blazeId(%" PRIi64 "), psnId(%" PRIu64 "), xblId(%" PRIu64 "), steamId(%" PRIu64 "), switchId(%s)", 
            ident.getName(), ident.getBlazeId(), ident.getPlatformInfo().getExternalIds().getPsnAccountId(), ident.getPlatformInfo().getExternalIds().getXblAccountId(), ident.getPlatformInfo().getExternalIds().getSteamAccountId(),  ident.getPlatformInfo().getExternalIds().getSwitchId()).c_str();
    }

    const Blaze::FixedString256 toExtUserLogStr(const UserIdentification &ident)
    {
        ClientPlatformType platform = ident.getPlatformInfo().getClientPlatform();
        Blaze::FixedString256 userStr(FixedString256::CtorSprintf(), "%s blazeId(%" PRIi64 ") ", ident.getName(), ident.getBlazeId());
        switch (platform)
        {
        case ps4:
        case ps5:
            userStr.append_sprintf("psnId:(%" PRIu64 ") ", ident.getPlatformInfo().getExternalIds().getPsnAccountId());
            break;
        case xone:
        case xbsx:
            userStr.append_sprintf("xblId:(%" PRIu64 ") ", ident.getPlatformInfo().getExternalIds().getXblAccountId());
            break;
        case steam:
            userStr.append_sprintf("steamId:(%" PRIu64 ") ", ident.getPlatformInfo().getExternalIds().getSteamAccountId());
            break;
        case nx:
            userStr.append_sprintf("switchId(%s) ", ident.getPlatformInfo().getExternalIds().getSwitchId());
            break;
        default:
            userStr.append_sprintf("<UNKNOWN EXTID>");
            break;
        }
        return userStr;
    }

    bool isExternalUserIdentificationForPlayer(const UserIdentification& ident, const UserInfoData& playerInfo)
    {
        switch (ident.getPlatformInfo().getClientPlatform())
        {
        case xone:
        case xbsx:
        case ps4:
        case ps5:
        case steam:
        case stadia:
        {
            ExternalId playerExtId = getExternalIdFromPlatformInfo(playerInfo.getPlatformInfo());
            ExternalId identExtId = getExternalIdFromPlatformInfo(ident.getPlatformInfo());

            return (playerExtId != INVALID_EXTERNAL_ID) && (identExtId == playerExtId);
        }
        case nx:
            return !playerInfo.getPlatformInfo().getExternalIds().getSwitchIdAsTdfString().empty() && playerInfo.getPlatformInfo().getExternalIds().getSwitchIdAsTdfString() == ident.getPlatformInfo().getExternalIds().getSwitchIdAsTdfString();
        default:
            return false;
        }
    }

    bool areUserIdsEqual(const UserIdentification& id1, const UserIdentification& id2)
    {
        // The fields below should only be compared if they are not invalid. If they are invalid for even one of the ids, we ignore them as some times the information is incomplete. However, if they are valid, then
        // they should match for both the ids. If all the fields are invalid, the ids compare as equal (invalid ids). 

        bool id1Invalid = (id1.getBlazeId() == INVALID_BLAZE_ID) 
            && (id1.getPlatformInfo().getExternalIds().getXblAccountId() != INVALID_XBL_ACCOUNT_ID) 
            && (id1.getPlatformInfo().getExternalIds().getPsnAccountId() != INVALID_PSN_ACCOUNT_ID)
            && (id1.getPlatformInfo().getExternalIds().getSteamAccountId() != INVALID_STEAM_ACCOUNT_ID)
            && (!id1.getPlatformInfo().getExternalIds().getSwitchIdAsTdfString().empty())
            && (id1.getPlatformInfo().getEaIds().getNucleusAccountId() != INVALID_ACCOUNT_ID)
            && (id1.getPlatformInfo().getEaIds().getOriginPersonaId() != INVALID_ORIGIN_PERSONA_ID);
        bool id2Invalid = (id2.getBlazeId() == INVALID_BLAZE_ID) 
            && (id2.getPlatformInfo().getExternalIds().getXblAccountId() != INVALID_XBL_ACCOUNT_ID) 
            && (id2.getPlatformInfo().getExternalIds().getPsnAccountId() != INVALID_PSN_ACCOUNT_ID)
            && (id2.getPlatformInfo().getExternalIds().getSteamAccountId() != INVALID_STEAM_ACCOUNT_ID)
            && (!id2.getPlatformInfo().getExternalIds().getSwitchIdAsTdfString().empty())
            && (id2.getPlatformInfo().getEaIds().getNucleusAccountId() != INVALID_ACCOUNT_ID)
            && (id1.getPlatformInfo().getEaIds().getOriginPersonaId() != INVALID_ORIGIN_PERSONA_ID);

        // Two completely invalid ids are considered equal.
        if (id1Invalid && id2Invalid)
        {
            return true;
        }

        // Now, we ignore any field where one of the ids has it as invalid as we might be dealing with incomplete info. But if the field is valid in both the ids, they must be equal.
        // We also keep a counter to make sure that at least one of the fields matched. This is to avoid scenarios similar to following.
        //
        // Field        Id1     Id2
        // Field 1     invalid  valid
        // Field 2     valid    invalid
        // Field 3     invalid  valid
        // 
        // And we do not make an early return as soon as one field matches as we want to test all the fields if they are valid.

        uint16_t fieldsMatched = 0;
        if (id1.getBlazeId() != INVALID_BLAZE_ID && id2.getBlazeId() != INVALID_BLAZE_ID)
        {
            if (id1.getBlazeId() != id2.getBlazeId())
            {
                return false; // If both fields are valid and they do not compare as equal, the ids are not equal.
            }
            else
            {
                ++fieldsMatched;
            }
        }

        if (id1.getPlatformInfo().getExternalIds().getXblAccountId() != INVALID_XBL_ACCOUNT_ID && id2.getPlatformInfo().getExternalIds().getXblAccountId() != INVALID_XBL_ACCOUNT_ID)
        {
            if (id1.getPlatformInfo().getExternalIds().getXblAccountId() != id2.getPlatformInfo().getExternalIds().getXblAccountId())
            {
                return false;
            }
            else
            {
                ++fieldsMatched;
            }
        }

        if (id1.getPlatformInfo().getExternalIds().getPsnAccountId() != INVALID_PSN_ACCOUNT_ID && id2.getPlatformInfo().getExternalIds().getPsnAccountId() != INVALID_PSN_ACCOUNT_ID)
        {
            if (id1.getPlatformInfo().getExternalIds().getPsnAccountId() != id2.getPlatformInfo().getExternalIds().getPsnAccountId())
            {
                return false;
            }
            else
            {
                ++fieldsMatched;
            }
        }

        if (id1.getPlatformInfo().getExternalIds().getSteamAccountId() != INVALID_STEAM_ACCOUNT_ID && id2.getPlatformInfo().getExternalIds().getSteamAccountId() != INVALID_STEAM_ACCOUNT_ID)
        {
            if (id1.getPlatformInfo().getExternalIds().getSteamAccountId() != id2.getPlatformInfo().getExternalIds().getSteamAccountId())
            {
                return false;
            }
            else
            {
                ++fieldsMatched;
            }
        }

        if (!id1.getPlatformInfo().getExternalIds().getSwitchIdAsTdfString().empty() && !id2.getPlatformInfo().getExternalIds().getSwitchIdAsTdfString().empty())
        {
            if (id1.getPlatformInfo().getExternalIds().getSwitchIdAsTdfString() != id2.getPlatformInfo().getExternalIds().getSwitchIdAsTdfString())
            {
                return false;
            }
            else
            {
                ++fieldsMatched;
            }
        }

        if (id1.getPlatformInfo().getEaIds().getNucleusAccountId() != INVALID_ACCOUNT_ID && id2.getPlatformInfo().getEaIds().getNucleusAccountId() != INVALID_ACCOUNT_ID)
        {
            if (id1.getPlatformInfo().getEaIds().getNucleusAccountId() != id2.getPlatformInfo().getEaIds().getNucleusAccountId())
            {
                return false;
            }
            else
            {
                ++fieldsMatched;
            }
        }

        if (id1.getPlatformInfo().getEaIds().getOriginPersonaId() != INVALID_ORIGIN_PERSONA_ID && id2.getPlatformInfo().getEaIds().getOriginPersonaId() != INVALID_ORIGIN_PERSONA_ID)
        {
            if (id1.getPlatformInfo().getEaIds().getOriginPersonaId() != id2.getPlatformInfo().getEaIds().getOriginPersonaId())
            {
                return false;
            }
            else
            {
                ++fieldsMatched;
            }
        }

        return (fieldsMatched > 0);
    }

    /*! ************************************************************************************************/
    /*! \brief return whether the inputs identify a potential primary platform first party user.
    ***************************************************************************************************/
    bool hasFirstPartyId(const PlatformInfo& platformInfo)
    {
        return ((platformInfo.getExternalIds().getPsnAccountId() != INVALID_PSN_ACCOUNT_ID) ||
                (platformInfo.getExternalIds().getXblAccountId() != INVALID_XBL_ACCOUNT_ID) ||
                (platformInfo.getExternalIds().getSteamAccountId() != INVALID_STEAM_ACCOUNT_ID) ||
                !platformInfo.getExternalIds().getSwitchIdAsTdfString().empty());
    }

    /*! ************************************************************************************************/
    /*! \brief return whether the identification info container has at least necessary first party relevant
         fields to identify a primary platform first party user.
        \param[in] ident user's identification info. Note, depending on the platform, certain fields must
        be set in order to create external player objects in games for the user.
    ***************************************************************************************************/
    bool hasFirstPartyId(const UserIdentification& ident)
    {
        return hasFirstPartyId(ident.getPlatformInfo());
    }

    /*! ************************************************************************************************/
    /*! \brief retrieve the user info for the player by its first party identification info, or BlazeId
        \param[in] ident contains player's first party identification info and/or BlazeId
        \return USER_ERR_USER_NOT_FOUND if user info not on the Blaze Server (or its db), ERR_OK if it is
    ***************************************************************************************************/
    BlazeRpcError lookupExternalUserByUserIdentification(const UserIdentification& ident, UserInfoPtr &userInfo)
    {
        if (ident.getBlazeId() != INVALID_BLAZE_ID)
        {
            return gUserSessionManager->lookupUserInfoByBlazeId(ident.getBlazeId(), userInfo);
        }

        return gUserSessionManager->lookupUserInfoByPlatformInfo(ident.getPlatformInfo(), userInfo);
    }

    void userIdentificationListToUserSessionIdList(const UserIdentificationList& externalUsers, UserSessionIdList& userSessionIdList)
    {
        for (const auto& userIdent : externalUsers)
        {
            UserInfoPtr userInfo;
            if (ERR_OK == lookupExternalUserByUserIdentification(*userIdent, userInfo))
            {
                UserSessionId primarySessionId = gUserSessionManager->getPrimarySessionId(userInfo->getId());
                if (primarySessionId != INVALID_USER_SESSION_ID)
                    userSessionIdList.push_back(primarySessionId);
            }
        }
    }


    /*! ************************************************************************************************/
    /*! \brief add copies of the joined reserved players info from one response to other response
    ***************************************************************************************************/
    void joinedReservedPlayerIdentificationsToOtherResponse(const JoinGameResponse& source, CreateGameResponse& target)
    {
        for (UserIdentificationList::const_iterator itr = source.getJoinedReservedPlayerIdentifications().begin(),
            end = source.getJoinedReservedPlayerIdentifications().end(); itr != end; ++itr)
        {
            (*itr)->copyInto(*target.getJoinedReservedPlayerIdentifications().pull_back());
        }
    }

    /*! ************************************************************************************************/
    /*! \brief add copies of the joined reserved players info from one response to other response
    ***************************************************************************************************/
    void joinedReservedPlayerIdentificationsToOtherResponse(const CreateGameResponse& source, JoinGameResponse& target)
    {
        for (UserIdentificationList::const_iterator itr = source.getJoinedReservedPlayerIdentifications().begin(),
            end = source.getJoinedReservedPlayerIdentifications().end(); itr != end; ++itr)
        {
            (*itr)->copyInto(*target.getJoinedReservedPlayerIdentifications().pull_back());
        }
    }

    /*! ************************************************************************************************/
    /*! \brief Find first user in the list, if one exists, with external session join permission. I.e. the first
        who could create/join the game's external session. 
        If platform is not INVALID, only returns users whose platform matches the requested platform.

        Returns nullptr if no user is found
    ***************************************************************************************************/
    const UserSessionInfo* getFirstExternalSessionJoinerInfo(const UserJoinInfoList& usersInfo, ClientPlatformType platform)
    {
        for(UserJoinInfoList::const_iterator iter = usersInfo.begin(); iter != usersInfo.end(); ++iter)
        {
            if ((*iter)->getUser().getHasExternalSessionJoinPermission() && (platform == INVALID || platform == (*iter)->getUser().getUserInfo().getPlatformInfo().getClientPlatform()))
            {
                return &(*iter)->getUser();
            }
        }

        return nullptr;
    }

} // namespace GameManager
} // namespace Blaze

