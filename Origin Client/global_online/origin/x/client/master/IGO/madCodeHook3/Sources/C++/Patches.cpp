// ***************************************************************
//  Patches.cpp               version: 1.0.0  ·  date: 2010-01-10
//  -------------------------------------------------------------
//  import/export table patching
//  -------------------------------------------------------------
//  Copyright (C) 1999 - 2010 www.madshi.net, All Rights Reserved
// ***************************************************************

#define _PATCHES_C

#include "SystemIncludes.h"
#include "Systems.h"
#include "SystemsInternal.h"


#ifdef _USE_IGO_LOGGER_        // we only use the logger in the igo32.dll, not in the Origin.exe !!!
#include "../../../IGOLogger.h"
#endif


SYSTEMS_API LPVOID* WINAPI FindWs2InternalProcList(HMODULE hModule)
// Winsock2 internally has an additional list of some exported APIs
// we need to find this list and modify it, too
// or else Winsock2 will refuse to initialize, when we change the export table
{
  LPVOID *result = NULL;
  PIMAGE_NT_HEADERS nh = GetImageNtHeaders(hModule);
  PIMAGE_EXPORT_DIRECTORY pexp = GetImageExportDirectory(hModule);
  if ((nh) && (pexp))
  {
    PIMAGE_SECTION_HEADER ish = (PIMAGE_SECTION_HEADER) ((ULONG_PTR) nh + sizeof(IMAGE_NT_HEADERS));
    for (int i1 = 0; i1 < nh->FileHeader.NumberOfSections; i1++)
    {
      if (  (ish->Name[0] == '.') &&
           ((ish->Name[1] == 'd') || (ish->Name[1] == 'D')) &&
           ((ish->Name[2] == 'a') || (ish->Name[1] == 'A')) &&
           ((ish->Name[3] == 't') || (ish->Name[1] == 'T')) &&
           ((ish->Name[4] == 'a') || (ish->Name[1] == 'A')) &&
            (ish->Name[5] == 0  ) )
      {
        ULONG_PTR buf[10];
        for (int i2 = 0; i2 < 10; i2++)
          buf[i2] = (ULONG_PTR) hModule + ((DWORD *) ((ULONG_PTR) hModule + pexp->AddressOfFunctions))[i2];
        for (int i3 = 0; i3 < (int) (ish->SizeOfRawData - sizeof(buf)); i3++)
          if (!memcmp(buf, (LPVOID) ((ULONG_PTR) hModule + ish->VirtualAddress + i3), sizeof(buf)))
            return (LPVOID*) ((ULONG_PTR) hModule + ish->VirtualAddress + i3);
        break;
      }
      ish++;
    }
  }
  return result;
}

SYSTEMS_API BOOL WINAPI HackWinsock2IfNecessary(LPVOID *ws2procList, LPVOID pOld, LPVOID pNew)
{
  bool result = (!ws2procList);
  if (!result)
  {
    int i1 = 0;
    while (ws2procList[i1])
    {
      if (ws2procList[i1] == pOld)
      {
        BOOL b1 = IsMemoryProtected(ws2procList[i1]);
        if (UnprotectMemory(ws2procList[i1], 4))
        {
          ws2procList[i1] = pNew;
          result = true;
          if (b1)
            ProtectMemory(ws2procList[i1], 4);
        }
        break;
      }
      else
        i1++;
    }
  }
  return result;
}

SYSTEMS_API BOOL WINAPI PatchExportTable(HMODULE hModule, LPVOID pOld, LPVOID pNew, LPVOID *ws2procList)
{
  BOOL result = false;

  __try
  {
    IMAGE_EXPORT_DIRECTORY *pExport = GetImageExportDirectory(hModule);
    if (pExport != NULL)
    {
      for (DWORD i = 0; i < pExport->NumberOfFunctions; i++)
      {
        DWORD address = ((DWORD *) ((ULONG_PTR) hModule + pExport->AddressOfFunctions))[i];
        if (((ULONG_PTR) hModule + address) == (ULONG_PTR) pOld)
        {
          if (HackWinsock2IfNecessary(ws2procList, pOld, pNew))
          {
            DWORD *p = (DWORD *) &((DWORD *) ((ULONG_PTR) hModule + pExport->AddressOfFunctions))[i];
            BOOL b = IsMemoryProtected(p);
            if (UnprotectMemory(p, 4))
            {
              *p = (DWORD) ((ULONG_PTR) pNew - (ULONG_PTR) hModule);
              result = true;
              if (b)
                ProtectMemory(p, 4);
            }
            else
              HackWinsock2IfNecessary(ws2procList, pNew, pOld);
          }
          break;
        }
      }
    }
  }
  __except (ExceptionFilter(L"PatchExportTable", GetExceptionInformation()))
  {
    result = false;
  }
  return result;
}

const LONG OFFSET_VIRTUAL_ADDRESS = FIELD_OFFSET(IMAGE_NT_HEADERS, OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);
#define OFFSET_VIRTUAL_ADDRESS_LITERAL 0x80

const LONG OFFSET_SIZEOF_IMAGE = FIELD_OFFSET(IMAGE_NT_HEADERS, OptionalHeader.SizeOfImage);
#define OFFSET_SIZEOF_IMAGE_LITERAL 0x50

const LONG OFFSET_NAME = FIELD_OFFSET(IMAGE_IMPORT_DESCRIPTOR, Name);
#define OFFSET_NAME_LITERAL 0x0c

const LONG OFFSET_FIRST_THUNK = FIELD_OFFSET(IMAGE_IMPORT_DESCRIPTOR, FirstThunk);
#define OFFSET_FIRST_THUNK_LITERAL 0x10

const LONG IMAGE_IMPORT_DESCRIPTOR_SIZE = sizeof(IMAGE_IMPORT_DESCRIPTOR);

// The following can be used in DEBUG builds to assure offsets have not changed
// as the SDK/Windows changes.
void CheckStructureOffsets()
{
#ifndef _WIN64
  ASSERT(OFFSET_VIRTUAL_ADDRESS == OFFSET_VIRTUAL_ADDRESS_LITERAL);
  ASSERT(OFFSET_SIZEOF_IMAGE == OFFSET_SIZEOF_IMAGE_LITERAL);
  ASSERT(OFFSET_NAME == OFFSET_NAME_LITERAL);
  ASSERT(OFFSET_FIRST_THUNK == OFFSET_FIRST_THUNK_LITERAL);
#endif
}

SYSTEMS_API void WINAPI PatchImportTable(HMODULE hModule, LPVOID pOld, LPVOID pNew)
{
  if (*((WORD *) hModule) == IMAGE_DOS_SIGNATURE)
  {
    DWORD offset = *(DWORD *) ((ULONG_PTR) hModule + NT_HEADERS_OFFSET);
    IMAGE_NT_HEADERS *pNtHeaders = (IMAGE_NT_HEADERS *) ((ULONG_PTR) hModule + offset);
    if (pNtHeaders->Signature == IMAGE_NT_SIGNATURE)
    {
      DWORD virtualAddress = pNtHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;
      if ((virtualAddress != 0) && (virtualAddress < pNtHeaders->OptionalHeader.SizeOfImage))
      {
        IMAGE_IMPORT_DESCRIPTOR *pImportDescriptor = (IMAGE_IMPORT_DESCRIPTOR *) ((ULONG_PTR) hModule + virtualAddress);
        if (pImportDescriptor != NULL)
        {
          while ( (pImportDescriptor->Name != NULL) && 
                  (pImportDescriptor->FirstThunk != 0) &&
                  (pImportDescriptor->FirstThunk < pNtHeaders->OptionalHeader.SizeOfImage) &&
                  ((ULONG_PTR) hModule + pImportDescriptor->FirstThunk != NULL) )
          {
            IMAGE_THUNK_DATA *pThunk = (IMAGE_THUNK_DATA *) ((ULONG_PTR) hModule + pImportDescriptor->FirstThunk);
            while (pThunk->u1.Function != NULL)
            {
#pragma warning(disable: 4312 4244)
              if ( ((LPVOID) pThunk->u1.Function == pOld) &&
                   (UnprotectMemory((LPVOID) pThunk, sizeof(LPVOID))) )
                pThunk->u1.Function = (ULONG_PTR) pNew;
              pThunk = (IMAGE_THUNK_DATA *) ((ULONG_PTR) pThunk + sizeof(LPVOID));
#pragma warning(default: 4312 4244)
            }
            pImportDescriptor = (IMAGE_IMPORT_DESCRIPTOR *) ((ULONG_PTR) pImportDescriptor + IMAGE_IMPORT_DESCRIPTOR_SIZE);
          }
        }
      }
    }
  }
}

void WINAPI PatchAllImportTables(LPVOID pOld, LPVOID pNew, BOOL shared)
{
  // Declare ALL locals
  MEMORY_BASIC_INFORMATION mbi;
  HMODULE hModule = NULL;
  ULONG_PTR address = 0;
  wchar_t fileName[8];
  while (VirtualQuery((LPCVOID) address, &mbi, sizeof(MEMORY_BASIC_INFORMATION)) == sizeof(MEMORY_BASIC_INFORMATION))
  {
    if ((mbi.State == MEM_COMMIT) && (mbi.AllocationBase != NULL) && (mbi.AllocationBase != hModule))
    {
#pragma warning(disable: 4312)
      if ((!shared) && (mbi.AllocationBase >= (LPVOID) 0x80000000))
#pragma warning(default: 4312)
        break;
      hModule = (HMODULE) mbi.AllocationBase;
      if (GetModuleFileName(hModule, fileName, 2) != 0)
        PatchImportTable(hModule, pOld, pNew);
    }
    address += mbi.RegionSize;
  }
}

void PatchMyImportTables(LPVOID pOld, LPVOID pNew)
{
  PatchAllImportTables(pOld, pNew, true);
}

BOOL IsAlreadyKnown(LPVOID pCode)
{
  TraceVerbose(L"%S(%p)", __FUNCTION__, pCode);

  BOOL result = FALSE;
  char mapName[MAX_PATH];
  ApiSpecialName(CNamedBuffer, CMAHPrefix, pCode, FALSE, mapName);
  HANDLE hMap = OpenGlobalFileMapping(mapName, false);
  if (hMap != NULL)
  {
    result = TRUE;
    VERIFY(CloseHandle(hMap));
  }
  else
    Trace(L"%S Failure - OpenGlobalFileMapping([%S])", __FUNCTION__, mapName);

  return result;
}

BOOL CheckCode(LPVOID pCode)
{
  TraceVerbose(L"%S(%p)", __FUNCTION__, pCode);

  HMODULE hModule;
  IMAGE_EXPORT_DIRECTORY *pImageExportDirectory;

  BOOL result = ResolveMixtureMode(&pCode) || IsAlreadyKnown(pCode);
  if (!result)
  {
    if (GetExportDirectory(pCode, &hModule, &pImageExportDirectory))
    {
      for (DWORD i = 0; i < pImageExportDirectory->NumberOfFunctions; i++)
      {
        DWORD functionAddress = ((DWORD *) ((ULONG_PTR) hModule + pImageExportDirectory->AddressOfFunctions))[i];
        if (pCode == (LPVOID) ((ULONG_PTR) hModule + functionAddress))
        {
          result = TRUE;
          break;
        }
      }
      if (!result)
        Trace(L"%S Failure - Failed to find pCode", __FUNCTION__);
    }
    else
      Trace(L"%S Failure - GetExportDirectory", __FUNCTION__);
  }
  return result;
}

SYSTEMS_API LPVOID WINAPI FindRealCode(LPVOID pCode, bool loadCheckOnly/* = false */)
{
  TraceVerbose(L"%S(%p)", __FUNCTION__, pCode);
#ifdef _USE_IGO_LOGGER_        // we only use the logger in the igo32.dll, not in the Origin.exe !!!
  OriginIGO::IGOLogWarn("%s(%p)", __FUNCTION__, pCode);
#endif

  LPVOID result = pCode;
  __try{

      if (pCode != NULL)
      {

          // Right now, simply checking whether the address is still mapped to an image - we could verify 
          // whether this is still the same module loaded since HANDLEs are reusable - but I doubt this would
          // happen in our scenarios
          BOOL loaded = FALSE;

          MEMORY_BASIC_INFORMATION mbi;
          if (VirtualQuery((PVOID)pCode, &mbi, sizeof(mbi)) == sizeof(MEMORY_BASIC_INFORMATION))
          {
              loaded = mbi.State == MEM_COMMIT && mbi.Type == MEM_IMAGE && mbi.Protect != PAGE_NOACCESS;

#ifdef _USE_IGO_LOGGER_        // we only use the logger in the igo32.dll, not in the Origin.exe !!!
              OriginIGO::IGOLogWarn("loaded %i mbi state %i type %i protect %i", loaded, mbi.State, mbi.Type, mbi.Protect);
#endif
              
              if (mbi.Protect == PAGE_EXECUTE_READWRITE)   // usually this is PAGE_EXECUTE_READ -> WNDPROC in Crysis 3 -> DRM
              {
#ifndef _WIN64
                  return (LPVOID)((LONG_PTR)-1);
#else
                  //seems fine for x64 according to Denuvo (Fifa15)
                  /*
                  From: Rauch, Leo[mailto:leo.rauch@denuvo.com]
                  Sent : Friday, August 7, 2015 9 : 57 AM
                  To : Fairweather, James <jamesf@ea.com>; Gabler, Christopher <christopher.gabler@denuvo.com>
                  Cc: Bruckschlegel, Thomas <TBruckschlegel@ea.com>
                  Subject : RE : Fifa 15->OOA + Denuvo

                  Hi,

                  we don’t have a tool for this.
                  The IGO hooks won’t affect our AntiTamper code and should work just fine.

                  Best regards,
                  Leo
                  */
#endif
              }
              if (!loaded)
                  return NULL;

              if (loadCheckOnly)
                  return loaded ? pCode : NULL;
          }
          else
          {
              WCHAR* pError = NULL;
              DWORD dwError = GetLastError();
              DWORD  dwResult = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, dwError,
                  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (WCHAR*)&pError, 0, NULL);
              if (dwResult)
              {


#ifdef _USE_IGO_LOGGER_        // we only use the logger in the igo32.dll, not in the Origin.exe !!!
                  OriginIGO::IGOLogWarn("VirtualQuery failed %i %S", dwError, pError);
#endif
                  LocalFree(pError);
              }

              if (loadCheckOnly)
                  return pCode;
      }


          if (CheckCode(pCode))
              result = pCode;
          else
          {
              DebugTrace((L"%S - CheckCode is False", __FUNCTION__));
#ifdef _USE_IGO_LOGGER_        // we only use the logger in the igo32.dll, not in the Origin.exe !!!
              OriginIGO::IGOLogWarn("%s - CheckCode is False", __FUNCTION__);
#endif

              if (*(WORD *)pCode == 0x25ff) // might crash here, if target DLL already unloaded
              {
#ifdef _WIN64
                  // RIP relative addressing
                  pCode = *(LPVOID*)((ULONG_PTR)pCode + 6 + *((DWORD *)((ULONG_PTR)pCode + 2)));
#else
                  pCode = (LPVOID) (ULONG_PTR) (*(*((DWORD **) ((ULONG_PTR) pCode + 2))));
#endif
                  if (CheckCode(pCode))
                      result = pCode;
                  else
                  {
                      // win9x debug mode
                  }
              }
          }
      }
  }

  __except (EXCEPTION_EXECUTE_HANDLER)
  {
      result = NULL;    // just a precaution !!!
  }
  return result;
}
