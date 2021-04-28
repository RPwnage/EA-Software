/*

JDep.cpp - Jason's dependency checker class implementation

*/

#include "StdAfx.h"
#include <shlwapi.h>
#include "JDep.h"
#include "JPrefilteredFile.h"
#include "JDefine.h"
#include "JExpression.h"
//#include "Timer.h"
//#include <StdIO.h>
#include <CType.h>
#include <Assert.h>
//#include <stdarg.h>

// for reading reference assembly
#using <mscorlib.dll>
#using <System.dll>

using namespace System;
using namespace System::Reflection;
using namespace System::Runtime::InteropServices;

static long SkipWhiteSpace(char *pBuf, long Pos);

//__int64 TotalTime = 0, IncTime = 0;

JDep::JDep() : bSkipDuplicates(true), bShowDuplicates(false), bShowSysIncludes(false), bSuppressWarnings(false),
bIgnoreMissingIncludes(false), Defines(1), bError(false), IncDepth(0)
{
    ExprCount = IfdefCount = IfCount = DefineCount = FileCount = 0;
}

JDep::~JDep()
{
}

void JDep::Reset(void)
{
    // Reset the internals
    bError = false;

    Defines.Purge();
    IncPaths.Purge();
    SysPaths.Purge();
    UsingPaths.Purge();
    Files.Purge();
}

void JDep::AddIncludePath(const char *pPath, bool bSysPath)
{
    ccNode *pNode = new ccNode;
    pNode->SetName(pPath);

    if(bSysPath)
        SysPaths.AddTail(pNode);
    else
        IncPaths.AddTail(pNode);
}

void JDep::AddUsingPath(const char *pPath)
{
    ccNode *pNode = new ccNode;
    pNode->SetName(pPath);

    UsingPaths.AddTail(pNode);
}

/*
ANSI standard defines should be included as well
ms-help://MS.VSCC/MS.MSDNVS/vclang/html/_predir_predefined_macros.htm
*/

// SIMPLE defines - No expressions allowed at the "pre-defined" level
void JDep::AddDefine(const char *pDef)
{
    JDefine *pNode = new JDefine;
    pNode->SetName(pDef);

    // Look for an =
    const char *pEq = strchr(pDef, '=');
    if(pEq)
    {
        int NameLen = (long)(pEq - pDef);
        ((char *)pNode->GetName())[NameLen] = 0;	// Forcibly truncate the name

        // Value
        const char *pName = pEq+1;
        int Value = 1;
        if(isdigit(pName[0]))
        {
            // Check to see if the character is prefixed with 0 (octal), 0x(hex), or contains a decimal
            if(pName[0] == '0' && (pName[1] == 'x' || pName[1] == 'X'))
                Value = AtoX(pName);
            else
                Value = AtoI(pName);
        }
        pNode->Value = Value;
        pNode->bIsValue = true;
        pNode->bEvaluated = true;
    }
    Defines.Add(pNode);
}

JDepFile *JDep::ParseCPPFile(const char *pFilename, JDepFile *pParent, bool bSysInc, JPrefilteredFile *pFilt)
{
    // OutputWarning("#include : {0}\n", pFilename);
    FileCount++;

    JDepFile *pDepFile;
    if(pParent)
        pDepFile = new JDepFile(pParent);
    else
        pDepFile = new JDepFile();

    pDepFile->SetName( pFilename );
    pDepFile->bSysInclude = bSysInc;

//    __int64 TotalTemp;

    bool bOpenResult = true;
    if(pFilt)
        pDepFile->Filt.Init(pFilt);
    else
    {
        JPrefilteredFile *pFilt = SearchCache.OpenFile(pFilename);
        if (!pFilt)
        {
            if(!bSuppressWarnings)
                OutputWarning("Could not open file {0}.  Aborting.\n", pFilename);
            bError = true;
            return NULL;
        }
        pDepFile->Filt.Init(pFilt);
    }

    if(bOpenResult == false)
    {
        if(!bSuppressWarnings)
        {
            OutputWarning("Could not open file.  Aborting.\n");
            OutputWarning("-- parsing {0}\n", pDepFile->GetName());
        }

        delete pDepFile;
        bError = true;
        return NULL;
    }

    if(pParent)
    {
        if(IncDepth > 512)
        {
            if(!bSuppressWarnings)
            {
                OutputWarning("Include recursion passed maximum depth.  Aborting.\n");
                OutputWarning("-- parsing {0}\n", pDepFile->GetName());
            }
            bError = true;
            delete pDepFile;
            return NULL;
        }
        pParent->Includes.AddTail(pDepFile);

        // Add a "master" include node (what we've seen already)
        ccHashNode *pIncNode = new ccHashNode;
        pIncNode->SetName(pFilename);
        pDepFile->pTopParent->MasterIncludes.AddTail(pIncNode);
    }
    else
    {
//        TimerStart(TotalTemp);
        IncDepth = 0;
        Files.AddTail(pDepFile);

        // Transfer all global defines into this file node as a starting point
        JDefine *pDef = GetFirstDefine();
        while(pDef)
        {
            JDefine *pNewDef = new JDefine;
            pNewDef->SetName( pDef->GetName() );
            pNewDef->bIsValue = pDef->bIsValue;
            pNewDef->Value = pDef->Value;
            pNewDef->bEvaluated = pDef->bEvaluated;

            pDepFile->Defines.Add(pNewDef);
            pDef = pDef->GetNext();
        }
    }

    ccNode *pToken;
    while( (pToken = pDepFile->Filt.GetNextToken()) && !bError )
        ProcessToken(pToken, pDepFile);

    if(bError && pParent == 0)
    {
        Files.RemNode(pDepFile);
        delete pDepFile;
        pDepFile = 0;
    }

    if(pParent == 0)
    {
//        TimerStop(TotalTemp);
//        TotalTime = TotalTime + TotalTemp;

        if(bSuppressWarnings == false)
        {
            /*
            double fTime;
            fTime = (double)TotalTime / 1500000.0;
            fprintf(stderr, "\n       DepGen %4.2fms (%d files)  (C:%d, S:%d)\n", fTime, FileCount,
            SearchCache.Count(), SearchCache.Stored());
            //*/

            if(bError)
                OutputWarning("Terminated with errors\n");

//            TotalTime = 0, IncTime = 0;
            FileCount = 0;
        }
    }
    return pDepFile;
}

JDepFile* JDep::ParseDotNetAssemblyFile(const char *pFilename, JDepFile *pParent, bool bSysInc)
{
    // OutputWarning("#include : {0}\n", pFilename);
    FileCount++;

    JDepFile *pDepFile;
    if(pParent)
        pDepFile = new JDepFile(pParent);
    else
        pDepFile = new JDepFile();

    pDepFile->SetName( pFilename );
    pDepFile->bSysInclude = bSysInc;

    if (pParent)
    {
        if(IncDepth > 512)
        {
            if(!bSuppressWarnings)
            {
                OutputWarning("Include recursion passed maximum depth.  Aborting.\n");
                OutputWarning("-- parsing {0}\n", pDepFile->GetName());
            }
            bError = true;
            delete pDepFile;
            return NULL;
        }
        pParent->Includes.AddTail(pDepFile);

        // Add a "master" include node (what we've seen already)
        ccHashNode *pIncNode = new ccHashNode;
        pIncNode->SetName(pFilename);
        pDepFile->pTopParent->MasterIncludes.AddTail(pIncNode);

        // TODO: add reference assembly

        Assembly^ asmFile = nullptr;
        
        try
        {
            System::String^ strFilename = gcnew System::String(pFilename);
            asmFile = Assembly::LoadFrom(strFilename);
        }
        catch(Exception^)
        {
        }

        if (asmFile != nullptr)
        {
            array<AssemblyName^>^ names = asmFile->GetReferencedAssemblies();
            char szUsingPath[1024];
            char szUsing[1024];

            for (int i = 0; i < names->Length; i++)
            {
                IntPtr p = Marshal::StringToHGlobalAnsi(names[i]->Name);
                char* szTemp = static_cast<char*>(p.ToPointer());
                strcpy(szUsing, szTemp);
                Marshal::FreeHGlobal(p);
                strcat(szUsing, ".dll");
                bool bIncludedAlready = false;
                bool fileFound = EvaluateUsingFileName(pDepFile, szUsing, true, false, szUsingPath);
                if (fileFound == true)
                {
                    // Check to see if the file has already been included
                    bIncludedAlready = pDepFile->pTopParent->MasterIncludes.Find(szUsingPath) != 0;

                    if(!bIncludedAlready)
                    {
                        IncDepth++;
                        if(ParseDotNetAssemblyFile(szUsingPath, pDepFile, bSysInc) == false)
                        {
                            bError = true;
                        }
                        IncDepth--;
                    }
                }
                else
                {
                    if(bIgnoreMissingIncludes)
                    {
                        if(!bSuppressWarnings)
                        {
                            OutputWarning("Using file not found (ignored) : {0}\n", szUsing);
                            OutputWarning("-- parsing {0}\n", pDepFile->GetName());
                        }
                    }
                    else
                    {
                        bError = true;
                    }

                    if(!bSuppressWarnings)
                    {
                        OutputWarning("Using file not found : {0}\n", szUsing);
                        OutputWarning("-- parsing {0}\n", pDepFile->GetName());
                    }
                }
            }
        }
    }
    else
    {
        // failed since there's no pParent
    }

    return pDepFile;
}


void JDep::ProcessToken(ccNode *pToken, JDepFile *pDepFile)
{
    const char *pTokName = pToken->GetName();
    bool bProcessedToken = false;

    //pDepFile->Filt.PrintCurrentLine();

    // This code won't work on a motorola processor - Assumes specific endian order
    switch( *(long *)pTokName )
    {
    case 'ifed'://defi
        if( *(short *)(pTokName+4) == 'en' && pTokName[6] == 0 )
        {
            if(pDepFile->Cond.InCondition() == false || pDepFile->Cond.GetState() == Cond_True)
            {
                ParseDefine(pDepFile);
                bProcessedToken = true;
            }
        }
        break;

    case 'ednu':// unde
        if( *(short *)(pTokName+4) == '\0f')
        {
            if(pDepFile->Cond.InCondition() == false || pDepFile->Cond.GetState() == Cond_True)
            {
                ParseUndef(pDepFile);
                bProcessedToken = true;
            }
        }
        break;

    case 'nisu':// using
        if( *(short *)(pTokName+4) == '\0g' )
        {
            // pDepFile->Filt.PrintCurrentLine();
            if(pDepFile->Cond.InCondition() == false || pDepFile->Cond.GetState() == Cond_True)
            {
                ParseUsing(pDepFile);
                bProcessedToken = true;
            }
            else
            {
                //printf("Skipped using\n");
                //pDepFile->Filt.PrintCurrentLine();
                //printf("-- parsing %s\n", pDepFile->GetName());
            }
        }
        break;

    case 'trop':// import
        if( *(short *)(pTokName+4) == '\0mi' )
        {
            // pDepFile->Filt.PrintCurrentLine();
            if(pDepFile->Cond.InCondition() == false || pDepFile->Cond.GetState() == Cond_True)
            {
                ParseImport(pDepFile);
                bProcessedToken = true;
            }
            else
            {
                //printf("Skipped import\n");
                //pDepFile->Filt.PrintCurrentLine();
                //printf("-- parsing %s\n", pDepFile->GetName());
            }
        }
        break;

    case 'lcni'://incl
        if( *(long *)(pTokName+4) == '\0edu' )
        {
            // pDepFile->Filt.PrintCurrentLine();
            if(pDepFile->Cond.InCondition() == false || pDepFile->Cond.GetState() == Cond_True)
            {
                ParseInclude(pDepFile);
                bProcessedToken = true;
            }
            else
            {
                //printf("Skipped include\n");
                //pDepFile->Filt.PrintCurrentLine();
                //printf("-- parsing %s\n", pDepFile->GetName());
            }
        }
        break;

    case 'edfi'://ifde
        if( *(short *)(pTokName+4) == '\0f' )
        {
            ParseIfdef(pDepFile);
            bProcessedToken = true;
        }
        break;

    case 'dnfi'://ifnd
        if( *(short *)(pTokName+4) == 'fe' && pTokName[6] == 0)
        {
            ParseIfndef(pDepFile);
            bProcessedToken = true;
        }
        break;

    case 'file'://elif
        if(pTokName[4] == 0)
        {
            assert(pDepFile->Cond.InCondition() == true);
            ParseElif(pDepFile);
            bProcessedToken = true;
        }
        break;

    case 'esle'://else
        if(pTokName[4] == 0)
        {
            ccNode *pNextTok = pToken->GetNext();
            if(pNextTok && strcmp("if", pNextTok->GetName()) == 0)	// else if?
            {
                pToken = pDepFile->Filt.GetNextToken();
                ParseElif(pDepFile);	// Uses the same code as "else if"
                bProcessedToken = true;
            }
            else	// just else
            {
                ParseElse(pDepFile);
                bProcessedToken = true;
            }
        }
        break;

    case 'idne'://endi
        if( *(short *)(pTokName+4) == '\0f' )
        {
            // Pop the last entry off the condition stack
            pDepFile->Cond.Pop();
            pDepFile->Filt.NextLine();	// Skip any remaining tokens
            bProcessedToken = true;
        }
        break;

    case 'orre'://erro
        if( *(short *)(pTokName+4) == '\0r')
        {
            if(pDepFile->Cond.InCondition() == false || pDepFile->Cond.GetState() == Cond_True)
            {
                if(!bSuppressWarnings)
                {
                    OutputWarning("Hit an #error directive during parse - continuing\n");
                    OutputWarning("-- parsing {0}\n", pDepFile->GetName());
                    //pDepFile->Filt.PrintCurrentLine(stderr);
                }
                bProcessedToken = true;
            }
        }
        break;

    default:
        if( *(short *)pTokName == 'fi' && pTokName[2] == 0 )
        {
            ParseIf(pDepFile);
            bProcessedToken = true;
        }
        break;
    }

    // Unrecognized token
    if(bProcessedToken == false)
        pDepFile->Filt.NextLine();
}

static long SkipWhiteSpace(char *pBuf, long Pos)
{
    while(isspace(pBuf[Pos]) && pBuf[Pos])
        Pos++;
    return Pos;
}

static void ForceBackSlashes(char *pBuf)
{
    while(*pBuf)
    {
        if(*pBuf == '/')
            *pBuf = '\\';
        pBuf++;
    }
}


void JDep::ParseUsing(JDepFile *pDepFile)
{
    bool bSysInc;
    bool bIsLocal;
    char szUsing[260];
    char szUsingPath[260];

    szUsing[0] = 0;

    if (ExtractFileName(pDepFile, bIsLocal, bSysInc, szUsing))
    {
        bool bIncludedAlready = false;
        bool fileFound = EvaluateUsingFileName(pDepFile, szUsing, bIsLocal, bSysInc, szUsingPath);
        if (fileFound == true)
        {
            // Check to see if the file has already been included
            bIncludedAlready = pDepFile->pTopParent->MasterIncludes.Find(szUsing) != 0;

            if(bIncludedAlready && bShowDuplicates && (bShowSysIncludes == true || pDepFile->bSysInclude == false) )
            {
                OutputWarning("Duplicated include : {0}\n", szUsingPath);
                OutputWarning("-- parsing {0}\n", pDepFile->GetName());
            }
        }
        else
        {
            if(bIgnoreMissingIncludes)
            {
                if(!bSuppressWarnings)
                {
                    OutputWarning("Using file not found (ignored) : {0}\n", szUsing);
                    OutputWarning("-- parsing {0}\n", pDepFile->GetName());
                }
                return;
            }

            if(!bSuppressWarnings)
            {
                OutputWarning("Using file not found : {0}\n", szUsing);
                OutputWarning("-- parsing {0}\n", pDepFile->GetName());
            }
            bError = true;
            return;
        }

        // Purge tokens
        ccNode *pTok = pDepFile->Filt.GetNextToken();
        if(pTok->GetName()[0])
        {
            if(!bSuppressWarnings)
            {
                OutputWarning("Syntax error on #using directive : {0}\n", szUsing);
                OutputWarning("-- parsing {0}\n", pDepFile->GetName());
            }
            bError = true;
            return;
        }

        if(bIncludedAlready && bSkipDuplicates)
        {
            return;
        }

        IncDepth++;
        if(ParseDotNetAssemblyFile(szUsingPath, pDepFile, bSysInc) == false)
        {
            bError = true;
        }
        IncDepth--;
    }
}

void JDep::ParseImport(JDepFile *pDepFile)
{
    // TODO: implement
}

// Parse an #include directive
void JDep::ParseInclude(JDepFile *pDepFile)
{
//    __int64 TempTimer;
//    TimerStart(TempTimer);

    bool bSysInc;
    bool bIsLocal;
    char szInclude[260];
    char szIncludePath[260];

    szInclude[0] = 0;

    if (ExtractFileName(pDepFile, bIsLocal, bSysInc, szInclude))
    {
        bool bIncludedAlready = false;
        JPrefilteredFile *pFilt = EvaluateIncludeFileName(pDepFile, szInclude, bIsLocal, bSysInc, szIncludePath);
        if (pFilt != NULL)
        {
            // Check to see if the file has already been included
            bIncludedAlready = pDepFile->pTopParent->MasterIncludes.Find(szIncludePath) != 0;

            if(bIncludedAlready && bShowDuplicates && (bShowSysIncludes == true || pDepFile->bSysInclude == false) )
            {
                OutputWarning("Duplicated include : {0}\n", szIncludePath);
                OutputWarning("-- parsing {0}\n", pDepFile->GetName());
            }
        }
        else
        {
            if(bIgnoreMissingIncludes)
            {
                if(!bSuppressWarnings)
                {
                    OutputWarning("Include file not found (ignored) : {0}\n", szInclude);
                    OutputWarning("-- parsing {0}\n", pDepFile->GetName());
                }
                return;
            }

            if(!bSuppressWarnings)
            {
                OutputWarning("Include file not found : {0}\n", szInclude);
                OutputWarning("-- parsing {0}\n", pDepFile->GetName());
            }
            bError = true;
            return;
        }

        // Purge tokens
        ccNode *pTok = pDepFile->Filt.GetNextToken();        
        if(pTok->GetName()[0])
        {
            // This is not an error. Wii system files contain code like this:
            // --------------------------------------------------------
            // typedef enum {
            //   #include "test.h"  NUM_TRACKING_NAMES
            // } TrackingNameEnum; 
            // --------------------------------------------------------
            // Print warning, but do not return with error 
            if(!bSuppressWarnings)
            {
                OutputWarning("Warning unexpected tokens following preprocessor directive - expected a newline : {0}\n", szInclude);
                OutputWarning("-- parsing {0}\n", pDepFile->GetName());
            }
        }

//        TimerStop(TempTimer);
//        IncTime = IncTime + TempTimer;

        if(bIncludedAlready && bSkipDuplicates)
        {
            return;
        }

        IncDepth++;
        if(ParseCPPFile(szIncludePath, pDepFile, bSysInc, pFilt) == false)
        {
            bError = true;
        }
        IncDepth--;
    }
}

bool JDep::ExtractFileName(JDepFile *pDepFile, bool& bIsLocal, bool& bSysInc, char* szInclude)
{
    ccNode *pIncNode = pDepFile->Filt.GetNextToken();
    char *pIncName = (char *)pIncNode->GetName();
    if(pIncName == 0)
    {
        if(!bSuppressWarnings)
        {
            OutputWarning("#include directive missing filename\n");
            OutputWarning("-- parsing {0}\n", pDepFile->GetName());
        }
        bError = true;
        return false;
    }

    szInclude[0] = 0;

    // Next char must be " or <
    if(pIncName[0] == '<')
    {
        // System + Paths include
        bIsLocal = false;
        bSysInc = true;

        int NameStart = SkipWhiteSpace(pIncName, 1);
        char *pTemp = pIncName+NameStart;
        int NameLen = 0;

        // Extract the filename
        while(pTemp[NameLen] && pTemp[NameLen] != '>')
            NameLen++;

        if(NameLen > 260)	// MAX_PATH
        {
            if(!bSuppressWarnings)
            {
                OutputWarning("#include path too long\n");
                OutputWarning("-- parsing {0}\n", pDepFile->GetName());
            }
            bError = true;
            return false;
        }

        strncpy(szInclude, pTemp, NameLen);
        szInclude[NameLen] = 0;
        return true;
    }
    else if(pIncName[0] == '\"')
    {
        // Local + System Paths include
        bIsLocal = true;
        bSysInc = false;

        int NameStart = SkipWhiteSpace(pIncName, 1);
        char *pTemp = pIncName+NameStart;
        int NameLen = 0;

        // Extract the filename
        while(pTemp[NameLen] && pTemp[NameLen] != '\"')
            NameLen++;

        if(NameLen > 260)	// MAX_PATH
        {
            if(!bSuppressWarnings)
            {
                OutputWarning("#include path too long\n");
                OutputWarning("-- parsing {0}\n", pDepFile->GetName());
            }
            bError = true;
            return false;
        }
        strncpy(szInclude, pTemp, NameLen);
        szInclude[NameLen] = 0;
        return true;
    }
    return false;
}

JPrefilteredFile* JDep::EvaluateIncludeFileName(JDepFile *pDepFile, const char* szInclude, bool bIsLocal, bool bSysInc, char* szIncludePath)
{
    JPrefilteredFile *pFilt = NULL;
    // Try to locate the named include file by searching paths in the correct order
    if(szInclude != NULL && szInclude[0])
    {
        // if path is absolute path, then just use it
        if (!PathIsRelative(szInclude))
        {
            strcpy(szIncludePath, szInclude);
            pFilt = SearchCache.OpenFile(szIncludePath);

            return pFilt;
        }
        else
        {
            if(bIsLocal)
            {
                // Search the path of the current file, up to the topmost parent if necessary
                JDepFile *pSearchNode = pDepFile;
                while(pSearchNode && !pFilt)
                {
                    MakeIncludePathFromFile(pSearchNode->GetName(), szInclude, szIncludePath);
                    pFilt = SearchCache.OpenFile(szIncludePath);

                    if(pFilt == 0)
                    {
                        pSearchNode = pSearchNode->pParent;
                    }
                }
            }

            // Search additional include directories
            if(pFilt == 0)
            {
                ccNode *pSearchNode = IncPaths.GetHead();
                while(pSearchNode && !pFilt)
                {
                    MakeIncludePathFromPath(pSearchNode->GetName(), szInclude, szIncludePath);
                    pFilt = SearchCache.OpenFile(szIncludePath);

                    if(pFilt == 0)
                    {
                        pSearchNode = pSearchNode->GetNext();
                    }
                }
            }


            // Now search any system directories
            if(pFilt == 0)
            {
                ccNode *pSearchNode = SysPaths.GetHead();
                while(pSearchNode && !pFilt)
                {
                    MakeIncludePathFromPath(pSearchNode->GetName(), szInclude, szIncludePath);
                    pFilt = SearchCache.OpenFile(szIncludePath);

                    if(pFilt == 0)
                    {
                        pSearchNode = pSearchNode->GetNext();
                    }
                }
            }
        }
    }

    return pFilt;
}

bool JDep::EvaluateUsingFileName(JDepFile *pDepFile, const char* szUsing, bool bIsLocal, bool bSysInc, char* szUsingPath)
{
    bool fileExist = false;
    // Try to locate the named include file by searching paths in the correct order
    if(szUsing != NULL && szUsing[0])
    {
        if(bIsLocal)
        {
            // Search the path of the current file, up to the topmost parent if necessary
            JDepFile *pSearchNode = pDepFile;
            while(pSearchNode && fileExist == false)
            {
                MakeIncludePathFromFile(pSearchNode->GetName(), szUsing, szUsingPath);
                fileExist = PathFileExists(szUsingPath) == TRUE;
                if(fileExist == false)
                {
                    pSearchNode = pSearchNode->pParent;
                }
            }
        }

        // Search user specified using directories
        if(fileExist == false)
        {
            ccNode *pSearchNode = UsingPaths.GetHead();
            while(pSearchNode && fileExist == false)
            {
                MakeIncludePathFromPath(pSearchNode->GetName(), szUsing, szUsingPath);
                fileExist = PathFileExists(szUsingPath) == TRUE;

                if(fileExist == false)
                {
                    pSearchNode = pSearchNode->GetNext();
                }
            }
        }
    }

    return fileExist;
}

void JDep::ParseDefine(JDepFile *pDepFile)
{
    DefineCount++;
    ccNode *pDefName = pDepFile->Filt.GetNextToken();

    JDefine *pDef = pDepFile->FindDefine( pDefName->GetName() );
    if(pDef != 0)
    {
        // Clear any previous value
        pDef->bIsValue = false;
        pDef->Tokens.Purge();
    }
    else
    {
        pDef = new JDefine;
        pDef->SetName( pDefName->GetName() );
        pDepFile->pTopParent->Defines.Add(pDef);
    }

    ccNode *pDefValue = pDepFile->Filt.GetNextToken();
    if(pDefValue->GetName()[0])
    {
        do {
            pDef->AddToken(pDefValue->GetName());
            pDefValue = pDefValue->GetNext();
        } while(pDefValue->GetName()[0]);

        pDef->AddToken("");	// Terminator
        pDef->bIsValue = true;
    }
    pDepFile->Filt.NextLine();
}

void JDep::ParseUndef(JDepFile *pDepFile)
{
    ccNode *pDefName = pDepFile->Filt.GetNextToken();

    JDefine *pDef = pDepFile->FindDefine(pDefName->GetName());
    if(pDef)
    {
        pDepFile->pTopParent->Defines.Remove(pDef);
        delete pDef;
    }
}


void JDep::ParseIfdef(JDepFile *pDepFile)
{
    IfdefCount++;
    ccNode *pSym = pDepFile->Filt.GetNextToken();
    if(pSym->GetName()[0] == 0)
    {
        if(!bSuppressWarnings)
        {
            OutputWarning("#ifdef parse error\n");
            OutputWarning("-- parsing {0}\n", pDepFile->GetName());
        }
        bError = true;
        return;
    }

    if(pDepFile->Cond.GetState() == Cond_True)
    {
        bool bDefined = IsDefined(pSym->GetName(), pDepFile);
        pDepFile->Cond.Push(bDefined ? Cond_True : Cond_False);
    }
    else
        pDepFile->Cond.Push(Cond_Finished);

    ccNode *pTok = pDepFile->Filt.GetNextToken();
    if(pTok->GetName()[0])
    {
        if(!bSuppressWarnings)
        {
            OutputWarning("Syntax error in #ifdef statement. Unexpected token {0}\n", pTok->GetName());
            OutputWarning("-- parsing {0}\n", pDepFile->GetName());
        }
        bError = true;
    }
}


void JDep::ParseIfndef(JDepFile *pDepFile)
{
    IfdefCount++;
    ccNode *pSym = pDepFile->Filt.GetNextToken();
    if(pSym->GetName()[0] == 0)
    {
        if(!bSuppressWarnings)
        {
            OutputWarning("#ifndef parse error\n");
            OutputWarning("-- parsing {0}\n", pDepFile->GetName());
        }
        bError = true;
        return;
    }

    if(pDepFile->Cond.GetState() == Cond_True)
    {
        bool bDefined = IsDefined(pSym->GetName(), pDepFile);
        pDepFile->Cond.Push(bDefined ? Cond_False : Cond_True);
    }
    else
        pDepFile->Cond.Push(Cond_Finished);

    ccNode *pTok = pDepFile->Filt.GetNextToken();
    if(pTok->GetName()[0])
    {
        if(!bSuppressWarnings)
        {
            OutputWarning("Syntax error in #ifndef statement. Unexpected token '{0}'\n", pTok->GetName());
            OutputWarning("-- parsing {0}\n", pDepFile->GetName());
        }
        bError = true;
    }
}


void JDep::ParseIf(JDepFile *pDepFile)
{
    IfCount++;
    ccNode *pToken = pDepFile->Filt.GetNextToken();
    if(pToken->GetName()[0] == 0)
    {
        if(!bSuppressWarnings)
        {
            OutputWarning("Syntax error in #if statement\n");
            OutputWarning("-- parsing {0}\n", pDepFile->GetName());
        }
        bError = true;
        return;
    }

    if(pDepFile->Cond.GetState() == Cond_True)
    {
        //pDepFile->Filt.PrintCurrentLine();

        int Result = EvaluateExpression(pToken, pDepFile);
        pDepFile->Cond.Push(Result != 0 ? Cond_True : Cond_False);
    }
    else
    {
        pDepFile->Cond.Push(Cond_Finished);
        pDepFile->Filt.NextLine();		// Purge tokens
    }
}

void JDep::ParseElse(JDepFile *pDepFile)
{
    if(pDepFile->Cond.GetState() != Cond_Finished)	// Are we skipping this condition state?
    {
        // Nope - Process it
        if(pDepFile->Cond.GetState() == Cond_True)
            pDepFile->Cond.SetConditionFinished();
        else
            pDepFile->Cond.SetConditionTrue();
    }
}

void JDep::ParseElif(JDepFile *pDepFile)
{
    IfCount++;
    if(pDepFile->Cond.GetState() == Cond_True)
        pDepFile->Cond.SetConditionFinished();
    else if(pDepFile->Cond.GetState() != Cond_Finished)
    {
        ccNode *pToken = pDepFile->Filt.GetNextToken();
        int Result;

        if(pToken->GetName()[0] == 0)
            Result = true;										// #elif (nothing) == true
        else
            Result = EvaluateExpression(pToken, pDepFile);		// Test the expression

        // Set the condition state if true - leave it alone if false
        if(Result != 0)
            pDepFile->Cond.SetConditionTrue();
    }
}


void JDep::MakeIncludePathFromFile(const char *pCurFilePath, const char *szInclude, char *pDest) const
{
    strcpy(pDest, pCurFilePath);
    int i = 0, Last = -1;
    while(pDest[i])
    {
        if(pDest[i] == '/' || pDest[i] == '\\')
            Last = i;
        i++;
    }

    char *pSlash;
    if(Last >= 0)
        pSlash = pDest + Last + 1;
    else
        pSlash = pDest;

    strcpy(pSlash, szInclude);
    pSlash = pDest;
    while(*pSlash)
    {
        if(*pSlash == '/')
            *pSlash = '\\';
        else
            *pSlash = tolower(*pSlash);

        pSlash++;
    }
}

void JDep::MakeIncludePathFromPath(const char *pCurPath, const char *szInclude, char *pDest) const
{
    strcpy(pDest, pCurPath);
    int i = (int)strlen(pDest);
    pDest[i++] = '\\';
    strcpy(pDest+i, szInclude);

    char *pSlash = pDest;
    while(*pSlash)
    {
        if(*pSlash == '/')
            *pSlash = '\\';
        else
            *pSlash = tolower(*pSlash);
        pSlash++;
    }
}

int JDep::EvaluateExpression(ccNode *pToken, JDepFile *pDepFile)
{
    JExpression Expr;

    ExprCount++;
    int Result = Expr.Evaluate(pToken, pDepFile);

    if(Expr.Error())
    {
        if(!bSuppressWarnings)
        {
            OutputWarning("Error parsing expression\n");
            OutputWarning("-- parsing {0}\n", pDepFile->GetName());
        }
        bError = true;
    }

    pDepFile->Filt.NextLine();		// Purge tokens
    return Result;
}

void JDep::OutputWarning(const char *pFmt)
{        
    System::Console::Error->Write(gcnew System::String(pFmt, 0, (int)strlen(pFmt), System::Text::Encoding::ASCII));
}
void JDep::OutputWarning(const char *pFmt, const char* arg1)
{
    System::Console::Error->Write(gcnew System::String(pFmt, 0, (int)strlen(pFmt), System::Text::Encoding::ASCII), 
                                  gcnew System::String(arg1, 0, (int)strlen(arg1), System::Text::Encoding::ASCII));
}

void JDep::OutputWarning(const char *pFmt, const char* arg1, const char* arg2)
{
    System::Console::Error->Write(gcnew System::String(pFmt, 0, (int)strlen(pFmt), System::Text::Encoding::ASCII), 
                                  gcnew System::String(arg1, 0, (int)strlen(arg1), System::Text::Encoding::ASCII), 
                                  gcnew System::String(arg2, 0, (int)strlen(arg2), System::Text::Encoding::ASCII));
}

void JDep::ReadCachedTokens(const char *pFilename)
{
    ccList ArgList;

    JDefine *pDNode;
    for (pDNode = Defines.GetHead(); pDNode; pDNode = pDNode->GetNext())
    {
        ccNode *pNewNode = new ccNode;
        char name[256];
        sprintf(name, "%s=%d", pDNode->GetName(), pDNode->Value);
        pNewNode->SetName(name);
        ArgList.AddTail(pNewNode);
    }

    ccNode *pNode;
    for (pNode = IncPaths.GetHead(); pNode; pNode = pNode->GetNext())
    {
        ccNode *pNewNode = new ccNode;
        pNewNode->SetName(pNode->GetName());
        ArgList.AddTail(pNewNode);
    }
    for (pNode = SysPaths.GetHead(); pNode; pNode = pNode->GetNext())
    {
        ccNode *pNewNode = new ccNode;
        pNewNode->SetName(pNode->GetName());
        ArgList.AddTail(pNewNode);
    }

    SearchCache.ReadCachedTokens(pFilename, ArgList);
}

void JDep::WriteCachedTokens(const char *pFilename)
{
    FILE *f = fopen(pFilename, "wb");
    SearchCache.WriteCachedTokens(f);
    fclose(f);
}

void JDep::WriteFileChecksums(FILE *f)
{
    SearchCache.WriteFileChecksums(f);
}
