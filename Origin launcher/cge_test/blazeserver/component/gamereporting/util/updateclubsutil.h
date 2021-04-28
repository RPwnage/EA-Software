/*************************************************************************************************/
/*!
    \file   updateclubsutil.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/
#ifndef BLAZE_GAMEREPORTING_UPDATECLUBSUTIL
#define BLAZE_GAMEREPORTING_UPDATECLUBSUTIL

#include "framework/database/dbscheduler.h"
#include "framework/database/dbconn.h"

#ifdef TARGET_clubs
#include "clubs/clubsslaveimpl.h"
#include "clubs/tdf/clubs.h"
#endif

namespace Blaze
{
namespace GameReporting
{
#ifdef TARGET_clubs
    class UpdateClubsUtil
    {
    public:
        UpdateClubsUtil()
            : mInitialized(false),
              mTransactionContextId(0),
              mComponent(nullptr)
        {
        }

        ~UpdateClubsUtil()
        {
            completeTransaction(false);
        }

        bool initialize(Clubs::ClubIdList &clubsToLock);

        void completeTransaction(bool success);

        bool updateClubs(Clubs::ClubId firstClubId, int32_t firstClubScore,
                         Clubs::ClubId secondClubId, int32_t secondClubScore);

        bool fetchClubRecords(Clubs::ClubId clubId);
        bool getClubRecord(Clubs::ClubRecordId recordId, Clubs::ClubRecordbook &record);

        bool updateClubRecordInt(Clubs::ClubId clubId,Clubs::ClubRecordId recordId, 
                                 BlazeId newRecordHolder, int64_t recordStatVal);
        bool updateClubRecordFloat(Clubs::ClubId clubId, Clubs::ClubRecordId recordId, 
                                   BlazeId newRecordHolder, float_t recordStatVal);
        bool updateClubRival(Clubs::ClubId clubId, Clubs::ClubRival &rival);

        bool awardClub(Clubs::ClubId clubId, Clubs::ClubAwardId awardId);
        bool checkMembership(Clubs::ClubId clubId, BlazeId blazeId);
        bool validateClubId(Clubs::ClubId clubId);
        bool checkUserInClub(BlazeId blazeId);
        bool getClubSettings(Clubs::ClubId clubId, Clubs::ClubSettings &clubSettings);     
        bool getClubsComponentSettings(Clubs::ClubsComponentSettings &settings);
        bool listClubRivals(Clubs::ClubId clubId, Clubs::ClubRivalList &rivalList);            
        bool listClubMembers(Clubs::ClubId clubId, Clubs::ClubMemberList &memberList);

    private:
        bool mInitialized;
        TransactionContextId mTransactionContextId;
        Clubs::ClubRecordbookMap mRecordMap;

        Clubs::ClubsSlave* mComponent;
    };
#endif
} //namespace GameReporting
} //namespace Blaze

#endif
