/**************************************************************************************************/
/*!
    \file 

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/**************************************************************************************************/

#ifndef BLAZE_BLAZEPERMISSIONS_H
#define BLAZE_BLAZEPERMISSIONS_H

/*** Include Files ********************************************************************************/

/*** Defines/Macros/Constants/Typedefs ************************************************************/

namespace Blaze
{

#define BLAZE_COMPONENT_PERMISSION(component, code) ((code << 16) | (component & ~(RPC_MASK_MASTER)))
#define BLAZE_COMPONENT_FROM_PERMISSION(perm) (static_cast<uint16_t>(perm & 0xFFFF))

namespace Authorization
{
    typedef uint32_t Permission;

    const Permission PERMISSION_NONE = 0x00000000;
    const Permission PERMISSION_ALL  = 0xFFFFFFFF;
}

} // Blaze


#endif // BLAZE_BLAZEPERMISSIONS_H

