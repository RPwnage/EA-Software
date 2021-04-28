//
//  Windows.cpp
//  sharedUtils
//
//  Created by Frederic Meraud on 5/25/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include "MacWindows.h"

#if defined(ORIGIN_MAC)

#include <Carbon/Carbon.h>
#include <sys/sysctl.h>
#include <dlfcn.h>
#include <sys/syslog.h>

#include "IGOTrace.h"


namespace Origin
{
    
namespace Mac
{

// Had to create this guy instead of directly using syslog because of conflicts with services/log/logServices
void LogMessage(const char * format, ...)
{
    va_list argp;
    va_start(argp, format);
    vsyslog(LOG_NOTICE, format, argp);
    va_end(argp);
}

void LogError(const char * format, ...)
{
    va_list argp;
    va_start(argp, format);
    vsyslog(LOG_ERR, format, argp);
    va_end(argp);
}

Semaphore::Semaphore(int count) 
    : _count(count) 
{
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    _isValid = pthread_mutex_init(&_mutex, NULL) | pthread_cond_init(&_cond, NULL);
    
    pthread_mutexattr_destroy(&attr);
}
    
Semaphore::~Semaphore()
{
    pthread_cond_destroy(&_cond);
    pthread_mutex_destroy(&_mutex);
}
    
bool Semaphore::IsValid() 
{ 
    return _isValid == 0; 
}
    
void Semaphore::Signal()
{
    pthread_mutex_lock(&_mutex);
    ++_count;
    if (_count > 0)
        pthread_cond_broadcast(&_cond);
    
    pthread_mutex_unlock(&_mutex);
}

    
void Semaphore::Wait()
{
    pthread_mutex_lock(&_mutex);
    InnerWait(NULL);
    pthread_mutex_unlock(&_mutex);
}

bool Semaphore::Wait(unsigned int timeoutInMS)
{
    struct timeval tv;
    struct timespec later;
    gettimeofday(&tv, NULL);
    later.tv_sec = tv.tv_sec + timeoutInMS / 1000;
    later.tv_nsec = (tv.tv_usec + ((timeoutInMS % 1000) * 1000)) * 1000;
    
    pthread_mutex_lock(&_mutex);
    bool success = InnerWait(&later);
    pthread_mutex_unlock(&_mutex);
    
    return success;
}
    
bool Semaphore::InnerWait(const timespec* abstime)
{
    if (_count <= 0)
    {
        int retVal = 0;
        do 
        {
            if (abstime)
                retVal = pthread_cond_timedwait(&_cond, &_mutex, abstime);
            else
                retVal = pthread_cond_wait(&_cond, &_mutex);
            
        } while (_count <= 0 && retVal != ETIMEDOUT);
    }
    
    if (_count > 0)
    {
        --_count;
        return true;
    }
    
    else
        return false;
}

// Returns 0 if successful; otherwise returns -1 (buffer not defined, buffer size too small, unable to retrieve home directory)
int GetHomeDirectory(char* buffer, size_t bufferSize)
{
    if (!buffer || bufferSize <= 0)
        return -1;
    
    for (int idx = 0; idx < 2; ++idx)
    {
        const  char* directory = NULL;
        if (idx == 0)
        {
            directory = getenv("HOME");
        }
        
        else
        {
            struct passwd* pw = getpwuid(getuid());
            directory = pw->pw_dir;        
        }
        
        if (directory)
        {
            size_t len = strlen(directory);
            if (snprintf(buffer, bufferSize, "%s", directory) != (int)len)
                return -1;
            
            struct stat dirStats;
            if (stat(buffer, &dirStats) == 0 && S_ISDIR(dirStats.st_mode))
                return 0;
        }
    }
    
    return -1;
}

void SaveRawRGBAToTGA(const char* fileName, size_t width, size_t height, const unsigned char* data, bool overwrite)
{
    if (!overwrite)
    {
        struct stat fileStats;
        if (stat(fileName, &fileStats) != -1 || errno != ENOENT)
            return;
    }
    
    FILE* pFile = fopen(fileName, "wb");
    if (pFile) 
    { 
    
        unsigned char defaultValueChar = 0; 
        short int defaultValueInt = 0;
    
        fwrite(&defaultValueChar, sizeof(unsigned char), 1, pFile);     // idlength: no identification string
        fwrite(&defaultValueChar, sizeof(unsigned char), 1, pFile);     // colourmaptype: no color map

        unsigned char imageType = 2; 
        fwrite(&imageType, sizeof(unsigned char), 1, pFile);            // datatypecode: 2 = uncompressed RGB
    
        fwrite(&defaultValueInt, sizeof(short int), 1, pFile);          // colourmaporigin
        fwrite(&defaultValueInt, sizeof(short int), 1, pFile);          // colourmaplength
        fwrite(&defaultValueChar, sizeof(unsigned char), 1, pFile);     // colourmapdepth
        fwrite(&defaultValueInt, sizeof(short int), 1, pFile);          // x_origin
        fwrite(&defaultValueInt, sizeof(short int), 1, pFile);          // y_origin
    
        fwrite(&width, sizeof(short int), 1, pFile);                    // width
        fwrite(&height, sizeof(short int), 1, pFile);                   // height
        
        unsigned char bits = 32;
        fwrite(&bits, sizeof(unsigned char), 1, pFile);                 // bitsperpixel
        
        unsigned char descriptor = 0x28;
        fwrite(&descriptor, sizeof(unsigned char), 1, pFile);           // imagedescriptor: Targa 32 (Bits 3-0) = 8
                                                                        // + Origin in upper left-hand corner (Bit5)
    
        int colorMode = 4; 
        long Size = width * height * colorMode;
        fwrite(data, sizeof(unsigned char), Size, pFile);
        
        fclose(pFile);
    }
}

// Extract the module name that contains the specific address. If addr = NULL, returns the full path to the current executable.
bool GetModuleFileName(void* addr, char* buffer, size_t bufferSize)
{
    bool success = false;
    
    if (!buffer || bufferSize == 0)
        return success;
    
    if (addr)
    {
        // Use the addr to locate the specific module of interest
        Dl_info info;
        if (dladdr(addr, &info))
        {
            if (info.dli_fname)
            {
                size_t len = ::strlen(info.dli_fname);
                if (len < bufferSize)
                {
                    strcpy(buffer, info.dli_fname);
                    success = true;
                }
                
                else
                {
                    IGO_TRACEF("Buffer too small to store module filename (%ld/%ld)", len, bufferSize);
                }
            }
            
            else
            {
                IGO_TRACE("No module name information!!!");
            }
        }
        
        else
        {
            IGO_TRACEF("Unable to locate module that contains addr=%p (%s)", addr, dlerror());
        }
    }
    
    else
    {
        // Look up the current executable full path
        int mib[3];
        int argMax;
        size_t size = sizeof(argMax);
        
        mib[0] = CTL_KERN;
        mib[1] = KERN_ARGMAX;
        
        if (sysctl(mib, 2, &argMax, &size, NULL, 0) == 0)
        {
            char* args = (char*)malloc(argMax);
            
            mib[0] = CTL_KERN;
            mib[1] = KERN_PROCARGS2;
            mib[2] = getpid();
            
            size = argMax;
            if (sysctl(mib, 3, args, &size, NULL, 0) == 0)
            {
                char* fullPath = args + sizeof(int);
                const char* fpStart = strchr(fullPath, '/');
                if (fpStart && *fpStart)
                {
                    size_t len = ::strlen(fpStart);
                    if (len < bufferSize)
                    {
                        strcpy(buffer, fpStart);
                        success = true;
                    }
                }
            }
            
            else
            {
                IGO_TRACEF("Unable to access process arguments (pid=%d)", getpid());
            }
            
            free(args);
        }
        
        else
        {
            IGO_TRACE("Unable to retrieve a process a maximum argument buffer size");
        }
    }
    
    return success;
    
}

int32_t GetHashCode(const char* value)
{
    int hashCode = 1537;
    
    if (!value)
        return -1;
    
    int idx = 0;
    while (value[idx])
    {
        hashCode += (int)value[idx] * 31;
        hashCode <<= 1;
        
        ++idx;
    }
    
    return hashCode;
}
    
int32_t CurrentInputSource()
{
    int32_t retVal = -1;
    CFArrayRef availInputs = TISCreateInputSourceList(NULL, false);
    
    CFIndex inputCount = CFArrayGetCount(availInputs);
    for (CFIndex idx = 0; idx < inputCount; ++idx)
    {
        TISInputSourceRef inputSource = (TISInputSourceRef)CFArrayGetValueAtIndex(availInputs, idx);
        CFBooleanRef isSelected = (CFBooleanRef)TISGetInputSourceProperty(inputSource, kTISPropertyInputSourceIsSelected);
        if (isSelected == kCFBooleanTrue)
        {
            CFStringRef category = (CFStringRef)TISGetInputSourceProperty(inputSource, kTISPropertyInputSourceCategory);
            if (CFStringCompare(category, kTISCategoryKeyboardInputSource, kCFCompareNumerically) == kCFCompareEqualTo)
            {
                char buffer[256];
                
                CFStringRef id = (CFStringRef)TISGetInputSourceProperty(inputSource, kTISPropertyInputSourceID);
                if (CFStringGetCString(id, buffer, sizeof(buffer), kCFStringEncodingISOLatin1))
                    retVal = GetHashCode(buffer);
                
                break;
            }
        }
    }    
    
    CFRelease(availInputs);
    
    return retVal;
}

bool SetInputSource(int32_t id)
{
    if (id == -1)
        return false;

    
    bool retVal = false;
    CFArrayRef availInputs = TISCreateInputSourceList(NULL, false);
    
    CFIndex inputCount = CFArrayGetCount(availInputs);
    for (CFIndex idx = 0; idx < inputCount; ++idx)
    {
        char buffer[256];

        TISInputSourceRef inputSource = (TISInputSourceRef)CFArrayGetValueAtIndex(availInputs, idx);
                
        CFStringRef isID = (CFStringRef)TISGetInputSourceProperty(inputSource, kTISPropertyInputSourceID);
        if (CFStringGetCString(isID, buffer, sizeof(buffer), kCFStringEncodingISOLatin1))
        {
            if (GetHashCode(buffer) == id)
            {
                CFBooleanRef isSelectable = (CFBooleanRef)TISGetInputSourceProperty(inputSource, kTISPropertyInputSourceIsSelectCapable);
                if (isSelectable == kCFBooleanTrue)
                {
                    CFStringRef category = (CFStringRef)TISGetInputSourceProperty(inputSource, kTISPropertyInputSourceCategory);
                    if (CFStringCompare(category, kTISCategoryKeyboardInputSource, kCFCompareNumerically) == kCFCompareEqualTo)
                    {
                        TISSelectInputSource(inputSource);
                        retVal = true;
                    }
                }
                
                break;
            }
        }
    }    

    
    CFRelease(availInputs);
    
    return retVal;
}

int32_t UseNewInputSource(CFIndex incr)
{
    int32_t retVal = -1;
    CFArrayRef availInputs = TISCreateInputSourceList(NULL, false);
    
    CFIndex inputCount = CFArrayGetCount(availInputs);
    CFIndex idx = 0;
    for (; idx < inputCount; ++idx)
    {
        TISInputSourceRef inputSource = (TISInputSourceRef)CFArrayGetValueAtIndex(availInputs, idx);
        CFBooleanRef isSelected = (CFBooleanRef)TISGetInputSourceProperty(inputSource, kTISPropertyInputSourceIsSelected);
        if (isSelected == kCFBooleanTrue)
        {
            CFStringRef category = (CFStringRef)TISGetInputSourceProperty(inputSource, kTISPropertyInputSourceCategory);
            if (CFStringCompare(category, kTISCategoryKeyboardInputSource, kCFCompareNumerically) == kCFCompareEqualTo)
                break;
        }
    }
    
    if (idx != inputCount)
    {
        CFIndex nextIdx = (idx + incr + inputCount) % inputCount;
        for (; nextIdx != idx; nextIdx = (nextIdx + incr) % inputCount)
        {
            TISInputSourceRef inputSource = (TISInputSourceRef)CFArrayGetValueAtIndex(availInputs, nextIdx);
            CFBooleanRef isSelectable = (CFBooleanRef)TISGetInputSourceProperty(inputSource, kTISPropertyInputSourceIsSelectCapable);
            if (isSelectable == kCFBooleanTrue)
            {
                CFStringRef category = (CFStringRef)TISGetInputSourceProperty(inputSource, kTISPropertyInputSourceCategory);
                if (CFStringCompare(category, kTISCategoryKeyboardInputSource, kCFCompareNumerically) == kCFCompareEqualTo)
                {
                    TISSelectInputSource(inputSource);
                    retVal = nextIdx;
                    break;
                }
            }
        }
    }
    
    CFRelease(availInputs);
    
    return retVal;
}

int32_t UseNextInputSource()
{
    return UseNewInputSource(1);
}

int32_t UsePreviousInputSource()
{
    return UseNewInputSource(-1);
}
    
    
}
}

#endif