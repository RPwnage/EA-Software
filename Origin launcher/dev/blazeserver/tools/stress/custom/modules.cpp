/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

#include "framework/blaze.h"
#include "tools/stress/stress.h"

#ifdef TARGET_osdkdynamicmessaging 
#include "osdkdynamicmessaging/stress/osdkdynamicmessagingmodule.h" 
#endif 

#ifdef TARGET_osdkticker2 
#include "osdkticker2/stress/osdkticker2module.h" 
#endif 

#ifdef TARGET_osdkliveevents 
#include "osdkliveevents/stress/osdkliveeventsmodule.h" 
#endif 

#ifdef TARGET_osdkweboffersurvey 
#include "osdkweboffersurvey/stress/osdkweboffersurveymodule.h" 
#endif 

#ifdef TARGET_osdkseasonalplay 
#include "osdkseasonalplay/stress/osdkseasonalplaymodule.h" 
#endif 

namespace Blaze
{
namespace Stress
{

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

/* Declare all the available stress modules and their creator functions.
 */
ModuleCreationInfo gCustomModuleCreators[] =
{
#ifdef TARGET_osdkliveevents 
    { "osdkliveevents", &OSDKLiveEventsModule::create }, 
#endif 
#ifdef TARGET_osdkdynamicmessaging  
    { "osdkdynamicmessaging", &OsdkDynamicMessagingModule::create }, 
#endif 
#ifdef TARGET_osdkticker2 
    { "osdkticker2", &OsdkTicker2Module::create }, 
#endif 
#ifdef TARGET_osdkweboffersurvey 
    { "osdkweboffersurvey", &OsdkWebOfferSurveyModule::create }, 
#endif 
#ifdef TARGET_osdkseasonalplay 
    { "osdkseasonalplay", &OSDKSeasonalPlayModule::create }, 
#endif
    { nullptr, nullptr }
};

} // namespace Stress
} // namespace Blaze
