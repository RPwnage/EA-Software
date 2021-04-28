///////////////////////////////////////////////////////////////////////////////
// SystemInformation.h
//
// Implements class that retrieves several system information
//
// Created by Thomas Bruckschlegel
// Copyright (c) 2010 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////


#ifndef SYSTEMINFORMATION_H
#define SYSTEMINFORMATION_H

#include "EbisuTelemetryAPI.h"
#include "GenericTelemetryElements.h"
#include "EASTL/string.h"

#if defined(ORIGIN_PC)
#include <comdef.h>
#include <Wbemidl.h>
#elif defined(ORIGIN_MAC)
#include <string>
#elif defined(__linux__)
#else
#error "Need platform implementation"
#endif



namespace NonQTOriginServices
{
    const uint32_t MAX_NUMBER_HARDWARE_DEVICE_ENTRIES = 512; // maximum number of devices, like cpus, hdds, etc.
    const uint32_t MAC_ADDR_SIZE = 64; 

    enum SYSTEM_INFORMATION_ERROR_TYPES
    {
        SYSINFO_NO_ERROR
        ,SYSINFO_GENERIC_ERROR
        ,SYSINFO_GET_SYSINFO_PROPERTY_ERROR
        ,SYSINFO_FILE_READ_ERROR
        ,SYSINFO_FILE_WRITE_ERROR
    };


class SystemInformation{

private:
    //  Need to match the string list in cpp file.
    enum HypervisorEnum
    {
        Hypervisor_Unknown,
        Hypervisor_Native,
        Hypervisor_VM,
        Hypervisor_VMRoot,  // Microsoft specific
        Hypervisor_VMChild, // Microsoft specific
    };

#if defined(WIN32)
    bool8_t COMInitialized;
#endif

    uint64_t mSysInfoChecksum;
    eastl::wstring mSysInfoChecksumFileName;
    eastl::wstring mMachineHashFileName;
    char8_t mMacAddr[MAC_ADDR_SIZE];

    uint64_t mMacAddressHash; // Old MachineHash
    uint64_t mUniqueMachineHashFNV1A; // MachineHash
    eastl::string8 clientVersion;
    eastl::string8 bootstrapVersion;

    //    CPU information
    eastl::string8 cpuDescription;
    uint32_t numCPUs;
    uint32_t numberOfLogicalProcessors;
    uint32_t numberOfCores;
    eastl::string8 cpuGHz;
    eastl::string8 instructionSets;
    eastl::string8 biosString;
    eastl::string8 computerName;
    eastl::string8 m_uniqueMachineHash;

    // Bios
    eastl::string8 biosSerialNumber;
    eastl::string8 biosManufacturer;

    //  Motherboard
    eastl::string8 motherboardString;
    eastl::string8 motherboardSerialNumber;

    //    Memory
    uint64_t installedMemory_MB;

    //    OS
    eastl::string8 OSBuildString;
    eastl::string8 OSVersion;
    uint32_t OSArchitecture; // 32 for 32-bit, 64 for 64-bit
    eastl::string8 OSDescription;
    eastl::string8 OSServicePack;
    HypervisorEnum Hypervisor;
    eastl::string8 OSInstallDate;
    eastl::string8 OSSerialNumber;

    //    Locale
    uint32_t OSLocale;            // From OS
    uint16_t UserLocale;          // From OS
    eastl::string8 localeSetting; // From EADM User settings in "enUS" form

    //    HDD:  Hard drisk drive
    uint32_t numHDDs;
    uint64_t freeHDDSpace_GB[MAX_NUMBER_HARDWARE_DEVICE_ENTRIES];
    uint64_t totalHDDSpace_GB[MAX_NUMBER_HARDWARE_DEVICE_ENTRIES];
    eastl::string8 hddName[MAX_NUMBER_HARDWARE_DEVICE_ENTRIES];
    eastl::string8 hddType[MAX_NUMBER_HARDWARE_DEVICE_ENTRIES];
    eastl::string8 hddInterface[MAX_NUMBER_HARDWARE_DEVICE_ENTRIES];
    eastl::string8 hddSerialNumber[MAX_NUMBER_HARDWARE_DEVICE_ENTRIES];

    //    ODD:  Optical disk drive
    uint32_t numODDs;
    eastl::string8 oddName[MAX_NUMBER_HARDWARE_DEVICE_ENTRIES];
    eastl::string8 oddType[MAX_NUMBER_HARDWARE_DEVICE_ENTRIES];

    //    Display
    uint32_t numDisplays;
    uint32_t displayResX[MAX_NUMBER_HARDWARE_DEVICE_ENTRIES];
    uint32_t displayResY[MAX_NUMBER_HARDWARE_DEVICE_ENTRIES];

    //    Video
    uint32_t numVideoController;
    uint32_t numVideoCaptureDevice;
    eastl::string8 videoAdapterName[MAX_NUMBER_HARDWARE_DEVICE_ENTRIES];
    eastl::string8 videoAdapterDriverVersion[MAX_NUMBER_HARDWARE_DEVICE_ENTRIES];
    eastl::string8 videoCaptureDevice[MAX_NUMBER_HARDWARE_DEVICE_ENTRIES];
    uint32_t videoAdapterRAM[MAX_NUMBER_HARDWARE_DEVICE_ENTRIES];
    uint32_t videoBitsPerPixel[MAX_NUMBER_HARDWARE_DEVICE_ENTRIES];
    uint32_t videoDeviceID[MAX_NUMBER_HARDWARE_DEVICE_ENTRIES];
    uint32_t videoVendorID[MAX_NUMBER_HARDWARE_DEVICE_ENTRIES];
    eastl::string8 installedDirectX;

    //    Virtual resolution
    uint32_t virtualResX;
    uint32_t virtualResY;

    //    Microphone
    uint32_t microphoneConnected;

    //    Network
    uint32_t networkSpeed;

    //    Browser
    eastl::string8 defaultBrowserName;

    /// Computed service name
    eastl::string8 mServiceName;

#if defined(WIN32)
    // WMI helper    
    SYSTEM_INFORMATION_ERROR_TYPES GetProperty_UINT16(const IWbemClassObject *pclsObj, const eastl::string16 &name, uint16_t &value);
    SYSTEM_INFORMATION_ERROR_TYPES GetProperty_UINT32(const IWbemClassObject *pclsObj, const eastl::string16 &name, uint32_t &value);
    SYSTEM_INFORMATION_ERROR_TYPES GetProperty_UINT64(const IWbemClassObject *pclsObj, const eastl::string16 &name, uint64_t &value);
    SYSTEM_INFORMATION_ERROR_TYPES GetProperty_BOOLEAN(const IWbemClassObject *pclsObj, const eastl::string16 &name, bool8_t &value);
    SYSTEM_INFORMATION_ERROR_TYPES GetProperty_STRING(const IWbemClassObject *pclsObj, const eastl::string16 &name, eastl::string8 &value);
    
    SYSTEM_INFORMATION_ERROR_TYPES DoQuery(const IWbemServices *pSvc, IEnumWbemClassObject* &pEnumerator, const eastl::string8 &query);

    SYSTEM_INFORMATION_ERROR_TYPES InitWMI(IWbemServices * &pSvc, IWbemLocator * &pLoc);
    void                ShutdownWMI(IWbemServices * &pSvc, IWbemLocator * &pLoc);
    
    SYSTEM_INFORMATION_ERROR_TYPES Retrieve_Win32_ComputerSystem_DATA(IWbemServices *pSvc);
    SYSTEM_INFORMATION_ERROR_TYPES Retrieve_Win32_ComputerName_DATA(IWbemServices *pSvc);
    SYSTEM_INFORMATION_ERROR_TYPES Retrieve_Win32_OperatingSystem_DATA(IWbemServices *pSvc);
    SYSTEM_INFORMATION_ERROR_TYPES Retrieve_Win32_Processor_DATA(IWbemServices *pSvc);
    SYSTEM_INFORMATION_ERROR_TYPES Retrieve_Win32_Bios_DATA(IWbemServices *pSvc);
    SYSTEM_INFORMATION_ERROR_TYPES Retrieve_Win32_LogicalDisk_DATA(IWbemServices *pSvc);
    SYSTEM_INFORMATION_ERROR_TYPES Retrieve_Win32_OpticalDrive(IWbemServices *pSvc);
    SYSTEM_INFORMATION_ERROR_TYPES Retrieve_Win32_DesktopMonitor_DATA(IWbemServices *pSvc);
    SYSTEM_INFORMATION_ERROR_TYPES Retrieve_Win32_VideoController_DATA(IWbemServices *pSvc);
    SYSTEM_INFORMATION_ERROR_TYPES Retrieve_Login_DATA(IWbemServices *pSvc);

    SYSTEM_INFORMATION_ERROR_TYPES Retrieve_Win32_VideoCaptureDevices();
    HRESULT EnumerateDevices(REFGUID category, IEnumMoniker **ppEnum);
    void EnumerateVideoCaptureDevices(IEnumMoniker *pEnum);

#elif defined (__APPLE__)
    void Retrieve_Mac_SystemInformation_DATA();
    std::string GetOSXVersion();
#elif defined(__linux__)
    // TBD
#else
#  error "Need platform specific implementation"
#endif

    SYSTEM_INFORMATION_ERROR_TYPES RetrieveDefaultBrowserName(eastl::string8 &value);
    
    uint64_t createHardwareSurveyChecksum();
    SYSTEM_INFORMATION_ERROR_TYPES WriteSysInfoChecksum();
    SYSTEM_INFORMATION_ERROR_TYPES ReadSysInfoChecksum(uint64_t &checksum);

    SYSTEM_INFORMATION_ERROR_TYPES WriteMachineHash();
    SYSTEM_INFORMATION_ERROR_TYPES ReadMachineHash(uint64_t &checksum);

    SYSTEM_INFORMATION_ERROR_TYPES UpdateHardwareSurvey();


    SystemInformation();

    /// extracts and returns the required service name from the domain name
    eastl::string8 serviceName();

public:

    /// singleton.
    static SystemInformation& instance()
    {

        static SystemInformation instance;

        return instance;
    }

    ~SystemInformation(){}

    SYSTEM_INFORMATION_ERROR_TYPES setMACaddress();
    bool8_t isChecksumValid();

    uint64_t GetMachineHash();
    
    const char8_t* GetClientVersion() const;
    const char8_t* GetBootstrapVersion() const;

    void SetClientVersion(const char8_t* clientVersionParam);
    void SetBootstrapVersion(const char8_t* bootstrapVersionParam);

    const eastl::string CalculateUniqueMachineHash();

    uint64_t uniqueMachineHashFNV1A() const { return mUniqueMachineHashFNV1A; }

    // accessor
    //uint64_t GetMacHash();
    const char8_t* GetBrowserName() const;
    const char8_t* GetOSLocale() const;
    const char8_t* GetUserLocale() const;

    const char8_t* GetCPUName() const;
    uint32_t GetNumCPUs() const;
    uint32_t GetNumCores() const
    {
        return numberOfCores;
    }
    const char8_t* GetCPUSpeed() const
    {
        return cpuGHz.c_str();
    }
    const char8_t* GetInstructionSet() const
    {
        return instructionSets.c_str();
    }
    const char8_t* GetBIOSString() const
    {
        return biosString.c_str();
    }
    eastl::string8 GetBIOSSerialNumber() const
    {
        return biosSerialNumber;
    }
    uint64_t GetInstalledMemory_MB() const;

    //  Motherboard
    const char8_t* GetMotherboardString() const
    {
        return motherboardString.c_str();
    }
    eastl::string8 GetMotherboardSerialNumber() const
    {
        return motherboardSerialNumber;
    }

    //    OS
    const char8_t* GetOSBuildString() const
    {
        return OSBuildString.c_str();
    }
    const char8_t* GetOSDescription() const;
    const char8_t* GetOSVersion() const;
    const char8_t* GetServicePackName() const;
    uint32_t GetOSArchitecture() const
    {
        return OSArchitecture;
    }
    const char8_t* GetHyperVisorString() const;

    //    Locale setting from the Nucleus account
    const char8_t* GetLocaleSetting() const
    {
        return localeSetting.c_str();
    }
    void SetLocaleSetting(const char* pLocale)
    {
        localeSetting = pLocale;
    }

    //    OOA Machine Hash
    const char8_t* GetUniqueMachineHash() const
    {
        return m_uniqueMachineHash.c_str();
    }

    const char8_t* GetMacAddr() const;
    
    uint32_t GetNumHDDs() const;
    uint64_t GetHDDSpace_GB(uint32_t i) const;
    const char8_t* GetHDDInterface(uint32_t i) const;
    const char8_t* GetHDDName(uint32_t i) const;
    const char8_t* GetHDDType(uint32_t i) const;
    eastl::string8 GetHDDSerialNumber(uint32_t i) const;

    uint32_t GetNumODDs() const;
    const char8_t* GetODDType(uint32_t i) const;
    const char8_t* GetODDName(uint32_t i) const;

    uint32_t GetNumDisplays() const;
    uint32_t GetDisplayResX(uint32_t i) const;
    uint32_t GetDisplayResY(uint32_t i) const;
    void AddDisplay(uint32_t x, uint32_t y);

    uint32_t GetNumVideoControllers() const;
    uint32_t GetNumVideoCaptureDevice() const;
    const char8_t* GetVideoAdapterName(uint32_t i) const;
    const char8_t* GetVideoAdapterDriverVersion(uint32_t i) const;
    const char8_t* GetVideoCaptureName(uint32_t i) const;
    uint32_t GetVideoAdapterBitsDepth(uint32_t i) const;
    uint32_t GetVideoAdapterVRAM(uint32_t i) const;
    uint32_t GetVideoAdapterDeviceID(uint32_t i) const;;
    uint32_t GetVideoAdapterVendorID(uint32_t i) const;

    //    Extended resolution
    uint32_t GetVirtualResX() const
    {
        return virtualResX;
    }
    uint32_t GetVirtualResY() const
    {
        return virtualResY;
    }

    uint32_t GetMicrophone() const
    {
        return microphoneConnected;
    }

    //  Detection of Surface Pro computer
    int IsComputerSurfacePro() const;
};

} //namespace


#endif // SYSTEMINFORMATION_H
