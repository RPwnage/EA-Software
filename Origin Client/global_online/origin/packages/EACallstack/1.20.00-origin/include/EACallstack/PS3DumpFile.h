///////////////////////////////////////////////////////////////////////////////
// PS3DumpFile.h
//
// Copyright (c) 2009 Electronic Arts Inc.
// Developed and maintained by Paul Pedriana.
//
// Implements an interpreter for a PS3 dump file.
///////////////////////////////////////////////////////////////////////////////


#ifndef EACALLSTACK_PS3DUMPFILE_H
#define EACALLSTACK_PS3DUMPFILE_H


#include <EACallstack/internal/Config.h>
#include <EACallstack/internal/EASTLCoreAllocator.h>
#include <EACallstack/internal/PS3DumpELF.h>
#include <EACallstack/EAAddressRep.h>
#include <EACallstack/ELFFile.h>
#include <EAIO/EAFileStream.h>
#include <coreallocator/icoreallocator_interface.h>
#include <EASTL/vector.h>


#ifdef _MSC_VER
    #pragma warning(push)           // We have to be careful about disabling this warning. Sometimes the warning is meaningful; sometimes it isn't.
    #pragma warning(disable: 4251)  // class (some template) needs to have dll-interface to be used by clients.
#endif


namespace EA
{
    namespace Callstack
    {

        /// PS3MemoryDump
        ///
        /// We implement a utility class that manages the memory records in a PS3 dump.
        ///
        class PS3MemoryDump
        {
        public:
            PS3MemoryDump();

            void Init(EA::IO::IStream* pStream);
            void AddProcessMemoryData(const PS3DumpELF::ProcessMemoryData& pmd);

            // Returns true if the memory could be entirely read.
            bool ReadMemory(void* pDest, uint64_t address, uint64_t byteCount) const;

            // Reads basic types. Return value is in local endian.
            uint8_t   ReadUint8  (uint64_t address) const;
            uint16_t  ReadUint16 (uint64_t address) const;
            uint32_t  ReadUint32 (uint64_t address) const;
            uint64_t  ReadUint64 (uint64_t address) const;

        public: // We don't really gain much by making this protected.
            typedef eastl::vector<PS3DumpELF::ProcessMemoryData> ProcessMemoryDataArray;

            IO::IStream*            mpStream;                   // The file stream for the ELF file. This is set externally; we do not allocate it.
            ProcessMemoryDataArray  mProcessMemoryDataArray;    // Stores info about the memory in the dump file. This doesn't store the memory itself, as that's potentially hundreds of megabytes.
        };



        /// PS3SPUMemoryDump
        ///
        /// Holds all the SPU thread memory images.
        ///
        class PS3SPUMemoryDump
        {
        public:
            PS3SPUMemoryDump();

            void Init(EA::IO::IStream* pStream);

            void AddLocalStorageData(const PS3DumpELF::SPULocalStorageData& lsd);

            const PS3DumpELF::SPULocalStorageData* GetLocalStorageData(uint32_t spuThreadID) const;

            // Returns true if the memory could be entirely read.
            bool ReadMemory(uint32_t spuThreadID, void* pDest, uint32_t address, uint32_t byteCount) const;

            // Finds where in the SPU memory the ELF image is, which will be zero if the image hasn't be displaced.
            // Returns -1 if the image is not found, otherwise returns a value in the range of [0, 0x40000). 
            // However, it should always be found for valid SPU memory images.
            int32_t GetELFImageOffset(uint32_t spuThreadID) const;

            // Reads basic types. Return value is in local endian.
            uint8_t   ReadUint8  (uint32_t spuThreadID, uint32_t address) const;
            uint16_t  ReadUint16 (uint32_t spuThreadID, uint32_t address) const;
            uint32_t  ReadUint32 (uint32_t spuThreadID, uint32_t address) const;
            uint64_t  ReadUint64 (uint32_t spuThreadID, uint32_t address) const;

        public: // We don't really gain much by making this protected.
            typedef eastl::vector<PS3DumpELF::SPULocalStorageData> LocalStorageDataArray;
            static const uint32_t kMemorySize = 0x40000;

            IO::IStream*          mpStream;                 // The file stream for the ELF file. This is set externally; we do not allocate it.
            LocalStorageDataArray mLocalStorageDataArray;   // 
        };



        /// PS3DumpFile
        ///
        /// Example usage:
        ///    PS3DumpFile ps3DumpFile(pCoreAllocator);
        ///
        ///    if(ps3DumpFile.Init(L"C:\\ps3core-SuperEAGame.elf", false)) {
        ///        eastl::string sOutput;
        ///        ps3DumpFile.Trace(sOutput, PS3DumpFile::kTSAutoDetect);
        ///        printf("%s", sOutput.c_str());
        ///    }
        ///
        /// For additional sample code, see the EACallstack unit tests.
        ///
        class EACALLSTACK_API PS3DumpFile
        {
        public:
            PS3DumpFile(EA::Allocator::ICoreAllocator* pCoreAllocator = NULL);
           ~PS3DumpFile();

            /// Should be called before any other function if the allocator wasn't 
            /// already set in the constructor. 
            void SetAllocator(EA::Allocator::ICoreAllocator* pCoreAllocator);

            /// Must be called before calling Load or any function that works with the loaded data.
            bool Init(const wchar_t* pDumpFilePath, bool bDelayLoad = true);

            /// Explicitly shut down the PS3DumpFile.
            /// Restores the state to that before Init.
            bool Shutdown();

            /// This function is used to allow for callstack symbol lookups for PPU and SPU threads.
            /// This is very much like IAddressRepLookup::AddDatabaseFile. 
            /// You can add multiple .elf/.self files or .map files to represent the PPU ELF and the 
            /// various SPU ELF files that may be involved in the process by calling this function multiple times.
            /// This function needs to be called before the Trace function or any other member function
            /// which could do symbol lookups.
            bool AddDatabaseFile(const wchar_t* pDatabaseFilePath, uint64_t baseAddress = 0, 
                                 bool bDelayLoad = true, EA::Allocator::ICoreAllocator* pCoreAllocator = NULL);

            // This is very much like AddressRepLookupSet::AddSourceCodeDirectory.
            void AddSourceCodeDirectory(const wchar_t* pSourceDirectory);

            /// Manually load the dump file. This is not needed if Init was called with bDelayLoad == false.
            void Load();

            ///////////////////////////////////////
            // Enumerations for the Trace function.
            ///////////////////////////////////////

            enum TraceStrategy
            {
                kTSCrashAnalysis = 0x00000001,          // Limit the output to display of information relative to a crash. Can be used with kTSGeneralTrace. 
                kTSGeneralTrace  = 0x00000002,          // Display a trace of the machine state in the dump. Can be used with kTSCrashAnalysis.
                kTSAutoDetect    = 0x10000000           // Auto-detect whether this is a crash or a static dump. Act as kTSCrashAnalysis if the dump represents a crash, else act as kTSGeneralTrace. 
            };

            enum TraceFilter
            {
                // General
                kTFSummaryData          = 0x00000001,     // 
                kTFProcessData          = 0x00000002,     // 
                kTFCrashCauseData       = 0x00000004,     // Details on crash cause beyond thread data (i.e. beyond kTAPPUData, kTASPUData, kTARSXData).
                kTFPRXData              = 0x00000008,     // List of PRXs (i.e. PS3 DLLs)
                kTFSkipCoreDumpThread   = 0x00000010,     // Don't report the core dump handler thread itself.

                // PPU
                kTFPPUData              = 0x00000100,     // Thread context: registers, callstack, stack data.
                kTFPPUMemoryData        = 0x00000200,     // 
                kTFPPUCallstackData     = 0x00000400,     // 
                kTFPPURegisterData      = 0x00000800,     // 
                kTFPPUSyncData          = 0x00001000,     // Synchronization data, such as Mutex, Semaphore, etc.
                kTFPPUAddressRep        = 0x00002000,     // Convert callstack addresses to strings (file/line, function/offset).
                kTFPPUCrashDasm         = 0x00004000,     // Disassembly around the crash instruction.
                kTFPPUCrashSource       = 0x00008000,     // Source code around the crash instruction (user needs to provide a source directory with AddSourceCodeDirectory, and needs to provide symbols via AddDatabaseFile with an ELF and not a MAP file).

                // SPU
                kTFSPUData              = 0x00100000,     // 
                kTFSPUCallstackData     = 0x00200000,     // 
                kTFSPURegisterData      = 0x00400000,     // 
                kTFSPUAddressRep        = 0x00800000,     // Convert callstack addresses to strings (file/line, function/offset).
                kTFSPUCrashDasm         = 0x01000000,     // Disassembly around the crash instruction.
                kTFSPUCrashSource       = 0x02000000,     // Source code around the crash instruction (user needs to provide a source directory with AddSourceCodeDirectory, and needs to provide symbols via AddDatabaseFile with an ELF and not a MAP file).

                // RSX
                kTFRSXData              = 0x10000000,     // GPU data (RSX is the name of the PS3 GPU).

                // Aggregates
                kTFDefault              = 0xffffffff,
                kTFAll                  = 0xffffffff
            };

            enum CrashProcessor
            {
                kCPNone     = 0x00,     // 
                kCPPPU      = 0x01,     // PPU (main) processor crash
                kCPSPU      = 0x02,     // SPU processor crash
                kCPRSX      = 0x04      // Video processor crash
            };

            // Write a text report on the core dump. This is the main user-level function.
            // This function may AddRef/Release the IStream argument, though any such AddRefs will
            // be matched by Releases.
            void  Trace(EA::IO::IStream* pOutput, int traceStrategy = kTSAutoDetect, int traceFilterFlags = 0);

            void  Trace(eastl::string& sOutput, int traceStrategy = kTSAutoDetect, int traceFilterFlags = 0);


            ///////////////////////////////////////
            // The rest of the functions below are typically not needed unless you are implementing your own alternative to Trace.
            ///////////////////////////////////////

            CrashProcessor             GetCrashProcessor() const;
            uint64_t                   GetCrashedPPUThreadID() const;
            uint32_t                   GetCrashedSPUThreadID() const;
            PS3DumpELF::SPUThreadData* GetSPUThreadData(uint32_t spuThreadID);

            const char8_t* GetPPUThreadName(uint64_t ppuThreadID) const;
            const char8_t* GetSPUThreadName(uint32_t spuThreadID) const;

            size_t GetPPUCallstack(uint64_t pReturnAddressArray[], size_t nReturnAddressArrayCapacity, uint64_t ppuThreadID) const;
            size_t GetSPUCallstack(uint32_t pReturnAddressArray[], size_t nReturnAddressArrayCapacity, uint32_t spuThreadID) const;

            bool GetUnknownAddressDescription(uint64_t address, FixedString& sUnknownAddressDescription);

            // If the given ppuThreadID appears to be in a deadlock with one or more other threads,
            // this function writes the involved thread ids into threadDeadlockChain and returns the
            // count of IDs written to threadDeadlockChain. Return value is 0 if there is no deadlock.
            uint32_t GetPPUThreadDeadlock(uint64_t ppuThreadID, PS3DumpELF::SyncInfo* pSyncInfoChainResult, size_t syncInfoChainCapacity) const;

            // Fills in array of synchronization locks that this thread holds (0 or more) and is waiting for (usually 0 or 1). 
            void GetPPUThreadSyncInfo(uint64_t ppuThreadID, PS3DumpELF::SyncInfoArray* pLockArray, PS3DumpELF::SyncInfoArray* pWaitArray) const;

            // Gets a list of threads waiting on the given Sync.
            uint32_t GetWaitingThreads(const PS3DumpELF::SyncInfo& syncInfo, Uint64Array& threadArray) const;

            // Gets the SPUThreadGroupData for an SPU thread.
            const PS3DumpELF::SPUThreadGroupData& GetSPUThreadGroupData(uint32_t spuThreadGroupID) const;

            // The core dump stores memory attribute data separately from the memory data itself.
            const PS3DumpELF::MemoryPageAttributeData& GetMemoryPageAttributeData(uint64_t startingAddress) const;

        protected:
            typedef eastl::vector<PS3DumpELF::PPUThreadRegisterData>    PPUThreadRegisterDataArray;
            typedef eastl::vector<PS3DumpELF::PPUThreadData>            PPUThreadDataArray;

            typedef eastl::vector<PS3DumpELF::SPUThreadRegisterData>    SPUThreadRegisterDataArray;
            typedef eastl::vector<PS3DumpELF::SPUThreadData>            SPUThreadDataArray;
            typedef eastl::vector<PS3DumpELF::SPULocalStorageData>      SPULocalStorageDataArray;
            typedef eastl::vector<PS3DumpELF::SPUThreadGroupData>       SPUThreadGroupDataArray;

            typedef eastl::vector<PS3DumpELF::MutexData>                MutexDataArray;
            typedef eastl::vector<PS3DumpELF::ConditionVariableData>    ConditionVariableDataArray;
            typedef eastl::vector<PS3DumpELF::RWLockData>               RWLockDataArray;
            typedef eastl::vector<PS3DumpELF::LWMutexData>              LWMutexDataArray;
            typedef eastl::vector<PS3DumpELF::EventQueueData>           EventQueueDataArray;
            typedef eastl::vector<PS3DumpELF::SemaphoreData>            SemaphoreDataArray;
            typedef eastl::vector<PS3DumpELF::LWConditionVariableData>  LWConditionVariableDataArray;

            typedef eastl::vector<PS3DumpELF::PRXData>                  PRXDataArray;
            typedef eastl::vector<PS3DumpELF::MemoryPageAttributeData>  MemoryPageAttributeDataArray;
            typedef eastl::vector<PS3DumpELF::MemoryContainerData>      MemoryContainerDataArray;
            typedef eastl::vector<PS3DumpELF::EventFlagData>            EventFlagDataArray;

        protected:
            void TraceInternal(EA::IO::IStream* pOutput, int traceFilterFlags);

            bool Load_PT_NOTE(const ELF::ELF64_Phdr& ph);
            bool Load_PT_LOAD(const ELF::ELF64_Phdr& ph);

            bool LoadSystemData             (PS3DumpELF::CommonProgramSegmentHeader& cpsh);
            bool LoadProcessData            (PS3DumpELF::CommonProgramSegmentHeader& cpsh);
            bool LoadPPUThreadRegisterData  (PS3DumpELF::CommonProgramSegmentHeader& cpsh);
            bool LoadSPUThreadRegisterData  (PS3DumpELF::CommonProgramSegmentHeader& cpsh);
            bool LoadPPUThreadData          (PS3DumpELF::CommonProgramSegmentHeader& cpsh);
            bool LoadCoreDumpCause          (PS3DumpELF::CommonProgramSegmentHeader& cpsh);
            bool LoadSPULocalStorageData    (PS3DumpELF::CommonProgramSegmentHeader& cpsh);
            bool LoadSPUThreadGroupData     (PS3DumpELF::CommonProgramSegmentHeader& cpsh);
            bool LoadSPUThreadData          (PS3DumpELF::CommonProgramSegmentHeader& cpsh);
            bool LoadPRXData                (PS3DumpELF::CommonProgramSegmentHeader& cpsh);
            bool LoadMemoryPageAttributeData(PS3DumpELF::CommonProgramSegmentHeader& cpsh);
            bool LoadMutexData              (PS3DumpELF::CommonProgramSegmentHeader& cpsh);
            bool LoadConditionVariableData  (PS3DumpELF::CommonProgramSegmentHeader& cpsh);
            bool LoadRWLockData             (PS3DumpELF::CommonProgramSegmentHeader& cpsh);
            bool LoadLWMutexData            (PS3DumpELF::CommonProgramSegmentHeader& cpsh);
            bool LoadEventQueueData         (PS3DumpELF::CommonProgramSegmentHeader& cpsh);
            bool LoadSemaphoreData          (PS3DumpELF::CommonProgramSegmentHeader& cpsh);
            bool LoadLWConditionVariableData(PS3DumpELF::CommonProgramSegmentHeader& cpsh);
            bool LoadMemoryContainerData    (PS3DumpELF::CommonProgramSegmentHeader& cpsh);
            bool LoadEventFlagData          (PS3DumpELF::CommonProgramSegmentHeader& cpsh);
            bool LoadRSXDebugData           (PS3DumpELF::CommonProgramSegmentHeader& cpsh);
            bool LoadMemoryStatisticsData   (PS3DumpELF::CommonProgramSegmentHeader& cpsh);

            bool FindDeadlockRecursive(const PS3DumpELF::SyncInfo& syncInfoCurrent, const PS3DumpELF::SyncInfo& syncInfoEndpoint, 
                                        PS3DumpELF::SyncInfo* pSyncInfoChain, size_t syncInfoChainCapacity, size_t& syncInfoChainSize) const;
            bool ThreadIsWaitingOnSyncInfo(uint64_t ppuThreadID, const PS3DumpELF::SyncInfo& syncInfo) const;
            bool ThreadIsWaitingOnAnySyncInfo(uint64_t ppuThreadID, PS3DumpELF::SyncInfo& syncInfo) const;

            IAddressRepLookup* GetSPULookup(const char8_t* pELFFilePath8);

            static void CopySyncInfo(PS3DumpELF::SyncInfo& si, const PS3DumpELF::MutexData& d);
            static void CopySyncInfo(PS3DumpELF::SyncInfo& si, const PS3DumpELF::ConditionVariableData& d);
            static void CopySyncInfo(PS3DumpELF::SyncInfo& si, const PS3DumpELF::RWLockData& d);
            static void CopySyncInfo(PS3DumpELF::SyncInfo& si, const PS3DumpELF::LWMutexData& d);
            static void CopySyncInfo(PS3DumpELF::SyncInfo& si, const PS3DumpELF::EventQueueData& d);
            static void CopySyncInfo(PS3DumpELF::SyncInfo& si, const PS3DumpELF::SemaphoreData& d);
            static void CopySyncInfo(PS3DumpELF::SyncInfo& si, const PS3DumpELF::LWConditionVariableData& d);

            static const char* GetCoreDumpCauseString(uint32_t type);
            static const char* GetPPUExceptionTypeString(uint64_t type);
            static bool        IsPPUExceptionDataAccess(uint64_t type);
            static uint32_t    GetSPUExceptionTypeString(uint32_t typeFlags, char* pString);
            static const char* GetPPUThreadStateString(uint32_t state);
            static const char* GetSPUThreadStateString(uint32_t state);
            static const char* GetSyncTypeString(PS3DumpELF::SyncType type);
            static void        GetMemoryPageAttributeString(uint64_t attribute, uint64_t accessRight, FixedString8& s8);
            static void        GetInterruptErrorStatusString(uint32_t status, FixedString8& s8);
            static void        GetInterruptFIFOErrorStatusString(uint32_t status, FixedString8& s8);
            static void        GetInterruptIOIFErrorStatusString(uint32_t status, FixedString8& s8);
            static void        GetInterruptGraphicsErrorStatusString(uint64_t status, FixedString8& s8);
            static const char* GetRSXGraphicsErrorString(uint32_t error);
            static const char* GetTileRegionUnitCompModeString(uint32_t nCompressionMode);
            static const char* GetZCullRegionAntialiasString(uint32_t nAntialias);

        public:
            EA::Allocator::ICoreAllocator* mpCoreAllocator;         /// 
            FixedStringW                   msDumpFilePath;          /// 
            EA::IO::FileStream             mFileStream;             /// Disk-based source of file data. 
            EA::IO::IStream*               mpStream;                /// Points to either mFileStream or some other stream. We don't currently AddRef this mpStream member.
            ELFFile                        mELFFile;                /// 
            bool                           mbInitialized;           /// 
            bool                           mbLoaded;                /// 
            AddressRepLookupSet            mAddressRepLookupSetPPU; /// Usually there will be just one .map, .elf, or .self here.
            AddressRepLookupSet            mAddressRepLookupSetSPU; /// There could be one for each SPU here.

            // Records
            PS3DumpELF::SystemData              mSystemData;
            PS3DumpELF::ProcessData             mProcessData;

            PPUThreadRegisterDataArray          mPPUThreadRegisterDataArray;
            PPUThreadDataArray                  mPPUThreadDataArray;

            SPUThreadRegisterDataArray          mSPUThreadRegisterDataArray;
            SPUThreadDataArray                  mSPUThreadDataArray;
          //SPULocalStorageDataArray            mSPULocalStorageDataArray;  // We store this in mPS3SPUMemoryDump instead.
            SPUThreadGroupDataArray             mSPUThreadGroupDataArray;
            PS3DumpELF::SPUThreadGroupData      mSPUThreadGroupDataDefault; // Used in case there is some error.

            MutexDataArray                      mMutexDataArray;
            ConditionVariableDataArray          mConditionVariableDataArray;
            RWLockDataArray                     mRWLockDataArray;
            LWMutexDataArray                    mLWMutexDataArray;
            EventQueueDataArray                 mEventQueueDataArray;
            SemaphoreDataArray                  mSemaphoreDataArray;
            LWConditionVariableDataArray        mLWConditionVariableDataArray;

            PS3DumpELF::RSXDebugInfo1Data       mRSXDebugInfo1Data;
            PS3DumpELF::MemoryStatisticsData    mMemoryStatisticsData;
            PS3DumpELF::CoreDumpCauseUnion      mCoreDumpCauseUnion;
            PRXDataArray                        mPRXDataArray;
            MemoryPageAttributeDataArray        mMemoryPageAttributeDataArray;
            PS3DumpELF::MemoryPageAttributeData mMemoryPageAttributeDataDefault;    // Used in case there is some error.
            MemoryContainerDataArray            mMemoryContainerDataArray;
            EventFlagDataArray                  mEventFlagDataArray;

            // Memory dump
            PS3MemoryDump                       mPS3MemoryDump;
            PS3SPUMemoryDump                    mPS3SPUMemoryDump;          // Stores the memory images of all SPU threads.

        }; // class PS3Dump

    } // namespace Callstack

} // namespace EA


#ifdef _MSC_VER
    #pragma warning(pop)
#endif


#endif // Header include guard



