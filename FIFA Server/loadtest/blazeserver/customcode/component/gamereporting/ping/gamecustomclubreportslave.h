/*************************************************************************************************/
/*!
    \file   gamecustomclubreportslave.h

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_CUSTOM_GAMECUSTOMCLUBREPORT_SLAVE_H
#define BLAZE_CUSTOM_GAMECUSTOMCLUBREPORT_SLAVE_H

/*** Include files *******************************************************************************/
#include "gamereporting/osdk/gameclubreportslave.h"
#include "gamereporting/ping/gamecustomclub.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{
namespace GameReporting
{

/*!
    class GameCustomClubReportSlave
*/
class GameCustomClubReportSlave: public GameClubReportSlave
{
public:
    GameCustomClubReportSlave(GameReportingSlaveImpl& component);
    static GameReportProcessor* create(GameReportingSlaveImpl& component);

    virtual ~GameCustomClubReportSlave();

protected:
    /*! ****************************************************************************/
    /*! \Pure virtual functions from GameClubReportSlave class.
    ********************************************************************************/
        virtual const char8_t* getCustomClubGameTypeName() const;
    /* A custom hook for Club Rival feature that game team needs to update winner club player's stats based on challenge type */ 
        virtual void updateCustomClubRivalChallengeWinnerPlayerStats(uint64_t challengeType, OSDKClubGameReportBase::OSDKClubPlayerReport& playerReport);
    /* A custom hook for Club Rival feature that game team needs to update winner club's stats based on challenge type */ 
        virtual void updateCustomClubRivalChallengeWinnerClubStats(uint64_t challengeType, OSDKClubGameReportBase::OSDKClubClubReport& clubReport);


    /*! ****************************************************************************/
    /*! \Virtual functions game team can overwrite with custom functionalities
    ********************************************************************************/
    /* A custom hook for game team to process for club records based on game report and cache the result in mNewRecordArray, called during updateClubStats() */
        virtual bool updateCustomClubRecords(int32_t iClubIndex, OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& OsdkPlayerReportsMap);
    /* A custom hook for game team to give club awards based on game report, called during updateClubStats() */
        virtual bool updateCustomClubAwards(Clubs::ClubId clubId,  OSDKClubGameReportBase::OSDKClubClubReport& clubReport, OSDKGameReportBase::OSDKReport& OsdkReport);
    /* A custom hook for game team to preform custom process for post stats update, ie. awards based on win streak*/
        virtual bool processCustomUpdatedStats();

};

}   // namespace GameReporting
}   // namespace Blaze

#endif  // BLAZE_CUSTOM_GAMECUSTOMCLUBREPORT_SLAVE_H

