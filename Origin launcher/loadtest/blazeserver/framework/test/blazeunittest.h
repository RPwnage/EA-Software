/*************************************************************************************************/
/*!
    \file header file containing macros used by Blaze unit tests. acronym BUT stands for "Blaze Unit Test"


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/
#ifndef __blazeunittest_h__
#define __blazeunittest_h__

/*** Include files *******************************************************************************/
#include <iostream>

/*** Defines/Macros/Constants/Typedefs ***********************************************************/
template<class X, class Y, class D>
bool delta(X x, Y y, D d)
{
    return ((y >= x - d) && (y <= x + d));
}

#define BUT_ASSERT(_condition) \
do { \
    if(!(_condition)) \
    { \
        std::cout << "BUT_ASSERT - " << __FILE__ << ":" << __LINE__ << "\n\n"; \
    } \
} while (0)

#define BUT_ASSERT_MSG(_msg, _condition) \
do { \
    if(!(_condition)) \
    { \
        std::cout << "BUT_ASSERT_MSG - " << _msg << " - "<< __FILE__ << ":" << __LINE__ << "\n\n"; \
    } \
} while (0)

#define BUT_ASSERT_DELTA(_x, _y, _delta) \
do { \
    if(!delta( _x, _y, _delta)) \
    { \
        std::cout << "BUT_ASSERT_DELTA - " << __FILE__ << ":" << __LINE__ << "\n\n"; \
    } \
} while (0)

#endif // __blazeunitttest_h__
