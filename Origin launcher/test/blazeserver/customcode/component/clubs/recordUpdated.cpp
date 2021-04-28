/*************************************************************************************************/
/*!
    \file   recordupdated.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"

// clubs includes
#include "clubsslaveimpl.h"

// FIFA SPECIFIC CODE START
#include "customcode/component/clubs/tdf/osdk_clubs_server.h"
// FIFA SPECIFIC CODE END

namespace Blaze
{
namespace Clubs
{

/*** Public Methods ******************************************************************************/

/*************************************************************************************************/
/*!
    \brief onRecordUpdated

    Custom processing upon club record updated.

*/
/*************************************************************************************************/
BlazeRpcError ClubsSlaveImpl::onRecordUpdated(ClubsDatabase *clubsDb,
    const ClubRecordbookRecord *record)
{
    // FIFA SPECIFIC CODE START
    bool result = true;
    const OSDKClubsConfig &config = static_cast<const OSDKClubsConfig &>(*getConfig().getCustomSettings());
    ClubRecordId champRecordId = config.getClubsChampRecordId();


    //DEBUG3("[ClubsSlaveImpl::recordUpdated.cpp] OSDK_CLUBS_CHAMP_RECORD_ID = %d", champRecordId);

    BlazeRpcError error = Blaze::ERR_OK;
    uint64_t version = 0;

    // is this the Champ record?
    if(record->recordId == champRecordId)
    {
        // This is example custom code for keeping track of belt-record history.

        // This define the max entry that can be stored in the club meta-data.
        //  In Ping, we calculated this base on assumption of using 1KB max in the meta data.
        //  Game teams can adjust this based on if club meta-data also get used in other cases.
        //  The absolute max history can be stored is 66 entries, based on the 2KB club meta-data
        
        uint32_t MAX_CHAMP_HISTORY = config.getClubsChampMaxRecordsStored();

        if(false == result)
        {
            WARN_LOG("[ClubsSlaveImpl::recordUpdated.cpp], could NOT find OSDK_CLUBS_CHAMP_MAX_RECORD_STORED setting in the configuration file.  Just use default value, 5, for now.");
        }

        const ClubId clubId = record->clubId;
        
        TRACE_LOG("[ClubsSlaveImpl::onRecordUpdated] Processing onRecordUpdated for club [" << clubId <<
                  "] and record [" << record->recordId << "], blazeId [" << record->blazeId << "], statInt[" << record->statValueInt << "]");
        
        Club club;
        error = clubsDb->getClub(clubId, &club, version);
        if (error != Blaze::ERR_OK)
        {
            ERR_LOG("[ClubsSlaveImpl::onRecordUpdated] Could not get club.");
            return error;
        }

        // OSDK requires to keep this format so OSDK able to decode on OSDK client side
        // Record history is kept as string and can be located anywhere in club metadata string
        //  Format:  Champ:0,time,winnerID,loserID;1,time,winnerID,loserID;2,time,GMID,0;3,time,0,0;1,time,winnerID,loserID;2,time,GMID,0:Champ
        //           All numbers are in Hexadecimal.
        //  enum:   w   -   winner newly earn the Champ from loser
        //          d   -   winner defended the Champ from loser
        //          r   -   GM reset the Champ
        //          l   -   Champ owner no longer belongs to the club
        
        const char8_t* CHAMP_RECORD_BEGIN_TAG = "Champ:";
        const char8_t* CHAMP_RECORD_END_TAG = ":Champ";
        const char8_t* ENTRIES_DELIM = ";";      // separator between record entries
        const char8_t* RECORD_WIN_TAG = "w";
        const char8_t* RECORD_DEFEND_TAG = "d";
        const char8_t* RECORD_RESET_TAG = "r";
        //const char8_t* RECORD_LEAVE_TAG = "l";
        const char8_t* RECORD_DELIM = ",";       // separator of data within a record entry 
        
        /***************************************************
            Parse out the History list from the meta-data
        ****************************************************/
        eastl::list<eastl::string> recordHistory;
            
        eastl::string metadata = club.getClubSettings().getMetaDataUnion().getMetadataString();
        eastl::string::size_type pos = metadata.find(CHAMP_RECORD_BEGIN_TAG);
        eastl::string::size_type endPos = metadata.find(CHAMP_RECORD_END_TAG);
        
        TRACE_LOG("[ClubsSlaveImpl::onRecordUpdated] metadata before process = " << metadata.c_str());

        eastl::string strOldChampHistory;

        eastl::string strCurrentEntry("");

        if (pos != metadata.npos && pos + 6 != endPos)
        {
            //get the Champ history sub string        
            strOldChampHistory = metadata.substr(pos + 6, endPos - (pos + 6));
            
            TRACE_LOG("[ClubsSlaveImpl::onRecordUpdated] Champ history before process = " << strOldChampHistory.c_str());

            eastl::string::size_type entryStartPos = 0;
            eastl::string::size_type entryEndPos = strOldChampHistory.find(ENTRIES_DELIM, 0);

            uint32_t i = 0;
            for(i = 0; i < MAX_CHAMP_HISTORY && entryEndPos != strOldChampHistory.npos; i++)
            {
                strCurrentEntry = strOldChampHistory.substr(entryStartPos, (entryEndPos + 1) - entryStartPos);
                recordHistory.push_back(strCurrentEntry);

                //for next history entry
                entryStartPos = entryEndPos + 1;
                entryEndPos = strOldChampHistory.find(ENTRIES_DELIM, entryStartPos);
            }
        }   
        
        /********************************************
            Add the new entry to the history list
        *********************************************/
        
        eastl::string strTime;
        strTime.sprintf("%x", TimeValue::getTimeOfDay().getSec());
        eastl::string strParam1("0");
        eastl::string strParam2("0");
        eastl::string strCurrentRecordType(RECORD_RESET_TAG);    // the default case is GM reseting
        if(record->blazeId > 0)    // it's a valid Blaze Id and that means it's a game case
        {
            strCurrentRecordType = RECORD_WIN_TAG; // it's a game, default it to winner EARNing this record

            // determine if last entry is a game and who's the last game winner
            if(strCurrentEntry.length() > 0)
            {
                eastl::string::size_type oldWinnerStartPos = strOldChampHistory.find(RECORD_DELIM, 0);

                // if the last entry type is 'win' or 'defend', then there's chance that this new entry is a 'defend'
                eastl::string strLastEntryType = strOldChampHistory.substr(0, 1);
                if(strLastEntryType.compare(RECORD_WIN_TAG) == 0 || strLastEntryType.compare(RECORD_DEFEND_TAG) == 0)
                {
                    oldWinnerStartPos = strOldChampHistory.find(RECORD_DELIM, oldWinnerStartPos + 1);
                    eastl::string::size_type oldWinnerEndPos = strOldChampHistory.find(RECORD_DELIM, oldWinnerStartPos + 1);
                    eastl::string strWinnerID = strOldChampHistory.substr(oldWinnerStartPos + 1, oldWinnerEndPos - (oldWinnerStartPos + 1));
                    //DEBUG3("last history's winner Id = %s", strWinnerID.c_str());

                    eastl::string strCurrentWinner;
                    strCurrentWinner.sprintf("%x", record->blazeId);

                    if(strCurrentWinner.compare(strWinnerID) == 0)
                    {
                        // the winner defended the record
                        strCurrentRecordType = RECORD_DEFEND_TAG;
                    }
                }
            }

            strParam1.sprintf("%x", static_cast<uint32_t>(record->blazeId));
            strParam2.sprintf("%x", static_cast<uint32_t>(record->statValueInt));
        }
        else
        {
            if(nullptr != gCurrentUserSession)
            {
                // This is a GM reset case.
                strCurrentRecordType = RECORD_RESET_TAG;
                strParam1.sprintf("%x", gCurrentUserSession->getBlazeId());
            }
            //NOTE: for owner no longer belong to club case, the belt history type will get converted afterward, in onMemberRemoved
        }
        strCurrentEntry.sprintf("%s,%s,%s,%s;", strCurrentRecordType.c_str(), strTime.c_str(), strParam1.c_str(), strParam2.c_str());
        TRACE_LOG("[ClubsSlaveImpl::onRecordUpdated] New History record to add = " << strCurrentEntry.c_str());
        recordHistory.push_front(strCurrentEntry);

        /********************************************************
            Put the History list back to the meta-data string
        *********************************************************/

        eastl::string strNewHistory(CHAMP_RECORD_BEGIN_TAG);

        if(pos != metadata.npos)
        {
            // check if there is other meta-data stored behind the Champ History
            eastl::string strOtherMetadata;
            if(endPos + 6 < metadata.npos)
            {
                strOtherMetadata = metadata.substr(endPos + 6);
            }
            
            uint32_t i = 1;
            for(eastl::list<eastl::string>::iterator it = recordHistory.begin();
                it != recordHistory.end() && i <= MAX_CHAMP_HISTORY;
                it++, i++)
            {
                strNewHistory.append(*it);
            }
            strNewHistory.append(CHAMP_RECORD_END_TAG);

            metadata.replace(pos, strNewHistory.size(), strNewHistory);            
        }
        else
        {
            uint32_t i = 0;

            for(eastl::list<eastl::string>::iterator it = recordHistory.begin();
                it != recordHistory.end() && i < MAX_CHAMP_HISTORY;
                it++, i++)
            {
                strNewHistory.append(*it);
            }
            strNewHistory.append(CHAMP_RECORD_END_TAG);
            metadata.append(strNewHistory);
        }

        TRACE_LOG("[ClubsSlaveImpl::onRecordUpdated] Final Meta-data: " << metadata.c_str());

        // TODO - revisit when adding intra-club h2h setting persistence
        MetadataUnion dbMetadata;
        dbMetadata.setMetadataString(metadata.c_str());
        error = clubsDb->updateMetaData(clubId, CLUBS_METADATA_UPDATE, dbMetadata);         
        if (error != Blaze::ERR_OK)
        {
            ERR_LOG("[ClubsSlaveImpl::onRecordUpdated] Failed to update metadata for club " << clubId << ".");
            return error;
        }
        
        TRACE_LOG("[ClubsSlaveImpl::onRecordUpdated] Done onRecordUpdated for club " << clubId << " and record " << record->recordId);
    }
    return error;
    // FIFA SPECIFIC CODE END
}

} // Clubs
} // Blaze
