/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class DbUtil

    Encapsulates utility commands for clients of the database.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

#include "framework/blaze.h"
#include "framework/database/dbutil.h"
#include "framework/util/shared/blazestring.h"
#include "EATDF/time.h"

namespace Blaze
{

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

/*** Public Methods ******************************************************************************/

/*************************************************************************************************/
/*!
    \brief dateTimeToEpoch

    Convert a database date time "YYYY-MM-DD hh:mm:ss" formatted string to a TimeValue

    \param[in] dbDateTime - a string date time from the database

    \return - seconds since the epoch, or 0 if parsing failed
*/
/*************************************************************************************************/
TimeValue DbUtil::dateTimeToTimeValue(const char8_t* dbDateTime)
{
    // Return a 0 time if the formatting of the input string is bad
    if (strlen(dbDateTime) != 19)
    {
        return 0;
    }
    if (dbDateTime[4] != '-' ||
        dbDateTime[7] != '-' ||
        dbDateTime[10] != ' ' ||
        dbDateTime[13] != ':' ||
        dbDateTime[16] != ':')
    {
        return 0;
    }

    if (blaze_strcmp(dbDateTime, "0000-00-00 00:00:00") == 0)
    {
        return 0;
    }

    uint32_t year = 0;
    uint32_t month = 0;
    uint32_t day = 0;
    uint32_t hour = 0;
    uint32_t minute = 0;
    uint32_t second = 0;
    blaze_str2int(dbDateTime + 0, &year);
    blaze_str2int(dbDateTime + 5, &month);
    blaze_str2int(dbDateTime + 8, &day);
    blaze_str2int(dbDateTime + 11, &hour);
    blaze_str2int(dbDateTime + 14, &minute);
    blaze_str2int(dbDateTime + 17, &second);

    return TimeValue(year, month, day, hour, minute, second);
}

}// Blaze
