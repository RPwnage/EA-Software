///////////////////////////////////////////////////////////////////////////////
// EAAddressRep.h
//
// Copyright (c) 2003-2005 Electronic Arts Inc.
// Created and maintained by Paul Pedriana
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// EAAddressRep is the symbol lookup and caching engine of EACallstack.
// It allows for symbol lookups at runtime for a given platform and for
// cross-platform symbols lookups outside of a running application.
//
// The class users should be most concerned with here is AddressRepCache,
// as it is a high-level accessor to the rest of the EAAddressRep library.
// Normally you want to do all your operations through AddressRepCache, 
// though you may be interested in doing uncached lookups through the 
// AddressRepLookupSet class.
//
// A typical high level function you might want to use is this:
//
//     bool GetAddressRepFromAddress(uint64_t address, eastl::string8& sAddressRep)
//     {
//         const char* pStrResults[Callstack::kARTCount];
//         int         pIntResults[Callstack::kARTCount];
//     
//         sAddressRep.clear();
//     
//         const int outputFlags = gAddressRepCache.GetAddressRep(mAddressRepTypeFlags, address, pStrResults, pIntResults);
//     
//         if(outputFlags & kARTFlagAddress)
//             sAddressRep.append_sprintf("%s\r\n", pStrResults[kARTSource]);
//     
//         if(outputFlags & kARTFlagFileLine)
//             sAddressRep.append_sprintf("%s(%d)\r\n", pStrResults[kARTFileLine], pIntResults[kARTFileLine]);
//     
//         if(outputFlags & kARTFlagFunctionOffset)
//             sAddressRep.append_sprintf("%s + %d\r\n", pStrResults[kARTFunctionOffset], pIntResults[kARTFunctionOffset]);
//     
//         if(outputFlags & kARTFlagSource)
//             sAddressRep.append_sprintf("%s\r\n", pStrResults[kARTSource]);
//     }
///////////////////////////////////////////////////////////////////////////////


#ifndef EACALLSTACK_EAADDRESSREP_H
#define EACALLSTACK_EAADDRESSREP_H


#include <EABase/eabase.h>
#include <EACallstack/internal/Config.h>
#include <EACallstack/internal/EASTLCoreAllocator.h>
#include <EAStdC/EAString.h>
#include <EASTL/string.h>
#include <EASTL/fixed_string.h>
#include <EASTL/hash_map.h>
#include <EASTL/list.h>
#include <coreallocator/icoreallocator_interface.h>


#ifdef _MSC_VER
	#pragma warning(push)           // We have to be careful about disabling this warning. Sometimes the warning is meaningful; sometimes it isn't.
	#pragma warning(disable: 4251)  // class (some template) needs to have dll-interface to be used by clients.
#endif




namespace EA
{
	namespace Callstack
	{
		// Forward declarations
		class IAddressRepLookup;


		/// Number of lines of context delivered with source code text.
		static const int kSourceContextLineCount = 8;

		// Return value from BaseAddress functions when the base address is unknown/the function fails.
		static const uint64_t kBaseAddressInvalid = 0; // To do: Change this to UINT64_C(0xffffffffffffffff). However, it will probably affect users.

		// Refers to when the base address for an AddressRep instance hasn't been explicitly specified by the user. 
		// For example, in the constructor for the PDBFile class its mBaseAddress will initially be set to kBaseAddressUnspecified.
		// Most of the time users don't need to use this constant, though it can be returned by AddressRep::GetBaseAddress().
		static const uint64_t kBaseAddressUnspecified = UINT64_C(0xffffffffffffffff);

		/// FixedString
		typedef eastl::fixed_string<char8_t,  256, true, EA::Callstack::EASTLCoreAllocator> FixedString;
		typedef eastl::fixed_string<char8_t,  256, true, EA::Callstack::EASTLCoreAllocator> FixedString8;
		typedef eastl::fixed_string<char16_t, 256, true, EA::Callstack::EASTLCoreAllocator> FixedString16;
		typedef eastl::fixed_string<char32_t, 256, true, EA::Callstack::EASTLCoreAllocator> FixedString32;
	   
		#if defined(EA_WCHAR_UNIQUE) && EA_WCHAR_UNIQUE
			typedef eastl::fixed_string<wchar_t, 256, true, EA::Callstack::EASTLCoreAllocator> FixedStringW;
		#else
			#if (EA_WCHAR_SIZE == 1)
				typedef FixedString8 FixedStringW;
			#elif (EA_WCHAR_SIZE == 2)
				typedef FixedString16 FixedStringW;
			#elif (EA_WCHAR_SIZE == 4)
				typedef FixedString32 FixedStringW;
			#endif
		#endif


		/// A string that takes an ICoreAllocator. Symbols names can be of highly variable
		/// length (from 1 to 100 chars), and we don't want to reserve a large fixed space
		/// for each. So we go with an ICoreAllocator.
		typedef eastl::basic_string<char, EA::Callstack::EASTLCoreAllocator> CAString8;


		/// ModuleHandle
		/// This is a runtime module identifier. For Microsoft Windows-like platforms
		/// this is the same thing as HMODULE. For other platforms it is a shared library
		/// runtime library pointer, id, or handle. For Microsoft platforms, each running
		/// DLL has a module handle.
		#if defined(EA_PLATFORM_MICROSOFT)
			typedef void*            ModuleHandle;  // HMODULE, from LoadLibrary()
		#elif defined(EA_PLATFORM_KETTLE)
			typedef int32_t          ModuleHandle;  // SceKernelModule
		#elif defined(EA_PLATFORM_UNIX) || defined(EA_PLATFORM_APPLE)
			typedef void*            ModuleHandle;  // void*, from dlopen()
		#else
			typedef uintptr_t        ModuleHandle;
		#endif

		/// kModuleHandleInvalid
		///
		/// Shorthand for a handle that is default initialized or othewise invalid.
		///
		static const ModuleHandle kModuleHandleInvalid = 0;



		/// AddressRepType
		///
		/// Defines the text representation type of an program address. 
		/// If you have and address of 0x12345678, that address can be thought of
		/// as referring to a source file/line number combination, a C++ function
		/// name and byte offset, a C++ line of source code, or simply as a string
		/// of the value "0x12345678".
		///
		enum AddressRepType
		{
			kARTFileLine = 0,             /// Source code file path plus line number in the file, beginning with 1.
			kARTFunctionOffset,           /// Function name plus byte count of instructions into the address.
			kARTSource,                   /// Source code text.
			kARTAddress,                  /// The address as a hex number. 
			kARTCount                     /// Count of AddressRepTypes
		};


		/// AddressRepTypeFlag
		///
		/// Flag versions of AddressRepType.
		///
		enum AddressRepTypeFlag
		{
			kARTFlagNone           =  0,  /// No flag in particular.
			kARTFlagFileLine       =  1,  /// Source code file path plus line number in the file, beginning with 1.
			kARTFlagFunctionOffset =  2,  /// Function name plus byte count of instructions into the address.
			kARTFlagSource         =  4,  /// Source code text.
			kARTFlagAddress        =  8,  /// The address as a hex number.
			kARTFlagAll            = 15   /// All flags ORd together.
		};


		/// AddressRepEntry
		///
		/// Defines a representation of what an address refers to. If you have a call
		/// stack that refers to a given line of source code, that line can be represented
		/// as a file/line pair, a function/offset pair, or simply as the source code
		/// text itself.
		///
		struct EACALLSTACK_API AddressRepEntry
		{
			CAString8	 mAddressRep[kARTCount];
			unsigned int mARTCheckFlags : kARTCount; 
			unsigned int mFunctionOffset: 12;
			unsigned int mFileOffset    : 16;

			AddressRepEntry(EA::Allocator::ICoreAllocator* pCoreAllocator = NULL);
			void SetAllocator(EA::Allocator::ICoreAllocator* pCoreAllocator);
		};


		/// AddressToString
		///
		/// Converts an address to a string. Deals with platform differences.
		/// The address string will padded with zeroes on the left. It will
		/// be prefixed by 0x if the bLeading0x parameter is true.
		///
		EACALLSTACK_API FixedString& AddressToString(uint64_t address, FixedString& s, bool bLeading0x = true);
		EACALLSTACK_API CAString8&   AddressToString(uint64_t address, CAString8&   s, bool bLeading0x = true);

		// Deprecated:
		inline FixedString& AddressToString(const void* pAddress, FixedString& s, bool bLeading0x = true) { return AddressToString((uint64_t)(uintptr_t)pAddress, s, bLeading0x); }
		inline CAString8&   AddressToString(const void* pAddress, CAString8&   s, bool bLeading0x = true) { return AddressToString((uint64_t)(uintptr_t)pAddress, s, bLeading0x); }


		/// SymbolInfoType
		///
		/// Defines a type of symbol database. A Microsoft .pdb file is a symbol
		/// database, for example. This enumeration identifies a specific format
		/// of a database, and thus a .pdb from VC7 might be different from a 
		/// .pdb from VC8. 
		///
		enum SymbolInfoType
		{
			kSymbolInfoTypeNone,        /// No type in particular.
			kSymbolInfoTypeMapVC,       /// Used by VC++ (all versions).
			kSymbolInfoTypeMapGCC,      /// Used by GCC 3.x and later. The format varies a bit with successive GCC versions, and some GCCs (e.g. Apple) use an entirely different map file format.
			kSymbolInfoTypeMapSN,       /// Used by SN linker.
			kSymbolInfoTypeMapApple,    /// Used by Apple's GCC-based linker.
			kSymbolInfoTypePDB7,        /// Used by VC7 (VS 2003). PDB (program database) files.
			kSymbolInfoTypePDB8,        /// Used by VC8 (VS 2005). PDB (program database) files.
			kSymbolInfoTypeDWARF3,      /// Used by GCC 3.x and later. Embedded in .elf files.
		};

		/// TargetOS
		///
		/// Identifies an executable target OS.
		///
		enum TargetOS
		{
			kTargetOSNone,
			kTargetOSWin32,
			kTargetOSWin64,
			kTargetOSMicrosoftMobile,
			kTargetOSMicrosoftConsole,
			kTargetOSLinux,
			kTargetOSOSX,           // Apple desktop OS
			kTargetOSIOS,           // Apple mobile OS (e.g iPhone)
			kTargetOSAndroid,
			kTargetOSNew            // Identifies an arbitrary new OS that exists but isn't necessarily identifiable.
		};

		/// TargetProcessor
		///
		/// Identifies an executable target processor.
		///
		enum TargetProcessor
		{
			kTargetProcessorNone,
			kTargetProcessorPowerPC32,
			kTargetProcessorPowerPC64,
			kTargetProcessorX86,
			kTargetProcessorX64,
			kTargetProcessorARM
		};


		/// GetCurrentProcessPath
		///
		/// Retrieves the file system path to the current process.
		/// Returns the strlen of the copied path. Returns 0 upon failure.
		/// Doesn't work on XBox 360 in a release build unless EA_XBDM_ENABLED is defined as 1.
		///
		EACALLSTACK_API size_t GetCurrentProcessPath(wchar_t* pPath);


		/// GetCurrentProcessDirectory
		///
		/// Retrieves the file system directory of the current process.
		/// Returns the strlen of the copied path. Returns 0 upon failure.
		///
		EACALLSTACK_API size_t GetCurrentProcessDirectory(wchar_t* pDirectory);


		/// GetModuleBaseAddress
		///
		/// Returns the module (e.g. .exe .dll) current base address in memory.
		/// This is required for doing symbol lookups against symbol database files.
		/// The pModuleName must be the file path or file name. If pModuleName is 
		/// NULL, the current module assumed. The module name is case-sensitive on 
		/// some platforms.
		/// You don't need the full path of the module if the module is part of your 
		/// currently running app.
		/// This function returns the runtime base address for the given module and 
		/// does not return the default or preferred base address which is sometimes
		/// stored in the .map or .pdb symbol file for the module. Returns kBaseAddressInvalid if the 
		/// base address is unknown.
		/// This function is useful only when run live in a running application and 
		/// not in a tool or external application.
		///
		/// Example usage:
		///     GetModuleBaseAddress("MyApp.exe"); // MyApp.exe is part of the currently running module.
		///     GetModuleBaseAddress("C:\\Program Files\\MyApp\\MyPlugin.dll");
		/// 
		EACALLSTACK_API uint64_t GetModuleBaseAddress(const wchar_t* pModuleName = NULL);
		EACALLSTACK_API uint64_t GetModuleBaseAddress(const char* pModuleName = NULL);

		/// GetModuleBaseAddressByHandle
		///
		/// Returns the base address for the module (e.g. exe, dll)
		/// of the given handle value. Note that the meaning of ModuleHandle differs
		/// between platforms. Returns kBaseAddressInvalid if the base address is unknown.
		/// This function is useful only when run live in a running application and 
		/// not in a tool or external application.
		///
		/// Example usage:
		///     void DoSomething()
		///     {
		///         GetModuleBaseAddressByHandle(GetModuleHandle(NULL)); // Windows code.
		///     }
		/// 
		EACALLSTACK_API uint64_t GetModuleBaseAddressByHandle(ModuleHandle moduleHandle = kModuleHandleInvalid);

		/// GetModuleBaseAddressByAddress
		///
		/// Returns the base address for the module (e.g. exe, dll)
		/// that has code at the given code location. Returns kBaseAddressInvalid if the base address is unknown.
		/// This function is useful only when run live in a running application and 
		/// not in a tool or external application.
		/// Note that EACallstack/EACallstack.h has the GetModuleFromAddress function,
		/// which returns the module handle or file name, as opposed to its base address.
		///
		/// Example usage:
		///     uint64_t GetBaseAddressForCurrentModule() // Note that you could just use GetModuleBaseAddress(NULL) instead of implementing this function.
		///     {
		///         void* pInstruction;
		///         EAGetInstructionPointer(pInstruction);
		///         return GetModuleBaseAddressByAddress(pInstruction);
		///     }
		/// 
		EACALLSTACK_API uint64_t GetModuleBaseAddressByAddress(const void* pCodeAddress);


		/// ModuleInfo
		///
		struct EACALLSTACK_API ModuleInfo
		{
			ModuleHandle mModuleHandle;  // The ModuleHandle for the module.
			FixedString8 mPath;          // File name or file path
			FixedString8 mName;          // Module name. Usually the same as the file name without the extension.
			uint64_t     mBaseAddress;   // Base address in memory.
			uint64_t     mSize;          // Module size in memory.

			ModuleInfo() : mModuleHandle(kModuleHandleInvalid), mPath(), mName(), mBaseAddress(kBaseAddressInvalid), mSize(0){}
		};

		/// operator==(const ModuleInfo& lhs, const ModuleInfo& rhs)
		///
		/// Compares the contents of lhs and rhs and returns true if they are equal.
		/// lhs and rhs member variable mPath is expected to be a normalized path (not a relative path specifier or a symbolic link)
		EACALLSTACK_API bool operator==(const ModuleInfo& lhs, const ModuleInfo& rhs);

		/// GetModuleInfo
		///
		/// Copies up to moduleArrayCapacity ModuleInfo objects to pModuleInfoArray.
		/// Returns the number of modules copied to pModuleInfoArray.
		/// If pModuleInfoArray is NULL, then the return value is the required moduleArrayCapacity.
		///
		EACALLSTACK_API size_t GetModuleInfo(ModuleInfo* pModuleInfoArray, size_t moduleArrayCapacity);

		/// GetModuleInfoByAddress
		///
		/// Loops through pModuleInfoArray and finds module that given pCodeAddress is in. Copies the ModuleInfo into given outparam
		/// Returns true if module was found in pModuleInfoArray
		EACALLSTACK_API bool GetModuleInfoByAddress(const void* pCodeAddress, ModuleInfo& moduleInfo, ModuleInfo* pModuleInfoArray, size_t moduleCount);

		/// GetModuleInfoByHandle
		///
		/// Gets base address for moduleHandle then calls GetModuleInfoByAddress with that address
		/// Returns true if module was found in pModuleInfoArray
		EACALLSTACK_API bool GetModuleInfoByHandle(ModuleHandle moduleHandle, ModuleInfo& moduleInfo, ModuleInfo* pModuleInfoArray, size_t moduleCount);
		
		/// GetModuleInfoByName
		///
		/// Loops through pModuleInfoArray and finds module with same name as pModuleName. Copies the ModuleInfo into given outparam
		/// Returns true if module was found in pModuleInfoArray
		EACALLSTACK_API bool GetModuleInfoByName(const char* pModuleName, ModuleInfo& moduleInfo, ModuleInfo* pModuleInfoArray, size_t moduleCount);

		/// GetSymbolInfoTypeFromDatabase
		///
		/// Given a symbol database file, determine what kind of SymbolInfoType goes with it.
		/// Note that the term "symbol database" refers to a .pdb file, .map file, or
		/// .elf file with debug information.
		///
		EACALLSTACK_API SymbolInfoType GetSymbolInfoTypeFromDatabase(const wchar_t* pDatabaseFilePath, 
																	 TargetOS* pTargetOS = NULL, 
																	 TargetProcessor* pTargetProcessor = NULL);
		/// GetCurrentOS
		/// 
		/// Returns the OS that the current application was compiled for. EABase's EA_PLATFORM_XXX
		/// It's possible that the application might be running in some kind of emulation
		/// mode under another OS, though. The most common example is a Win32 app under Win64.
		/// This is useful for using in conjunction with GetSymbolInfoTypeFromDatabase.
		///
		TargetOS GetCurrentOS();


		/// GetCurrentProcessor
		/// 
		/// Returns the processor that the current application was compiled for. EABase's EA_PROCESSOR_XXX
		/// This is useful for using in conjunction with GetSymbolInfoTypeFromDatabase.
		///
		TargetProcessor GetCurrentProcessor();


		/// MakeAddressRepLookupFromDatabase
		///
		/// Returns a new appropriate IAddressRepLookup object based on the the given 
		/// database file path. Returns NULL if the type could not be determined.
		/// The return value is allocated via global new and the call assumes ownership
		/// of the pointer. This function does not call Init on the resulting instance;
		/// it is merely a factory for the instance.
		/// The returned IAddressRepLookup* should be destroyed with DestoryAddressRepLookup
		/// called with the same ICoreAllocator as was passed to this function.
		///
		EACALLSTACK_API IAddressRepLookup* MakeAddressRepLookupFromDatabase(const wchar_t* pDatabaseFilePath, 
																			EA::Allocator::ICoreAllocator* pCoreAllocator,
																			SymbolInfoType symbolInfoType = kSymbolInfoTypeNone);
		/// DestroyAddressRepLookup
		///
		/// Matches MakeAddressRepLookupFromDatabase
		///
		EACALLSTACK_API void DestroyAddressRepLookup(IAddressRepLookup* pLookup, EA::Allocator::ICoreAllocator* pCoreAllocator);


		/// IAddressRepLookupBase
		///
		class EACALLSTACK_API IAddressRepLookupBase
		{
		public:
			/// GetAddressRep
			///
			/// Returns an address representation (e.g. file/line) for a given address.
			/// See AddressRepLookupSet for example usage.
			///
			virtual int GetAddressRep(int addressRepTypeFlags, uint64_t address, FixedString* pRepArray, int* pIntValueArray) = 0;

			// Deprecated:
			virtual int GetAddressRep(int addressRepTypeFlags, const void* pAddress, FixedString* pRepArray, int* pIntValueArray)
				{ return GetAddressRep(addressRepTypeFlags, (uint64_t)(uintptr_t)pAddress, pRepArray, pIntValueArray); }

			virtual ~IAddressRepLookupBase() {}
		};


		/// IAddressRepLookup
		///
		/// This is the virtual base class for all symbol lookups.
		/// We call this AddressRepLookup because it looks up a representation
		/// for a process's instruction address, and this doesn't necessarily involve  
		/// just debug symbols. It can additionally include, for example, source code.
		///
		/// If you are implementing an instance of this class, your instance usually
		/// need only respect the kARTFunctionOffset and kARTFileLine flags. 
		/// The kARTFlagSource and kARTFlagAddress flags are taken care of automatically 
		/// in the AddressRepLookupSet class, as they are independependent of symbol files.
		///
		class EACALLSTACK_API IAddressRepLookup : public IAddressRepLookupBase
		{
		public:
			/// ~IAddressRepLookup
			virtual ~IAddressRepLookup() { }

			/// AsInterface
			/// Safely and dynamically casts this instance to a parent or child class.
			/// A primary usage of this is to allow code to safely downcast an IAddressRepLookup
			/// to a subclass, usually for the purpose of calling functions specific to that 
			/// concrete class.
			///
			/// Example usage:
			///    DWARF3File* pDwarfLookup = (DWARF3File*)pAddressRepLookup->AsInterface(DWARF3File::kTypeId);
			///    if(pDwarfLookup)
			///        pDwarfLookup->Blah();
			virtual void* AsInterface(int typeId) = 0;

			/// SetAllocator
			///
			/// Sets the memory allocator used by an instance of this class. By default
			/// the global EACallstack ICoreAllocator is used (EA::Callstack::GetAllocator()).
			/// 
			virtual void SetAllocator(EA::Allocator::ICoreAllocator* pCoreAllocator) = 0;

			/// Init
			///
			/// Initializes the given instance with a symbol file path.
			/// If bDelayLoad is true, the instance is expected to delay loading of 
			/// symbol information until the GetAddressRep function is called.
			///
			virtual bool Init(const wchar_t* pDatabasePath, bool bDelayLoad = true) = 0;

			/// Shutdown
			///
			/// Returns the instance to a state equivalent to that before Init was called.
			///
			virtual bool Shutdown() = 0;

			/// enum Option
			///
			enum Option
			{
				kOptionCopyFileToMemory = 1,  /// Value is 0 or 1; default is 0. A value of 1 means the symbol file is entirely copied to system memory during usage. This results in (potentially much) larger memory usage, but faster performance.
				kOptionSymbolCacheLevel = 2,  /// Value is >= 0; default is 0. A value of 0 means there is no symbol caching; less memory is used, but lookups are significantly slower. A value of 1 enables basic symbol caching.
				kOptionLowMemoryUsage   = 3,  /// Value is 0 or 1; default is 0. A value of 1 means that memory usage is reduced, but lookups may be significantly slower. This is usually due to reading the symbols as we go instead of loading the entire file. This is not the same as kOptionCopyFileToMemory, though can be used in conjunction with it.
				kOptionFileBufferSize   = 4,  /// Value is between 64 and 65536. Specifies the size of the file buffer used for file reading. This is especially applicable in the case of kOptionLowMemoryUsage being enabled. Default is typically 4096.
				kOptionOfflineLookup    = 5   /// Value is 0 or 1; default is 0. Specifies if lookups will be done for addresses of an offline (remote) application, such as a PC analyzing console addresses. A value of 0 means that the addresses correspond to the currently running application. This option affects the ability of the DB to dynamically look up its base address at runtime in order to support DLLs. With a value of 0 (runtime) DBs for DLLs can guess their base address correctly, but with a value of 1 (offline) the user will need to manually supply the base addresses to DLLs (PRXs) that have been relocated when the app was run.
			};

			/// SetOption
			///
			/// Options must be set before Init is called.
			/// Options are persistent after Shutdown is called. They are not cleared in the case of Shutdown.
			///
			/// Example usage:
			///    pAddressRepLookup->SetOption(IAddressRepLookup::kOptionSymbolCacheLevel, 1);
			///
			virtual void SetOption(int option, int value) = 0;

			/// SetBaseAddress
			///
			/// Sets the base address of the module. 
			/// If the base address is not set, then the GetAddressRep function assumes that
			/// the base address is the base address of the currently executing module.
			/// You should call this function when you are reading symbols for an address
			/// of a process that isn't the one currently running.
			/// This function should be called before Init.
			///
			virtual void SetBaseAddress(uint64_t baseAddress) = 0;

			/// GetBaseAddress
			///
			/// Returns the base address for the module, which is the address set by SetBaseAddress.
			/// If SetBaseAddress was not called then it is the default address for the module or kBaseAddressUnspecified
			/// if the default hasn't been calculated yet.
			virtual uint64_t GetBaseAddress() const = 0;

			/// GetAddressRep
			///
			/// Inherited from the IAddressRepLookupBase base class.
			/// Returns an address representation (e.g. file/line) for a given address.
			/// See AddressRepLookupSet for example usage.
			///
			virtual int GetAddressRep(int addressRepTypeFlags, uint64_t address, FixedString* pRepArray, int* pIntValueArray) = 0;

			// Deprecated:
			virtual int GetAddressRep(int addressRepTypeFlags, const void* pAddress, FixedString* pRepArray, int* pIntValueArray)
				{ return GetAddressRep(addressRepTypeFlags, (uint64_t)(uintptr_t)pAddress, pRepArray, pIntValueArray); }

			/// GetDatabasePath
			///
			/// Returns the path to the database source (usually a disk file) specified
			/// in the Init function.
			virtual const wchar_t* GetDatabasePath() const = 0;
		};



		/// AddressRepLookupSet
		///
		/// Implements a container of IAddressRepLookup objects, presenting them as a single
		/// IAddressRepLookup interface. It may happen that you have multiple symbol files
		/// (e.g. .pdb files) to work with. This class manages the set of them and makes it
		/// so you can work with a single interface to all of them.
		/// 
		/// This class additionally has the ability to implement kARTSource lookups using
		/// the information from kARTFileLine lookups.
		///
		/// Normally you will want to use the AddressRepCache class instead of this, as 
		/// that class owns an instance of this class but will cache lookup results it 
		/// gets from this class and will thus run much faster under most practical 
		/// circumstances, though at the cost of the cache memory.
		///
		class EACALLSTACK_API AddressRepLookupSet : public IAddressRepLookupBase
		{
		public:
			AddressRepLookupSet(EA::Allocator::ICoreAllocator* pCoreAllocator = NULL);
		   ~AddressRepLookupSet();

			/// SetAllocator
			///
			/// Sets the memory allocator used by an instance of this class. By default
			/// the global EACallstack ICoreAllocator is used (EA::Callstack::GetAllocator()).
			/// 
			void SetAllocator(EA::Allocator::ICoreAllocator* pCoreAllocator);


			/// Init
			///
			/// Initializes the class for use after construction. Init is the first function 
			/// that may possibly allocate memory or resources needed for this class to work.
			///
			bool Init();


			/// Shutdown    
			///
			/// Shuts down an instance of this class to match a call to Init. Upon Shutdown,
			/// any resources allocated during or after Init will be freed.
			///
			bool Shutdown();


			/// GetLookupsAreLocal    
			///
			/// Indicates whether lookups are being done refer to the current process, as opposed to 
			/// some other process. There are some advantages that can be had when looking up symbols
			/// for the current process, such as using some kinds of system-provided lookup functionality.
			///
			bool GetLookupsAreLocal() const        { return mbLookupsAreLocal;   }
			void SetLookupsAreLocal(bool bLocal)   { mbLookupsAreLocal = bLocal; }


			/// AddDatabaseFile
			///
			/// Adds a symbol database for use when looking up symbols for a program address.
			/// A database is a .pdb file, .map file, or .elf file, for example.
			/// A variety of symbol database formats are supported, including PDB, DWARF3 (contained
			/// in ELF/SELF executables), and Map files. Currently the type of database is
			/// determined by the file extension in the path provided.
			/// The return value indicates whether the given is registered upon return.
			/// Doesn't re-register a database file if it's already registered.
			///
			/// This function is a higher level alternative to the AddAddressRepLookup function. 
			/// It creates the appropriate IAddressRepLookup class from the given symbol
			/// file and then calls AddAddressRepLookup.
			///
			/// The pCoreAllocator argument specifies the ICoreAllocator that the database file
			/// object (an instance of IAddressRepLookup) will use. If pCoreAllocator is NULL, 
			/// our own allocator will be used.
			///
			bool AddDatabaseFile(const wchar_t* pDatabaseFilePath, uint64_t baseAddress = kBaseAddressUnspecified, 
								  bool bDelayLoad = true, EA::Allocator::ICoreAllocator* pCoreAllocator = NULL);

			/// Same as AddDatabaseFile excepts adds two bool arguments.
			/// If bDBPreviouslyPresent, bBaseAddressAlreadySet both return as true then 
			/// this call had no effect, but the return value will still be true.
			/// The return value indicates whether the given is registered upon return.
			bool AddDatabaseFileEx(const wchar_t* pDatabaseFilePath, uint64_t baseAddress, 
								   bool bDelayLoad, EA::Allocator::ICoreAllocator* pCoreAllocator,
								   bool& bDBPreviouslyPresent, bool& bBaseAddressAlreadySet);

			/// RemoveDatabaseFile
			///
			/// Removes a database file specified by AddDatabaseFile.
			/// Returns true if a the database file was found, in which case it can always
			/// be removed.
			///
			bool RemoveDatabaseFile(const wchar_t* pDatabaseFilePath);


			/// GetDatabaseFileCount
			///
			/// Returns the number of databases successfully added via AddDatabaseFile.
			///
			size_t GetDatabaseFileCount() const;


			/// GetDatabaseFile
			///
			/// Returns the Nth IAddressRepLookup which was added via AddDatabaseFile.
			/// Returns NULL if the given index is >= the database file count.
			/// The return value is valid only as long as RemoveDatabaseFile isn't called.
			///
			IAddressRepLookup* GetDatabaseFile(size_t index);


			/// GetAddressRepLookup
			///
			/// Returns the IAddressRepLookup for a database file previously registered via AddDatabaseFile.
			/// Returns NULL if no database file for the given path was added, or if one was added by removed via RemoveDatabaseFile.
			/// The user can use AsInterface to cast the return value to a specific type.
			///
			IAddressRepLookup* GetAddressRepLookup(const wchar_t* pDatabaseFilePath);


			/// AddAddressRepLookup
			///
			/// Adds an IAddressRepLookup object to the managed set. 
			/// The added object is added to the back of the queue of IAddressRepLookup
			/// objects that are consulted in the GetAddressRep function.
			/// It is expected that the Init function has already been called on the 
			/// pAddressRepLookup instance.
			///
			/// This function is a lower level alternative to the AddDatabaseFile function 
			/// and is preferred when you want to have more control over the AddressRepLookup
			/// object, such as specifying a custom memory allocator. 
			///
			void AddAddressRepLookup(IAddressRepLookup* pAddressRepLookup);


			/// AddSourceCodeDirectory
			///
			/// Adds a file system directory to the set we use to find file source code.
			/// If a given directory is the root of a tree of source directories, you need only
			/// supply the root directory. Since most symbol databases store full paths
			/// to source files, often only the home or root directory location of the source
			/// code is required. The source directory supplied here need not be the same 
			/// directory path as the one that the symbol files were built with.
			///
			void AddSourceCodeDirectory(const wchar_t* pSourceDirectory);


			/// GetAddressRep
			///
			/// Looks up one or more of the given AddressRepType values. We have a single function for
			/// all three types because it is much faster to do lookups of multiple types at the same time.
			/// Returns flags indicating which of the AddressRepType values were successfully looked up.
			/// The input pRepArray and pIntValueArray need to have a capacity for all AddressRepTypes
			/// (i.e. kARTCount), as each result is written to its position in the array.
			/// Unused results are set to empty strings.
			/// The kARTAddress AddressRepType always succeeds, as it is merely a translation
			/// of a pointer to a hex number.
			///
			/// Example usage:
			///     FixedString pStrResults[kARTCount];
			///     int         pIntResults[kARTCount];
			///     int         inputFlags = (kARTFlagFunctionOffset | kARTFlagFileLine | kARTFlagSource);
			///     int         outputFlags;
			///
			///     outputFlags = addressRepLookup.GetAddressRep(inputFlags, pSomeAddress, pStrResults, pIntResults);
			///
			///     if(outputFlags & kARTFlagFileLine)
			///         printf("File: %s, line: %d", pStrResults[kARTFileLine].c_str(), pIntResults[kARTFileLine]);
			///
			int GetAddressRep(int addressRepTypeFlags, uint64_t address, FixedString* pRepArray, int* pIntValueArray);

			// Deprecated:
			int GetAddressRep(int addressRepTypeFlags, const void* pAddress, FixedString* pRepArray, int* pIntValueArray)
				{ return GetAddressRep(addressRepTypeFlags, (uint64_t)(uintptr_t)pAddress, pRepArray, pIntValueArray); }


			/// EnableAutoDatabaseFind
			///
			/// Enables or disables the ability to automatically find the associated symbol file
			/// for the currently running application. This is simply a matter of looking for files
			/// with the same name as the executable but with a different extension, such as .map.
			/// This functionality is only useful when an application needs to do symbol lookups
			/// on itself, and is not useful for when you are doing symbol lookups for some other
			/// application. Auto-find only occurs if the user hasn't added any IAddressRepLookup
			/// objects. By default auto-database-find is enabled.
			///
			/// If you expect to use this class within an executing exception handler, you might 
			/// want to disable auto-database-find, at least with some platforms. For example, the 
			/// PS3 platform exception handling system gives you few resources to work with and 
			/// users have seen crashes when trying to auto-load DBs during exception handling.
			///
			void EnableAutoDatabaseFind(bool bEnable);


			/// SetSourceCodeLineCount
			///
			/// Sets the number of lines shown for the given address.
			/// The default value is defined by kSourceContextLineCount. 
			///
			void     SetSourceCodeLineCount(unsigned lineCount);
			unsigned GetSourceCodeLineCount() const { return mnSourceContextLineCount; }

		protected:
			/// AutoDatabaseAdd
			void AutoDatabaseAdd();

			/// Looks up the AddressRep in our mAddressRepLookupList. 
			int GetAddressRepFromSet(int addressRepTypeFlags, uint64_t address, FixedString* pRepArray, int* pIntValueArray);

			/// Gets a kARTAddress for the given pAddress. This is a simple operation,
			/// as it merely calls the AddressToString function, which itself merely
			/// takes a numeric address and converts it to a string.
			int GetAddressRepAddress(uint64_t address, FixedString* pRepArray, int* pIntValueArray);

			/// Attempts to get a kARTSource for the given pAddress.
			int GetAddressRepSource(FixedString* pRepArray, int* pIntValueArray);
		  
		protected:
			typedef eastl::list<FixedStringW,       EA::Callstack::EASTLCoreAllocator> FixedStringWList;
			typedef eastl::list<IAddressRepLookup*, EA::Callstack::EASTLCoreAllocator> AddressRepLookupList;

			EA::Allocator::ICoreAllocator* mpCoreAllocator;
			bool                           mbLookupsAreLocal;         /// If true, then the lookups we are doing refer to the current process. True by default.
			bool                           mbEnableAutoDatabaseFind;  /// If true, look for DB files in our home directory if mAddressRepLookupList is empty. 
			bool                           mbLookedForDBFiles;        /// If true, then our file lists are empty and we already looked for them in our home directory.
			AddressRepLookupList           mAddressRepLookupList;
			FixedStringWList               mSourceCodeDirectoryList;
			int                            mnSourceCodeFailureCount;
			unsigned                       mnSourceContextLineCount;
		};



		/// AddressRepCache
		///
		/// Implements a cache of AddressRepEntry values.
		///
		/// It is expensive to do lookups of AddressRepEntry information out of source code
		/// map or database files (e.g. VC++ .pdb files). So we implement a cache of
		/// results so that database lookups for a given address need only occur once.
		///
		/// Example usage:
		///     AddressRepCache arc;
		///     arc.AddDatabaseFile(L"D:\application.pdb");
		///
		///     int   line;
		///     char* pFile = arc.GetAddressRep(kARTFlagFileLine, 0x12345678, &line);
		///
		///     if(pFile)
		///         printf("Address: 0x%08x -> File: %s, line: %d", 0x12345678, pFile, line);
		///
		class EACALLSTACK_API AddressRepCache : public IAddressRepLookupBase
		{
		public:
			AddressRepCache(EA::Allocator::ICoreAllocator* pCoreAllocator = NULL);
		   ~AddressRepCache();


			/// SetAllocator
			///
			/// Sets the memory allocator used by an instance of this class. By default
			/// the global EACallstack ICoreAllocator is used (EA::Callstack::GetAllocator()).
			/// 
			void SetAllocator(EA::Allocator::ICoreAllocator* pCoreAllocator);


			/// GetLookupsAreLocal    
			///
			/// Indicates whether lookups are being done refer to the current process, as opposed to 
			/// some other process. There are some advantages that can be had when looking up symbols
			/// for the current process, such as using some kinds of system-provided lookup functionality.
			///
			bool GetLookupsAreLocal() const
				{ return mAddressRepLookupSet.GetLookupsAreLocal(); }
				
			void SetLookupsAreLocal(bool bLocal)
				{ mAddressRepLookupSet.SetLookupsAreLocal(bLocal); }


			/// AddDatabaseFile
			///
			/// Adds a symbol database for use when looking up symbols for a program address.
			/// A database is a .pdb file, .map file, or .elf file, for example.
			/// A variety of symbol database formats are supported, including PDB, DWARF3 (contained
			/// in ELF/SELF executables), and Map files. Currently the type of database is
			/// determined by the file extension in the path provided.
			/// Doesn't re-register a database file if it's already registered.
			///
			/// The pCoreAllocator argument specifies the ICoreAllocator that the database file
			/// object (an instance of IAddressRepLookup) will use. If pCoreAllocator is NULL, 
			/// our own allocator will be used.
			///
			/// See AddressRepLookupSet::AddDatabaseFile.
			///
			bool AddDatabaseFile(const wchar_t* pDatabaseFilePath, uint64_t baseAddress = kBaseAddressUnspecified, 
								  bool bDelayLoad = true, EA::Allocator::ICoreAllocator* pCoreAllocator = NULL)
				{ return mAddressRepLookupSet.AddDatabaseFile(pDatabaseFilePath, baseAddress, bDelayLoad, pCoreAllocator); } 

			/// See AddressRepLookupSet::AddDatabaseFileEx.
			bool AddDatabaseFileEx(const wchar_t* pDatabaseFilePath, uint64_t baseAddress, 
								   bool bDelayLoad, EA::Allocator::ICoreAllocator* pCoreAllocator,
								   bool& bDBPreviouslyPresent, bool& bBaseAddressAlreadySet)
				{ return mAddressRepLookupSet.AddDatabaseFileEx(pDatabaseFilePath, baseAddress, bDelayLoad, pCoreAllocator, bDBPreviouslyPresent, bBaseAddressAlreadySet); }


			/// RemoveDatabaseFile
			///
			/// Removes a database file specified by AddDatabaseFile.
			/// Returns true if a the database file was found, in which case it can always
			/// be removed. 
			/// This function does not clear any cache entries that derived from the 
			/// given database file.
			///
			bool RemoveDatabaseFile(const wchar_t* pDatabaseFilePath)
				{ return mAddressRepLookupSet.RemoveDatabaseFile(pDatabaseFilePath); } 


			/// GetAddressRepLookupSet
			///
			/// Returns the underlying AddressRepLookupSet used for lookups.
			///
			AddressRepLookupSet& GetAddressRepLookupSet()
				{ return mAddressRepLookupSet; }

				
			/// GetAddressRepLookup
			///
			/// The user can use AsInterface to cast the return value to a specific type.
			///
			IAddressRepLookup* GetAddressRepLookup(const wchar_t* pDatabaseFilePath)
				{ return mAddressRepLookupSet.GetAddressRepLookup(pDatabaseFilePath); }


			/// AddAddressRepLookup
			///
			/// Adds an IAddressRepLookup object to the managed set. 
			/// The added object is added to the back of the queue of IAddressRepLookup
			/// objects that are consulted in the GetAddressRep function.
			/// It is expected that the Init function has already been called on the 
			/// pAddressRepLookup instance.
			///
			/// This function is a lower level alternative to the AddDatabaseFile function 
			/// and is preferred when you want to have more control over the AddressRepLookup
			/// object, such as specifying a custom memory allocator. 
			///
			void AddAddressRepLookup(IAddressRepLookup* pAddressRepLookup)
				{ return mAddressRepLookupSet.AddAddressRepLookup(pAddressRepLookup); } 


			/// GetDatabaseFileCount
			///
			/// See IAddresssRepLookupSet::GetDatabaseFileCount
			///
			size_t GetDatabaseFileCount() const
				{ return mAddressRepLookupSet.GetDatabaseFileCount(); } 


			/// GetDatabaseFile
			///
			/// See IAddresssRepLookupSet::GetDatabaseFile
			///
			IAddressRepLookup* GetDatabaseFile(size_t index)
				{ return mAddressRepLookupSet.GetDatabaseFile(index); } 


			/// AddSourceCodeDirectory
			///
			/// Adds a file system directory to the set we use to find file source code.
			/// If a given directory is the root of a tree of source directories, you need only
			/// supply the root directory. Since most symbol databases store full paths
			/// to source files, often only the home directory or root drive location of
			/// the source code is required when loading source text from the host PC.
			///
			void AddSourceCodeDirectory(const wchar_t* pSourceDirectory)
				{ mAddressRepLookupSet.AddSourceCodeDirectory(pSourceDirectory); } 


			/// SetSourceCodeLineCount
			///
			/// Sets the number of lines shown for the given address.
			/// The default value is defined by kSourceContextLineCount. 
			///
			void SetSourceCodeLineCount(unsigned lineCount)
				{ mAddressRepLookupSet.SetSourceCodeLineCount(lineCount); } 

			unsigned GetSourceCodeLineCount() const
				{ return mAddressRepLookupSet.GetSourceCodeLineCount(); }
 

			/// GetAddressRep
			///
			/// Inherited from the IAddressRepLookupBase base class.
			/// Returns an address representation (e.g. file/line) for a given address.
			/// See AddressRepLookupSet for example usage.
			///
			int GetAddressRep(int addressRepTypeFlags, uint64_t address, FixedString* pRepArray, int* pIntValueArray);

			// Deprecated:
			int GetAddressRep(int addressRepTypeFlags, const void* pAddress, FixedString* pRepArray, int* pIntValueArray)
				{ return GetAddressRep(addressRepTypeFlags, (uint64_t)(uintptr_t)pAddress, pRepArray, pIntValueArray); }


			/// GetAddressRep
			/// 
			/// Gets an individual address representation.
			/// This function is an alternative to the GetAddressRep function which takes
			/// addreessRepFlags and returns potentially more than one address rep.
			/// 
			const char* GetAddressRep(AddressRepType addressRepType, uint64_t address, int* pIntValue, bool bLookupIfMissing);

			// Deprecated:
			const char* GetAddressRep(AddressRepType addressRepType, const void* pAddress, int* pIntValue, bool bLookupIfMissing)
				{ return GetAddressRep(addressRepType, (uint64_t)(uintptr_t)pAddress, pIntValue, bLookupIfMissing); }


			/// GetAddressRep
			///
			/// Looks up one or more of the given AddressRepType values. We have a single function for
			/// all three types because it is much faster to do lookups of multiple types at the same time.
			/// Returns flags indicating which of the AddressRepType values were successfully looked up.
			/// The input pRepArray and pIntValueArray need to have a capacity for all AddressRepTypes
			/// (i.e. kARTCount), as each result is written to its position in the array.
			/// Unused results are set to empty strings.
			/// The kARTAddress AddressRepType always succeeds, as it is merely a translation
			/// of a pointer to a hex number.
			///
			/// Example usage:
			///     const char* pStrResults[kARTCount];
			///     int         pIntResults[kARTCount];
			///     int         inputFlags = (kARTFlagFunctionOffset | kARTFlagFileLine | kARTFlagSource);
			///     int         outputFlags;
			///
			///     outputFlags = addressRepLookup.GetAddressRep(inputFlags, pSomeAddress, pStrResults, pIntResults);
			///
			///     if(outputFlags & kARTFlagFileLine)
			///         printf("File: %s, line: %d", pStrResults[kARTFileLine], pIntResults[kARTFileLine]);
			///
			int GetAddressRep(int addressRepTypeFlags, uint64_t address, const char** pRepArray, int* pIntValueArray, bool bLookupIfMissing = true);

			// Deprecated:
			int GetAddressRep(int addressRepTypeFlags, const void* pAddress, const char** pRepArray, int* pIntValueArray, bool bLookupIfMissing = true)
				{ return GetAddressRep(addressRepTypeFlags, (uint64_t)(uintptr_t)pAddress, pRepArray, pIntValueArray, bLookupIfMissing); }


			/// SetAddressRep
			///
			/// Sets the AddressRepEntry associated with the program address to the given data.
			/// Typically this is only used internally by the AddressRepCache class.
			///
			void SetAddressRep(AddressRepType addressRepType, uint64_t address, const char* pRep, size_t nRepLength, int intValue);

			// Deprecated:
			void SetAddressRep(AddressRepType addressRepType, const void* pAddress, const char* pRep, size_t nRepLength, int intValue)
				{ return SetAddressRep(addressRepType, (uint64_t)(uintptr_t)pAddress, pRep, nRepLength, intValue); }


			/// EnableAutoDatabaseFind
			///
			void EnableAutoDatabaseFind(bool bEnable)
				{ mAddressRepLookupSet.EnableAutoDatabaseFind(bEnable); } 


			/// kMaxCacheSizeDefault
			///
			static const size_t kMaxCacheSizeDefault = 32768;

			/// SetMaxCacheSize
			///
			/// Sets the max amount of memory the cache uses.
			/// Without a max cache limit, the cache would grow without bounds. This function limits
			/// it to a given number of types. The cache size limit applies only to the address representation
			/// cache and not to the memory used by individual AddressRepLookup classes such as the PDBFile class.
			/// The default max cache size is kMaxCacheSizeDefault.
			/// 
			void SetMaxCacheSize(size_t maxSize);


			/// PurgeCache
			///
			/// Purges entries from the cache. How much to purge is determined by the newCacheSize
			/// argument. The newCacheSize argument specifies the desired size of the cache after
			/// the purge. The newCacheSize argument is a hint and the actual amount of remaining
			/// memory may be slightly different from newCacheSize, due to the size of cache entries.
			///
			void PurgeCache(size_t newCacheSize = 0);

		protected:
			size_t CalculateMemoryUsage(bool bNewNode, size_t nRepLength);

		protected:
			typedef eastl::hash_map<uint64_t, AddressRepEntry, eastl::hash<uint64_t>, eastl::equal_to<uint64_t>, 
									 EA::Callstack::EASTLCoreAllocator> AddressRepMap;

			EA::Allocator::ICoreAllocator* mpCoreAllocator;        /// The memory allocator we use.
			AddressRepMap                  mAddressRepMap;         /// The address representation cache.
			AddressRepLookupSet            mAddressRepLookupSet;   /// The address representation lookup.
			size_t                         mCacheSize;             /// The current estimated cache size.
			size_t                         mMaxCacheSize;          /// The max size in bytes that the cache (mAddressRepMap) uses.

		}; // AddressRepCache



		/// GetAddressRepCache
		///
		/// Retrieves the default global AddressRepCache object.
		///
		EACALLSTACK_API AddressRepCache* GetAddressRepCache();


		/// SetAddressRepCache
		///
		/// Sets the default global AddressRepCache object.
		/// Returns the old AddressRepCache object.
		/// The old AddressRepCache is not deleted, as this function is 
		/// merely an accessor modifier.
		///
		EACALLSTACK_API AddressRepCache* SetAddressRepCache(AddressRepCache* pAddressRepCache);


	} // namespace Callstack

} // namespace EA


#ifdef _MSC_VER
	#pragma warning(pop)
#endif


#endif // Header include guard.







