/*************************************************************************************************/
/*!
    \file

    \attention
    (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#include "test/common/testinghooks.h"

namespace TestingUtils
{
    TestingHooks gTesting;

    // Call to install logging helpers
    void TestingHooks::init(
        LogCallback logCb,
        CheckHasErrCallback checkHasErrCb)
    {
        mLogCb = logCb;
        mCheckHasErrCb = checkHasErrCb;
    }

    // logs error via your log cb, with file, line. called via the testerr() macro.
    void TestingHooks::err(const char8_t* file, int line, const char8_t* fmt, ...) const
    {
        if (mLogCb != nullptr)
        {
            mErr.clear();
            va_list args;
            va_start(args, fmt);
            mErr.append_sprintf_va_list(fmt, args);
            mErr.append("\n");
            va_end(args);
            mLogCb(file, line, mErr.c_str(), LogLevel::ERR);
        }
    }
    // logs trace via your log cb, with file, line. called via the testlog() macro.
    // Dispatches the installed LogCallback, to add a trace log line. Called by testlog macro.
    void TestingHooks::trace(const char8_t* file, int line, const char8_t* fmt, ...) const
    {
        if (mLogCb != nullptr)
        {
            mLog.clear();
            va_list args;
            va_start(args, fmt);
            mLog.append_sprintf_va_list(fmt, args);
            mLog.append("\n");
            va_end(args);
            mLogCb(file, line, mLog.c_str(), LogLevel::TRACE);
        }
    }
    // Dispatches the installed CheckForErrCallback, to return whether current test had any err.
    bool TestingHooks::hasErr() const
    {
        return (mCheckHasErrCb != nullptr ? mCheckHasErrCb() : false);
    }
    
    
    void TestingHooks::defaultLogCallback(const char8_t* file, int line, const char8_t* msg, LogLevel level)
    {
        if (msg == nullptr)
            return;
        switch (level)
        {
        case LogLevel::TRACE:
            printf("\n[TestingHooks] %s", msg);
            break;
        case LogLevel::ERR:
            printf("\n[TestingHooks] ERR: %s", msg);
            break;
        default:
            break;
        }
    }

    bool TestingHooks::defaultCheckHasErrCallback()
    {
        if (!TestingUtils::gTesting.mErr.empty())
            return true;
        return false;
    }



}//ns
