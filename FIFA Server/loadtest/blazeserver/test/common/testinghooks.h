/*************************************************************************************************/
/*!
    \file testingutil.h

    \attention
    (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/
#ifndef TESTINGUTILS_TESTING_HOOKS_H
#define TESTINGUTILS_TESTING_HOOKS_H

#include <EASTL/string.h>

// Error/log hooks/wrappers. Used in GamePackerTool also//TODO: reformat

// Macro to log test error, with the current file and line number.
#define testerr(msg, ...) TestingUtils::gTesting.err(__FILE__, __LINE__, msg, ##__VA_ARGS__)
#define testerr_macroCall( x ) x
#define has_testerr() TestingUtils::gTesting.hasErr()

#define testlog(msg, ...) TestingUtils::gTesting.trace(__FILE__, __LINE__, msg, ##__VA_ARGS__)
#define testlog_macroCall( x ) x

// Macro to early out of current method, if has testerr.
#define return_if_testerr() { if (has_testerr()) { testerr("early out due to testerr."); return; } }
#define return_if_testerr_msg(msg, ...) { if (has_testerr()) { testerr_macroCall(testerr(msg, __VA_ARGS__)); return; } }


namespace TestingUtils
{
    class TestingHooks
    {
    public:

        enum class LogLevel { TRACE, ERR, E_COUNT };
        //Log hook
        typedef void(*LogCallback) (const char8_t* file, int line, const char8_t *msg, LogLevel level);

        //Hook for checking if test has had an error
        typedef bool(*CheckHasErrCallback) ();

        void init(LogCallback logCb = defaultLogCallback, CheckHasErrCallback checkHasErrCb = defaultCheckHasErrCallback);

        void err(const char8_t* file, int line, const char8_t* fmt, ...) const;
        void trace(const char8_t* file, int line, const char8_t* fmt, ...) const;
        bool hasErr() const;

    private:
        static void defaultLogCallback(const char8_t* file, int line, const char8_t* msg, LogLevel level);
        static bool defaultCheckHasErrCallback();
        LogCallback mLogCb;
        CheckHasErrCallback mCheckHasErrCb;
        mutable eastl::string mErr;
        mutable eastl::string mLog;
    };

    // Main test util
    extern TestingHooks gTesting;

    enum VerifyBool { VERIFY_FALSE = 0, VERIFY_TRUE = 1, VERIFY_NOTHING };//non gtest-clashing names
}

#endif