/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_INPUTFILTER_H
#define BLAZE_INPUTFILTER_H

/*** Include files *******************************************************************************/

#include "EASTL/vector.h"
#include "framework/tdf/frameworkconfigtypes_server.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

#ifdef EA_PLATFORM_WINDOWS

#include <regex> // C++ 11 has built-in multi-platform regex even Windows can use
typedef std::regex* regex_t;
#define REG_NOSUB std::regex::flag_type::nosubs
#define REG_ICASE std::regex::flag_type::icase
#define REG_EXTENDED std::regex::flag_type::extended

#else
#include <regex.h>
#endif
namespace Blaze
{
class InputFilter
{
public:
    InputFilter()
        : mIgnoreCase(false),
        mRegexType(REGEX_NONE),
        mInput(nullptr)
#ifdef EA_PLATFORM_WINDOWS
        , mRegex(nullptr)
#endif
    {
    }
    ~InputFilter();
    const char8_t* getInput() const;
    bool regexEnabled() const;
    bool initialize(const char8_t* inputString, bool ignoreCase, RegexType regexType);
    bool match(const char8_t* input) const;

private:
    bool mIgnoreCase;
    RegexType mRegexType;
    char8_t* mInput;
    regex_t mRegex;
};

} // Blaze

#endif // BLAZE_INPUTFILTER_H
