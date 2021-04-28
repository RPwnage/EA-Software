/////////////////////////////////////////////////////////////////////////////
// TestCallstackRecorder.cpp
//
// Copyright (c) 2008, Electronic Arts Inc. All rights reserved.
// Created by Paul Pedriana
/////////////////////////////////////////////////////////////////////////////


#include <EACallstackTest/EACallstackTest.h>
#include <EACallstack/EACallstackRecorder.h>
#include <EAIO/EAStream.h>
#include <EAIO/PathString.h>
#include <EAStdC/EAString.h>
#if EASTDC_VERSION_N >= 10400
    #include <EAStdC/EAProcess.h>
#endif
#include <EATest/EATest.h>


///////////////////////////////////////////////////////////////////////////////
// TestCallstackRecorder
//
EA::Callstack::CallstackRecorder* gpCallstackRecorder;

struct TCRTest
{
    int mRefCount;

    TCRTest() : mRefCount(0){ }

    int AddRef();
    int Release();
};

int TCRTest::AddRef()
{
    RecordCurrentCallstack(*gpCallstackRecorder, "SomeClass::AddRef", (uintptr_t)this);

    return ++mRefCount;
}

int TCRTest::Release()
{
    RecordCurrentCallstack(*gpCallstackRecorder, "SomeClass::Release", (uintptr_t)this);

    if(mRefCount > 1)
        return --mRefCount;
    delete this;
    return 0;
}

void TCR1(TCRTest* p);
void TCR1(TCRTest* p)
{
    p->AddRef();
    p->Release();
}

void TCR2(TCRTest* p);
void TCR2(TCRTest* p)
{
    TCR1(p);
}

void TCR3(TCRTest* p);
void TCR3(TCRTest* p)
{
    TCR2(p);
}

int TestCallstackRecorder()
{
    using namespace EA::Callstack;

    EA::UnitTest::ReportVerbosity(1, "\nTestCallstackRecorder\n");

    int nErrorCount = 0;

    gpCallstackRecorder = new EA::Callstack::CallstackRecorder;

    TCRTest* const p = new TCRTest;
    p->AddRef();

    for(int i = 0; i < 2; i++)
    {
        TCR1(p);
        TCR2(p);
        TCR3(p);
    }

    p->Release();

    // Set up an AddressRepCache. Normally in a real application you
    // wouldn't save and restore these; you'd just create one for the 
    // application.
    AddressRepCache arc;
    AddressRepCache* const pARCSaved = SetAddressRepCache(&arc);

    // Set up the debug info settings.
    wchar_t dbPath[EA::IO::kMaxPathLength] = { 0 };

    if(gpApplicationPath)
        EA::StdC::Strlcpy(dbPath, gpApplicationPath, EA::IO::kMaxPathLength);
    else
        EA::StdC::GetCurrentProcessPath(dbPath);


    if(!dbPath[0])
    {
        // What to do?
    }

    // On Microsoft platforms we use the .pdb or .map file associated 
    // with the executable. On GCC-based platforms, the executable is 
    // an ELF file which has the debug information built into it.
    #if defined(_MSC_VER)
        EA::StdC::Strcpy(EA::IO::Path::GetFileExtension(dbPath), L".map");
        arc.AddDatabaseFile(dbPath);

        EA::StdC::Strcpy(EA::IO::Path::GetFileExtension(dbPath), L".pdb");
        arc.AddDatabaseFile(dbPath);
    #else
        arc.AddDatabaseFile(dbPath);
    #endif

    // Write out the results.
    gpCallstackRecorder->SetName("Callstack Test");

    // Need to use a debug output stream instead of a file stream.
    // Need a way to verify results, as the code here just prints them for inspecting.
    if(EA::UnitTest::GetVerbosity() >= 1)
    {
        EA::IO::StreamLog fs1;
        fs1.AddRef();
        EA::Callstack::TraceCallstackRecording(*gpCallstackRecorder, &fs1, true);

        EA::IO::StreamLog fs2;
        fs2.AddRef();
        EA::Callstack::TraceCallstackRecording(*gpCallstackRecorder, &fs2, false);
    }

    // Restore old AddressRepCache.
    SetAddressRepCache(pARCSaved);

    delete gpCallstackRecorder;

    return nErrorCount;
}




