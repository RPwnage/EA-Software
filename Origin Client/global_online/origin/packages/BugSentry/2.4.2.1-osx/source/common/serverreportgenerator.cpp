/******************************************************************************/
/*! 
    ServerReportGenerator
    Copyright 2010 Electronic Arts Inc.

    Version       1.0     04/05/2010 (mbaylis)
*/
/******************************************************************************/

/*** Includes *****************************************************************/
#include "BugSentry/serverreportgenerator.h"
#include "BugSentry/ireportwriter.h"
#include "BugSentry/serverdata.h"
#include "BugSentry/bugsentryconfig.h"
#include "BugSentry/bugsentrymgrbase.h"

/*** Implementation ***********************************************************/
namespace EA
{
    namespace BugSentry
    {
        static const char* SERVER_ERRORS[SERVER_ERROR_TYPE_MAX] =
        {
            "connectionfailure",    // SERVER_ERROR_TYPE_CONNECTIONFAILURE
            "disconnect"            // SERVER_ERROR_TYPE_DISCONNECT
        };

        /******************************************************************************/
        /*! ServerReportGenerator::GenerateReport

            \brief      Generates an XML report of the server error suitable for submission to GOS' BugSentry.

            \param       - writeInterface: The writer interface to use for writing the report.
            \param       - bugData: The BugData object.
            \param       - config: The BugSentryConfig object.
            \return      - True if the report was generated successfully, false if there was an error.
        */
        /******************************************************************************/
        bool ServerReportGenerator::GenerateReport(IReportWriter* writeInterface, const BugData* bugData, const BugSentryConfig* config)
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
                success = success && WriteReportType("server");
                success = success && WriteSku();
                success = success && WriteCreationTime();
                success = success && WriteBuildSignature();
                success = success && WriteCategoryId();
                success = success && WriteServerType();
                success = success && WriteServerName();
                success = success && WriteServerError();
                success = success && WriteContextData();
                success = success && WriteReportEnd();
            }

            mWriteInterface = NULL;
            mBugData = NULL;

            return success;
        }
        
        /******************************************************************************/
        /*! ServerReportGenerator::WriteCategoryId

            \brief      Writes the categoryid tag.

            \param       - none.
            \return      - True if successful, false if there was an error.
        */
        /******************************************************************************/
        bool ServerReportGenerator::WriteCategoryId() const
        {
            bool success = true;
            const char *tag = "categoryid";
            const ServerData* serverData = static_cast<const ServerData*>(mBugData);

            success = success && WriteXmlTagStart(tag);
            if (serverData)
            {
                success = success && WriteString(serverData->mCategoryId);
            }
            success = success && WriteXmlTagEnd(tag);

            return success;
        }

        /******************************************************************************/
        /*! ServerReportGenerator::WriteServerType

            \brief      Writes the servertype tag.

            \param       - none.
            \return      - True if successful, false if there was an error.
        */
        /******************************************************************************/
        bool ServerReportGenerator::WriteServerType() const
        {
            bool success = true;
            const char *tag = "servertype";
            const ServerData* serverData = static_cast<const ServerData*>(mBugData);

            success = success && WriteXmlTagStart(tag);
            if (serverData)
            {
                success = success && WriteString(serverData->mServerType);
            }
            success = success && WriteXmlTagEnd(tag);

            return success;
        }

        /******************************************************************************/
        /*! ServerReportGenerator::WriteServerName

            \brief      Writes the servername tag.

            \param       - none.
            \return      - True if successful, false if there was an error.
        */
        /******************************************************************************/
        bool ServerReportGenerator::WriteServerName() const
        {
            bool success = true;
            const char *tag = "servername";
            const ServerData* serverData = static_cast<const ServerData*>(mBugData);

            success = success && WriteXmlTagStart(tag);
            if (serverData)
            {
                success = success && WriteString(serverData->mServerName);
            }
            success = success && WriteXmlTagEnd(tag);

            return success;
        }

        /******************************************************************************/
        /*! ServerReportGenerator::WriteServerError

            \brief      Writes the servererror tag.

            \param       - none.
            \return      - True if successful, false if there was an error.
        */
        /******************************************************************************/
        bool ServerReportGenerator::WriteServerError() const
        {
            bool success = true;
            const char *tag = "servererror";
            const ServerData* serverData = static_cast<const ServerData*>(mBugData);
            BugSentryServerErrorType errorType = SERVER_ERROR_TYPE_INVALID;

            // Get the server error type
            if (serverData)
            {
                errorType = static_cast<BugSentryServerErrorType>(serverData->mServerErrorType);
            }

            // Write data
            success = success && WriteXmlTagStart(tag);

            if ((errorType != SERVER_ERROR_TYPE_INVALID) && (errorType < SERVER_ERROR_TYPE_MAX))
            {
                success = success && WriteString(SERVER_ERRORS[errorType]);
            }

            success = success && WriteXmlTagEnd(tag);

            return success;
        }

        /******************************************************************************/
        /*! ServerReportGenerator :: WriteContextData

            \brief  Writes the contextdata tag.

            \param          - none.
            \return         - True if successful, false if there was an error.
        */
        /******************************************************************************/
        bool ServerReportGenerator::WriteContextData()
        {
            bool success = true;
            const char* tag = "contextdata";
            const ServerData* serverData = static_cast<const ServerData*>(mBugData);

            success = success && WriteXmlTagStart(tag);

            // To optimize memory usage, BugSentry references the ContextData memory buffer owned
            // by the game directly, instead of allocating additional memory to copy it over.
            // This data can be very large, so it makes sense to just use the buffer already allocated!
            if (serverData && serverData->mContextData)
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
    }
}
