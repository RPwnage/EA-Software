/*************************************************************************************************/
/*!
    \file gtmenuparams.h

    \attention
    (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef GTMENU_PARAMS_H
#define GTMENU_PARAMS_H
#include <string>

namespace TestingUtils
{
    //Parameters passed by GTMenu tool to the test framework/program (e.g gtest).
    class GTMenuParams
    {
    public:
        class GTMenuParamNames
        {
        public:
            //Directory where GTMenu expects test logs to be outputted under
            static const std::string reportroot() { return "GTMenuReportRoot"; }

            //By contract, test program may use this to attach and timeout the attempt to attach to debugger. Timeout is in seconds
            static const std::string debuggertimeout() { return "GTMenuDebuggerTimeoutS"; }

            //By contract, response's lines should be prefixed with this, for GTMenu to identify it as file-location data
            static const std::string printtestlocationslinestart() { return "GTMenuPrintTestLocations"; }

            struct PrintTestLocationsParams
            {
                static const std::string projectparam() { return "project"; }
                static const std::string fixtureparam() { return "fixture"; }
                static const std::string testparam() { return "test"; }
                static const std::string fileparam() { return "file"; }
                static const std::string lineparam() { return "line"; }
                static const std::string genparams() { return "genparams"; }
            };
            static const PrintTestLocationsParams printtestlocation() { return PrintTestLocationsParams(); }
        };
        const GTMenuParamNames paramnames() const { return GTMenuParamNames(); }

        std::string mReportRoot;
        uint32_t mDebuggerTimeoutS;
        bool mPrintTestLocations;
    };
}

#endif