/******************************************************************************/
/*!
    SessionReportGenerator

    Copyright 2010 Electronic Arts Inc.

    Version       1.0     02/23/2010 (mbaylis)
*/
/******************************************************************************/

#pragma warning(disable:4266) // no override available for virtual member function from base 'CException'; function is hidden

/*** Includes *****************************************************************/
#include "BugSentry/sessionreportgenerator.h"
#include "BugSentry/ireportwriter.h"
#include "BugSentry/sessiondata.h"
#include "BugSentry/bugsentryconfig.h"
#include "BugSentry/bugsentrymgrbase.h"

/*** Implementation ***********************************************************/
namespace EA
{
    namespace BugSentry
    {
        static const char* SESSION_NAMES[SESSION_MAX] =
        {
            "boot",     // SESSION_BOOT
            "game",     // SESSION_GAME
            "server"    // SESSION_SERVER
        };

        /******************************************************************************/
        /*! SessionReportGenerator::GenerateReport

            \brief      Generates an XML report to report the session to GOS' BugSentry.

            \param       - writeInterface: The writer interface to use for writing the report.
            \param       - bugData: The BugData object.
            \param       - config: The BugSentryConfig object.
            \return      - True if the report was generated successfully, false if there was an error.
        */
        /******************************************************************************/
        bool SessionReportGenerator::GenerateReport(IReportWriter* writeInterface, const BugData* bugData, const BugSentryConfig* config)
        {
            bool success = false;

            mWriteInterface = writeInterface;
            mBugData = bugData;

            if (mBugData && config && mWriteInterface)
            {
                const SessionData* sessionData = static_cast<const SessionData*>(mBugData);

                success = true;
                success = success && WriteHeader();
                success = success && WriteSessionStart();
                success = success && WriteVersion();
                success = success && WriteSku();
                success = success && WriteBuildSignature();
                success = success && WriteSessionId();
                success = success && WriteSessionType();

                // Detect the session type and write the sessionType-specific tags
                switch (sessionData->mSessionType)
                {
                    case SESSION_SERVER:
                    {
                        success = success && WriteSessionServerType();
                        success = success && WriteSessionServerName();
                	      break;
                    }

                    case SESSION_BOOT:  // intentional fall-through
                    case SESSION_GAME:  // intentional fall-through
                    default:
                    {
                        // no extra tags to write
                        break;
                    }
                }

                success = success && WriteSessionEnd();
            }

            mWriteInterface = NULL;
            mBugData = NULL;

            return success;
        }

        /******************************************************************************/
        /*! SessionReportGenerator::WriteSessionStart

            \brief      Writes the session start tag.

            \param       - none.
            \return      - True if successful, false if there was an error.
        */
        /******************************************************************************/
        bool SessionReportGenerator::WriteSessionStart() const
        {
            bool success = WriteXmlTagStart("session");

            return success;
        }

        /******************************************************************************/
        /*! SessionReportGenerator::WriteSessionEnd

            \brief      Writes the session end tag.

            \param       - none.
            \return      - True if successful, false if there was an error.
        */
        /******************************************************************************/
        bool SessionReportGenerator::WriteSessionEnd() const
        {
            bool success = WriteXmlTagEnd("session");

            return success;
        }

        /******************************************************************************/
        /*! SessionReportGenerator::WriteSessionType

            \brief      Writes the session type tag.

            \param       - none.
            \return      - True if successful, false if there was an error.
        */
        /******************************************************************************/
        bool SessionReportGenerator::WriteSessionType() const
        {
            bool success = true;
            const char *tag = "type";
            const SessionData* sessionData = static_cast<const SessionData*>(mBugData);
            BugSentrySession sessionType = SESSION_INVALID;
            
            // Get the session type
            if (sessionData)
            {
                sessionType = static_cast<BugSentrySession>(sessionData->mSessionType);
            }

            // Write data
            success = success && WriteXmlTagStart(tag);
            
            if ((sessionType != SESSION_INVALID) && (sessionType < SESSION_MAX))
            {
                success = success && WriteString(SESSION_NAMES[sessionType]);
            }

            success = success && WriteXmlTagEnd(tag);

            return success;
        }

        /******************************************************************************/
        /*! SessionReportGenerator::WriteSessionServerType

            \brief      Writes the session servertype tag.

            \param       - none.
            \return      - True if successful, false if there was an error.
        */
        /******************************************************************************/
        bool SessionReportGenerator::WriteSessionServerType() const
        {
            bool success = true;
            const char *tag = "servertype";
            const SessionData* sessionData = static_cast<const SessionData*>(mBugData);
            BugSentrySession sessionType = SESSION_INVALID;
            
            // Get the session type
            if (sessionData)
            {
                sessionType = static_cast<BugSentrySession>(sessionData->mSessionType);
            }

            // This tag is only valid for the "server" session type!
            if (sessionData && (sessionType == SESSION_SERVER))
            {
                // Write data
                success = success && WriteXmlTagStart(tag);
                success = success && WriteString(sessionData->mSessionConfig.mServerSession.mServerType);
                success = success && WriteXmlTagEnd(tag);
            }

            return success;
        }

        /******************************************************************************/
        /*! SessionReportGenerator::WriteSessionServerName

            \brief      Writes the session servername tag.

            \param       - none.
            \return      - True if successful, false if there was an error.
        */
        /******************************************************************************/
        bool SessionReportGenerator::WriteSessionServerName() const
        {
            bool success = true;
            const char *tag = "servername";
            const SessionData* sessionData = static_cast<const SessionData*>(mBugData);
            BugSentrySession sessionType = SESSION_INVALID;
            
            // Get the session type
            if (sessionData)
            {
                sessionType = static_cast<BugSentrySession>(sessionData->mSessionType);
            }

            // This tag is only valid for the "server" session type!
            if (sessionData && (sessionType == SESSION_SERVER))
            {
                // Write data
                success = success && WriteXmlTagStart(tag);
                success = success && WriteString(sessionData->mSessionConfig.mServerSession.mServerName);
                success = success && WriteXmlTagEnd(tag);
            }

            return success;
        }
    }
}
