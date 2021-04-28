///////////////////////////////////////////////////////////////////////////////
// EAPatchClient/SystemInfo.cpp
//
// Copyright (c) Electronic Arts. All Rights Reserved.
/////////////////////////////////////////////////////////////////////////////


#include <EAPatchClient/Config.h>
#include <EAPatchClient/SystemInfo.h>
#include <EAPatchClient/Debug.h>
#include <EAPatchClient/File.h>
#include <EAPatchClient/XML.h>
#include <EAStdC/EAProcess.h>
#include <EAStdC/EASprintf.h>
#include <EAIO/EAFileBase.h>
#include <EAIO/EAStream.h>

// Much of these system info code is taken from the EA ExceptionHandler package's ReportWriter class.
#if defined(EA_PLATFORM_XENON)
    #include <xtl.h>
    #if EA_XBDM_ENABLED
        #include <xbdm.h>
        // #pragma comment(lib, "xbdm.lib") The application is expected to provide this if needed. 
    #endif

#elif defined(EA_PLATFORM_MICROSOFT)
    #pragma warning(push, 0)
    #include <Windows.h>
    #include <WinVer.h>
    #pragma warning(pop)

    #if defined(EA_PLATFORM_WINDOWS) || EA_WINAPI_FAMILY_PARTITION(EA_WINAPI_PARTITION_DESKTOP)
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

            extern "C" WINBASEAPI BOOL WINAPI GetComputerNameExA(COMPUTER_NAME_FORMAT NameType, LPSTR lpBuffer, LPDWORD nSize);
        #endif

        #pragma comment(lib, "version.lib") 
    #else
        // Temporary while waiting for formal support:
        extern "C" WINBASEAPI DWORD WINAPI GetModuleFileNameA(HMODULE, LPSTR, DWORD);
    #endif

#elif defined(EA_PLATFORM_PS3)
    #include <sdk_version.h>
    #include <netex/libnetctl.h>
    #include <sysutil/sysutil_common.h>
    #include <sysutil/sysutil_sysparam.h>
    #include <sysutil/sysutil_common.h>
    #include <cell/sysmodule.h>
    #include <sys/prx.h>

#elif defined(EA_PLATFORM_APPLE)
    #include <unistd.h>
    #include <sys/types.h>
    #include <sys/sysctl.h>
    #include <sys/utsname.h>
    #include <mach/mach.h>
    #include <mach/task.h>
    #include <mach/mach_error.h>

#elif defined(EA_PLATFORM_UNIX)
    #include <unistd.h>
    #include <sys/types.h>
    #include <sys/utsname.h>
#endif


#if defined(EA_PLATFORM_WINDOWS) || EA_WINAPI_FAMILY_PARTITION(EA_WINAPI_PARTITION_DESKTOP)
    static bool Is64BitOperatingSystem()
    {
        #if (EA_PLATFORM_PTR_SIZE >= 8)
            return true;
        #elif defined(_WIN32)
            #if defined(EA_PLATFORM_WINDOWS) || EA_WINAPI_FAMILY_PARTITION(EA_WINAPI_PARTITION_DESKTOP)
                BOOL b64bitOS = FALSE;
                bool bDoesWos64ProcessExist = (GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "IsWow64Process") != NULL);
                return (bDoesWos64ProcessExist && IsWow64Process(GetCurrentProcess(), &b64bitOS) && b64bitOS);
            #else
                return false;
            #endif
        #else
            // To do: Implement this for other OSs.
            return false;
        #endif
    }
#endif


#if defined(EA_PLATFORM_WINDOWS) || EA_WINAPI_FAMILY_PARTITION(EA_WINAPI_PARTITION_DESKTOP)

    typedef eastl::pair<uint16_t, const char8_t*> LCIDMapping;

    // http://msdn.microsoft.com/en-us/library/dd318693%28v=vs.85%29.aspx
    const LCIDMapping lcidMappingArray[] =
    {
	    LCIDMapping(0x0036, "af"),
	    LCIDMapping(0x0436, "af_ZA"),
	    LCIDMapping(0x001C, "sq"),
	    LCIDMapping(0x041C, "sq_AL"),
	    LCIDMapping(0x0001, "ar"),
	    LCIDMapping(0x1401, "ar_DZ"),
	    LCIDMapping(0x3C01, "ar_BH"),
	    LCIDMapping(0x0C01, "ar_EG"),
	    LCIDMapping(0x0801, "ar_IQ"),
	    LCIDMapping(0x2C01, "ar_JO"),
	    LCIDMapping(0x3401, "ar_KW"),
	    LCIDMapping(0x3001, "ar_LB"),
	    LCIDMapping(0x1001, "ar_LY"),
	    LCIDMapping(0x1801, "ar_MA"),
	    LCIDMapping(0x2001, "ar_OM"),
	    LCIDMapping(0x4001, "ar_QA"),
	    LCIDMapping(0x0401, "ar_SA"),
	    LCIDMapping(0x2801, "ar_SY"),
	    LCIDMapping(0x1C01, "ar_TN"),
	    LCIDMapping(0x3801, "ar_AE"),
	    LCIDMapping(0x2401, "ar_YE"),
	    LCIDMapping(0x002B, "hy"),
	    LCIDMapping(0x042B, "hy_AM"),
	    LCIDMapping(0x002C, "az"),
	    LCIDMapping(0x002D, "eu"),
	    LCIDMapping(0x042D, "eu_ES"),
	    LCIDMapping(0x0023, "be"),
	    LCIDMapping(0x0423, "be_BY"),
	    LCIDMapping(0x0002, "bg"),
	    LCIDMapping(0x0402, "bg_BG"),
	    LCIDMapping(0x0003, "ca"),
	    LCIDMapping(0x0403, "ca_ES"),
	    LCIDMapping(0x0C04, "zh_HK"),
	    LCIDMapping(0x1404, "zh_MO"),
	    LCIDMapping(0x0804, "zh_CN"),
	    LCIDMapping(0x0004, "zh_CHS"),
	    LCIDMapping(0x1004, "zh_SG"),
	    LCIDMapping(0x0404, "zh_TW"),
	    LCIDMapping(0x001A, "hr"),
	    LCIDMapping(0x041A, "hr_HR"),
	    LCIDMapping(0x0005, "cs"),
	    LCIDMapping(0x0405, "cs_CZ"),
	    LCIDMapping(0x0006, "da"),
	    LCIDMapping(0x0406, "da_DK"),
	    LCIDMapping(0x0065, "div"),
	    LCIDMapping(0x0013, "nl"),
	    LCIDMapping(0x0813, "nl_BE"),
	    LCIDMapping(0x0413, "nl_NL"),
	    LCIDMapping(0x0009, "en"),
	    LCIDMapping(0x0C09, "en_AU"),
	    LCIDMapping(0x2809, "en_BZ"),
	    LCIDMapping(0x1009, "en_CA"),
	    LCIDMapping(0x2409, "en_CB"),
	    LCIDMapping(0x1809, "en_IE"),
	    LCIDMapping(0x2009, "en_JM"),
	    LCIDMapping(0x1409, "en_NZ"),
	    LCIDMapping(0x3409, "en_PH"),
	    LCIDMapping(0x0464, "ph_PH"), // Filipino (fil -> "ph") /Philippines (PH), used "ph" to avoid confusion with finnish (fi)
	    LCIDMapping(0x1C09, "en_ZA"),
	    LCIDMapping(0x2C09, "en_TT"),
	    LCIDMapping(0x0809, "en_GB"),
	    LCIDMapping(0x0409, "en_US"),
	    LCIDMapping(0x3009, "en_ZW"),
	    LCIDMapping(0x0025, "et"),
	    LCIDMapping(0x0425, "et_EE"),
	    LCIDMapping(0x0038, "fo"),
	    LCIDMapping(0x0438, "fo_FO"),
	    LCIDMapping(0x0029, "fa"),
	    LCIDMapping(0x0429, "fa_IR"),
	    LCIDMapping(0x000B, "fi"),
	    LCIDMapping(0x040B, "fi_FI"),
	    LCIDMapping(0x000C, "fr"),
	    LCIDMapping(0x080C, "fr_BE"),
	    LCIDMapping(0x0C0C, "fr_CA"),
	    LCIDMapping(0x040C, "fr_FR"),
	    LCIDMapping(0x140C, "fr_LU"),
	    LCIDMapping(0x180C, "fr_MC"),
	    LCIDMapping(0x100C, "fr_CH"),
	    LCIDMapping(0x0056, "gl"),
	    LCIDMapping(0x0456, "gl_ES"),
	    LCIDMapping(0x0037, "ka"),
	    LCIDMapping(0x0437, "ka_GE"),
	    LCIDMapping(0x0007, "de"),
	    LCIDMapping(0x0C07, "de_AT"),
	    LCIDMapping(0x0407, "de_DE"),
	    LCIDMapping(0x1407, "de_LI"),
	    LCIDMapping(0x1007, "de_LU"),
	    LCIDMapping(0x0807, "de_CH"),
	    LCIDMapping(0x0008, "el"),
	    LCIDMapping(0x0408, "el_GR"),
	    LCIDMapping(0x0047, "gu"),
	    LCIDMapping(0x0447, "gu_IN"),
	    LCIDMapping(0x000D, "he"),
	    LCIDMapping(0x040D, "he_IL"),
	    LCIDMapping(0x0039, "hi"),
	    LCIDMapping(0x0439, "hi_IN"),
	    LCIDMapping(0x000E, "hu"),
	    LCIDMapping(0x040E, "hu_HU"),
	    LCIDMapping(0x000F, "is"),
	    LCIDMapping(0x040F, "is_IS"),
	    LCIDMapping(0x0021, "id"),
	    LCIDMapping(0x0421, "id_ID"),
	    LCIDMapping(0x0010, "it"),
	    LCIDMapping(0x0410, "it_IT"),
	    LCIDMapping(0x0810, "it_CH"),
	    LCIDMapping(0x0011, "ja"),
	    LCIDMapping(0x0411, "ja_JP"),
	    LCIDMapping(0x004B, "kn"),
	    LCIDMapping(0x044B, "kn_IN"),
	    LCIDMapping(0x003F, "kk"),
	    LCIDMapping(0x043F, "kk_KZ"),
	    LCIDMapping(0x0057, "ko_ko"),
	    LCIDMapping(0x0012, "ko"),
	    LCIDMapping(0x0412, "ko_KR"),
	    LCIDMapping(0x0040, "ky"),
	    LCIDMapping(0x0440, "ky_KZ"),
	    LCIDMapping(0x0026, "lv"),
	    LCIDMapping(0x0426, "lv_LV"),
	    LCIDMapping(0x0027, "lt"),
	    LCIDMapping(0x0427, "lt_LT"),
	    LCIDMapping(0x002F, "mk"),
	    LCIDMapping(0x042F, "mk_MK"),
	    LCIDMapping(0x003E, "ms"),
	    LCIDMapping(0x083E, "ms_BN"),
	    LCIDMapping(0x043E, "ms_MY"),
	    LCIDMapping(0x004E, "mr"),
	    LCIDMapping(0x044E, "mr_IN"),
	    LCIDMapping(0x0050, "mn"),
	    LCIDMapping(0x0450, "mn_MN"),
	    LCIDMapping(0x0014, "no"),
	    LCIDMapping(0x0414, "nb_NO"),
	    LCIDMapping(0x0814, "nn_NO"),
	    LCIDMapping(0x0015, "pl"),
	    LCIDMapping(0x0415, "pl_PL"),
	    LCIDMapping(0x0016, "pt"),
	    LCIDMapping(0x0416, "pt_BR"),
	    LCIDMapping(0x0816, "pt_PT"),
	    LCIDMapping(0x0046, "pa"),
	    LCIDMapping(0x0446, "pa_IN"),
	    LCIDMapping(0x0018, "ro"),
	    LCIDMapping(0x0418, "ro_RO"),
	    LCIDMapping(0x0019, "ru"),
	    LCIDMapping(0x0419, "ru_RU"),
	    LCIDMapping(0x004F, "sa"),
	    LCIDMapping(0x044F, "sa_IN"),
	    LCIDMapping(0x001B, "sk"),
	    LCIDMapping(0x041B, "sk_SK"),
	    LCIDMapping(0x0024, "sl"),
	    LCIDMapping(0x0424, "sl_SI"),
	    LCIDMapping(0x000A, "es"),
	    LCIDMapping(0x2C0A, "es_AR"),
	    LCIDMapping(0x400A, "es_BO"),
	    LCIDMapping(0x340A, "es_CL"),
	    LCIDMapping(0x240A, "es_CO"),
	    LCIDMapping(0x140A, "es_CR"),
	    LCIDMapping(0x1C0A, "es_DO"),
	    LCIDMapping(0x300A, "es_EC"),
	    LCIDMapping(0x440A, "es_SV"),
	    LCIDMapping(0x100A, "es_GT"),
	    LCIDMapping(0x480A, "es_HN"),
	    LCIDMapping(0x080A, "es_MX"),
	    LCIDMapping(0x4C0A, "es_NI"),
	    LCIDMapping(0x180A, "es_PA"),
	    LCIDMapping(0x3C0A, "es_PY"),
	    LCIDMapping(0x280A, "es_PE"),
	    LCIDMapping(0x500A, "es_PR"),
	    LCIDMapping(0x0C0A, "es_ES"),
	    LCIDMapping(0x380A, "es_UY"),
	    LCIDMapping(0x200A, "es_VE"),
	    LCIDMapping(0x0041, "sw"),
	    LCIDMapping(0x0441, "sw_KE"),
	    LCIDMapping(0x001D, "sv"),
	    LCIDMapping(0x081D, "sv_FI"),
	    LCIDMapping(0x041D, "sv_SE"),
	    LCIDMapping(0x0049, "ta"),
	    LCIDMapping(0x0449, "ta_IN"),
	    LCIDMapping(0x0044, "tt"),
	    LCIDMapping(0x0444, "tt_RU"),
	    LCIDMapping(0x004A, "te"),
	    LCIDMapping(0x044A, "te_IN"),
	    LCIDMapping(0x001E, "th"),
	    LCIDMapping(0x041E, "th_TH"),
	    LCIDMapping(0x001F, "tr"),
	    LCIDMapping(0x041F, "tr_TR"),
	    LCIDMapping(0x0022, "uk"),
	    LCIDMapping(0x0422, "uk_UA"),
	    LCIDMapping(0x0020, "ur"),
	    LCIDMapping(0x0420, "ur_PK"),
	    LCIDMapping(0x0043, "uz"),
	    LCIDMapping(0x002A, "vi"),
	    LCIDMapping(0x042A, "vi_VN")
    };

#endif



namespace EA
{
namespace Patch
{

/////////////////////////////////////////////////////////////////////////////
// SystemInfo
/////////////////////////////////////////////////////////////////////////////

SystemInfo::SystemInfo(bool bSet, bool bEnableUserInfo)
  : mbEnableUserInfo(bEnableUserInfo)
{
    if(bSet)
        Set();
}


void SystemInfo::Reset()
{
  //mEAUserId.set_capacity(0);              // Not cleared because this is set externally.
    mMachineName.set_capacity(0);           // This causes the memory to be freed while also clearing the container.
    mMachineNetworkName.set_capacity(0);
    mMachineType.set_capacity(0);
    mMachineProcessor.set_capacity(0);
    mMachineMemory.set_capacity(0);
    mMachineCPUCount.set_capacity(0);
    mLanguage.set_capacity(0);
    mUserName.set_capacity(0);
    mOSVersion.set_capacity(0);
    mSDKVersion.set_capacity(0);
    mCompilerVersion.set_capacity(0);
    mCompilerPlatformTarget.set_capacity(0);
    mAppPath.set_capacity(0);
    mAppVersion.set_capacity(0);
    mEAPatchVersion.set_capacity(0);
}


bool SystemInfo::operator==(const SystemInfo& x)
{
    return 
        (mEAUserId               == x.mEAUserId)               && 
        (mMachineName            == x.mMachineName)            && 
        (mMachineNetworkName     == x.mMachineNetworkName)     && 
        (mMachineType            == x.mMachineType)            && 
        (mMachineProcessor       == x.mMachineProcessor)       && 
        (mMachineMemory          == x.mMachineMemory)          && 
        (mMachineCPUCount        == x.mMachineCPUCount)        && 
        (mLanguage               == x.mLanguage)               && 
        (mUserName               == x.mUserName)               && 
        (mOSVersion              == x.mOSVersion)              && 
        (mSDKVersion             == x.mSDKVersion)             && 
        (mCompilerVersion        == x.mCompilerVersion)        && 
        (mCompilerPlatformTarget == x.mCompilerPlatformTarget) && 
        (mAppPath                == x.mAppPath)                && 
        (mAppVersion             == x.mAppVersion)             &&
        (mEAPatchVersion         == x.mEAPatchVersion);
}


void SystemInfo::Set()
{
    // mMachineName
    {
        mMachineName.clear();

        if(mbEnableUserInfo)
        {
            #define kMachineNameUnknown "(unknown machine name)"
            #define kMachineNameUnset   "(unset machine name)"

            const size_t kCapacity = 512;
            char8_t nameString[kCapacity];
            nameString[0] = 0;

            #if defined(EA_PLATFORM_XENON)
                #if EA_XBDM_ENABLED
                    DWORD dwCapacity = (DWORD)kCapacity;

                    if(DmGetXboxName(nameString, &dwCapacity) != XBDM_NOERR)
                        EA::StdC::Strlcpy(nameString, kMachineNameUnknown, kCapacity);
                #endif

            #elif defined(EA_PLATFORM_MICROSOFT) && (defined(EA_PLATFORM_WINDOWS) || EA_WINAPI_FAMILY_PARTITION(EA_WINAPI_PARTITION_DESKTOP))
                DWORD dwCapacity = (DWORD)kCapacity;

                if(GetComputerNameA(nameString, &dwCapacity) == 0)
                    EA::StdC::Strlcpy(nameString, kMachineNameUnknown, kCapacity);

            #elif defined(EA_PLATFORM_PS3)
                CellNetCtlInfo info;

                int result = cellNetCtlGetInfo(CELL_NET_CTL_INFO_DHCP_HOSTNAME, &info);

                if((result == 0) && info.dhcp_hostname[0])
                    EA::StdC::Strlcpy(nameString, info.dhcp_hostname, kCapacity);
                else
                {
                    result = cellNetCtlGetInfo(CELL_NET_CTL_INFO_IP_ADDRESS, &info);

                    if((result == 0) && info.ip_address[0])
                        EA::StdC::Strlcpy(nameString, info.ip_address, kCapacity);
                    else
                        EA::StdC::Strlcpy(nameString, kMachineNameUnknown, kCapacity);
                }

            #elif defined(EA_PLATFORM_REVOLUTION)
                EA::StdC::Strlcpy(nameString, kMachineNameUnknown, kCapacity);

            #elif defined(EA_PLATFORM_OSX)
                // http://stackoverflow.com/questions/4063129/get-computer-name-on-mac
                // http://stackoverflow.com/questions/1506912/get-short-user-name-from-full-name
                
                gethostname(nameString, (int)kCapacity);
                nameString[kCapacity - 1] = 0;

            #elif defined(EA_PLATFORM_UNIX)
                gethostname(nameString, (int)kCapacity);
                nameString[kCapacity - 1] = 0;

            #else
                // We don't do anything. Perhaps we should.
                EA::StdC::Strlcpy(nameString, kMachineNameUnknown, kCapacity);
            #endif

            #undef kMachineNameUnknown
            #undef kMachineNameUnset

            mMachineName = nameString;
        }
    }

    // mMachineNetworkName
    {
        mMachineNetworkName.clear();

        if(mbEnableUserInfo)
        {
            #if defined(EA_PLATFORM_WINDOWS) || EA_WINAPI_FAMILY_PARTITION(EA_WINAPI_PARTITION_DESKTOP)
                char  nameString[256];
                DWORD dwCapacity = (DWORD)sizeof(nameString);

                if(GetComputerNameExA(ComputerNameDnsFullyQualified, nameString, &dwCapacity) > 0)
                    mMachineNetworkName = nameString;

            #elif defined(EA_PLATFORM_UNIX)
                char nameString[256];

                gethostname(nameString, (int)sizeof(nameString));
                nameString[sizeof(nameString) - 1] = 0;

                mMachineNetworkName = nameString;
            #endif
        }
    }

    // mMachineType
    {
        mMachineType.clear();

        #if defined(EA_PLATFORM_XENON)
            #if EA_XBDM_ENABLED
                DWORD dwResult = 0xffffffff;
                DmGetConsoleType(&dwResult);

                if(dwResult == DMCT_DEVELOPMENT_KIT)
                    mMachineType = "Development kit";
                else if(dwResult == DMCT_TEST_KIT)
                    mMachineType = "Text kit";
                else if(dwResult == DMCT_REVIEWER_KIT)
                    mMachineType = "Reviewer kit";
                else
                    mMachineType = "Retail kit";
            #endif
        #elif defined(EA_PLATFORM_APPLE)

            // Print basic sysctl data
            // https://developer.apple.com/library/mac/#documentation/Darwin/Reference/Manpages/man3/sysctl.3.html#//apple_ref/doc/man/3/sysctl

            //int    name[2];
            //int    intValue;
            //char   strValue[512];// We use a fixed-size buffer as it's unsafe for us to allocate memory while in an exception handler.
            //size_t length;

        #else
            // To do.
        #endif
    }


    // mMachineProcessor
    {
        mMachineProcessor.clear();

        #if defined(EA_PLATFORM_WINDOWS) || EA_WINAPI_FAMILY_PARTITION(EA_WINAPI_PARTITION_DESKTOP)
            // To consider: Get more precise processor info.
            if(Is64BitOperatingSystem())           // This is more reliable than using systemInfo, as Windows lies about the latter.
                mMachineProcessor = "x86-64";
            else
                mMachineProcessor = "x86";

            SYSTEM_INFO systemInfo;
            GetNativeSystemInfo(&systemInfo);
            mMachineProcessor.append_sprintf(", revision %u", (unsigned)systemInfo.wProcessorRevision);
        #else
            // To do.
        #endif
    }


    // mMachineMemory
    {
        mMachineMemory.clear();

        #if defined(EA_PLATFORM_WINDOWS) || EA_WINAPI_FAMILY_PARTITION(EA_WINAPI_PARTITION_DESKTOP)
            // Memory information
            MEMORYSTATUSEX memoryStatusEx;
            memset(&memoryStatusEx, 0, sizeof(memoryStatusEx));
            memoryStatusEx.dwLength = sizeof(memoryStatusEx);

            if(GlobalMemoryStatusEx(&memoryStatusEx))
                mMachineMemory.sprintf("%I64d Mb", memoryStatusEx.ullTotalPhys / (1024 * 1024));// Or are Mebibytes equal to (1024 * 1000)

        #elif defined(EA_PLATFORM_XENON)
            mMachineMemory = "512 Mb";// shared CPU and GPU memory.

        #elif defined(EA_PLATFORM_PS3)
            mMachineMemory = "256 Mb";

        #elif defined(EA_PLATFORM_APPLE)
            // Read kernel memory size.
            host_basic_info_data_t hostinfo;
            mach_msg_type_number_t count = HOST_BASIC_INFO_COUNT;
            kern_return_t          kr = host_info(mach_host_self(), HOST_BASIC_INFO, (host_info_t)&hostinfo, &count);

            if(kr == KERN_SUCCESS)
            {
                const uint64_t memoryMib = (uint64_t)hostinfo.max_mem / (1024 * 1024);
                mMachineMemory.sprintf("%I64d Mib", memoryMib);
            }

        #else
            // To do.
        #endif
    }


    // mMachineCPUCount
    {
        mMachineCPUCount.clear();

        #if defined(EA_PLATFORM_WINDOWS) || EA_WINAPI_FAMILY_PARTITION(EA_WINAPI_PARTITION_DESKTOP)
            // System info
            SYSTEM_INFO systemInfo;
            GetNativeSystemInfo(&systemInfo);

            mMachineCPUCount.sprintf("%u", systemInfo.dwNumberOfProcessors);

        #elif defined(EA_PLATFORM_APPLE)
            int    name[2]  = { CTL_HW, HW_NCPU };
            int    intValue = 0;
            size_t length   = sizeof(intValue);

            if(sysctl(name, 2, &intValue, &length, NULL, 0) == 0)
                mMachineCPUCount.sprintf("%d", intValue);

        #elif defined(_SC_NPROCESSORS_CONF)
            long nprocs_max = sysconf(_SC_NPROCESSORS_CONF);// There is also _SC_NPROCESSORS_ONLN
            mMachineCPUCount.sprintf("%ld", nprocs_max);

        #elif defined(EA_PLATFORM_XENON)
            mMachineCPUCount = "3";
        #else
            mMachineCPUCount = "1";
        #endif
    }

    // mLanguage
    {
        mLanguage.clear();

        #if defined(EA_PLATFORM_WINDOWS) || EA_WINAPI_FAMILY_PARTITION(EA_WINAPI_PARTITION_DESKTOP)
            bool    bDoesAPIExist = (GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "GetUserDefaultLocaleName") != NULL);

            if(bDoesAPIExist) // True for Vista+.
            {
                wchar_t localeName[64];


                // Prototype the dll function
                typedef int(__stdcall * PrototypeGetUserDefaultLocaleName)(LPWSTR lpLocaleName, int cchLocaleName); //__stdcall used for maximum compatibility. 

                //Cast the generic function pointer to a specific prototyped function pointer
                EA_DISABLE_VC_WARNING(4191); //Disable unsafe conversion warning
                PrototypeGetUserDefaultLocaleName DllGetUserDefaultLocaleName = (PrototypeGetUserDefaultLocaleName)(GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "GetUserDefaultLocaleName"));
                EA_RESTORE_VC_WARNING();

                if(DllGetUserDefaultLocaleName(localeName, EAArrayCount(localeName)) > 0) // Returns a string like "en-US"
                {
                    mLanguage = EA::StdC::Strlcpy<EA::Patch::String, wchar_t>(localeName);

                    if((mLanguage.length() >= 3) && (mLanguage[2] == '-'))
                        mLanguage[2] = '_';   // Convert - to _. This assumes the third char is the '-' char, which ought to be true, but maybe not.
                }
            }
            else
            {
                const LCID lcid = GetUserDefaultLCID();

                for(size_t i = 0; i < EAArrayCount(lcidMappingArray); i++) // This search doesn't need to be fast at all.
                {
                    if(lcidMappingArray[i].first == lcid)
                    {
                        mLanguage = lcidMappingArray[i].second;
                        break;
                    }
                }
            }

        #elif defined(EA_PLATFORM_PS3)
            int  value;
            int  result = cellSysutilGetSystemParamInt(CELL_SYSUTIL_SYSTEMPARAM_ID_LANG, &value);

            if(result == CELL_OK)
            {
                const char8_t* pLanguage = NULL;

                #if !defined(CELL_SYSUTIL_LANG_ENGLISH_US) // If using an older SDK version...
                    #define CELL_SYSUTIL_LANG_ENGLISH_US CELL_SYSUTIL_LANG_ENGLISH
                #endif
                #if !defined(CELL_SYSUTIL_LANG_PORTUGUESE_PT)
                    #define CELL_SYSUTIL_LANG_PORTUGUESE_PT CELL_SYSUTIL_LANG_PORTUGUESE
                #endif
                #if !defined(CELL_SYSUTIL_LANG_PORTUGUESE_BR)
                    #define CELL_SYSUTIL_LANG_PORTUGUESE_BR 17
                #endif
                #if !defined(CELL_SYSUTIL_LANG_ENGLISH_GB)  
                    #define CELL_SYSUTIL_LANG_ENGLISH_GB 18
                #endif

                switch(value)
                {
                    case CELL_SYSUTIL_LANG_JAPANESE:
                        pLanguage = "jp";
                        break;
                    case CELL_SYSUTIL_LANG_ENGLISH_US:
                        pLanguage = "en_us";
                        break;
                    case CELL_SYSUTIL_LANG_FRENCH:
                        pLanguage = "fr";
                        break;
                    case CELL_SYSUTIL_LANG_SPANISH:
                        pLanguage = "es";
                        break;
                    case CELL_SYSUTIL_LANG_GERMAN:
                        pLanguage = "de";
                        break;
                    case CELL_SYSUTIL_LANG_ITALIAN:
                        pLanguage = "it";
                        break;
                    case CELL_SYSUTIL_LANG_DUTCH:
                        pLanguage = "nl";
                        break;
                    case CELL_SYSUTIL_LANG_PORTUGUESE_PT:
                        pLanguage = "pt_pt";
                        break;
                    case CELL_SYSUTIL_LANG_RUSSIAN:
                        pLanguage = "ru";
                        break;
                    case CELL_SYSUTIL_LANG_KOREAN:
                        pLanguage = "ko";
                        break;
                    case CELL_SYSUTIL_LANG_CHINESE_T:
                        pLanguage = "zh_hk";
                        break;
                    case CELL_SYSUTIL_LANG_CHINESE_S:
                        pLanguage = "zh_cn";
                        break;
                    case CELL_SYSUTIL_LANG_FINNISH:
                        pLanguage = "fi";
                        break;
                    case CELL_SYSUTIL_LANG_SWEDISH:
                        pLanguage = "sv";
                        break;
                    case CELL_SYSUTIL_LANG_DANISH:
                        pLanguage = "da";
                        break;
                    case CELL_SYSUTIL_LANG_NORWEGIAN:
                        pLanguage = "no";
                        break;
                    case CELL_SYSUTIL_LANG_POLISH:
                        pLanguage = "pl";
                        break;
                    case CELL_SYSUTIL_LANG_PORTUGUESE_BR:
                        pLanguage = "pt_br";
                        break;
                    case CELL_SYSUTIL_LANG_ENGLISH_GB:
                        pLanguage = "en_gb";
                        break;
                }

                if(pLanguage)
                    mLanguage = pLanguage;
            }

        #endif
    }


    // mUserName
    {
        mUserName.clear();

        if(mbEnableUserInfo)
        {
            #if defined(EA_PLATFORM_MICROSOFT)
                #if defined(EA_PLATFORM_WINDOWS) || EA_WINAPI_FAMILY_PARTITION(EA_WINAPI_PARTITION_DESKTOP)
                    char  buffer[256];
                    DWORD dwCapacity;

                    dwCapacity = (DWORD)sizeof(buffer);
                    if(GetUserNameA(buffer, &dwCapacity) > 0)           
                        mUserName = buffer;
                #else
                    // To do.
                #endif
            #elif defined(EA_PLATFORM_PS3)
                char username[CELL_SYSUTIL_SYSTEMPARAM_CURRENT_USERNAME_SIZE];
                int result = cellSysutilGetSystemParamString(CELL_SYSUTIL_SYSTEMPARAM_ID_CURRENT_USERNAME, username, sizeof(username));
                if(result != CELL_OK)
                    strcpy(username, "unknown");
                mUserName.append_sprintf("%s, ", username);

                char nickname[CELL_SYSUTIL_SYSTEMPARAM_NICKNAME_SIZE];
                result = cellSysutilGetSystemParamString(CELL_SYSUTIL_SYSTEMPARAM_ID_NICKNAME, nickname, sizeof(nickname));
                if(result != CELL_OK)
                    strcpy(nickname, "unknown");
                 mUserName.append_sprintf("%s", nickname);
            #endif
        }
    }


    // mOSVersion
    {
        mOSVersion.clear();

        #if defined(EA_PLATFORM_XENON)
            #if EA_XBDM_ENABLED
                // System version info.
                DM_SYSTEM_INFO dmSystemInfo;
                memset(&dmSystemInfo, 0, sizeof(dmSystemInfo));
                dmSystemInfo.SizeOfStruct = sizeof(dmSystemInfo);

                DmGetSystemInfo(&dmSystemInfo);

                mOSVersion.append_sprintf("Base kernel version: %u.%u.%u.%u, ", dmSystemInfo.BaseKernelVersion.Major, dmSystemInfo.BaseKernelVersion.Minor, dmSystemInfo.BaseKernelVersion.Build, dmSystemInfo.BaseKernelVersion.Qfe);
                mOSVersion.append_sprintf("kernel version: %u.%u.%u.%u, ",      dmSystemInfo.KernelVersion.Major,     dmSystemInfo.KernelVersion.Minor,     dmSystemInfo.KernelVersion.Build,     dmSystemInfo.KernelVersion.Qfe);
                mOSVersion.append_sprintf("XDK version: %u.%u.%u.%u",           dmSystemInfo.XDKVersion.Major,        dmSystemInfo.XDKVersion.Minor,        dmSystemInfo.XDKVersion.Build,        dmSystemInfo.XDKVersion.Qfe);
            #endif
        #elif defined(EA_PLATFORM_WINDOWS) || EA_WINAPI_FAMILY_PARTITION(EA_WINAPI_PARTITION_DESKTOP)

            OSVERSIONINFOEXA osVersionInfoEx;
            memset(&osVersionInfoEx, 0, sizeof(osVersionInfoEx));
            osVersionInfoEx.dwOSVersionInfoSize = sizeof(osVersionInfoEx);

            if(GetVersionExA((LPOSVERSIONINFOA)&osVersionInfoEx))
            {
                char windowsVersion[512] = "Unknown, probably a successor to Windows 7 or Windows server 2008.";

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
                            strcpy(windowsVersion, "Windows 7");
                        else
                            strcpy(windowsVersion, "Windows Server 2008 R2");
                    }
                    else
                    {
                        if(osVersionInfoEx.wProductType == VER_NT_WORKSTATION)
                            strcpy(windowsVersion, "Windows Vista");
                        else
                            strcpy(windowsVersion, "Windows Server 2008");
                    }
                }
                else if(osVersionInfoEx.dwMajorVersion >= 5)
                {
                    if(osVersionInfoEx.dwMinorVersion == 0)
                        strcpy(windowsVersion, "Windows 2000");
                    else if(osVersionInfoEx.dwMinorVersion == 1)
                        strcpy(windowsVersion, "Windows XP");
                    else // osVersionInfoEx.dwMinorVersion == 2
                    {
                        if(GetSystemMetrics(SM_SERVERR2) != 0)
                            strcpy(windowsVersion, "Windows Server 2003 R2");
                        else if(osVersionInfoEx.wSuiteMask & VER_SUITE_WH_SERVER)
                            strcpy(windowsVersion, "Windows Home Server");
                        if(GetSystemMetrics(SM_SERVERR2) == 0)
                            strcpy(windowsVersion, "Windows Server 2003");
                        else
                            strcpy(windowsVersion, "Windows XP Professional x64 Edition");
                    }
                }
                else
                    strcpy(windowsVersion, "Windows 98 or earlier");

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
                            strcat(windowsVersion, ", Unlicensed");
                            break;
                        case PRODUCT_ULTIMATE:
                            strcat(windowsVersion, ", Ultimate Edition");
                            break;
                        case 0x00000030: // PRODUCT_PROFESSIONAL:   // Not all Windows SDK versions define PRODUCT_PROFESSIONAL
                            strcat(windowsVersion, ", Professional");
                            break;
                        case PRODUCT_HOME_PREMIUM:
                            strcat(windowsVersion, ", Home Premium Edition");
                            break;
                        case PRODUCT_HOME_BASIC:
                            strcat(windowsVersion, ", Home Basic Edition");
                            break;
                        case PRODUCT_ENTERPRISE:
                            strcat(windowsVersion, ", Enterprise Edition");
                            break;
                        case PRODUCT_BUSINESS:
                            strcat(windowsVersion, ", Business Edition");
                            break;
                        case PRODUCT_STARTER:
                            strcat(windowsVersion, ", Starter Edition");
                            break;
                        case PRODUCT_CLUSTER_SERVER:
                            strcat(windowsVersion, ", Cluster Server Edition");
                            break;
                        case PRODUCT_DATACENTER_SERVER:
                            strcat(windowsVersion, ", Datacenter Edition");
                            break;
                        case PRODUCT_DATACENTER_SERVER_CORE:
                            strcat(windowsVersion, ", Datacenter Edition (core installation)");
                            break;
                        case PRODUCT_ENTERPRISE_SERVER:
                            strcat(windowsVersion, ", Enterprise Edition");
                            break;
                        case PRODUCT_ENTERPRISE_SERVER_CORE:
                            strcat(windowsVersion, ", Enterprise Edition (core installation)");
                            break;
                        case PRODUCT_ENTERPRISE_SERVER_IA64:
                            strcat(windowsVersion, ", Enterprise Edition for Itanium-based Systems");
                            break;
                        case PRODUCT_SMALLBUSINESS_SERVER:
                            strcat(windowsVersion, ", Small Business Server");
                            break;
                        case PRODUCT_SMALLBUSINESS_SERVER_PREMIUM:
                            strcat(windowsVersion, ", Small Business Server Premium Edition");
                            break;
                        case PRODUCT_STANDARD_SERVER:
                            strcat(windowsVersion, ", Standard Edition");
                            break;
                        case PRODUCT_STANDARD_SERVER_CORE:
                            strcat(windowsVersion, ", Standard Edition (core installation)");
                            break;
                        case PRODUCT_WEB_SERVER:
                            strcat(windowsVersion, ", Web Server Edition");
                            break;
                        default:
                        {
                            char buffer[64];
                            EA::StdC::Sprintf(buffer, "Unknown edition: %u", (unsigned)dwProductType);
                            strcat(windowsVersion, ", ");
                            strcat(windowsVersion, buffer);
                            break;
                        }
                    }
                }

                if(Is64BitOperatingSystem())
                    strcat(windowsVersion, ", 64 bit");

                mOSVersion = windowsVersion;
                mOSVersion.append_sprintf(", version %u.%u.%u", osVersionInfoEx.dwMajorVersion, osVersionInfoEx.dwMinorVersion, osVersionInfoEx.dwBuildNumber);
                mOSVersion.append_sprintf(", service pack %s", osVersionInfoEx.szCSDVersion[0] ? osVersionInfoEx.szCSDVersion : "<none>");
            }
        #elif defined(EA_PLATFORM_UNIX)
            // http://linux.die.net/man/2/uname
            utsname utsName;
            memset(&utsName, 0, sizeof(utsName));

            if(uname(&utsName) == 0)
            {
                mOSVersion.append_sprintf("%s, ", utsName.sysname);// "Linux"
                mOSVersion.append_sprintf("%s, ", utsName.release);// "2.6.28-11-generic"
                mOSVersion.append_sprintf("%s, ", utsName.version);// "#42-Ubuntu SMP Fri Apr 17 01:57:59 UTC 2009"
            }
        #endif
    }


    // mSDKVersion
    {
        mSDKVersion.clear();

        #if defined(EA_PLATFORM_MICROSOFT)
            // http://msdn.microsoft.com/en-us/library/windows/desktop/aa383745%28v=vs.85%29.aspx
            // http://blogs.msdn.com/b/oldnewthing/archive/2007/04/11/2079137.aspx

            #if defined(_WIN32_WINNT)
                // We aren't really identifying the SDK version but the SDK version 
                // compiled against, which may be <= the installed SDK version.
                EA_DISABLE_VC_WARNING(4127) //  4127:conditional expression is constant
                if(_WIN32_WINNT >= 0x0602)
                    mSDKVersion = "Windows 8";
                else if(_WIN32_WINNT >= 0x0601)
                    mSDKVersion = "Windows 7";
                else if(_WIN32_WINNT >= 0x0600)
                    mSDKVersion = "Windows Server 2008";
                else if(_WIN32_WINNT >= 0x0600)
                    mSDKVersion = "Windows Vista";
                else if(_WIN32_WINNT >= 0x0502)
                    mSDKVersion = "Windows Server 2003 SP1";
                else if(_WIN32_WINNT >= 0x0501)
                    mSDKVersion = "Windows XP";// Also Windows Server 2003
                else
                    mSDKVersion = "Unknown";
                EA_RESTORE_VC_WARNING()
            #endif

        #elif defined(EA_PLATFORM_XENON)
            #if EA_XBDM_ENABLED
                mSDKVersion.sprintf("%u", (unsigned)_XDK_VER);

                // #if defined(EA_DEBUG) // XDebugGetSystemVersion isn't an xbdm call, but is a call available only in the debug version of xapilib.
                //     XDebugGetSystemVersion(buffer, sizeof(buffer));
                //     if(buffer[0])
                //         WriteKeyValue("System version", buffer);
                // 
                //     XDebugGetXTLVersion(buffer, sizeof(buffer));
                //     if(buffer[0])
                //         WriteKeyValue("XTL version", buffer);
                // #endif
            #endif

        #elif defined(EA_PLATFORM_PS3)
            mSDKVersion.sprintf("0x%08x", CELL_SDK_VERSION);
        #endif 
    }


    // mCompilerVersion
    {
        mCompilerVersion.sprintf("%s, %s", EA_COMPILER_STRING, (EA_PLATFORM_PTR_SIZE == 4) ? "32 bit" : "64 bit");// EA_COMPILER_STRING comes from EABase.
    }


    // mCompilerPlatformTarget
    {
        mCompilerPlatformTarget.sprintf("%s", EA_PLATFORM_DESCRIPTION);// EA_PLATFORM_DESCRIPTION comes from EABase.
    }

    // mAppPath
    {
        char appPath[IO::kMaxPathLength];
        appPath[0] = 0;
        EA::StdC::GetCurrentProcessPath(appPath);
        mAppPath.sprintf(appPath);
    }

    // mAppVersion
    {
        mAppVersion.clear();

        #if defined(EA_PLATFORM_WINDOWS) || EA_WINAPI_FAMILY_PARTITION(EA_WINAPI_PARTITION_DESKTOP)
            // To consider: Read the Windows VERSIONINFO struct that may be embedded 
            // in the app. However, reading this struct is ridiculously tedious.
            char appPath[IO::kMaxPathLength];
            EA::StdC::GetCurrentProcessPath(appPath);

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
                            mAppVersion.sprintf("%u.%u.%u.%u", HIWORD(pFFI->dwFileVersionMS), LOWORD(pFFI->dwFileVersionMS), 
                                                               HIWORD(pFFI->dwFileVersionLS), LOWORD(pFFI->dwFileVersionLS));
                        }
                    }

                    LocalFree((HLOCAL)pVersionData);
                }
            }
        #else
            // To do.
        #endif
    }

    // mEAPatchVersion
    {
        mEAPatchVersion = EAPATCHCLIENT_VERSION_STRING;
    }
}



/* Disabled until deemed useful.

/////////////////////////////////////////////////////////////////////////////
// SystemInfoSerializer
/////////////////////////////////////////////////////////////////////////////

bool SystemInfoSerializer::Serialize(const SystemInfo& systemInfo, const char8_t* pFilePath)
{
    if(mbSuccess)
    {
        File file;

        mbSuccess = file.Open(pFilePath, EA::IO::kAccessFlagWrite, EA::IO::kCDCreateAlways, 
                                EA::IO::FileStream::kShareRead, EA::IO::FileStream::kUsageHintSequential);
        if(mbSuccess)
        {
            Serialize(systemInfo, file.GetStream(), true);
            file.Close();
        }

        if(file.HasError())         // This will catch errors from file.Open or file.Close above.
            TransferError(file);    // Copy the error state from file to us.
    }

    return mbSuccess;
}

bool SystemInfoSerializer::Serialize(const SystemInfo& systemInfo, EA::IO::IStream* pStream, bool writeDoc)
{
    XMLWriter writer;

    if(writeDoc)
      writer.BeginDocument(pStream);

      writer.BeginParent("SystemInfo");
        writer.WriteValue("EAUserId",                  systemInfo.mEAUserId);
        writer.WriteValue("MachineName",               systemInfo.mMachineName);
        writer.WriteValue("MachineNetworkName",        systemInfo.mMachineNetworkName);
        writer.WriteValue("MachineType",               systemInfo.mMachineType);
        writer.WriteValue("MachineProcessor",          systemInfo.mMachineProcessor);
        writer.WriteValue("MachineMemory",             systemInfo.mMachineMemory);
        writer.WriteValue("MachineCPUCount",           systemInfo.mMachineCPUCount);
        writer.WriteValue("Language",                  systemInfo.mLanguage);
        writer.WriteValue("UserName",                  systemInfo.mUserName);
        writer.WriteValue("OSVersion",                 systemInfo.mOSVersion);
        writer.WriteValue("SDKVersion",                systemInfo.mSDKVersion);
        writer.WriteValue("CompilerVersion",           systemInfo.mCompilerVersion);
        writer.WriteValue("CompilerPlatformTarget",    systemInfo.mCompilerPlatformTarget);
        writer.WriteValue("AppPath",                   systemInfo.mAppPath);
        writer.WriteValue("AppVersion",                systemInfo.mAppVersion);
        writer.WriteValue("EAPatchVersion",            systemInfo.mEAPatchVersion);
      writer.EndParent("SystemInfo");

    if(writeDoc)
      writer.EndDocument();

    TransferError(writer);

    return mbSuccess;
}

bool SystemInfoSerializer::Deserialize(SystemInfo& systemInfo, const char8_t* pFilePath)
{
    if(mbSuccess)
    {
        File file;

        mbSuccess = file.Open(pFilePath, EA::IO::kAccessFlagRead, EA::IO::kCDOpenExisting, 
                                EA::IO::FileStream::kShareRead, EA::IO::FileStream::kUsageHintSequential);
        if(mbSuccess)
        {
            Deserialize(systemInfo, file.GetStream(), true);
            file.Close();
        }

        if(file.HasError())         // This will catch errors from file.Open or file.Close above.
            TransferError(file);    // Copy the error state from file to us.
    }

    return mbSuccess;
}

bool SystemInfoSerializer::Deserialize(SystemInfo& systemInfo, EA::IO::IStream* pStream, bool readDoc)
{
    XMLReader reader;
    bool      bFound;

    if(readDoc)
      reader.BeginDocument(pStream);

      reader.BeginParent("SystemInfo", bFound); EAPATCH_ASSERT(bFound);
        reader.ReadValue("EAUserId",               bFound, systemInfo.mEAUserId);
        reader.ReadValue("MachineName",            bFound, systemInfo.mMachineName);
        reader.ReadValue("MachineNetworkName",     bFound, systemInfo.mMachineNetworkName);
        reader.ReadValue("MachineType",            bFound, systemInfo.mMachineType);
        reader.ReadValue("MachineProcessor",       bFound, systemInfo.mMachineProcessor);
        reader.ReadValue("MachineMemory",          bFound, systemInfo.mMachineMemory);
        reader.ReadValue("MachineCPUCount",        bFound, systemInfo.mMachineCPUCount);
        reader.ReadValue("Language",               bFound, systemInfo.mLanguage);
        reader.ReadValue("UserName",               bFound, systemInfo.mUserName);
        reader.ReadValue("OSVersion",              bFound, systemInfo.mOSVersion);
        reader.ReadValue("SDKVersion",             bFound, systemInfo.mSDKVersion);
        reader.ReadValue("CompilerVersion",        bFound, systemInfo.mCompilerVersion);
        reader.ReadValue("CompilerPlatformTarget", bFound, systemInfo.mCompilerPlatformTarget);
        reader.ReadValue("AppPath",                bFound, systemInfo.mAppPath);
        reader.ReadValue("AppVersion",             bFound, systemInfo.mAppVersion);
        reader.ReadValue("EAPatchVersion",         bFound, systemInfo.mEAPatchVersion);
      reader.EndParent("SystemInfo");

    if(readDoc)
      reader.EndDocument();

    TransferError(reader);

    return mbSuccess;
}

*/


} // namespace Patch
} // namespace EA



