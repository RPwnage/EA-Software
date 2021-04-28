// Copyright (C) 2004, Matt Conover (mconover@gmail.com)

#pragma warning(disable:4255) // no function prototype given: converting '()' to '(void)'

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <assert.h>
#ifndef ORIGIN_MAC
#include <windows.h>
#else
#include "MacWindows.h"
#endif
#include "disasm.h"

#ifdef NO_SANITY_CHECKS
#define NDEBUG
#undef assert
#define assert(x)
#endif

#ifdef _DEBUG
#ifndef CRTDBG_MAP_ALLOC
#define CRTDBG_MAP_ALLOC
#endif
#endif

//////////////////////////////////////////////////////////////////////
// Global variables
//////////////////////////////////////////////////////////////////////

ARCHITECTURE_FORMAT SupportedArchitectures[] =
{
	{ ARCH_X86,	&X86 },
	{ ARCH_X86_16, &X86 },
	{ ARCH_X64,	&X86 },
	{ ARCH_UNKNOWN, NULL }
};

//////////////////////////////////////////////////////////////////////
// Function prototypes
//////////////////////////////////////////////////////////////////////

BOOL InitInstruction(INSTRUCTION *Instruction, DISASSEMBLER *Disassembler);
static ARCHITECTURE_FORMAT *GetArchitectureFormat(ARCHITECTURE_TYPE Type);

//////////////////////////////////////////////////////////////////////
// Disassembler setup
//////////////////////////////////////////////////////////////////////

BOOL InitDisassembler(DISASSEMBLER *Disassembler, ARCHITECTURE_TYPE Architecture)
{
	ARCHITECTURE_FORMAT *ArchFormat;

	memset(Disassembler, 0, sizeof(DISASSEMBLER));
	Disassembler->Initialized = DISASSEMBLER_INITIALIZED;
	
	ArchFormat = GetArchitectureFormat(Architecture);
	if (!ArchFormat) { assert(0); return FALSE; }
	Disassembler->ArchType = ArchFormat->Type;
	Disassembler->Functions = ArchFormat->Functions;
	return TRUE;
}

void CloseDisassembler(DISASSEMBLER *Disassembler)
{
	memset(Disassembler, 0, sizeof(DISASSEMBLER));
}

//////////////////////////////////////////////////////////////////////
// Instruction setup
//////////////////////////////////////////////////////////////////////

BOOL InitInstruction(INSTRUCTION *Instruction, DISASSEMBLER *Disassembler)
{
	memset(Instruction, 0, sizeof(INSTRUCTION));
	Instruction->Initialized = INSTRUCTION_INITIALIZED;
	Instruction->Disassembler = Disassembler;
	memset(Instruction->String, ' ', MAX_OPCODE_DESCRIPTION-1);
	Instruction->String[MAX_OPCODE_DESCRIPTION-1] = '\0';
	return TRUE;
}

// If Decode = FALSE, only the following fields are valid:
// Instruction->Length, Instruction->Address, Instruction->Prefixes, Instruction->PrefixCount,
// Instruction->OpcodeBytes, Instruction->Instruction->OpcodeLength, Instruction->Groups,
// Instruction->Type, Instruction->OperandCount
//
// If Disassemble = TRUE, then Instruction->String is valid (also requires Decode = TRUE)
//
// WARNING: This will overwrite the previously obtained instruction
INSTRUCTION *GetInstruction(DISASSEMBLER *Disassembler, U64 VirtualAddress, U8 *Address, U32 Flags)
{
	if (Disassembler->Initialized != DISASSEMBLER_INITIALIZED) { assert(0); return NULL; }
	assert(Address);
	InitInstruction(&Disassembler->Instruction, Disassembler);
	Disassembler->Instruction.Address = Address;	
	Disassembler->Instruction.VirtualAddressDelta = VirtualAddress - (U64)Address;
	if (!Disassembler->Functions->GetInstruction(&Disassembler->Instruction, Address, Flags))
	{
		assert(Disassembler->Instruction.Address == Address);
		assert(Disassembler->Instruction.Length < MAX_INSTRUCTION_LENGTH);

		// Save the address that failed, in case the lower-level disassembler didn't
		Disassembler->Instruction.Address = Address;
		Disassembler->Instruction.ErrorOccurred = TRUE;
		return NULL;
	}
	return &Disassembler->Instruction;
}

///////////////////////////////////////////////////////////////////////////
// Miscellaneous
///////////////////////////////////////////////////////////////////////////

static ARCHITECTURE_FORMAT *GetArchitectureFormat(ARCHITECTURE_TYPE Type)
{
	ARCHITECTURE_FORMAT *Format;
	for (Format = SupportedArchitectures; Format->Type != ARCH_UNKNOWN; Format++)
	{
		if (Format->Type == Type) return Format;
	}

	assert(0);
	return NULL;
}

// ORIGIN ADDITIONS

static disasmLogInfo InfoLogger = NULL;
static disasmLogError ErrorLogger = NULL;

void DisasmSetupLogging(disasmLogInfo infoLogger, disasmLogError errorLogger)
{
    InfoLogger = infoLogger;
    ErrorLogger = errorLogger;
}

void DisasmLogInfo(const char *file, int line, int alwaysLogMessage, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    
    if (InfoLogger)
        InfoLogger(file, line, alwaysLogMessage, fmt, args);
    
    va_end(args);
}

void DisasmLogError(const char *file, int line, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    
    if (ErrorLogger)
        ErrorLogger(file, line, fmt, args);
    
    va_end(args);
}


