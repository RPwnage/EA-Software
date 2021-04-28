/*************************************************************************************************/
/*!
    \file nucleusresult.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class NucleusResult
        Used in conjunction with NucleusXmlParser to parse out the XML results
        from nucleus easily.

    \notes

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"
#include "framework/util/xmlbuffer.h"

#include "framework/oauth/nucleus/nucleusstringtocode.h"

#include "authentication/nucleusresult/nucleusresult.h"

namespace Blaze
{
namespace Authentication
{

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

/*** Public Methods ******************************************************************************/

/*************************************************************************************************/
/*!
    \brief checkSetError

    Checks to see if the incoming xpath marks an error.  If so, store it and return true.

    \param[in]  fullname - Current path (e.g. "/error" or "/users")
    \param[in]  attributes - Attributes that are coming in.
    \param[in]  attributeCount - Number of attributes.

    \return - true if error, otherwise false.

    \notes

*/
/*************************************************************************************************/
bool NucleusResult::checkSetError(const char8_t* fullname, const Blaze::XmlAttribute* attributes, size_t attributeCount)
{
    char8_t buffer[256];

    // Check for error.
    if (blaze_stricmp("/error", fullname) == 0)
    {        
        // Get error code.
        for (size_t i = 0; i < attributeCount; i++)
        {
            // Copy attribute name to buffer for comparison.
            blaze_strcpyFixedLengthToNullTerminated(buffer, sizeof(buffer), attributes->name, attributes->nameLen);

            // Look for "code" and copy the result.
            if (blaze_stricmp("code", buffer) == 0)
            {
            blaze_strcpyFixedLengthToNullTerminated(mCodeString, sizeof(mCodeString), attributes->value, attributes->valueLen);
                mCode = Blaze::Nucleus::NucleusCode::stringToCode(mCodeString);
            } // if

            // advance pointer
            attributes++;
        } // for

        TRACE_LOG("[NucleusResult].checkSetError() - Parsed error.  Code (" << mCodeString << ")");
        mHasError = true;
        return true;
    } // if

    // Check for failure.
    if (blaze_stricmp("/error/failure", fullname) == 0)
    {
        // Get error code.
        mFieldErrors.push_back();
        for (size_t i = 0; i < attributeCount; i++)
        {
            // Copy attribute name to buffer for comparison.
            blaze_strcpyFixedLengthToNullTerminated(buffer, sizeof(buffer), attributes->name, attributes->nameLen);

            // Look for "field" and copy the result.
            if (blaze_stricmp("field", buffer) == 0)
            {
                FieldError &f = mFieldErrors.back();
                blaze_strcpyFixedLengthToNullTerminated(f.mFieldString, sizeof(f.mFieldString), attributes->value, attributes->valueLen);
                f.mField = Blaze::Nucleus::NucleusField::stringToCode(f.mFieldString);
            } // if

            // Look for "cause" and copy the result.
            if (blaze_stricmp("cause", buffer) == 0)
            {
                FieldError &f = mFieldErrors.back();
                blaze_strcpyFixedLengthToNullTerminated(f.mCauseString, sizeof(f.mCauseString), attributes->value, attributes->valueLen);
                f.mCause = Blaze::Nucleus::NucleusCause::stringToCode(f.mCauseString);
            } // if

            // advance pointer
            attributes++;
        } // for

        TRACE_LOG("[NucleusResult].checkSetError() - Parsed Failure."
                " Field(" << mFieldErrors.back().mFieldString << ") Cause(" << mFieldErrors.back().mCauseString << ")");
        mHasError = true;
        return true;
    } // if

    return false;
} // checkSetError

void NucleusResult::setError(const char8_t* code, const char8_t* field, const char8_t* cause)
{
    blaze_strnzcpy(mCodeString, code, sizeof(mCodeString));
    mHasError = true;
    if ((field != nullptr) || (cause != nullptr))
    {
        FieldError error;
        if (field != nullptr)
            blaze_strnzcpy(error.mFieldString, field, sizeof(error.mFieldString));
        else
            error.mFieldString[0] = '\0';
        if (cause != nullptr)
            blaze_strnzcpy(error.mCauseString, cause, sizeof(error.mCauseString));
        else
            error.mCauseString[0] = '\0';
        mFieldErrors.push_back(error);
    }
}

void NucleusResult::setUserSecurityState(const char8_t* securityState)
{
     blaze_strnzcpy(mSecurityState, securityState, sizeof(mSecurityState));
}


/*************************************************************************************************/
/*!
    \brief setHttpError

    We hit an HTTP error such as TIMEOUT so set fields accordingly

    \notes

*/
/*************************************************************************************************/
void NucleusResult::setHttpError(BlazeRpcError err)
{
    mHasError = true;
    mCode = Blaze::Nucleus::NucleusCode::UNKNOWN;
    if (err == ERR_SYSTEM)
        blaze_strnzcpy(mCodeString, "HttpError", sizeof(mCodeString));
    else
        blaze_strnzcpy(mCodeString, ErrorHelp::getErrorName(err), sizeof(mCodeString));
}

/*** Protected Methods ***************************************************************************/

/*** Private Methods *****************************************************************************/

} // Authentication
} // Blaze

