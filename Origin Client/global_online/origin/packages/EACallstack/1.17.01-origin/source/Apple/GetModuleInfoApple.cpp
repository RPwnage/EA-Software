
///////////////////////////////////////////////////////////////////////////////
// GetModuleInfoApple.cpp
//
// Copyright (c) 2010 Electronic Arts Inc.
// Created by Paul Pedriana
///////////////////////////////////////////////////////////////////////////////


#include <EACallstack/Apple/EACallstackApple.h>
#include <EAStdC/EAString.h>
#include <EAStdC/EAScanf.h>
#include <EAIO/PathString.h>
#include <EASTL/string.h>
#include <unistd.h>

namespace EA
{
namespace Callstack
{
// GetModuleInfoApple
//
// This function determines the UUID of a given module and 
void GetModuleUuid(eastl::string& modulePath, eastl::string& moduleUuid)
{
    moduleUuid.clear();
    
    char command[PATH_MAX];
    sprintf(command, "otool -l %s | grep uuid", modulePath.data());
    FILE*  pFile = popen(command, "r");
    if (pFile)
    {
        char line[512];
        
        while(fgets(line, EAArrayCount(line), pFile))
        {
            char cmd[16];
            char uuid[64];
            int fieldCount = EA::StdC::Sscanf(line, "%16s %64s", cmd, uuid);
            if (fieldCount == 2 && strcmp(cmd, "uuid") == 0)
            {
                moduleUuid.assign(uuid);
            }
        }
        
        pclose(pFile);
    }
}

void GetAllModuleInfoAppleUUIDs(ModuleInfoAppleArray& moduleInfos)
{
    typedef eastl::map<eastl::string, eastl::string> UuidMap;
    UuidMap uuids;
    
    ModuleInfoAppleArray::iterator end(moduleInfos.end());
    for(ModuleInfoAppleArray::iterator i(moduleInfos.begin()); i != end; ++i)
    {
        eastl::string path(&(i->mPath[0]));
        UuidMap::iterator where = uuids.find(path);
        if (where != uuids.end())
        {
            strncpy(i->mUuid, where->second.c_str(), sizeof(i->mUuid));
        }
        else
        {
            eastl::string uuid;
            GetModuleUuid(path, uuid);
            if (!uuid.empty())
            {
                uuids[path] = uuid;
                strncpy(i->mUuid, uuid.c_str(), sizeof(i->mUuid));
            }
        }
    }
}


// GetModuleInfoApple
//
// This function exists for the purpose of being a central module/VM map info collecting function,
// used by a couple functions within EACallstack.
//
// https://developer.apple.com/library/mac/#documentation/Darwin/Reference/ManPages/man1/vmmap.1.html
// Executing this:
//     MacBook:~ PPedriana$ vmmap 26459
// Results in this:
//     . . .
//     ==== Non-writable regions for process 26459
//     __TEXT                 00000001094aa000-0000000109546000 [  624K] r-x/rwx SM=COW  /bin/bash
//     __LINKEDIT             0000000109554000-000000010955d000 [   36K] r--/rwx SM=COW  /bin/bash
//     MALLOC metadata        000000010955d000-000000010955e000 [    4K] r--/rwx SM=COW  
//     MALLOC guard page      000000010958c000-000000010958d000 [    4K] ---/rwx SM=NUL  
//     MALLOC metadata        000000010958d000-000000010958e000 [    4K] r--/rwx SM=COW  
//     VM_ALLOCATE            000000010958e000-000000010958f000 [    4K] r--/rw- SM=ALI  
//     STACK GUARD            00007fff650aa000-00007fff688aa000 [ 56.0M] ---/rwx SM=NUL  stack guard for thread 0
//     __TEXT                 00007fff690aa000-00007fff690df000 [  212K] r-x/rwx SM=COW  /usr/lib/dyld
//
//     ==== Writable regions for process 26459
//     . . .
//
ModuleInfoAppleArray gModuleInfoAppleArrayCache;

size_t GetModuleInfoApple(ModuleInfoAppleArray& moduleInfoAppleArray, const char* pTypeFilter, bool bEnableCache)
{
    if(bEnableCache)
    {
        if(gModuleInfoAppleArrayCache.empty())
            GetModuleInfoApple(gModuleInfoAppleArrayCache, NULL, false); // Call ourselves recursively.

        for(eastl_size_t i = 0, iEnd = gModuleInfoAppleArrayCache.size(); i != iEnd; i++)
        {
            const ModuleInfoApple& mia = gModuleInfoAppleArrayCache[i];

            if(!pTypeFilter || strstr(mia.mType, pTypeFilter))
                moduleInfoAppleArray.push_back(mia);
        }
    }
    else
    {
        char   command[32];
        pid_t  pid = getpid();
        sprintf(command, "vmmap -w %lld", (int64_t)pid);
        FILE*  pFile = popen(command, "r");

        if(pFile)
        {
            char line[512];
            bool bFoundStart   = false;
            bool bFoundRegions = false;
           
            ////////////////////////////////////////////////////////////////////////////////////
            // Problem: vmmap may require a higher privilege than the running app has. Can that happen  
            // even though this process is asking for information about itself?

            // The following logic is fairly hard-coded to the output of the vmmap utility. If it changes
            // significantly then the following may stop working.
            ////////////////////////////////////////////////////////////////////////////////////
            
            while(fgets(line, EAArrayCount(line), pFile))
            {
                TryAgain:
                if(!bFoundStart)
                {
                    if(strstr(line, "Virtual") != line) // We expect the first line to read "Virtual Memory Map of process ..." 
                    {
                        // Looks like the data isn't what we expect, so quit.
                        // Need to kill the process?
                        break;
                    }
                    
                    bFoundStart = true;
                }
                else if(!bFoundRegions)
                {
                    if(strstr(line, "regions for process") != NULL) // If we've found the start of the lines we want...
                        bFoundRegions = true;
                }
                else if((strlen(line) < 8) || (strstr(line, "====") == line)) // If we've reached the end of a region...
                {
                    bFoundRegions = false; // Go back to waiting for regions. There are usually two sets: read-only and read-write.
                    goto TryAgain;
                }
                else
                {
                    // The line is of the form (64 bit):
                    //__TEXT                 00000001094aa000-0000000109546000 [  624K] r-x/rwx SM=COW  /bin/bash
                    //STACK GUARD            00007fff650aa000-00007fff688aa000 [ 56.0M] ---/rwx SM=NUL  stack guard for thread 0
                    ModuleInfoApple& info = moduleInfoAppleArray.push_back();

                    uint64_t addressEnd;
                    char     modulePath[512];
                    int      fieldCount = EA::StdC::Sscanf(line, "%32s %I64x-%I64x [ %*s %16s %*s %[^\n]", info.mType, &info.mBaseAddress, &addressEnd, info.mPermissions, modulePath);
                    
                    if((fieldCount == 5) && (!pTypeFilter || strstr(info.mType, pTypeFilter)))
                    {
                        info.mSize = (addressEnd - info.mBaseAddress);
                        info.mPath = modulePath;
                        info.mName = EA::IO::Path::GetFileName(modulePath);
                    }
                    else
                        moduleInfoAppleArray.pop_back();
                }
            }
            
            pclose(pFile);
        }
        
        GetAllModuleInfoAppleUUIDs(moduleInfoAppleArray);
    }
    
    return (size_t)moduleInfoAppleArray.size();
}



}
}

