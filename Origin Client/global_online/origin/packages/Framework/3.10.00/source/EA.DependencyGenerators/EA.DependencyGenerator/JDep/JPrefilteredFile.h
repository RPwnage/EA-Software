/*

	JPrefilteredFile.h

*/

#ifndef JPREFILTEREDFILE_H_
#define JPREFILTEREDFILE_H_

#include "ccList.h"
#include <StdIO.h>

class JPrefilteredFile;

class JPrefiltState
{
public:
	JPrefiltState();

	void Init(JPrefilteredFile *pFilt);

	ccNode *GetNextToken(void);
	void NextLine(void);

	void PrintCurrentLine(FILE *pStream = stdout);

private:
	ccNode *pNextToken;
	ccNode *pLineStartToken;
};


class JPrefilteredFile
{
public:
	void ReleaseAll(void);

	bool Open(const char *pFilename);
	void Open(const char *pBuffer, long Length);

	void ReadCachedTokens(char *pTokens, long NumTokens);
	void WriteCachedTokens(FILE *f);

	int GetNumTokens()	{return Tokens.GetNumElements();}

private:
	ccList Tokens;

	friend class JPrefiltState;
};

#endif
