/******************************************************************************/
/*! 
    DesyncReportGenerator
    Copyright 2010 Electronic Arts Inc.

    Version       1.0     07/17/2009 (gsharma)
*/
/******************************************************************************/

/*** Includes *****************************************************************/
#include "BugSentry/desyncreportgenerator.h"
#include "BugSentry/desyncdata.h"
#include "BugSentry/ireportwriter.h"
#include "BugSentry/bugsentryconfig.h"

/*** Implementation ***********************************************************/
namespace EA
{
    namespace BugSentry
    {
        /******************************************************************************/
        /*! DesyncReportGenerator::GenerateReport

            \brief      Generates an XML report of the desync suitable for submission to GOS' BugSentry.

            \param       - writeInterface: The writer interface to use for writing the report.
            \param       - bugData: The BugData object.
            \param       - config: The BugSentryConfig object.
            \return      - True if the report was generated successfully, false if there was an error.
        */
        /******************************************************************************/
        bool DesyncReportGenerator::GenerateReport(IReportWriter* writeInterface, const BugData* bugData, const BugSentryConfig* config)
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
                success = success && WriteReportType("desync");
                success = success && WriteSku();
                success = success && WriteCreationTime();
                success = success && WriteBuildSignature();
                success = success && WriteSystemConfig();
                success = success && WriteCategoryId();
                success = success && WriteDesyncId();
                success = success && WriteScreenshot(config);
                success = success && WriteDesyncData();
                success = success && WriteReportEnd();
            }

            mWriteInterface = NULL;
            mBugData = NULL;

            return success;
        }


        /******************************************************************************/
        /*! DesyncReportGenerator :: WriteDesyncData

            \brief  Writes the desyncdata tag.

            \param          - none.
            \return         - True if successful, false if there was an error.
        */
        /******************************************************************************/
        bool DesyncReportGenerator::WriteDesyncData()
        {
            bool success = true;
            const char* tag = "desyncdata";
            const DesyncData* desyncData = static_cast<const DesyncData*>(mBugData);

            success = success && WriteXmlTagStart(tag);

            // To optimize memory usage, BugSentry references the DesyncInfo memory buffer owned
            // by the game directly, instead of allocating additional memory to copy it over.
            // This data can be very large, so it makes sense to just use the buffer already allocated!
            if (desyncData && desyncData->mDesyncInfo)
            {
                // Get the current position in our buffer (so the uploader knows when to switch to the other buffer)
                mDesyncDataPosition = mWriteInterface->GetCurrentPosition();
            }
            else
            {
                // No desync information supplied by game, set the pointer to NULL so the uploader just continues
                mDesyncDataPosition = NULL;
            }

            success = success && WriteXmlTagEnd(tag);

            return success;
        }
        
        /******************************************************************************/
        /*! DesyncReportGenerator::WriteDesyncId

            \brief      Writes the desync id tag.

            \param       - none.
            \return      - True if successful, false if there was an error.
        */
        /******************************************************************************/
        bool DesyncReportGenerator::WriteDesyncId() const
        {
            bool success = true;
            const char* tag= "desyncid";
            const DesyncData* desyncData = static_cast<const DesyncData*>(mBugData);

            success = success && WriteXmlTagStart(tag);
            if (desyncData)
            {
                success = success && WriteString(desyncData->mDesyncId);
            }
            success = success && WriteXmlTagEnd(tag);

            return success;
        }
        
        /******************************************************************************/
        /*! DesyncReportGenerator::WriteCategoryId

            \brief      Writes the categoryid tag.

            \param       - none.
            \return      - True if successful, false if there was an error.
        */
        /******************************************************************************/
        bool DesyncReportGenerator::WriteCategoryId() const
        {
            bool success = true;
            const char *tag = "categoryid";
            const DesyncData* desyncData = static_cast<const DesyncData*>(mBugData);

            success = success && WriteXmlTagStart(tag);
            if (desyncData)
            {
                success = success && WriteString(desyncData->mCategoryId);
            }
            success = success && WriteXmlTagEnd(tag);

            return success;
        }
    }
}
