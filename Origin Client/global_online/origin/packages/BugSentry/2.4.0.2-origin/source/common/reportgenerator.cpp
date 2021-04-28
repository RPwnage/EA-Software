/******************************************************************************/
/*!
    ReportGenerator

    Copyright 2010 Electronic Arts Inc.

    Version       1.0     10/17/2008 (lvillegas/mbaylis)
*/
/******************************************************************************/

/*** Includes *****************************************************************/
#include "BugSentry/reportgenerator.h"
#include "BugSentry/base64encoderwriter.h"
#include "BugSentry/imagesampler.h"
#include "BugSentry/imagecompressor.h"
#include "BugSentry/bugdata.h"
#include "BugSentry/bugsentryconfig.h"
#include "EAStdC/EAString.h"
#include "EAStdC/EADateTime.h"

/*** Variables ****************************************************************/
const char* EA::BugSentry::ReportGenerator::REPORT_VERSION = "2";

/*** Implementation ***********************************************************/
namespace EA
{
    namespace BugSentry
    {
        /******************************************************************************/
        /*! ReportGenerator::WriteHeader

            \brief      Writes the header of the report

            \param       - none.
            \return      - True if successful, false if there was an error.
        */
        /******************************************************************************/
        bool ReportGenerator::WriteHeader() const
        {
            bool success = WriteString("<?xml version=\"1.0\" encoding=\"UTF-8\"?>");

            return success;
        }

        /******************************************************************************/
        /*! ReportGenerator::WriteReportStart

            \brief      Writes the report start tag.

            \param       - none.
            \return      - True if successful, false if there was an error.
        */
        /******************************************************************************/
        bool ReportGenerator::WriteReportStart() const
        {
            bool success = WriteXmlTagStart("report");

            return success;
        }

        /******************************************************************************/
        /*! ReportGenerator::WriteVersion

            \brief      Writes the version tag.

            \param       - none.
            \return      - True if successful, false if there was an error.
        */
        /******************************************************************************/
        bool ReportGenerator::WriteVersion() const
        {
            bool success = true;
            const char* tag= "version";

            success = success && WriteXmlTagStart(tag);
            success = success && WriteString(REPORT_VERSION);
            success = success && WriteXmlTagEnd(tag);

            return success;
        }

        /******************************************************************************/
        /*! ReportGenerator::WriteSessionId

            \brief      Writes the session id tag.

            \param       - none.
            \return      - True if successful, false if there was an error.
        */
        /******************************************************************************/
        bool ReportGenerator::WriteSessionId() const
        {
            bool success = true;
            const char* tag= "sessionid";

            success = success && WriteXmlTagStart(tag);
            success = success && WriteString(mBugData->mSessionId);
            success = success && WriteXmlTagEnd(tag);

            return success;
        }
        
        /******************************************************************************/
        /*! ReportGenerator::WriteSku

            \brief      Writes the sku tag.

            \param       - none.
            \return      - True if successful, false if there was an error.
        */
        /******************************************************************************/
        bool ReportGenerator::WriteSku() const
        {
            bool success = true;
            const char* tag= "sku";

            success = success && WriteXmlTagStart(tag);
            success = success && WriteString(mBugData->mSku);
            success = success && WriteXmlTagEnd(tag);

            return success;
        }

        /******************************************************************************/
        /*! ReportGenerator::WriteCreationTime

            \brief      Writes the creation time tag.

            \param       - none.
            \return      - True if successful, false if there was an error.
        */
        /******************************************************************************/
        bool ReportGenerator::WriteCreationTime() const
        {
            // how big is the time buffer supposed to be?
            // year:        4 chars
            // month:       2 chars
            // day:         2 chars
            // hour:        2 chars
            // minute:      2 chars
            // seconds:     2 chars
            // timezone:    3 chars or ~20 for windows
            // hyphens:     2 chars
            // colons:      2 chars
            // spaces:      2 chars
            // null (\0):   1 char
            // TOTAL:       24 chars/41 chars, we used 64

            // note that windows/pc the time zone
            // might be a little bit whacky since it can be either
            // a full name or the abbreviation for it, the thing is
            // that it depends on the registry which we have no control of
            // so we allow enough space for either one

            bool success = true;
            const char* tag = "createtime";
            tm timeInfo;
            const int32_t TIME_BUFFER_SIZE = 64;
            char8_t timeBuffer[TIME_BUFFER_SIZE] = {0};

            // Get the current time (as UTC) and convert to the Tm object.
            EA::StdC::DateTime dateTime(EA::StdC::kTimeFrameUTC);
            EA::StdC::DateTimeToTm(dateTime, timeInfo);

            // Format the timestamp to what the BugSentry server expects
            EA::StdC::Strftime(timeBuffer, sizeof(timeBuffer), "%Y-%m-%d %H:%M:%S", &timeInfo);

            success = success && WriteXmlTagStart(tag);
            success = success && WriteString(timeBuffer);
            success = success && WriteXmlTagEnd(tag);

            return success;
        }

        /******************************************************************************/
        /*! ReportGenerator::WriteBuildSignature

            \brief      Writes the buildsignature tag.

            \param       - none.
            \return      - True if successful, false if there was an error.
        */
        /******************************************************************************/
        bool ReportGenerator::WriteBuildSignature() const
        {
            bool success = true;
            const char* tag= "buildsignature";

            success = success && WriteXmlTagStart(tag);
            success = success && WriteString(mBugData->mBuildSignature);
            success = success && WriteXmlTagEnd(tag);

            return success;
        }
        
        /******************************************************************************/
        /*! ReportGenerator::WriteReportType

            \brief  Writes the type tag.

            \param  reportType - Value to write within <type> tag.
            \return      - True if successful, false if there was an error.
        */
        /******************************************************************************/
        bool ReportGenerator::WriteReportType(const char* reportType) const
        {
            bool success = true;
            const char* tag = "type";

            success = success && WriteXmlTagStart(tag);
            success = success && WriteString(reportType);
            success = success && WriteXmlTagEnd(tag);

            return success;
        }

        /******************************************************************************/
        /*! ReportGenerator::WriteSystemConfig

            \brief      Writes the system config tag.

            \param       - none.
            \return      - True if successful, false if there was an error.
        */
        /******************************************************************************/
        bool ReportGenerator::WriteSystemConfig() const
        {
            bool success = true;
            const char* tag = "systemconfig";

            success = success && WriteXmlTagStart(tag);
            //TODO: Needs final implementation
            //The body of this tag is optional 
            //Will be popuplated by a BugSentry Util or the Game Client via some Utils platform specific calls.
            //add something like "success = success && WriteString(SomeUtils::GetSystemConfig());"
            success = success && WriteXmlTagEnd(tag);

            return success;
        }
        
        /******************************************************************************/
        /*! ReportGenerator::WriteScreenshot

            \brief      Writes the screenshot tag.

            \param       - config: The BugSentryConfig object.
            \return      - True if successful, false if there was an error.
        */
        /******************************************************************************/
        bool ReportGenerator::WriteScreenshot(const BugSentryConfig* config) const
        {
            bool success = true;
            const char* tag= "screenshot";

            success = success && WriteXmlTagStart(tag);

            if (success &&
                config &&
                config->mEnableScreenshot &&
                config->mAllocator &&
                mBugData->mImageInfo.mImageData)
            {
                ImageCompressor compressor(config->mAllocator);
                Base64EncoderWriter encoderWriter(mWriteInterface);
                ImageSampler sampler(&(mBugData->mImageInfo));

                success = compressor.Compress(&sampler, &encoderWriter);
            }

            success = success && WriteXmlTagEnd(tag);

            return success;
        }
        
        /******************************************************************************/
        /*! ReportGenerator::WriteReportEnd

            \brief      Writes the report end tag.

            \param       - none.
            \return      - True if successful, false if there was an error.
        */
        /******************************************************************************/
        bool ReportGenerator::WriteReportEnd() const
        {
            bool success = WriteXmlTagEnd("report");

            return success;
        }

        /******************************************************************************/
        /*! ReportGenerator::WriteXmlTagStart

            \brief      Writes an xml start tag given the string for the tag name.

            \param       - tagName - The name of the XML tag.
            \return      - True if successful, false if there was an error.
        */
        /******************************************************************************/
        bool ReportGenerator::WriteXmlTagStart(const char* tagName) const
        {
            bool success = true;

            success = success && WriteString("<");
            success = success && WriteString(tagName);
            success = success && WriteString(">");

            return success;
        }

        /******************************************************************************/
        /*! ReportGenerator::WriteXmlTagEnd

            \brief      Writes an xml end tag given the string for the tag name.

            \param       - tagName - The name of the XML tag.
            \return      - True if successful, false if there was an error.
        */
        /******************************************************************************/
        bool ReportGenerator::WriteXmlTagEnd(const char* tagName) const
        {
            bool success = true;

            success = success && WriteString("</");
            success = success && WriteString(tagName);
            success = success && WriteString(">");

            return success;
        }

        /******************************************************************************/
        /*! ReportGenerator::WriteString

            \brief      Writes the specified string to the writer.

            \param       - string - The string to write.
            \return      - True if successful, false if there was an error.
        */
        /******************************************************************************/
        bool ReportGenerator::WriteString(const char* string) const
        {
            bool success = false;

            if (mWriteInterface && string)
            {
                int strLength = static_cast<int>(EA::StdC::Strlen(string));

                //If the string is empty, don't try to Write, just return true
                if (0 == strLength)
                {
                    success = true;
                }
                else
                {
                    success = mWriteInterface->Write(string, strLength);
                }
            }

            return success;
        }
    }
}
