/*************************************************************************************************/
/*!
    \file   memberremoved.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"
#include "framework/usersessions/userinfo.h"
#include "framework/usersessions/usersession.h"
// FIFA SPECIFIC CODE START
#include "framework/controller/controller.h"

#include "stats/updatestatsrequestbuilder.h"
#include "stats/updatestatshelper.h"

// clubs includes
#include "clubs/clubsslaveimpl.h"

#include "customcode/component/clubs/tdf/osdk_clubs_server.h"

// seasonal play includes
#include "osdkseasonalplay/tdf/osdkseasonalplaytypes.h"
#include "osdkseasonalplay/rpc/osdkseasonalplayslave.h"
// FIFA SPECIFIC CODE END
#include "rivals.h"

namespace Blaze
{

namespace Clubs
{

/*** Public Methods ******************************************************************************/

/*************************************************************************************************/
/*!
    \brief onMemberRemoved

    Custom processing upon member removed.

*/
/*************************************************************************************************/
BlazeRpcError ClubsSlaveImpl::onMemberRemoved(ClubsDatabase * clubsDb, 
        const ClubId clubId, const BlazeId blazeId, bool bLeave, bool bClubDelete)
{
    // FIFA SPECIFIC CODE START
    BlazeRpcError result = Blaze::ERR_OK;
    uint64_t version = 0;

    /***************************************************************
        update Users Club OTP stats and Club OTP Free Agent stats
    ****************************************************************/
    {
        Stats::UpdateStatsRequestBuilder updateStatsUtil;
        Blaze::Clubs::ClubsUtil clubUtilhelper;
        clubUtilhelper.SetClubMemberAsFreeAgent(&updateStatsUtil, blazeId, true);

        // Update stats
        Stats::UpdateStatsHelper updateStatsHelper;
        BlazeRpcError error = ERR_OK;
        error = updateStatsHelper.initializeStatUpdate((Stats::UpdateStatsRequest&)updateStatsUtil);
        if (ERR_OK != error)
        {
            ERR_LOG("[ClubsSlaveImpl].onMemberRemoved() - initializeStatUpdate expecting error code: ERR_OK, but got: " << ErrorHelp::getErrorName(error));
        }
        else
        {
            error = updateStatsHelper.commitStats();
            if (ERR_OK != error)
            {
                 ERR_LOG("[ClubsSlaveImpl].onMemberRemoved() - commitStats expecting error code: ERR_OK, but got: " << ErrorHelp::getErrorName(error));
            }
        }
    }

    // end of update Users Club OTP stats and Club OTP Free Agent stats

    /**********************
      CLUB CHAMP MODE
    ***********************/
    {

        // If the leaving user has the Club Champ record, reset it
        bool bResult = true;
        ClubRecordId champRecordId = 0;
        const OSDKClubsConfig &config = static_cast<const OSDKClubsConfig&>(*getConfig().getCustomSettings());
        champRecordId = config.getClubsChampRecordId();
        if(false == bResult)
        {
            WARN_LOG("[ClubsSlaveImpl].onMemberRemoved() - could NOT find OSDK_CLUBS_CHAMP_RECORD_ID setting in the configuration file.");
        }
        else
        {
            ClubRecordbookRecord clubRecdbookRecd;
            clubRecdbookRecd.clubId = clubId;
            clubRecdbookRecd.recordId = champRecordId;
            result = clubsDb->getClubRecord(clubRecdbookRecd);

            if (result != Blaze::ERR_OK)
            {
                WARN_LOG("[ClubsSlaveImpl].onMemberRemoved() - could NOT find the Club CHAMP Record [id = " << champRecordId << "] from the configuration file.");
            }
            else if(blazeId == clubRecdbookRecd.blazeId)
            {
                /******************************
                reset the Champ record
                ******************************/         
                ClubRecordIdList clubRecdIdList;
                clubRecdIdList.push_back(champRecordId);
                result = resetClubRecords(clubsDb->getDbConn(), clubId, clubRecdIdList);
                
                if(result != Blaze::ERR_OK)
                {
                    WARN_LOG("[ClubsSlaveImpl].onMemberRemoved() - could NOT reset the Club Champ Record [" << champRecordId << "] for club [" << clubId << "]");
                }
                else
                {
                    /*******************************************************************
                        Replace the latest Champ History entry inserted by onRecordUpdate()
                            from "GM reset" to "member no longer in the club"
                    *******************************************************************/
                    // This is example custom code for keeping track of belt-record history.

                    // This define the max entry that can be stored in the club meta-data.
                    //  In Ping, we calculated this base on assumption of using 1KB max in the meta data.
                    //  Game teams can adjust this based on if club meta-data also get used in other cases.
                    //  The absolute max history can be stored is 66 entries, based on the 2KB club meta-data

                    // OSDK requires to keep this format so OSDK able to decode on OSDK client side
                    // Record history is kept as string and can be located anywhere in club metadata string
                    //  Format:  Champ:w,time,winnerID,loserID;d,time,winnerID,loserID;r,time,GMID,0;l,time,0,0;d,time,winnerID,loserID;r,time,GMID,0;:Champ
                    //           All numbers are in Hexadecimal.
                    //  enum:   w   -   winner newly earn the Champ from loser
                    //          d   -   winner defended the Champ from loser
                    //          r   -   GM reset the Champ
                    //          l   -   Champ owner no longer belongs to the club

                    const char8_t* CHAMP_RECORD_BEGIN_TAG = "Champ:";
                    const char8_t* ENTRIES_DELIM = ";";      // separator between record entries
                    const char8_t* RECORD_RESET_TAG = "r";
                    const char8_t* RECORD_LEAVE_TAG = "l";
                    const char8_t* RECORD_DELIM = ",";       // separator of data within a record entry 

                    eastl::list<eastl::string> recordHistory;

                    Club club;
                    result = clubsDb->getClub(clubId, &club, version);
                    if (result != Blaze::ERR_OK)
                    {
                        WARN_LOG("[memberremoved.cpp] Could not get club [" << clubId << "].");
                    }
                    else
                    {
                        eastl::string metadata = club.getClubSettings().getMetaDataUnion().getMetadataString();
                        eastl::string::size_type pos = metadata.find(CHAMP_RECORD_BEGIN_TAG);
                        //DEBUG3("metadata = %s, pos = %d", metadata.c_str(), pos);
                        if(pos != metadata.npos)
                        {
                            //get the last champ history entry
                            eastl::string::size_type lastHistoryEndPos = metadata.find(ENTRIES_DELIM, pos);
                            eastl::string strLastChampHistory = metadata.substr(pos + 6, lastHistoryEndPos - (pos + 6));
                            //DEBUG3("[memberremoved.cpp] strLasChampHistory = %s", strLastChampHistory.c_str());
                            eastl::string lastHistoryType = strLastChampHistory.substr(0, 1);
                            if(lastHistoryType.compare(RECORD_RESET_TAG) == 0)
                            {
                                //The onRecordUpdate has treated this record result as a GM triggered reset,
                                //  convert this back to "onMemberRemoved" type of rest
                                eastl::string::size_type firstCommaPos = strLastChampHistory.find(RECORD_DELIM);
                                eastl::string::size_type secondCommaPos = strLastChampHistory.find(RECORD_DELIM, firstCommaPos + 1);
                                eastl::string strLastHistoryTime = strLastChampHistory.substr(firstCommaPos + 1, secondCommaPos - firstCommaPos - 1);
                                eastl::string strNewLastHistory;
                                strNewLastHistory.sprintf("%s,%s,0,0", RECORD_LEAVE_TAG, strLastHistoryTime.c_str());

                                //Put the new History back
                                eastl::string strNewMeta = metadata.substr(0, pos + 6);
                                strNewMeta.append(strNewLastHistory);
                                strNewMeta.append(metadata.substr(lastHistoryEndPos));
                                TRACE_LOG("[memberremoved.cpp] new Metadata = " << strNewMeta.c_str());
                                metadata = strNewMeta;
                            }
                            else
                            {
                                WARN_LOG("[memberremoved.cpp] expect the last Champ history to be the GM reset type, but it's actually in type = " << lastHistoryType.c_str());
                            }
                        }
                        else
                        {
                            WARN_LOG("[memberremoved.cpp] un-expected, the current club meta data has no Champ history");
                        }
                    }
                }
            }
        }
    }
    // end of CLUB CHAMP MODE
    
    /*************************
        CLUB RIVAL MODE
    *************************/
    {
        // lock the club for potential setting changes
        Club club;
        result = clubsDb->lockClub(clubId, version, &club);
        if(result != Blaze::CLUBS_ERR_INVALID_CLUB_ID)
        {
            if (result != Blaze::ERR_OK)
            {
                WARN_LOG("[ClubsSlaveImpl::onMemberRemoved] - Failed to obtain club " << clubId << ".");
            }
            else
            {
                // this club is NOT disbanded, process for the Club Rival feature
                if (club.getClubSettings().getAcceptanceFlags().getCLUBS_ACCEPT_RIVALS())
                {
                    // check club eligibility for rivals
                    if (club.getClubInfo().getMemberCount() < minMemberCountReqForRivals)
                    {
                        // this club just has member count less than minimum in order to be eligible for Rival feature
                        
                        if(club.getClubSettings().getCustClubSettings().getCustOpt5() == CLUBS_RIVAL_PENDING_ASSIGNMENT)
                        {
                            // this club is currently in "Pending Rival Assignment" state; however, a user just left
                            //      making this club no longer eligible for Rival assignment
                            //update the club's setting to "NOT ELIGIBLE"
                            ClubSettings& settings = club.getClubSettings();
                            settings.getCustClubSettings().setCustOpt5(CLUBS_RIVAL_NOT_ELIGIBLE);
                            result = clubsDb->updateClubSettings(club.getClubId(), settings, version);
                            if(result != Blaze::ERR_OK)
                            {
                                WARN_LOG("[ClubsSlaveImpl::onMemberRemoved] - Failed to update club settings for club " << clubId << ".");
                            }
                            else
                            {
                                result = setIsRankedOnRivalLeaderboard(clubId, false);
                                if(result != Blaze::ERR_OK)
                                {
                                    WARN_LOG("[ClubsSlaveImpl::onMemberRemoved] - Failed to set club [" << clubId << "] as ranked on Rival Leaderboard");
                                }
                            }
                            
                            updateCachedClubInfo(version, club.getClubId(), club);
                        }
                    }
                }
            }
        }
        else
        {
            // last club member left so the club already disbanded.
            result = Blaze::ERR_OK;
        }
    }
    // end of CLUB RIVAL MODE

    /**************************************
        update Club News to log Remove Event
     **************************************/
    {
        if (!bClubDelete)
        {
            UserInfoPtr userInfo;
            result = gUserSessionManager->lookupUserInfoByBlazeId(blazeId, userInfo);
            if (result != Blaze::ERR_OK)
            {
                WARN_LOG("[onMemberRemoved] error (" << result << ") looking up user " << blazeId <<
                         " for club " << clubId);
            }
            else
            {
                char _blazeId[32];
                blaze_snzprintf(_blazeId, sizeof(_blazeId), "%" PRId64, userInfo->getId());
                result = logEvent(clubsDb, clubId,
                        getEventString(bLeave ? LOG_EVENT_LEAVE_CLUB : LOG_EVENT_REMOVED_FROM_CLUB),
                        NEWS_PARAM_BLAZE_ID, _blazeId);
                if (result != ERR_OK)
                {
                    WARN_LOG("[onMemberRemoved] unable to log event for user " << blazeId <<
                             " in club " << clubId);
                }
            }
        }
        else
        {
            // Removing a club Id from the osdk seasonalplay registration table when the club is removed.
            OSDKSeasonalPlay::OSDKSeasonalPlaySlave *seasonalPlayComponent =
                static_cast<OSDKSeasonalPlay::OSDKSeasonalPlaySlave*>(gController->getComponent(OSDKSeasonalPlay::OSDKSeasonalPlaySlave::COMPONENT_ID, false));
            if (nullptr != seasonalPlayComponent)
            {
                OSDKSeasonalPlay::DeleteRegistrationRequest req;
                req.setMemberId(static_cast<OSDKSeasonalPlay::MemberId>(clubId));
                req.setMemberType(OSDKSeasonalPlay::SEASONALPLAY_MEMBERTYPE_CLUB);
                result = seasonalPlayComponent->deleteRegistration(req);
                if (result != Blaze::ERR_OK)
                {
                    TRACE_LOG("[onMemberRemoved] error deleting club " << clubId << " in seasonalplay");
                    if(result == Blaze::OSDKSEASONALPLAY_ERR_NOT_FOUND)
                    {
                        // The club might have been removed from the registration table by another member remove call of the
                        // member in the same club.
                        result = Blaze::ERR_OK;
                    }
                }
            }
            else
            {
                WARN_LOG("[ClubsSlaveImpl].onMemberRemoved() - osdkseasonalplay component is not available");
            }

        }
    }
    // end of CLUB NEWS
    // FIFA SPECIFIC CODE END

    return result;
}
} // Clubs
} // Blaze
