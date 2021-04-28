/*

	JPrefilteredFile.cpp

*/

#include "StdAfx.h"
#include "JPrefilteredFile.h"
#include "JFileFilter.h"

JPrefiltState::JPrefiltState()
{
	pNextToken = 0;
	pLineStartToken = 0;
}

void JPrefiltState::Init(JPrefilteredFile *pFilt)
{
	pLineStartToken = pFilt->Tokens.GetHead();
	pNextToken = pLineStartToken;
}

ccNode *JPrefiltState::GetNextToken(void)
{
	ccNode *pCurToken = pNextToken;
	if(pNextToken)
	{
		if(pNextToken->GetName()[0] == 0)
			pLineStartToken = pNextToken->GetNext();

		pNextToken = pNextToken->GetNext();
	}

	return pCurToken;
}

void JPrefiltState::PrintCurrentLine(FILE *pStream)
{
	ccNode *pTok = pLineStartToken;
	if(pTok == 0) return;

	do {
		fprintf(pStream, "%s ", pTok->GetName());
		pTok = pTok->GetNext();
	} while(pTok && pTok->GetName()[0]);
	fprintf(pStream, "\n");
}

void JPrefiltState::NextLine(void)
{
	if(pNextToken == 0 || pNextToken == pLineStartToken)	// Already parsed to the next line
		return;

	while(pNextToken->GetName()[0])
		pNextToken = pNextToken->GetNext();

	pNextToken = pNextToken->GetNext();
	pLineStartToken = pNextToken;
}


bool JPrefilteredFile::Open(const char *pFilename)
{
JFileFilter Filt;

	if(Filt.Open(pFilename) == false)
		return false;

	while( Filt.BuildLineTokens(Tokens) )
		;

	return true;
}

void JPrefilteredFile::Open(const char *pBuffer, long Length)
{
JFileFilter Filt;

	Filt.Open(pBuffer, Length);

	while( Filt.BuildLineTokens(Tokens) )
		;
}

void JPrefilteredFile::ReadCachedTokens(char *pTokens, long NumTokens)
{
	int Pos = 0;
	for (int n = 0; n < NumTokens; n++)
	{
		int TokenLen = (int)strlen(&pTokens[Pos]);

		ccNode *pToken = new ccNode;
		pToken->SetName(&pTokens[Pos], TokenLen);
		Tokens.AddTail(pToken);

		Pos = Pos + TokenLen + 1;
	}
}

void JPrefilteredFile::WriteCachedTokens(FILE *f)
{
	for (ccNode *pToken = Tokens.GetHead(); pToken; pToken = pToken->GetNext())
	{
		int len = (int)strlen(pToken->GetName());

		fwrite(pToken->GetName(), 1, len + 1, f);

		pToken->GetNext();
	}
}

