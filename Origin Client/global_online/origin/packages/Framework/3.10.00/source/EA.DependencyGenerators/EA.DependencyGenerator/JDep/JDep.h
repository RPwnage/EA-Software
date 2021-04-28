/*

	JDep.h - Jason's dependency generator class definition

*/

#ifndef __JDEP_H__
#define __JDEP_H__

#include "JDep.h"
#include "JDepFile.h"
#include "JSearchCache.h"

class JPrefilteredFile;
class JDefine;

class JDep
{
public:
	JDep();
	~JDep();

	void Reset(void);												// Clear all internals

	void AddIncludePath(const char *pPath, bool bSysPath = false);	// Pointer to include path to add
    void AddUsingPath(const char *pPath);	// Pointer to include path to add
	void AddDefine(const char *pDef);								// pointer to string to parse into a #define value or expression

	void OutputWarning(const char *pFmt);							// display a warning
	void OutputWarning(const char *pFmt, const char*);							// display a warning
	void OutputWarning(const char *pFmt, const char*, const char*);							// display a warning

	// Load & parse a file (pFilename), or if you pass a buffer, it assumes the file was loaded by you, and will be freed by you
	JDepFile *ParseCPPFile(const char *pFilename, JDepFile *pParent = 0, bool bSysInc = false, JPrefilteredFile *pFilt = 0);

    // Load & parse a file (pFilename), or if you pass a buffer, it assumes the file was loaded by you, and will be freed by you
    JDepFile *ParseDotNetAssemblyFile(const char *pFilename, JDepFile *pParent = 0, bool bSysInc = false);

	JDepFile *GetFirstFile(void) {return (JDepFile *)Files.GetHead();}
	JDefine *GetFirstDefine(void) {return Defines.GetHead();}

	bool WasError(void) const {return bError;}

 	void ReadCachedTokens(const char *pFilename);
 	void WriteCachedTokens(const char *pFilename);
 
 	void WriteFileChecksums(FILE *f);


	bool bSkipDuplicates, bShowDuplicates, bShowSysIncludes, bSuppressWarnings;
	bool bIgnoreMissingIncludes;

	int ExprCount, IfdefCount, IfCount, DefineCount, FileCount;

private:
	ccList		IncPaths;	// Include paths
	ccList		SysPaths;	// System include paths
    ccList		UsingPaths; // System include paths
	JDefineList	Defines;	// List of global defines

	ccList			Files;		// List of previously parsed files
	JSearchCache	SearchCache;// List of previously searched file locations

	bool	bError;
	int		IncDepth;	// Include depth - used as a terminator for infinite includes

	void ProcessToken(ccNode *pToken, JDepFile *pDepFile);


	long Preprocess(char *pFile, long Pos, long FileLen, JDepFile *pDepFile);				// Attempt to process a #define, #include, etc
	long FindEOL(char *pFile, long Pos, long FileLen);				// Scan for EOL or EOF

	long SkipComment(char *pFile, long Pos, long FileLen);			// Skip past a C-style comment block
	long GetIdentifierLen(char *pFile, long Pos, long FileLen) const;// Get the length of an identifier starting at Pos

    bool ExtractFileName(JDepFile *pDepFile, bool& bIsLocal, bool& bSysInc, char* szInclude);
    JPrefilteredFile* EvaluateIncludeFileName(JDepFile *pDepFile, const char* szInclude, bool bIsLocal, bool bSysInc, char* szIncludePath);
    bool EvaluateUsingFileName(JDepFile *pDepFile, const char* szInclude, bool bIsLocal, bool bSysInc, char* szIncludePath);
    void ParseInclude(JDepFile *pDepFile);							// Parse an #include directive
    void ParseUsing(JDepFile *pDepFile);                            // Parse a #using directive
    void ParseImport(JDepFile *pDepFile);                           // Parse a #import directive
	void ParseDefine(JDepFile *pDepFile);							// Parse a #define directive

	void ParseUndef(JDepFile *pDepFile);							// Parse an #undef directive

	void MakeIncludePathFromFile(const char *pCurFilePath, const char *szInclude, char *pDest) const;
	void MakeIncludePathFromPath(const char *pCurPath, const char *szInclude, char *pDest) const;

	void ParseIfdef(JDepFile *pDepFile);							// Parse an #ifdef
	void ParseIfndef(JDepFile *pDepFile);							// Parse an #ifndef
	void ParseElse(JDepFile *pDepFile);								// Parse an #else
	void ParseElif(JDepFile *pDepFile);								// Parse an #elif
	void ParseIf(JDepFile *pDepFile);								// Parse an #if

	bool IsDefined(const char *pSymbol, JDepFile *pDepFile) const {return pDepFile->FindDefine(pSymbol) != 0;}

	int EvaluateExpression(ccNode *pToken, JDepFile *pDepFile);
};

#endif
