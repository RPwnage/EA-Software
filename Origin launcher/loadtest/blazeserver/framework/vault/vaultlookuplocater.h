/*************************************************************************************************/
/*!
    \file vaultlookuplocater.h

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include guard *******************************************************************************/

#ifndef VAULT_LOOKUP_LOCATER_H
#define VAULT_LOOKUP_LOCATER_H

/*** Include files *******************************************************************************/

#include <EATDF/tdf.h>

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

class VaultConfigLookupLocater : private EA::TDF::TdfMemberVisitorConst
{
public:
    typedef eastl::vector<EA::TDF::TdfGenericReference> TdfParentList;
    typedef eastl::hash_map<eastl::string, TdfParentList> LookupMap;

    VaultConfigLookupLocater();
    ~VaultConfigLookupLocater() override;
    LookupMap& getLookups() { return mLookupMap; }
    LookupMap& lookup(EA::TDF::Tdf &tdf);
    bool visitValue(const EA::TDF::TdfVisitContextConst &context) override;
    bool postVisitValue(const EA::TDF::TdfVisitContextConst &context) override; 

private:
    LookupMap mLookupMap;
};

} // namespace Blaze

#endif // VAULT_LOOKUP_LOCATER_H
