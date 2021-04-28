/*************************************************************************************************/
/*!
    \file nucleusresult.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/
#ifndef BLAZE_AUTHENTICATION_NUCLEUSRESULT_H
#define BLAZE_AUTHENTICATION_NUCLEUSRESULT_H

/*** Include files *******************************************************************************/
#include "framework/protocol/outboundhttpresult.h"

#include "framework/tdf/nucleuscodes.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{
// Forward declarations
struct XmlAttribute;

namespace Authentication
{

class NucleusResult : public OutboundHttpResult
{
public:
    NucleusResult() { clearError(); }
    ~NucleusResult() override {}

    bool checkSetError(const char8_t* fullname,
            const Blaze::XmlAttribute* attributes, size_t attributeCount) override;

    void setUserSecurityState(const char8_t* securityState) override;
    
    void setError(const char8_t* code, const char8_t* field, const char8_t* cause);

    const bool hasError() const { return mHasError; }
    const bool isSuccess() const { return !mHasError; }
    Blaze::Nucleus::NucleusCode::Code getCode() const { return mCode; }
    Blaze::Nucleus::NucleusField::Code getField() const { return !mFieldErrors.empty() ? mFieldErrors[0].mField : Blaze::Nucleus::NucleusField::UNKNOWN; }
    Blaze::Nucleus::NucleusCause::Code getCause() const { return !mFieldErrors.empty() ? mFieldErrors[0].mCause : Blaze::Nucleus::NucleusCause::UNKNOWN; }

    const char8_t* getCodeString() const { return mCodeString; }
    const char8_t* getFieldString() const { return !mFieldErrors.empty() ? mFieldErrors[0].mFieldString : ""; }
    const char8_t* getCauseString() const { return !mFieldErrors.empty() ? mFieldErrors[0].mCauseString : ""; }
    const char8_t* getUserSecurityState() const {return mSecurityState ;};

    // Used to store the error codes.
    struct FieldError
    {
        Blaze::Nucleus::NucleusField::Code mField;
        Blaze::Nucleus::NucleusCause::Code mCause;
        char8_t mFieldString[128];
        char8_t mCauseString[256];
    };
    typedef eastl::vector<FieldError> FieldErrorList;

    const FieldErrorList &getFieldErrors() const { return mFieldErrors; }

    void setHttpError(BlazeRpcError err = ERR_SYSTEM) override;
    
    virtual void clearAll() = 0;

protected:
    void clearError() { mCode = Blaze::Nucleus::NucleusCode::UNKNOWN; mCodeString[0] = 0; mFieldErrors.clear(); mHasError = false; mSecurityState[0] = 0 ;}
    bool mHasError;

private:

    Blaze::Nucleus::NucleusCode::Code mCode;
    char8_t mCodeString[128];
    FieldErrorList mFieldErrors;
    char8_t mSecurityState[32];

    // The following two methods are intentionally private and unimplemented to prevent passing
    // and returning objects by value.  The compiler will give a linking error if either is used.

    // Copy constructor
    NucleusResult(const NucleusResult& otherObj);

    // Assignment operator
    NucleusResult& operator=(const NucleusResult& otherObj);
};

} // Authentication
} // Blaze

#endif // BLAZE_AUTHENTICATION_NUCLEUSRESULT_H
