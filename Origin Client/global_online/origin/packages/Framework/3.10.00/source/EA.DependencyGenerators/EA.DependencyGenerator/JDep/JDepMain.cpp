// JDep.cpp : Defines the entry point for the console application.
//

#include "StdAfx.h"
#include <Windows.h>
#include "JDep.h"
#include "JDepFile.h"
#include "Tokenizer.h"
#include "Timer.h"

// http://msdn.microsoft.com/library/default.asp?url=/library/en-us/vclang/html/vcrefpreprocessorreference.asp

// Standard GCC preprocessor options : -w -nostdinc -MM -MG
// -MM = don't list <inc.h>
// -MG = assume unfound headers = generated files
// -w inhibit all warning messages (quiet)
// -D "value" = predefine "value"
// -I "include path" = specify an additional search path
// -S "include path" = specify a system include path (generally unused)
// -OP "something" = prepend “something” to all generated include paths when output
// -TokenCacheFile = Specify cached token file (big speedup).
// @"file.txt" = parse commands from file.txt

JDep Dep;
ccList FileList, IncList;
char szOutputPrefix[512] = "";
FILE *gOutf = stdout;

bool bHideSysIncludes = false;
char gNFSScriptName[256] = {0};			// Optional NFS Script filename. (From command line)
char gTokenCacheFilename[256] = {0};	// Optional Token cache filename. (From command line)

void FormatGNUPath(char *pDest, const char *pSrc);
void FormatWatcomPath(char *pDest, const char *pSrc);
void DumpIncludes(JDepFile *pDepFile, int Indent = 0);
bool ParseCommandLine(int argc, char *argv[]);
bool ParseCommandFile(const char *pFilename);


int main(int argc, char* argv[])
{
	// TEMP -------------------------------
	//Dep.bShowDuplicates = true;
	//Dep.bSkipDuplicates = false;
	// TEMP -------------------------------

	if(ParseCommandLine(argc, argv) == false)
		return -1;

	ccNode *pNode = FileList.GetHead();
	if(pNode == 0)
		printf("No files specified\n");

	// Read TokenCache
	if (gTokenCacheFilename[0])
		Dep.ReadCachedTokens(gTokenCacheFilename);

	if (gNFSScriptName[0])
		gOutf = fopen(gNFSScriptName, "w");

	while(pNode)
	{
		Dep.ParseCPPFile( pNode->GetName() );
		pNode = pNode->GetNext();
	}

	JDepFile *pDepFile = Dep.GetFirstFile();
	while(pDepFile)
	{
		char szTemp[512];
		//FormatGNUPath(szTemp, pDepFile->GetName());
		FormatWatcomPath(szTemp, pDepFile->GetName());
		char *pExt = strrchr(szTemp, '.');
		if(pExt == 0)
			strcat(szTemp, ".o");
		else
			strcpy(pExt, ".o");

		// Remove the path from the .o filename
		char *pSlash = strrchr(szTemp, '/');
		if(pSlash) pSlash++;
		else pSlash = szTemp;

		fprintf(gOutf, "DEP: %s%s    ", szOutputPrefix, pSlash);

		//FormatGNUPath(szTemp, pDepFile->GetName());
		FormatWatcomPath(szTemp, pDepFile->GetName());
		fprintf(gOutf, szTemp);

		DumpIncludes(pDepFile);
		IncList.Purge();
		fprintf(gOutf,"\n");

		pDepFile = pDepFile->GetNext();
	}

	Dep.WriteFileChecksums(gOutf);
	fclose(gOutf);

	// Write back TokenCache
	if (gTokenCacheFilename[0])
		Dep.WriteCachedTokens(gTokenCacheFilename);

	if(Dep.WasError() && Dep.bSuppressWarnings == false)
	{
		printf("\nTerminated with error\n");
		return 1;
	}

	if (0)
	{
		extern int ChangedFilenameTableLength;
		extern char ChangedFilenameTable[];
		printf("Newer: ");
		int len = 0;
		while ((len < 160) && (len < ChangedFilenameTableLength))
		{
			printf(" %s", &ChangedFilenameTable[len]);
			len += strlen(&ChangedFilenameTable[len]) + 1;
		}
		if (len < ChangedFilenameTableLength)
			printf(" ...");
		printf("\n");
	}

	return 0;
}

void FormatGNUPath(char *pDest, const char *pSrc)
{
	while(*pSrc)
	{
		if(*pSrc == '\\')
			*pDest++ = '/';
		else if(*pSrc == ' ')
		{
			*pDest++ = '\\';
			*pDest++ = ' ';
		}
		else
			*pDest++ = *pSrc;
		pSrc++;
	}
	*pDest = 0;
}

void FormatWatcomPath(char *pDest, const char *pSrc)
{
	if(strchr(pDest, ' ') != 0)
	{
		pDest[0] = '"';
		strcpy(pDest+1, pSrc);
		strcat(pDest, "\"");
	}
	else
		strcpy(pDest, pSrc);
}

void DumpIncludes(JDepFile *pDepFile, int Indent)
{
	JDepFile *pChild = pDepFile->GetFirstInclude();
	while(pChild)
	{
		if(IncList.FindNode(pChild->GetName()) == 0 && (bHideSysIncludes == false || pChild->bSysInclude == false))
		{
			ccNode *pNode = new ccNode;

			// Convert the file path into GNU CPP format
			char szTemp[512];

			// Hacky!  I store the 'Indent' in the first 2 bytes of the name because I
			// want to recover it during printing.
			if (Indent > 99)
				Indent = 99;	// Could this ever happen?
			sprintf(szTemp, "%02d", Indent);

			//FormatGNUPath(&szTemp[2], pChild->GetName());
			FormatWatcomPath(&szTemp[2], pChild->GetName());

			pNode->SetName(szTemp);
			IncList.AddTail(pNode);
		}

		if(bHideSysIncludes == false || pChild->bSysInclude == false)
			DumpIncludes(pChild, Indent+1);
		pChild = pChild->GetNext();
	}

	if(Indent == 0)
	{
		ccNode *pNode;
		while( pNode = IncList.RemHead() )
		{
			int RecoveredIndent = (pNode->GetName()[0] - '0') * 10 + (pNode->GetName()[1] - '0');

			// Strip off the indent.
			const char *Name = &pNode->GetName()[2];

			fprintf(gOutf, "\n");
			while (RecoveredIndent--)
				fprintf(gOutf, "    ");
			fprintf(gOutf,"    %s", Name);
		}
		printf("");	// Gives me something to break on
	}
}

static void StripQuotes(char *pDest, const char *pSrc)
{
	if(*pSrc == '\"')
		pSrc++;

	while(*pSrc && *pSrc != '\"')
		*pDest++ = *pSrc++;

	*pDest = 0;
}


bool ParseCommandLine(int argc, char *argv[])
{
	if(argc == 1)
	{
		printf(
			"Usage :\n"
			"\n"
			"JDep [/IIncPath] [/SSysPath] [/DPredefines] [/OPOutputPrefix] Files ...\n"
			"  - The /D, /I, and /S commands may have an optional space\n"
			"    between them and their arguments, and may use the form\n"
			"    \"-D\" instead of \"/D\"\n"
			"  - Paths or includes which contain spaces must be in quotes\n"
			"  - /OP (or -OP) designates a prefix which will precede name.o: in the output.\n"
			);
	}

	int Arg = 1;
	bool bResult = true;
	char pArgBuf[1024];

	while(Arg < argc && bResult == true)
	{
		char *pArg = argv[Arg];

		if( pArg[0] == '/' || pArg[0] == '-')
		{
			if(pArg[1] == 'D')			// /D "Predefined"
			{
				if(pArg[2])
					StripQuotes(pArgBuf, pArg+2);
				else
				{
					Arg++;
					StripQuotes(pArgBuf, argv[Arg]);
				}

				Dep.AddDefine(pArgBuf);
			}
			else if(pArg[1] == 'I')		// /I "IncludePath"
			{
				if(pArg[2])
					StripQuotes(pArgBuf, pArg+2);
				else
				{
					Arg++;
					StripQuotes(pArgBuf, argv[Arg]);
				}
				Dep.AddIncludePath(pArgBuf, false);
			}
			else if(pArg[1] == 'S')		// /S "SysIncludePath"
			{
				if(pArg[2])
					StripQuotes(pArgBuf, pArg+2);
				else
				{
					Arg++;
					StripQuotes(pArgBuf, argv[Arg]);
				}
				Dep.AddIncludePath(pArgBuf, true);
			}
			else if(pArg[1] == 'M')
			{
				if(pArg[2] == 'M')	// -MM
					bHideSysIncludes = true;
				else if(pArg[2] == 'G')	// -MG
					Dep.bIgnoreMissingIncludes = true;
			}
			else if(pArg[1] == 'w')
				Dep.bSuppressWarnings = true;
			else if(pArg[1] == 'O' && pArg[2] == 'P')
			{
				// OP = OutputPrefix
				if(pArg[3])
					StripQuotes(pArgBuf, pArg+3);
				else
				{
					Arg++;
					StripQuotes(pArgBuf, argv[Arg]);
				}
				strcpy(szOutputPrefix, pArgBuf);
			}
			else if (_strnicmp(&pArg[1], "NFSScript", strlen("NFSScript")) == 0)
			{
				// NFSScript "filename"
				Arg++;
				StripQuotes(pArgBuf, argv[Arg]);
				strcpy(gNFSScriptName, pArgBuf);
			}
			else if (_strnicmp(&pArg[1], "TokenCacheFile", strlen("TokenCacheFile")) == 0)
			{
				// TokenCacheFile "filename"
				Arg++;
				StripQuotes(pArgBuf, argv[Arg]);
				strcpy(gTokenCacheFilename, pArgBuf);
			}
		}
		else if(pArg[0] == '@')
		{
			if( ParseCommandFile(pArg+1) == false )
				bResult = false;
		}
		else
		{
			// This is probably an input filename
			ccNode *pNode = new ccNode;
			pNode->SetName( pArg );
			FileList.AddTail(pNode);
		}

		Arg++;
	}

	return bResult;
}

// Defined in JFileFilter.cpp
bool IsEOL(int c);


bool ParseCommandFile(const char *pFilename)
{
	FILE *pFile = fopen(pFilename, "rb");
	if(pFile == 0)
		return false;

	Tokenizer Tok;
	Tok.SetWhiteSpace("\t ");

	fseek(pFile, 0, SEEK_END);
	long Len = ftell(pFile);
	fseek(pFile, 0, SEEK_SET);

	char *pBuf = new char[Len+1];
	fread(pBuf, 1, Len, pFile);
	pBuf[Len] = 0;
	fclose(pFile);

	int NumArgs = 1;
	char *Args[4096];
	Args[0] = "Dummy";	// This is where the executable name is expected for ParseCommandLine()

	long Pos = 0, LineLen, LineStart;
	bool bResult = true;

	while(Pos < Len && bResult == true)
	{
		// Measure the line length
		LineStart = Pos;
		while( Pos < Len && !IsEOL(pBuf[Pos]) )
			Pos++;
		LineLen = Pos - LineStart;
		pBuf[Pos++] = 0;	// Delimit the line by overwriting the 1st EOL char

		// Skip any additional EOL chars
		while( Pos < Len && IsEOL(pBuf[Pos]) )
			Pos++;

		// Parse the line
		Tok.Parse(pBuf + LineStart);
		Token *pTok = Tok.GetFirstToken();
		while(pTok && bResult == true)
		{
			Args[NumArgs++] = _strdup(pTok->GetName());
			pTok = pTok->GetNext();
		}
		Tok.Purge();
	}

	delete [] pBuf;

	ParseCommandLine(NumArgs, Args);

	for (int n = 1; n < NumArgs; n++)
		free(Args[n]);

	return bResult;
}
