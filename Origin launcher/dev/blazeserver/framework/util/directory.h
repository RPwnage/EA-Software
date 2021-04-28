
/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_DIRECTORY_H
#define BLAZE_DIRECTORY_H

/*** Include files *******************************************************************************/

#include "EASTL/vector.h"
#include "EASTL/string.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

class Directory
{
    NON_COPYABLE(Directory);

public:
    typedef eastl::vector<eastl::string> FilenameList;

    static void getDirectoryFilenames(const char8_t* directory, const char8_t* extension, 
            FilenameList& files);
    
};

} // Blaze

#endif // BLAZE_DIRECTORY_H

