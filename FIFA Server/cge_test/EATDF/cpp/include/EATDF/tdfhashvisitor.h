/*! *********************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*************************************************************************************************/

#ifndef EA_TDF_HASH_VISTOR_H
#define EA_TDF_HASH_VISTOR_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

/************* Include files ***************************************************************************/

#include <EATDF/tdfvisit.h>

/************ Defines/Macros/Constants/Typedefs *******************************************************/

namespace EA
{

namespace TDF
{

typedef uint32_t TdfHashValue;

class EATDF_API TdfHashVisitor : public TdfMemberVisitorConst
{
public:
    TdfHashVisitor();
    ~TdfHashVisitor() override {};

    bool visitValue(const TdfVisitContextConst& context) override { return true; };
    bool postVisitValue(const TdfVisitContextConst& context) override;

    TdfHashValue getHash() const { return mHash; }

private:
    TdfHashValue mHash;
};

} //namespace TDF

} //namespace EA


#endif // EA_TDF_HASH_VISTOR_H


