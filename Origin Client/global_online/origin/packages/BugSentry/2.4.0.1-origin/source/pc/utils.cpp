/******************************************************************************/
/*!
    Utils

    Copyright 2010 Electronic Arts Inc.

    Version       1.0     04/09/2010 (mbaylis)
*/
/******************************************************************************/

#pragma warning(disable:4263) // member function does not override any base class virtual member function
#pragma warning(disable:4264) // no override available for virtual member function from base 'IDWriteTextFormat'; function is hidden


/*** Includes *****************************************************************/
#include "BugSentry/utils.h"

#include <windows.h>
#include <Tlhelp32.h>
#include <DbgHelp.h>
#include <cstdio>

/*** Implementation ***********************************************************/
namespace EA
{
    namespace BugSentry
    {
        /*** Varibles *********************************************************/
#ifdef USE_RTL_CAPTURE_BACKTRACE
        typedef USHORT (WINAPI* CaptureStackBackTraceFunction)(ULONG, ULONG, PVOID*, PULONG);
        CaptureStackBackTraceFunction gCaptureStackBackTrace;
#endif

	  /******************************************************************************/
        /*! Utils::Utils

            \brief      Constructor - load kernel32.dll and get the address of the function RtlCaptureStackBackTrace.

            \param       - none.
            \return      - none.
        */
        /******************************************************************************/
        Utils::Utils()
        {
#ifdef USE_RTL_CAPTURE_BACKTRACE
            HMODULE lib = LoadLibraryA("kernel32.dll");
            reinterpret_cast<void*&>(gCaptureStackBackTrace) = GetProcAddress(lib, "RtlCaptureStackBackTrace");
#endif
        }

	  /******************************************************************************/
        /*! Utils::GetStackBounds

            \brief      Get the callstack information (the addresses).

            \param       exceptionInformation - The platform-specific exception information.
            \param       top - conceptual top of the stack.
            \param       bottom - conceptual bottom of the stack.
            \return      true if successful, false if not.
        */
        /******************************************************************************/
        bool GetStackBounds(LPEXCEPTION_POINTERS exceptionInformation, PBYTE* top, PBYTE* bottom)
        {
            MEMORY_BASIC_INFORMATION stMemBasicInfo;
            bool success = false;

            //Note: top and bottom refers to the conceptual top and bottom of the stack.
            //In terms of pointer values, top < bottom since the stack grows downwards.
#ifdef _M_IX86
            PBYTE pPtr = reinterpret_cast<PBYTE>(exceptionInformation->ContextRecord->Esp); //Stack pointer
#else //_M_X64
            PBYTE pPtr = reinterpret_cast<PBYTE>(exceptionInformation->ContextRecord->Rsp); //Stack pointer
#endif

            if (VirtualQuery(pPtr, &stMemBasicInfo, sizeof(stMemBasicInfo)))
            {
                success = true;
                PBYTE pPos = static_cast<PBYTE>(stMemBasicInfo.AllocationBase);

                //This loop will do three iterations: unused pages, guard page, used pages.
                //pPos will point to the start of of the stack when finished.
                //Note: the stack grows downwards, we loop upwards.
                do
                {
                    if (!VirtualQuery(pPos, &stMemBasicInfo, sizeof(stMemBasicInfo)))
                    {
	                    success = false;
                          break;
                    }
                    if (stMemBasicInfo.RegionSize == 0)
                    {
	                    success = false;
                          break;
                    }

                    pPos += stMemBasicInfo.RegionSize;
                }
                while (pPos < pPtr);

                if (success)
                {
                    *top = pPtr;
                    *bottom = pPos;
                }
            }

            return success;
        }

	  /******************************************************************************/
        /*! Utils::GetCallstack

            \brief      Get the callstack information (the addresses).

            \param       exceptionPointers - The platform-specific exception information.
            \param       addresses - An array to store the addresses into.
            \param       addressesMax - The max number of entries available in the addresses array.
            \param       addressesFilledCount - The number of entries used when collecting the callstack (optional).
            \return      true if successful, false if not.
        */
        /******************************************************************************/
        bool Utils::GetCallstack(void *exceptionPointers, uintptr_t* addresses, int addressesMax, int* addressesFilledCount)
        {
            bool callstackRetrieved = false;

#if defined(_M_IX86) || defined(_M_X64)
            LPEXCEPTION_POINTERS exceptionInformation = static_cast<LPEXCEPTION_POINTERS>(exceptionPointers);

            if (exceptionPointers && addresses && addressesFilledCount && (addressesMax > 0))
            {
                int addressCount = 0;

                //Stackwalk64 changes the context it gets passed. Make a copy so that consecutive
                //calls to this function with the same input produces the same result.
                CONTEXT Context;
                memcpy(&Context, exceptionInformation->ContextRecord, sizeof(CONTEXT));

                STACKFRAME64 StackFrame64;
                memset(&StackFrame64, 0, sizeof(StackFrame64));

                DWORD MachineType = IMAGE_FILE_MACHINE_I386;

                StackFrame64.AddrPC.Mode = AddrModeFlat;
                StackFrame64.AddrStack.Mode = AddrModeFlat;
                StackFrame64.AddrFrame.Mode = AddrModeFlat;
#ifdef _M_IX86
                StackFrame64.AddrPC.Offset = Context.Eip;
                StackFrame64.AddrStack.Offset = Context.Esp;
                StackFrame64.AddrFrame.Offset = Context.Ebp;
#else //_M_X64
                StackFrame64.AddrPC.Offset = Context.Rip;
                StackFrame64.AddrStack.Offset = Context.Rsp;
                StackFrame64.AddrFrame.Offset = Context.Rbp;
                MachineType = IMAGE_FILE_MACHINE_AMD64;
#endif
                BOOL StackWalkOk = TRUE;
                while (StackWalkOk && addressCount < addressesMax)
                {
                    StackWalkOk = StackWalk64(MachineType,
                        GetCurrentProcess(),
                        GetCurrentThread(),
                        &StackFrame64,
                        &Context,
                        NULL,
                        SymFunctionTableAccess64,
                        SymGetModuleBase64,
                        NULL);

                    if (StackWalkOk)
                    {
                        addresses[addressCount++] = static_cast<uintptr_t>(StackFrame64.AddrPC.Offset);
                    }
                }

                *addressesFilledCount = addressCount;
                callstackRetrieved = true;
            }
#else
    #error "Platform not supported!"
#endif //defined(_M_IX86) || defined(_M_X64)

            return callstackRetrieved;
        }
 
        /******************************************************************************/
        /*! Utils::GetCallstackHeuristic

            \brief      Make a best effort stack trace using heuristics.

            \param       exceptionPointers - The platform-specific exception information.
            \param       buffer - string to put platform info in.
            \param       bufferSize - size of the buffer.
            \return      true if successful, false if not.
        */
        /******************************************************************************/
        bool Utils::GetCallstackHeuristic(void *exceptionPointers, uintptr_t* addresses, int addressesMax, int* addressesFilledCount)
        {
            bool success = false;

#if defined(_M_IX86) //|| defined(_M_X64)
            LPEXCEPTION_POINTERS exceptionInformation = static_cast<LPEXCEPTION_POINTERS>(exceptionPointers);

            //The first entry is the instruction where we crashed.
            int addressCount = 0;
            addresses[addressCount++] = exceptionInformation->ContextRecord->Eip;
            *addressesFilledCount = addressCount; //At least we will have this.

            //Loop down the stack in DWORD steps until we hit the bottom of the stack or
            //have filled in the maximum number of addresses. Note: stack grows downwards.
            PBYTE top, bottom;
            if (GetStackBounds(exceptionInformation, &top, &bottom))
            {
                MODULEENTRY32 moduleEntry = {sizeof(moduleEntry)};
                HANDLE hSnapshot = NULL;

                hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, 0);
                PBYTE ptr = top;
                while (ptr < bottom  && addressCount < addressesMax)
                {
                    //Try to match the value vs. a valid address in a module.
                    if ((hSnapshot != INVALID_HANDLE_VALUE) && Module32First(hSnapshot, &moduleEntry))
                    {
                        do
                        {
                            PBYTE test = (PBYTE)(*((DWORD*)ptr));
                            if (DWORD(test - moduleEntry.modBaseAddr) < moduleEntry.modBaseSize)
                            {
                                addresses[addressCount++] = (unsigned int)test;
                                break;
                            }
                        }
                        while (Module32Next(hSnapshot, &moduleEntry));
                    }
                    ptr += sizeof(DWORD);
                }
                CloseHandle(hSnapshot);
                *addressesFilledCount = addressCount;

                success = true;
            }
#endif
            return success;
		}

        /******************************************************************************/
        /*! Utils::GetStackDump

            \brief      Get the stack dump information.

            \param       exceptionPointers - The platform-specific exception information.
            \param       dump - An array of bytes to store the addresses into.
            \param       maxDumpSize - The max number of bytes available in the dump.
            \param       dumpSize - The number of bytes used when collecting the stack dump.
            \return      true if successful, false if not.
        */
        /******************************************************************************/
        bool Utils::GetStackDump(void *exceptionPointers, unsigned char* dump, int maxDumpSize, int *dumpSize)
        {
            bool success = false;

            LPEXCEPTION_POINTERS exceptionInformation = static_cast<LPEXCEPTION_POINTERS>(exceptionPointers);

            PBYTE top, bottom;
            if (GetStackBounds(exceptionInformation, &top, &bottom))
            {
                int size = static_cast<int>(min((bottom - top), maxDumpSize));
                memcpy(dump, top, static_cast<size_t>(size));
                *dumpSize = size;
                success = true;
            }

            return success;
        }

        /******************************************************************************/
        /*! Utils::GetModuleInfo

            \brief      Get windows module information.

            \param       exceptionPointers - The platform-specific exception information.
            \param       buffer - string to put platform info in.
            \param       bufferSize - size of the buffer.
            \return      true if successful, false if not.
        */
        /******************************************************************************/
        bool Utils::GetModuleInfo(void* exceptionPointers, char* buffer, int bufferSize)
        {
#if defined(_M_IX86)
            MODULEENTRY32 moduleEntry = {sizeof(moduleEntry)};
            HANDLE  hSnapshot = NULL;
            static const int LINE_LENGTH = MAX_PATH + 1 + 8 + 8 + 1 + 1; //path + space + addr + size + \n + \0
            char lineBuffer[LINE_LENGTH];

            hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, 0);

            char* tmpEnd = buffer + bufferSize - 1;
            char* tmpBuffer = buffer;
            if ((hSnapshot != INVALID_HANDLE_VALUE) && Module32First(hSnapshot, &moduleEntry))
            {
                do
                {
#ifdef _UNICODE
                    sprintf(lineBuffer, "%S %08X %08X\n", moduleEntry.szModule, moduleEntry.modBaseAddr, moduleEntry.modBaseSize);
#else
                    sprintf(lineBuffer, "%s %08X %08X\n", moduleEntry.szModule, moduleEntry.modBaseAddr, moduleEntry.modBaseSize);
#endif
                    strncpy(tmpBuffer, lineBuffer, static_cast<size_t>(tmpEnd - tmpBuffer));
                    tmpBuffer += strlen(lineBuffer);
                }
                while ((tmpBuffer < tmpEnd) && Module32Next(hSnapshot, &moduleEntry));

                buffer[bufferSize-1] = 0; //In case we hit the limit during strncpy.
            }

            CloseHandle(hSnapshot);

            return true;
#else
            return false;
#endif
        }
    }
}