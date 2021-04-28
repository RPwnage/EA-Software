/*************************************************************************************************/
/*!
    \file regexutil.h

    \attention
    (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef TESTINGUTILS_REGEX_UTIL_H
#define TESTINGUTILS_REGEX_UTIL_H

#include <EABase/eabase.h>
#if defined(EA_COMPILER_CPP11_ENABLED)
#include <regex> // C++ 11 has built-in multi-platform regex even Windows can use
#endif

namespace TestingUtils
{
    class RegexUtil
    {
    public:
        static bool isMatch(const char8_t *searchIn, const char8_t* expr)
        {
        #ifdef EA_COMPILER_CPP11_ENABLED
            std::regex e(expr, std::regex_constants::basic);
            std::cmatch m;
            auto result = std::regex_search(searchIn, m, e);
            return result;
        #else
            // This won't really fully regex, but everyone should be on C++/11 already anyhow:
            return (blaze_strstr(searchIn, expr) != nullptr);
        #endif
        }
    };
}

#endif