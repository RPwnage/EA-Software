/*************************************************************************************************/
/*!
    \file   fut_lastopponentextensions.h

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_CUSTOM_FUT_LASTOPPONENTEXTENSIONS_H
#define BLAZE_CUSTOM_FUT_LASTOPPONENTEXTENSIONS_H

/*** Include files *******************************************************************************/

#include "EASTL/hash_set.h"
#include "EASTL/vector.h"
#include "EATDF/tdfobjectid.h"
#include "gamemanager/tdf/gamemanager.h"
#include "gamereporting/fifa/fifa_lastopponent/fifa_lastopponentextensions.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

namespace GameReporting
{

// Not inheriting from HeadToHeadLastOpponent because it does not have a virtual destructor
class FutOtpLastOpponent : public IFifaLastOpponentExtension
{
public:
	typedef eastl::hash_set<GameManager::PlayerId> ResultSet;
	struct ExtensionData
	{
		ProcessedGameReport*	mProcessedReport;
	};
	FutOtpLastOpponent();
	virtual ~FutOtpLastOpponent();

	virtual void setExtensionData(void* extensionData) override;

	virtual const char8_t* getStatCategory() override;
	virtual void getEntityIds(eastl::vector<int64_t>& ids) override;
	virtual EntityId getOpponentId(EntityId myEntityId) override;

private:
	ExtensionData mExtensionData;
};
}   // namespace GameReporting
}   // namespace Blaze

#endif  // BLAZE_CUSTOM_FUT_LASTOPPONENTEXTENSIONS_H

