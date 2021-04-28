/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef EA_TDF_TDFENCODER_H
#define EA_TDF_TDFENCODER_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

/*** Include files *******************************************************************************/
#include <EATDF/tdfvisit.h>
#include <EAIO/EAStream.h>

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace EA
{
namespace TDF
{
    struct EATDF_API EncodeOptions
    {
        typedef enum
        {
            FORMAT_USE_TDF_NAMES,
            FORMAT_USE_TDF_RAW_NAMES,
            FORMAT_USE_TDF_TAGS
        } ResponseFormat;

        typedef enum
        {
            XML_RESPONSE_FORMAT_ELEMENTS,
            XML_RESPONSE_FORMAT_ATTRIBUTES
        } XmlResponseFormat;

        typedef enum
        {
            ENUM_FORMAT_IDENTIFIER,
            ENUM_FORMAT_VALUE
        } EnumResponseFormat;

        EncodeOptions()
        {
            format = FORMAT_USE_TDF_NAMES;
            xmlResponseFormat = XML_RESPONSE_FORMAT_ELEMENTS;
            enumFormat = ENUM_FORMAT_IDENTIFIER;
            useCommonEntryName = false;
        }

        ResponseFormat format;
        XmlResponseFormat xmlResponseFormat;
        EnumResponseFormat enumFormat;
        bool useCommonEntryName;
    };


class EATDF_API TdfEncoder
{
public:
    static const int32_t MAX_ENCODER_ERR_MESSAGE_LEN = 255;

    TdfEncoder()
    {
        *mErrorMessage = '\0';
        *(mErrorMessage + MAX_ENCODER_ERR_MESSAGE_LEN) = '\0';
    }
    virtual ~TdfEncoder() {}

    virtual bool encode(EA::IO::IStream& iStream, const Tdf& tdf, const EncodeOptions* encOptions = nullptr, const MemberVisitOptions& visitOptions = MemberVisitOptions()) = 0;

/*************************************************************************************************/
/*!
    \brief convertMemberToElement

    Utility method to take a member name and convert it into an element name by doing the 
    following:  strip the leading 'm' or 'm_' member tag, and convert everything to lowercase

    \param[in] memberName - Unmodified member name
    \param[in] buf - Where to write the modified member
    \param[in] buflen - Length in bytes of buf

    \return - true if conversion fits in the provided buffer, false if there isn't enough room or parameters are invalid
*/
/*************************************************************************************************/
    bool convertMemberToElement(const char8_t *memberName, char8_t* buf, const size_t buflen) const;

    const char8_t* getErrorMessage() const
    {
        return mErrorMessage;
    }

protected:
    char8_t mErrorMessage[MAX_ENCODER_ERR_MESSAGE_LEN + 1];
};

} //namespace TDF
} //namespace EA

#endif // EA_TDF_TDFENCODER_H
