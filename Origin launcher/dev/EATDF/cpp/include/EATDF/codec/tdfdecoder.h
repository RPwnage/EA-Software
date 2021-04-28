/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef EA_TDF_TDFDECODER_H
#define EA_TDF_TDFDECODER_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

/*** Include files *******************************************************************************/
#include <EATDF/tdfvisit.h>
#include <EAIO/EAStream.h>

namespace EA
{
namespace TDF
{

class EATDF_API TdfDecoder 
{
public:
    static const int32_t MAX_DECODER_ERR_MESSAGE_LEN = 255;

    TdfDecoder() 
    {
        *mErrorMessage = *(mErrorMessage + MAX_DECODER_ERR_MESSAGE_LEN) = '\0';
    }
    virtual ~TdfDecoder() {}
    virtual bool decode(EA::IO::IStream& iStream, EA::TDF::Tdf& tdf, const MemberVisitOptions& options = MemberVisitOptions()) = 0;

    virtual const char8_t* getName() const = 0;

    const char8_t* getErrorMessage() const
    {
        return mErrorMessage;
    }

protected:
    char8_t mErrorMessage[MAX_DECODER_ERR_MESSAGE_LEN + 1];
};

} //namespace TDF
} //namespace EA

#endif // EA_TDF_TDFDECODER_H


