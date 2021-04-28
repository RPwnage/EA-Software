/******************************************************************************/
/*!
    ReportUploader

    Copyright 2010 Electronic Arts Inc.

    Version       1.0     10/17/2008 (lvillegas/mbaylis)
*/
/******************************************************************************/

/*** Includes *****************************************************************/
#include "BugSentry/bugsentrymgr.h"
#include "BugSentry/reportuploader.h"
#include "BugSentry/reporter.h"
#include "dirtysock.h"
#include "netconn.h"
#include "protohttp.h"
#include "EAStdC/EAMemory.h"
#include "EAStdC/EAString.h"

/*** Variables ****************************************************************/

#if defined(EA_PLATFORM_XENON)
const char* EA::BugSentry::ReportUploader::BUG_SENTRY_DEV_URL = "http://elephant.online.ea.com:8080/bugsentry";
const char* EA::BugSentry::ReportUploader::BUG_SENTRY_TEST_URL = "http://test.reports.tools.gos.ea.com:8008/bugsentry";
const char* EA::BugSentry::ReportUploader::BUG_SENTRY_PROD_URL = "http://reports.tools.gos.ea.com:8008/bugsentry";
const char* EA::BugSentry::ReportUploader::BUG_SENTRY_TEST_UNSECURE_URL = "http://test.reports.tools.gos.ea.com/bugsentry";
#else
const char* EA::BugSentry::ReportUploader::BUG_SENTRY_DEV_URL = "http://elephant.online.ea.com/bugsentry";
const char* EA::BugSentry::ReportUploader::BUG_SENTRY_TEST_URL = "http://test.reports.tools.gos.ea.com/bugsentry";
const char* EA::BugSentry::ReportUploader::BUG_SENTRY_PROD_URL = "https://reports.tools.gos.ea.com/bugsentry";
#endif

/*** Implementation ***********************************************************/
namespace EA
{
    namespace BugSentry
    {
        /******************************************************************************/
        /*! ReportUploader::ReportUploader

            \brief      ReportUploader Constructor

            \param       - none.
        */
        /******************************************************************************/
        ReportUploader::ReportUploader() : mPostData(NULL), mPostSize(0), mCallNetConnShutdown(false)
        {
            // initialize our buffer
            EA::StdC::Memset8(mPostDataBuffer,0,sizeof(mPostDataBuffer));
            EA::StdC::Memset8(mUrl,0,sizeof(mUrl));

            // initialize DirtySock
            if (-1 != NetConnStartup("-inet")) // This param is specific to Wii, it will be ignored by other platforms.
            {
                mCallNetConnShutdown = true;

                // give time to DirtySock idle functions
                NetConnIdle();
            }
        }

        /******************************************************************************/
        /*! ReportUploader::~ReportUploader

            \brief      ReportUploader Destructor

            \param       - none.
        */
        /******************************************************************************/
        ReportUploader::~ReportUploader()
        {
            if (mCallNetConnShutdown)
            {
                // Shutdown DirtySock
                NetConnShutdown(0);
            }
        }

        /******************************************************************************/
        /*! ReportUploader::FormatReport

            \brief      Formats the crash report for an HTTP POST to the BugSentry webservice.

            \param      reporter - The BugSentry Reporter object.
            \return      - none.
        */
        /******************************************************************************/
        void ReportUploader::FormatReport(const Reporter &reporter)
        {
            int reportMemoryBytesUsed = 0;

            // Create XML report in memory
            EA::StdC::Memset8(mPostDataBuffer,0,sizeof(mPostDataBuffer));
            reporter.CreateInMemoryReport(mPostDataBuffer, sizeof(mPostDataBuffer), &reportMemoryBytesUsed);

            // Save off a pointer to the buffer, and the size of the data
            mPostData = mPostDataBuffer;
            mPostSize = static_cast<int>(EA::StdC::Strlen(mPostData)*sizeof(char));
        }

        /******************************************************************************/
        /*! ReportUploader::GetReportingBaseUrl

            \brief      Returns the URL to use for uploading reports to the BugSentry webservice.

            \param       mode - The BugSentryMode that we are currently configured as.
            \return      URL - The URL to use for the the mode BugSentry is in.
        */
        /******************************************************************************/
        const char* ReportUploader::GetReportingBaseUrl(EA::BugSentry::BugSentryMode mode) const
        {
            const char* url = BUG_SENTRY_PROD_URL;

            switch (mode)
            {
                case BUGSENTRYMODE_DEV:
                    url = BUG_SENTRY_DEV_URL;
                    break;
                case BUGSENTRYMODE_TEST:
                    url = BUG_SENTRY_TEST_URL;
#if defined(EA_PLATFORM_XENON)
                    if ((NetConnStatus('sset',0,NULL,0) == 0) ||
                        (NetConnStatus('sset',0,NULL,1) == 1 && NetConnStatus('conn',0,NULL,0) != '+onl'))
                    {   // not in secure mode
                        url = BUG_SENTRY_TEST_UNSECURE_URL;
                    }
#endif
                    break;
                case BUGSENTRYMODE_PROD: // intentional fall-through
                default:
                    url = BUG_SENTRY_PROD_URL;
                    break;
            }

            return url;
        }
    }
}
