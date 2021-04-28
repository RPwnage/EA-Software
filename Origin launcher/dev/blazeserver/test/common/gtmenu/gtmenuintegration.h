/*************************************************************************************************/
/*!
    \file gtmenuintegration.h

    \attention
    (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef GTMENU_INTEGRATION_H
#define GTMENU_INTEGRATION_H
#include <string>
#include "test/common/gtmenu/gtmenuparams.h" // For GTMenuParams mArgs

namespace TestingUtils
{
    class GTMenuIntegration
    {
    public:

        //For GTMenu integration, should print to std out test locations, using your test framework, based on the inputs
        //\param[in] argv Non-serialized interface, uses cmd line parsing on these GTMenu caller cmd line
        //\param[in] args serialized interface, already populated GTMenu integration args
        typedef void(*GTMenuPrintTestLocationsCallback) (int argc, char **argv, const TestingUtils::GTMenuParams& args);

        void init(int argc, char **argv, GTMenuPrintTestLocationsCallback printTestLocationsCb);

        void processGTMenuParams(int argc, char **argv, GTMenuPrintTestLocationsCallback printTestLocationsCb);
        static void checkGTMenuDebugger(uint32_t timeoutMs = 5);

        GTMenuParams mArgs;
    };
    extern GTMenuIntegration gGTMenu;
}

#endif