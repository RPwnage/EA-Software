/*************************************************************************************************/
/*!
    \file

    \attention
    (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#include "test/common/gtmenu/gtmenuintegration.h"

#include <EAStdC/EAString.h> // for AtoU32 in processGTMenuParams
#include <EASTL/string.h>
#if defined(EA_DEBUG) && defined (EA_PLATFORM_WINDOWS)
#include <Windows.h>
#include <chrono> // std::chrono::system_clock
#include <eathread/eathread.h> // for ThreadSleep in checkGTMenuDebugger
#endif

namespace TestingUtils
{
    GTMenuIntegration gGTMenu;

    void GTMenuIntegration::init(int argc, char **argv, GTMenuPrintTestLocationsCallback printTestLocationsCb)
    {
        processGTMenuParams(argc, argv, printTestLocationsCb);
    }

    void GTMenuIntegration::processGTMenuParams(int argc, char **argv, GTMenuPrintTestLocationsCallback printTestLocationsCb)
    {
        auto paramNames = mArgs.paramnames();

        fflush(stdout);
        for (int i = 1; i < argc; ++i)
        {
            printf("arg %2d = %s\n", i, argv[i]);//just a debug msg
            eastl::string arg = argv[i];

            // Fix up and cache GTMenParams
            if (arg.substr(0, paramNames.reportroot().length()).comparei(paramNames.reportroot().c_str()) == 0)
            {
                mArgs.mReportRoot = arg.substr(paramNames.reportroot().length() + 1).c_str();
            }

            if (arg.substr(0, paramNames.debuggertimeout().length()).comparei(paramNames.debuggertimeout().c_str()) == 0)
            {
                mArgs.mDebuggerTimeoutS = EA::StdC::AtoU32(arg.substr(paramNames.debuggertimeout().length() + 1).c_str());
                if (mArgs.mDebuggerTimeoutS != 0)
                {
                    checkGTMenuDebugger(mArgs.mDebuggerTimeoutS);
                }
            }

            if ((printTestLocationsCb != nullptr) &&
                arg.substr(0, paramNames.printtestlocationslinestart().length()).comparei(paramNames.printtestlocationslinestart().c_str()) == 0)
            {
                auto listTestLocationsStr = arg.substr(paramNames.printtestlocationslinestart().length() + 1);
                mArgs.mPrintTestLocations = (listTestLocationsStr.comparei("true") == 0);
                if (mArgs.mPrintTestLocations)
                {
                    printf("\n");//just in case, ensure separated from other lines
                    printTestLocationsCb(argc, argv, mArgs);
                    printf("\n");
                }
            }
        }
        fflush(stdout);
    }

    // Helper to allow attaching GTMenu debugger.
    void GTMenuIntegration::checkGTMenuDebugger(uint32_t timeoutS)
    {
        if (timeoutS == 0)
            return;
#if defined(EA_DEBUG) && defined (EA_PLATFORM_WINDOWS)
        auto startTime = std::chrono::system_clock::now();
        while (!IsDebuggerPresent())
        {
            auto currTime = std::chrono::system_clock::now();
            std::chrono::duration<double> diff = currTime - startTime;
            auto diffInS = std::chrono::duration<double>(currTime.time_since_epoch()).count();
            if (diffInS >= timeoutS)
            {
                break;
            }
            Sleep(100);
        }
        //TODO: IsDebuggerPresent() workaround: using this to ensure sufficient delay for now for it to hookup before run:
        EA::Thread::ThreadSleep(timeoutS * 1000);
        return;
#endif
    }


}//ns
