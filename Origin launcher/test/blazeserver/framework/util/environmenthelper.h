
/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_ENVIRONMENTHELPER_H
#define BLAZE_ENVIRONMENTHELPER_H

/*** Include files *******************************************************************************/


/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

class EnvironmentHelper
{
    NON_COPYABLE(EnvironmentHelper);

public:
    /* Gets the command line arguments that have been passed/will be passed into main() */
    static void getCmdLine(int32_t& argc, char8_t**& argv);

    /* Returns the pointer to the passed optName if found, or the trailing argument if found and hasValue is true.  Returns nullptr if not found */
    static const char8_t* getCmdLineOption(const char8_t* optName, bool hasValue = false);
};

} // Blaze

#endif // BLAZE_ENVIRONMENTHELPER_H

