#ifndef BLAZE_CUSTOM_FIFACLUBTAGUPDATER_H
#define BLAZE_CUSTOM_FIFACLUBTAGUPDATER_H

/*** Include files *******************************************************************************/
#include "gamereporting/fifa/fifaclubbasereportslave.h"
#include "gamereporting/fifa/fifacustomclub.h"

#include "gamereporting/fifa/fifa_seasonalplay/fifa_seasonalplay.h"
#include "gamereporting/fifa/fifa_seasonalplay/fifa_seasonalplayextensions.h"
#include "gamereporting/fifa/fifa_elo_rpghybridskill/fifa_elo_rpghybridskill.h"
#include "gamereporting/fifa/fifa_elo_rpghybridskill/fifa_elo_rpghybridskillextensions.h"
#include "gamereporting/fifa/fifa_vpro/fifa_vpro.h"

namespace Blaze
{
namespace GameReporting
{
namespace FIFA
{

class FifaClubTagUpdater {
public:
	FifaClubTagUpdater();
	bool8_t transformTags();

	void setExtension(ClubSeasonsExtension* clubsExtension);
protected:
	bool8_t intialize();
	bool8_t changeClubDivision(Clubs::ClubTagList& tagList, int64_t newDivision);
	bool    getClub(Clubs::ClubId clubId, Clubs::Club& club);
	bool    sendUpdateRequest(Clubs::ClubId clubId, const Clubs::Club &club);

	Clubs::ClubsSlave*    mComponent;
	ClubSeasonsExtension* mClubsExtension;
};

}   // namespace FIFA 
}   // namespace GameReporting
}   // namespace Blaze

#endif  // BLAZE_CUSTOM_FIFACLUBTAGUPDATER_H

