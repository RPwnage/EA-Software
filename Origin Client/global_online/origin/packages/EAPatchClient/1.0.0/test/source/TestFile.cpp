/////////////////////////////////////////////////////////////////////////////
// EAPatchClientTest/TestFile.cpp
//
// Copyright (c) Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////


#include <EAPatchClientTest/Test.h>
#include <EAPatchClient/Allocator.h>
#include <EAPatchClient/File.h>
#include <EAPatchClient/MemoryFileSystem.h>
#include <EAStdC/EARandom.h>
#include <EAStdC/EAString.h>
#include <EATest/EATest.h>
#include <stdlib.h>




enum Action
{
    kActionExists,
    kActionCreate,
    kActionDestroy,
    kActionCopy,
    kActionMove,
    kActionRename,
};

enum EntryType
{
    kDir,
    kFile
};

struct Instruction
{
    Action      mAction;
    EntryType   mEntryType;
    const char* mpFirstArg;
    const char* mpSecondArg;
    int64_t     mResult;
};

const Instruction gMFSInstructionArray[] = 
{
    { kActionExists, kDir,   "/",                   "", 1 },
    { kActionExists, kFile,  "/",                   "", 0 },
  //{ kActionExists, kDir,   "//",                  "", 0 }, // Temporarily disabled because the code is currently tricked by this case.
    { kActionExists, kFile,  "//",                  "", 0 },
    { kActionExists, kFile,  "/abc/",               "", 0 },
    { kActionExists, kFile,  "/abc/def",            "", 0 },
    { kActionExists, kFile,  "/abc",                "", 0 },
    { kActionExists, kDir,   "/abc",                "", 0 },

    { kActionCreate, kDir,   "/",                   "", 1 }, // Should already exist.
    { kActionCreate, kDir,   "/DirA/",              "", 1 },
    { kActionExists, kDir,   "/DirA/",              "", 1 },
  //{ kActionExists, kDir,   "/DirA//",             "", 0 }, // Temporarily disabled because the code is currently tricked by this case.
    { kActionCreate, kFile,  "/FileA",              "", 1 },
    { kActionExists, kFile,  "/FileA",              "", 1 },
    { kActionCreate, kFile,  "/FileB",              "", 1 },
    { kActionExists, kFile,  "/FileB",              "", 1 },
    { kActionCreate, kFile,  "/DirA/FileB",         "", 1 },
    { kActionExists, kFile,  "/DirA/FileB",         "", 1 },
    { kActionExists, kFile,  "/DirA/FileC",         "", 0 },
    { kActionCreate, kFile,  "/DirA/FileC",         "", 1 },
    { kActionExists, kFile,  "/DirA/FileC",         "", 1 },

    { kActionDestroy, kFile,  "/xxxx",              "", 0 },
    { kActionDestroy, kFile,  "/FileB",             "", 1 },
    { kActionExists,  kFile,  "/FileB",             "", 0 },
    { kActionCreate,  kFile,  "/FileB",             "", 1 },
    { kActionExists,  kFile,  "/FileB",             "", 1 },
    { kActionCreate,  kDir,   "/DirB/DirC/",        "", 1 }, // Exercize creating a dir path as opposed to a single leaf dir.
    { kActionCreate,  kFile,  "/DirB/DirC/FileD",   "", 1 },
    { kActionCreate,  kFile,  "/DirB/DirC/FileE",   "", 1 },
    { kActionCreate,  kFile,  "/DirB/FileE",        "", 1 },
    { kActionCreate,  kFile,  "/DirB/DirD/FileF",   "", 1 },
    { kActionCreate,  kFile,  "/DirB/DirD/FileG",   "", 1 },
    { kActionCreate,  kFile,  "/DirB/DirE/FileH",   "", 1 },
    { kActionExists,  kFile,  "/DirB/DirE/FileH",   "", 1 },
    { kActionCreate,  kFile,  "/DirB/DirE/FileI",   "", 1 },
    { kActionDestroy, kFile,  "/DirB/DirE/FileG",   "", 0 }, // Intentionally use wrong middle directory.
    { kActionDestroy, kFile,  "/DirB/DirD/FileG",   "", 1 },

    // At this point we have a fairly built up tree.

    { kActionCopy,    kFile,  "/DirB/DirD/FileF",  "/DirB/DirD/FileJ", 1 },
    { kActionExists,  kFile,  "/DirB/DirD/FileF",  "",                 1 },
    { kActionExists,  kFile,  "/DirB/DirD/FileJ",  "",                 1 },
    { kActionCopy,    kFile,  "/DirB/DirD/FileJ",  "/DirK/FileJ",      1 },
    { kActionExists,  kFile,  "/DirB/DirD/FileJ",  "",                 1 },
    { kActionExists,  kFile,  "/DirK/FileJ",       "",                 1 },
    { kActionMove,    kFile,  "/DirB/DirD/FileJ",  "/DirL/FileN",      1 }, // Move a file
    { kActionExists,  kFile,  "/DirB/DirD/FileN",  "",                 0 },
    { kActionExists,  kFile,  "/DirL/FileN",       "",                 1 },
    { kActionMove,    kDir,   "/DirB/DirD/",       "/DirB/DirM",       1 }, // Move a directory
    { kActionExists,  kDir,   "/DirB/DirD/",       "",                 0 },
    { kActionExists,  kDir,   "/DirB/DirM/",       "",                 1 },
    { kActionMove,    kDir,   "/DirB/DirM",        "/DirB/DirM",       1 }, // Move dir to self
    { kActionExists,  kDir,   "/DirB/DirM/",       "",                 1 },
    { kActionMove,    kFile,  "/DirL/FileN",       "/DirL/FileN",      1 }, // Move file to self
    { kActionExists,  kFile,  "/DirL/FileN",       "",                 1 },

    // Advanced recursive copy/overwrite testing.
    { kActionCopy,    kDir,   "/DirB/",  "/DirK/",                     1 }, // Overwrite a directory tree with another tree.
    { kActionCopy,    kDir,   "/DirK/",  "/DirP/",                     1 }, // Move a directory tree.
    { kActionMove,    kDir,   "/DirP/",  "/DirA/",                     1 }, // Move a directory tree over another tree.
    { kActionMove,    kDir,   "/DirA/",  "/DirB/",                     1 }, // Move that directory tree over the original tree.

    // Test moving a directory tree to a child, and to it's own parent.
    { kActionCreate,  kDir,   "/DirX/DirY/DirZ/",  "",                 1 },
    { kActionMove,    kDir,   "/DirX/DirY/",       "/DirX/DirY/DirZ/", 0 }, // Disallowed, same as with Microsoft Windows.
  //Disabled because we don't currently support it. But neither does Windows.
  //{ kActionMove,    kDir,   "/DirX/DirY/",       "/DirX/",           1 }, // Move DirY and it's children over DirX
  //{ kActionExists,  kDir,   "/DirX/DirZ/",       "",                 1 }, // The previous move should have created /DirX/DirZ/

    // Test copying a directory tree to a child, and to it's own parent.
    { kActionCreate,  kDir,   "/DirJ/DirK/DirL/",  "",                 1 },
    { kActionCopy,    kDir,   "/DirJ/DirK/",       "/DirJ/DirK/DirL/", 0 }, // Disallowed, same as with Microsoft Windows.
  //Disabled because we don't currently support it. But neither does Windows.
  //{ kActionMove,    kDir,   "/DirJ/DirK/",       "/DirJ/",           1 }, // Move DirK and it's children over DirJ
  //{ kActionExists,  kDir,   "/DirJ/DirL/",       "",                 1 }, // The previous copy should have created /DirJ/DirL/
  //{ kActionExists,  kDir,   "/DirJ/DirK/",       "",                 1 }, // And the original /DirJ/DirK/ should still exist.

    // Last test.
    { kActionDestroy, kDir,   "/", "", 0 }, // Not allowed to destroy the root.
    { kActionExists,  kDir,   "/", "", 1 }
};



///////////////////////////////////////////////////////////////////////////////
// TestMFS
//
int TestMFS()
{
    using namespace EA::MFS;

    int nErrorCount(0);

    SetAllocator(EA::Patch::GetAllocator());

    MemoryFileSystem mfs;
    String8          sOutput;

    bool initResult = mfs.Init();
    EATEST_VERIFY(initResult);
    SetDefaultFileSystem(&mfs);

    // MemoryFileSystem
    {
        bool bResult;
        bool bPrintTree = false;

        for(size_t i = 0; i < EAArrayCount(gMFSInstructionArray); i++)
        {
            const Instruction& instruction = gMFSInstructionArray[i];

            switch (instruction.mAction)
            {
                case kActionExists:
                {
                    if(instruction.mEntryType == kDir)
                        bResult = mfs.DirectoryExists(instruction.mpFirstArg);
                    else
                        bResult = mfs.FileExists(instruction.mpFirstArg);
                    EATEST_VERIFY_F(bResult == (instruction.mResult != 0), "%zu) %s(%s)", i, instruction.mEntryType == kDir ? "DirectoryExists" : "FileExists", instruction.mpFirstArg);
                    break;
                }

                case kActionCreate:
                {
                    if(instruction.mEntryType == kDir)
                        bResult = mfs.DirectoryCreate(instruction.mpFirstArg);
                    else
                        bResult = mfs.FileCreate(instruction.mpFirstArg);
                    EATEST_VERIFY_F(bResult == (instruction.mResult != 0), "%zu) %s(%s)", i, instruction.mEntryType == kDir ? "DirectoryCreate" : "FileCreate", instruction.mpFirstArg);
                    break;
                }

                case kActionDestroy:
                {
                    if(instruction.mEntryType == kDir)
                        bResult = mfs.DirectoryDestroy(instruction.mpFirstArg);
                    else
                        bResult = mfs.FileDestroy(instruction.mpFirstArg);
                    EATEST_VERIFY_F(bResult == (instruction.mResult != 0), "%zu) %s(%s)", i, instruction.mEntryType == kDir ? "DirectoryDestroy" : "FileDestroy", instruction.mpFirstArg);
                    break;
                }

                case kActionCopy:
                {
                    if(instruction.mEntryType == kDir)
                        bResult = mfs.DirectoryCopy(instruction.mpFirstArg, instruction.mpSecondArg);
                    else
                        bResult = mfs.FileCopy(instruction.mpFirstArg, instruction.mpSecondArg);
                    EATEST_VERIFY_F(bResult == (instruction.mResult != 0), "%zu) %s(%s, %s)", i, instruction.mEntryType == kDir ? "DirectoryCopy" : "FileCopy", instruction.mpFirstArg, instruction.mpSecondArg);
                    break;
                }

                case kActionMove:
                {
                    if(instruction.mEntryType == kDir)
                        bResult = mfs.DirectoryMove(instruction.mpFirstArg, instruction.mpSecondArg);
                    else
                        bResult = mfs.FileMove(instruction.mpFirstArg, instruction.mpSecondArg);
                    EATEST_VERIFY_F(bResult == (instruction.mResult != 0), "%zu) %s(%s, %s)", i, instruction.mEntryType == kDir ? "DirectoryMove" : "FileMove", instruction.mpFirstArg, instruction.mpSecondArg);
                    break;
                }

                case kActionRename:
                {
                    if(instruction.mEntryType == kDir)
                        bResult = mfs.DirectoryRename(instruction.mpFirstArg, instruction.mpSecondArg);
                    else
                        bResult = mfs.FileRename(instruction.mpFirstArg, instruction.mpSecondArg);
                    EATEST_VERIFY_F(bResult == (instruction.mResult != 0), "%zu) %s(%s, %s)", i, instruction.mEntryType == kDir ? "DirectoryRename" : "FileRename", instruction.mpFirstArg, instruction.mpSecondArg);
                    break;
                }
            }

            if(bPrintTree) // Debugging.
            {
                bPrintTree = false;
                mfs.PrintTree(sOutput);
                EA::UnitTest::Report("%s", sOutput.c_str());
            }
        }

        if(bPrintTree) // Debugging.
        {
            bPrintTree = false;
            mfs.PrintTree(sOutput);
            EA::UnitTest::Report("%s", sOutput.c_str());
        }

        EATEST_VERIFY_F(mfs.Validate(sOutput), "%s", sOutput.c_str());
    }


    {
        // MemoryFileSystemIteration
        // DirectoryIterator
        mfs.Reset();

        mfs.FileCreate     ("/Dir5/File0");          // "Dir5" means there are 6 child entries.
        mfs.FileCreate     ("/Dir5/File1");
        mfs.DirectoryCreate("/Dir5/Dir0/");          // 0 child entries
        mfs.FileCreate     ("/Dir5/Dir1/File0");     // 1 child entry
        mfs.FileCreate     ("/Dir5/Dir2/File0");     // 2 child entries
        mfs.FileCreate     ("/Dir5/Dir2/File1");     

        EATEST_VERIFY_F(mfs.Validate(sOutput), "%s", sOutput.c_str());

        MemoryFileSystemIteration    mfsI;
        DirectoryIterator            di;
        DirectoryIterator::EntryList entryList;
        bool                         bResult;
        size_t                       count;

        {
            bResult = mfsI.IterateBegin("/Nonexistant", "*");
            EATEST_VERIFY(!bResult);
        }

        {
            bResult = mfsI.IterateBegin("/Dir5", "*");
            EATEST_VERIFY(bResult);
            if(bResult)
            {
                EATEST_VERIFY(EA::StdC::Strcmp(mfsI.GetEntryName(), "File0") == 0);
                EATEST_VERIFY(mfsI.GetEntryType() == ENTRY_TYPE_FILE);
                EATEST_VERIFY(mfsI.IterateNext());
                EATEST_VERIFY(EA::StdC::Strcmp(mfsI.GetEntryName(), "File1") == 0);
                EATEST_VERIFY(mfsI.GetEntryType() == ENTRY_TYPE_FILE);
                EATEST_VERIFY(mfsI.IterateNext());
                EATEST_VERIFY(EA::StdC::Strcmp(mfsI.GetEntryName(), "Dir0") == 0);
                EATEST_VERIFY(mfsI.GetEntryType() == ENTRY_TYPE_DIRECTORY);
                EATEST_VERIFY(mfsI.IterateNext());
                EATEST_VERIFY(EA::StdC::Strcmp(mfsI.GetEntryName(), "Dir1") == 0);
                EATEST_VERIFY(mfsI.GetEntryType() == ENTRY_TYPE_DIRECTORY);
                EATEST_VERIFY(mfsI.IterateNext());
                EATEST_VERIFY(EA::StdC::Strcmp(mfsI.GetEntryName(), "Dir2") == 0);
                EATEST_VERIFY(mfsI.GetEntryType() == ENTRY_TYPE_DIRECTORY);
                EATEST_VERIFY(!mfsI.IterateNext());
                mfsI.IterateEnd();
            }
        }
        {
            bResult = mfsI.IterateBegin("/", "*");
            EATEST_VERIFY(bResult);
            if(bResult)
            {
                EATEST_VERIFY(EA::StdC::Strcmp(mfsI.GetEntryName(), "Dir5") == 0);
                EATEST_VERIFY(mfsI.GetEntryType() == ENTRY_TYPE_DIRECTORY);
                EATEST_VERIFY(!mfsI.IterateNext());
                mfsI.IterateEnd();
            }
        }

        {
            bResult = mfsI.IterateBegin("/Dir5/Dir0", "*");
            EATEST_VERIFY(!bResult);
        }

        {
            bResult = mfsI.IterateBegin("/Dir5/Dir1/", "*");
            EATEST_VERIFY(bResult);
            if(bResult)
            {
                EATEST_VERIFY(EA::StdC::Strcmp(mfsI.GetEntryName(), "File0") == 0);
                EATEST_VERIFY(mfsI.GetEntryType() == ENTRY_TYPE_FILE);
                EATEST_VERIFY(!mfsI.IterateNext());
                mfsI.IterateEnd();
            }
        }

        {
            bResult = mfsI.IterateBegin("/Dir5/Dir2", "*");
            EATEST_VERIFY(bResult);
            if(bResult)
            {
                EATEST_VERIFY(EA::StdC::Strcmp(mfsI.GetEntryName(), "File0") == 0);
                EATEST_VERIFY(mfsI.GetEntryType() == ENTRY_TYPE_FILE);
                EATEST_VERIFY(mfsI.IterateNext());
                EATEST_VERIFY(EA::StdC::Strcmp(mfsI.GetEntryName(), "File1") == 0);
                EATEST_VERIFY(mfsI.GetEntryType() == ENTRY_TYPE_FILE);
                EATEST_VERIFY(!mfsI.IterateNext());
                mfsI.IterateEnd();
            }
        }

        {
            count = di.ReadRecursive("/", entryList, NULL, ENTRY_TYPE_FILE | ENTRY_TYPE_DIRECTORY, true, true);
            EATEST_VERIFY(count == 9);

            for(DirectoryIterator::EntryList::const_iterator itE = entryList.begin(); itE != entryList.end(); ++itE)
            {
                const DirectoryIterator::Entry& entry = *itE;
                EA::UnitTest::Report("%s\n", entry.msName.c_str());
            }

            count = di.ReadRecursive("/", entryList, NULL, ENTRY_TYPE_FILE, true, true);
            EATEST_VERIFY(count == 5);

            count = di.ReadRecursive("/", entryList, NULL, ENTRY_TYPE_DIRECTORY, true, true);
            EATEST_VERIFY(count == 4);
        }

        {
            count = di.ReadRecursive("/Dir5/Dir0/", entryList, NULL, ENTRY_TYPE_FILE | ENTRY_TYPE_DIRECTORY, true, true);
            EATEST_VERIFY(count == 0);
        }

        {
            count = di.ReadRecursive("/Dir5/Nonexistant/", entryList, NULL, ENTRY_TYPE_FILE | ENTRY_TYPE_DIRECTORY, true, true);
            EATEST_VERIFY(count == 0);
        }

        {
            // Reading from a file path should always result in nothing found.
            count = di.ReadRecursive("/Dir5/File0/", entryList, NULL, ENTRY_TYPE_FILE | ENTRY_TYPE_DIRECTORY, true, true);
            EATEST_VERIFY(count == 0);
        }

        {
            count = di.ReadRecursive("/Dir5/Dir2/", entryList, NULL, ENTRY_TYPE_FILE, true, true);
            EATEST_VERIFY(count == 2);

            count = di.ReadRecursive("/Dir5/Dir2/", entryList, NULL, ENTRY_TYPE_DIRECTORY, true, true);
            EATEST_VERIFY(count == 0);
        }

        EATEST_VERIFY_F(mfs.Validate(sOutput), "%s", sOutput.c_str());

        {
            // MemoryFileSystem::ValidatePath
            struct ValidatePathResults
            {
                const char8_t*     mpPath;
                EA::MFS::EntryType mEntryType;
                bool               mbValid;
            };

            const ValidatePathResults vprArray[] = 
            {
                { "",           ENTRY_TYPE_NONE,       false  },
                { "",           ENTRY_TYPE_FILE,       false  },
                { "",           ENTRY_TYPE_DIRECTORY,  false  },

                { "/",          ENTRY_TYPE_NONE,       true   },
                { "/",          ENTRY_TYPE_FILE,       false  },
                { "/",          ENTRY_TYPE_DIRECTORY,  true   },

                { "/a",         ENTRY_TYPE_NONE,       true   },
                { "/a",         ENTRY_TYPE_FILE,       true   },
                { "/a",         ENTRY_TYPE_DIRECTORY,  false  },

                { "//",         ENTRY_TYPE_NONE,       false  },
                { "//",         ENTRY_TYPE_FILE,       false  },
                { "//",         ENTRY_TYPE_DIRECTORY,  false  },

                { "/abc//",     ENTRY_TYPE_NONE,       false  },
                { "/abc//",     ENTRY_TYPE_FILE,       false  },
                { "/abc//",     ENTRY_TYPE_DIRECTORY,  false  },

                { "//abc/",     ENTRY_TYPE_NONE,       false  },
                { "//abc/",     ENTRY_TYPE_FILE,       false  },
                { "//abc/",     ENTRY_TYPE_DIRECTORY,  false  },

                { "//abc//",    ENTRY_TYPE_NONE,       false  },
                { "//abc//",    ENTRY_TYPE_NONE,       false  },
                { "//abc//",    ENTRY_TYPE_DIRECTORY,  false  },

                { "/abc/def/",  ENTRY_TYPE_NONE,       true   },
                { "/abc/def/",  ENTRY_TYPE_FILE,       false  },
                { "/abc/def/",  ENTRY_TYPE_DIRECTORY,  true   },

                { "/abc/def",   ENTRY_TYPE_NONE,       true   },
                { "/abc/def",   ENTRY_TYPE_FILE,       true   },
                { "/abc/def",   ENTRY_TYPE_DIRECTORY,  false  }
            };

            for(size_t i = 0; i < EAArrayCount(vprArray); i++)
            {
                bResult = mfs.ValidatePath(vprArray[i].mpPath, vprArray[i].mEntryType);

                EATEST_VERIFY_F(bResult == vprArray[i].mbValid, "MemoryFileSystem::ValidatePath failure: %zu: %s mistakenly found to be %s.", i, vprArray[i].mpPath, bResult ? "valid" : "invalid");
            }
        }

        EATEST_VERIFY_F(mfs.Validate(sOutput), "%s", sOutput.c_str());
    }

    {
        // MemoryFile
        mfs.Reset();

        uint64_t size;
        bool     bResult;

        {
            // Creation disposition
            MemoryFile mf0;
            MemoryFile mf1;
            MemoryFile mf2;
            MemoryFile mf3;

            // DISPOSITION_CREATE_NEW           // Fails if file already exists.
            mfs.FileCreate("/DirA/TestA.txt");  // Test opening a file that already exists.
            mf0.SetPath("/DirA/TestA.txt");
            bResult = mf0.Open(OPEN_WRITE, DISPOSITION_CREATE_NEW);
            EATEST_VERIFY(!bResult);

            mf0.SetPath("/DirA/TestB.txt");     // Test opening a file that doesn't already exist.
            bResult = mf0.Open(OPEN_WRITE, DISPOSITION_CREATE_NEW);
            EATEST_VERIFY(bResult);
            bResult = mf0.SetSize(1024);
            EATEST_VERIFY(bResult);
            size = mf0.GetSize();
            EATEST_VERIFY(size == 1024);
            mf0.Close();
            size = mfs.FileGetSize("/DirA/TestB.txt");
            EATEST_VERIFY(size == 1024);

            // DISPOSITION_CREATE_ALWAYS        // Never fails, always opens or creates and truncates to 0.
            mf0.SetPath("/DirA/TestB.txt");     // Test opening an existing file of non-zero size.
            bResult = mf0.Open(OPEN_WRITE, DISPOSITION_CREATE_ALWAYS);
            EATEST_VERIFY(bResult);
            size = mf0.GetSize();
            EATEST_VERIFY(size == 0);
            bResult = mf0.SetSize(2048);
            EATEST_VERIFY(bResult);
            mf0.Close();
            size = mfs.FileGetSize("/DirA/TestB.txt");
            EATEST_VERIFY(size == 2048);

            mf0.SetPath("/DirA/TestC.txt");     // Test opening a non-existing file.
            bResult = mf0.Open(OPEN_WRITE, DISPOSITION_CREATE_ALWAYS);
            EATEST_VERIFY(bResult);
            size = mf0.GetSize();
            EATEST_VERIFY(size == 0);
            mf0.Close();

            // DISPOSITION_OPEN_EXISTING        // Fails if file doesn't exist, keeps contents.
            mf0.SetPath("/DirA/TestB.txt");     // Test opening an existing file of non-zero size.
            bResult = mf0.Open(OPEN_WRITE, DISPOSITION_OPEN_EXISTING);
            EATEST_VERIFY(bResult);
            size = mf0.GetSize();
            EATEST_VERIFY(size == 2048);
            mf0.Close();

            mf0.SetPath("/DirA/TestD.txt");     // Test opening a non-existing file.
            bResult = mf0.Open(OPEN_WRITE, DISPOSITION_OPEN_EXISTING);
            EATEST_VERIFY(!bResult);

            // DISPOSITION_OPEN_ALWAYS          // Never fails, creates if doesn't exist, keeps contents.
            mf0.SetPath("/DirA/TestD.txt");     // Test opening a non-existing file.
            bResult = mf0.Open(OPEN_WRITE, DISPOSITION_OPEN_ALWAYS);
            EATEST_VERIFY(bResult);
            mf0.Close();

            mf0.SetPath("/DirA/TestB.txt");     // Test opening a non-existing file.
            bResult = mf0.Open(OPEN_WRITE, DISPOSITION_OPEN_ALWAYS);
            EATEST_VERIFY(bResult);
            size = mf0.GetSize();
            EATEST_VERIFY(size == 2048);
            mf0.Close();

            // DISPOSITION_TRUNCATE_EXISTING    // Fails if file doesn't exist, but truncates to 0 if it does.
            mf0.SetPath("/DirA/TestD.txt");     // Test opening an existing file of non-zero size.
            bResult = mf0.Open(OPEN_WRITE, DISPOSITION_TRUNCATE_EXISTING);
            EATEST_VERIFY(bResult);
            size = mf0.GetSize();
            EATEST_VERIFY(size == 0);
            mf0.Close();

            mf0.SetPath("/DirA/TestE.txt");     // Test opening a non-existing file.
            bResult = mf0.Open(OPEN_WRITE, DISPOSITION_TRUNCATE_EXISTING);
            EATEST_VERIFY(!bResult);

            // DISPOSITION_DEFAULT              // Default disposition, depending on file open flags.
            // To consider: come up with a test for this.
        }

        EATEST_VERIFY_F(mfs.Validate(sOutput), "%s", sOutput.c_str());

        {
            // Open file access rights.
            MemoryFile mf0("/DirA/FileA.txt");
            MemoryFile mf1("/DirA/FileA.txt");
            MemoryFile mf2("/DirA/FileA.txt");
            MemoryFile mf3("/DirA/FileA.txt");

            // Verify that you can't open a file for write if others have it open for write access.
            bResult = mf0.Open(OPEN_WRITE, DISPOSITION_OPEN_ALWAYS);
            EATEST_VERIFY(bResult);
            bResult = mf1.Open(OPEN_WRITE, DISPOSITION_OPEN_ALWAYS);
            EATEST_VERIFY(!bResult);
            bResult = mf1.Open(OPEN_WRITE, DISPOSITION_OPEN_EXISTING);
            EATEST_VERIFY(!bResult);
            mf0.Close();

            // Verify that you can't open a file for write if others have it open for read access.
            bResult = mf0.Open(OPEN_READ, DISPOSITION_OPEN_EXISTING);
            EATEST_VERIFY(bResult);
            bResult = mf1.Open(OPEN_READ, DISPOSITION_OPEN_EXISTING);
            EATEST_VERIFY(bResult);
            bResult = mf2.Open(OPEN_READ, DISPOSITION_OPEN_EXISTING);
            EATEST_VERIFY(bResult);
            bResult = mf3.Open(OPEN_WRITE, DISPOSITION_OPEN_EXISTING);
            EATEST_VERIFY(!bResult);
            mf1.Close();
            bResult = mf3.Open(OPEN_WRITE, DISPOSITION_OPEN_EXISTING);
            EATEST_VERIFY(!bResult);
            mf0.Close();
            mf2.Close();
            bResult = mf3.Open(OPEN_WRITE, DISPOSITION_OPEN_EXISTING);
            EATEST_VERIFY(bResult);
            mf3.Close();

            // Verify that you can't delete a file if it is open for any access.
            // Verify that you can copy a file if it can be opened for read access.
            bResult = mf0.Open(OPEN_READ, DISPOSITION_OPEN_EXISTING);
            EATEST_VERIFY(bResult);
            bResult = mfs.FileDestroy("/DirA/FileA.txt");
            EATEST_VERIFY(!bResult);
            bResult = mfs.FileMove("/DirA/FileA.txt", "/DirA/FileB.txt");
            EATEST_VERIFY(!bResult);
            bResult = mfs.FileRename("/DirA/FileA.txt", "/DirA/FileC.txt");
            EATEST_VERIFY(!bResult);
            bResult = mfs.FileCopy("/DirA/FileA.txt", "/DirA/FileB.txt");
            EATEST_VERIFY(bResult);
            mf0.Close();

            // Verify that you can't delete a directory if any child of it recursively is open for any access.
            // Verify that you can copy a directory if any child of it recursively is open for any access.
            mf0.SetPath("/DirA/DirB/DirC/FileD.txt");
            bResult = mf0.Open(OPEN_WRITE, DISPOSITION_OPEN_ALWAYS);
            EATEST_VERIFY(bResult);
            bResult = mfs.DirectoryDestroy("/DirA/");
            EATEST_VERIFY(!bResult);
            bResult = mfs.DirectoryMove("/DirA/", "/DirB/");
            EATEST_VERIFY(!bResult);
            bResult = mfs.DirectoryRename("/DirA/", "/DirB/");
            EATEST_VERIFY(!bResult);
            bResult = mfs.DirectoryCopy("/DirA/", "/DirB/");
            EATEST_VERIFY(bResult);
            mf0.Close();
            bResult = mfs.DirectoryDestroy("/DirA/");   // Now that the file is closed, we should be able to delete one of its directory parents.
            EATEST_VERIFY(bResult);
        }

        EATEST_VERIFY_F(mfs.Validate(sOutput), "%s", sOutput.c_str());
    }

    mfs.Shutdown();

    return nErrorCount;
}



int TestFile()
{
    int nErrorCount(0);

    // Test the MemoryFileSystem, which is a file system implemented in RAM (like a RAM disk).
    nErrorCount += TestMFS();

    // To do: Test other File APIs from our File module.

    return nErrorCount;
}





