/*************************************************************************************************/
/*!
    \file   osdkseasonalplayutil.cpp
    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/
#include "gamereporting/osdk/osdkseasonalplayutil.h"

#include "framework/blaze.h"
#include "framework/controller/controller.h"
#include "framework/database/dbscheduler.h"

// seasonal play includes
#include "osdkseasonalplay/rpc/osdkseasonalplayslave.h"

namespace Blaze
{
namespace GameReporting
{

    OSDKSeasonalPlayUtil::OSDKSeasonalPlayUtil() :
        mSeasonalPlaySlave(NULL),
        mMemberId(OSDKSeasonalPlay::MEMBER_ID_INVALID),
        mMemberType(OSDKSeasonalPlay::SEASONALPLAY_MEMBERTYPE_USER),
        mSeasonId(OSDKSeasonalPlay::SEASON_ID_INVALID)
    {
        mSeasonalPlaySlave = static_cast<OSDKSeasonalPlay::OSDKSeasonalPlaySlave*>(
            gController->getComponent( OSDKSeasonalPlay::OSDKSeasonalPlaySlave::COMPONENT_ID, false ));
        if (NULL == mSeasonalPlaySlave)
        {
            ERR_LOG("[OSDKSeasonalPlayUtil:" << this << "].OSDKSeasonalPlayUtil() - unable to request Seasonal Play component");
        }
    }

    void OSDKSeasonalPlayUtil::setMember(OSDKSeasonalPlay::MemberId memberId, OSDKSeasonalPlay::MemberType memberType)
    {
        if (NULL != mSeasonalPlaySlave)
        {
            mMemberId = memberId;
            mMemberType = memberType;
            
            // determine season id for this member
            OSDKSeasonalPlay::GetSeasonIdRequest req;
            OSDKSeasonalPlay::GetSeasonIdResponse resp;
            req.setMemberId(mMemberId);
            req.setMemberType(mMemberType);

            // to invoke this RPC will require authentication
            UserSession::pushSuperUserPrivilege();
            BlazeRpcError seasonIdError = mSeasonalPlaySlave->getSeasonId(req,resp);
            UserSession::popSuperUserPrivilege();

            if (ERR_OK != seasonIdError)
            {
                ERR_LOG("[OSDKSeasonalPlayUtil:" << this << "].setMember() - error requesting season id");
                return;
            }

            mSeasonId = resp.getSeasonId();
        }
    }

    OSDKSeasonalPlay::SeasonId OSDKSeasonalPlayUtil::getSeasonId() const
    {
        OSDKSeasonalPlay::SeasonId memberSeasonId = OSDKSeasonalPlay::SEASON_ID_INVALID;

        if (NULL != mSeasonalPlaySlave)
        {
            memberSeasonId = mSeasonId;
        }

        return memberSeasonId;
    }

    OSDKSeasonalPlay::SeasonState OSDKSeasonalPlayUtil::getSeasonState() const
    {
        OSDKSeasonalPlay::SeasonState memberSeasonState = OSDKSeasonalPlay::SEASONALPLAY_SEASON_STATE_NONE;
        
        OSDKSeasonalPlay::SeasonDetails details;
        if(getSeasonDetails(details))
        {
            memberSeasonState = details.getSeasonState();
            TRACE_LOG("[OSDKSeasonalPlayUtil:" << this << "].getSeasonState() for Season Id " << mSeasonId << " as " << memberSeasonState);
        }
        
        return memberSeasonState;
    }

    bool OSDKSeasonalPlayUtil::getSeasonStatsCategory(char8_t *strCategoryName, uint32_t uSize) const
    {
        OSDKSeasonalPlay::SeasonConfiguration config;
        bool ret = false;

        if(getSeasonConfiguration(config))
        {
            if(NULL != strCategoryName && uSize > strlen(config.getStatCategoryName()))
            {
                blaze_strnzcpy(strCategoryName, config.getStatCategoryName(), uSize);
                ret = true;
            }
        }

        return ret;
    }

    OSDKTournaments::TournamentId OSDKSeasonalPlayUtil::getSeasonTournamentId() const
    {
        OSDKTournaments::TournamentId memberTournamentId = OSDKTournaments::INVALID_TOURNAMENT_ID;
        
        OSDKSeasonalPlay::SeasonConfiguration config;
        if(getSeasonConfiguration(config))
        {
            memberTournamentId = config.getTournamentId();
            TRACE_LOG("[OSDKSeasonalPlayUtil:" << this << "].getSeasonTournamentId() for Season Id " << mSeasonId << " as " << memberTournamentId);
        }

        return memberTournamentId;
    }

    bool OSDKSeasonalPlayUtil::getSeasonDetails(OSDKSeasonalPlay::SeasonDetails& details) const
    {
        bool ret = false;

        if (NULL != mSeasonalPlaySlave && OSDKSeasonalPlay::SEASON_ID_INVALID != mSeasonId)
        {
            OSDKSeasonalPlay::GetSeasonDetailsRequest req;
            req.setSeasonId(mSeasonId);

            // to invoke this RPC will require authentication
            UserSession::pushSuperUserPrivilege();
            BlazeRpcError error = mSeasonalPlaySlave->getSeasonDetails(req,details);
            UserSession::popSuperUserPrivilege();
			
            if (ERR_OK != error)
            {
                ERR_LOG("[OSDKSeasonalPlayUtil:" << this << "].getSeasonDetails() - error requesting season details");
            }
            else
            {
                ret = true;
            }
        }

        return ret;
    }

    bool OSDKSeasonalPlayUtil::getSeasonConfiguration(OSDKSeasonalPlay::SeasonConfiguration& config) const
    {
        bool ret = false;

        if (NULL != mSeasonalPlaySlave && OSDKSeasonalPlay::SEASON_ID_INVALID != mSeasonId)
        {
            OSDKSeasonalPlay::GetSeasonConfigurationResponse resp;

            // to invoke this RPC will require authentication
            UserSession::pushSuperUserPrivilege();
            BlazeRpcError error = mSeasonalPlaySlave->getSeasonConfiguration(resp);
            UserSession::popSuperUserPrivilege();

            if (ERR_OK != error)
            {
                ERR_LOG("[OSDKSeasonalPlayUtil:" << this << "].getSeasonConfiguration() - error requesting season configuration");
            }
            else
            {
                OSDKSeasonalPlay::GetSeasonConfigurationResponse::SeasonConfigurationList& list = resp.getInstanceConfigList();
                OSDKSeasonalPlay::GetSeasonConfigurationResponse::SeasonConfigurationList::const_iterator it = list.begin();
                for (; it != list.end(); it++)
                {
                    if((*it)->getSeasonId() == mSeasonId)
                    {
                        (*it)->copyInto(config);
                        ret = true;
                        break;
                    }
                }
            }
        }

        return ret;
    }

} //namespace GameReporting
} //namespace Blaze


