/*************************************************************************************************/
/*!
    \file vaultinvalidatehandler.h

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include guard *******************************************************************************/

#ifndef VAULT_INVALIDATE_HANDLER_H
#define VAULT_INVALIDATE_HANDLER_H

/*** Include files *******************************************************************************/


/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

template <class Type> struct VaultInvalidateHandler
{
public:
    typedef Type InvalidateType;
    VaultInvalidateHandler() { }
    virtual ~VaultInvalidateHandler() { }
    virtual void handleInvalidate(const InvalidateType &instance) { }
};

} // namespace Blaze

#endif // VAULT_INVALIDATE_HANDLER_H
