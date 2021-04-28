/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class Directory

    Provides utility helpers to fetch the list of files in directory.

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

#include "framework/blaze.h"
#ifdef EA_PLATFORM_WINDOWS
#include <io.h>
#else
#include <dirent.h>
#endif
#include <sys/stat.h>
#include "framework/util/directory.h"
#include "framework/util/shared/blazestring.h"

namespace Blaze
{

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

/*** Public Methods ******************************************************************************/

/*************************************************************************************************/
/*!
     \brief getDirectoryFilenames
 
      Return a list of files that exist in the provided directory with the matching extension.
      Subdirectories are excluded.
  
     \param[in] directory - Directory to get file list for
     \param[in] extension - If provided, only match files with given extension.  nullptr for all files
 
*/
/*************************************************************************************************/
#ifdef EA_PLATFORM_WINDOWS
// static
void Directory::getDirectoryFilenames(const char8_t* directory, const char8_t* extension,
        Directory::FilenameList& list)
{
    char8_t searchSpec[256];
    if (extension == nullptr)
        extension = "";
    blaze_snzprintf(searchSpec, sizeof(searchSpec), "%s/*%s", directory, extension);

    struct _finddata_t fileInfo;
    intptr_t handle = _findfirst(searchSpec, &fileInfo);
    if (handle == -1)
        return;

    int32_t rc = 0;
    while (rc != -1)
    {
        char8_t fname[256];
        blaze_snzprintf(fname, sizeof(fname), "%s/%s", directory, fileInfo.name);

        // Make sure the entry is a regular file and not a directory, etc
        if ((fileInfo.attrib & (_A_SYSTEM | _A_SUBDIR)) == 0)
        {
            list.push_back(fname);
        }

        rc = _findnext(handle, &fileInfo);
    }
    _findclose(handle);
}

#else

// static
void Directory::getDirectoryFilenames(const char8_t* directory, const char8_t* extension,
        Directory::FilenameList& list)
{

    DIR* dir = opendir(directory);
    if (dir == nullptr)
        return;

    size_t extLen = 0;
    if ((extension != nullptr) && (extension[0] != '\0'))
        extLen = strlen(extension);

    dirent* ent;
    while ((ent = readdir(dir)) != nullptr)
    {
        if (extLen > 0)
        {
            // Check if the extension matches
            size_t len = strlen(ent->d_name);
            if (len < extLen)
                continue;

            bool match = true;
            for(size_t idx = 0; (idx < extLen) && match; ++idx)
            {
                if (ent->d_name[len - extLen + idx] != extension[idx])
                    match = false;
            }
            if (!match)
                continue;
        }

        char8_t fname[256];
        blaze_snzprintf(fname, sizeof(fname), "%s/%s", directory, ent->d_name);

        // Make sure the entry is a regular file and not a directory, etc
        struct stat st;
        if (stat(fname, &st) == -1)
            continue;
        if (!S_ISREG(st.st_mode))
            continue;

        list.push_back(fname);
    }
    closedir(dir);
}
#endif


} // Blaze

