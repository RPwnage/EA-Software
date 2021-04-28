///////////////////////////////////////////////////////////////////////////////
// SystemInformation.cpp
//
// Implements class that retrieves several system information
//
// Created by Thomas Bruckschlegel
// Copyright (c) 2010 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#include "version/version.h"
#include "SystemInformation.h"
#include "Utilities.h"
#include "TelemetryConfig.h"
#include "TelemetryLogger.h"

#include "EAStdC/EAString.h"
#include "EAStdC/EASprintf.h"
#include "EAIO/EAFileUtil.h"
#include "EAIO/EAIniFile.h"
#include "EAIO/PathString.h"
#include "EASTL/map.h"
#include "EAAssert/eaassert.h"
#include <algorithm>

#include "netconn.h"

#if defined(ORIGIN_PC)
#  include <dshow.h>
#  include <comdef.h>
#  include <Wbemidl.h>
#  include <sddl.h>
#  include <shlobj.h>
#  include <intrin.h>
#  pragma comment(lib, "wbemuuid.lib")
#  pragma comment(lib, "strmiids")

#elif defined(ORIGIN_MAC)
#  include <CoreFoundation/CoreFoundation.h>
#  include <ApplicationServices/ApplicationServices.h>
#  include <iostream>
#  include <string>
#  include <fstream>
#  include <sstream>
#  include <stdexcept>
#  include <QuickTime/QuickTime.h>
#  include <QuickTime/MoviesFormat.h>
#  include <CoreAudio/CoreAudio.h>
#  include <IOKit/audio/IOAudioTypes.h>

#elif defined(__linux__)
#include <assert.h>
#include <langinfo.h>

#endif



extern bool g_dirtynetStarted;

namespace NonQTOriginServices
{

#if defined(ORIGIN_MAC)

/// \brief Handy template for scoping references to CoreFoundation entities that
/// need get cleaned up before the return of the function.
template <typename T>
class ScopedCFRef
{
public:
    
    ScopedCFRef()
    : _ref(NULL)
    {
    }
    
    explicit ScopedCFRef(T ref)
    : _ref(ref)
    {
    }
    
    ~ScopedCFRef()
    {
        if (_ref != NULL)
        {
            CFRelease(_ref);
        }
    }
    
    T& operator()()
    {
        return _ref;
    }
    
    T& operator*()
    {
        return _ref;
    }
    
    bool isNull()
    {
        return _ref == NULL;
    }
    
private:
    
    T _ref;
};

#endif

#ifndef COM_SAFE_RELEASE
#define COM_SAFE_RELEASE(p) { if (p) { (p)->Release();   (p)=NULL; } }
#endif

const wchar_t* const ksysInfoChecksumFileName = L"HWSWCache";
const wchar_t* const kmachineHashFileName = L"data";

// C9x prettiness....
const char8_t kTelemetryNoneString[] = "none";
const wchar_t kMacTempDataPath[] = L"/var/tmp/Origin/";



//  Needs to match HypervisorEnum in header file
const char* HypervisorString[] = 
{
    "HypervisorUnknown",
    "HypervisorNative",
    "HypervisorVM",
    "HypervisorVMRoot",  // Microsoft specific
    "HypervisorVMChild", // Microsoft specific
};

typedef eastl::map<uint32_t, eastl::string8> _oslocal_map_t;
typedef _oslocal_map_t::value_type _oslocal_map_entry_t;

//    Locale mapping from :
//    http://msdn.microsoft.com/en-us/library/dd318693%28v=vs.85%29.aspx
const _oslocal_map_entry_t _tmp_locale_map[] =
{
    _oslocal_map_entry_t(0x0036, "af"),
    _oslocal_map_entry_t(0x0436, "afZA"),
    _oslocal_map_entry_t(0x001C, "sq"),
    _oslocal_map_entry_t(0x041C, "sqAL"),
    _oslocal_map_entry_t(0x0001, "ar"),
    _oslocal_map_entry_t(0x1401, "arDZ"),
    _oslocal_map_entry_t(0x3C01, "arBH"),
    _oslocal_map_entry_t(0x0C01, "arEG"),
    _oslocal_map_entry_t(0x0801, "arIQ"),
    _oslocal_map_entry_t(0x2C01, "arJO"),
    _oslocal_map_entry_t(0x3401, "arKW"),
    _oslocal_map_entry_t(0x3001, "arLB"),
    _oslocal_map_entry_t(0x1001, "arLY"),
    _oslocal_map_entry_t(0x1801, "arMA"),
    _oslocal_map_entry_t(0x2001, "arOM"),
    _oslocal_map_entry_t(0x4001, "arQA"),
    _oslocal_map_entry_t(0x0401, "arSA"),
    _oslocal_map_entry_t(0x2801, "arSY"),
    _oslocal_map_entry_t(0x1C01, "arTN"),
    _oslocal_map_entry_t(0x3801, "arAE"),
    _oslocal_map_entry_t(0x2401, "arYE"),
    _oslocal_map_entry_t(0x002B, "hy"),
    _oslocal_map_entry_t(0x042B, "hyAM"),
    _oslocal_map_entry_t(0x002C, "az"),
    //_oslocal_map_entry_t(0x082C, "azAZCyrl"),
    //_oslocal_map_entry_t(0x042C, "azAZLatn"),
    _oslocal_map_entry_t(0x002D, "eu"),
    _oslocal_map_entry_t(0x042D, "euES"),
    _oslocal_map_entry_t(0x0023, "be"),
    _oslocal_map_entry_t(0x0423, "beBY"),
    _oslocal_map_entry_t(0x0002, "bg"),
    _oslocal_map_entry_t(0x0402, "bgBG"),
    _oslocal_map_entry_t(0x0003, "ca"),
    _oslocal_map_entry_t(0x0403, "caES"),
    _oslocal_map_entry_t(0x0C04, "zhHK"),
    _oslocal_map_entry_t(0x1404, "zhMO"),
    _oslocal_map_entry_t(0x0804, "zhCN"),
    _oslocal_map_entry_t(0x0004, "zhCHS"),
    _oslocal_map_entry_t(0x1004, "zhSG"),
    _oslocal_map_entry_t(0x0404, "zhTW"),
    //_oslocal_map_entry_t(0x7C04, "zhCHT"),
    _oslocal_map_entry_t(0x001A, "hr"),
    _oslocal_map_entry_t(0x041A, "hrHR"),
    _oslocal_map_entry_t(0x0005, "cs"),
    _oslocal_map_entry_t(0x0405, "csCZ"),
    _oslocal_map_entry_t(0x0006, "da"),
    _oslocal_map_entry_t(0x0406, "daDK"),
    _oslocal_map_entry_t(0x0065, "div"),
    //_oslocal_map_entry_t(0x0465, "divMV"),
    _oslocal_map_entry_t(0x0013, "nl"),
    _oslocal_map_entry_t(0x0813, "nlBE"),
    _oslocal_map_entry_t(0x0413, "nlNL"),
    _oslocal_map_entry_t(0x0009, "en"),
    _oslocal_map_entry_t(0x0C09, "enAU"),
    _oslocal_map_entry_t(0x2809, "enBZ"),
    _oslocal_map_entry_t(0x1009, "enCA"),
    _oslocal_map_entry_t(0x2409, "enCB"),
    _oslocal_map_entry_t(0x1809, "enIE"),
    _oslocal_map_entry_t(0x2009, "enJM"),
    _oslocal_map_entry_t(0x1409, "enNZ"),
    _oslocal_map_entry_t(0x3409, "enPH"),
    _oslocal_map_entry_t(0x0464, "phPH"), // Filipino (fil -> "ph") /Philippines (PH), used "ph" to avoid confusion with finnish (fi)
    _oslocal_map_entry_t(0x1C09, "enZA"),
    _oslocal_map_entry_t(0x2C09, "enTT"),
    _oslocal_map_entry_t(0x0809, "enGB"),
    _oslocal_map_entry_t(0x0409, "enUS"),
    _oslocal_map_entry_t(0x3009, "enZW"),
    _oslocal_map_entry_t(0x0025, "et"),
    _oslocal_map_entry_t(0x0425, "etEE"),
    _oslocal_map_entry_t(0x0038, "fo"),
    _oslocal_map_entry_t(0x0438, "foFO"),
    _oslocal_map_entry_t(0x0029, "fa"),
    _oslocal_map_entry_t(0x0429, "faIR"),
    _oslocal_map_entry_t(0x000B, "fi"),
    _oslocal_map_entry_t(0x040B, "fiFI"),
    _oslocal_map_entry_t(0x000C, "fr"),
    _oslocal_map_entry_t(0x080C, "frBE"),
    _oslocal_map_entry_t(0x0C0C, "frCA"),
    _oslocal_map_entry_t(0x040C, "frFR"),
    _oslocal_map_entry_t(0x140C, "frLU"),
    _oslocal_map_entry_t(0x180C, "frMC"),
    _oslocal_map_entry_t(0x100C, "frCH"),
    _oslocal_map_entry_t(0x0056, "gl"),
    _oslocal_map_entry_t(0x0456, "glES"),
    _oslocal_map_entry_t(0x0037, "ka"),
    _oslocal_map_entry_t(0x0437, "kaGE"),
    _oslocal_map_entry_t(0x0007, "de"),
    _oslocal_map_entry_t(0x0C07, "deAT"),
    _oslocal_map_entry_t(0x0407, "deDE"),
    _oslocal_map_entry_t(0x1407, "deLI"),
    _oslocal_map_entry_t(0x1007, "deLU"),
    _oslocal_map_entry_t(0x0807, "deCH"),
    _oslocal_map_entry_t(0x0008, "el"),
    _oslocal_map_entry_t(0x0408, "elGR"),
    _oslocal_map_entry_t(0x0047, "gu"),
    _oslocal_map_entry_t(0x0447, "guIN"),
    _oslocal_map_entry_t(0x000D, "he"),
    _oslocal_map_entry_t(0x040D, "heIL"),
    _oslocal_map_entry_t(0x0039, "hi"),
    _oslocal_map_entry_t(0x0439, "hiIN"),
    _oslocal_map_entry_t(0x000E, "hu"),
    _oslocal_map_entry_t(0x040E, "huHU"),
    _oslocal_map_entry_t(0x000F, "is"),
    _oslocal_map_entry_t(0x040F, "isIS"),
    _oslocal_map_entry_t(0x0021, "id"),
    _oslocal_map_entry_t(0x0421, "idID"),
    _oslocal_map_entry_t(0x0010, "it"),
    _oslocal_map_entry_t(0x0410, "itIT"),
    _oslocal_map_entry_t(0x0810, "itCH"),
    _oslocal_map_entry_t(0x0011, "ja"),
    _oslocal_map_entry_t(0x0411, "jaJP"),
    _oslocal_map_entry_t(0x004B, "kn"),
    _oslocal_map_entry_t(0x044B, "knIN"),
    _oslocal_map_entry_t(0x003F, "kk"),
    _oslocal_map_entry_t(0x043F, "kkKZ"),
    _oslocal_map_entry_t(0x0057, "kok"),
    //_oslocal_map_entry_t(0x0457, "kokIN"),
    _oslocal_map_entry_t(0x0012, "ko"),
    _oslocal_map_entry_t(0x0412, "koKR"),
    _oslocal_map_entry_t(0x0040, "ky"),
    _oslocal_map_entry_t(0x0440, "kyKZ"),
    _oslocal_map_entry_t(0x0026, "lv"),
    _oslocal_map_entry_t(0x0426, "lvLV"),
    _oslocal_map_entry_t(0x0027, "lt"),
    _oslocal_map_entry_t(0x0427, "ltLT"),
    _oslocal_map_entry_t(0x002F, "mk"),
    _oslocal_map_entry_t(0x042F, "mkMK"),
    _oslocal_map_entry_t(0x003E, "ms"),
    _oslocal_map_entry_t(0x083E, "msBN"),
    _oslocal_map_entry_t(0x043E, "msMY"),
    _oslocal_map_entry_t(0x004E, "mr"),
    _oslocal_map_entry_t(0x044E, "mrIN"),
    _oslocal_map_entry_t(0x0050, "mn"),
    _oslocal_map_entry_t(0x0450, "mnMN"),
    _oslocal_map_entry_t(0x0014, "no"),
    _oslocal_map_entry_t(0x0414, "nbNO"),
    _oslocal_map_entry_t(0x0814, "nnNO"),
    _oslocal_map_entry_t(0x0015, "pl"),
    _oslocal_map_entry_t(0x0415, "plPL"),
    _oslocal_map_entry_t(0x0016, "pt"),
    _oslocal_map_entry_t(0x0416, "ptBR"),
    _oslocal_map_entry_t(0x0816, "ptPT"),
    _oslocal_map_entry_t(0x0046, "pa"),
    _oslocal_map_entry_t(0x0446, "paIN"),
    _oslocal_map_entry_t(0x0018, "ro"),
    _oslocal_map_entry_t(0x0418, "roRO"),
    _oslocal_map_entry_t(0x0019, "ru"),
    _oslocal_map_entry_t(0x0419, "ruRU"),
    _oslocal_map_entry_t(0x004F, "sa"),
    _oslocal_map_entry_t(0x044F, "saIN"),
    //_oslocal_map_entry_t(0x0C1A, "srSPCyrl"),
    //_oslocal_map_entry_t(0x081A, "srSPLatn"),
    _oslocal_map_entry_t(0x001B, "sk"),
    _oslocal_map_entry_t(0x041B, "skSK"),
    _oslocal_map_entry_t(0x0024, "sl"),
    _oslocal_map_entry_t(0x0424, "slSI"),
    _oslocal_map_entry_t(0x000A, "es"),
    _oslocal_map_entry_t(0x2C0A, "esAR"),
    _oslocal_map_entry_t(0x400A, "esBO"),
    _oslocal_map_entry_t(0x340A, "esCL"),
    _oslocal_map_entry_t(0x240A, "esCO"),
    _oslocal_map_entry_t(0x140A, "esCR"),
    _oslocal_map_entry_t(0x1C0A, "esDO"),
    _oslocal_map_entry_t(0x300A, "esEC"),
    _oslocal_map_entry_t(0x440A, "esSV"),
    _oslocal_map_entry_t(0x100A, "esGT"),
    _oslocal_map_entry_t(0x480A, "esHN"),
    _oslocal_map_entry_t(0x080A, "esMX"),
    _oslocal_map_entry_t(0x4C0A, "esNI"),
    _oslocal_map_entry_t(0x180A, "esPA"),
    _oslocal_map_entry_t(0x3C0A, "esPY"),
    _oslocal_map_entry_t(0x280A, "esPE"),
    _oslocal_map_entry_t(0x500A, "esPR"),
    _oslocal_map_entry_t(0x0C0A, "esES"),
    _oslocal_map_entry_t(0x380A, "esUY"),
    _oslocal_map_entry_t(0x200A, "esVE"),
    _oslocal_map_entry_t(0x0041, "sw"),
    _oslocal_map_entry_t(0x0441, "swKE"),
    _oslocal_map_entry_t(0x001D, "sv"),
    _oslocal_map_entry_t(0x081D, "svFI"),
    _oslocal_map_entry_t(0x041D, "svSE"),
    _oslocal_map_entry_t(0x005A, "syr"),
    _oslocal_map_entry_t(0x045A, "syrSY"),
    _oslocal_map_entry_t(0x0049, "ta"),
    _oslocal_map_entry_t(0x0449, "taIN"),
    _oslocal_map_entry_t(0x0044, "tt"),
    _oslocal_map_entry_t(0x0444, "ttRU"),
    _oslocal_map_entry_t(0x004A, "te"),
    _oslocal_map_entry_t(0x044A, "teIN"),
    _oslocal_map_entry_t(0x001E, "th"),
    _oslocal_map_entry_t(0x041E, "thTH"),
    _oslocal_map_entry_t(0x001F, "tr"),
    _oslocal_map_entry_t(0x041F, "trTR"),
    _oslocal_map_entry_t(0x0022, "uk"),
    _oslocal_map_entry_t(0x0422, "ukUA"),
    _oslocal_map_entry_t(0x0020, "ur"),
    _oslocal_map_entry_t(0x0420, "urPK"),
    _oslocal_map_entry_t(0x0043, "uz"),
    //_oslocal_map_entry_t(0x0843, "uzUZCyrl"),
    //_oslocal_map_entry_t(0x0443, "uzUZLatn"),
    _oslocal_map_entry_t(0x002A, "vi"),
    _oslocal_map_entry_t(0x042A, "viVN"),
};

const _oslocal_map_t NumericLocale_to_Name( _tmp_locale_map, _tmp_locale_map + sizeof(_tmp_locale_map)/sizeof(_tmp_locale_map[0]) );


#if defined(ORIGIN_PC)

SYSTEM_INFORMATION_ERROR_TYPES SystemInformation::InitWMI(IWbemServices * &pSvc, IWbemLocator * &pLoc)
{
    HRESULT hres=0;
    hres =  CoInitializeEx(0, COINIT_MULTITHREADED); 
    
    if(hres == RPC_E_CHANGED_MODE)
        COMInitialized=false;
    else
        COMInitialized=true;
    
    hres =  CoInitializeSecurity(
                                 NULL, 
                                 -1,                          // COM authentication
                                 NULL,                        // Authentication services
                                 NULL,                        // Reserved
                                 RPC_C_AUTHN_LEVEL_DEFAULT,   // Default authentication 
                                 RPC_C_IMP_LEVEL_IMPERSONATE, // Default Impersonation  
                                 NULL,                        // Authentication info
                                 EOAC_NONE,                   // Additional capabilities 
                                 NULL                         // Reserved
                                 );
    
    
    if (0)//FAILED(hres)) // can fail in the ui client
    {
#ifdef _DEBUG
        //EA_TRACE_FORMATTED(("Failed to initialize security. Error code =%x\n", hres));
#endif
        return SYSINFO_GENERIC_ERROR;                    // Program has failed.
    }
    
    hres = CoCreateInstance(
                            CLSID_WbemLocator,             
                            0, 
                            CLSCTX_INPROC_SERVER, 
                            IID_IWbemLocator, (LPVOID *) &pLoc);
    
    if (FAILED(hres))
    {
#ifdef _DEBUG
        //EA_TRACE_FORMATTED(("Failed to create IWbemLocator object. Err code = %x\n", hres));
#endif
        return SYSINFO_GENERIC_ERROR;                 // Program has failed.
    }
    
    hres = pLoc->ConnectServer(
                               _bstr_t(L"ROOT\\CIMV2"), 
                               NULL,                    
                               NULL,                    
                               0,                       
                               NULL,                    
                               0,                       
                               0,                       
                               &pSvc                    
                               );
    
    if (FAILED(hres))
    {
#ifdef _DEBUG
        //EA_TRACE_FORMATTED(("Could not connect. Error code = %x\n", hres));
#endif
        COM_SAFE_RELEASE(pLoc);
        return SYSINFO_GENERIC_ERROR;                
    }
    
    hres = CoSetProxyBlanket(
                             pSvc,                        
                             RPC_C_AUTHN_WINNT,           
                             RPC_C_AUTHZ_NONE,            
                             NULL,                        
                             RPC_C_AUTHN_LEVEL_CALL,      
                             RPC_C_IMP_LEVEL_IMPERSONATE, 
                             NULL,                        
                             EOAC_NONE                     
                             );
    
    if (FAILED(hres))
    {
#ifdef _DEBUG
        //EA_TRACE_FORMATTED(("Could not set proxy blanket. Error code = %x\n", hres));
#endif
        COM_SAFE_RELEASE(pSvc);
        COM_SAFE_RELEASE(pLoc);
        
        return SYSINFO_GENERIC_ERROR;              
    }
    
    return SYSINFO_NO_ERROR;
}

void SystemInformation::ShutdownWMI(IWbemServices * &pSvc, IWbemLocator * &pLoc)
{
    COM_SAFE_RELEASE(pSvc);
    COM_SAFE_RELEASE(pLoc);
    
    if(COMInitialized)
        CoUninitialize();    // too risky....
}

SYSTEM_INFORMATION_ERROR_TYPES SystemInformation::GetProperty_UINT16(const IWbemClassObject *pclsObj, const eastl::string16 &name, uint16_t &value)
{
    VARIANT vtProp;
    value=0;
    
    if(!pclsObj)
        return SYSINFO_GET_SYSINFO_PROPERTY_ERROR;
    
    HRESULT hr = const_cast<IWbemClassObject*>(pclsObj)->Get(name.c_str(), 0, &vtProp, 0, 0);    
    
    if(hr==S_OK && vtProp.vt != VT_NULL && vtProp.bstrVal!=NULL)
    {
        value= static_cast<uint16_t>(vtProp.uintVal);
        VariantClear(&vtProp);
    }
    else
        return SYSINFO_GET_SYSINFO_PROPERTY_ERROR;
    
    return SYSINFO_NO_ERROR;
}

SYSTEM_INFORMATION_ERROR_TYPES SystemInformation::GetProperty_UINT32(const IWbemClassObject *pclsObj, const eastl::string16 &name, uint32_t &value)
{
    VARIANT vtProp;
    value=0;
    
    if(!pclsObj)
        return SYSINFO_GET_SYSINFO_PROPERTY_ERROR;
    
    HRESULT hr = const_cast<IWbemClassObject*>(pclsObj)->Get(name.c_str(), 0, &vtProp, 0, 0);    
    
    if(hr==S_OK && vtProp.vt != VT_NULL && vtProp.bstrVal!=NULL)
    {
        value= vtProp.uintVal;
        VariantClear(&vtProp);
    }
    else
        return SYSINFO_GET_SYSINFO_PROPERTY_ERROR;
    
    return SYSINFO_NO_ERROR;
}

SYSTEM_INFORMATION_ERROR_TYPES SystemInformation::GetProperty_UINT64(const IWbemClassObject *pclsObj, const eastl::string16 &name, uint64_t &value)
{
    VARIANT vtProp;
    value=0;
    
    if(!pclsObj)
        return SYSINFO_GET_SYSINFO_PROPERTY_ERROR;
    
    HRESULT hr = const_cast<IWbemClassObject*>(pclsObj)->Get(name.c_str(), 0, &vtProp, 0, 0);    
    
    if(hr==S_OK && vtProp.vt != VT_NULL && vtProp.bstrVal!=NULL)
    {
        value=EA::StdC::AtoI64(vtProp.bstrVal);
        VariantClear(&vtProp);
    }
    else
        return SYSINFO_GET_SYSINFO_PROPERTY_ERROR;
    
    return SYSINFO_NO_ERROR;
}


SYSTEM_INFORMATION_ERROR_TYPES SystemInformation::GetProperty_BOOLEAN(const IWbemClassObject *pclsObj, const eastl::string16 &name, bool8_t &value)
{
    VARIANT vtProp;
    value=false;
    
    if(!pclsObj)
        return SYSINFO_GET_SYSINFO_PROPERTY_ERROR;
    
    HRESULT hr = const_cast<IWbemClassObject*>(pclsObj)->Get(name.c_str(), 0, &vtProp, 0, 0);    
    
    if(hr==S_OK && vtProp.vt != VT_NULL && vtProp.bstrVal!=NULL)
    {
        value= vtProp.boolVal==VARIANT_TRUE ? true : false;
        VariantClear(&vtProp);
    }
    else
        return SYSINFO_GET_SYSINFO_PROPERTY_ERROR;
    
    return SYSINFO_NO_ERROR;
}

SYSTEM_INFORMATION_ERROR_TYPES SystemInformation::GetProperty_STRING(const IWbemClassObject *pclsObj, const eastl::string16 &name, eastl::string8 &value)
{
    VARIANT vtProp;
    value.clear();
    
    if(!pclsObj)
        return SYSINFO_GET_SYSINFO_PROPERTY_ERROR;
    
    HRESULT hr = const_cast<IWbemClassObject*>(pclsObj)->Get(name.c_str(), 0, &vtProp, 0, 0);    
    
    if(hr==S_OK && vtProp.vt != VT_NULL && vtProp.bstrVal!=NULL)
    {
        eastl::string16 tmpValue;
        tmpValue.assign(reinterpret_cast<char16_t*>(vtProp.bstrVal));
        value  = EA::StdC::ConvertString<eastl::string16, eastl::string8>(tmpValue);
        VariantClear(&vtProp);
    }
    else
        return SYSINFO_GET_SYSINFO_PROPERTY_ERROR;
    
    return SYSINFO_NO_ERROR;
}


SYSTEM_INFORMATION_ERROR_TYPES SystemInformation::DoQuery(const IWbemServices *pSvc, IEnumWbemClassObject* &pEnumerator, const eastl::string8 &query)
{
    if(!pSvc)
        return SYSINFO_GENERIC_ERROR; 
    
    pEnumerator = NULL;
    HRESULT hres = const_cast<IWbemServices*>(pSvc)->ExecQuery(
                                                               bstr_t("WQL"), 
                                                               bstr_t(query.c_str()),
                                                               WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, 
                                                               NULL,
                                                               &pEnumerator);
    
    if (FAILED(hres))
    {
#ifdef _DEBUG
        //EA_TRACE_FORMATTED(("Query Error code = %x\n", hres));
#endif
        return SYSINFO_GENERIC_ERROR;               
    }
    
    return SYSINFO_NO_ERROR; 
}

SYSTEM_INFORMATION_ERROR_TYPES SystemInformation::Retrieve_Win32_ComputerSystem_DATA(IWbemServices *pSvc)
{
    //    Check if microphone is connected to computer
    microphoneConnected = waveInGetNumDevs();
    
    SYSTEM_INFORMATION_ERROR_TYPES res=SYSINFO_NO_ERROR;
    IEnumWbemClassObject* pEnumerator = NULL;
    ULONG uReturn = 0;
    
    DoQuery(pSvc, pEnumerator, "SELECT Capacity FROM Win32_PhysicalMemory");    
    if(pEnumerator)
    {
        IWbemClassObject *pclsObj[MAX_NUMBER_HARDWARE_DEVICE_ENTRIES];
        /*HRESULT hr = */pEnumerator->Next(WBEM_INFINITE, MAX_NUMBER_HARDWARE_DEVICE_ENTRIES, &pclsObj[0], &uReturn);
        
        if(0 != uReturn && pclsObj!=NULL)
        {
            //    Iterate and total up each stick of RAM
            installedMemory_MB = 0;
            for(uint32_t i=0; i<uReturn; i++)
            {
                uint64_t stickOfRAM;
                res = GetProperty_UINT64(pclsObj[i], L"Capacity", stickOfRAM);
                installedMemory_MB += stickOfRAM;
                COM_SAFE_RELEASE(pclsObj[i]);
            }
            installedMemory_MB /= 1024 * 1024; // convert in MegaBytes
            
#ifdef _DEBUG
            //EA_TRACE_FORMATTED(("installedMemory_MB: %I64u MB\n", installedMemory_MB)); 
#endif
        }
        else
            res=SYSINFO_GET_SYSINFO_PROPERTY_ERROR;
        
        COM_SAFE_RELEASE(pEnumerator)
    }
    else
        res=SYSINFO_GET_SYSINFO_PROPERTY_ERROR;
    
    return res;
}

SYSTEM_INFORMATION_ERROR_TYPES SystemInformation::Retrieve_Win32_ComputerName_DATA(IWbemServices *pSvc)
{
    SYSTEM_INFORMATION_ERROR_TYPES res=SYSINFO_NO_ERROR;
    IEnumWbemClassObject* pEnumerator = NULL;
    ULONG uReturn = 0;
    
    DoQuery(pSvc, pEnumerator, "SELECT Name FROM Win32_ComputerSystem");    
    if(pEnumerator)
    {
        IWbemClassObject *pclsObj;
        pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);
        
        if(0 != uReturn && pclsObj!=NULL)
        {
            GetProperty_STRING(pclsObj, L"Name", computerName); // Computer Name
            computerName.trim();
            COM_SAFE_RELEASE(pclsObj);
        }
        else
            res=SYSINFO_GET_SYSINFO_PROPERTY_ERROR;
        
        COM_SAFE_RELEASE(pEnumerator)
    }
    else
        res=SYSINFO_GET_SYSINFO_PROPERTY_ERROR;
    
    return SYSINFO_NO_ERROR;
}

SYSTEM_INFORMATION_ERROR_TYPES SystemInformation::Retrieve_Win32_OperatingSystem_DATA(IWbemServices *pSvc)
{
    SYSTEM_INFORMATION_ERROR_TYPES res=SYSINFO_NO_ERROR;
    IEnumWbemClassObject* pEnumerator = NULL;
    ULONG uReturn = 0;

    // Win32_OperatingSystem
    DoQuery(pSvc, pEnumerator, "SELECT BuildNumber,InstallDate,Locale,OSLanguage,SerialNumber,ServicePackMajorVersion,ServicePackMinorVersion,Version FROM Win32_OperatingSystem");
    if(pEnumerator)
    {
        IWbemClassObject *pclsObj=NULL;
        /*HRESULT hr = */pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);
        
        if(0 != uReturn && pclsObj!=NULL)
        {
            //    OS Locale
            eastl::string8 dummyStr8;
            GetProperty_STRING(pclsObj, L"Locale", dummyStr8);
            UserLocale= static_cast<uint16_t>(EA::StdC::StrtoI32(dummyStr8.c_str(), NULL, 16));
            
            // OS Install Date
            GetProperty_STRING(pclsObj, L"InstallDate", OSInstallDate);
            OSInstallDate.trim();

            // OS Serial Number
            GetProperty_STRING(pclsObj, L"SerialNumber", OSSerialNumber);
            OSSerialNumber.trim();

            //  OS Language
            res = GetProperty_UINT32(pclsObj, L"OSLanguage", OSLocale);

            //  OS Version
            GetProperty_STRING(pclsObj, L"BuildNumber", OSBuildString);
            GetProperty_STRING(pclsObj, L"Version", OSVersion);
            uint16_t servicepackMajor;
            GetProperty_UINT16(pclsObj, L"ServicePackMajorVersion", servicepackMajor);
            uint16_t servicepackMinor;
            GetProperty_UINT16(pclsObj, L"ServicePackMinorVersion", servicepackMinor);
            OSServicePack.append_sprintf("%d.%d", (int) servicepackMajor, (int) servicepackMinor);

            
#ifdef _DEBUG        
            //EA_TRACE_FORMATTED(("UserLocale %x (%s)\n", UserLocale, GetUserLocale())); 
            //EA_TRACE_FORMATTED(("OSLocale %x (%s)\n", OSLocale, GetOSLocale())); 
#endif
            
            //    Determine if OS is 32 or 64-bit
            SYSTEM_INFO SystemInfo;
            GetNativeSystemInfo(&SystemInfo);
            OSArchitecture = (SystemInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64) ? 64 : 32;
            
            COM_SAFE_RELEASE(pclsObj);
        }
        else
            res=SYSINFO_GET_SYSINFO_PROPERTY_ERROR;
        
        COM_SAFE_RELEASE(pEnumerator)
    }
    else
        res=SYSINFO_GET_SYSINFO_PROPERTY_ERROR;
    

    // Win32_BaseBoard:  Determine if we are running in virtual environment (ie: virtualpc, vmware, virtualbox)
    DoQuery(pSvc, pEnumerator, "SELECT Manufacturer,SerialNumber FROM Win32_BaseBoard");
    if(pEnumerator)
    {
        IWbemClassObject *pclsObj=NULL;
        pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);

        if(0 != uReturn && pclsObj!=NULL)
        {
            //    Get motherboard string
            GetProperty_STRING(pclsObj, L"Manufacturer", motherboardString);
            // Get motherboard serial number
            res = GetProperty_STRING(pclsObj, L"SerialNumber", motherboardSerialNumber);
            motherboardString.trim();
            motherboardSerialNumber.trim();
        }

        COM_SAFE_RELEASE(pEnumerator)
    }
    return res;
}



//
SYSTEM_INFORMATION_ERROR_TYPES SystemInformation::Retrieve_Win32_Processor_DATA(IWbemServices *pSvc)
{
    SYSTEM_INFORMATION_ERROR_TYPES res=SYSINFO_NO_ERROR;
    IEnumWbemClassObject* pEnumerator = NULL;
    ULONG uReturn = 0;
    // Win32_Processor
    // Note that a SELECT * here will take 2 seconds (!) so be careful about
    // which fields we request
    //    Refer to:  http://msdn.microsoft.com/en-us/library/aa394373(v=VS.85).aspx
    DoQuery(pSvc, pEnumerator, "SELECT Name,MaxClockSpeed,NumberOfLogicalProcessors,NumberOfCores FROM Win32_Processor");    
    if(pEnumerator)
    {
        IWbemClassObject *pclsObj[MAX_NUMBER_HARDWARE_DEVICE_ENTRIES];
        /*HRESULT hr = */pEnumerator->Next(WBEM_INFINITE, MAX_NUMBER_HARDWARE_DEVICE_ENTRIES, &pclsObj[0], &uReturn);
        
        if(0 != uReturn && pclsObj!=NULL)
        {
            GetProperty_STRING(pclsObj[0], L"Name", cpuDescription);
            
            while(1)    // replace spaces
            {
                eastl::string16::size_type pos=cpuDescription.find("  ");
                if(pos==eastl::string8::npos)
                    break;
                else
                    cpuDescription = cpuDescription.replace(pos, 1, "");
            }
            
            //    Get the CPU clock speed
            uint32_t cpuSpeed;
            res = GetProperty_UINT32(pclsObj[0], L"MaxClockSpeed", cpuSpeed);
            float cpuSpeedFloat((float)cpuSpeed);
            cpuSpeedFloat /= 1000.f;
            char8_t cpuSpeedString[32];
            EA::StdC::Ftoa(cpuSpeedFloat, cpuSpeedString, sizeof(cpuSpeedString) - 1, 3, false);
            cpuGHz = cpuSpeedString;
            
            //    Number of CPUs based on return value
            numCPUs = uReturn;
            
            //    Determine if hyperthreading is enabled by comparing NumberOfLogicalProcessors 
            //    and NumberOfCores. If hyperthreading is enabled in the BIOS for the processor, 
            //    then NumberOfCores is less than NumberOfLogicalProcessors.
            GetProperty_UINT32(pclsObj[0], L"NumberOfLogicalProcessors", numberOfLogicalProcessors);
            GetProperty_UINT32(pclsObj[0], L"NumberOfCores", numberOfCores);
            if (numberOfLogicalProcessors > numberOfCores)
                instructionSets = ",HT";
            
            //    Used CPUID instruction to extract processor info
            //    Refer to:  http://msdn.microsoft.com/en-us/library/hskdteyh.aspx
            int CPUInfo[4];
            __cpuid(CPUInfo, 1);
            if (CPUInfo[3] & (1 << 23))
                instructionSets += ",MMX";
            if (CPUInfo[3] & (1 << 25))
                instructionSets += ",SSE";
            if (CPUInfo[3] & (1 << 26))
                instructionSets += ",SSE2";
            if (CPUInfo[2] & (1 << 0))
                instructionSets += ",SSE3";
            if (CPUInfo[2] & (1 << 19))
                instructionSets += ",SSE41";
            if (CPUInfo[2] & (1 << 20))
                instructionSets += ",SSE42";
            
            //  Detect hypervisor
            // call __cpuid to check if a Hypervisor is present.
            if ((CPUInfo[3] >> 31) & 1)
            {
                // Hypervisor is present
                // Check CPUID leaf 0x40000000 EBX, ECX, EDX for Hypervisor prod name
                __cpuid(CPUInfo, 0x40000000);
                char HVProduct[13];
                memset(HVProduct, 0, sizeof(HVProduct));
                memcpy(HVProduct, CPUInfo + 1, 12);

                if (!strcmp(HVProduct, "Microsoft Hv"))
                {
                    // Check CPUID leaf 0x40000003 bit 1 (CreatePartitions bit)
                    __cpuid(CPUInfo, 0x40000003);
                    if (CPUInfo[1] & 1) 
                    {
                        Hypervisor = Hypervisor_VMRoot;
                    }
                    else
                    {
                        Hypervisor = Hypervisor_VMChild;
                    }
                }
                else 
                    Hypervisor = Hypervisor_VM;
            }
            else 
                Hypervisor = Hypervisor_Native;

            __cpuid(CPUInfo, 0x80000001);
            if (CPUInfo[2] & (1 << 6))
                instructionSets += ",SSE4a";
            
            //    Fix instruction set string formatting, remove initial comma if necessary
            if (instructionSets[0] == ',')
                instructionSets = instructionSets.replace(0, 1, "");

            //    Release the memory from the query
            for (uint32_t i=0; i<uReturn; i++)
                COM_SAFE_RELEASE(pclsObj[i]);
        }
        else
            res=SYSINFO_GET_SYSINFO_PROPERTY_ERROR;
        
        COM_SAFE_RELEASE(pEnumerator)
    }
    else
        res=SYSINFO_GET_SYSINFO_PROPERTY_ERROR;

    //  Get the BIOS string
    //  Initially requested by Darren Su to find out how many people using PC client on Mac hardware
    HKEY hKey;
    DWORD retCode;
    eastl::string8 BIOSString;
    retCode = ::RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("HARDWARE\\DESCRIPTION\\System"), 0, KEY_QUERY_VALUE, &hKey);

    biosString = "Unknown";  // Default value for errors
    if (retCode == ERROR_SUCCESS)
    {
        DWORD datatype = REG_SZ;
        char16_t buffer[256];
        DWORD bufferlength = sizeof(buffer);
        retCode = ::RegQueryValueEx(hKey, TEXT("SystemBiosVersion"), NULL, &datatype, (LPBYTE) &buffer, &bufferlength);
        if (retCode == ERROR_SUCCESS)
        {
            //  Replace multi-line null terminators
            for (uint32_t i = 0; i < (bufferlength/2) - 2; i++)
            {
                if (buffer[i] == 0)
                {
                    buffer[i] = ' ';
                }
            }
            eastl::string16 tmpValue;
            tmpValue.assign(reinterpret_cast<char16_t*>(buffer));
            biosString = EA::StdC::ConvertString<eastl::string16, eastl::string8>(tmpValue);
            biosString.trim();
        }
    }
    RegCloseKey(hKey);

    return res;
}

SYSTEM_INFORMATION_ERROR_TYPES SystemInformation::Retrieve_Win32_Bios_DATA(IWbemServices *pSvc)
{
    SYSTEM_INFORMATION_ERROR_TYPES res=SYSINFO_NO_ERROR;
    IEnumWbemClassObject* pEnumerator = NULL;
    ULONG uReturn = 0;
    
    DoQuery(pSvc, pEnumerator, "SELECT Manufacturer,SerialNumber FROM Win32_Bios");    
    if(pEnumerator)
    {
        IWbemClassObject *pclsObj;
        pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);
        
        if(0 != uReturn && pclsObj!=NULL)
        {
            GetProperty_STRING(pclsObj, L"Manufacturer", biosManufacturer); // Manufacturer
            GetProperty_STRING(pclsObj, L"SerialNumber", biosSerialNumber); // Serial Number
            biosManufacturer.trim();
            biosSerialNumber.trim();
            COM_SAFE_RELEASE(pclsObj);
        }
        else
            res=SYSINFO_GET_SYSINFO_PROPERTY_ERROR;
        
        COM_SAFE_RELEASE(pEnumerator)
    }
    else
        res=SYSINFO_GET_SYSINFO_PROPERTY_ERROR;
    
    return res;
}

SYSTEM_INFORMATION_ERROR_TYPES SystemInformation::Retrieve_Win32_LogicalDisk_DATA(IWbemServices *pSvc)
{
    SYSTEM_INFORMATION_ERROR_TYPES res=SYSINFO_NO_ERROR;
    IEnumWbemClassObject* pEnumerator = NULL;
    ULONG uReturn = 0;
    
    //    Win32_DiskDrive:  Gets the actual disk drive hardware and not the partitions.
    //    NOTES:  Seems to only return harddrives, which is good, but aren't CDROM and floppy 
    //    disk drives also?
    DoQuery(pSvc, pEnumerator, "SELECT InterfaceType,MediaType,Model,Size,SerialNumber FROM Win32_DiskDrive");    
    if(pEnumerator)
    {
        IWbemClassObject *pclsObj[MAX_NUMBER_HARDWARE_DEVICE_ENTRIES];
        /*HRESULT hr = */pEnumerator->Next(WBEM_INFINITE, MAX_NUMBER_HARDWARE_DEVICE_ENTRIES, &pclsObj[0], &uReturn);
        
        if(0 != uReturn && pclsObj!=NULL)
        {
            for(uint32_t i=0; i<uReturn; i++)
            {
                eastl::string8 dummyStr8;
                GetProperty_STRING(pclsObj[i], L"InterfaceType", dummyStr8); // IDE
                hddInterface[i] = dummyStr8;
                GetProperty_STRING(pclsObj[i], L"MediaType", dummyStr8); // Fixed hard disk media
                hddType[i] = dummyStr8;
                GetProperty_STRING(pclsObj[i], L"Model", dummyStr8); // ST3250820AS
                hddName[i] = dummyStr8;
                GetProperty_STRING(pclsObj[i], L"SerialNumber", dummyStr8); // ST3250820AS
                dummyStr8.trim();
                hddSerialNumber[i] = dummyStr8;

                res = GetProperty_UINT64(pclsObj[i], L"Size", totalHDDSpace_GB[i]);
                totalHDDSpace_GB[i] /= (1024 * 1024 * 1024); // convert in Gigabytes
                COM_SAFE_RELEASE(pclsObj[i]);
            }
            
            numHDDs=uReturn;
        }
        else
            res=SYSINFO_GET_SYSINFO_PROPERTY_ERROR;
        
        COM_SAFE_RELEASE(pEnumerator)
    }
    else
        res=SYSINFO_GET_SYSINFO_PROPERTY_ERROR;
    
    return res;
}

SYSTEM_INFORMATION_ERROR_TYPES SystemInformation::Retrieve_Win32_OpticalDrive(IWbemServices *pSvc)
{
    SYSTEM_INFORMATION_ERROR_TYPES res=SYSINFO_NO_ERROR;
    IEnumWbemClassObject* pEnumerator = NULL;
    ULONG uReturn = 0;
    
    //    Win32_DiskDrive:  Gets the actual disk drive hardware and not the partitions.
    //    NOTES:  Seems to only return harddrives, which is good, but aren't CDROM and floppy 
    //    disk drives also?
    DoQuery(pSvc, pEnumerator, "SELECT MediaType,Name FROM Win32_CDROMDrive");    
    if(pEnumerator)
    {
        IWbemClassObject *pclsObj[MAX_NUMBER_HARDWARE_DEVICE_ENTRIES];
        pEnumerator->Next(WBEM_INFINITE, MAX_NUMBER_HARDWARE_DEVICE_ENTRIES, &pclsObj[0], &uReturn);
        
        if(0 != uReturn && pclsObj!=NULL)
        {
            for (uint32_t i=0; i<uReturn; i++)
            {
                eastl::string8 dummyStr8;
                
                GetProperty_STRING(pclsObj[i], L"MediaType", dummyStr8); // DVD-ROM
                oddType[i] = dummyStr8;
                GetProperty_STRING(pclsObj[i], L"Name", dummyStr8); // HL-DT-ST DVD-ROM GDR8164B ATA Device
                oddName[i] = dummyStr8;
                
                COM_SAFE_RELEASE(pclsObj[i]);
            }
            
            numODDs = uReturn;
        }
        else
            res=SYSINFO_GET_SYSINFO_PROPERTY_ERROR;
        
        COM_SAFE_RELEASE(pEnumerator)
    }
    else
        res=SYSINFO_GET_SYSINFO_PROPERTY_ERROR;
    
    return res;
}

BOOL CALLBACK enumMonitorsCallback(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData)
{
    SystemInformation* sysInfo = reinterpret_cast<SystemInformation*>(dwData);
    sysInfo->AddDisplay(lprcMonitor->right - lprcMonitor->left, lprcMonitor->bottom - lprcMonitor->top);
    return TRUE;
}

SYSTEM_INFORMATION_ERROR_TYPES SystemInformation::Retrieve_Win32_DesktopMonitor_DATA(IWbemServices *pSvc)
{
    ::EnumDisplayMonitors(NULL, NULL, enumMonitorsCallback, (LPARAM)this);

    return SYSINFO_NO_ERROR;
}

SYSTEM_INFORMATION_ERROR_TYPES SystemInformation::Retrieve_Login_DATA(IWbemServices *pSvc)
{
    SYSTEM_INFORMATION_ERROR_TYPES res=SYSINFO_NO_ERROR;
    IEnumWbemClassObject* pEnumerator = NULL;
    ULONG uReturn = 0;
    
    DoQuery(pSvc, pEnumerator, "SELECT Caption,StartTime FROM Win32_LogonSession");    
    if(pEnumerator)
    {
        IWbemClassObject *pclsObj[MAX_NUMBER_HARDWARE_DEVICE_ENTRIES];
        /*HRESULT hr = */pEnumerator->Next(WBEM_INFINITE, MAX_NUMBER_HARDWARE_DEVICE_ENTRIES, &pclsObj[0], &uReturn);
        
        if(0 != uReturn && pclsObj!=NULL)
        {
            for (uint32_t i=0; i<uReturn; i++)
            {
                eastl::string8 dummyStr8;
                GetProperty_STRING(pclsObj[i], L"Caption", dummyStr8);
                res = GetProperty_STRING(pclsObj[i], L"StartTime", dummyStr8);
                COM_SAFE_RELEASE(pclsObj[i]);
            }
        }
        else
            res=SYSINFO_GET_SYSINFO_PROPERTY_ERROR;
        
        COM_SAFE_RELEASE(pEnumerator)
    }
    else
        res=SYSINFO_GET_SYSINFO_PROPERTY_ERROR;

    
    return res;
}

uint32_t ConvertHexStringToUint(const eastl::string8& str)
{
    uint32_t value = 0;
    
    for (eastl::string8::size_type i = 0; i < str.size(); ++i)
    {
        char8_t c = str[i];
        if ((c >= '0') && (c <= '9'))
            c -= '0';
        else if ((c >= 'a') && (c <= 'z'))
            c = c - 'a' + 10;
        else if ((c >= 'A') && (c <= 'Z'))
            c = c - 'A' + 10;
        else
            return 0; // Invalid character
        
        value = (value << 4) + c;
    }
    
    return value;
}

SYSTEM_INFORMATION_ERROR_TYPES SystemInformation::Retrieve_Win32_VideoController_DATA(IWbemServices *pSvc)
{
    //    Get the virtual resolution of screen
    virtualResX = ::GetSystemMetrics(SM_CXVIRTUALSCREEN);
    virtualResY = ::GetSystemMetrics(SM_CYVIRTUALSCREEN);
    
    //    Get installed DirectX version
    HMODULE handle = 0;
    if ((handle = LoadLibrary(L"d3d11.dll")) != NULL)
    {
        installedDirectX = "11";
    }
    else if ((handle = LoadLibrary(L"d3d10_1.dll"))!= NULL)
    {
        installedDirectX = "10.1";
    }
    else if ((handle = LoadLibrary(L"d3d10.dll")) != NULL)
    {
        installedDirectX = "10";
    }
    else if ((handle = LoadLibrary(L"d3d9.dll")) != NULL)
    {
        installedDirectX = "9";
    }
    else if ((handle = LoadLibrary(L"d3d8.dll")) != NULL)
    {
        installedDirectX = "8";
    }
    if (handle != 0)
    {
        FreeLibrary(handle);
    }
    
    SYSTEM_INFORMATION_ERROR_TYPES res=SYSINFO_NO_ERROR;
    IEnumWbemClassObject* pEnumerator = NULL;
    ULONG uReturn = 0;
    
    // Win32_VideoController
    DoQuery(pSvc, pEnumerator, "SELECT Name,DriverVersion,AdapterRAM,CurrentBitsPerPixel,PNPDeviceID FROM Win32_VideoController WHERE Availability=3");    
    if(pEnumerator)
    {
        IWbemClassObject *pclsObj[MAX_NUMBER_HARDWARE_DEVICE_ENTRIES];
        /*HRESULT hr = */pEnumerator->Next(WBEM_INFINITE, MAX_NUMBER_HARDWARE_DEVICE_ENTRIES, &pclsObj[0], &uReturn);
        
        if(0 != uReturn && pclsObj!=NULL)
        {
            for(uint32_t i=0; i<uReturn; i++)
            {
                //    "PCI\VEN_10DE&DEV_0191&SUBSYS_039C10DE&REV_A2\4&2270B39&0&0020"
                //    Assumption that vendorID and deviceID is always 4 characters long
                eastl::string8 pnpDeviceId;
                GetProperty_STRING(pclsObj[i], L"PNPDeviceID", pnpDeviceId);
                
                //    Get the vendor ID
                eastl::string8::size_type vendorIndex = pnpDeviceId.find("VEN_");
                if (pnpDeviceId.size() > (vendorIndex + 8))
                {
                    eastl::string8 vendorString = pnpDeviceId.substr(vendorIndex + 4, 4);
                    videoVendorID[i] = ConvertHexStringToUint(vendorString);
                }
                
                //    Get the device ID
                eastl::string8::size_type devIndex = pnpDeviceId.find("DEV_");
                if (pnpDeviceId.size() > (devIndex + 8))
                {
                    eastl::string8 deviceString = pnpDeviceId.substr(devIndex + 4, 4);
                    videoDeviceID[i] = ConvertHexStringToUint(deviceString);
                }
                
                GetProperty_STRING(pclsObj[i], L"Name", videoAdapterName[i]);
                // we use this generic way to get a driver version, not a vendor specific ways like nvapi or amdags or dxdiag... (they need extra DLL's and could be very very slow!)
                GetProperty_STRING(pclsObj[i], L"DriverVersion", videoAdapterDriverVersion[i]);

                //    Note that some cards uses system RAM for video RAM so this could be misleading
                GetProperty_UINT32(pclsObj[i], L"AdapterRAM", videoAdapterRAM[i]);
                videoAdapterRAM[i]/=1048576; // convert in MegaBytes
                res = GetProperty_UINT32(pclsObj[i], L"CurrentBitsPerPixel", videoBitsPerPixel[i]);
                
#ifdef _DEBUG
                //EA_TRACE_FORMATTED(("Video Adapter %i \"%s\":\t%lu MB\t%i bits\n", i, videoAdapterName[i].c_str(), videoAdapterRAM[i], videoBitsPerPixel[i])); 
#endif
                COM_SAFE_RELEASE(pclsObj[i]);
            }
            
            numVideoController=uReturn;
        }
        else
            res=SYSINFO_GET_SYSINFO_PROPERTY_ERROR;
        
        COM_SAFE_RELEASE(pEnumerator)
    }
    else 
        res=SYSINFO_GET_SYSINFO_PROPERTY_ERROR;
    
    return res;
}

SYSTEM_INFORMATION_ERROR_TYPES SystemInformation::Retrieve_Win32_VideoCaptureDevices()
{
    // Use COINIT_MULTITHREADED because we already have a COM thread initalized
    HRESULT hr = CoInitializeEx(0, COINIT_MULTITHREADED);
    numVideoCaptureDevice = 0;
    if (SUCCEEDED(hr))
    {
        IEnumMoniker *pEnum;
        // Grab all video capture devices
        hr = EnumerateDevices(CLSID_VideoInputDeviceCategory, &pEnum);
        
        // There are video capture devices connected. Grab a list of all of them.
        if (SUCCEEDED(hr))
        {
            EnumerateVideoCaptureDevices(pEnum);
            pEnum->Release();
        }
    }
    return SYSINFO_NO_ERROR;
}

HRESULT SystemInformation::EnumerateDevices(REFGUID category, IEnumMoniker **ppEnum)
{
    // Create the System Device Enumerator.
    ICreateDevEnum *pDevEnum;
    HRESULT hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL,
                                  CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pDevEnum));
    
    if (SUCCEEDED(hr))
    {
        // Create an enumerator for the category.
        hr = pDevEnum->CreateClassEnumerator(category, ppEnum, 0);
        if (hr == S_FALSE)
        {
            hr = VFW_E_NOT_FOUND;  // The category is empty. Treat as an error.
        }
        pDevEnum->Release();
    }
    return hr;
}

void SystemInformation::EnumerateVideoCaptureDevices(IEnumMoniker *pEnum)
{
    IMoniker *pMoniker = NULL;
    while (pEnum->Next(1, &pMoniker, NULL) == S_OK)
    {
        IPropertyBag *pPropBag;
        HRESULT hr = pMoniker->BindToStorage(0, 0, IID_PPV_ARGS(&pPropBag));
        if (FAILED(hr))
        {
            pMoniker->Release();
            continue;
        }
        
        VARIANT var;
        VariantInit(&var);
        
        // Get description or friendly name.
        hr = pPropBag->Read(L"Description", &var, 0);
        if (FAILED(hr))
        {
            hr = pPropBag->Read(L"FriendlyName", &var, 0);
        }
        if (SUCCEEDED(hr))
        {
            char *videoCaptureName = _com_util::ConvertBSTRToString(var.bstrVal);
            videoCaptureDevice[numVideoCaptureDevice] = videoCaptureName;
            numVideoCaptureDevice++;
            delete[] videoCaptureName;
            
        }
        pPropBag->Release();
        pMoniker->Release();
    }
}


#elif defined(ORIGIN_MAC)
// Define some errors.
struct WrongTypeID: public std::runtime_error
{
    WrongTypeID( const std::string& what ): std::runtime_error( what ) {}
};

struct ItemNotFound: public std::runtime_error
{
    ItemNotFound( const std::string& what ): std::runtime_error( what ) {}
};

///
/// PropertyList wraps any of the following types, and can be used to wrap a CFPropertyList:
///          CFArray, CFDictionary, CFString, CFNumber (Real or Float), CFBoolean, CFData
///
/// NOTE: PropertyList uses exceptions for error handling, so it is important to wrap usage
/// in a try/catch block to avoid exceptions leaking out beyond intended.  All thrown exceptions
/// derive from std::exception (or use catch (...) ).
///
class PropertyList
{
    CFErrorRef error_; // normally left uninitialized, only used when failing construction
    CFPropertyListRef object_;
    bool callRelease_;
    
public:
    PropertyList( const std::string& xmlPropertyList ); // Parse and wrap the resulting CFPropertyListRef
    PropertyList( CFPropertyListRef object, bool callRelease = false ); // Blindly wrap the supplied object, calling CFRelease if callRelease==true.
    
    PropertyList( const PropertyList& rhs );
    PropertyList& operator=( const PropertyList& rhs );
    
    ~PropertyList();
    
    // Creates a PropertyList to wrap the supplied std::string as a CFString;
    static PropertyList wrapCFString( std::string value );
    
    // Returns the dictionary (within an array) containing the supplied key/value.
    PropertyList dictionaryMatch( std::string key, std::string value );
    
    // Returns the value (within a dictionary) associated with the supplied key.
    PropertyList value( std::string key );
    
    // Returns the count (within an array) of items.
    size_t arraySize();
    
    // Returns the item (within an array) at the given index.
    PropertyList index( size_t index );
    
    // Test if this PropertyList is of a given type.
    bool isString();
    bool isReal();
    bool isFloat();
    bool isBoolean();
    
    // Return the value of this PropertyList as a given type.
    std::string string();
    int real();
    long long longReal();
    bool boolean();
    
    CFStringRef getCFStringRef();
};


PropertyList::PropertyList( CFPropertyListRef plist, bool callRelease )
: object_( plist )
, callRelease_( callRelease )
{
    if ( object_ == 0 )
        throw std::runtime_error( "PropertyList::PropertyList(): NULL object." );
}

PropertyList::PropertyList( const std::string& xmlPropertyList )
    : object_( CFPropertyListCreateWithData( kCFAllocatorDefault, ( CFDataRef ) PropertyList( CFDataCreateWithBytesNoCopy( kCFAllocatorDefault, (unsigned char*)xmlPropertyList.c_str(), xmlPropertyList.size(), kCFAllocatorNull ), true ).object_, kCFPropertyListImmutable, NULL, &error_ ) )
    , callRelease_( true )
{
    // Bail if property list creation failed.
    if ( object_ == 0 )
    {
        std::string message = "PropertyList::PropertyList(): error creating property list: ";
        try {
            message.append( PropertyList( (CFPropertyListRef) CFErrorCopyDescription( error_ ), true ).string() );
        } catch (...) {
            message.append( "unknown error." );
        }
        CFRelease(error_); // error_ is only valid if we get into this block; it can be safely ignored elsewhere.
        throw std::runtime_error( message );
    }
}

PropertyList::PropertyList( const PropertyList& rhs )
: error_( rhs.error_ )
, object_( rhs.object_ )
, callRelease_( rhs.callRelease_ )
{
    if ( callRelease_ )
        object_ = (CFPropertyListRef) CFRetain( object_ );
}

PropertyList& PropertyList::operator=( const PropertyList& rhs )
{
    if ( callRelease_ )
        CFRelease( object_ );
    
    error_ = rhs.error_;
    object_ = rhs.object_;
    callRelease_ = rhs.callRelease_;
    
    if ( callRelease_ )
        object_ = (CFPropertyListRef) CFRetain( object_ );
    
    return *this;
}

PropertyList::~PropertyList()
{
    if ( callRelease_ )
        CFRelease( object_ );
}

PropertyList PropertyList::wrapCFString( std::string value )
{
    // Create a new PropertyList to wrap the created CFString and ensure that it is released properly.
    return PropertyList( (CFPropertyListRef) CFStringCreateWithBytes(0, (const unsigned char*) value.c_str(), value.length(), kCFStringEncodingASCII, false), true );
}

PropertyList PropertyList::dictionaryMatch(std::string key, std::string value)
{
    // Bail if this object is not an array.
    if ( CFGetTypeID( object_ ) != CFArrayGetTypeID() )
        throw WrongTypeID( "PropertyList::dictionaryMatch(): object is not an array.");
    
    PropertyList searchValue = wrapCFString( value );
    
    // Iterate through the items in this array.
    CFIndex count = CFArrayGetCount( (CFArrayRef) object_ );
    for (CFIndex i = 0; i != count; ++i )
    {
        PropertyList item( (CFPropertyListRef) CFArrayGetValueAtIndex( (CFArrayRef) object_, i ) );
        
        // If the item key/value matches, return it.
        if ( CFStringCompare( (CFStringRef) item.value( key ).object_, searchValue.getCFStringRef(), 0 ) == kCFCompareEqualTo )
            return item;
    }
    
    throw ItemNotFound( "PropertyList::dictionaryMatch(): no match found." );
}

PropertyList PropertyList::value(std::string key)
{
    // Bail if this object is not a dictionary.
    if ( CFGetTypeID( object_ ) != CFDictionaryGetTypeID() ) throw WrongTypeID( "PropertyList::value(): object is not a dictionary." );
    
    // Lookup this key, if present.
    const void* value;
    Boolean keyFound = CFDictionaryGetValueIfPresent( (CFDictionaryRef) object_, wrapCFString( key ).getCFStringRef(), &value );
    
    // Bail if the key was not present.
    if ( ! keyFound ) throw ItemNotFound( "PropertyList::value(): key not found." );
    
    // Return the value matching this key.
    return PropertyList( (CFPropertyListRef) value );
}

size_t PropertyList::arraySize()
{
    // Bail if this object is not an array.
    if ( CFGetTypeID( object_ ) != CFArrayGetTypeID() ) throw WrongTypeID( "PropertyList::getDictMatching(): object is not an array.");
    
    return CFArrayGetCount( (CFArrayRef) object_ );
}

PropertyList PropertyList::index( size_t index )
{
    // Bail if this object is not an array.
    if ( CFGetTypeID( object_ ) != CFArrayGetTypeID() ) throw WrongTypeID( "PropertyList::getDictMatching(): object is not an array.");
    
    CFIndex count = CFArrayGetCount( (CFArrayRef) object_ );
    if ( (CFIndex) index < count )
        return PropertyList( (CFPropertyListRef) CFArrayGetValueAtIndex( (CFArrayRef) object_, index ) );
    
    throw ItemNotFound( "PropertyList::item(): array index out of bounds." );
}

bool PropertyList::isString() { return CFGetTypeID( object_ ) == CFStringGetTypeID(); }
bool PropertyList::isReal() { return ( CFGetTypeID( object_ ) == CFNumberGetTypeID() ) && ( ! CFNumberIsFloatType( (CFNumberRef) object_ ) ); }
bool PropertyList::isFloat() { return ( CFGetTypeID( object_ ) == CFNumberGetTypeID() ) && ( CFNumberIsFloatType( (CFNumberRef) object_ ) ); }
bool PropertyList::isBoolean() { return CFGetTypeID( object_ ) == CFBooleanGetTypeID(); }

std::string PropertyList::string()
{
    std::string val;
    if ( isString() )
    {
        const char* str = CFStringGetCStringPtr( (CFStringRef) object_, kCFStringEncodingUTF8 );
        if ( str )
            val = str;
        else
        {
            CFIndex len = CFStringGetLength( ( CFStringRef ) object_ );
            CFIndex size = CFStringGetMaximumSizeForEncoding( len, kCFStringEncodingUTF8 ) + 1;
            char *bytes = new char[ size ];
            if ( CFStringGetCString( (CFStringRef) object_, bytes, size, kCFStringEncodingUTF8 ) )
                val = std::string( bytes );
            delete[] bytes;
        }
    }
    return val;
}

int PropertyList::real()
{
    int val = 0;
    if (isReal())
        CFNumberGetValue( (CFNumberRef) object_, kCFNumberIntType, &val);
    
    return val;
}

long long PropertyList::longReal()
{
    long long val = 0;
    if (isReal())
        CFNumberGetValue( (CFNumberRef) object_, kCFNumberLongLongType, &val);
    
    return val;
}

bool PropertyList::boolean()
{
    bool val = false;
    if (CFGetTypeID(object_) == CFBooleanGetTypeID())
        val = CFBooleanGetValue( (CFBooleanRef) object_ );
    
    return val;
}

CFStringRef PropertyList::getCFStringRef()
{
    if ( isString() )
        return (CFStringRef) object_;
    else
        return 0;
}

// Returns the substring delimited between the two other supplied strings.  If either string
// is empty, it is taken to represent the beginning and end of the string respectively.  Searching begins
// at the beginning and end.  The returned string contains neither delimiting string.
std::string substrDelimited( const std::string& str, const std::string& startsWith, const std::string& endsWith )
{
    size_t start=0, end=str.length();
    
    if ( startsWith.length() )
    {
        size_t pos = str.find( startsWith, start );
        if ( pos != std::string::npos ) start = pos + startsWith.length();
    }
    
    if ( endsWith.length() )
    {
        size_t pos = str.rfind( endsWith, end );
        if ( pos != std::string::npos ) end = pos;
    }
    
    return str.substr( start, end - start );
}

// Converts the supplied hex string to an unsigned int.  The supplied string must
// be a hex string, optionally containing a leading "0x" or "0X".
unsigned intFromHexString( const std::string& string )
{
    std::string substring = string;
    char secondChar = string[ 1 ];
    if ( secondChar == 'x' || secondChar == 'X' )
        substring = string.substr( 2, std::string::npos );
    
    std::stringstream str( substring );
    unsigned val;
    str >> std::hex >> val;
    return val;
}

//  Execute a UNIX command and return its output as a string.
std::string executeCommand( const std::string& cmd)
{
    FILE* pipe = popen(cmd.c_str(), "r");
    
    if (!pipe) return NULL;
    char buffer[128];
    std::string data = "";
    while(!feof(pipe)) {
        if(fgets(buffer, 128, pipe) != NULL)
            data += buffer;
    }
    pclose(pipe);
    
    return data;
}

PropertyList propertyList_from_pascal_string(const Str255 pstr)
{
    return PropertyList(CFStringCreateWithPascalString(0, pstr, CFStringGetSystemEncoding()), true); // PropertyList will call CFRelease on the supplied CFString
}

void handleFailure( bool value, const char*description )
{
    if (! value) throw std::runtime_error(description);
}

// Populates all Apple-specific system information with the exception of:
//      macAddr[] -- see UpdateInformation()
//      defaultBrowserName -- see RetrieveDefaultBrowserName() and UpdateInformation()
void SystemInformation::Retrieve_Mac_SystemInformation_DATA()
{
    // Obtain video capture devices.
    try {
        SeqGrabComponent   videoIn;
        SGChannel videoChannel;
        SGDeviceList deviceList;
        
        // Open the QuickTime Sequence Grabber component and create a
        // channel to query for available devices.
        handleFailure( (videoIn = OpenDefaultComponent (SeqGrabComponentType, 0)), "Video Input Not Supported" );
        handleFailure( SGInitialize (videoIn)==noErr, "Video Input Disabled" );
        handleFailure( SGSetDataRef (videoIn, 0, 0, seqGrabDontMakeMovie)==noErr, "Video Subsystem Out of Memory" );
        handleFailure( SGNewChannel (videoIn, VideoMediaType, & (videoChannel))==noErr, "No Video Input Devices" );
        handleFailure( SGGetChannelDeviceList( videoChannel, sgDeviceListDontCheckAvailability | sgDeviceListIncludeInputs, &deviceList)==noErr, "System Video Input Setup Problem" );
        
        // Loop over available devices for this channel.
        for ( int device = 0; device != (*deviceList)->count; ++device )
        {
            SGDeviceName deviceNameRec = (*deviceList)->entry[device];
            SGDeviceInputList deviceInputList = deviceNameRec.inputs;
            
            // Loop over available inputs for this device.
            int inputCount=0;
            if ( deviceInputList ) inputCount = (*deviceInputList)->count;
            for ( int input = 0; input != inputCount; ++input )
            {
                
                PropertyList deviceName = propertyList_from_pascal_string((*deviceInputList)->entry[input].name);
                PropertyList inputName = propertyList_from_pascal_string(deviceNameRec.name);
                
                std::string caption = deviceName.string() + " - " + inputName.string();
                
                videoCaptureDevice[numVideoCaptureDevice] = caption.c_str();
                numVideoCaptureDevice++;
            }
        }
        
        SGDisposeDeviceList(videoIn, deviceList);
        SGDisposeChannel(videoIn, videoChannel);
        CloseComponent(videoIn);
    }
    
    // Ignore exceptions.
    catch ( std::exception& ) {}
    catch (...) {}

    // Obtain microphone inputs.
    try {
        // Prepare to get the audio device list.
        UInt32 structSize;
        AudioDeviceID *devices = NULL;
        AudioObjectPropertyAddress AllGlobalMasterDevices = { kAudioHardwarePropertyDevices, kAudioObjectPropertyScopeGlobal, kAudioObjectPropertyElementMaster };

        // Get the size of the PropertyData struct.
        handleFailure( AudioObjectGetPropertyDataSize(kAudioObjectSystemObject, &AllGlobalMasterDevices, 0, NULL, &structSize)==noErr, "Error in AudioObjectGetPropertyDataSize." );
        
        // Get the audio device list.
        UInt32 deviceCount = structSize / sizeof(AudioDeviceID);
        devices = (AudioDeviceID*) calloc(deviceCount, sizeof(AudioDeviceID));
        handleFailure( AudioObjectGetPropertyData(kAudioObjectSystemObject, &AllGlobalMasterDevices, 0, NULL, &structSize, devices)==noErr, "Error in AudioObjectGetPropertyData." );

        // Loop over all audio devices.
        for (UInt32 i=0; i < deviceCount; i++)
        {
            // Prepare to lookup the data source subtype for this device.
            UInt32 dataSource, size = sizeof(UInt32);
            AudioObjectPropertyAddress DataSourceInputMaster = { kAudioDevicePropertyDataSource, kAudioDevicePropertyScopeInput, kAudioObjectPropertyElementMaster };
            
            // If we can successfully lookup the data source subtype for this device,
            if (AudioObjectGetPropertyData(devices[i], &DataSourceInputMaster, 0, NULL, &size, &dataSource)==noErr)
            {
                // If the subtype is either internal or external microphone,
                if (dataSource == kIOAudioInputPortSubTypeInternalMicrophone ||
                    dataSource == kIOAudioInputPortSubTypeExternalMicrophone)
                {
                    // Increase the count.
                    ++microphoneConnected;
                }
            }
        }
        
        free(devices);
    }
    
    // Ignore all errors.
    catch ( std::exception& ) {}
    catch (...) {}
    
    try
    {
        // Obtain system profiler data.
        std::string systemProfilerData = executeCommand(
                                                        "/usr/sbin/system_profiler -xml "
                                                        "SPDisplaysDataType "
                                                        "SPHardwareDataType "
                                                        "SPDiscBurningDataType "
                                                        "SPNetworkDataType "
                                                        "SPSoftwareDataType" );
        
        // Obtain disk drive data
        std::string diskutilData = executeCommand( "/usr/sbin/diskutil list -plist" );
 
        // Parse the xml property lists into object trees.
        PropertyList systemProfilerPlist(systemProfilerData);
        PropertyList diskutilPlist(diskutilData);
        
        // Catch exceptions.
        try
        {
            //
            // Retrieve_Win32_OperatingSystem_DATA
            PropertyList osInfo = systemProfilerPlist
            .dictionaryMatch( "_dataType", "SPSoftwareDataType" )
            .value( "_items" )
            .dictionaryMatch( "_name", "os_overview" );
            
            //          OSDescription -- sprintf("%d.%d", osvi.dwMajorVersion, osvi.dwMinorVersion);
            OSDescription = osInfo.value( "os_version" ).string().c_str();
            //          OSServicePack -- sprintf("%d.%d", osvi.wServicePackMajor, osvi.wServicePackMinor);
            //          OSBuildString
            OSBuildString = substrDelimited( osInfo.value( "os_version" ).string(), "(", ")" ).c_str();
            //          OSVersion
            //OSVersion = substrDelimited( OSDescription.c_str(), "OS X ", " (" ).c_str();
            //          OSArchitecture -- int, 32 or 64

            // Read the OS X version from file to attempt to workaround SimCity install problem
            std::string osVersion(GetOSXVersion());
            OSVersion = eastl::string(osVersion.data());
            
            std::string arch = executeCommand( "/usr/bin/uname -m" );
            if ( arch.find( "x86_64" ) != std::string::npos ) OSArchitecture = 64;
            else if ( arch.find( "i386" ) != std::string::npos ) OSArchitecture = 32;
            else OSArchitecture = 0; // unknown architecture
        }
        
        // Ignore exceptions.
        catch ( std::exception& ) {}
        catch (...) {}

        try
        {
            //
            // Retrieve_Win32_ComputerSystem_DATA
            PropertyList hardwareInfo = systemProfilerPlist
            .dictionaryMatch( "_dataType", "SPHardwareDataType" )
            .value( "_items" )
            .dictionaryMatch( "_name", "hardware_overview" );
            
            //             installedMemory_MB -- RAM (MB)
            {
                std::stringstream mem_string( hardwareInfo.value( "physical_memory" ).string() );
                mem_string >> installedMemory_MB; // take the first space-separated value as a number
                installedMemory_MB *= 1024;
            }
            
            // Retrieve_Win32_Processor_DATA
            //          cpuDescription -- Name (spaces removed)
            cpuDescription = hardwareInfo.value( "cpu_type" ).string().c_str();
            
            //          cpuGHz -- max clock speed
            cpuGHz = substrDelimited( hardwareInfo.value( "current_processor_speed" ).string(), "", " GHz" ).c_str();
            
            //          numCPUs
            numberOfLogicalProcessors = hardwareInfo.value( "number_processors" ).real();
            numCPUs = hardwareInfo.value( "packages" ).real();
            if ( numCPUs > 0 ) numberOfCores = numberOfLogicalProcessors / numCPUs;
            
            //          instructionSets -- comma-separated list of these: HT, MMX, SSE, SSE3, SSE3, SSE41, SSE42, SSE4a
            instructionSets = "unknown";
            
            //          motherboardString -- On Mac, provide the machine model.
            motherboardString = hardwareInfo.value( "machine_model" ).string().c_str();
        }
        
        // Ignore exceptions.
        catch ( std::exception& ) {}
        catch (...) {}
        
        // Catch exceptions.
        try
        {
            //
            // Retrieve_Win32_LogicalDisk_DATA
            PropertyList wholeDisks = diskutilPlist.value( "WholeDisks" );
            PropertyList allDisksAndPartitions = diskutilPlist.value( "AllDisksAndPartitions" );
            
            // Iterate over the physical disk devices.
            size_t diskCount = wholeDisks.arraySize();
            for ( size_t i=0; i != diskCount; ++i )
            {
                std::string devName = wholeDisks.index( i ).string();
                
                // Catch exceptions.
                try {
                    // Obtain information about this disk.
                    std::string diskInfoCmd = "/usr/sbin/diskutil info -plist ";
                    diskInfoCmd.append(devName);
                    std::string xml = executeCommand( diskInfoCmd );
                    PropertyList diskInfo( xml );
                    
                    // Catch exceptions.
                    try {
                        // Check whether this disk contains the OpticalDeviceType key.
                        diskInfo.value( "OpticalDeviceType" );
                        
                        // If the key was found, skip to the next physical disk device.
                        continue;
                    }
                    
                    // Ignore Item Not Found exceptions.
                    catch (ItemNotFound&) {}
                    
                    //          numHDDs -- count of HDs
                    int j = numHDDs++;
                    
                    //          hddInterface[i] -- interface type, e.g., IDE
                    hddInterface[ j ] = diskInfo.value( "BusProtocol" ).string().c_str();
                    
                    //          hddType[i] -- media type, "Fixed hard disk media"
                    hddType[ j ] = diskInfo.value( "MediaType" ).string().c_str();
                    
                    //          hddName[i] -- Model, e.g., "ST3250820AS"
                    hddName[ j ] = diskInfo.value( "MediaName" ).string().c_str();
                    
                    //          totalHDDSpace_GB[i] --
                    totalHDDSpace_GB[ j ] = diskInfo.value( "TotalSize" ).longReal() / 1024 / 1024 / 1024;
                }
                
                // Ignore exceptions.
                catch ( std::exception& ) {}
                catch (...) {}
            }
        }
        
        // Ignore exceptions.
        catch ( std::exception& ) {}
        catch (...) {}
        
        try
        {
            //
            // Retrieve_Win32_OpticalDrive
            PropertyList opticalDisks = systemProfilerPlist
            .dictionaryMatch( "_dataType", "SPDiscBurningDataType" )
            .value( "_items" );
            
            //          numODDs -- count of ODs
            numODDs = (unsigned) opticalDisks.arraySize();
            
            for ( unsigned i = 0; i != numODDs; ++i )
            {
                //          oddType[i] -- Media Type, e.g., DVD-ROM
                if ( opticalDisks.index( i ).value( "device_readdvd" ).string() == "yes" )
                    oddType[ i ] = "DVD-ROM";
                else
                    oddType[ i ] = "CD-ROM";
                
                //          oddName[i] -- Name, e.g., "HL-DT-ST DVD-ROM GDR8164B ATA Device"
                oddName[ i ] = opticalDisks.index( i ).value( "_name" ).string().c_str();
            }
        }
        
        // Ignore exceptions.
        catch ( std::exception& ) {}
        catch (...) {}
        
        // Catch exceptions.
        try
        {
            //
            // Retrieve_Win32_DesktopMonitor_DATA
            PropertyList driverInfo = systemProfilerPlist
            .dictionaryMatch( "_dataType", "SPDisplaysDataType" )
            .value( "_items" );
            
            // Iterate over the display drivers.
            size_t driverCount = driverInfo.arraySize();
            
            //          numDisplays -- count of displays
            numDisplays = 0;
            
            //          numVideoController -- count
            numVideoController = (unsigned) driverCount;
            
            for ( size_t driver = 0; driver != driverCount; ++driver )
            {
                PropertyList driverDict = driverInfo.index( driver );
                
                //
                // Retrieve_Win32_VideoController_DATA
                //          videoVendorID[i]
                videoVendorID[ driver ] = intFromHexString( substrDelimited( driverDict.value( "spdisplays_vendor" ).string(), "(", ")" ) );
                //          videoDeviceID[i]
                videoDeviceID[ driver ] = intFromHexString( driverDict.value( "spdisplays_device-id" ).string() );
                //          videoAdapterName[i] -- name
                videoAdapterName[ driver ] = driverDict.value( "sppci_model" ).string().c_str();
                //          videoAdapterRAM[i] -- in MB
                std::stringstream vramString( driverDict.value( "spdisplays_vram" ).string() );
                vramString >> videoAdapterRAM[ driver ];
                std::string vramUnits; vramString >> vramUnits; // get units
                if ( vramUnits == "GB" ) videoAdapterRAM[ driver ] <<= 10; // if units == GB, convert to MB.
                //          videoBitsPerPixel[i] -- current bits per pixel
                videoBitsPerPixel[ driver ] = 32; // minimum for Mac OS X 10.6 Snow Leopard and following

                // Catch any errors.
                try
                {
                    // Prepare to iterate over the displays for this driver.
                    PropertyList displays = driverDict.value( "spdisplays_ndrvs" );
                    size_t displayCount = displays.arraySize();
                    
                    // Iterate over the displays for this driver.
                    for ( size_t display = 0; display != displayCount; ++display )
                    {
                        // Prepare to extract the resolution.
                        std::stringstream resolution( displays.index( display ).value( "spdisplays_resolution" ).string() );
                        char x;
                        
                        //          displayResX[i] -- screen width
                        resolution >> displayResX[ display ];
                        resolution >> x;
                        
                        //          displayResY[i] -- screen height
                        resolution >> displayResY[ display ];
                    }
                    
                    numDisplays += (unsigned) displayCount;
                }
                
                // Ignore exceptions.
                catch ( std::exception& ) {}
                catch (...) {}
            }
        }
        
        // Ignore exceptions.
        catch ( std::exception& ) {}
        catch (...) {}
    }
    
    // Ignore exceptions.
    // NOTE: it is important to wrap PropertyList objects in a try/catch block
    // to avoid exceptions leaking out beyond this function.
    catch ( std::exception& ) {}
    catch (...) {}
}
#elif defined(__linux__)
// TBD
#else
#  error "Need platform specific implementation."
#endif


SYSTEM_INFORMATION_ERROR_TYPES SystemInformation::RetrieveDefaultBrowserName(eastl::string8 &value)
{    
#if defined(ORIGIN_PC)
    // default browser
    HKEY hKey=NULL;
    char16_t szValue[1024]={0};
    DWORD dwType = REG_SZ;
    DWORD dwSize = sizeof(szValue)/sizeof(szValue[0]);
    
    LONG returnStatus = RegOpenKeyEx(HKEY_CLASSES_ROOT, L"\\http\\shell\\open\\command", 0L, KEY_QUERY_VALUE, &hKey);
    if( returnStatus == ERROR_SUCCESS )
    {
        returnStatus = RegQueryValueEx(hKey, NULL, NULL, &dwType, (LPBYTE)&szValue, &dwSize);
        if( returnStatus == ERROR_SUCCESS )
        {
            // cut out exe file name
            
            eastl::string16 exeFile = szValue;
            
            eastl::string16::size_type pos=exeFile.find_last_of(L"\\");
            if(pos!=eastl::string16::npos)
            {
                exeFile=exeFile.substr(exeFile.length()>pos+1 ? pos+1 : pos, exeFile.length());
                pos=exeFile.find_last_of(L".");
                if(pos!=eastl::string16::npos)
                    exeFile=exeFile.substr(0, pos);
            }
            else
            {
                RegCloseKey(hKey);
                return SYSINFO_GET_SYSINFO_PROPERTY_ERROR;
            }
            
            value = EA::StdC::ConvertString<eastl::string16, eastl::string8>(exeFile);
        }
        else
            return SYSINFO_GET_SYSINFO_PROPERTY_ERROR;
        
        RegCloseKey(hKey);
    }
    else
        return SYSINFO_GET_SYSINFO_PROPERTY_ERROR;
    
#ifdef _DEBUG
    //EA_TRACE_FORMATTED(("Default browser: %s\n", defaultBrowserName.c_str()));
#endif
    
#elif defined(ORIGIN_MAC)

    try
    {
        // The default browser is defined as the default application for handling 'http' URL schemes.
        PropertyList browser( LSCopyDefaultHandlerForURLScheme( CFSTR( "http" ) ), true );
        value = browser.string().c_str(); // On Mac, the default browser is identified by its bundle ID, e.g., "com.apple.Safari".
    }
    catch ( std::exception& )
    {
        // NOTE: it is important to wrap PropertyList objects in a try/catch block
        // to avoid exceptions leaking out beyond this function.
        return SYSINFO_GET_SYSINFO_PROPERTY_ERROR;
    }
    
#elif defined(__linux__)
// TBD
#else
#error "Requires implementation."
#endif
    
    return SYSINFO_NO_ERROR;
}

uint64_t SystemInformation::createHardwareSurveyChecksum()
{
    uint64_t checkSum=0;
    
    checkSum += mMacAddressHash;
    
    //    CPU information
    checkSum += NonQTOriginServices::Utilities::calculateFNV1A(cpuDescription.c_str());
    checkSum += numCPUs;
    checkSum += numberOfLogicalProcessors;
    checkSum += numberOfCores;
    checkSum += NonQTOriginServices::Utilities::calculateFNV1A(cpuGHz.c_str());
    checkSum += NonQTOriginServices::Utilities::calculateFNV1A(instructionSets.c_str());
    
    //    Memory
    checkSum += installedMemory_MB;
    
    //    OS
    checkSum += NonQTOriginServices::Utilities::calculateFNV1A(OSVersion.c_str());
    checkSum += OSArchitecture;
    checkSum += NonQTOriginServices::Utilities::calculateFNV1A(OSDescription.c_str());
    checkSum += NonQTOriginServices::Utilities::calculateFNV1A(OSServicePack.c_str());
    
    //    Locale
    checkSum += OSLocale;
    checkSum += UserLocale;
    
    //    HDD
    checkSum += numHDDs;
    for(uint32_t i=0; i<numHDDs; i++)
    {
        checkSum += totalHDDSpace_GB[i];
    }
    
    //    Display
    checkSum += numDisplays;
    for(uint32_t i=0; i<numDisplays; i++)
    {
        checkSum += displayResX[i];
        checkSum += displayResY[i];
    }
    
    //    Video
    checkSum += numVideoController;
    for(uint32_t i=0; i<numVideoController; i++)
    {
        checkSum += NonQTOriginServices::Utilities::calculateFNV1A(videoAdapterName[i].c_str());
        checkSum += NonQTOriginServices::Utilities::calculateFNV1A(videoAdapterDriverVersion[i].c_str());
        checkSum += videoAdapterRAM[i];
        checkSum += videoBitsPerPixel[i];
    }
    
    // Webcam
    checkSum += numVideoCaptureDevice;
    for(uint32_t i=0; i<numVideoCaptureDevice; i++)
    {
        checkSum += NonQTOriginServices::Utilities::calculateFNV1A(videoCaptureDevice[i].c_str());
    }

    //    Virtual resolution
    checkSum += virtualResX;
    checkSum += virtualResY;
    
    //    Microphone
    checkSum += microphoneConnected;
    
    //    Network
    checkSum += networkSpeed;
    
    //    Browser
    checkSum += NonQTOriginServices::Utilities::calculateFNV1A(defaultBrowserName.c_str());
    
#ifdef _DEBUG
    //EA_TRACE_FORMATTED(("HW/SW checksum: %I64x\n", checkSum));
#endif
    
    return checkSum;
}

SYSTEM_INFORMATION_ERROR_TYPES SystemInformation::setMACaddress()
{
    // Get the MAC address from dirtysock.
    memset(&mMacAddr[0], 0, sizeof(mMacAddr));
    EA::StdC::Strcpy(mMacAddr, NetConnMAC());
    return SYSINFO_NO_ERROR;
}

SYSTEM_INFORMATION_ERROR_TYPES SystemInformation::UpdateHardwareSurvey()
{
    SYSTEM_INFORMATION_ERROR_TYPES res;
    
#if defined(ORIGIN_PC)
    IWbemServices* pSvc = NULL;
    IWbemLocator* pLoc = NULL;
    
    res=InitWMI(pSvc, pLoc);
    if(res == SYSINFO_NO_ERROR)
    {
        Retrieve_Win32_ComputerSystem_DATA(pSvc);
        Retrieve_Win32_ComputerName_DATA(pSvc);
        Retrieve_Win32_OperatingSystem_DATA(pSvc);
        Retrieve_Win32_Processor_DATA(pSvc);
        Retrieve_Win32_Bios_DATA(pSvc);
        Retrieve_Win32_LogicalDisk_DATA(pSvc);
        Retrieve_Win32_OpticalDrive(pSvc);
        Retrieve_Win32_DesktopMonitor_DATA(pSvc);
        Retrieve_Win32_VideoController_DATA(pSvc);
        Retrieve_Win32_VideoCaptureDevices();
    }
    res=RetrieveDefaultBrowserName(defaultBrowserName);
    ShutdownWMI(pSvc, pLoc);
    
    m_uniqueMachineHash = CalculateUniqueMachineHash();

    mSysInfoChecksum = createHardwareSurveyChecksum();
    
    NonQTOriginServices::Utilities::getIOLocation(OriginTelemetry::TelemetryConfig::telemetryDataPath(), mSysInfoChecksumFileName);
    mSysInfoChecksumFileName+=ksysInfoChecksumFileName;
    
#elif defined(ORIGIN_MAC)
    res = SYSINFO_NO_ERROR;
    // Retrieve Mac-specific system information.
    Retrieve_Mac_SystemInformation_DATA();
    RetrieveDefaultBrowserName(defaultBrowserName);

    // Create the checksum.
    mSysInfoChecksum = createHardwareSurveyChecksum();

    // Get the location to save the checksum in.
    NonQTOriginServices::Utilities::getIOLocation(OriginTelemetry::TelemetryConfig::telemetryDataPath(), mSysInfoChecksumFileName);
    mSysInfoChecksumFileName+=ksysInfoChecksumFileName;

#elif defined(__linux__)
    assert(false);
    res = SYSINFO_NO_ERROR;
#else
    res = SYSINFO_NO_ERROR;
#  error "Requires implementation."
#endif
    
    return res;
}

bool8_t SystemInformation::isChecksumValid()
{
    uint64_t crc1 = 0;
    ReadSysInfoChecksum(crc1);
    if(mSysInfoChecksum!=crc1)
    {
        WriteSysInfoChecksum();
        return false;
    }
    return true;
}

const char8_t* SystemInformation::GetClientVersion() const
{
    return clientVersion.c_str();
}

void SystemInformation::SetClientVersion(const char8_t* clientVersionParam)
{
    clientVersion = clientVersionParam;
}

const char8_t* SystemInformation::GetBootstrapVersion() const
{
    return bootstrapVersion.c_str();
}

void SystemInformation::SetBootstrapVersion(const char8_t* bootstrapVersionParam)
{
    bootstrapVersion = bootstrapVersionParam;
}

SYSTEM_INFORMATION_ERROR_TYPES SystemInformation::WriteMachineHash()
{
    FILE *file = NonQTOriginServices::Utilities::openFile(mMachineHashFileName, L"wb");
    if(!file)
        return SYSINFO_FILE_WRITE_ERROR;
    
    const void *data = &mMacAddressHash;
    uint64_t dataSize = sizeof(mMacAddressHash);

    TelemetryCheckSum tmp = { 0 };
    NonQTOriginServices::Utilities::CalculcateIncrementalChecksum(data, dataSize, tmp);
    
    // write checksum
    size_t s=fwrite(&tmp, 1, (size_t)sizeof(tmp), file);
    if(s!=(size_t)sizeof(tmp))
    {
        fclose(file);
        return SYSINFO_FILE_WRITE_ERROR;
    }
    
    // write data
    s=fwrite(data, 1, (size_t)dataSize, file);
    fclose(file);
    if(s!=(size_t)dataSize)
        return SYSINFO_FILE_WRITE_ERROR;
    
    return SYSINFO_NO_ERROR;
}

SYSTEM_INFORMATION_ERROR_TYPES SystemInformation::ReadMachineHash(uint64_t &hash)
{
    size_t s = EA::IO::File::GetSize(mMachineHashFileName.c_str());
    
    FILE *file = NonQTOriginServices::Utilities::openFile(mMachineHashFileName, L"rb");
    if(!file || s==0)
        return SYSINFO_FILE_READ_ERROR;
    
    // read checksum
    TelemetryCheckSum crc={0};
    s = fread(&crc, 1, sizeof(crc), file);
    if(s!=sizeof(crc))
    {
        hash=0;
        fclose(file);
        return SYSINFO_FILE_READ_ERROR;
    }
    
    // read data
    s=fread(&hash, 1, sizeof(hash), file);
    if(s!=sizeof(hash))
    {
        hash=0;
        fclose(file);
        return SYSINFO_FILE_READ_ERROR;
    }
    
    fclose(file);
    
    TelemetryCheckSum tmp={0};
    NonQTOriginServices::Utilities::CalculcateIncrementalChecksum(&hash, (size_t)sizeof(hash), tmp);
    // compare checksum
    if(tmp.A != crc.A || tmp.B != crc.B || tmp.C != crc.C || tmp.D != crc.D)
    {
        hash=0;
        return SYSINFO_FILE_READ_ERROR;
    }
    
    return SYSINFO_NO_ERROR;
}

SYSTEM_INFORMATION_ERROR_TYPES SystemInformation::WriteSysInfoChecksum()
{
    const void *data = &mSysInfoChecksum;
    uint64_t dataSize = sizeof(mSysInfoChecksum);
    
    FILE *file = NonQTOriginServices::Utilities::openFile(mSysInfoChecksumFileName, L"wb");
    if(!file)
        return SYSINFO_FILE_WRITE_ERROR;
    
    // write data
    size_t s=fwrite(data, 1, (size_t)dataSize, file);
    fclose(file);
    if(s!=(size_t)dataSize)
        return SYSINFO_FILE_WRITE_ERROR;
    
    return SYSINFO_NO_ERROR;
}

SYSTEM_INFORMATION_ERROR_TYPES SystemInformation::ReadSysInfoChecksum(uint64_t &checksum)
{
    size_t s = EA::IO::File::GetSize(mSysInfoChecksumFileName.c_str());
    
    FILE *file = NonQTOriginServices::Utilities::openFile(mSysInfoChecksumFileName, L"rb");
    if(!file || s==0)
        return SYSINFO_FILE_READ_ERROR;
    
    s=fread(&checksum, 1, sizeof(checksum), file);
    
    if(s!=sizeof(checksum))
    {
        checksum=0;
        fclose(file);
        return SYSINFO_FILE_READ_ERROR;
    }
    
    fclose(file);
    
    return SYSINFO_NO_ERROR;
}

SystemInformation::SystemInformation() :
#if defined(ORIGIN_PC)
    COMInitialized(false),
#endif
    mSysInfoChecksum(0)
,   mMacAddressHash(0)
, mUniqueMachineHashFNV1A(0)
    //    CPUs
,    numCPUs(0)
,    numberOfLogicalProcessors(0)
,    numberOfCores(0)
    //    Memory
,    installedMemory_MB(0)
    //    OS
,    OSArchitecture(0)
,   Hypervisor(Hypervisor_Unknown)
    //    Locale
,    OSLocale(0)
,    UserLocale(0)
    //    HDD
,    numHDDs(0)
    //    ODD
,    numODDs(0)
    //    Display
,    numDisplays(0)
    //    Video
,    numVideoController(0)
    // Webcam
,   numVideoCaptureDevice(0)
    //    Virtual resolution
,    virtualResX(0)
,    virtualResY(0)
    //    Microphone
,    microphoneConnected(0)
    //    Network
,    networkSpeed(0)
{
    memset(&mMacAddr[0], 0, sizeof(mMacAddr));
    
    //    Display
    memset(videoDeviceID, 0, sizeof(videoDeviceID));
    memset(videoVendorID, 0, sizeof(videoVendorID));
    SetClientVersion(EALS_VERSION_P_DELIMITED);
    
#if defined(ORIGIN_PC)
    NonQTOriginServices::Utilities::getIOLocation(OriginTelemetry::TelemetryConfig::telemetryDataPath(), mMachineHashFileName);
#elif defined(ORIGIN_MAC)
    // on Mac, the machine hash file "data" is located in /var/tmp/Origin/
    mMachineHashFileName = kMacTempDataPath;
#endif 

    mMachineHashFileName += kmachineHashFileName;

    if (!g_dirtynetStarted)
    {
        // NetConnStartup sets the DirtyCertName
        int32_t netConnStartupResult = NetConnStartup(serviceName().c_str());

        if (netConnStartupResult == -1 || netConnStartupResult == 0)
        {
            // -1 means netconStartup was already successfully called.
            g_dirtynetStarted = true;
        }
        else
        {
            //we couldn't startup dirtySDK this is bad.
            eastl::string8 netConnErrorMsg;
            netConnErrorMsg.append_sprintf("SystemInformation Constructor: NetConnStartup returned error %d. This is Bad. we won't be able to send telemetry.", netConnStartupResult);
            OriginTelemetry::TelemetryLogger::GetTelemetryLogger().logTelemetryError(netConnErrorMsg);
            EA_FAIL_MSG(netConnErrorMsg.c_str());
        }
    }
    // TODO review this code: we might not need this anymore
    // TODO Get the MacAddress through a NetConnStatus, returns array of integers
    ReadMachineHash(mMacAddressHash);
    if (mMacAddressHash == 0 && g_dirtynetStarted)
    {
        uint8_t aMacAddr[6]={0};
        uint8_t zeroBuffer[6]={0};      
        if (NetConnStatus('macx', 0, aMacAddr, sizeof(aMacAddr)) >= 0) // find mac address
        {
            if(memcmp(aMacAddr, zeroBuffer, sizeof(aMacAddr)) != 0)    // if we actual have a MAC address use it
            {
                mMacAddressHash = NonQTOriginServices::Utilities::calculateFNV1A((char8_t*)&aMacAddr[0], sizeof(aMacAddr));
                WriteMachineHash();
            }  
            else
            {
                eastl::string8 errorMsg = "Error: SystemInformation Constructor failed to generate a mac hash. This should not happen!";
                EA_FAIL_MSG(errorMsg.c_str());
                OriginTelemetry::TelemetryLogger::GetTelemetryLogger().logTelemetryError(errorMsg);
            }
        }
    }
    // TODO then get mac address in string form
    setMACaddress();
    UpdateHardwareSurvey();
    eastl::string8 concatString = motherboardString + motherboardSerialNumber + biosManufacturer + biosSerialNumber + OSInstallDate + OSSerialNumber;
    mUniqueMachineHashFNV1A = NonQTOriginServices::Utilities::calculateFNV1A(concatString.c_str());

    if (mMacAddressHash == 0)
    {
        eastl::string8 errorMsg = "Error: SystemInformation Constructor mMacAddressHash is 0 at end of constructor. This should not happen!";
        EA_FAIL_MSG(errorMsg.c_str());
        OriginTelemetry::TelemetryLogger::GetTelemetryLogger().logTelemetryError(errorMsg);
    }
}

eastl::string8 SystemInformation::serviceName()
{
    if (mServiceName.length() == 0)
    {
        const eastl::string8 platformStr =
#ifdef ORIGIN_PC
            "pc";
#elif defined(ORIGIN_MAC)
            "mac";
#endif // DEBUG
        mServiceName.append_sprintf("-servicename=origin-2014-%s", platformStr.c_str());
    }
    return mServiceName;
}

const char8_t *SystemInformation::GetMacAddr() const
{
    return mMacAddr;
}

const char8_t* SystemInformation::GetBrowserName() const
{
    return defaultBrowserName.c_str();
}


const char8_t* SystemInformation::GetOSLocale() const
{
#if defined(ORIGIN_PC)
    _oslocal_map_t::const_iterator iter =  NumericLocale_to_Name.find( OSLocale );
    if(iter != NumericLocale_to_Name.end())
    {
        return iter->second.c_str();
    }
    
    return kTelemetryNoneString;
#elif defined(ORIGIN_MAC) || defined(__linux)
    return GetUserLocale();
#endif
}

const char8_t* SystemInformation::GetUserLocale() const
{
#if defined(ORIGIN_MAC)
    // We can grab the Windows locale code using Core Foundation on Mac
    CFLocaleRef cfUserLocale = CFLocaleCopyCurrent();
    uint16_t localeWinCode = (uint16_t) CFLocaleGetWindowsLocaleCodeFromLocaleIdentifier (CFLocaleGetIdentifier (cfUserLocale));
    CFRelease (cfUserLocale);
    
    _oslocal_map_t::const_iterator iter =  NumericLocale_to_Name.find( localeWinCode );
    if(iter != NumericLocale_to_Name.end())
    {
        return iter->second.c_str();
    }
#elif defined(ORIGIN_PC)
    _oslocal_map_t::const_iterator iter =  NumericLocale_to_Name.find( UserLocale );
    if(iter != NumericLocale_to_Name.end())
    {
        return iter->second.c_str();
    }
#elif defined(__linux__)
    char* s = getenv("LANG");
    if (s != NULL)
    {
        setlocale(LC_ALL, s);
        return nl_langinfo(_NL_IDENTIFICATION_LANGUAGE);
    }
#else
#error "Implement"
#endif
    
    return kTelemetryNoneString;
}

const char8_t* SystemInformation::GetCPUName() const 
{
    return cpuDescription.c_str();
}

uint32_t SystemInformation::GetNumCPUs() const 
{
    return numCPUs;
}

uint64_t SystemInformation::GetInstalledMemory_MB() const 
{
    return installedMemory_MB;
}

const char8_t* SystemInformation::GetOSDescription() const
{
    return OSDescription.c_str();
}

const char8_t* SystemInformation::GetOSVersion() const
{
    return OSVersion.c_str();
}

const char8_t* SystemInformation::GetServicePackName() const
{
    return OSServicePack.c_str();
}

//-----------------------------------------------------------------------------
//    STORAGE:  HDD
//-----------------------------------------------------------------------------
uint32_t SystemInformation::GetNumHDDs() const 
{
    return numHDDs;
}

uint64_t SystemInformation::GetHDDSpace_GB(uint32_t i) const 
{
    return totalHDDSpace_GB[std::min(i, numHDDs-1)];
}

const char8_t* SystemInformation::GetHDDInterface(uint32_t i) const
{
    return hddInterface[std::min(i, numHDDs-1)].c_str();
}

const char8_t* SystemInformation::GetHDDName(uint32_t i) const
{
    return hddName[std::min(i, numHDDs-1)].c_str();
}

const char8_t* SystemInformation::GetHDDType(uint32_t i) const
{
    return hddType[std::min(i, numHDDs-1)].c_str();
}
eastl::string8 SystemInformation::GetHDDSerialNumber(uint32_t i) const
{
    return hddSerialNumber[std::min(i, numHDDs-1)];
}

//-----------------------------------------------------------------------------
//    STORAGE:  ODD
//-----------------------------------------------------------------------------
uint32_t SystemInformation::GetNumODDs() const 
{
    return numODDs;
}

const char8_t* SystemInformation::GetODDType(uint32_t i) const 
{
    return oddType[std::min(i, numODDs-1)].c_str();
}

const char8_t* SystemInformation::GetODDName(uint32_t i) const 
{
    return oddName[std::min(i, numODDs-1)].c_str();
}

//-----------------------------------------------------------------------------
//    DISPLAY
//-----------------------------------------------------------------------------
void SystemInformation::AddDisplay(uint32_t x, uint32_t y)
{
    //  Ensure we don't overflow
    if (numDisplays >= MAX_NUMBER_HARDWARE_DEVICE_ENTRIES)
        return;

    displayResX[numDisplays] = x;
    displayResY[numDisplays] = y;
    numDisplays++;
}

uint32_t SystemInformation::GetNumDisplays() const 
{
    return numDisplays;
}

uint32_t SystemInformation::GetDisplayResX(uint32_t i) const 
{
    return displayResX[std::min(i, numDisplays-1)];
}

uint32_t SystemInformation::GetDisplayResY(uint32_t i) const 
{
    return displayResY[std::min(i, numDisplays-1)];
}

uint32_t SystemInformation::GetNumVideoControllers() const 
{
    return numVideoController;
}

uint32_t SystemInformation::GetNumVideoCaptureDevice() const 
{
    return numVideoCaptureDevice;
}

const char8_t* SystemInformation::GetVideoAdapterName(uint32_t i) const 
{
    return videoAdapterName[std::min(i, numVideoController-1)].c_str();
}

const char8_t* SystemInformation::GetVideoAdapterDriverVersion(uint32_t i) const
{
    return videoAdapterDriverVersion[std::min(i, numVideoController - 1)].c_str();
}

const char8_t* SystemInformation::GetVideoCaptureName(uint32_t i) const
{
    return videoCaptureDevice[std::min(i, numVideoCaptureDevice-1)].c_str();
}

uint32_t SystemInformation::GetVideoAdapterBitsDepth(uint32_t i) const 
{
    return videoBitsPerPixel[std::min(i, numVideoController-1)];
}

uint32_t SystemInformation::GetVideoAdapterVRAM(uint32_t i) const 
{
    return videoAdapterRAM[std::min(i, numVideoController-1)];
}

uint32_t SystemInformation::GetVideoAdapterDeviceID(uint32_t i) const 
{
    return videoDeviceID[i];
}

uint32_t SystemInformation::GetVideoAdapterVendorID(uint32_t i) const 
{
    return videoVendorID[i];
}

const char8_t* SystemInformation::GetHyperVisorString() const
{
    return HypervisorString[Hypervisor];
}

//  NOTE: The return codes are a little strange so please watch out.
int SystemInformation::IsComputerSurfacePro() const
{
    //  Running under hypervisor
    if ((Hypervisor == Hypervisor_Unknown) ||
        (Hypervisor == Hypervisor_Native))
    {
        return 10;
    }

    //  Virtual PC reports motherboard from Microsoft
    eastl::string8 win8MotherboardString("Microsoft Corporation");
    if (motherboardString.comparei(win8MotherboardString))
    {
        return 30;
    }

    //  Most likely Microsoft Surface Pro if reached here.
    return 1;
}

const eastl::string SystemInformation::CalculateUniqueMachineHash()
{
    // OOA uses a string concatentation of computer name + mobo manufacturer + mobo serial
    // number + bios manufacturer + bios serial number + os install date + os serial number
    // as the machine hash. OOA then uses SHA1 to hash the string. This data is relevant
    // to EBICHANGE-1379. More info on OOA https://developer.origin.com/documentation/display/D2CTech/Access+DRM+-+Licensing+Machine+ID
    eastl::string8 concatString = computerName + motherboardString + motherboardSerialNumber + biosManufacturer + biosSerialNumber + OSInstallDate + OSSerialNumber;
    return NonQTOriginServices::Utilities::encryptSHA1(concatString);
}

uint64_t SystemInformation::GetMachineHash()
{
    if (mMacAddressHash == 0)
    {
        eastl::string8 errorMsg = "Error: SystemInformation::GetMachineHash() returned 0. This should not happen!";
        EA_FAIL_MSG(errorMsg.c_str());
        OriginTelemetry::TelemetryLogger::GetTelemetryLogger().logTelemetryError(errorMsg);
    }
    return mMacAddressHash;
}


}
