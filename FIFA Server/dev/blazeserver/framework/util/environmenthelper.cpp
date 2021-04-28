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

#ifdef EA_PLATFORM_LINUX
#include <stdio.h>
#include <string.h>
#include <sys/user.h>
#include <fcntl.h>
#endif

#include "framework/util/environmenthelper.h"

namespace Blaze
{

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

/*** Public Methods ******************************************************************************/

void EnvironmentHelper::getCmdLine(int32_t& argc, char8_t**& argv)
{

#if defined (EA_PLATFORM_WINDOWS)
    //Super simple on windows:
    //These are compiler defined variables pointing to the argc/argv hooked in by the OS
    argc = __argc;
    argv = __argv;
#elif defined(EA_PLATFORM_LINUX)

    //If only things were so easy in linux.  We need to go into the proc cmdline file to 
    //find out what the actual command line is.  This memory is pulled from the same area 
    //as the actual argc/argv, so you're guarunteed to get the right values, but its kind of 
    //a round about way to do it.  Supposedly there is an equivalent global argc/argv var as in windows
    //but the GNU libc stuff just doesn't expose it.

    static char cmdLine[PAGE_SIZE]; //The largest size of the data is PAGE_SIZE
    static char* newArgV[PAGE_SIZE]; //We can't allocate anything in this method, as its called prior to allocators running, so 
                                     //assume that we can't have anything larger than PAGE_SIZE args.  A bit wasteful, but better
                                     //to be correct here than save a few bytes
    static int newArgC = 0;

    if (newArgC == 0)
    {
        //argc can't be zero if we've been initialized
        memset(cmdLine, 0, sizeof(cmdLine));
        memset(newArgV, 0, sizeof(newArgV));

        //Read in the proc cmdline file.  This will contain the same nullptr separated list pointed to by our internal 
        //memory
        int32_t fd =  open("/proc/self/cmdline", O_RDONLY);
        if (fd != -1)
        {
            //Slurp in the whole thing.  Under linux, kernel limitations dictate that the string can be no longer than
            //a single PAGE_SIZE of memory.  Use low level open,read,close commands to avoid calls to malloc
            int32_t bytesRead = 0;
            size_t totalBytesRead = 0;
            
            do
            {
                bytesRead = read(fd, cmdLine + totalBytesRead, PAGE_SIZE - totalBytesRead);
                if (bytesRead > 0)
                {
                    totalBytesRead += bytesRead;
                }
            } while (bytesRead > 0);
            close(fd);

            //Break up the command line buffer by scanning through it and finding all the nullptr breaks. 
            //because we memset prior to this, hitting a double nullptr or reaching the end of the 
            //array is our terminus.  We can't use strtok or the like because we're using a \0 as the delimiter
            newArgV[0] = cmdLine;
            newArgC++;
            for (size_t x = 1; x < totalBytesRead - 1; ++x)
            {
                if (cmdLine[x] == '\0')
                {
                    if (cmdLine[x + 1] != '\0')
                    {
                        newArgV[newArgC++] = &cmdLine[x + 1];
                        x++;
                    }
                    else
                    {
                        break;
                    }
                }
            }

        }
    }

    argc = newArgC;
    argv = newArgV;
#endif
}

const char8_t* EnvironmentHelper::getCmdLineOption(const char8_t* optName, bool hasValue)
{
    int32_t argc;
    char8_t** argv;
    const char8_t* result = nullptr;
    getCmdLine(argc, argv);

    int32_t maxArgC = hasValue ? argc - 1 : argc; //make sure we don't go over the loop if we're expecting a value
    for (int32_t counter = 0; counter < maxArgC; ++counter)
    {
        if (blaze_strcmp(argv[counter], optName) == 0)
        {
            result = hasValue ? argv[counter + 1] : argv[counter]; 
            break;
        }
    }

    return result;
}


} // Blaze

