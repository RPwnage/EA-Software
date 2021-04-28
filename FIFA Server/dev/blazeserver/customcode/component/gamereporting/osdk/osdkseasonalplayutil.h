/*************************************************************************************************/
/*!
    \file   osdkseasonalplayutil.h
    \attention
        (c) Electronic Arts. All Rights Reserved
*/
/*************************************************************************************************/
#ifndef BLAZE_GAMEREPORTING_OSDKSEASONALPLAYUTIL
#define BLAZE_GAMEREPORTING_OSDKSEASONALPLAYUTIL

#include "osdkseasonalplay/tdf/osdkseasonalplaytypes.h"

namespace Blaze
{

namespace GameReporting
{

    class OSDKSeasonalPlayUtil
    {
    public:
        OSDKSeasonalPlayUtil();

        ~OSDKSeasonalPlayUtil() {}

        /*************************************************************************************************/
        /*!
            \brief setMember

            Sets the current season member.

            \input - memberId : the id of the member
            \input - memberType : the type of the member (user, club, etc.)
        */
        /*************************************************************************************************/
        void setMember(OSDKSeasonalPlay::MemberId memberId, OSDKSeasonalPlay::MemberType memberType);

        /*************************************************************************************************/
        /*!
            \brief getSeasonId

            Returns the id of the season associated with the set member.

            \return - the entity's season id (INVALID if not found or unset member)
        */
        /*************************************************************************************************/
        OSDKSeasonalPlay::SeasonId getSeasonId() const;

        /*************************************************************************************************/
        /*!
            \brief getSeasonState

            Returns the state of the season associated with the set member.

            \return - the entity's season state (NONE if not found or unset member)
        */
        /*************************************************************************************************/
        OSDKSeasonalPlay::SeasonState getSeasonState() const;

        /*************************************************************************************************/
        /*!
            \brief getStatsCategory

            Returns the stats category name of the season associated with the set member

            \input strCategoryName - a pointer to an allocated buffer that will be filled with the category name
            \input uSize - available buffer size

            \return - a boolean value indicating if the category name could be retrieved
        */
        /*************************************************************************************************/
        bool getSeasonStatsCategory(char8_t *strCategoryName, uint32_t uSize) const;

        /*************************************************************************************************/
        /*!
            \brief getSeasonTournamentId

            Helper function for returning the tournament id associated with the season.

            \return - the season's tournament id (INVALID if not found or unset member)
        */
        /*************************************************************************************************/
        OSDKTournaments::TournamentId getSeasonTournamentId() const;

    private:
        /*************************************************************************************************/
        /*!
            \brief getSeasonDetails

            Helper function for returning the detailed information for the season.

            \input - details : Contains the detailed information if the return value is true
            \return - Whether the detailed information is retrieved successfully or not
        */
        /*************************************************************************************************/
        bool getSeasonDetails(OSDKSeasonalPlay::SeasonDetails& details) const;

         /*************************************************************************************************/
        /*!
            \brief getSeasonConfiguration

            Helper function for returning the season configuration data.

            \input - config : Contains the configuration data if the return value is true
            \return - Whether the configuration data is retrieved successfully or not
        */
        /*************************************************************************************************/
        bool getSeasonConfiguration(OSDKSeasonalPlay::SeasonConfiguration& config) const;

    private:
        OSDKSeasonalPlay::OSDKSeasonalPlaySlave* mSeasonalPlaySlave;
        
        OSDKSeasonalPlay::MemberId      mMemberId;
        OSDKSeasonalPlay::MemberType    mMemberType;

        OSDKSeasonalPlay::SeasonId      mSeasonId;

    };
} //namespace GameReporting
} //namespace Blaze

#endif //BLAZE_GAMEREPORTING_OSDKSEASONALPLAYUTIL
