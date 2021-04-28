/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_DBUTIL_H
#define BLAZE_DBUTIL_H

/*** Include files *******************************************************************************/

#include "EATDF/time.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

class DbUtil
{
public:
    static EA::TDF::TimeValue dateTimeToTimeValue(const char8_t* dbDateTime);

private:
    DbUtil();
};

}// Blaze
#endif // BLAZE_DBUTIL_H
