/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

#include "framework/blaze.h"
#include "stress.h"
#include "TestScenarios/playermodule.h"

namespace Blaze
{
namespace Stress
{

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

/* Declare all the available stress modules and their creator functions.
 */
ModuleCreationInfo gModuleCreators[] =
{
	{ "FifaModule", &PlayerModule::create },
    { NULL, NULL }
};

} // namespace Stress
} // namespace Blaze


