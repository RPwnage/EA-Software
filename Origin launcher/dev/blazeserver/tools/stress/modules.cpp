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
#ifdef TARGET_arson
#include "arson/stress/arsonmodule.h"
#endif
#ifdef TARGET_stats
#include "stats/stress/statsmodule.h"
#endif

#ifdef TARGET_messaging
#include "messaging/stress/messagingmodule.h"
#endif

#ifdef TARGET_gamemanager
#include "gamemanager/stress/gamebrowsermodule.h"
#include "gamemanager/stress/gamemanagermodule.h"
#include "gamemanager/stress/matchmakermodule.h"
#include "gamemanager/stress/dedicatedservermodule.h"
#include "gamemanager/stress/gamegroupsmodule.h"
#endif

#ifdef TARGET_clubs
#include "clubs/stress/clubsmodule.h"
#endif

#ifdef TARGET_gamereporting
#include "gamereporting/stress/gamereportingmodule.h"
#endif 

#ifdef TARGET_associationlists
#include "associationlists/stress/associationlistsmodule.h"
#endif

#ifdef TARGET_util
#include "util/stress/utilmodule.h"
#endif

#ifdef TARGET_example
#include "example/stress/examplemodule.h"
#endif

#ifdef TARGET_redirector
#include "proxycomponent/redirector/stress/redirectormodule.h"
#endif



namespace Blaze
{
namespace Stress
{

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

/* Declare all the available stress modules and their creator functions.
 */
ModuleCreationInfo gModuleCreators[] =
{
#ifdef TARGET_arson
    { "arson", &ArsonModule::create },
#endif
#ifdef TARGET_stats
    { "stats", &StatsModule::create },
#endif
#ifdef TARGET_messaging
    { "messaging", &MessagingModule::create },
#endif
#ifdef TARGET_gamemanager
    { "gamebrowser", &GameBrowserModule::create },
    { "gamemanager", &GameManagerModule::create },
    { "matchmaker", &MatchmakerModule::create },
    { "dedicatedserver", &DedicatedServerModule::create },
    { "gamegroups", &GamegroupsModule::create },
#endif
#ifdef TARGET_clubs
    { "clubs", &ClubsModule::create },
#endif
#ifdef TARGET_gamereporting
    { "gamereporting", &GameReportingModule::create },
#endif
#ifdef TARGET_associationlists
    { "associationlists", &AssociationListsModule::create },
#endif

#ifdef TARGET_util
    { "util", &UtilModule::create },
#endif

#ifdef TARGET_example
    { "example", &ExampleModule::create },
#endif

#ifdef TARGET_redirector
    { "redirector", &RedirectorModule::create },
#endif

    { nullptr, nullptr }
};

} // namespace Stress
} // namespace Blaze


