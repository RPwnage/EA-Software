///////////////////////////////////////////////////////////////////////////////
// ReportWriter.cpp
// 
// Copyright (c) 2005, Electronic Arts. All Rights Reserved.
// Written and maintained by Paul Pedriana.
//
// Exception handling and reporting facilities.
///////////////////////////////////////////////////////////////////////////////

#ifdef _MSC_VER
#pragma warning(disable:4263) // member function does not override any base class virtual member function
#pragma warning(disable:4264) // no override available for virtual member function from base 'IDWriteTextFormat'; function is hidden
#endif

#include <EACallstack/EAAddressRep.h>
#include <EACallstack/EACallstack.h>
#include <EACallstack/EADasm.h>
#include <EACallstack/Context.h>
#include <eathread/eathread.h>
#include <eathread/eathread_thread.h>
#include <eathread/eathread_callstack.h>
#include <ExceptionHandler/ReportWriter.h>
#include <ExceptionHandler/ExceptionHandler.h> // For the ExceptionHandler::IsDebuggerPresent function.
#include <EASTL/algorithm.h>
#include <EAStdC/EAString.h>
#include <EAStdC/EAScanf.h>
#include <EAStdC/EACType.h>
#include <EAStdC/EASprintf.h>
#include <EAStdC/EADateTime.h>
#include <EAStdC/EAProcess.h>
#include <EAStdC/EAEndian.h>
#include <EAStdC/EABitTricks.h>
#include <EAAssert/eaassert.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <ctype.h>

#ifdef _MSC_VER
    #pragma warning(push, 0)
#endif

#if defined(EA_PLATFORM_XENON)
    #include <xtl.h>
    #if EA_XBDM_ENABLED
        #include <xbdm.h>
        // #pragma comment(lib, "xbdm.lib") The application is expected to provide this if needed. 
    #endif

#elif defined(EA_PLATFORM_MICROSOFT)
    #pragma warning(push, 0)
    #if !defined(EA_PLATFORM_WINDOWS_PHONE) // Windows phone requires you to use WinRT-based socket API.
        #include <winsock2.h>   // You should always #include <winsock2.h> before <Windows.h>, otherwise Winsock2 will be broken.
        #include <ws2tcpip.h>
    #endif
    #include <Windows.h>
    #include <WinVer.h>
    #include <share.h>
    #if EA_WINAPI_FAMILY_PARTITION(EA_WINAPI_PARTITION_DESKTOP)
        #include <Psapi.h>
        #include <TlHelp32.h>
    #endif

    #pragma warning(pop)

    #if EA_WINAPI_FAMILY_PARTITION(EA_WINAPI_PARTITION_DESKTOP)
        #if (_WIN32_WINNT < 0x0500) // If windows.h is set not to define the items below, we define them ourselves.
            typedef enum _COMPUTER_NAME_FORMAT {
                ComputerNameNetBIOS,
                ComputerNameDnsHostname,
                ComputerNameDnsDomain,
                ComputerNameDnsFullyQualified,
                ComputerNamePhysicalNetBIOS,
                ComputerNamePhysicalDnsHostname,
                ComputerNamePhysicalDnsDomain,
                ComputerNamePhysicalDnsFullyQualified,
                ComputerNameMax
            } COMPUTER_NAME_FORMAT;

            extern "C" WINBASEAPI BOOL WINAPI GetComputerNameExA(IN COMPUTER_NAME_FORMAT NameType, OUT LPSTR lpBuffer, IN OUT LPDWORD nSize);
        #endif

        #pragma comment(lib, "version.lib") 
    #else
        // Temporary while waiting for formal support:
        extern "C" WINBASEAPI DWORD WINAPI GetModuleFileNameA(HMODULE, LPSTR, DWORD);
    #endif

    #if (_WIN32_WINNT < 0x0400) // If windows.h is set not to define the items below, we define them ourselves.
        extern "C" WINBASEAPI BOOL WINAPI IsDebuggerPresent(VOID);
    #endif

#elif defined(EA_PLATFORM_PS3)
    #include <sdk_version.h>
    #include <netex/libnetctl.h>
    #include <sysutil/sysutil_common.h>
    #include <sysutil/sysutil_sysparam.h>
    #include <cell/sysmodule.h>
    #include <sys/prx.h>

#elif defined(EA_PLATFORM_APPLE)
    #include <EACallstack/Apple/EACallstackApple.h>
    #include <unistd.h>
    #include <sys/types.h>
    #include <sys/sysctl.h>
    #include <mach/mach.h>
    #include <mach/task.h>
    #include <mach/vm_map.h>
    #include <mach/vm_region.h>
    #include <mach/mach_error.h>
    #include <mach/thread_status.h>
    
    #if defined(EA_PLATFORM_OSX) // Not available on iOS
        // libproc is supoosedly available only in iPhone SDK 2.0+ and OSX SDK 10.5+. There are alternatives to 
        // libproc which are more complicated to use but which might support older OS versions.
        #include <libproc.h>
        #include <sys/proc_info.h>
    #endif
    #if defined(EA_MACH_VM_AVAILABLE) && EA_MACH_VM_AVAILABLE // vm.h is available only when the Apple Kernel framework is enabled in the build. I'm still trying to figure out how to enable that.
        #include <mach/vm.h>
    #endif

#elif defined(EA_PLATFORM_UNIX)
    #include <unistd.h>
    #include <sys/types.h>
    #include <sys/utsname.h>
#endif

#ifdef _MSC_VER
    #pragma warning(pop)
#endif

#ifdef _MSC_VER
    #pragma warning(disable: 4509) // nonstandard extension used: 'x' uses SEH and 'y' has destructor
#endif



namespace EA {

namespace Debug {


#if EA_WINAPI_FAMILY_PARTITION(EA_WINAPI_PARTITION_DESKTOP)
    static bool Is64BitOperatingSystem()
    {
        #if (EA_PLATFORM_PTR_SIZE >= 8)
            return true;
        #elif defined(_WIN32) // We may have the case of a 32 bit app running on a 64 bit OS.
            #if EA_WINAPI_FAMILY_PARTITION(EA_WINAPI_PARTITION_DESKTOP)
                BOOL b64bitOS = FALSE;
                bool bDoesWos64ProcessExist = (GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "IsWow64Process") != NULL);
                return (bDoesWos64ProcessExist && IsWow64Process(GetCurrentProcess(), &b64bitOS) && b64bitOS);
            #else
                return false;
            #endif
        #elif defined(EA_PLATFORM_UNIX) // We may have the case of a 32 bit app running on a 64 bit OS.
            utsname utsName;
            memset(&utsName, 0, sizeof(utsName));

            return (uname(&utsName) == 0) && (strcmp(utsName.machine, "x86_64") == 0);
        #else
            return false;
        #endif
    }
#endif


#if defined(EA_PLATFORM_WINDOWS) && EA_WINAPI_FAMILY_PARTITION(EA_WINAPI_PARTITION_DESKTOP)

    struct IO_STATUS_BLOCK {
        union {
            NTSTATUS            Status;
            PVOID               Pointer;
        };
        ULONG_PTR               Information;
    };

    struct UNICODE_STRING {
        USHORT                  Length;
        USHORT                  MaximumLength;
        PWSTR                   Buffer;
    } ;

    struct SYSTEM_HANDLE {
        ULONG                   ProcessId;
        BYTE                    ObjectTypeNumber;
        BYTE                    Flags;
        USHORT                  Handle;
        PVOID                   Object;
        ACCESS_MASK             GrantedAccess;
    };

    struct SYSTEM_HANDLE_INFORMATION {
        ULONG                   HandleCount;
        SYSTEM_HANDLE           Handles[1];
    };

    #ifndef SystemHandleInformation
        #define SystemHandleInformation 16
    #endif
    #ifndef ObjectBasicInformation
        #define ObjectBasicInformation 0
    #endif
    #ifndef ObjectNameInformation
        #define ObjectNameInformation 1
    #endif
    #ifndef ObjectTypeInformation
        #define ObjectTypeInformation 2
    #endif
    #ifndef STATUS_INFO_LENGTH_MISMATCH
        #define STATUS_INFO_LENGTH_MISMATCH 0xc0000004
    #endif
    #ifndef STATUS_END_OF_FILE
        #define STATUS_END_OF_FILE 0xC0000011
    #endif
    #ifndef NT_SUCCESS
        #define NT_SUCCESS(x) ((x) >= 0)
    #endif

    //  kernel handle object type name, i.e. "File", "Thread" etc.
    struct PUBLIC_OBJECT_TYPE_INFORMATION {
        UNICODE_STRING          TypeName;
        ULONG Reserved          [22];    // reserved for internal use
    };

    typedef NTSTATUS (NTAPI *FN_NTQUERYSYSTEMINFORMATION)(ULONG SystemInformationClass, PVOID SystemInformation, ULONG SystemInformationLength, PULONG ReturnLength);
    typedef NTSTATUS (NTAPI *FN_NTQUERYOBJECT)(HANDLE ObjectHandle, ULONG ObjectInformationClass, PVOID ObjectInformation, ULONG ObjectInformationLength, PULONG ReturnLength);

    // http://msdn.microsoft.com/en-us/library/windows/desktop/aa366789%28v=vs.85%29.aspx

    bool GetFileNameFromHandle(HANDLE hFile, wchar_t* pFilePath, size_t filePathCapacity, uint64_t& fileSize) 
    {
        bool   bSuccess     = false;
        DWORD  dwFileSizeHi = 0;
        DWORD  dwFileSizeLo = GetFileSize(hFile, &dwFileSizeHi); 

        pFilePath[0] = 0;

        if((dwFileSizeLo != 0) || (dwFileSizeHi != 0))
        {
            fileSize = ((uint64_t)dwFileSizeHi << 32) | dwFileSizeLo;

            HANDLE hFileMap = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 1, NULL);

            if(hFileMap) 
            {
                void* pMem = MapViewOfFile(hFileMap, FILE_MAP_READ, 0, 0, 1);

                if(pMem) 
                {
                    if(GetMappedFileNameW(GetCurrentProcess(), pMem, pFilePath, (DWORD)filePathCapacity)) 
                    {
                        // Translate path with device name to drive letters.
                        wchar_t tempBuffer[512] = {};

                        if(GetLogicalDriveStringsW(EAArrayCount(tempBuffer) - 1, tempBuffer)) 
                        {
                            wchar_t  pNameTemp[MAX_PATH];
                            wchar_t  pDrive[3] = L" :";
                            bool     bFound = false;
                            wchar_t* p = tempBuffer;

                            do{
                                // Copy the drive letter to the template string
                                *pDrive = *p;

                                if(QueryDosDeviceW(pDrive, pNameTemp, MAX_PATH))
                                {
                                    size_t uNameLen = EA::StdC::Strlen(pNameTemp);

                                    if(uNameLen < MAX_PATH) 
                                    {
                                        bFound = (EA::StdC::Strnicmp(pFilePath, pNameTemp, uNameLen) == 0) && (pFilePath[uNameLen] == '\\');

                                        if(bFound) 
                                        {
                                            wchar_t pFilePathTemp[MAX_PATH]; // Need to make an temp copy.
                                            EA::StdC::Snprintf(pFilePathTemp, EAArrayCount(pFilePathTemp), L"%s%s", pDrive, pFilePath + uNameLen);
                                            EA::StdC::Strlcpy(pFilePath, pFilePathTemp, filePathCapacity);
                                        }
                                    }
                                }

                                // Go to the next NULL character.
                                while(*p++)
                                    {} 
                            } while(!bFound && *p);
                        }
                    }

                    bSuccess = true;
                    UnmapViewOfFile(pMem);
                } 

                CloseHandle(hFileMap);
            }
        }

        return bSuccess;
    }

#endif // defined(EA_PLATFORM_WINDOWS) && EA_WINAPI_FAMILY_PARTITION(EA_WINAPI_PARTITION_DESKTOP)



#if defined(EA_PLATFORM_MICROSOFT)

EA_DISABLE_VC_WARNING(4505) // unreferenced local function has been removed

///////////////////////////////////////////////////////////////////////////////
// GetCurrentMachineAddress
//
static bool GetCurrentMachineAddressString(char* buffer, size_t capacity)
{
    bool returnValue = false;

    buffer[0] = 0;

    #if defined(EA_PLATFORM_MICROSOFT) && !defined(EA_PLATFORM_XENON) && !defined(EA_PLATFORM_WINDOWS_PHONE)
        // The code here works only on Winsock2, which should be present on all modern Windows machines, though not Windows phone.
        const SOCKET s = WSASocket(AF_INET, SOCK_DGRAM, 0, NULL, 0, 0);

        if(s != SOCKET_ERROR)
        {
            union AddressList
            {
                char                buffer[1024];
                SOCKET_ADDRESS_LIST socketAddressList;
            };

            bool           ip4AddrFound = false;
            bool           ip6AddrFound = false;
            char           addrString[64];
            AddressList    addressList;
            unsigned long  nBytesReturned;
            const int      result = WSAIoctl(s, SIO_ADDRESS_LIST_QUERY, 0, 0, &addressList, sizeof(addressList), &nBytesReturned, 0, 0);

            if(result != SOCKET_ERROR)
            {
                const int addressCount = addressList.socketAddressList.iAddressCount;

                for(int i = 0; (i < addressCount) && (!ip4AddrFound || !ip6AddrFound); i++)
                {
                    const u_short nAddressFamily      = addressList.socketAddressList.Address[i].lpSockaddr->sa_family;
                  //const bool    bAddressFamilyMatch = ((nAddressFamily == AF_INET)  && !ip4AddrFound) || 
                  //                                    ((nAddressFamily == AF_INET6) && !ip6AddrFound);

                    if(nAddressFamily == AF_INET)
                    {
                        const sockaddr_in* const pAddress = (const sockaddr_in*)addressList.socketAddressList.Address[i].lpSockaddr;

                        if((pAddress->sin_addr.S_un.S_addr != INADDR_ANY) && 
                           (pAddress->sin_addr.S_un.S_addr != INADDR_LOOPBACK))
                        {
                            const uint32_t nAddress = (uint32_t)pAddress->sin_addr.S_un.S_addr;

                            EA::StdC::Sprintf(addrString, "%u.%u.%u.%u", nAddress >> 24, (nAddress >> 16) & 0xff, (nAddress >> 8) & 0xff, nAddress & 0xff);
                            EA::StdC::Strlcat(buffer, addrString, capacity);

                            ip4AddrFound = true;
                        }
                    }
                    else if(nAddressFamily == AF_INET6)
                    {
                        #ifndef IN6ADDR_ANY_INIT
                            #define IN6ADDR_ANY_INIT      { 0 }
                            #define IN6ADDR_LOOPBACK_INIT { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 }
                        #endif

                        const  sockaddr_in6* const pAddress     = (const sockaddr_in6*)addressList.socketAddressList.Address[i].lpSockaddr;
                        const  in6_addr*     const pAddr        = &pAddress->sin6_addr;
                        struct in6_addr            anyAddr      = IN6ADDR_ANY_INIT;
                        struct in6_addr            lookbackAddr = IN6ADDR_LOOPBACK_INIT;

                        if((memcmp(pAddr, &anyAddr,      sizeof(anyAddr)      != 0)) &&
                           (memcmp(pAddr, &lookbackAddr, sizeof(lookbackAddr) != 0)))
                        {
                            #if EA_WINAPI_FAMILY_PARTITION(EA_WINAPI_PARTITION_DESKTOP) && (defined(NTDDI_VERSION) && defined(NTDDI_VISTA) && (NTDDI_VERSION >= NTDDI_VISTA))
                                InetNtopA(AF_INET6, (PVOID)pAddr, addrString, EAArrayCount(addrString)); // InetNtop can produce a shortened string, which is arguably better.
                            #else
                                EA::StdC::Sprintf(addrString, "%04x:%04x:%04x:%04x:%04x:%04x:%04x:%04x", 
                                                 pAddr->u.Word[0], pAddr->u.Word[0], pAddr->u.Word[0], pAddr->u.Word[0], pAddr->u.Word[0], pAddr->u.Word[0], pAddr->u.Word[0], pAddr->u.Word[0]);
                            #endif

                            EA::StdC::Strlcat(buffer, addrString, capacity);

                            ip6AddrFound = true;
                        }
                    }
                }
            }

            returnValue = (ip4AddrFound || ip6AddrFound);
            closesocket(s);
        }
    #else
        EA_UNUSED(capacity);
    #endif

    return returnValue;
}

#endif // #if defined(EA_PLATFORM_MICROSOFT)



#if defined(EA_PLATFORM_PS3)
    // We set this to unilaterally run on startup, as we are often called during
    // an exception and we don't want to be doing such module loads like this when
    // inside an exception. It turns out that all applications of significance 
    // will likely need to have these libraries loaded anyway.
    void ReportWriterSystemInit() __attribute__ ((constructor));

    void ReportWriterSystemInit()
    {
        // We don't appear to need this right now.
        // cellSysmoduleLoadModule(CELL_SYSMODULE_NET);
        // sys_net_initialize_network();

        cellSysmoduleLoadModule(CELL_SYSMODULE_NETCTL);
        cellNetCtlInit(); // As it stands now, we allow this to fail, though in practice it doesn't fail.

        cellSysmoduleLoadModule(CELL_SYSMODULE_SYSUTIL);
    }
#else
    void ReportWriterSystemInit();
    void ReportWriterSystemInit() { }
#endif


///////////////////////////////////////////////////////////////////////////////
// ReportWriter
//
ReportWriter::ReportWriter()
  :   mpCoreAllocator(EA::Callstack::GetAllocator()),
    //mOption, 
    //mPath,
      mpFile(NULL),
    //mWriteBuffer,
    //mScratchBuffer,
      mnSectionDepth(0),
      mExceptionThreadId(EA::Thread::kThreadIdInvalid),
      mExceptionSysThreadId(EA::Thread::kSysThreadIdInvalid),
    //mExceptionCallstack,
      mExceptionCallstackCount(0)
{
    memset(mOption, 0, sizeof(mOption));
    memset(mExceptionCallstack, 0, sizeof(mExceptionCallstack));

    #if defined(EA_DEBUG)
        // Enabled by default only in debug because otherwise it could represent a leak of 
        // personal information. You can explictly enable these options with SetOption.
        mOption[kOptionReportUserName]     = 1;
        mOption[kOptionReportComputerName] = 1;
        mOption[kOptionReportProcessList]  = 1;
    #endif
    
    mPath[0] = 0;
}


///////////////////////////////////////////////////////////////////////////////
// ~ReportWriter
//
ReportWriter::~ReportWriter()
{
    ReportWriter::CloseReport();
}


///////////////////////////////////////////////////////////////////////////////
// SetExceptionThreadIds
//
void ReportWriter::SetExceptionThreadIds(const Thread::ThreadId& threadId, const Thread::SysThreadId& exceptionSysThreadId)
{
    mExceptionThreadId    = threadId;
    mExceptionSysThreadId = exceptionSysThreadId;
}


///////////////////////////////////////////////////////////////////////////////
// SetExceptionCallstack
//
void ReportWriter::SetExceptionCallstack(const void* addressArray, size_t count)
{
    if(count > EAArrayCount(mExceptionCallstack))
        count = EAArrayCount(mExceptionCallstack);

    mExceptionCallstackCount = count;
    memcpy(mExceptionCallstack, addressArray, count * sizeof(void*));
}


///////////////////////////////////////////////////////////////////////////////
// GetAllocator
//
EA::Allocator::ICoreAllocator* ReportWriter::GetAllocator() const
{
    return mpCoreAllocator;
}


///////////////////////////////////////////////////////////////////////////////
// SetAllocator
//
void ReportWriter::SetAllocator(EA::Allocator::ICoreAllocator* pCoreAllocator)
{
    mpCoreAllocator = pCoreAllocator;
}


///////////////////////////////////////////////////////////////////////////////
// GetFileNameExtension
//
int ReportWriter::GetOption(Option option) const
{
    if((option >= 0) && (option < kOptionCount))
        return mOption[option];
    return 0;
}


///////////////////////////////////////////////////////////////////////////////
// SetOption
//
void ReportWriter::SetOption(Option option, int value)
{
    if((option >= 0) && (option < kOptionCount))
        mOption[option] = value;
}


///////////////////////////////////////////////////////////////////////////////
// GetFileNameExtension
//
void ReportWriter::GetFileNameExtension(char* pExtension, size_t capacity)
{
    EA::StdC::Strlcpy(pExtension, ".txt", capacity);
}


///////////////////////////////////////////////////////////////////////////////
// OpenReport
//
bool ReportWriter::OpenReport(const char* pReportPath)
{
    if(!mpFile) // If not already open...
    {
        if(pReportPath)
            EA::StdC::Strlcpy(mPath, pReportPath, EAArrayCount(mPath));

        #if defined(EA_PLATFORM_MICROSOFT) && defined(_SH_DENYWR)     // If we can use the version that permits us to use file sharing...
            mpFile = _fsopen(mPath, "wb", _SH_DENYWR);                // This makes the code easier to debug, among other things.
        #else
            mpFile = fopen(mPath, "wb");
        #endif

        // It's important that we don't write anything in this function, as we 
        // may be subclassed and the subclass wants to see all writes after it 
        // sees what the file handle is.

        return (mpFile != NULL);
    }

    return false;
}


///////////////////////////////////////////////////////////////////////////////
// CloseReport
//
bool ReportWriter::CloseReport()
{
    if(mpFile)
    {
        fclose(mpFile);
        mpFile = NULL;
    }

    return (mpFile != NULL);
}


///////////////////////////////////////////////////////////////////////////////
// BeginReport
//
bool ReportWriter::BeginReport(const char* /*pReportName*/)
{
    // In this class we do nothing. A subclass might want to do something.
    return true;
}


///////////////////////////////////////////////////////////////////////////////
// EndReport
//
bool ReportWriter::EndReport()
{
    // In this class we do nothing. A subclass might want to do something.
    return true;
}


///////////////////////////////////////////////////////////////////////////////
// BeginSection
//
bool ReportWriter::BeginSection(const char* pSectionName)
{
    if(++mnSectionDepth == 1)
        return WriteFormatted("[%s]\r\n", pSectionName);

    return false;
}


///////////////////////////////////////////////////////////////////////////////
// EndSection
//
bool ReportWriter::EndSection(const char* /*pSectionName*/)
{
    --mnSectionDepth;

    return WriteBlankLine();
}


///////////////////////////////////////////////////////////////////////////////
// WriteText
//
bool ReportWriter::WriteText(const char* pText, bool bAppendNewline)
{
    if(Write(pText))
    {
        if(bAppendNewline)
            return Write("\r\n", 2);

        return true;
    }

    return false;
}


///////////////////////////////////////////////////////////////////////////////
// WriteFormatted
//
bool ReportWriter::WriteFormatted(const char* pFormat, ...)
{
    bool    bReturnValue;
    va_list arguments;

    // We don't have support for dealing with the case whereby mWriteBuffer
    // isn't big enough. We can fix this if it becomes an issue.
    va_start(arguments, pFormat);
    const int result = EA::StdC::Vsnprintf(mWriteBuffer, EAArrayCount(mWriteBuffer), pFormat, arguments);

    if((result > 0) && (result < (int)EAArrayCount(mWriteBuffer)))
        bReturnValue = Write(mWriteBuffer, (size_t)result);
    else
    {
        Write("<buffer too small>");
        bReturnValue = false;
    }

    va_end(arguments);

    return bReturnValue;
}


///////////////////////////////////////////////////////////////////////////////
// WriteKeyValue
//
bool ReportWriter::WriteKeyValue(const char* pKey, const char* pValue)
{
    // To do: allow user to control amount of spaces between the key and 
    // value for alignment of multiple vertical values.

    if(Write(pKey, EA::StdC::Strlen(pKey)))
    {
        if(Write(": ", 2))
        {
            if(Write(pValue, EA::StdC::Strlen(pValue)))
                return Write("\r\n", 2);
        }
    }

    return false;
}


///////////////////////////////////////////////////////////////////////////////
// WriteKeyValueFormatted
//
bool ReportWriter::WriteKeyValueFormatted(const char* pKey, const char* pFormat, ...)
{
    bool    bReturnValue;
    va_list arguments;

    // We don't have support for dealing with the case whereby mWriteBuffer
    // isn't big enough. We can fix this if it becomes an issue.
    va_start(arguments, pFormat);
    const int result = EA::StdC::Vsnprintf(mWriteBuffer, EAArrayCount(mWriteBuffer), pFormat, arguments);

    if((result > 0) && (result < (int)EAArrayCount(mWriteBuffer)))
        bReturnValue = WriteKeyValue(pKey, mWriteBuffer);
    else
    {
        Write("<buffer too small>");
        bReturnValue = false;
    }

    va_end(arguments);

    return bReturnValue;
}


///////////////////////////////////////////////////////////////////////////////
// WriteBlankLine
//
bool ReportWriter::WriteBlankLine()
{
    return Write("\r\n", 2);
}


///////////////////////////////////////////////////////////////////////////////
// WriteFlush
//
bool ReportWriter::WriteFlush()
{
    if(mpFile)
    {
        fflush(mpFile);
        return true;
    }
    
    return false;
}


///////////////////////////////////////////////////////////////////////////////
// Write
//
bool ReportWriter::Write(const char* pData, size_t count)
{
    if(mpFile)
    {
        if(count > 1000000)
            count = EA::StdC::Strlen(pData);

        const size_t result = fwrite(pData, 1, count, mpFile);

        // We flush the stream because the FILE stream is buffered and this report is
        // intended to be used for the writing of exception reports. So the app could
        // crash at any time while writing stack and memory data for the app, due to 
        // the app being in a possibly unstable state.
        fflush(mpFile);

        return (result == count);
    }

    return false;
}


///////////////////////////////////////////////////////////////////////////////
// GetPosition
//
size_t ReportWriter::GetPosition()
{
    if(mpFile)
        return (size_t)ftell(mpFile);
    return 0;
}


///////////////////////////////////////////////////////////////////////////////
// SetPosition
//
void ReportWriter::SetPosition(size_t pos)
{
    if(mpFile)
        fseek(mpFile, (long)(unsigned long)pos, SEEK_SET);
}


bool ReportWriter::ReadReportBegin()
{
    if(!mpFile)
    {
        mpFile = fopen(mPath, "rb");
        return (mpFile != NULL);
    }

    return false;
}


size_t ReportWriter::ReadReportPart(char* buffer, size_t size)
{
    if(mpFile)
    {
        size_t result = fread(buffer, 1, size, mpFile);

        if((result == size) && !ferror(mpFile))
            return result;
    }

    return (size_t)(ssize_t)-1;
}


void ReportWriter::ReadReportEnd()
{
    if(mpFile)
    {
        fclose(mpFile);
        mpFile = NULL;
    }
}


///////////////////////////////////////////////////////////////////////////////
// GetTimeString
//
size_t ReportWriter::GetTimeString(char* pTimeString, size_t capacity, const tm* pTime, bool /*bLongFormat*/) const
{
    if(!pTime)
    {
        const time_t timeValue = time(NULL);
        pTime = localtime(&timeValue);
    }

    //if(bLongFormat)
    //{
    //    // It's not defined whether pTime is local or universal time. If it's local then we might want to also print universal time here, and vice versa.
    //    return EA::StdC::Strftime(pTimeString, capacity, "Local: %H.%M.%S GMT: %H.%M.S", pTime, pGMTime);
    //}
    //else
        return EA::StdC::Strftime(pTimeString, capacity, "%H.%M.%S", pTime);
}


///////////////////////////////////////////////////////////////////////////////
// GetDateString
//
size_t ReportWriter::GetDateString(char* pDateString, size_t capacity, const tm* pTime, bool bFourDigitYear) const
{
    if(!pTime)
    {
        const time_t timeValue = time(NULL);
        pTime = localtime(&timeValue);
    }

    if(bFourDigitYear)
        return EA::StdC::Strftime(pDateString, capacity, "%Y-%m-%d", pTime);

    return EA::StdC::Strftime(pDateString, capacity, "%y-%m-%d", pTime);
}


///////////////////////////////////////////////////////////////////////////////
// GetMachineName
//
size_t ReportWriter::GetMachineName(char* pNameString, size_t capacity)
{
    #define kMachineNameUnknown "(unknown machine name)"
    #define kMachineNameUnset   "(unset machine name)"

    pNameString[0] = 0;

    #if defined(EA_PLATFORM_XENON)
        #if EA_XBDM_ENABLED
            DWORD dwCapacity = (DWORD)capacity;

            if(DmGetXboxName(pNameString, &dwCapacity) != XBDM_NOERR)
                EA::StdC::Strlcpy(pNameString, kMachineNameUnknown, capacity);
        #endif

    #elif defined(EA_PLATFORM_MICROSOFT) && EA_WINAPI_FAMILY_PARTITION(EA_WINAPI_PARTITION_DESKTOP)
        DWORD dwCapacity = (DWORD)capacity;

        if(GetComputerNameA(pNameString, &dwCapacity) == 0)
            EA::StdC::Strlcpy(pNameString, kMachineNameUnknown, capacity);

    #elif defined(EA_PLATFORM_MICROSOFT)
        if(!GetCurrentMachineAddressString(pNameString, capacity))
            EA::StdC::Strlcpy(pNameString, kMachineNameUnknown, capacity);

    #elif defined(EA_PLATFORM_PS3)
        CellNetCtlInfo info;

        int result = cellNetCtlGetInfo(CELL_NET_CTL_INFO_DHCP_HOSTNAME, &info);

        if((result == 0) && info.dhcp_hostname[0])
            EA::StdC::Strlcpy(pNameString, info.dhcp_hostname, capacity);
        else
        {
            result = cellNetCtlGetInfo(CELL_NET_CTL_INFO_IP_ADDRESS, &info);

            if((result == 0) && info.ip_address[0])
                EA::StdC::Strlcpy(pNameString, info.ip_address, capacity);
            else
                EA::StdC::Strlcpy(pNameString, kMachineNameUnknown, capacity);
        }

    #elif defined(EA_PLATFORM_REVOLUTION)
        EA::StdC::Strlcpy(pNameString, kMachineNameUnknown, capacity);

    #elif defined(EA_PLATFORM_OSX)
        // http://stackoverflow.com/questions/4063129/get-computer-name-on-mac
        // http://stackoverflow.com/questions/1506912/get-short-user-name-from-full-name
        
        // To do: 
        gethostname(pNameString, (int)capacity);
        pNameString[capacity - 1] = 0;

    #elif defined(EA_PLATFORM_UNIX)
        gethostname(pNameString, (int)capacity);
        pNameString[capacity - 1] = 0;

    #else
        // We don't do anything. Perhaps we should.
        EA::StdC::Strlcpy(pNameString, kMachineNameUnknown, capacity);
    #endif

    #undef kMachineNameUnknown
    #undef kMachineNameUnset

    return EA::StdC::Strlen(pNameString);
}


///////////////////////////////////////////////////////////////////////////////
// GetAddressLocation
// 
size_t ReportWriter::GetAddressLocation(const void* pAddress, char* buffer, size_t capacity, uint32_t& section, uint32_t& offset)
{
    buffer[0] = 0;
    section   = 0;
    offset    = 0;

    #if defined(EA_PLATFORM_MICROSOFT) && !defined(EA_PLATFORM_XENON)
        MEMORY_BASIC_INFORMATION mbi;

        if(VirtualQuery(pAddress, &mbi, sizeof(mbi)))
        {
            HMODULE hModule = (HMODULE)mbi.AllocationBase;

            char moduleName[512];
            moduleName[0] = moduleName[511] = 0;

            if(hModule && GetModuleFileNameA(hModule, moduleName, 511))
            {
                // Charset encoding may (in theory) be wrong here.
                PIMAGE_DOS_HEADER     pDosHdr  = (PIMAGE_DOS_HEADER)hModule;  // Point to the DOS header in memory.
                PIMAGE_NT_HEADERS     pNtHdr   = (PIMAGE_NT_HEADERS)((uintptr_t)hModule + pDosHdr->e_lfanew);  // From the DOS header, find the NT (PE) header.
                PIMAGE_SECTION_HEADER pSection = IMAGE_FIRST_SECTION(pNtHdr);
                const uintptr_t       rva      = (uintptr_t)pAddress - (uintptr_t)hModule; // RVA is offset from module load address

                // Iterate through the section table, looking for the one that encompasses the linear address.
                for(WORD i = 0; i < pNtHdr->FileHeader.NumberOfSections; i++, pSection++)
                {
                    uintptr_t sectionStart = pSection->VirtualAddress;
                    uintptr_t sectionEnd   = sectionStart + pSection->Misc.VirtualSize;

                    if((rva >= sectionStart) && (rva < sectionEnd)) //If the address in this section...
                    {
                        EA::StdC::Strlcpy(buffer, moduleName, capacity);

                        section = (uint32_t)(i + 1);
                        offset  = (uint32_t)(rva - sectionStart);

                        return EA::StdC::Strlen(buffer);
                    }
                }
            }
        }

        EA::StdC::Strlcpy(buffer, "<unknown>", capacity);

        return 0; // Return 0 and not the strlen, as we are telling the caller that we failed.

    #elif defined(EA_PLATFORM_XENON)
        #if EA_XBDM_ENABLED
            DMN_MODLOAD      ModLoad;
            PDM_WALK_MODULES pWalkMod = NULL;

            while(DmWalkLoadedModules(&pWalkMod, &ModLoad) == XBDM_NOERR)
            {
                if((pAddress >= ModLoad.BaseAddress) && (pAddress < ((BYTE*)ModLoad.BaseAddress + ModLoad.Size)))
                {
                    EA::StdC::Strlcpy(buffer, ModLoad.Name, capacity);
                    break;
                }
            }

            DmCloseLoadedModules(pWalkMod);

            if(buffer[0]) // found module?
            {
                DMN_SECTIONLOAD  SecLoad;
                PDM_WALK_MODSECT pWalkSec = NULL;

                section = 0;
                offset  = 0;

                while(DmWalkModuleSections(&pWalkSec, buffer, &SecLoad) == XBDM_NOERR)
                {
                    section++;

                    if((pAddress >= SecLoad.BaseAddress) && (pAddress < (BYTE*)SecLoad.BaseAddress + (SecLoad.Size)))
                    {
                        offset = (DWORD)pAddress - (DWORD)SecLoad.BaseAddress;
                        break;
                    }
                }

                DmCloseModuleSections(pWalkSec);
            }

            return strlen(buffer);
        #else
            EA::StdC::Strlcpy(buffer, "<unknown> -- Xbdm not enabled.", capacity);

            return 0; // Return 0 and not the strlen, as we are telling the caller that we failed.
        #endif

    #else
        section = 0; // These are useful for Microsoft only.
        offset  = 0;
        
        const size_t requiredStrlen = EA::Callstack::GetModuleFromAddress(pAddress, buffer, capacity);
    
        if(requiredStrlen > 0)
            return EA::StdC::Strlen(buffer);
    
        EA::StdC::Strlcpy(buffer, "<unknown>", capacity);
        return 0; // Return 0 and not the strlen, as we are telling the caller that we failed.
    #endif
}


///////////////////////////////////////////////////////////////////////////////
// GetAddressLocationString
// 
size_t ReportWriter::GetAddressLocationString(const void* pAddress, char* buffer, size_t capacity)
{
    char     moduleName[384];
    uint32_t section = 0;
    uint32_t offset = 0;

    if(GetAddressLocation(pAddress, moduleName, sizeof(moduleName), section, offset))
    {
        #if (EA_PLATFORM_PTR_SIZE == 4)
            #if defined(EA_PLATFORM_MICROSOFT) // If reporting module segment/offset information...
                EA::StdC::Snprintf(buffer, capacity, "0x%08x \"%s\":0x%04x:0x%08x", (unsigned)(uintptr_t)pAddress, moduleName, (unsigned)section, (unsigned)offset);
            #else
                EA::StdC::Snprintf(buffer, capacity, "0x%08x \"%s\"", (unsigned)(uintptr_t)pAddress, moduleName);
            #endif
        #else
            #if defined(EA_PLATFORM_MICROSOFT)
                EA::StdC::Snprintf(buffer, capacity, "0x%016I64x \"%s\":0x%04x:0x%08x", (uintptr_t)pAddress, moduleName, (unsigned)section, (unsigned)offset);
            #else
                EA::StdC::Snprintf(buffer, capacity, "0x%016I64x \"%s\"", (uintptr_t)pAddress, moduleName);
            #endif
        #endif
    }
    else
    {
        #if (EA_PLATFORM_PTR_SIZE == 4)
            EA::StdC::Snprintf(buffer, capacity, "0x%08x <unknown module>", (unsigned)(uintptr_t)pAddress);
        #else
            EA::StdC::Snprintf(buffer, capacity, "0x%016I64x <unknown module>", (uintptr_t)pAddress);
        #endif
    }

    return true;
}


///////////////////////////////////////////////////////////////////////////////
// WriteSystemInfo
// 
// We write something like this (Win32):
//   Computer name: ppedriana-0024
//   Computer DNS name: ppedriana-0024.rws.ad.ea.com
//   User name: PPedrian
//   EA_PLATFORM: Windows on X86
//   OS name: Windows XP
//   OS version_number: 5.1.2600
//   OS service_pack: Service Pack 2
//   Debugger present: yes
//   CPU count: 2
//   Processor type: x86
//   Processor level: 15
//   Processor revision: 1027
//   Memory load: 30%
//   Total physical memory: 3326 Mb
//   Available physical memory: 2307 Mb
//   Total page file_memory: 5209 Mb
//   Available page file_memory: 4084 Mb
//   Total virtual memory: 2047 Mb
//   Free virtual memory: 2000 Mb
//
// We write something like this (Xenon):
//   Computer name: ppedrianaXenon
//   System version: 1234
//   XTL version: 2345
//   Machine type: Text kit
//   Debugger present: yes
//
// We write something like this (PS3):
//   [System info]
//   Computer name: ppedrianaPS3
//   System language: 1
//   Button assignment: 1
//   Date format: 2
//   Time format: 0
//   Parental level: 0
//
bool ReportWriter::WriteSystemInfo()
{
    // Should we implement a basic system version information gathering 
    // mechanism here, or should we use EAPlatform/SysVersion functionality
    // (which is largely unimplemented as of this writing in June 2006)?

    #if defined(EA_PLATFORM_XENON)

        if(mOption[kOptionReportComputerName])
        {
            GetMachineName(mScratchBuffer, sizeof(mScratchBuffer));
            WriteKeyValue("Computer name", mScratchBuffer);
        }

        #if EA_XBDM_ENABLED
            // System version info.
            DM_SYSTEM_INFO dmSystemInfo;
            memset(&dmSystemInfo, 0, sizeof(dmSystemInfo));
            dmSystemInfo.SizeOfStruct = sizeof(dmSystemInfo);

            DmGetSystemInfo(&dmSystemInfo);

            EA::StdC::Sprintf(mScratchBuffer, "%u.%u.%u.%u", dmSystemInfo.BaseKernelVersion.Major, dmSystemInfo.BaseKernelVersion.Minor, dmSystemInfo.BaseKernelVersion.Build, dmSystemInfo.BaseKernelVersion.Qfe);
            WriteKeyValue("Base kernel version: %s", mScratchBuffer);

            EA::StdC::Sprintf(mScratchBuffer, "%u.%u.%u.%u", dmSystemInfo.KernelVersion.Major, dmSystemInfo.KernelVersion.Minor, dmSystemInfo.KernelVersion.Build, dmSystemInfo.KernelVersion.Qfe);
            WriteKeyValue("Kernel version: %s", mScratchBuffer);

            EA::StdC::Sprintf(mScratchBuffer, "%u.%u.%u.%u", dmSystemInfo.XDKVersion.Major, dmSystemInfo.XDKVersion.Minor, dmSystemInfo.XDKVersion.Build, dmSystemInfo.XDKVersion.Qfe);
            WriteKeyValue("XDK version: %s", mScratchBuffer);

            WriteKeyValue("Hard drive present: %s", (dmSystemInfo.dmSystemInfoFlags & DM_XBOX_HW_FLAG_HDD) ? "yes" : "no");
            WriteKeyValue("Machine is test kit: %s", (dmSystemInfo.dmSystemInfoFlags & DM_XBOX_HW_FLAG_TESTKIT) ? "yes" : "no");

            #if defined(EA_DEBUG) // XDebugGetSystemVersion isn't an xbdm call, but is a call available only in the debug version of xapilib.
                XDebugGetSystemVersion(mScratchBuffer, sizeof(mScratchBuffer));
                if(mScratchBuffer[0])
                    WriteKeyValue("System version", mScratchBuffer);

                XDebugGetXTLVersion(mScratchBuffer, sizeof(mScratchBuffer));
                if(mScratchBuffer[0])
                    WriteKeyValue("XTL version", mScratchBuffer);
            #endif

            DWORD dwResult = 0xffffffff;
            DmGetConsoleType(&dwResult);

            if(dwResult == DMCT_DEVELOPMENT_KIT)
                WriteKeyValue("Machine type", "Development kit");
            else if(dwResult == DMCT_TEST_KIT)
                WriteKeyValue("Machine type", "Text kit");
            else if(dwResult == DMCT_REVIEWER_KIT)
                WriteKeyValue("Machine type", "Reviewer kit");
            else
                WriteKeyValue("Machine type", "Retail kit");

            WriteKeyValue("Debugger present", ExceptionHandler::IsDebuggerPresent() ? "yes" : "no");

            // Debug memory.
            DWORD dwMemory = 0;

            DmGetDebugMemorySize(&dwMemory); // Returns 0 on anything but the XDK-GB machine.
            WriteKeyValueFormatted("Debug memory", "%u", (uint32_t)dwMemory);

            DmGetAdditionalTitleMemorySetting(&dwMemory); // Returns 0 on anything but the XDK-GB machine.
            WriteKeyValueFormatted("Additional title memory", "%u", (uint32_t)dwMemory);

        #endif // EA_XBDM_ENABLED

        // System level view of memory.
        MEMORYSTATUS stat;
        memset(&stat, 0, sizeof(stat));
        GlobalMemoryStatus(&stat);

        WriteKeyValueFormatted("Virtual memory free",       "%I64u", (uint64_t)stat.dwTotalVirtual);
        WriteKeyValueFormatted("Virtual memory available",  "%I64u", (uint64_t)stat.dwAvailVirtual);
        WriteKeyValueFormatted("Physical memory free",      "%I64u", (uint64_t)stat.dwTotalPhys);
        WriteKeyValueFormatted("Physical memory available", "%I64u", (uint64_t)stat.dwAvailPhys);

    #elif defined(EA_PLATFORM_MICROSOFT)
        DWORD dwCapacity;

        #if EA_WINAPI_FAMILY_PARTITION(EA_WINAPI_PARTITION_DESKTOP)
            // Write the computer name.
            if(mOption[kOptionReportComputerName])
            {
                char machineName[32];
                GetMachineName(machineName, sizeof(machineName));
                WriteKeyValue("Computer name", machineName);
            }

            // Write the computer DNS name.
            if(mOption[kOptionReportComputerName])
            {
                dwCapacity = (DWORD)sizeof(mScratchBuffer);
                if(GetComputerNameExA(ComputerNameDnsFullyQualified, mScratchBuffer, &dwCapacity) > 0)
                    WriteKeyValue("Computer DNS name", mScratchBuffer);
            }

            // Write the user name.
            if(mOption[kOptionReportUserName])
            {
                dwCapacity = (DWORD)sizeof(mScratchBuffer);
                if(GetUserNameA(mScratchBuffer, &dwCapacity) > 0)           
                    WriteKeyValue("User name", mScratchBuffer);
            }
        #else
            // To do.
        #endif

        // System version info
        WriteKeyValue("EA_PLATFORM", EA_PLATFORM_DESCRIPTION); // EA_PLATFORM_DESCRIPTION comes from EABase.

        #if EA_WINAPI_FAMILY_PARTITION(EA_WINAPI_PARTITION_DESKTOP)

            OSVERSIONINFOEXA osVersionInfoEx;
            memset(&osVersionInfoEx, 0, sizeof(osVersionInfoEx));
            osVersionInfoEx.dwOSVersionInfoSize = sizeof(osVersionInfoEx);

            if(GetVersionExA((LPOSVERSIONINFOA)&osVersionInfoEx))
            {
                strcpy(mScratchBuffer, "Unknown, probably a successor to Windows 7 or Windows server 2008.");

                // Look up OSVERSIONINFOEX at the Microsoft MSDN site to see how the version numbers relate to OS releases.
                if(osVersionInfoEx.dwMajorVersion >= 7)
                {
                    // To do: Handle this case if and when Microsoft creates it.
                }
                if(osVersionInfoEx.dwMajorVersion >= 6)
                {
                    if(osVersionInfoEx.dwMajorVersion >= 2)
                    {
                        // To do: Handle this case if and when Microsoft creates it.
                    }
                    if(osVersionInfoEx.dwMajorVersion >= 1)
                    {
                        if(osVersionInfoEx.wProductType == VER_NT_WORKSTATION)
                            strcpy(mScratchBuffer, "Windows 7");
                        else
                            strcpy(mScratchBuffer, "Windows Server 2008 R2");
                    }
                    else
                    {
                        if(osVersionInfoEx.wProductType == VER_NT_WORKSTATION)
                            strcpy(mScratchBuffer, "Windows Vista");
                        else
                            strcpy(mScratchBuffer, "Windows Server 2008");
                    }
                }
                else if(osVersionInfoEx.dwMajorVersion >= 5)
                {
                    if(osVersionInfoEx.dwMinorVersion == 0)
                        strcpy(mScratchBuffer, "Windows 2000");
                    else if(osVersionInfoEx.dwMinorVersion == 1)
                        strcpy(mScratchBuffer, "Windows XP");
                    else // osVersionInfoEx.dwMinorVersion == 2
                    {
                        if(GetSystemMetrics(SM_SERVERR2) != 0)
                            strcpy(mScratchBuffer, "Windows Server 2003 R2");
                        else if(osVersionInfoEx.wSuiteMask & VER_SUITE_WH_SERVER)
                            strcpy(mScratchBuffer, "Windows Home Server");
                        if(GetSystemMetrics(SM_SERVERR2) == 0)
                            strcpy(mScratchBuffer, "Windows Server 2003");
                        else
                            strcpy(mScratchBuffer, "Windows XP Professional x64 Edition");
                    }
                }
                else
                    strcpy(mScratchBuffer, "Windows 98 or earlier");

                // Windows Vista+ supports GetProductInfo.
                typedef BOOL (WINAPI *PGetProductInfo)(DWORD, DWORD, DWORD, DWORD, PDWORD);
                PGetProductInfo pGPI = (PGetProductInfo)(uintptr_t)GetProcAddress(GetModuleHandleA("kernel32.dll"), "GetProductInfo");

                if(pGPI)
                {
                    DWORD dwProductType = 0;
                    pGPI(osVersionInfoEx.dwMajorVersion, osVersionInfoEx.dwMinorVersion, 0, 0, &dwProductType);

                    // See http://msdn.microsoft.com/en-us/library/ms724358%28v=vs.85%29.aspx for documentation and new identifiers.
                    // To consider: Make this a string table instead of a large switch statement.
                    switch(dwProductType)
                    {
                        case 0xABCDABCD: // PRODUCT_UNLICENSED:     // Not all Windows SDK versions define PRODUCT_UNLICENSED
                            strcat(mScratchBuffer, ", Unlicensed");
                            break;
                        case PRODUCT_ULTIMATE:
                            strcat(mScratchBuffer, ", Ultimate Edition");
                            break;
                        case 0x00000030: // PRODUCT_PROFESSIONAL:   // Not all Windows SDK versions define PRODUCT_PROFESSIONAL
                            strcat(mScratchBuffer, ", Professional");
                            break;
                        case PRODUCT_HOME_PREMIUM:
                            strcat(mScratchBuffer, ", Home Premium Edition");
                            break;
                        case PRODUCT_HOME_BASIC:
                            strcat(mScratchBuffer, ", Home Basic Edition");
                            break;
                        case PRODUCT_ENTERPRISE:
                            strcat(mScratchBuffer, ", Enterprise Edition");
                            break;
                        case PRODUCT_BUSINESS:
                            strcat(mScratchBuffer, ", Business Edition");
                            break;
                        case PRODUCT_STARTER:
                            strcat(mScratchBuffer, ", Starter Edition");
                            break;
                        case PRODUCT_CLUSTER_SERVER:
                            strcat(mScratchBuffer, ", Cluster Server Edition");
                            break;
                        case PRODUCT_DATACENTER_SERVER:
                            strcat(mScratchBuffer, ", Datacenter Edition");
                            break;
                        case PRODUCT_DATACENTER_SERVER_CORE:
                            strcat(mScratchBuffer, ", Datacenter Edition (core installation)");
                            break;
                        case PRODUCT_ENTERPRISE_SERVER:
                            strcat(mScratchBuffer, ", Enterprise Edition");
                            break;
                        case PRODUCT_ENTERPRISE_SERVER_CORE:
                            strcat(mScratchBuffer, ", Enterprise Edition (core installation)");
                            break;
                        case PRODUCT_ENTERPRISE_SERVER_IA64:
                            strcat(mScratchBuffer, ", Enterprise Edition for Itanium-based Systems");
                            break;
                        case PRODUCT_SMALLBUSINESS_SERVER:
                            strcat(mScratchBuffer, ", Small Business Server");
                            break;
                        case PRODUCT_SMALLBUSINESS_SERVER_PREMIUM:
                            strcat(mScratchBuffer, ", Small Business Server Premium Edition");
                            break;
                        case PRODUCT_STANDARD_SERVER:
                            strcat(mScratchBuffer, ", Standard Edition");
                            break;
                        case PRODUCT_STANDARD_SERVER_CORE:
                            strcat(mScratchBuffer, ", Standard Edition (core installation)");
                            break;
                        case PRODUCT_WEB_SERVER:
                            strcat(mScratchBuffer, ", Web Server Edition");
                            break;
                        default:
                        {
                            EA::StdC::Sprintf(mScratchBuffer, "Unknown edition: %u", (unsigned)dwProductType);
                            strcat(mScratchBuffer, ", ");
                            strcat(mScratchBuffer, mScratchBuffer);
                            break;
                        }
                    }
                }

                if(Is64BitOperatingSystem())
                    strcat(mScratchBuffer, ", 64 bit");

                WriteKeyValue("OS name", mScratchBuffer);

                EA::StdC::Sprintf(mScratchBuffer, "%u.%u.%u", osVersionInfoEx.dwMajorVersion, osVersionInfoEx.dwMinorVersion, osVersionInfoEx.dwBuildNumber);
                WriteKeyValue("OS version number", mScratchBuffer);

                WriteKeyValue("OS service pack", osVersionInfoEx.szCSDVersion[0] ? osVersionInfoEx.szCSDVersion : "<none>");

                // To consider: do something with this:
                // osVersionInfoEx.dwPlatformId;
            }
        #else
            // To do.
        #endif

        WriteKeyValue("Debugger present", ExceptionHandler::IsDebuggerPresent() ? "yes" : "no");

        #if EA_WINAPI_FAMILY_PARTITION(EA_WINAPI_PARTITION_DESKTOP)
            // System info
            SYSTEM_INFO systemInfo;
            GetNativeSystemInfo(&systemInfo);

            EA::StdC::Sprintf(mScratchBuffer, "%u", systemInfo.dwNumberOfProcessors);
            WriteKeyValue("CPU count", mScratchBuffer);

            // Windows XP SP2 or later:
            // BOOL WINAPI IsWow64Process(HANDLE hProcess, PBOOL Wow64Process);
            // You can use IsWow64Process on 32 bit platforms to see if they are running on Win64. 

            // Windows Vista and later:
            // BOOL WINAPI GetLogicalProcessorInformation(PSYSTEM_LOGICAL_PROCESSOR_INFORMATION Buffer, PDWORD ReturnLength);

            if(systemInfo.wProcessorArchitecture == 0)
                WriteKeyValue("Processor type", "x86");
            else if(systemInfo.wProcessorArchitecture == 9)
                WriteKeyValue("Processor type", "x86-64");
            else if(systemInfo.wProcessorArchitecture == 10)
                WriteKeyValue("Processor type", "x86 on x86-64");

            EA::StdC::Sprintf(mScratchBuffer, "%u", systemInfo.wProcessorLevel);
            WriteKeyValue("Processor level", mScratchBuffer);

            EA::StdC::Sprintf(mScratchBuffer, "%u", systemInfo.wProcessorRevision);
            WriteKeyValue("Processor revision", mScratchBuffer);
        #else
            // To do.
        #endif

        #if EA_WINAPI_FAMILY_PARTITION(EA_WINAPI_PARTITION_DESKTOP)
            // Memory information
            MEMORYSTATUSEX memoryStatusEx;
            memset(&memoryStatusEx, 0, sizeof(memoryStatusEx));
            memoryStatusEx.dwLength = sizeof(memoryStatusEx);

            if(GlobalMemoryStatusEx(&memoryStatusEx))
            {
                EA::StdC::Sprintf(mScratchBuffer, "%d%%", memoryStatusEx.dwMemoryLoad);
                WriteKeyValue("Memory load", mScratchBuffer);

                EA::StdC::Sprintf(mScratchBuffer, "%I64d Mb", memoryStatusEx.ullTotalPhys / (1024 * 1024)); // Or are Mebibytes equal to (1024 * 1000)
                WriteKeyValue("Total physical memory", mScratchBuffer);

                EA::StdC::Sprintf(mScratchBuffer, "%I64d Mb", memoryStatusEx.ullAvailPhys / (1024 * 1024));
                WriteKeyValue("Available physical memory", mScratchBuffer);

                EA::StdC::Sprintf(mScratchBuffer, "%I64d Mb", memoryStatusEx.ullTotalPageFile / (1024 * 1024));
                WriteKeyValue("Total page file memory", mScratchBuffer);

                EA::StdC::Sprintf(mScratchBuffer, "%I64d Mb", memoryStatusEx.ullAvailPageFile / (1024 * 1024));
                WriteKeyValue("Available page file memory", mScratchBuffer);

                EA::StdC::Sprintf(mScratchBuffer, "%I64d Mb", memoryStatusEx.ullTotalVirtual / (1024 * 1024));
                WriteKeyValue("Total virtual memory", mScratchBuffer);

                EA::StdC::Sprintf(mScratchBuffer, "%I64d Mb", memoryStatusEx.ullAvailVirtual / (1024 * 1024));
                WriteKeyValue("Free virtual memory", mScratchBuffer);
            }
        #else
            // To do.
        #endif

        // To consider: Basic video card info would be nice here.

    #elif defined(EA_PLATFORM_PS3)

        char machineName[32];
        int  value;
        int  result;

        if(mOption[kOptionReportComputerName])
        {
            GetMachineName(machineName, sizeof(machineName));
            WriteKeyValue("Computer name", machineName);
        }

        WriteKeyValue("Debugger present", ExceptionHandler::IsDebuggerPresent() ? "yes" : "no");

        if(mOption[kOptionReportUserName])
        {
            char username[CELL_SYSUTIL_SYSTEMPARAM_CURRENT_USERNAME_SIZE];
            result = cellSysutilGetSystemParamString(CELL_SYSUTIL_SYSTEMPARAM_ID_CURRENT_USERNAME, username, sizeof(username));
            if(result != CELL_OK)
                strcpy(username, "unknown");
            WriteKeyValueFormatted("User name", "%s", username);
        }

        if(mOption[kOptionReportUserName])
        {
            char nickname[CELL_SYSUTIL_SYSTEMPARAM_NICKNAME_SIZE];
            result = cellSysutilGetSystemParamString(CELL_SYSUTIL_SYSTEMPARAM_ID_NICKNAME, nickname, sizeof(nickname));
            if(result != CELL_OK)
                strcpy(nickname, "unknown");
            WriteKeyValueFormatted("User nickname", "%s", nickname);
        }

        result = cellSysutilGetSystemParamInt(CELL_SYSUTIL_SYSTEMPARAM_ID_LANG, &value);
        if(result != CELL_OK)
            value = -1;
        WriteKeyValueFormatted("System language", "%d", value);

        result = cellSysutilGetSystemParamInt(CELL_SYSUTIL_SYSTEMPARAM_ID_ENTER_BUTTON_ASSIGN, &value);
        if(result != CELL_OK)
            value = -1;
        WriteKeyValueFormatted("Button assignment", "%d", value);

        result = cellSysutilGetSystemParamInt(CELL_SYSUTIL_SYSTEMPARAM_ID_DATE_FORMAT, &value);
        if(result != CELL_OK)
            value = -1;
        WriteKeyValueFormatted("Date format", "%d", value);

        result = cellSysutilGetSystemParamInt(CELL_SYSUTIL_SYSTEMPARAM_ID_TIME_FORMAT, &value);
        if(result != CELL_OK)
            value = -1;
        WriteKeyValueFormatted("Time format", "%d", value);

        result = cellSysutilGetSystemParamInt(CELL_SYSUTIL_SYSTEMPARAM_ID_GAME_PARENTAL_LEVEL, &value);
        if(result != CELL_OK)
            value = -1;
        WriteKeyValueFormatted("Parental level", "%d", value);

    #elif defined(EA_PLATFORM_OSX)
        if(mOption[kOptionReportComputerName])
        {
            GetMachineName(mScratchBuffer, sizeof(mScratchBuffer));
            WriteKeyValue("Computer name", mScratchBuffer);
        }

        // Write the user name.
        if(mOption[kOptionReportUserName])
        {
            // To do.         
            //    WriteKeyValue("User name", mScratchBuffer);
        }

        WriteKeyValue("Debugger present", ExceptionHandler::IsDebuggerPresent() ? "yes" : "no");

    #elif defined(EA_PLATFORM_UNIX)
        // http://linux.die.net/man/2/uname
        //    "Linux"
        //    "asus-u6"
        //    "2.6.28-11-generic"
        //    "#42-Ubuntu SMP Fri Apr 17 01:57:59 UTC 2009"
        //    "i686" or "x86_64"
        // http://linux.die.net/man/2/gethostname
        // http://linux.die.net/man/2/getdomainname
        
        utsname utsName;
        memset(&utsName, 0, sizeof(utsName));

        if(uname(&utsName) == 0)
        {
            WriteKeyValueFormatted("OS name",    "%s", utsName.sysname); // "Linux"
            WriteKeyValueFormatted("OS version", "%s", utsName.release); // "2.6.28-11-generic"
            WriteKeyValueFormatted("OS build",   "%s", utsName.version); // "#42-Ubuntu SMP Fri Apr 17 01:57:59 UTC 2009"
            WriteKeyValueFormatted("OS machine", "%s", utsName.machine); // "x86_64" (tells you if the OS is 32 or 64 bit)
        }

        #if defined(_SC_NPROCESSORS_CONF)
            long nprocs_max = sysconf(_SC_NPROCESSORS_CONF);
            WriteKeyValueFormatted("CPU count", "%ld", nprocs_max);
        #endif

        #if defined(_SC_NPROCESSORS_ONLN)
            long nprocs_available = sysconf(_SC_NPROCESSORS_ONLN);
            WriteKeyValueFormatted("CPU available", "%ld", nprocs_available);
        #endif

        if(mOption[kOptionReportComputerName])
        {
            GetMachineName(mScratchBuffer, sizeof(mScratchBuffer));
            WriteKeyValue("Computer name", mScratchBuffer);
        }

        if(mOption[kOptionReportComputerName])
        {
            gethostname(mScratchBuffer, (int)sizeof(mScratchBuffer));
            mScratchBuffer[sizeof(mScratchBuffer) - 1] = 0;
            WriteKeyValue("Computer DNS name", mScratchBuffer);
        }

    #endif

    return true;
}


///////////////////////////////////////////////////////////////////////////////
// WriteApplicationInfo
//
// We write something like this (Win32):
//    [Application info]
//    Language: C++
//    Compiler: Microsoft Visual C++ compiler, version 1310
//    App_path: c:\SomeApp\SomeApp.exe
//
// We write something like this (Xenon):
//    [Application info]
//    Language: C++
//    Compiler: Microsoft Visual C++ compiler, version 1400
//    App_path: game:\SomeApp\SomeApp.exe
//
// We write something like this (PS3):
//    [Application info]
//    Language: C++
//    Compiler: GCC compiler, version 4.0
//    SDK version: 0x00102002
//
bool ReportWriter::WriteApplicationInfo()
{
    char appPath[IO::kMaxPathLength];
    EA::StdC::GetCurrentProcessPath(appPath);
    WriteKeyValue("App path", appPath);

    WriteKeyValue("Language", "C++");
    WriteKeyValue("Compiler", EA_COMPILER_STRING); // EA_COMPILER_STRING comes from EABase.

    #if (EA_PLATFORM_PTR_SIZE == 4)
        WriteKeyValue("Addressing", "32 bit");
    #else
        WriteKeyValue("Addressing", "64 bit");
    #endif
    
    #if defined(EA_PLATFORM_XENON)
        EA::StdC::Sprintf(mScratchBuffer, "%u", _XDK_VER);
        WriteKeyValue("XDK", mScratchBuffer);

    #elif defined(EA_PLATFORM_MICROSOFT)
        #if EA_WINAPI_FAMILY_PARTITION(EA_WINAPI_PARTITION_DESKTOP)
            // To consider: Read the Windows VERSIONINFO struct that may be embedded 
            // in the app. However, reading this struct is ridiculously tedious.
            DWORD dwDummy, dwSize = GetFileVersionInfoSizeA(appPath, &dwDummy);

            if(dwSize > 0)
            {
                void* const pVersionData = (void*)LocalAlloc(LPTR, dwSize);

                if(pVersionData)
                {
                    if(GetFileVersionInfoA(appPath, 0, dwSize, pVersionData))
                    {
                        VS_FIXEDFILEINFO* pFFI;
                        UINT size;

                        if(VerQueryValueA(pVersionData, "\\", (void**)&pFFI, &size))
                        {
                            WriteKeyValueFormatted("App version", "%u.%u.%u.%u", HIWORD(pFFI->dwFileVersionMS), LOWORD(pFFI->dwFileVersionMS), 
                                                                                 HIWORD(pFFI->dwFileVersionLS), LOWORD(pFFI->dwFileVersionLS));
                        }
                    }

                    LocalFree((HLOCAL)pVersionData);
                }
            }
        #else
            // To do.
        #endif

    #elif defined(EA_PLATFORM_PS3)

        WriteKeyValueFormatted("SDK version", "0x%08x", CELL_SDK_VERSION);

    #elif defined(EA_PLATFORM_UNIX) || defined(EA_PLATFORM_IPHONE)

        const pid_t processId = getpid();
        WriteKeyValueFormatted("Process id: ", "%I64d (0x%I64x)", (int64_t)processId, (int64_t)processId);
 
        // sysctl exists for Linux OSs as well, but there are different identifiers and a different format for the function call.
        #if defined(EA_PLATFORM_APPLE)
            // Print basic sysctl data
            // https://developer.apple.com/library/mac/#documentation/Darwin/Reference/Manpages/man3/sysctl.3.html#//apple_ref/doc/man/3/sysctl

            int     name[2];
            int     intValue;
            size_t  length;

            name[0] = CTL_KERN;
            name[1] = KERN_OSTYPE;
            length = sizeof(mScratchBuffer);
            mScratchBuffer[0] = 0;
            if(sysctl(name, 2, mScratchBuffer, &length, NULL, 0) == 0)
                WriteKeyValueFormatted("sysctl/CTL_KERN/KERN_OSTYPE", "%s", mScratchBuffer);

            name[0] = CTL_KERN;
            name[1] = KERN_OSREV;
            length = sizeof(intValue);
            intValue = 0;
            if(sysctl(name, 2, &intValue, &length, NULL, 0) == 0)
                WriteKeyValueFormatted("sysctl/CTL_KERN/KERN_OSREV", "%d", intValue);

            name[0] = CTL_HW;
            name[1] = HW_MACHINE;
            length = sizeof(mScratchBuffer);
            mScratchBuffer[0] = 0;
            if(sysctl(name, 2, mScratchBuffer, &length, NULL, 0) == 0)
                WriteKeyValueFormatted("sysctl/CTL_HW/HW_MACHINE", "%s", mScratchBuffer);

            name[0] = CTL_HW;
            name[1] = HW_MODEL;
            length = sizeof(mScratchBuffer);
            mScratchBuffer[0] = 0;
            if(sysctl(name, 2, mScratchBuffer, &length, NULL, 0) == 0)
                WriteKeyValueFormatted("sysctl/CTL_HW/HW_MODEL", "%s", mScratchBuffer);

            name[0] = CTL_HW;
            name[1] = HW_NCPU;
            length = sizeof(intValue);
            intValue = 0;
            if(sysctl(name, 2, &intValue, &length, NULL, 0) == 0)
                WriteKeyValueFormatted("sysctl/CTL_HW/HW_NCPU", "%d", intValue);

            name[0] = CTL_KERN;
            name[1] = KERN_OSRELEASE;
            length = sizeof(mScratchBuffer);
            mScratchBuffer[0] = 0;
            if(sysctl(name, 2, mScratchBuffer, &length, NULL, 0) == 0)
                WriteKeyValueFormatted("sysctl/CTL_KERN/KERN_OSRELEASE", "%s", mScratchBuffer);

            length = sizeof(mScratchBuffer);
            mScratchBuffer[0] = 0;
            if(sysctlbyname("hw.acpi.thermal.tz0.temperature", &mScratchBuffer, &length, NULL, 0) == 0)
                WriteKeyValueFormatted("hw.acpi.thermal.tz0.temperature", "%s", mScratchBuffer);

            #if defined(EA_PLATFORM_APPLE)
                length = sizeof(mScratchBuffer);
                mScratchBuffer[0] = 0;
                if(sysctlbyname("machdep.cpu.brand_string", &mScratchBuffer, &length, NULL, 0) == 0)
                    WriteKeyValueFormatted("machdep.cpu.brand_string", "%s", mScratchBuffer);

                // Read kernel memory size.
                host_basic_info_data_t hostinfo;
                mach_msg_type_number_t count = HOST_BASIC_INFO_COUNT;
                kern_return_t          kr = host_info(mach_host_self(), HOST_BASIC_INFO, (host_info_t)&hostinfo, &count);

                if(kr == KERN_SUCCESS)
                {
                    const uint64_t memoryMib = (uint64_t)hostinfo.max_mem / (1024 * 1024);
                    WriteKeyValueFormatted("System memory", "%I64d Mib (%.2f Gib)", memoryMib, (double)memoryMib / 1024);
                }
            #endif
        #endif
    #endif

    return true;
}


///////////////////////////////////////////////////////////////////////////////
// WriteOpenFileList
//
bool ReportWriter::WriteOpenFileList()
{
    // To do: Implement this for Windows.
    // http://forum.sysinternals.com/enumerate-opened-files_topic3577.html
    // http://www.codeproject.com/Articles/18975/Listing-Used-Files

    #if defined(EA_PLATFORM_OSX) || defined(EA_PLATFORM_LINUX) || (defined(EA_PLATFORM_BSD) && defined(EA_PLATFORM_DESKTOP))
    
        ////////////////////////////////////////////////////////////////////////////////////
        // Unix platforms usually don't provide a programmatic way to list open files, but the system-provided lsof 
        // function does do so. We can spawn that process through popen and read its output, given sufficient privileges.
        // We call with -b to not block, -s to always display the file size, -w to suppress warning output.
        //
        // http://linux.die.net/man/8/lsof
        // http://developer.apple.com/library/mac/#documentation/Darwin/Reference/ManPages/man8/lsof.8.html
        // 
        // MacBook-Air:~ PPedriana$ lsof -b -s -w -p 26459
        // COMMAND   PID      USER   FD   TYPE DEVICE       SIZE  NODE NAME
        // bash    26459 PPedriana  cwd    DIR   14,2       816   327919 /Users/PPedriana
        // bash    26459 PPedriana  txt    REG   14,2   1371648     8702 /bin/bash
        // bash    26459 PPedriana  txt    REG   14,2    599280 29736407 /usr/lib/dyld
        // bash    26459 PPedriana    2u   CHR   16,1   0t43325      957 /dev/ttys001
        // bash    26459 PPedriana  255u   CHR   16,1                  4 /dev/pts/2
        //
        // Note that the SIZE field might be absent.
        ////////////////////////////////////////////////////////////////////////////////////
        
        char   command[32];
        pid_t  pid = getpid();
        EA::StdC::Sprintf(command, "lsof -b -s -w -p  %lld", (int64_t)pid);
        FILE*  pFile = popen(command, "r");

        if(pFile)
        {
            char line[512];
            bool bFoundHeader  = false;
           
            // Problem: lsof may require a higher privilege than the running app has. Can that happen  
            // even though this process is asking for information about itself?
            //
            // The following logic is fairly hard-coded to the output of the lsof utility. If it changes
            // significantly then the following may stop working.
            
            while(fgets(line, EAArrayCount(line), pFile))
            {
                if(!bFoundHeader)
                {
                    if(EA::StdC::Strstr(line, "COMMAND") != line) // We expect the first line to read "COMMAND PID ..." 
                    {
                        // Looks like the data isn't what we expect, so quit. Need to kill the process?
                        break;
                    }
                    
                    bFoundHeader = true;
                }
                else
                {
                    // The line is of the form (64 bit):
                    //bash    26459 PPedriana  txt    REG   14,2    599280 29736407 /usr/lib/dyld
                    //bash    26459 PPedriana  255u   CHR   16,1                  4 /dev/pts/2
                    
                    uint64_t fileSize;
                    uint64_t fileNode; 
                    char     modulePath[512];
                    int      fieldCount = EA::StdC::Sscanf(line, "%*s %*s %*s %*s %*s %*s %llu %llu %512s", &fileSize, &fileNode, modulePath);
                    
                    if(fieldCount == 3) // If the size field is absent then this will be < 3. 
                        WriteFormatted("Size: %9llu Path: %s\r\n", fileSize, modulePath);
                }
            }
            
            pclose(pFile);
        }
        else
            WriteText("(Unable to gather file information, possibly due to rights needed for executing lsof.)");

        return true;

    #elif defined(EA_PLATFORM_WINDOWS) && EA_WINAPI_FAMILY_PARTITION(EA_WINAPI_PARTITION_DESKTOP)
        bool result = false;

        HMODULE hNtDll = GetModuleHandleW(L"ntdll.dll");
        if(!hNtDll)
            goto Failure;

        FN_NTQUERYSYSTEMINFORMATION pNtQuerySystemInformation = (FN_NTQUERYSYSTEMINFORMATION)(uintptr_t)GetProcAddress(hNtDll, "NtQuerySystemInformation");
        FN_NTQUERYOBJECT            pNtQueryObject            = (FN_NTQUERYOBJECT)           (uintptr_t)GetProcAddress(hNtDll, "NtQueryObject");

        if(!pNtQuerySystemInformation || !pNtQueryObject)
            goto Failure;

        SYSTEM_HANDLE_INFORMATION* pSHI     = NULL;
        NTSTATUS                   ntStatus = 0;
        ULONG                      shiSize  = 65536; // This will need to be 2,000,000 or more on modern systems such as Window 7 or later.

        TryAgain:
        pSHI = (SYSTEM_HANDLE_INFORMATION*)malloc(shiSize); // Must do: Replace this with an OS-level memory allocation call. Our heap should be considered unsable due to this being exception handling code.

        if(pSHI)
        {
            ntStatus = (*pNtQuerySystemInformation)(SystemHandleInformation, pSHI, shiSize, &shiSize);

            if(!NT_SUCCESS(ntStatus) && (ntStatus == STATUS_INFO_LENGTH_MISMATCH) && (shiSize > 0) && (shiSize < (2 * 1024 * 1024)))
            {
                free(pSHI);
                shiSize = (shiSize * 4) / 3;
                goto TryAgain; 
            }
        }

        if(!NT_SUCCESS(ntStatus))
        {
            free(pSHI);
            goto Failure;
        }

        result = true;
        DWORD currentProcessID = GetCurrentProcessId();

        for(ULONG c = 0; c < pSHI->HandleCount; c++) // HandleCount will be upwards of 100,000 or more on modern systems such as Windows 7 and later.
        {
            SYSTEM_HANDLE& systemHandle = pSHI->Handles[c];

            if(systemHandle.ProcessId == currentProcessID)
            {
                // Only ObjectTypeNumbers of 25 and 28 matter to us, as they refer to file types. This may change for future versions of Windows.
                if((systemHandle.ObjectTypeNumber == 28) || (systemHandle.ObjectTypeNumber == 25))
                {
                    // Other source code for enumerating files has code like the following, which ignores certain types
                    // of files (named pipes) based on their SYSTEM_HANDLE::GrantedAccess value. The reason for this is 
                    // that calls on them may block. 
                    // http://forum.sysinternals.com/discussion-howto-enumerate-handles_topic19403_page5.html
                    // http://blogs.charteris.com/blogs/chrisdi/archive/2008/06/16/exploring-the-wcf-named-pipe-binding-part-2.aspx
                    // http://forum.madshi.net/viewtopic.php?t=4996

                    if((systemHandle.GrantedAccess != 0x0012019f) &&    // Blocking named pipes
                       (systemHandle.GrantedAccess != 0x001a019f) &&    // Blocking named pipes
                       (systemHandle.GrantedAccess != 0x00120189) &&    // Blocking named pipes
                       (systemHandle.GrantedAccess != 0x00160001) &&    // System directories
                       (systemHandle.GrantedAccess != 0x00100001) &&    // System directories
                       (systemHandle.GrantedAccess != 0x00100000) &&    // SYNCHRONIZE only
                       (systemHandle.GrantedAccess != 0x00100020))      // SxS files
                    {  
                        HANDLE                          hFile = (HANDLE)systemHandle.Handle;
                        const ULONG                     kObjectNameBufferSize = 8192;
                        char                            objectNameBuffer[kObjectNameBufferSize + sizeof(wchar_t)];
                        ULONG                           objectNameSize = 0;
                        PUBLIC_OBJECT_TYPE_INFORMATION* pObjectTypeName = (PUBLIC_OBJECT_TYPE_INFORMATION*)objectNameBuffer;
                        wchar_t                         objectNameW[16] = {};

                        NTSTATUS nts = (*pNtQueryObject)(hFile, ObjectTypeInformation, pObjectTypeName, kObjectNameBufferSize, &objectNameSize);

                        if(NT_SUCCESS(nts) && (pObjectTypeName->TypeName.Length == (4 * sizeof(wchar_t))))
                        {
                            memcpy(objectNameW, pObjectTypeName->TypeName.Buffer, (4 * sizeof(wchar_t)));
                            objectNameW[5] = 0;

                            if(EA::StdC::Stricmp(objectNameW, L"File") == 0)
                            {
                                wchar_t  pFilePath[MAX_PATH];
                                uint64_t fileSize = 0; // To do.

                                if(GetFileNameFromHandle(hFile, pFilePath, EAArrayCount(pFilePath), fileSize))
                                {
                                    const DWORD dwFileAttributes = GetFileAttributesW(pFilePath); 

                                    WriteFormatted("Size: %9I64u Type: %4s Path: %ls\r\n", fileSize, (dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? "dir" : "file", pFilePath);
                                }
                                //else
                                //    WriteFormatted("File\r\n");
                            }
                        }
                    }
                }
            }
        }

        free(pSHI);

        Failure:
        if(!result)
            WriteText("(info currently unavailable for this platform or process)\r\n");

        return result;

    #else
        WriteText("(info currently unavailable for this platform or process)\r\n");
        return false;
    #endif
}


#if EXCEPTION_HANDLER_GET_CALLSTACK_FROM_THREAD_SUPPORTED
    // We aren't currently using EASuspendThread.
#else
    // EAThread doesn't provide a SuspendThread function. And it's pretty risky.
    static bool EASuspendThread(EA::Thread::ThreadId threadId, EA::Thread::SysThreadId sysThreadId, bool bSuspend)
    {
        #if (defined(EA_PLATFORM_MICROSOFT) && EA_WINAPI_FAMILY_PARTITION(EA_WINAPI_PARTITION_DESKTOP)) || defined(EA_PLATFORM_XENON)
            EA_UNUSED(sysThreadId);
            if(threadId)
            {
                if(bSuspend)
                    return (SuspendThread(threadId) != 0xffffffff);
                else
                    return (ResumeThread(threadId) != 0xffffffff);
            }
            return true;

        #elif defined(EA_PLATFORM_APPLE)
            EA_UNUSED(threadId);
            if(bSuspend)
                return (thread_suspend(sysThreadId) == KERN_SUCCESS); 
            else
                return (thread_resume(sysThreadId) == KERN_SUCCESS);

        #else
            EA_UNUSED(sysThreadId);
            EA_UNUSED(threadId);
            EA_UNUSED(bSuspend);
            return false;
        #endif
    }
#endif


void ReportWriter::WriteAbbreviatedCallstackForThread(EA::Thread::ThreadId threadId, EA::Thread::SysThreadId sysThreadId, bool bCurrentThread)
{
    // Print a brief callstack for each thread. We need to suspend the thread before 
    // we do anything with it, otherwise it will change as we go. If we don't trust 
    // pausing a thread then we may need to use the signal handling injection method.
    EA::Callstack::CallstackContext context;

    if(bCurrentThread)
    {
        EA::Callstack::GetCallstackContext(context, (intptr_t)EA::Thread::kThreadIdInvalid);
        Write("Callstack:\r\n");
        WriteCallStackFromCallstackContext(&context, Callstack::kARTFlagFileLine | Callstack::kARTFlagFunctionOffset | Callstack::kARTFlagAddress, true);
    }
    else
    {
        // For some platforms it's a lot better to get the callstack via the thread id or lwp id. 
        // For example, on Unix and Unix-derived platforms the best way to get the callstack of another
        // thread is to inject signal handlers into that thread and trigger the signals via the thread id.
        #if EXCEPTION_HANDLER_GET_CALLSTACK_FROM_THREAD_SUPPORTED
            EA_UNUSED(sysThreadId);

            #if (EATHREAD_VERSION_N >= 12002)
                void*     callstack[32];
                size_t    callstackCount = EA::Thread::GetCallstack(callstack, EAArrayCount(callstack), threadId); // This is a new EAThread callstack function

                Write("Callstack:\r\n");
                WriteCallStackFromCallstack(callstack, callstackCount, Callstack::kARTFlagFileLine | Callstack::kARTFlagFunctionOffset | Callstack::kARTFlagAddress, true);
            #endif
        #else
            if(EASuspendThread(threadId, sysThreadId, true)) // If could suspend...
            {
                if(EA::Callstack::GetCallstackContextSysThreadId(context, (intptr_t)sysThreadId))
                {
                    Write("Callstack:\r\n");
                    WriteCallStackFromCallstackContext(&context, Callstack::kARTFlagFileLine | Callstack::kARTFlagFunctionOffset | Callstack::kARTFlagAddress, true);
                }
            
                EASuspendThread(threadId, sysThreadId, false);
            }
        #endif
    }
}




///////////////////////////////////////////////////////////////////////////////
// WriteThreadList
//
bool ReportWriter::WriteThreadList()
{
    bool bSuccess = false;
    
    #if defined(EA_PLATFORM_MICROSOFT) && EA_WINAPI_FAMILY_PARTITION(EA_WINAPI_PARTITION_DESKTOP)
    
        DWORD  dwCurrentProcessId = GetCurrentProcessId();
        HANDLE hThreadSnap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, dwCurrentProcessId); // It turns out that CreateToolhelp32Snapshot ignores dwCurrentProcessId, so we need to re-check it below.
        
        if(hThreadSnap != INVALID_HANDLE_VALUE) 
        {
            THREADENTRY32 te32;
            te32.dwSize = sizeof(THREADENTRY32); 
         
            if(Thread32First(hThreadSnap, &te32)) 
            {            
                do { 
                    if(te32.th32OwnerProcessID == dwCurrentProcessId)
                    {
                        // To do: Provide a function in EAThread which converts a Microsoft DWORD thread  
                        // id (EA::Thread::SysThreadId) to the Microsoft thread HANDLE (EA::Thread::ThreadId)
                        // used by the given EAThread (if from EAThread).
                        // We could use hThread = OpenThread(); ...; CloseHandle(hThread), but the hThread 
                        // woudl be a different value than EAThread is using, as hThread is just a pointer
                        // to the DWORD-based thread info and is not unique itself.

                        // There is no such thing as a unique thread HANDLE you can get from the system 
                        // for a DWORD thread id. Along with this we can also get the name of the thread
                        // as provided by the user for debugging.
                        const bool bThreadIsExceptionThread = (te32.th32ThreadID == mExceptionSysThreadId);
                        const bool bCurrentThread           = (te32.th32ThreadID == GetCurrentThreadId());

                        WriteKeyValueFormatted("Thread", "SysThreadId (DWORD): %u%s, Windows thread base priority: %ld", 
                                                te32.th32ThreadID, bThreadIsExceptionThread ? " (exception thread)" : "", te32.tpBasePri);
                        
                        if(!bThreadIsExceptionThread) // If we haven't already printed the callstack for this thread, because it's the exception thread...
                        {
                            HANDLE hThread = ::OpenThread(THREAD_SUSPEND_RESUME | THREAD_GET_CONTEXT, TRUE, te32.th32ThreadID);

                            if(hThread)
                            {
                                WriteAbbreviatedCallstackForThread(hThread, te32.th32ThreadID, bCurrentThread);
                                ::CloseHandle(hThread);
                            }
                        }
                    }
                } while(Thread32Next(hThreadSnap, &te32));

                bSuccess = true;
            }
            
            CloseHandle(hThreadSnap);
        }

    #elif defined(EA_PLATFORM_APPLE)

        // http://www.gnu.org/software/hurd/gnumach-doc/Task-Information.html
        mach_port_t             taskSelf   = mach_task_self();   // Mach task id, which is the underlying kernel data for the Unix process.
        mach_port_t             threadSelf = mach_thread_self(); // Mach kernel thread id, which is the underlying kernel thread for pthreads.
        thread_act_port_array_t threadArray;                     // Kernel thread array.
        mach_msg_type_number_t  threadCount;                     // Count of elements in threadArray.
        
        kern_return_t result = task_threads(taskSelf, &threadArray, &threadCount);
        
        if(result == KERN_SUCCESS)
        {
            for(mach_msg_type_number_t i = 0; i < threadCount; i++)
            {
                // Question: Do we need to suspend the given thread to read all the info about it below?
                
                mach_port_t thread  = threadArray[i];
                pthread_t   pthread = pthread_from_mach_thread_np(thread); // It's possible that this thread was not created by pthreads, in which case the pthread value would be invalid. The Posix API doesn't define what an invalid value is or how to detect one.
                const bool  bCurrentThread = (thread == threadSelf);
                const bool  bThreadIsExceptionThread = (thread == mExceptionSysThreadId);

                char threadName[32];
                pthread_getname_np(pthread, threadName, sizeof(threadName));
                if(threadName[0] == 0)
                    EA::StdC::Strcpy(threadName, "(unavailable)");
                    
                union {
                    natural_t         words[THREAD_INFO_MAX];
                    thread_basic_info bi;
                } biUnion;
                char threadState[32] = "unknown";
                mach_msg_type_number_t threadInfoCount = THREAD_INFO_MAX;
                kern_return_t result = thread_info(thread, THREAD_BASIC_INFO, (thread_info_t)&biUnion, &threadInfoCount);
                if(result == KERN_SUCCESS)
                {
                    switch (biUnion.bi.run_state)
                    {
                        case TH_STATE_RUNNING:         EA::StdC::Strlcpy(threadState, "running",         sizeof(threadState)); break;
                        case TH_STATE_STOPPED:         EA::StdC::Strlcpy(threadState, "stopped",         sizeof(threadState)); break;
                        case TH_STATE_WAITING:         EA::StdC::Strlcpy(threadState, "waiting",         sizeof(threadState)); break;
                        case TH_STATE_UNINTERRUPTIBLE: EA::StdC::Strlcpy(threadState, "uninterruptible", sizeof(threadState)); break;
                        case TH_STATE_HALTED:          EA::StdC::Strlcpy(threadState, "halted",          sizeof(threadState)); break;
                    }
                    
                    if(biUnion.bi.flags & TH_FLAGS_SWAPPED)
                        EA::StdC::Strlcat(threadState, ", swapped", sizeof(threadState));
                        
                    if(biUnion.bi.flags & TH_FLAGS_IDLE)
                        EA::StdC::Strlcat(threadState, ", idle", sizeof(threadState));
                }
                

                // The thread_id and thread_handle reported in thread_identifier_info are 
                // not the same as either the kernel thread id (mach_port_t) or the pthread
                // id (pthread_t). thread_handle appears to be an internal pointer to some
                // data that seems to be close in memory to the pthread value.
                thread_identifier_info threadIdentifierInfo; memset(&threadIdentifierInfo, 0, sizeof(threadIdentifierInfo));
                mach_msg_type_number_t threadIdentifierInfoCount = THREAD_IDENTIFIER_INFO_COUNT;
                /*result=*/ thread_info(thread, THREAD_IDENTIFIER_INFO, (thread_info_t)&threadIdentifierInfo, &threadIdentifierInfoCount);

                int currentPriority = 0;

                #if defined(EA_PLATFORM_OSX) // Not available on iOS
                    pid_t           processId  = getpid();           // Unix process id.
                    proc_threadinfo procThreadInfo; memset(&procThreadInfo, 0, sizeof(procThreadInfo));
                    int procResult = proc_pidinfo(processId, PROC_PIDTHREADINFO, threadIdentifierInfo.thread_handle, &procThreadInfo, sizeof(procThreadInfo));
                    if(procResult >= 0)
                        currentPriority = procThreadInfo.pth_curpri;
                #endif
                
                WriteKeyValueFormatted("Thread", "pthread id: %llx%s, kernel thread id: %d, name: %s%s, state: %s, suspend count: %d, kernel priority: %d", 
                                        (unsigned long long)pthread, bThreadIsExceptionThread ? " (exception thread)" : "", thread, threadName, 
                                        bCurrentThread ? " (exception handling thread)" : "", threadState, (int)biUnion.bi.suspend_count, currentPriority);

                if(!bThreadIsExceptionThread)  // If we haven't already printed the callstack for this thread, because it's the exception thread...
                    WriteAbbreviatedCallstackForThread(pthread, thread, bCurrentThread);
            }
            
            // The docs say to deallocate with vm_deallocate, which seems odd. Example code on the Internet is like so:
            vm_deallocate(taskSelf, (vm_address_t)threadArray, threadCount * sizeof(thread_act_t));
            
            bSuccess = true;
        }

    #elif defined(EA_PLATFORM_XENON)
        #if EA_XBDM_ENABLED
            DWORD   dwThreadArray[256];
            DWORD   dwThreadArrayCount = 256;
            HRESULT hResult = DmGetThreadList(dwThreadArray, &dwThreadArrayCount);

            if(hResult == XBDM_NOERR) // Should we add "|| (hResult == XBDM_BUFFER_TOO_SMALL)" ?
            {
                for(DWORD i = 0; i < dwThreadArrayCount; i++)
                {
                    const bool bThreadIsExceptionThread = (dwThreadArray[i] == mExceptionSysThreadId);
                    const bool bCurrentThread           = (dwThreadArray[i] == GetCurrentThreadId());
                    HANDLE     hThread                  = ::OpenThread(THREAD_SUSPEND_RESUME | THREAD_GET_CONTEXT, TRUE, dwThreadArray[i]);
                    int        priority                 = hThread ? ::GetThreadPriority(hThread) : -1;

                    WriteKeyValueFormatted("Thread", "SysThreadId (DWORD): %u%s, Windows thread base priority: %d", 
                                            dwThreadArray[i], bThreadIsExceptionThread ? " (exception thread)" : "", priority);

                    if(hThread)
                    {
                        if(!bThreadIsExceptionThread) // If we haven't already printed the callstack for this thread, because it's the exception thread...
                            WriteAbbreviatedCallstackForThread(hThread, dwThreadArray[i], bCurrentThread);
                    }
                    else
                        WriteText("   (Unable to read thread)\r\n");

                    if(hThread)
                        ::XCloseHandle(hThread);
                }
            }

            bSuccess = true;
        #else
            // Use EAThread to get a list of threads, though it's not available as of this writing.
            // Merge the code directly above with this code when we do so.
        #endif

    #else
        // Note about the PS3 platform:
        // sys_dbg_get_ppu_thread_ids doesn't work on PS3 and by design always fails. 
        // The only way for this to work is for EAThread to provide thread enumeration.
        // However, one EA user reported that they believe sys_dbg_get_ppu_thread_ids
        // works when within an exception handler, at least in recent SDK versions.

        #if (EATHREAD_VERSION_N >= 12006) // If EA::Thread::EnumerateThreads is available...
            EA::Thread::ThreadEnumData threadEnumData[24];
            size_t                     threadEnumDataCount = EA::Thread::EnumerateThreads(threadEnumData, EAArrayCount(threadEnumData));

            for(size_t i = 0; i < threadEnumDataCount; i++)
            {
                // We have a problem here in that there isn't a consistent way to get a ThreadId from ThreadDynamicData. It currently differs per platform.
                #if EA_USE_CPP11_CONCURRENCY
                    EA::Thread::ThreadId threadId = threadEnumData[i].mpThreadDynamicData->mThread.get_id();
                #elif defined(EA_PLATFORM_MICROSOFT) && !EA_POSIX_THREADS_AVAILABLE
                    EA::Thread::ThreadId threadId = threadEnumData[i].mpThreadDynamicData->mhThread;
                #else
                    EA::Thread::ThreadId threadId = threadEnumData[i].mpThreadDynamicData->mThreadId;
                #endif

                EA::Thread::SysThreadId sysThreadId = 0;

                if(threadId != EA::Thread::kThreadIdInvalid)
                   sysThreadId = EA::Thread::GetSysThreadId(threadId);

                WriteKeyValueFormatted("Thread", "ThreadId: %s, ThreadSysId: %s, name: %s", 
                                                EAThreadThreadIdToString(threadId),
                                                EAThreadSysThreadIdToString(sysThreadId),                                        
                                                threadEnumData[i].mpThreadDynamicData->mName);

                threadEnumData[i].Release();
            }
            bSuccess = true;
        #endif
    #endif

    if(!bSuccess)
        WriteText("(info currently unavailable for this platform or process)\r\n");
        
    return bSuccess;
}


///////////////////////////////////////////////////////////////////////////////
// WriteProcessList
//
bool ReportWriter::WriteProcessList()
{
    bool bSuccess = false;

    if(mOption[kOptionReportUserName])
    {
        #if defined(EA_PLATFORM_WINDOWS) && EA_WINAPI_FAMILY_PARTITION(EA_WINAPI_PARTITION_DESKTOP)
            HANDLE hProcessSnap;

            // Take a snapshot of all processes in the system.
            hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

            if(hProcessSnap != INVALID_HANDLE_VALUE)
            {
                PROCESSENTRY32W pe32;
                memset(&pe32, 0, sizeof(pe32));
                pe32.dwSize = sizeof(pe32);

                if(Process32FirstW(hProcessSnap, &pe32))
                {
                    bSuccess = true;

                    do {
                        WriteFormatted("0x%08x %ls\r\n", pe32.th32ProcessID, pe32.szExeFile);
                    } while(Process32NextW(hProcessSnap, &pe32));
                }

                CloseHandle(hProcessSnap);
            }
        #elif defined(EA_PLATFORM_OSX)
            // As odd as it seems proc_listpids takes a memory size for its fourth argument, not array count. And it returns memory size.
            // If you pass NULL and 0 for the last to arguments, it returns the required size to hold all processes.
            // If your pidArray is smaller than required then it fills in what it can and returns how much it wrote as per above.
            pid_t pidArray[256]; 
            int   processCount = (proc_listpids(PROC_ALL_PIDS, 0, pidArray, sizeof(pidArray)) / sizeof(pid_t));
            char  processFilePath[PATH_MAX];

            for(int i = 0; i < processCount; i++)
            {
                if(proc_pidpath(pidArray[i], processFilePath, sizeof(processFilePath)) > 0)
                    WriteFormatted("%6d %s\r\n", pidArray[i], processFilePath);
            }
            
            bSuccess = (processCount > 0);
        #endif
        
        if(!bSuccess)
            WriteText("(info currently unavailable for this platform or process)\r\n");
    }
    else
        WriteText("kOptionReportProcessList is currently disabled. Use ReportWriter::SetOption to enable it.\r\n");

    return bSuccess;
}



#if (defined(EA_PROCESSOR_POWERPC) || defined(EA_PROCESSOR_X86) || defined(EA_PROCESSOR_ARM))
    void ReportWriter::WriteDisassembly(Disassembler& dasm, const uint8_t* pInstructionData, size_t count, const uint8_t* pInstructionPointer)
    {
        if(pInstructionData && (pInstructionData < (pInstructionData + count)))
        {
            #ifdef _MSC_VER
                __try
                {
            #endif
                    EA::Dasm::DasmData dd;

                    const void* pCurrent = pInstructionData;
                    const void* pEnd     = pInstructionData + count;

                    while(pCurrent != pEnd)
                    {
                        pCurrent = dasm.Dasm(pCurrent, pEnd, dd, EA::Dasm::kOFHex | EA::Dasm::kOFMnemonics, (uintptr_t)pCurrent);

                        if(pInstructionPointer && (pInstructionPointer <= pCurrent))
                        {
                            pInstructionPointer = NULL;
                            EA::StdC::Snprintf(mWriteBuffer, kWriteBufferSize, "%s => %s %s\r\n", dd.mAddress, dd.mOperation, dd.mOperands);
                        }
                        else
                            EA::StdC::Snprintf(mWriteBuffer, kWriteBufferSize, "%s    %s %s\r\n", dd.mAddress, dd.mOperation, dd.mOperands);

                        WriteText(mWriteBuffer, false);
                    }
            #ifdef _MSC_VER
                }
                __except(EXCEPTION_EXECUTE_HANDLER)
                {
                    WriteText("Disassembly write failed due to an exception.\r\n", false);
                }
            #endif
        }
        else
            WriteText("Instruction pointer appears to point to invalid memory.\r\n", false);
    }
#endif


///////////////////////////////////////////////////////////////////////////////
// ReportWriteDisassembly
//
// We write something that looks like this (PowerPC):
//     004058f1    lis     r9,-24577
//     004058fe    ori     r9,r9,38912
//     00405801    li      r0,255
//     004059d7 => lwz     r8,4(r9)
//     004059e1    sth     r6,4(r7)
//     004059e8    stb     r7,4(r9)
//     004059ea    blr
//
bool ReportWriter::WriteDisassembly(const uint8_t* pInstructionData, size_t count, const uint8_t* pInstructionPointer)
{
    #if (defined(EA_PROCESSOR_POWERPC) || defined(EA_PROCESSOR_X86) || defined(EA_PROCESSOR_ARM))
        Disassembler dasm;
        WriteDisassembly(dasm, pInstructionData, count, pInstructionPointer);
    #else
        // EACallstack didn't have DASM support for the current platform the last time we checked.
        WriteMemoryView(pInstructionData, count, pInstructionPointer);
    #endif

    return true;
}



enum MemoryAccess
{
    kMemoryAccessNone    = 0x00,
    kMemoryAccessRead    = 0x01,
    kMemoryAccessWrite   = 0x02,
    kMemoryAccessExecute = 0x04,
    kMemoryAccessUnknown = 0x10
};


///////////////////////////////////////////////////////////////////////////////
// GetMemoryAccess
//
// Returns MemoryAccess flags. Returns kMemoryAccessUnknown for unknown access.
//
static int GetMemoryAccess(const void* p)
{
    int flags = 0;
    
    #if defined(EA_PLATFORM_XENON)
        // Cannot use VirtualQuery here as it will fail on call stack. Fortunately
        // there is functional replacement for it.
        const DWORD pageAccess = XQueryMemoryProtect((LPVOID)p);

        if(pageAccess & (PAGE_READONLY | PAGE_READWRITE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE))
            flags |= kMemoryAccessRead;
        if(pageAccess & (PAGE_READWRITE | PAGE_EXECUTE_READWRITE))
            flags |= kMemoryAccessWrite;
      //if(((pageAccess & PAGE_NOACCESS) == 0) && ((pageAccess & PAGE_READWRITE) == 0)) // To do: Test if this works before enabling it.
      //    flags |= kMemoryAccessExecute;

    #elif defined(EA_PLATFORM_MICROSOFT)
        MEMORY_BASIC_INFORMATION mbi;

        if(VirtualQuery(p, &mbi, sizeof(mbi)))
        {
            if(mbi.State == MEM_COMMIT)
            {
                if(mbi.Protect & PAGE_READONLY)
                    flags |= kMemoryAccessRead;
                if(mbi.Protect & PAGE_READWRITE)
                    flags |= (kMemoryAccessRead | kMemoryAccessWrite);
                if(mbi.Protect & PAGE_EXECUTE)
                    flags |= kMemoryAccessExecute;
                if(mbi.Protect & PAGE_EXECUTE_READ)
                    flags |= (kMemoryAccessRead | kMemoryAccessExecute);
                if(mbi.Protect & PAGE_EXECUTE_READWRITE)
                    flags |= (kMemoryAccessRead | kMemoryAccessWrite | kMemoryAccessExecute);
                if(mbi.Protect & PAGE_EXECUTE_WRITECOPY)
                    flags |= (kMemoryAccessWrite | kMemoryAccessExecute);
                if(mbi.Protect & PAGE_WRITECOPY)
                    flags |= kMemoryAccessWrite;
            }
        }

    #elif defined(EA_PLATFORM_PS3)
        sys_page_attr_t pageAccess;

        if(sys_memory_get_page_attribute((sys_addr_t)p, &pageAccess) == CELL_OK)
        {
            if(pageAccess.attribute & SYS_MEMORY_PROT_READ_ONLY)
                flags |= kMemoryAccessRead;
            if(pageAccess.access_right & SYS_MEMORY_PROT_READ_WRITE)
                flags |= (kMemoryAccessRead | kMemoryAccessWrite);
            // The PS3 doesn't seem to provide a way to tell if memory is executable. 
            // There are some hacky ways to help tell executability, but nothing perfect.
        }

    #elif defined(EA_PLATFORM_APPLE)
        
        #if !defined(EA_MACH_VM_AVAILABLE)
            #if (EA_PLATFORM_PTR_SIZE == 4)
                #define EA_MACH_VM_AVAILABLE 1
            #else
                #define EA_MACH_VM_AVAILABLE 0
            #endif
        #endif
                
        #if (EA_MACH_VM_AVAILABLE)// If the 64-bit savvy mach_xxx function set is available (you need to enable the Kernel framework for it to be so)...
            #if (EA_PLATFORM_PTR_SIZE == 8)
                mach_port_t                    taskSelf   = mach_task_self();
                vm_region_flavor_t             flavor     = VM_REGION_BASIC_INFO_64;    // This is supported by OSX 32 and 64, but is it supported by iOS?
                vm_region_basic_info_data_64_t info;
                mach_msg_type_number_t         infoCount  = VM_REGION_BASIC_INFO_COUNT_64;
                mach_vm_size_t                 vmSize     = 0; // The returned size will be the size of the entire region, and not necessarily a single page.
                mach_vm_address_t              address    = (mach_vm_address_t)p;
                mach_port_t                    objectName = MACH_PORT_NULL;

                kern_return_t result = mach_vm_region(taskSelf, &address, &vmSize, flavor, (vm_region_info_t)&info, &infoCount, &objectName);
            #else           
                // We can use the legacy 32 bit functionality that doesn't require enabling the Kernel framework.
                mach_port_t                 taskSelf   = mach_task_self();
                vm_region_flavor_t          flavor     = VM_REGION_BASIC_INFO;    // This is supported by OSX 32 and 64, but is it supported by iOS?
                vm_region_basic_info_data_t info;
                mach_msg_type_number_t      infoCount  = VM_REGION_BASIC_INFO_COUNT;
                vm_size_t                   vmSize     = 0; // The returned size will be the size of the entire region, and not necessarily a single page.
                vm_address_t                address    = (vm_address_t)p;
                mach_port_t                 objectName = MACH_PORT_NULL;

                // It turns out that vm_region is a liar or underfeatured, and it says that memory is 
                // readable when in fact it is not. Probably there's some explanation, but the fact 
                // is that we cannot read all memory that it says has VM_PROT_READ.
                kern_return_t result = vm_region(taskSelf, &address, &vmSize, flavor, (vm_region_info_t)&info, &infoCount, &objectName);
                
                if((result == KERN_SUCCESS) && (info.protection & VM_PROT_READ))
                {
                    // Verify that the supposedly readable memory can in fact be read.
                    uint64_t  memory[4];
                    vm_size_t dataCount = 32;
                    result = vm_read_overwrite(taskSelf, address, 32, (vm_address_t)memory, &dataCount);
                }
            #endif

        #else
            EA_UNUSED(p);
            vm_region_basic_info_data_64_t info;
            memset(&info, 0, sizeof(info));

            kern_return_t result = KERN_FAILURE;
        #endif
        
        if(result == KERN_SUCCESS)
        {
            // The max_protection fields refer to the max -allowed- rights, not the current rights (indicated by the protection field).
            if(info.protection & VM_PROT_READ)
                flags |= kMemoryAccessRead;
            if(info.protection & VM_PROT_WRITE)
                flags |= kMemoryAccessWrite;
            if(info.protection & VM_PROT_EXECUTE)
                flags |= kMemoryAccessExecute;
        }

    #elif defined(EA_PLATFORM_LINUX)
        // http://stackoverflow.com/questions/2152958/reading-memory-protection
        // http://stackoverflow.com/questions/9198385/on-os-x-how-do-you-find-out-the-current-memory-protection-level
        // http://www.kernel.org/doc/man-pages/online/pages/man5/proc.5.html
        //  /proc/[pid]/maps, /proc/[pid]/mem
        // See the EACallstack GetModuleInfoApple function for example code for doing something similar to this.
        EA_UNUSED(p);

    #else
        // To do: Fix this for the given platform.
        EA_UNUSED(p);

    #endif

    return flags;
}



///////////////////////////////////////////////////////////////////////////////
// GetMemoryPageSize
//
// Returns the page size for the given memory, or a suitable base size if 
// unknown for the platform or pointer.
//
static size_t GetMemoryPageSize(const void* p)
{
    #if defined(EA_PLATFORM_XENON)
        // To do: Get the actual page size. There doesn't seem to be a way to do this.
        EA_UNUSED(p);
        const size_t pageSize = 4096; // Actually, the page size is not fixed but can be 4096 or 65536, depending on the page.
        
    #elif defined(EA_PLATFORM_MICROSOFT)
        EA_UNUSED(p);
        #if EA_WINAPI_FAMILY_PARTITION(EA_WINAPI_PARTITION_DESKTOP)
            static size_t pageSize = 0;

            if(pageSize == 0)
            {
                SYSTEM_INFO sysInfo;
                GetSystemInfo(&sysInfo);
                pageSize = sysInfo.dwPageSize;
            }
        #else
            const size_t pageSize = 4096;
        #endif
        
    #elif defined(EA_PLATFORM_PS3)
        size_t          pageSize;
        sys_page_attr_t pageAccess;

        if(sys_memory_get_page_attribute((sys_addr_t)p, &pageAccess) == CELL_OK)
            pageSize = pageAccess.page_size;
        else
            pageSize = 65536;

    #elif defined(EA_PLATFORM_APPLE)
        EA_UNUSED(p);
        const size_t pageSize = getpagesize();

    #elif defined(EA_PLATFORM_UNIX)
        EA_UNUSED(p);
        size_t pageSize;

        #if defined(_SC_PAGESIZE)
            pageSize = sysconf(_SC_PAGESIZE);
        #elif defined(_SC_PAGE_SIZE)
            pageSize = sysconf(_SC_PAGE_SIZE);
        #else
            pageSize = getpagesize();
        #endif
        
    #else
        EA_UNUSED(p);
        const size_t pageSize = 4096;
    #endif
                
    return pageSize;
}


///////////////////////////////////////////////////////////////////////////////
// ReadMemory
//
// This is like memcpy but takes precations to try to handle the case of 
// the source memory being invalid. 
//
static bool ReadMemory(void* pDestination, const void* pSource, size_t size, char replacement)
{
    bool bReturnValue = true;

    #if defined(EA_PLATFORM_APPLE)        
        vm_size_t     dataCount = size;
        kern_return_t result = vm_read_overwrite(mach_task_self(), (uintptr_t)pSource, size, (vm_address_t)pDestination, &dataCount);
        
        if(result != KERN_SUCCESS)
        {
            memset(pDestination, replacement, size);
            return false;
        }

    #elif defined(EA_PLATFORM_WINDOWS) && EA_WINAPI_FAMILY_PARTITION(EA_WINAPI_PARTITION_DESKTOP)
        SIZE_T nBytesRead = 0;
        BOOL   bResult = ReadProcessMemory(GetCurrentProcess(), pSource, pDestination, size, &nBytesRead);

        if(!bResult || (nBytesRead != size))
        {
            memset(pDestination, replacement, size);
            bReturnValue = false;
        }

    #elif defined(EA_PLATFORM_MICROSOFT)
        struct Holder { 
            static LONG ReadMemoryExceptionFilter(_EXCEPTION_POINTERS*)
                { return EXCEPTION_EXECUTE_HANDLER; } // This means run the code in the __except block then continue with the app.
        };

        __try {
            if(size == 1)
                *reinterpret_cast<char*>(pDestination) = *reinterpret_cast<const char*>(pSource);
            else
                memcpy(pDestination, pSource, size);
        }
        __except(Holder::ReadMemoryExceptionFilter(GetExceptionInformation())) {
            memset(pDestination, replacement, size);
            bReturnValue = false;
        }

    //#elif defined(EA_PLATFORM_UNIX)
    //    // http://www.kernel.org/doc/man-pages/online/pages/man5/proc.5.html
    //    // /proc/[pid]/maps, /proc/[pid]/mem. You may need to mount /proc in order for this to work.
    // 
    //    ptrace(PTRACE_ATTACH, pid_self, NULL, NULL)
    //    if ptrace fails, it's probably because a debugger is already attached.
    //    int memfd = open(/proc/[pid]/mem, O_RDONLY);
    //    size = pread(memfd, outbuf, size, address);
    //    close(memfd);
    //    ptrace(PTRACE_DETACH, pid, NULL, 0);

    #else
        // To do: make reading safe for additional platforms.
        EA_UNUSED(replacement);

        if(size == 1)
            *reinterpret_cast<char*>(pDestination) = *reinterpret_cast<const char*>(pSource);
        else
            memcpy(pDestination, pSource, size);
    #endif
    
    return bReturnValue;
}



///////////////////////////////////////////////////////////////////////////////
// WriteMemoryView
//
// This function writes data like this:
//     0012f840 | 00 00 00 00 00 00 00 00<68>ff 12 00 c0 fc 12 00 |         h....... |
//     0012f850 | 00 00 00 00 cc cc cc cc cc cc cc cc cc cc cc cc | ................ |
//     0012f860 | 00 00 00 00 cc cc cc cc 00 00 00 00 00 00 00 00 | ................ |
//     ...
// The pMark parameter is where we should write <> around a byte.
//
bool ReportWriter::WriteMemoryView(const uint8_t* pMemory, size_t count, const uint8_t* pMark, const char* pFirstLineHeader)
{    
    intptr_t memory = reinterpret_cast<intptr_t>(pMemory);

    if((memory >= 65536) || (memory < -65536)) // if the memory looks like a pointer (we assume values like 100 or 10000 aren't valid pointers on modern platforms).
    {
        const uintptr_t kAllBits = (uintptr_t)-1;

        if(pMemory > (const uint8_t*)(kAllBits - (uintptr_t)count)) // If (pMemory + count) would overflow...
           pMemory = (const uint8_t*)(kAllBits - (uintptr_t)count); // Move pMemory back by some bytes.

        #ifdef _MSC_VER
            __try
            {
        #endif
                size_t         lineCount = 0;
                size_t         headerSize = pFirstLineHeader ? EA::StdC::Strlen(pFirstLineHeader) : 0;
                const size_t   kAddressLength = (2 + (2 * EA_PLATFORM_PTR_SIZE)); // 0x12345678 or 0x1122334455667788
                const size_t   kLineBufferLength = kAddressLength + ((3 * 16) + 3 + 16 + 5) + 32; // +32 for slop.
                char           lineBuffer[kLineBufferLength];
                const uint8_t* pStart = (const uint8_t*)((uintptr_t)pMemory & (kAllBits & (uintptr_t)~15)); // Align down 16 bytes.
                size_t         pageSize = GetMemoryPageSize(pStart); // pageSize is always a power of 2.
                bool           bPageReadable = ((GetMemoryAccess(pStart) & kMemoryAccessRead) != 0);
                
                while((pStart < (pMemory + count)) && (lineCount < (count * 32))) // The lineCount check is merely a sanity check to make sure we don't have infinite loop bugs below.
                {
                    memset(lineBuffer, 0, sizeof(lineBuffer));

                    // We assume that the page size is divisible by 16, which it will in fact be. 
                    #if (EA_PLATFORM_PTR_SIZE == 8)
                        EA::StdC::Sprintf(lineBuffer, "%016" PRIxPTR " |", (uintptr_t)pStart);
                    #else
                        EA::StdC::Sprintf(lineBuffer, "%08x |", (unsigned int)(uintptr_t)pStart);
                    #endif

                    for(const uint8_t* p = pStart; p < (pStart + 16); p++)
                    {
                        if(((uintptr_t)p & (pageSize - 1)) == 0) // If starting a new page... reassess the memory readability.
                        {
                            pageSize      = GetMemoryPageSize(pStart);
                            bPageReadable = ((GetMemoryAccess(p) & kMemoryAccessRead) != 0);
                        }

                        // Note that sprintf starts on the NUL written by the previous sprintf
                        if((p < pMemory) || (p >= (pMemory + count)))
                        {
                            EA::StdC::Sprintf(lineBuffer + kAddressLength + (3 * (p - pStart)), "    ");
                            lineBuffer[kAddressLength + (3 * 16) + 3 + (p - pStart)] = ' ';
                        }
                        else
                        {
                            const char mark = (p == pMark) ? '<' : (p == pMark + 1) ? '>' : ' ';
                            uint8_t c = 0;
                            bool    bReadable = (bPageReadable && ReadMemory(&c, p, 1, '?'));
                                                        
                            if(bReadable)
                            {
                                bool bShouldPrintChar = (c >= 32) && (c < 127);
                                EA::StdC::Sprintf(lineBuffer + kAddressLength + (3 * (p - pStart)), "%c%02x", mark, (unsigned)c);
                                lineBuffer[kAddressLength + (3 * 16) + 3 + (p - pStart)] = bShouldPrintChar ? (char)c : '.';
                            }
                            else
                            {
                                EA::StdC::Sprintf(lineBuffer + kAddressLength + (3 * (p - pStart)), "%c??", mark);
                                lineBuffer[kAddressLength + (3 * 16) + 3 + (p - pStart)] = '?';
                            }
                        }
                    } // for loop

                    memcpy(lineBuffer + kAddressLength + (3 * 16), " | ", 3); // this will also overwrite NUL
                    memcpy(lineBuffer + kAddressLength + (3 * 16) + 3 + 16, " |\r\n", 5);

                    if(pFirstLineHeader)
                    {
                        if(lineCount == 0)
                            WriteText(pFirstLineHeader, false);
                        else
                        {
                            for(size_t i = 0; i < headerSize; i++)
                                Write(" ", 1);
                        }
                    }

                    WriteText(lineBuffer, false);

                    if(EA::StdC::UnsignedAdditionWouldOverflow<uintptr_t>((uintptr_t)pStart, 16)) // If there was integer wraparound...
                    {
                        WriteText("\r\n", false);
                        break;
                    }

                    pStart += 16;
                    lineCount++;
                } // while loop.

        #ifdef _MSC_VER
            }
            __except(EXCEPTION_EXECUTE_HANDLER)
            {
                WriteText("Memory view write failed due to an exception.\r\n", false);
            }
        #endif
    }
    else
    {
        if(pFirstLineHeader)
            WriteText(pFirstLineHeader, false);

        #if (EA_PLATFORM_PTR_SIZE == 8)
            WriteFormatted("%016" PRIxPTR " | Address appears to be invalid.\r\n", memory);
        #else
            WriteFormatted("%08x | Address appears to be invalid.\r\n", (unsigned int)memory);
        #endif
    }
    
    return true;
}


///////////////////////////////////////////////////////////////////////////////
// WriteStackMemoryView
//
// This function writes data like this:
//     0012f840 | 00 00 00 00 00 00 00 00<68>ff 12 00 c0 fc 12 00 |         h....... |
//     0012f850 | 00 00 00 00 cc cc cc cc cc cc cc cc cc cc cc cc | ................ |
//     0012f860 | 00 00 00 00 cc cc cc cc 00 00 00 00 00 00 00 00 | ................ |
//     ...
//
#if (EACALLSTACK_VERSION_N < 11206)
    #define GetStackLimit GetStackTop
    #define GetStackBase  GetStackBottom
#endif

bool ReportWriter::WriteStackMemoryView(const uint8_t* pStackPointer, size_t count)
{
    #if defined(EA_PLATFORM_MICROSOFT) // If running on a platform that executes exception handling in the same thread as the exception...
        // In using the pointers below, we make the assumption that the given exception 
        // context refers to the same thread as this exception handler. Under Win32, when you have
        // and exception handler installed, it executes in the same thread as the thread that 
        // generated the exception.
        const uint8_t* pOSReportedStackBase = (const uint8_t*)(intptr_t)EA::Callstack::GetStackBase();
        const uint8_t* pStackLowMemory      = (const uint8_t*)(intptr_t)EA::Callstack::GetStackLimit();
        const uint8_t* pStackHighMemory     = pOSReportedStackBase ? pOSReportedStackBase : (const uint8_t*)(intptr_t)EA::Callstack::GetStackBase();

        if(pStackLowMemory > pStackHighMemory) // If this platform grows the stack upwards (uncommon)...
        {
            const uint8_t* pTemp = pStackLowMemory;
            pStackLowMemory = pStackHighMemory;
            pStackHighMemory = pTemp;
        }

        if(pStackHighMemory == 0) // If it was unreadable... make it be something close to pStackPointer.
            pStackHighMemory = (pStackPointer + 1024);

        if(pStackLowMemory == 0) // If it was unreadable... make it be something close to pStackPointer.
            pStackLowMemory = (pStackPointer - 1024);
        else
            pStackLowMemory -= 16384; // Give it some leeway because we have a range check below.

        const uint8_t* const pStackReportBegin = AlignPointerDown(eastl::max_alt(pStackLowMemory,  pStackPointer        ), kMemoryViewAlignment);
        const uint8_t* const pStackReportEnd   = AlignPointerUp  (eastl::min_alt(pStackHighMemory, pStackPointer + count), kMemoryViewAlignment);

        if((pStackPointer >= pStackLowMemory) && (pStackPointer < pStackHighMemory) && (pStackReportBegin < pStackReportEnd))
            WriteMemoryView(pStackReportBegin, (size_t)(pStackReportEnd - pStackReportBegin), pStackPointer, NULL);
        else
            WriteFormatted("Stack pointer (%p) appears to point to invalid memory or some other thread's memory.\r\n", pStackPointer);
    #else
        // To do: Make the EACallstack GetStackBase/GetStackLimit/GetStackCurrent functions take a threadId parameter.
        const uint8_t* const pStackReportBegin = AlignPointerDown(pStackPointer - count, kMemoryViewAlignment);
        const uint8_t* const pStackReportEnd   = AlignPointerUp  (pStackPointer + count, kMemoryViewAlignment);

        WriteMemoryView(pStackReportBegin, (size_t)(pStackReportEnd - pStackReportBegin), pStackPointer, NULL);
    #endif

    return true;
}


///////////////////////////////////////////////////////////////////////////////
// WriteModuleList
//
// We write something that looks like this (Windows):
//     [Modules]
//     base 0x00400000 size 0x00097000 entry 0x004663d0 Blah.exe   c:\Projects\Blah.exe
//     base 0x7c900000 size 0x000b0000 entry 0x7c913156 Blah1.dll  c:\Projects\Blah1.dll  
//     base 0x7c800000 size 0x000f4000 entry 0x7c80b436 Blah2.dll  c:\Projects\Blah2.dll
//
// We write something that looks like this (Xenon):
//     [Modules]
//     base 0x82000000 size 0x00097000 entry 0x820063d0 Blah.xex
//     base 0x84000000 size 0x000b0000 entry 0x84003156 Blah1.dll
//     base 0x86000000 size 0x000f4000 entry 0x8600b436 Blah2.dll
//
// We write something that looks like this (PS3):
//     [Modules]
//     base 0x10050000 size 0x00010430 entry 0x10050000 liblv2                /dev_flash/sys/external/liblv2.sprx
//     base 0x10080000 size 0x00002dc0 entry 0x10080000 cellSysmodule_Library /dev_flash/sys/external/libsysmodule.sprx
//     base 0x100a0000 size 0x0001b6b0 entry 0x100a0000 cellSysutil_Library   /dev_flash/sys/external/libsysutil.sprx
//     base 0x100d0000 size 0x0000b0b8 entry 0x100d0000 cellGcm_Library       /dev_flash/sys/external/libgcm_sys.sprx
//     base 0x100f0000 size 0x000019dc entry 0x100f0000 cellAudio_Library     /dev_flash/sys/external/libaudio.sprx
//
// We write something that looks like this (Mac OS X 64):
//     [Modules]
//     type __TEXT base 0x00000001094aa000 size 0x424000 /bin/Blah.bin
//     type __TEXT base 0x00007fff90057000 size 0x20200  /usr/lib/system/libmathCommon.A.dylib
//
bool ReportWriter::WriteModuleList()
{
    #if defined(EA_PLATFORM_MICROSOFT) && EA_WINAPI_FAMILY_PARTITION(EA_WINAPI_PARTITION_DESKTOP)
        HMODULE hPSAPI = LoadLibraryA("psapi.dll");

        if(!hPSAPI)
        {
            WriteText("PSAPI is not available. Cannot dump module list.\r\n", false);
            return false;
        }

        // PSAPI functions (note: non-Unicode version)
        typedef BOOL  (__stdcall *ENUMPROCESSMODULES)  (HANDLE hProcess, HMODULE* phModule, DWORD cb, LPDWORD lpcbNeeded);
        typedef DWORD (__stdcall *GETMODULEFILENAMEEX) (HANDLE hProcess, HMODULE   hModule, LPSTR lpFilename, DWORD nSize);
        typedef DWORD (__stdcall *GETMODULEBASENAME)   (HANDLE hProcess, HMODULE   hModule, LPSTR lpFilename, DWORD nSize);
        typedef BOOL  (__stdcall *GETMODULEINFORMATION)(HANDLE hProcess, HMODULE   hModule, MODULEINFO* pmi, DWORD nSize);

        ENUMPROCESSMODULES   pEnumProcessModules   = (ENUMPROCESSMODULES)  (void*)GetProcAddress(hPSAPI, "EnumProcessModules");
        GETMODULEFILENAMEEX  pGetModuleFileNameEx  = (GETMODULEFILENAMEEX) (void*)GetProcAddress(hPSAPI, "GetModuleFileNameExA");
        GETMODULEBASENAME    pGetModuleBaseName    = (GETMODULEBASENAME)   (void*)GetProcAddress(hPSAPI, "GetModuleBaseNameA");
        GETMODULEINFORMATION pGetModuleInformation = (GETMODULEINFORMATION)(void*)GetProcAddress(hPSAPI, "GetModuleInformation");

        // We'll do some creative formatting on this one. The first 49 (32 bit) or 118 (64 bit) bytes is for
        // base/size/entry information, then base name (usually much shorter than MAX_PATH),
        // then one space, and then full file name.
        char         pBaseName[MAX_PATH + 2];
        char         pFullName[MAX_PATH + 2];
        char         pReportLine[128 + sizeof(pBaseName) + sizeof(pFullName)];
        DWORD        cbNeeded;
        HANDLE       hProcess = GetCurrentProcess();
        const size_t kModuleCountMax = 256; // This value is used below as 256, so if you modify this, modify below.
        HMODULE      hModuleBuffer[kModuleCountMax];

        if(pEnumProcessModules(hProcess, hModuleBuffer, sizeof(hModuleBuffer), &cbNeeded))
        {
            size_t moduleCount = cbNeeded / sizeof(HMODULE);

            if(moduleCount > kModuleCountMax)
            {
                WriteText("Module count exceeds our enumeration limit of 256. Only 256 will be displayed.\r\n", false);
                moduleCount = kModuleCountMax;
            }

            // And go through the list one by one
            for(size_t i = 0; i < moduleCount; i++)
            {
                MODULEINFO mi;

                if(!pGetModuleInformation(hProcess, hModuleBuffer[i], &mi, sizeof(mi)))
                {
                    mi.EntryPoint  = NULL;
                    mi.lpBaseOfDll = NULL;
                    mi.SizeOfImage = 0;
                }

                pBaseName[0] = '"';
                if(!pGetModuleBaseName(hProcess, hModuleBuffer[i], pBaseName + 1, MAX_PATH))
                    strcpy(pBaseName + 1, "<unknown>");
                pBaseName[EA::StdC::Strlen(pBaseName) + 1] = '\0';
                pBaseName[EA::StdC::Strlen(pBaseName)]     = '"';

                pFullName[0] = '"';
                if(!pGetModuleFileNameEx(hProcess, hModuleBuffer[i], pFullName + 1, MAX_PATH))
                    strcpy(pFullName + 1, "<unknown>");
                pFullName[EA::StdC::Strlen(pFullName) + 1] = '\0';
                pFullName[EA::StdC::Strlen(pFullName)]     = '"';

                #if (EA_PLATFORM_PTR_SIZE >= 8)
                    EA::StdC::Snprintf(pReportLine, EAArrayCount(pReportLine), "base 0x%016I64x size 0x%016I64x entry 0x%08x %-48s %s",
                            (uint64_t)(uintptr_t)mi.lpBaseOfDll, (uint64_t)mi.SizeOfImage, (uint64_t)(uintptr_t)mi.EntryPoint, pBaseName, pFullName);
                #else
                    EA::StdC::Snprintf(pReportLine, EAArrayCount(pReportLine), "base 0x%08x size 0x%08x entry 0x%08x %-48s %s",
                            (uint32_t)((uintptr_t)mi.lpBaseOfDll & 0xffffffff), (uint32_t)mi.SizeOfImage, (uint32_t)((uintptr_t)mi.EntryPoint & 0xffffffff),pBaseName, pFullName);
                #endif

                WriteText(pReportLine, true);
            }
        }
        else
            WriteText("Failed to enumerate modules.\r\n", false);

    #elif defined(EA_PLATFORM_XENON)
        #if EA_XBDM_ENABLED
            DMN_MODLOAD      ModLoad;
            PDM_WALK_MODULES pWalkMod = NULL;

            // We write out the data here in a format compatible with the Win32 version,
            // in order to have a consistent output. This results in redundant data below.
            while(DmWalkLoadedModules(&pWalkMod, &ModLoad) == XBDM_NOERR)
            {
                char pQuotedModuleName[MAX_PATH + 2];

                pQuotedModuleName[0] = '"';
                strcpy(pQuotedModuleName + 1, ModLoad.Name);
                pQuotedModuleName[EA::StdC::Strlen(pQuotedModuleName) + 1] = '\0';
                pQuotedModuleName[EA::StdC::Strlen(pQuotedModuleName)]     = '"';

                WriteFormatted("base 0x%08x size 0x%08x entry 0x%08x %-48s %s\r\n", 
                                (uint32_t)ModLoad.BaseAddress, (uint32_t)ModLoad.Size, 
                                (uint32_t)ModLoad.BaseAddress, pQuotedModuleName, pQuotedModuleName);
            }

            DmCloseLoadedModules(pWalkMod);
        #else
            WriteText("Failed to enumerate modules. Xbdm unavailable.\r\n", false);
        #endif

    #elif defined(EA_PLATFORM_PS3)

        // Currently we have a way to get the loaded PRX modules (PS3 equivalent to DLLs) but not
        // the loaded primary process module. 

        sys_prx_get_module_list_t moduleList;
        sys_prx_id_t              idArray[128];

        memset(&moduleList, 0, sizeof(moduleList));
        moduleList.size   = sizeof(moduleList);
        moduleList.max    = sizeof(idArray) / sizeof(idArray[0]);
        moduleList.idlist = idArray;

        int result = sys_prx_get_module_list(0, &moduleList);

        if(result == CELL_OK)
        {
            for(uint32_t i = 0; i < moduleList.count; i++)
            {
                char                   fileName[256];
                sys_prx_segment_info_t segmentInfoArray[8];
                sys_prx_module_info_t  info;

                memset(&fileName, 0, sizeof(fileName));
                memset(&segmentInfoArray, 0, sizeof(segmentInfoArray));
                memset(&info, 0, sizeof(info));

                info.size          = sizeof(info);
                info.filename      = fileName;
                info.filename_size = sizeof(fileName);
                info.segments      = segmentInfoArray;
                info.segments_num  = sizeof(segmentInfoArray) / sizeof(segmentInfoArray[0]);

                result = sys_prx_get_module_info(moduleList.idlist[i], 0, &info);

                if(result == CELL_OK)
                {
                    // Currently we assume the module has just a single segment.
                    WriteFormatted("base 0x%08x size 0x%08x entry 0x%08x %s %s\r\n", 
                                    (uint32_t)segmentInfoArray[0].base, segmentInfoArray[0].memsz, 
                                    (uint32_t)segmentInfoArray[0].base, info.name, info.filename);
                }
            }
        }

        if(result != 0) // If any of the above failed...
            WriteText("Unable to display module (PRX) list on the PS3.\r\n", false);

    #elif defined(EA_PLATFORM_OSX)
        EA::Callstack::ModuleInfoAppleArray moduleInfoAppleArray;
        size_t count = EA::Callstack::GetModuleInfoApple(moduleInfoAppleArray, NULL);
        
        for(size_t i = 0; i < count; i++)
        {
            const EA::Callstack::ModuleInfoApple& info = moduleInfoAppleArray[(eastl_size_t)i];
            
            WriteFormatted("type: %s base: 0x%016I64x size: 0x%016I64x permissions: %s module: %s\r\n", info.mType, info.mBaseAddress, info.mSize, info.mPermissions, info.mPath.c_str());
        }
        
        return count;
    
    #elif defined(EA_PLATFORM_LINUX)
        // http://stackoverflow.com/questions/2152958/reading-memory-protection
        // http://stackoverflow.com/questions/9198385/on-os-x-how-do-you-find-out-the-current-memory-protection-level
    
    #else
        // To do: This is possible for other platforms, such as Wii, WiiU, and others.

    #endif

    return true;
}


///////////////////////////////////////////////////////////////////////////////
// ReportWriteRegisterValues
//
// We write a section like this (x86):
//     eax: 00000001
//     ebx: 7ffdf000
//     ecx: 00000000
//     edx: 00000000
//
// or like this(PowerPC):
//     r0: 00000001
//     r1: 7ffdf000
//     r2: 00000000
//     r3: 00000000
//     r4: 0012faac
//
bool ReportWriter::WriteRegisterValues(const void* pPlatformContext)
{
    #ifdef EA_PROCESSOR_X86
        const Callstack::ContextX86* const pContext = (const Callstack::ContextX86*)pPlatformContext;

        EA::StdC::Sprintf(mScratchBuffer, "%08x", (unsigned)pContext->Eip);       WriteKeyValue("eip", mScratchBuffer);
        EA::StdC::Sprintf(mScratchBuffer, "%08x", (unsigned)pContext->Eax);       WriteKeyValue("eax", mScratchBuffer);
        EA::StdC::Sprintf(mScratchBuffer, "%08x", (unsigned)pContext->Ebx);       WriteKeyValue("ebx", mScratchBuffer);
        EA::StdC::Sprintf(mScratchBuffer, "%08x", (unsigned)pContext->Ecx);       WriteKeyValue("ecx", mScratchBuffer);
        EA::StdC::Sprintf(mScratchBuffer, "%08x", (unsigned)pContext->Edx);       WriteKeyValue("edx", mScratchBuffer);
        EA::StdC::Sprintf(mScratchBuffer, "%08x", (unsigned)pContext->Esi);       WriteKeyValue("esi", mScratchBuffer);
        EA::StdC::Sprintf(mScratchBuffer, "%08x", (unsigned)pContext->Edi);       WriteKeyValue("edi", mScratchBuffer);
        EA::StdC::Sprintf(mScratchBuffer, "%08x", (unsigned)pContext->Ebp);       WriteKeyValue("ebp", mScratchBuffer);
        EA::StdC::Sprintf(mScratchBuffer, "%08x", (unsigned)pContext->EFlags);    WriteKeyValue("efl", mScratchBuffer);
        EA::StdC::Sprintf(mScratchBuffer, "%08x", (unsigned)pContext->Esp);       WriteKeyValue("esp", mScratchBuffer);

    #elif defined(EA_PROCESSOR_X86_64)
        const Callstack::ContextX86_64* const pContext = (const Callstack::ContextX86_64*)pPlatformContext;

        EA::StdC::Sprintf(mScratchBuffer, "%016" PRIx64, pContext->Rax);       WriteKeyValue("rax", mScratchBuffer);
        EA::StdC::Sprintf(mScratchBuffer, "%016" PRIx64, pContext->Rbx);       WriteKeyValue("rbx", mScratchBuffer);
        EA::StdC::Sprintf(mScratchBuffer, "%016" PRIx64, pContext->Rcx);       WriteKeyValue("rcx", mScratchBuffer);
        EA::StdC::Sprintf(mScratchBuffer, "%016" PRIx64, pContext->Rdx);       WriteKeyValue("rdx", mScratchBuffer);
        EA::StdC::Sprintf(mScratchBuffer, "%016" PRIx64, pContext->Rsi);       WriteKeyValue("rsi", mScratchBuffer);
        EA::StdC::Sprintf(mScratchBuffer, "%016" PRIx64, pContext->Rdi);       WriteKeyValue("rdi", mScratchBuffer);
        EA::StdC::Sprintf(mScratchBuffer, "%016" PRIx64, pContext->Rsp);       WriteKeyValue("rsp", mScratchBuffer);
        EA::StdC::Sprintf(mScratchBuffer, "%016" PRIx64, pContext->Rbp);       WriteKeyValue("rbp", mScratchBuffer);
        EA::StdC::Sprintf(mScratchBuffer, "%016" PRIx64, pContext->R8);        WriteKeyValue("r8 ", mScratchBuffer);
        EA::StdC::Sprintf(mScratchBuffer, "%016" PRIx64, pContext->R9);        WriteKeyValue("r9 ", mScratchBuffer);
        EA::StdC::Sprintf(mScratchBuffer, "%016" PRIx64, pContext->R10);       WriteKeyValue("r10", mScratchBuffer);
        EA::StdC::Sprintf(mScratchBuffer, "%016" PRIx64, pContext->R11);       WriteKeyValue("r11", mScratchBuffer);
        EA::StdC::Sprintf(mScratchBuffer, "%016" PRIx64, pContext->R12);       WriteKeyValue("r12", mScratchBuffer);
        EA::StdC::Sprintf(mScratchBuffer, "%016" PRIx64, pContext->R13);       WriteKeyValue("r13", mScratchBuffer);
        EA::StdC::Sprintf(mScratchBuffer, "%016" PRIx64, pContext->R14);       WriteKeyValue("r14", mScratchBuffer);
        EA::StdC::Sprintf(mScratchBuffer, "%016" PRIx64, pContext->R15);       WriteKeyValue("r15", mScratchBuffer);

    #elif defined(EA_PROCESSOR_POWERPC)
        const Callstack::Context* const pContext = (const Callstack::Context*)pPlatformContext;

        EA::StdC::Sprintf(mScratchBuffer, "%08x", (unsigned)pContext->mIar);
        WriteKeyValue("iar", mScratchBuffer);

        //EA::StdC::Sprintf(mScratchBuffer, "%08x", (unsigned)pContext->mMsr); // Doesn't exist in PS3 context. Find a way to deal with this.
        //WriteKeyValue("msr", mScratchBuffer);

        EA::StdC::Sprintf(mScratchBuffer, "%08x", (unsigned)pContext->mLr);
        WriteKeyValue("lr", mScratchBuffer);

        #if (EA_PLATFORM_WORD_SIZE == 8)
            EA::StdC::Sprintf(mScratchBuffer, "%016" PRIx64, pContext->mCtr);
        #else
            EA::StdC::Sprintf(mScratchBuffer, "%08x", (unsigned)pContext->mCtr);
        #endif
        WriteKeyValue("ctr", mScratchBuffer);

        for(int i = 0; i < 32; i++)
        {
            char name[32];
            EA::StdC::Sprintf(name, "r%d", i);
            EA::StdC::Sprintf(mScratchBuffer, "%016" PRIx64, pContext->mGpr[i]);
            WriteKeyValue(name, mScratchBuffer);
        }

        // Floating point registers
        #if defined(EA_PLATFORM_XENON)
            EA::StdC::Sprintf(mScratchBuffer, "%016" PRIx64, *(uint64_t*)&pContext->mFpscr); // Microsoft declares this flags register as a double.
        #else
            EA::StdC::Sprintf(mScratchBuffer, "%08x", pContext->mFpscr);
        #endif
        WriteKeyValue("fpscr", mScratchBuffer);

        for(int i = 0; i < 32; i++)
        {
            char name[32];
            EA::StdC::Sprintf(name, "f%d", i);
            EA::StdC::Sprintf(mScratchBuffer, "%.16f", pContext->mFpr[i]);
            WriteKeyValue(name, mScratchBuffer);
        }

        // To do: VMX Registers

    #elif defined(EA_PROCESSOR_ARM) && (!(EA_PLATFORM_PTR_SIZE > 4) && !(EA_PLATFORM_WORD_SIZE > 4)) // 32 bit ARM

        const Callstack::ContextARM* const pContext = (const Callstack::ContextARM*)pPlatformContext;

        for(int i = 0; i < 16; i++)
        {
            char name[8];
            EA::StdC::Sprintf(name, "r%d", i);
            EA::StdC::Sprintf(mScratchBuffer, "%08x", (unsigned)pContext->mGpr[i]);
            WriteKeyValue(name, mScratchBuffer);
        }

        // To do: The rest of the registers.

    #else
        EA_UNUSED(pPlatformContext);
        WriteText("Failed to list registers. Platform unsupported.\r\n", false);
    #endif

    return true;
}


///////////////////////////////////////////////////////////////////////////////
// WriteRegisterMemoryView
//
// We write something that looks like this (PowerPC):
//     r0  8207b638 | 82 07 b6 cc<39>60 00 00 91 61 04 64 0f e0 00 16 | ....9`...a.d.... |
//     r1  7004f22c | 00 00 00 00<70>04 f6 c0 80 00 00 00 00 00 00 0e | ....p........... |
//     r2  7004f28d | cc cc cc 00<00>00 00 00 00 00 00 00 00 00 00 00 | ................ |
//     r3  000003f8 | ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? | ???????????????? |
//     r4  7004f68c | 00 00 00 00<cc>cc cc cc 00 00 00 00 cc cc cc cc | ................ |
//     r5  0000b02c | ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? | ???????????????? |
//     r6  3001101c | 00 10 6f 50<06>00 00 00 00 00 00 00 30 01 10 28 | ..oP........0..( |
//     r7  ccccccc8 | ff ff ff ff<ff>ff ff ff ff ff ff ff ff ff ff ff | ................ |
//     r8  8207b624 | 4e 80 04 20<82>07 b6 3c 82 07 b6 5c 82 07 b6 60 | N.. ...<...\...` |
//
// We write something that looks like this (x86):
//     ebx 7ffdeffc | 00 00 00 00<00>00 01 00 ff ff ff ff 00 00 40 00 | ..............@. |
//         7ffdf00c | a0 1e 24 00 00 00 02 00 00 00 00 00 00 00 14 00 | ..$............. |
//     esi 0012faa8 | 00 00 00 00<68>ff 12 00 0c fb 12 00 00 f0 fd 7f | ....h........... |
//         0012fab8 | cc cc cc cc 05 00 00 c0 cc cc cc cc 58 ee a8 00 | ............X... |
//     edi 0012fa50 | 00 00 00 00<cc>cc cc cc 79 e6 b4 d5 90 fa 12 00 | ........y....... |
//         0012fa60 | fc db 40 00 01 00 00 00 68 ff 12 00 ac fa 12 00 | ..@.....h....... |
//     ebp 0012fa58 | 79 e6 b4 d5<90>fa 12 00 fc db 40 00 01 00 00 00 | y.........@..... |
//         0012fa68 | 68 ff 12 00 ac fa 12 00 00 f0 fd 7f cc cc 00 01 | h............... |
//
bool ReportWriter::WriteRegisterMemoryView(const void* pPlatformContext)
{
    if(pPlatformContext)
    {
        #if defined(EA_PROCESSOR_X86)
            const Callstack::ContextX86* const pContext = (const Callstack::ContextX86*)pPlatformContext;

            const uint8_t* const Eax = (uint8_t*)(uintptr_t)pContext->Eax;  WriteMemoryView(Eax, 32, Eax, "eax ");
            const uint8_t* const Ebx = (uint8_t*)(uintptr_t)pContext->Ebx;  WriteMemoryView(Ebx, 32, Ebx, "ebx ");
            const uint8_t* const Ecx = (uint8_t*)(uintptr_t)pContext->Ecx;  WriteMemoryView(Ecx, 32, Ecx, "ecx ");
            const uint8_t* const Edx = (uint8_t*)(uintptr_t)pContext->Edx;  WriteMemoryView(Edx, 32, Edx, "edx ");
            const uint8_t* const Esi = (uint8_t*)(uintptr_t)pContext->Esi;  WriteMemoryView(Esi, 32, Esi, "esi ");
            const uint8_t* const Edi = (uint8_t*)(uintptr_t)pContext->Edi;  WriteMemoryView(Edi, 32, Edi, "edi ");
            const uint8_t* const Esp = (uint8_t*)(uintptr_t)pContext->Esp;  WriteMemoryView(Esp, 32, Esp, "esp ");
            const uint8_t* const Ebp = (uint8_t*)(uintptr_t)pContext->Ebp;  WriteMemoryView(Ebp, 32, Ebp, "ebp ");

            return true;

        #elif defined(EA_PROCESSOR_X86_64)
            const Callstack::ContextX86_64* const pContext = (const Callstack::ContextX86_64*)pPlatformContext;

            const uint8_t* const Rax = (uint8_t*)(uintptr_t)pContext->Rax;  WriteMemoryView(Rax, 32, Rax, "rax ");
            const uint8_t* const Rbx = (uint8_t*)(uintptr_t)pContext->Rbx;  WriteMemoryView(Rbx, 32, Rbx, "rbx ");
            const uint8_t* const Rcx = (uint8_t*)(uintptr_t)pContext->Rcx;  WriteMemoryView(Rcx, 32, Rcx, "rcx ");
            const uint8_t* const Rdx = (uint8_t*)(uintptr_t)pContext->Rdx;  WriteMemoryView(Rdx, 32, Rdx, "rdx ");
            const uint8_t* const Rsi = (uint8_t*)(uintptr_t)pContext->Rsi;  WriteMemoryView(Rsi, 32, Rsi, "rsi ");
            const uint8_t* const Rdi = (uint8_t*)(uintptr_t)pContext->Rdi;  WriteMemoryView(Rdi, 32, Rdi, "rdi ");
            const uint8_t* const Rsp = (uint8_t*)(uintptr_t)pContext->Rsp;  WriteMemoryView(Rsp, 32, Rsp, "rsp ");
            const uint8_t* const Rbp = (uint8_t*)(uintptr_t)pContext->Rbp;  WriteMemoryView(Rbp, 32, Rbp, "rbp ");
            const uint8_t* const R8  = (uint8_t*)(uintptr_t)pContext->R8;   WriteMemoryView(R8,  32, R8,  "r8  ");
            const uint8_t* const R9  = (uint8_t*)(uintptr_t)pContext->R9;   WriteMemoryView(R9,  32, R8,  "r9  ");
            const uint8_t* const R10 = (uint8_t*)(uintptr_t)pContext->R10;  WriteMemoryView(R10, 32, R10, "r10 ");
            const uint8_t* const R11 = (uint8_t*)(uintptr_t)pContext->R11;  WriteMemoryView(R11, 32, R11, "r11 ");
            const uint8_t* const R12 = (uint8_t*)(uintptr_t)pContext->R12;  WriteMemoryView(R12, 32, R12, "r12 ");
            const uint8_t* const R13 = (uint8_t*)(uintptr_t)pContext->R13;  WriteMemoryView(R13, 32, R13, "r13 ");
            const uint8_t* const R14 = (uint8_t*)(uintptr_t)pContext->R14;  WriteMemoryView(R14, 32, R14, "r14 ");
            const uint8_t* const R15 = (uint8_t*)(uintptr_t)pContext->R15;  WriteMemoryView(R15, 32, R15, "r15 ");

            return true;

        #elif defined(EA_PROCESSOR_POWERPC)
            // There are multiple PowerPC-related Context defintions, such as ContextPowerPC32, ContextPowerPC64, ContextPowerXenon, ContextPowerPS3, etc.
            // We use the generic version so that we can use the 32 and 64 bit versions with the same code.
            const Callstack::Context* const pContext = (const Callstack::Context*)pPlatformContext;

            const uint8_t* const r0  = (uint8_t*)(uintptr_t)pContext->mGpr[0];  WriteMemoryView(r0, 32, r0, "r0  ");
            // r1 is the stack register
            const uint8_t* const r2  = (uint8_t*)(uintptr_t)pContext->mGpr[2];  WriteMemoryView(r2, 32, r2, "r2  ");
            const uint8_t* const r3  = (uint8_t*)(uintptr_t)pContext->mGpr[3];  WriteMemoryView(r3, 32, r3, "r3  ");
            const uint8_t* const r4  = (uint8_t*)(uintptr_t)pContext->mGpr[4];  WriteMemoryView(r4, 32, r4, "r4  ");
            const uint8_t* const r5  = (uint8_t*)(uintptr_t)pContext->mGpr[5];  WriteMemoryView(r5, 32, r5, "r5  ");
            const uint8_t* const r6  = (uint8_t*)(uintptr_t)pContext->mGpr[6];  WriteMemoryView(r6, 32, r6, "r6  ");
            const uint8_t* const r7  = (uint8_t*)(uintptr_t)pContext->mGpr[7];  WriteMemoryView(r7, 32, r7, "r7  ");
            const uint8_t* const r8  = (uint8_t*)(uintptr_t)pContext->mGpr[8];  WriteMemoryView(r8, 32, r8, "r8  ");
            // Is there much benefit to showing r9-r31?
            const uint8_t* const r31 = (uint8_t*)(uintptr_t)pContext->mGpr[31]; WriteMemoryView(r31, 32, r31, "r31 ");

            return true;

        #elif defined(EA_PROCESSOR_ARM)
            const Callstack::ContextARM* const pContext = (const Callstack::Context*)pPlatformContext;

            for(int i = 0; i < 16; i++)
            {
                const uint8_t* const r = (uint8_t*)(uintptr_t)pContext->mGpr[i];
                EA::StdC::Sprintf(mScratchBuffer, "r%-4d", i);
                WriteMemoryView(r, 32, r, mScratchBuffer);
            }

        #else
            WriteText("Failed to display register memory. Platform unsupported.\r\n", false);

        #endif
    }
    else
        WriteText("Unable to display register memory; no thread context is available.\r\n", false);

    return false;
}


///////////////////////////////////////////////////////////////////////////////
// WriteCallStack
//
// This function writes data like this:
//     c:\Projects\ExceptionText\ExceptionTest.exe
//     0x0040cd6a
//     EA::Debug::Internal::CreateExceptionImpl()+0x0000007a
//     exceptionhandler.cpp: line 82
//
//     0x0040cf4c
//     EA::Debug::CreateException()+0x0000005c
//     exceptionhandler.cpp: line 176
//
//     0x00400f8c
//     TestAccessViolation()+0x0000000c
//     exceptionhandlertest.cpp: line 18
//
// To do: We need to change this code to remove the Context* argument and provide instead a callstack[] argument
// which the caller gets from the exception handler itself. The exception handler should be recording the callstack
// itself and not us, as it turns out that on some platforms and situations only the exception handler can really 
// know how to best read the exception thread's callstack. 
//
bool ReportWriter::WriteCallStack(const EA::Callstack::Context* pContext, int addressRepTypeFlags, bool bAbbreviated)
{
    using namespace EA::Callstack;

    EA::Callstack::CallstackContext ctx;
    EA::Callstack::GetCallstackContext(ctx, pContext);

    return WriteCallStackFromCallstackContext(&ctx, addressRepTypeFlags, bAbbreviated);
}


bool ReportWriter::WriteCallStackFromCallstackContext(const EA::Callstack::CallstackContext* pCallstackContext, int addressRepTypeFlags, bool bAbbreviated)
{
    // To do / work in progress:
    // We are migrating towards having the exception handler gather the callstack for us instead of us doing it here. This is because 
    // it turns out that the handler may know better on some platforms how the get the callstack of the exception, and the means may
    // differ based on how the exception handling was done. We temporarily leave code here to get our own callstack in case it wasn't
    // set by the exception handler earlier (which will be so for some older platforms).

    if(mExceptionCallstackCount == 0) // If the exception's callstack wasn't externally set, read our own callstack. 
        mExceptionCallstackCount = EA::Thread::GetCallstack(mExceptionCallstack, EAArrayCount(mExceptionCallstack), pCallstackContext);

    if(mExceptionCallstackCount)
        return WriteCallStackFromCallstack(mExceptionCallstack, mExceptionCallstackCount, addressRepTypeFlags, bAbbreviated);
    else
        WriteText("Callstack not available\r\n", false);

    return false;
}


bool ReportWriter::WriteCallStackFromCallstack(void* callstack[], size_t callstackCount, int addressRepTypeFlags, bool bAbbreviated)
{
    using namespace EA::Callstack;

    bool             returnValue = true;
    AddressRepCache* pARC = Callstack::GetAddressRepCache();
    AddressRepCache  arcLocal;

    if(!pARC)
        pARC = &arcLocal;

    if(pARC)
    {
        const char* pStrResults[kARTCount];
        int         pIntResults[kARTCount];

        if(mpCoreAllocator)
            pARC->SetAllocator(mpCoreAllocator);

        #if defined(EA_PLATFORM_PS3)
            // We have a problem on PS3: The exception handler thread has such a small amount of stack space that 
            // we cannot execute the callstack database lookups while in the exception handling thread. Otherwise
            // our code will crash due to hitting the stack limit.
            addressRepTypeFlags = Callstack::kARTFlagAddress;
            pARC->EnableAutoDatabaseFind(false);
        #endif

        for(size_t i = 0; i < callstackCount; i++)
        {
            const void* pAddress    = callstack[i];
            const int   outputFlags = pARC->GetAddressRep(addressRepTypeFlags, pAddress, pStrResults, pIntResults, true);

            if(bAbbreviated)
            {
                if(outputFlags & kARTFlagFileLine)
                    WriteFormatted("%#p %s: %d\r\n", pAddress, pStrResults[kARTFileLine], pIntResults[kARTFileLine]); // %#p means print %p with 0x in front of it.
                else if(outputFlags & kARTFlagFunctionOffset)
                    WriteFormatted("%#p %s + %d\r\n", pAddress, pStrResults[kARTFunctionOffset], pIntResults[kARTFunctionOffset]);
                else
                    WriteFormatted("%#p\r\n", pAddress); // %#p means print %p with 0x in front of it.
            }
            else
            {
                BeginSection("Callstack entry");

                // To consider: Write the module base address here after the module name.
                Callstack::GetModuleFromAddress(pAddress, mScratchBuffer, sizeof(mScratchBuffer));
                if(mScratchBuffer[0])
                    WriteText(mScratchBuffer, true);

                if((outputFlags & kARTFlagAddress) && pStrResults[kARTAddress])
                    WriteText(pStrResults[kARTAddress], true);

                if((outputFlags & kARTFlagFileLine) && pStrResults[kARTFileLine])
                    WriteFormatted("%s: %d\r\n", pStrResults[kARTFileLine], pIntResults[kARTFileLine]);

                if((outputFlags & kARTFlagFunctionOffset) && pStrResults[kARTFunctionOffset])
                    WriteFormatted("%s + %d\r\n", pStrResults[kARTFunctionOffset], pIntResults[kARTFunctionOffset]);

                if((outputFlags & kARTFlagSource) && pStrResults[kARTSource])
                    WriteText(pStrResults[kARTSource], true);

                EndSection("Callstack entry");
            }
        }
    }

    return returnValue;
}




} // namespace Debug

} // namespace EA












