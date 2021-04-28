/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_QUANTIZE_H
#define BLAZE_QUANTIZE_H

/*** Include files *******************************************************************************/


/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

    /** \brief Maps incoming value to the *numerically* closest element in data[] using binary search. data[] must be sorted. */
    uint32_t quantizeToNearestValue(uint32_t value, const uint32_t data[], size_t count);


} // Blaze

#endif // BLAZE_QUANTIZE_H
