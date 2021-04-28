/*************************************************************************************************/
/*!
    \file

    
    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_LESS_PTR_H
#define BLAZE_LESS_PTR_H

/*** Include files *******************************************************************************/
#include "EASTL/functional.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/
namespace Blaze
{

// Binary less function for use when storing object pointers in STL containers instead
// of object instances.  This calls the normal operator<() function on the class
// by deferencing the pointers.
template<typename T>
struct less_ptr : eastl::binary_function<T, T, bool>
{
    bool operator() (const T lhs, const T rhs) const  { return (*lhs) < (*rhs); }
};

} // Blaze
#endif // BLAZE_LESS_PTR_H
