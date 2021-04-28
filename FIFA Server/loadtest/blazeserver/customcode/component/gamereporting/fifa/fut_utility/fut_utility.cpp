/*************************************************************************************************/
/*!
\file   futgamereportcollator.cpp

\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "fut_utility.h"

namespace Blaze
{
namespace GameReporting
{
namespace FUT
{

/*! ****************************************************************************/
/*! \brief Extract the game type id from the string gameType##.
********************************************************************************/
int32_t GetGameTypeId(const char8_t* gameTypeName)
{
	static size_t gameTypeLen = strlen("gameType");

	if (gameTypeName == NULL || strlen(gameTypeName) < gameTypeLen)
	{
		WARN_LOG("[GetGameTypeId() : Bad game type used:" << gameTypeName);
		return (-1);
	}

	gameTypeName += gameTypeLen;

	return atoi(gameTypeName);
}

}   // namespace FUT
}   // namespace GameReporting
}   // namespace Blaze