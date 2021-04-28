/////////////////////////////////////////////////////////////////////////////
// EAPatchMaker/TestFileGenerator.cpp
//
// Copyright (c) Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////


#include <MemoryMan/MemoryMan.inl>
#include <MemoryMan/CoreAllocator.inl>
#include <EAIO/EAFileStream.h>
#include <EAIO/EAFileUtil.h>
#include <EAIO/EAStreamAdapter.h>
#include <EAIO/EAStreamBuffer.h>
#include <EAIO/PathString.h>
#include <EAStdC/EASprintf.h>
#include <EAStdC/EAString.h>
#include <EATest/EATest.h>
#include <EAPatchClientTest/ValidationFile.h>


///////////////////////////////////////////////////////////////////////////////
// EASTL requirements
///////////////////////////////////////////////////////////////////////////////

int Vsnprintf8(char8_t* pDestination, size_t n, const char8_t*  pFormat, va_list arguments)
    { return EA::StdC::Vsnprintf(pDestination, n, pFormat, arguments); }

int Vsnprintf16(char16_t* pDestination, size_t n, const char16_t* pFormat, va_list arguments)
    { return EA::StdC::Vsnprintf(pDestination, n, pFormat, arguments); }

int Vsnprintf32(char32_t* pDestination, size_t n, const char32_t* pFormat, va_list arguments)
    { return EA::StdC::Vsnprintf(pDestination, n, pFormat, arguments); }

#if defined(EA_WCHAR_UNIQUE) && EA_WCHAR_UNIQUE
int VsnprintfW(wchar_t* pDestination, size_t n, const wchar_t* pFormat, va_list arguments)
    { return EA::StdC::Vsnprintf(pDestination, n, pFormat, arguments); }
#endif


///////////////////////////////////////////////////////////////////////////////
// Returns true if this process was started in a new console view. This is useful to know 
// because it usually means the app was started by double-clicking an app icon and will
// go away without the user being able to see any output unless we intentionally wait on exit.
// This function must be called right away on process startup before anything is printed to the screen.
static bool IsConsoleNew()
{
    #if defined(EA_PLATFORM_WINDOWS)
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

        if(hConsole != NULL)
        {
            CONSOLE_SCREEN_BUFFER_INFO csbi;

            if(GetConsoleScreenBufferInfo(hConsole, &csbi))
            {
                // If the cursor position is at [0,0], we assume the console
                // is newly created and hasn't been written to. Strictly speaking,
                // this isn't necessarily so, but most often it will be so.
                return ((csbi.dwCursorPosition.X == 0) && 
                        (csbi.dwCursorPosition.Y == 0));
            }
        }
    #endif

    return false;
}


///////////////////////////////////////////////////////////////////////////////
// kMaxFileSize
//
static const uint64_t kMaxFileSizeGB = 32;
static const uint64_t kMaxFileSize   = kMaxFileSizeGB * 1024 * 1024 * 1024;


static void PrintHelp(bool bPrintBanner)
{
    static bool bHelpPrinted = false;

    if(!bHelpPrinted)
    {
        bHelpPrinted = true;

        if(bPrintBanner)
        {
            EA::StdC::Printf("*******************************************************************************\n");
            EA::StdC::Printf("TestFileGenerator\n");
            EA::StdC::Printf("Developed and maintained by EATech\n");
            EA::StdC::Printf("\n");
            EA::StdC::Printf("This app generates a sized test file, which is a file of some size whose bytes\n");
            EA::StdC::Printf("are in some pattern that allows you to verify the file's contents based on the\n");
            EA::StdC::Printf("bytes alone. Each byte has a predetermined value which is specific for its location\n");
            EA::StdC::Printf("in the file.\n");
        }

        EA::StdC::Printf("Available arguments:\n");
        EA::StdC::Printf("    -help / -h / -?     Optional. Displays this usage information\n");
        EA::StdC::Printf("    -wait / -w          Optional. Waits for user confirmation before app exit.\n");
        EA::StdC::Printf("    -file:<file path>   Required. Specifies the path to the file to create.\n");
        EA::StdC::Printf("    -size:<size>        Required. Specifies the size (up to %I64u GB) of the file to create.\n", kMaxFileSizeGB);
        EA::StdC::Printf("    -overwrite:<yes|no> Optional. If specified then the file is overwritten if it already exists. Default is no.\n");
        EA::StdC::Printf("\n");
        EA::StdC::Printf("Example usage:\n");
        EA::StdC::Printf("    TestFileGenerator -overwrite:yes -file:C:\\SomeDir\\SomeFile.txt -size:3680\n");
        EA::StdC::Printf("    TestFileGenerator -file:SomeFile.txt -size:42MB\n");

        if(bPrintBanner)
           EA::StdC::Printf("*******************************************************************************\n");
    }
}



///////////////////////////////////////////////////////////////////////////////
// main
//
int main(int argc, char** argv)
{
    int                                  returnValue = 0;
    EA::EAMain::CommandLine              cmdLine(argc, argv);
    EA::EAMain::CommandLine::FixedString sTemp;
    EA::EAMain::CommandLine::FixedString sFilePath;
    uint64_t                             fileSize = UINT64_MAX;
    bool                                 bWaitAtEnd = IsConsoleNew();
    bool                                 bOverwrite = false;

    {   // Read command line arguments.
        // Example usage: -?
        if((cmdLine.FindSwitch("-help", false, NULL, 0) >= 0) ||
           (cmdLine.FindSwitch("-h",    false, NULL, 0) >= 0) ||
           (cmdLine.FindSwitch("-?",    false, NULL, 0) >= 0))
        {
            PrintHelp(true);
        }

        // Example usage: -wait
        if((cmdLine.FindSwitch("-wait", false, NULL, 0) >= 0) || 
           (cmdLine.FindSwitch("-w",    false, NULL, 0) >= 0))
        {
            bWaitAtEnd = true;
        }

        // Example usage: -file:C:\Dir\File.abc 
        cmdLine.FindSwitch("-file", false, &sFilePath, 0);

        // Example usage: -size:4006 
        // Example usage: -size:32KB 
        if(cmdLine.FindSwitch("-size", false, &sTemp, 0) >= 0)
        {
            if(!sTemp.empty())
            {
                fileSize = EA::StdC::AtoU64(sTemp.c_str());

                sTemp.make_upper();
                if(sTemp.find("KB") < sTemp.length())
                    fileSize *= 1024;
                else if(sTemp.find("MB") < sTemp.length())
                    fileSize *= 1024 * 1024;
                else if(sTemp.find("GB") < sTemp.length())
                    fileSize *= 1024 * 1024 * 1024;
            }                
        }

        // Example usage: -overwrite:yes 
        if(cmdLine.FindSwitch("-overwrite", false, &sTemp, 0) >= 0)
            bOverwrite = (sTemp.comparei("no") != 0);
    }

    if(sFilePath.empty())
    {
        EA::EAMain::Report("No file path was specified. ");
        if(cmdLine.FindSwitch("-file", false, &sTemp, 0) >= 0)
            EA::EAMain::Report("The -file argument was present, but there was no file path immediately after the colon (with no space).\n");
        else
            EA::EAMain::Report("The -file argument was not present\n");
        PrintHelp(false);
        returnValue = -1;
        goto End;
    }

    if(fileSize == UINT64_MAX)
    {
        EA::EAMain::Report("No file size was specified. ");
        if(cmdLine.FindSwitch("-size", false, &sTemp, 0) >= 0)
            EA::EAMain::Report("The -size argument was present, but there was no numerical value immediately after the colon (with no space).\n");
        else
            EA::EAMain::Report("The -size argument was not present\n");
        PrintHelp(false);
        returnValue = -1;
        goto End;
    }
    else if(fileSize >= kMaxFileSize)
    {
        EA::EAMain::Report("File size too big. Max size is %I64u GB\n", kMaxFileSizeGB);
        PrintHelp(false);
        returnValue = -1;
        goto End;
    }

    {
        eastl::string sError;
        bool result = EA::Patch::GenerateValidateFile(sFilePath.c_str(), fileSize, bOverwrite, sError);

        if(!result)
        {
            EA::EAMain::Report("%s", sError.c_str());
            returnValue = -1;
            goto End;
        }
    }

    End:
    if(bWaitAtEnd)
    {
        EA::EAMain::Report("\nPress any key to exit.\n");
        getchar(); // Wait for the user and shutdown
    }

    return returnValue;
}







