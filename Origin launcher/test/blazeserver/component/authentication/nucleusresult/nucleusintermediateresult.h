/*************************************************************************************************/
/*!
    \file nucleusgetpersonaresult.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/
#ifndef BLAZE_AUTHENTICATION_NUCLEUSINTERMEDIATERESULT_H
#define BLAZE_AUTHENTICATION_NUCLEUSINTERMEDIATERESULT_H

/*** Include files *******************************************************************************/
#include "authentication/nucleusresult/nucleusresult.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{
namespace Authentication
{

class NucleusIntermediateResult : public NucleusResult
{
public:
    NucleusIntermediateResult() { clearAll(); }
    ~NucleusIntermediateResult() override { }

    void clearAll() override;

    void setAttributes(const char8_t* fullname,
        const Blaze::XmlAttribute* attributes, size_t attributeCount) override;

    void setValue(const char8_t* fullname, const char8_t* data, size_t dataLen) override;

    const char8_t* getURI() const { return mURI; }


protected:

private:

    char8_t mURI[256];

    // The following two methods are intentionally private and unimplemented to prevent passing
    // and returning objects by value.  The compiler will give a linking error if either is used.

    // Copy constructor
    NucleusIntermediateResult(const NucleusIntermediateResult& otherObj);

    // Assignment operator
    NucleusIntermediateResult& operator=(const NucleusIntermediateResult& otherObj);
};

} // Authentication
} // Blaze

#endif // BLAZE_AUTHENTICATION_NUCLEUSINTERMEDIATERESULT_H
