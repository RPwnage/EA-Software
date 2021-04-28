
#ifndef IGNITIONDEFINES_H
#define IGNITIONDEFINES_H

#include "PyroSDK/pyrosdk.h"

#include "coreallocator/icoreallocator.h"
#include "DirtySDK/dirtysock/dirtymem.h"
#include "EAStdC/EASprintf.h"

namespace Ignition
{

class Allocator
    : public EA::Allocator::ICoreAllocator
{
    public:
        virtual void *Alloc(size_t size, const char8_t *name, unsigned int flags) { return malloc(size); }
        virtual void *Alloc(size_t size, const char8_t *name, unsigned int flags, unsigned int align, unsigned int alignoffset)  { return malloc(size); }
        virtual void *AllocDebug(size_t size, const DebugParams debugParams, unsigned int flags)  { return malloc(size); }
        virtual void *AllocDebug(size_t size, const DebugParams debugParams, unsigned int flags, unsigned int align, unsigned int alignOffset)  { return malloc(size); }
        virtual void Free(void *ptr, size_t) { free(ptr); }
};

extern Allocator gsAllocator;

}

#ifndef BLAZESDK_DLL  // If this is not built with Dirtysock as a DLL... provide our own default DirtyMemAlloc/Free
extern "C"
{
    void *DirtyMemAlloc(int32_t iSize, int32_t iMemModule, int32_t iMemGroup, void *pMemGroupUserData);
    void DirtyMemFree(void *pMem, int32_t iMemModule, int32_t iMemGroup, void *pMemGroupUserData);
}
#endif

#if defined(EA_PLATFORM_ANDROID) || defined(EA_PLATFORM_IPHONE) || defined(EA_PLATFORM_OSX) || defined(EA_PLATFORM_PS4) || defined(EA_PLATFORM_PS5) || defined(EA_PLATFORM_LINUX) || defined(EA_PLATFORM_REVOLUTION) || defined(EA_PLATFORM_NX)
    #define REPORT_LISTENER_CALLBACK(comment)
    #define REPORT_FUNCTOR_CALLBACK(comment)
    #define CMT(comment)
#else
    #define REPORT_LISTENER_CALLBACK(comment, ...) getUiDriver()->addDiagnostic_va(__FILE__, EA_CURRENT_FUNCTION, __LINE__, Pyro::ErrorLevel::NONE, "Listener", comment, __VA_ARGS__)
    #define REPORT_FUNCTOR_CALLBACK(comment, ...) getUiDriver()->addDiagnostic_va(__FILE__, EA_CURRENT_FUNCTION, __LINE__, Pyro::ErrorLevel::NONE, "Functor", comment, __VA_ARGS__)
    #define CMT(comment, ...) getUiDriver()->addDiagnostic_va(__FILE__, EA_CURRENT_FUNCTION, __LINE__, Pyro::ErrorLevel::NONE, "Comment", comment, __VA_ARGS__)
#endif

#define REPORT_CALL(context, func) addDiagnostic(getUiDriver(), __FILE__, EA_CURRENT_FUNCTION, __LINE__, Pyro::ErrorLevel::NONE, context, func)
#define rLOG(call) (REPORT_CALL("BlazeSDK Function", #call) ? call : call)
#define vLOG(call) REPORT_CALL("BlazeSDK Function", #call); call
#define vLOGIgnition(call) REPORT_CALL("Ignition Function", #call); call

inline bool addDiagnostic(Pyro::UiDriver *uiDriver, const char8_t *filename, const char8_t *functionName, int32_t lineNumber,
                          Pyro::ErrorLevel::Type errorLevel, const char8_t *description, const char8_t *details)
{
    if (uiDriver != nullptr)
        uiDriver->addDiagnostic(filename, functionName, lineNumber, errorLevel, description, details);
    return true;
}

#endif
