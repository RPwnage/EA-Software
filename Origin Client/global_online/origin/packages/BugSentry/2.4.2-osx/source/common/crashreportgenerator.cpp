/******************************************************************************/
/*!
    CrashReportGenerator

    Copyright 2010 Electronic Arts Inc.

    Version       1.0     10/17/2008 (lvillegas/mbaylis)
*/
/******************************************************************************/

/*** Includes *****************************************************************/
#include "EAStdC/EAMemory.h"
#include "EAStdC/EASprintf.h"
#include "BugSentry/crashreportgenerator.h"
#include "BugSentry/ireportwriter.h"
#include "BugSentry/crashdata.h"
#include "BugSentry/bugsentryconfig.h"

/*** Implementation ***********************************************************/
namespace EA
{
    namespace BugSentry
    {
        /******************************************************************************/
        /*! CrashReportGenerator::GenerateReport

            \brief      Generates an XML report of the crash suitable for submission to GOS' BugSentry.

            \param       - writeInterface: The writer interface to use for writing the report.
            \param       - bugData: The BugData object.
            \param       - config: The BugSentryConfig object.
            \return      - True if the report was generated successfully, false if there was an error.
        */
        /******************************************************************************/
        bool CrashReportGenerator::GenerateReport(IReportWriter* writeInterface, const BugData* bugData, const BugSentryConfig* config)
        {
            bool success = false;

            mWriteInterface = writeInterface;
            mBugData = bugData;

            if (mBugData && config && mWriteInterface)
            {
                success = true;
                success = success && WriteHeader();
                success = success && WriteReportStart();
                success = success && WriteVersion();
                success = success && WriteSessionId();
                success = success && WriteReportType("crash");
                success = success && WriteSku();
                success = success && WriteCreationTime();
                success = success && WriteBuildSignature();
                success = success && WriteSystemConfig();
                success = success && WriteCategoryId();
                success = success && WriteCallstack();
                success = success && WriteThreads();
                success = success && WriteScreenshot(config);
                success = success && WriteMemDump();
                success = success && WriteContextData();
                success = success && WriteReportEnd();
            }

            mWriteInterface = NULL;
            mBugData = NULL;

            return success;
        }

        /******************************************************************************/
        /*! CrashReportGenerator::WriteCategoryId

            \brief      Writes the categoryid tag.

            \param       - none.
            \return      - True if successful, false if there was an error.
        */
        /******************************************************************************/
        bool CrashReportGenerator::WriteCategoryId() const
        {
            bool success = true;
            const char *tag = "categoryid";
            const CrashData* crashData = static_cast<const CrashData*>(mBugData);
            
            // Write start tag
            success = success && WriteXmlTagStart(tag);

            // If the game supplied the category id, use it
            if((crashData->mCategoryId != NULL) && (crashData->mCategoryId[0] != '\0'))
            {
                success = success && WriteString(crashData->mCategoryId);
            }
            else
            {
                const int ADDRESS_STRING_HEX_SIZE = (EA_PLATFORM_PTR_SIZE*2)+1; // +1 for \0 character
                char stringAddress[ADDRESS_STRING_HEX_SIZE];

                // Otherwise, the category id (or unique id) will be the hex address of the crash.
                EA::StdC::Memset8(stringAddress, 0, sizeof(stringAddress));
                EA::StdC::Snprintf(stringAddress, sizeof(stringAddress), "%p", crashData->mStackAddresses[0]);
                
                // Write category id
                success = success && WriteString(" 0x");
                success = success && WriteString(stringAddress);
            }

            // Write end tag
            success = success && WriteXmlTagEnd(tag);

            return success;
        }

        /******************************************************************************/
        /*! CrashReportGenerator::WriteCallstack

            \brief      Writes the callstack tag.

            \param       - none.
            \return      - True if successful, false if there was an error.
        */
        /******************************************************************************/
        bool CrashReportGenerator::WriteCallstack() const
        {
            bool success = true;
            const int ADDRESS_STRING_HEX_SIZE = (EA_PLATFORM_PTR_SIZE*2)+1; // +1 for \0 character
            const char *tag = "stack";
            char stringAddress[ADDRESS_STRING_HEX_SIZE];
            const CrashData* crashData = static_cast<const CrashData*>(mBugData);

            success = success && WriteXmlTagStart(tag);

            if (success && (crashData->mStackAddressesCount > 0))
            {
                for (int addressIndex = 0; success && addressIndex < crashData->mStackAddressesCount; ++addressIndex)
                {
                    EA::StdC::Memset8(stringAddress, 0, sizeof(stringAddress));
                    EA::StdC::Snprintf(stringAddress, sizeof(stringAddress), "%p", crashData->mStackAddresses[addressIndex]);
                    success = success && WriteString(" 0x");
                    success = success && WriteString(stringAddress);
                }
            }
            success = success && WriteXmlTagEnd(tag);

            return success;
        }

        /******************************************************************************/
        /*! CrashReportGenerator::WriteThreads

            \brief      Writes the threads tag.

            \param       - none.
            \return      - True if successful, false if there was an error.
        */
        /******************************************************************************/
        bool CrashReportGenerator::WriteThreads() const
        {
            bool success = true;
            const char* tag = "threads";

            success = success && WriteXmlTagStart(tag);
            //TODO: Needs final implementation
            //The body of this tag is optional 
            //Will be popuplated by a BugSentry Util or the Game Client via some Utils platform specific calls.
            //add something like "success = success && WriteString(SomeUtils::GetThreads());"
            success = success && WriteXmlTagEnd(tag);

            return success;
        }        

        /******************************************************************************/
        /*! CrashReportGenerator::WriteContextData

            \brief      Writes the contextdata tag.

            \param       - none.
            \return      - True if successful, false if there was an error.
        */
        /******************************************************************************/
        bool CrashReportGenerator::WriteContextData()
        {
            bool success = true;
            const char* tag = "contextdata";
            const CrashData* crashData = static_cast<const CrashData*>(mBugData);

            success = success && WriteXmlTagStart(tag);
            
            // To optimize memory usage, BugSentry references the ContextData memory buffer owned
            // by the game directly, instead of allocating additional memory to copy it over.
            // This data can be very large, so it makes sense to just use the buffer already allocated!
            if (crashData && crashData->mContextData)
            {
                // Get the current position in our buffer (so the uploader knows when to switch to the other buffer)
                mContextDataPosition = mWriteInterface->GetCurrentPosition();
            }
            else
            {
                // No context data information supplied by game, set the pointer to NULL so the uploader just continues
                mContextDataPosition = NULL;
            }

            success = success && WriteXmlTagEnd(tag);

            return success;
        }

        /******************************************************************************/
        /*! CrashReportGenerator::WriteMemDump

            \brief      Writes the memdump tag.

            \param       - none.
            \return      - True if successful, false if there was an error.
        */
        /******************************************************************************/
        bool CrashReportGenerator::WriteMemDump()
        {
            bool success = true;
            const char* tag = "memdump";

            success = success && WriteXmlTagStart(tag);
            //TODO: Future addition planned
            //The body of this tag is optional 
            //Will be popuplated by a BugSentry Util or the Game Client via some Utils platform specific calls.
            //add something like "success = success && WriteString(SomeUtils::GetMemDump());"
            success = success && WriteXmlTagEnd(tag);

            return success;
        }
    }
}
