/////////////////////////////////////////////////////////////////////////////
// CommandLine.cpp
//
// Copyright (c) Electronic Arts Inc. All rights reserved.
//
// Implements basic command line parsing.
/////////////////////////////////////////////////////////////////////////////


#include <EAPatchMakerApp/CommandLine.h>
#include <EAPatchClient/Allocator.h>
#include <EAStdC/EAProcess.h>
#include <EAStdC/EAString.h>
#include <EAStdC/EACType.h>
#include <stddef.h> // size_t definition.
#include <string.h> // strlen, etc.
#include EA_ASSERT_HEADER

#if defined(EA_PLATFORM_WINDOWS)
    #pragma warning(push, 0)
    #include <windows.h>
    #pragma warning(pop)
#elif defined(EA_PLATFORM_XENON)
    #pragma warning(push, 1)
    #include <xtl.h>
    #pragma warning(pop)
#endif


namespace EA
{
    namespace Patch
    {
        namespace Local
        {
            const int kArgumentCountMax  = 100000;

            const char8_t kSwitchIDs[]     = { '-', '/' };
            const int     kSwitchIDCount   = EAArrayCount(kSwitchIDs);

            const char8_t kWhitespace[]    = { ' ', '\t', '\n', '\r' };
            const int     kWhitespaceCount = EAArrayCount(kWhitespace);

            inline bool IsWhiteSpace(char8_t c)
            {
                for(int i(0); i < kWhitespaceCount; ++i)
                {
                    if(c == kWhitespace[i])
                        return true;
                }
                return false;
            }
        }
    }
}


namespace EA
{
namespace Patch
{


/////////////////////////////////////////////////////////////////////////////
// CommandLine
/////////////////////////////////////////////////////////////////////////////

CommandLine::CommandLine(EA::Allocator::ICoreAllocator* pCoreAllocator)
  : mStringArray(CoreAllocator(EAPATCHMAKER_ALLOC_PREFIX "CommandLine", pCoreAllocator ? pCoreAllocator : EA::Patch::GetAllocator())),
    msCommandLine(CoreAllocator(EAPATCHMAKER_ALLOC_PREFIX "CommandLine", pCoreAllocator ? pCoreAllocator : EA::Patch::GetAllocator())),
    msEmpty(CoreAllocator(EAPATCHMAKER_ALLOC_PREFIX "CommandLine", pCoreAllocator ? pCoreAllocator : EA::Patch::GetAllocator()))
{
    // Empty
}


CommandLine::CommandLine(const char8_t* pCommandLineString, EA::Allocator::ICoreAllocator* pCoreAllocator)
  : mStringArray(CoreAllocator(EAPATCHMAKER_ALLOC_PREFIX "CommandLine", pCoreAllocator ? pCoreAllocator : EA::Patch::GetAllocator())),
    msCommandLine(CoreAllocator(EAPATCHMAKER_ALLOC_PREFIX "CommandLine")),
    msEmpty(CoreAllocator(EAPATCHMAKER_ALLOC_PREFIX "CommandLine"))
{
    SetCommandLineText(pCommandLineString);
}


CommandLine::CommandLine(int nArgumentCount, char8_t** ppArgumentArray, EA::Allocator::ICoreAllocator* pCoreAllocator)
  : mStringArray(CoreAllocator(EAPATCHMAKER_ALLOC_PREFIX "CommandLine", pCoreAllocator ? pCoreAllocator : EA::Patch::GetAllocator())),
    msCommandLine(CoreAllocator(EAPATCHMAKER_ALLOC_PREFIX "CommandLine")),
    msEmpty(CoreAllocator(EAPATCHMAKER_ALLOC_PREFIX "CommandLine"))
{
    using namespace EA::Patch::Local;

    EA_ASSERT_MESSAGE((nArgumentCount > 0) && (nArgumentCount < kArgumentCountMax), "Invalid CommandLine count for CommandLine. Some platforms screw up argc/argv; do you want to use AppCommandLine instead, as it fixes that problem?\n");

    if((nArgumentCount > 0) && (nArgumentCount < kArgumentCountMax))
    {
        mStringArray.resize((eastl_size_t)nArgumentCount, String(CoreAllocator(EASTL_NAME_VAL(EAPATCHMAKER_ALLOC_PREFIX "CommandLine"))));

        if(ppArgumentArray)
        {
            for(eastl_size_t i(0), iEnd((eastl_size_t)nArgumentCount); (i < iEnd) && ppArgumentArray[i]; ++i)
                mStringArray[i] = ppArgumentArray[i];
        }

        // Leave msCommandLine empty until it is asked for.
    }
}


CommandLine::CommandLine(const CommandLine& commandLine)
  : mStringArray(commandLine.mStringArray),
    msCommandLine(commandLine.msCommandLine),
    msEmpty(CoreAllocator(EAPATCHMAKER_ALLOC_PREFIX "CommandLine"))
{
    // Empty
}


CommandLine& CommandLine::operator=(const CommandLine& commandLine)
{
    if(&commandLine != this)
    {
        mStringArray  = commandLine.mStringArray;
        msCommandLine = commandLine.msCommandLine;
    }
    return *this;
}


int CommandLine::GetArgumentCount() const
{
    return (int)mStringArray.size();
}


const String& CommandLine::operator[](int nIndex) const
{
    if((eastl_size_t)nIndex < mStringArray.size())
        return mStringArray[(eastl_size_t)nIndex];
    return msEmpty;
}


String& CommandLine::operator[](int nIndex)
{
    if((eastl_size_t)nIndex < mStringArray.size())
        return mStringArray[(eastl_size_t)nIndex];
    return msEmpty; // This won't be empty any more if the user modifies it.
}


const String& CommandLine::GetCommandLineText() const
{
    if(msCommandLine.empty()) // If it's empty, then it needs to be rebuilt.
        ConvertStringArrayToString(mStringArray, msCommandLine);
    return msCommandLine;
}


void CommandLine::SetCommandLineText(const char8_t* pCommandLineString)
{
    if(pCommandLineString)
        msCommandLine = pCommandLineString;
    else
        msCommandLine.clear();

    ConvertStringToStringArray(msCommandLine, mStringArray);
}


void CommandLine::InsertArgument(const char8_t* pArgument, int nIndex)
{
    const size_t nSize(mStringArray.size());

    if((eastl_size_t)nIndex > nSize)
        nIndex = (int)nSize;

    // To consider: Change this code so it doesn't create a temporary string.
    const String sArgument(pArgument);
    mStringArray.insert(mStringArray.begin() + nIndex, sArgument);
    msCommandLine.clear();
}


bool CommandLine::EraseArgument(int nIndex, bool bResize)
{
    if((eastl_size_t)nIndex >= mStringArray.size())
        return false;

    if(bResize)
        mStringArray.erase(mStringArray.begin() + nIndex);
    else
        mStringArray[(eastl_size_t)nIndex].clear();
    msCommandLine.clear();
    return true;
}


int CommandLine::FindArgument(const char8_t* pArgument, bool bFindCompleteString, bool bCaseSensitive, String* pResult, int nStartingIndex) const
{
    int nReturnValue(kIndexUnknown);

    if(nStartingIndex < 0)
        nStartingIndex = 0;

    eastl_size_t i    = (eastl_size_t)nStartingIndex;
    eastl_size_t iEnd = mStringArray.size();

    for(; i < iEnd; ++i)
    {
        if(bCaseSensitive)
        {
            if(bFindCompleteString)
            {
                if(mStringArray[i] == pArgument)
                {
                    nReturnValue = (int)i;
                    break;
                }
            }
            else
            {
                if(mStringArray[i].find(pArgument) != String::npos)
                {
                    nReturnValue = (int)i;
                    break;
                }
            }
        }
        else{
            if(bFindCompleteString)
            {
                if(EA::StdC::Stricmp(mStringArray[i].c_str(), pArgument) == 0)
                {
                    nReturnValue = (int)i;
                    break;
                }
            }
            else
            {
                if(EA::StdC::Stristr(mStringArray[i].c_str(), pArgument) != 0)
                {
                    nReturnValue = (int)i;
                    break;
                }
            }
        }
    }

    if(pResult)
    {
        if(nReturnValue == kIndexUnknown)
            pResult->clear();
        else
            pResult->assign(mStringArray[i]);
    }

    return nReturnValue;
}


bool CommandLine::GetSwitch(int nIndex, String* pSwitch, String* pResult) const
{
    using namespace EA::Patch::Local;

    if(pSwitch)
        pSwitch->clear();
    if(pResult)
        pResult->clear();

    if((size_t)nIndex < mStringArray.size())
    {
        const String& sArgument = mStringArray[(eastl_size_t)nIndex];

        for(eastl_size_t i(0); i < (eastl_size_t)kSwitchIDCount; ++i)
        {
            if((sArgument.length() >= 2) && (sArgument[0] == kSwitchIDs[i])) // If this argument is a switch (i.e. begins with "-" as in "-x")...
            {
                const eastl_size_t nColonPosition = sArgument.find(':');

                if(nColonPosition == eastl::string8::npos)
                {
                    if(pSwitch)
                        pSwitch->assign(&sArgument[1], sArgument.length() - 1);
                }
                else
                {
                    if(pSwitch)
                        pSwitch->assign(&sArgument[1], (eastl_size_t)(nColonPosition - 1));

                    if(pResult)
                    {
                        if((sArgument[nColonPosition + 1] == '\"') && (sArgument[sArgument.length() - 1] == '\"')) // If surrounded by quotes...
                            pResult->assign(&sArgument[nColonPosition + 2], sArgument.length() - nColonPosition - 3);
                        else
                            pResult->assign(&sArgument[nColonPosition + 1], sArgument.length() - nColonPosition - 1);
                    }
                }

                return true;
            }
        }
    }

    return false;
}


int CommandLine::FindSwitch(const char8_t* pSwitch, bool bCaseSensitive, String* pResult, int nStartingIndex) const
{
    if(pResult)
        pResult->clear();

    if(nStartingIndex < 0)
        nStartingIndex = 0;

    // Here we move the input pSwitch past any one leading switch indicator such as '-'.
    for(int i = 0; i < Local::kSwitchIDCount; ++i)
    {
        if(*pSwitch == Local::kSwitchIDs[i])
        {
            ++pSwitch;
            break;
        }
    }

    const size_t nSwitchLength = EA::StdC::Strlen(pSwitch);
    const size_t nArraySize    = mStringArray.size();

    if(nSwitchLength && ((eastl_size_t)nStartingIndex < mStringArray.size()))
    {
        for(eastl_size_t i = (eastl_size_t)nStartingIndex; i < nArraySize; ++i)
        {
            const String& sCurrent = mStringArray[i];

            if(sCurrent.length() >= 2) // Enough, for example, for "-x".
            {
                int j;

                // Make sure the string starts with a switch ID (e.g. '-').
                for(j = 0; j < Local::kSwitchIDCount; ++j)
                {
                    if(sCurrent[0] == Local::kSwitchIDs[j])
                        break;
                }

                if(j < Local::kSwitchIDCount) // If a leading '-' was found...
                {
                    const char8_t* pCurrent = bCaseSensitive ? EA::StdC::Strstr(sCurrent.c_str() + 1, pSwitch) : EA::StdC::Stristr(sCurrent.c_str() + 1, pSwitch);
                    const char8_t* pCStr    = sCurrent.c_str();

                    if(pCurrent == (pCStr + 1)) // If the user's input switch matched at least the start of the current argument switch...
                    {
                        pCurrent += nSwitchLength; // Move pCurrent past the input switch. 

                        // At this point, we require that *pResult is either 0 or ':'.
                        if((*pCurrent == 0) || (*pCurrent == ':'))
                        {
                            // We have a match. Now possibly return a result string.
                            if(*pCurrent == ':')
                            {
                                if(++pCurrent)
                                {
                                    if(pResult)
                                    {
                                        // Strip any surrounding quotes from the current string during assignment.
                                        const eastl_size_t nLength(sCurrent.length());

                                        if((*pCurrent == '\"') && (sCurrent[nLength - 1] == '\"'))
                                            pResult->assign(pCurrent + 1, (eastl_size_t)(nLength - (pCurrent - pCStr) - 2));
                                        else
                                            pResult->assign(pCurrent);
                                    }
                                }
                            }

                            return (int)i;
                        }
                    }
                }
            }
        }
    }

    return kIndexUnknown;
}


void CommandLine::ConvertStringToStringArray(const String& sInput, StringArray& stringArray)
{
    using namespace EA::Patch::Local;

    eastl_size_t i, iEnd, iCurrentStringStart;
    int          nQuoteLevel;
    String       sCurrentArgument(CoreAllocator(EAPATCHMAKER_ALLOC_PREFIX "CommandLine"));
    String       sInputUsed(sInput);

    stringArray.clear();

    // Remove spaces at the front and rear of the string.
    while(sInputUsed.length() && IsWhiteSpace(sInputUsed[sInputUsed.length() - 1]))
        sInputUsed.erase(sInputUsed.length() - 1, 1);
    while(sInputUsed.length() && IsWhiteSpace(sInputUsed[0]))
        sInputUsed.erase(0, 1);

    if(sInputUsed.length())
    {
        for(i = 0, iEnd = sInputUsed.length(), iCurrentStringStart = 0, nQuoteLevel = 0; i < iEnd; ++i)
        {
            if(IsWhiteSpace(sInputUsed[i])) // If this is potentially an argument ending character.
            {
                if(nQuoteLevel & 1) // If we are in the middle of quotes (an odd number of quotes have been read).
                    continue;
                // Else this is the end of this argument string.

                nQuoteLevel = 0;
                sCurrentArgument.assign(&sInputUsed[iCurrentStringStart], i - iCurrentStringStart);

                if(sCurrentArgument[0] == '\"' && sCurrentArgument[sCurrentArgument.length() - 1] == '\"')
                {
                    sCurrentArgument.erase(sCurrentArgument.length() - 1, 1); //Remove the surrounding quotes.
                    if(sCurrentArgument.length()) // Remember, there may have been only one " char in the string.
                        sCurrentArgument.erase(0, 1);
                }

                stringArray.push_back(sCurrentArgument);
                while(IsWhiteSpace(sInputUsed[i + 1])) // Now remove the spaces between us an the next argument.
                    ++i; // Note that since the command line *doesn't* end in a space or spaces, we know here that we must be between arguments.
                iCurrentStringStart = i + 1;
                continue;
            }
            else if(sInputUsed[i] == '\"')
                ++nQuoteLevel;
        }

        sCurrentArgument.assign(&sInputUsed[iCurrentStringStart], i - iCurrentStringStart); //-1 because we don't want the space character.

        if(sCurrentArgument[0] == '\"' && sCurrentArgument[sCurrentArgument.length() - 1] == '\"')
        {
            sCurrentArgument.erase(sCurrentArgument.length() - 1, 1);  //Remove the surrounding quotes.
            if(sCurrentArgument.length()) // Remember, there may have been only one " char in the string.
                sCurrentArgument.erase(0, 1);
        }

        stringArray.push_back(sCurrentArgument);
    }
}



void CommandLine::ConvertStringArrayToString(const StringArray& stringArray, String& sResult)
{
    using namespace EA::Patch::Local;

    sResult.erase();

    for(eastl_size_t i(0), iEnd(stringArray.size()); i < iEnd; ++i)
    {
        bool bSpacePresent(false);
        bool bNonSpacePresent(false);

        for(eastl_size_t j(0), jEnd(stringArray[i].length()); (j < jEnd) && !bSpacePresent && !bNonSpacePresent; ++j)
        {
            if(IsWhiteSpace(stringArray[i][j]))
                bSpacePresent = true;
            else
                bNonSpacePresent = true;
        }

        if(bNonSpacePresent) 
        {
            if(bSpacePresent)
                sResult += '\"';

            sResult += stringArray[i];

            if(bSpacePresent)
                sResult += '\"';

            if((i + 1) < iEnd)
                sResult += ' ';
        } // Ignore empty arguments
    }
}




///////////////////////////////////////////////////////////////////////////////
// AppCommandLine
///////////////////////////////////////////////////////////////////////////////

AppCommandLine::AppCommandLine()
:   CommandLine() 
{
    // Empty. Nothing to do.
}


AppCommandLine::AppCommandLine(const char8_t* pCommandLineString)
:   CommandLine(pCommandLineString)
{
    FixCommandLine(*this);
}


AppCommandLine::AppCommandLine(int nArgumentCount, char8_t** ppArgumentArray)
  : CommandLine(nArgumentCount, ppArgumentArray)
{
    EA_UNUSED(nArgumentCount);
    EA_UNUSED(ppArgumentArray);

    #if defined(EA_PLATFORM_XENON)
        // The Xenon doesn't implement the argc/argv arguments properly.
        // The workaround is to use the Xenon OS GetCommandLine() function.
        const char* const pCommandLine = GetCommandLineA();
        SetCommandLineText(pCommandLine);
    #else
        FixCommandLine(*this);
    #endif
}


AppCommandLine::AppCommandLine(const AppCommandLine& appCommandLine)
  : CommandLine(appCommandLine) 
{
    FixCommandLine(*this);
}


AppCommandLine& AppCommandLine::operator=(const AppCommandLine& appCommandLine)
{
    CommandLine::operator=(appCommandLine);
    FixCommandLine(*this);

    return *this;
}


void AppCommandLine::SetCommandLineText(const char8_t* pCommandLineString)
{
    CommandLine::SetCommandLineText(pCommandLineString);
    FixCommandLine(*this);
}


void AppCommandLine::FixCommandLine(CommandLine& cmdLine, const char8_t* pAppPath)
{
    bool   bAppPathArgumentPresent(cmdLine.GetArgumentCount() > 0);
    String sAppPath(CoreAllocator(EAPATCHMAKER_ALLOC_PREFIX "CommandLine"));
    String sArgument0(CoreAllocator(EAPATCHMAKER_ALLOC_PREFIX "CommandLine"));

    if(bAppPathArgumentPresent) // If 'bAppPathArgumentPresent' is true, then cmdLine.argc is > 0.
        sArgument0 = cmdLine[0];

    #if defined(EA_PLATFORM_WINDOWS)
        char8_t szAppPath[512] = { 0 };

        if(pAppPath)
            EA::StdC::Strcpy(szAppPath, pAppPath);
        else
            EA::StdC::GetCurrentProcessPath(szAppPath);

        if(bAppPathArgumentPresent) // If there is any need to even do the code below...
        {
            String sAppPathName(CoreAllocator(EAPATCHMAKER_ALLOC_PREFIX "CommandLine"));
            String sArgument0PathName(CoreAllocator(EAPATCHMAKER_ALLOC_PREFIX "CommandLine"));

            if(sAppPath.empty()) // If the code above didn't assign it already.
                sAppPath = szAppPath;

            ///////////////////////////////////////////////////////////////////
            // What we want to do here is get just the file name of sAppPath
            // and sArgument0. Since we are within #ifdef WIN32 here, we can
            // make assumptions about Win32 file path formats here. 
            // Below we extract just the file name (no path or extension) out 
            // of the two paths and do a case-insensitive comparison between them.

            // Get the file name of sAppPath.
            sAppPathName = sAppPath;
            eastl_size_t nPosition = sAppPathName.find_last_of("\\/:");
            if(nPosition < sAppPathName.length())
                sAppPathName.erase(0, nPosition+1);
            nPosition = sAppPathName.find_last_of(".");
            if(nPosition < sAppPathName.length())
                sAppPathName.erase(nPosition);
            sAppPathName.make_lower();

            // Get the file name of sArgument0.
            sArgument0PathName = sArgument0;
            nPosition = sArgument0PathName.find_last_of("\\/:");
            if(nPosition < sArgument0PathName.length())
            {
                // To do: Find a simple way to tell if the path up to nPosition 
                // consists entirely of a valid path string. The following is
                // a very sorry attempt at this:
                if(EA::StdC::Isalpha(sArgument0PathName[0]) && (sArgument0PathName[1] == ':'))
                    sArgument0PathName.erase(0, nPosition+1);
                else if((sArgument0PathName[0] == '\\') || (sArgument0PathName[0] == '/'))
                    sArgument0PathName.erase(0, nPosition+1);
            }
            nPosition = sArgument0PathName.find_last_of(".");
            if(nPosition < sArgument0PathName.length())
                sArgument0PathName.erase(nPosition);
            sArgument0PathName.make_lower();
            ///////////////////////////////////////////////////////////////////

            if(sAppPathName == sArgument0PathName)
            {
                // If they are the same name, it is possible that the current argv[0]
                // name is not a full path but instead just an app name. What we do here
                // is simply always overwrite argv[0] with what we know to be the right
                // name, regardless of what was there before.
                cmdLine.EraseArgument(0);
                cmdLine.InsertArgument(sAppPath.c_str(), 0);
            }
            else
                bAppPathArgumentPresent = false;
        }

        if(!bAppPathArgumentPresent)
            sAppPath = szAppPath;

    #elif defined(EA_PLATFORM_XENON)
        if(pAppPath)
        {
            // Test pAppPath against cmdLine[0]
        }

    #else
        // Find a way to get the executable path here. Actually, on probably
        // any OS other than Windows, we won't need to do this because those
        // OSs probably get command line parameters correct.
        EA_UNUSED(pAppPath);
    #endif

    if(!bAppPathArgumentPresent && !sAppPath.empty())
        cmdLine.InsertArgument(sAppPath.c_str(), 0);

} // FixCommandLine


} // namespace Patch
} // namespace EA








