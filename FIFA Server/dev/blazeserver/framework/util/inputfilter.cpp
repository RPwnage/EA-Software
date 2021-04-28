/*************************************************************************************************/
/*!
    \file   inputfilter.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class InputFilter

    Input Filter Implementation.

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"
#include "framework/util/inputfilter.h"
#include "framework/util/shared/blazestring.h"
#include "framework/util/locales.h"

#ifdef EA_PLATFORM_WINDOWS
int regcomp(regex_t *preg, const char *pattern, int cflags)
{
#if EASTL_EXCEPTIONS_ENABLED
    try
    {
#endif
        *preg = BLAZE_NEW std::regex(pattern, (std::regex::flag_type)cflags);
#if EASTL_EXCEPTIONS_ENABLED
    }
    catch (const std::regex_error& e) 
    {
        BLAZE_ERR_LOG(Blaze::Log::UTIL, "[InputFilter:].initialize: unable to compile regular expression '"
            << pattern << "': " << e.what());
        return 1;
    }
#endif
    return 0;
}

int regexec(const regex_t *preg, const char *string, size_t nmatch, void* pmatch, int eflags)
{
    std::cmatch m;
    auto result = std::regex_search(string, m, **preg);
    return result ? 0 : 1;
}

size_t regerror(int errcode, const regex_t *preg, char *errbuf, size_t errbuf_size)
{
    if (errbuf_size > 0)
        *errbuf = 0;
    return 0;
}

void regfree(regex_t *preg)
{
    delete *preg;
}
#endif

namespace Blaze
{
InputFilter::~InputFilter()
{
    if (regexEnabled())
        regfree(&mRegex);
    BLAZE_FREE(mInput);
}

const char8_t* InputFilter::getInput() const
{
    return mInput;
}

bool InputFilter::regexEnabled() const
{
    return ((mRegexType != REGEX_NONE) && (mRegexType != REGEX_STRMATCH));
}

bool InputFilter::initialize(const char8_t* inputString, bool ignoreCase, RegexType regexType)
{
    mIgnoreCase = ignoreCase;
    mRegexType = regexType;
    mInput = (inputString == nullptr) ? nullptr : blaze_strdup(inputString);
    if (regexEnabled())
    {
        if (mInput == nullptr)
        {
            // Disable regex if there isn't any input string provided
            mRegexType = REGEX_NONE;
        }
        else
        {
            int32_t flags = REG_NOSUB;
            if (mIgnoreCase)
                flags |= REG_ICASE;
            switch (mRegexType)
            {
                case REGEX_EXTENDED:
                    flags |= REG_EXTENDED;
                    break;
                case REGEX_NONE:
                case REGEX_NORMAL:
                case REGEX_STRMATCH:
                default:
                    break;
            }
            int32_t rc = regcomp(&mRegex, mInput, flags);
            if (rc != 0)
            {
                char8_t errBuf[256];
                regerror(rc, &mRegex, errBuf, sizeof(errBuf));
                BLAZE_ERR_LOG(Log::UTIL, "[InputFilter:].initialize: unable to compile regular expression '" 
                        << mInput << "': " << errBuf);
                mRegexType = REGEX_NONE;
                return false;
            }
        }
    }
    return true;
}

bool InputFilter::match(const char8_t* input) const
{
    if ((mInput == nullptr) || (*mInput == '\0'))
        return true;

    if ((input == nullptr) || (*input == '\0'))
        return false;

    //only handles prefix and full match 
    if (mRegexType == REGEX_STRMATCH)
    {
        auto strLen = strlen(mInput);
        
        //Prefix Match
        if (mInput[0] == '^')
        {
            if (mIgnoreCase)
            {
                return blaze_strnicmp(mInput+1, input, strLen-1) == 0;
            }                
            return blaze_strncmp(mInput+1, input, strLen-1) == 0;
        }

        //Full Match
        else 
        { 
            if (mIgnoreCase)
            {
                return blaze_stricmp(mInput, input) == 0;
            }
            return blaze_strcmp(mInput, input) == 0;
        }
    }

    if (mRegexType == REGEX_NONE)
    {
        if (mIgnoreCase)
            return (blaze_stricmp(input, mInput) == 0);
        return (strcmp(input, mInput) == 0);
    }

    if (mInput[0] == '^' && isalpha(mInput[1]))
    {
        // NOTE: This quick early out may seem redundant given that regexec
        // evaluation always correctly handles this case; however,
        // relying on regexec(which allocates memory) for this very common 
        // mismatch runs *many* thousands of times slower!
        if (input[0] != mInput[1])
        {
            bool caseSensitive = !mIgnoreCase;
            if (caseSensitive)
                return false;
            if (tolower(input[0]) != tolower(mInput[1]))
                return false;
        }
    }

    return (regexec(&mRegex, input, 0, nullptr, 0) == 0);
}
} // namespace Blaze
