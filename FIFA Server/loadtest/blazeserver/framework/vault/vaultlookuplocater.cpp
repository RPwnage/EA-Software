/*************************************************************************************************/
/*!
    \file vaultlookuplocater.cpp

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

#include "framework/blaze.h"
#include "framework/tdf/frameworkconfigtypes_server.h"
#include "vaultlookuplocater.h"
#include "vaultmanager.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

/*************************************************************************************************/
/*!
    \class VaultLookupLocater

    Walks a TDF object and the child objects searching for Blaze::VaultLookup members.
    Creates a map where each key is a VaultLookup path, and each entry is a list of TDFs
    that are direct parents of a VaultLookup with said path.
*/
/*************************************************************************************************/

/******************************************************************************/
/*! 
    \brief Constructor for the vault lookup locater.
*/
/******************************************************************************/
VaultConfigLookupLocater::VaultConfigLookupLocater()
{ 
}

/******************************************************************************/
/*! 
    \brief Destructor for the vault lookup locater.
*/
/******************************************************************************/
VaultConfigLookupLocater::~VaultConfigLookupLocater()
{
}

/******************************************************************************/
/*! 
    \brief Recursively searches the given TDF for Blaze::VaultLookup members.
           Creates a lookup map where each key is a VaultLookup path, and each entry
           is a list of TDFs that are direct parents of a VaultLookup with said path.

    \param[in] tdf - the TDF configuration

    \return the lookup map
*/
/******************************************************************************/
VaultConfigLookupLocater::LookupMap& VaultConfigLookupLocater::lookup(EA::TDF::Tdf &tdf)
{
    mLookupMap.clear();
    EA::TDF::MemberVisitOptions options;
    tdf.visit(*this, options);
    return mLookupMap;
}

/******************************************************************************/
/*! 
    \return true

    \note Always returns true so that the member visitor will recursively visit
          each of the TDF members.
*/
/******************************************************************************/
bool VaultConfigLookupLocater::visitValue(const EA::TDF::TdfVisitContextConst& context)
{ 
    return true; 
}  

/******************************************************************************/
/*! 
    \brief If the TDF is a Blaze::VaultLookup instance, appends its parent to
           the list of TdfGenericReferences in the lookup map entry keyed
           by the VaultLookup's path.

    \param[in] context - the TDF visitor context

    \return true

    \note Always returns true so that the member visitor will recursively visit
          each of the TDF members.
    \see postVisitValue
*/
/******************************************************************************/
bool VaultConfigLookupLocater::postVisitValue(const EA::TDF::TdfVisitContextConst& context)
{ 
    if (context.getType() == EA::TDF::TDF_ACTUAL_TYPE_TDF &&
        context.getMemberInfo() != nullptr && 
        context.getMemberInfo()->getTypeId() == VaultLookup::TDF_ID)
    {

        eastl::string path;

        EA::TDF::TdfGenericReferenceConst pathRef;
        if (context.getValue().asTdf().getValueByTag(VaultLookup::TAG_PATH, pathRef))
        {
            path = pathRef.asString().c_str();
        }

        if (!path.empty())
        {
            mLookupMap[path.c_str()].emplace_back(context.getParent()->getValue());
        }
    }
    return true; 
} 

} // namespace Blaze
