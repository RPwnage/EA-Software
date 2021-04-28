#include "StdAfx.h"

#using <mscorlib.dll>
#using <System.dll>

#include <windows.h>

// (JDep defines a String class in the global namespace which conflicts with System::String)
//using namespace System; 
using namespace System::Collections::Specialized;
using namespace System::Runtime::InteropServices;

#include "JDep.h"


namespace EA {
namespace DependencyGenerator {

public ref class DependencyGenerator : public IDependencyGenerator {
public:
    DependencyGenerator() {
        pDep = new JDep();
        pDep->bSuppressWarnings = true;
		pDep->bIgnoreMissingIncludes = true;
    }

    //NOTICE(RASHIN):
    //This will automatically compile into the following:
    //void Dispose() { blah; GC.SuppressFinalize(this); }
    //It also will force the class the extend the IDisposable interface.
    ~DependencyGenerator() {
        delete pDep;
    }

    virtual void Reset() override {
        pDep->Reset();
    }

    virtual void AddIncludePath(System::String^ includePath) override {
        AddIncludePath(includePath, false);
    }

    virtual void AddIncludePath(System::String^ includePath, bool systemInclude) override {
        const char* str = (const char*) Marshal::StringToHGlobalAnsi(includePath).ToPointer();
        pDep->AddIncludePath(str, systemInclude);
        Marshal::FreeHGlobal(System::IntPtr((void*) str));
    }

    virtual void AddUsingPath(System::String^ usingPath) override {
        const char* str = (const char*) Marshal::StringToHGlobalAnsi(usingPath).ToPointer();
        pDep->AddUsingPath(str);
        Marshal::FreeHGlobal(System::IntPtr((void*) str));
    }

    virtual void AddDefine(System::String^ define) override {
        const char* str = (const char*) Marshal::StringToHGlobalAnsi(define).ToPointer();
        pDep->AddDefine(str);
        Marshal::FreeHGlobal(System::IntPtr((void*) str));
    }

	virtual StringCollection^ ParseFile(System::String^ path) override {
        const char* ansiPath = (const char*) Marshal::StringToHGlobalAnsi(path).ToPointer();

		// this pointer is deleted when the JDep instance is deleted in the destructor
        JDepFile * pMainFile = pDep->ParseCPPFile(ansiPath);
        Marshal::FreeHGlobal(System::IntPtr((void*) ansiPath));

		if (pMainFile == NULL || pDep->WasError()) {
            throw gcnew System::Exception("Could not parse file.");
        }

		// Recursively build the dependency list
        StringCollection^ dependentFiles = gcnew StringCollection();
		AddFileIncludes(pMainFile, dependentFiles);

        return dependentFiles;
    }

    void AddFileIncludes(JDepFile *pParentFile, StringCollection^ dependentFiles) {
        JDepFile* pChildFile = pParentFile->GetFirstInclude();
        while (pChildFile != NULL) {
            dependentFiles->Add(gcnew System::String(pChildFile->GetName()));

			if(pChildFile->GetFirstInclude())	// Recurse into the child files
				AddFileIncludes(pChildFile, dependentFiles);

            pChildFile = pChildFile->GetNext();
        }
    }


    // TODO: Add properties that should be exposed by nant to the user here.  Examples might include:
    // * SkipSystemIncludes (ignore #include <string.h> files)
    // * FastAndDangerous (what you were talking about Jason but give it a better name or is that SkipDuplicates?)

    property bool SkipDuplicates
    {
        virtual bool get() override
        {
            return pDep->bSkipDuplicates;
        }

        virtual void set(bool value) override
        {
            pDep->bSkipDuplicates = value;
        }
    }

    property bool ShowDuplicates
    {
        virtual bool get() override
        {
            return pDep->bShowDuplicates;
        }

        virtual void set(bool value) override
        {
            pDep->bShowDuplicates = value;
        }
    }

    property bool IgnoreMissingIncludes
    {
        virtual bool get() override
        {
            return pDep->bIgnoreMissingIncludes;
        }

        virtual void set(bool value) override
        {
            pDep->bIgnoreMissingIncludes = value;
        }
    }

    property bool SuppressWarnings
    {
        virtual bool get() override
        {
            return pDep->bSuppressWarnings;
        }

        virtual void set(bool value) override
        {
            pDep->bSuppressWarnings = value;
        }
    }

private:
    JDep* pDep;
};

}
}
