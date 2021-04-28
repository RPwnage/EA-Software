/*

	Tokenizer.cpp - String Tokenizer class functions

*/

#include "StdAfx.h"
#include "Tokenizer.h"

Tokenizer::Tokenizer()
{
	SetWhiteSpace(" \t\n");		// Spaces, Tabs, & NewLine by default
}

Tokenizer::~Tokenizer()
{
}

void Tokenizer::Purge(void)
{
	ccNode *pNode;
	while(pNode = List.RemHead())	// Transfer all the tokens to the free list
		FreeList.AddHead(pNode);
}

void Tokenizer::SetWhiteSpace(char *pSpace)
{
	memset(WhiteSpace, 0, 256);
	while(*pSpace)
		WhiteSpace[ *pSpace++ ] = 1;
}

char *Tokenizer::SkipWhite(char *pStart)
{
	while(*pStart && (WhiteSpace[*pStart]))
		pStart++;

	return pStart;
}

unsigned long Tokenizer::TokenLen(char *pStart)
{
unsigned long Len = 0;
bool InQuote = false;

	while(pStart[Len] && (InQuote || !WhiteSpace[pStart[Len]]) )
	{
		if(pStart[Len] == '"')
		{
			InQuote = !InQuote;
			if(!InQuote) // Just emerged from quotes
			{
				Len++;
				break;
			}
		}
		Len++;
	}

	return Len;
}

int Tokenizer::Parse(char *pString)
{
int Count = 0;
unsigned long Len;
Token *pToken;
char cTemp;

	if(pString == 0) return 0;

	while(pString[0])
	{
		pString = SkipWhite(pString);

		if(*pString == 0)
			break;

		Len = TokenLen(pString);
		if(pString[0] == '"' && pString[Len-1] == '"' && Len > 1)
		{
			pString[Len-1] = 0;
			pToken = (Token *)FreeList.RemHead();
			if(pToken == 0) pToken = new Token;
			pToken->SetName( pString+1 );
			List.AddTail(pToken);
			pString[Len-1] = '"';
		}
		else
		{
			cTemp = pString[Len];
			pString[Len] = 0;
			pToken = (Token *)FreeList.RemHead();
			if(pToken == 0) pToken = new Token;
			pToken->SetName( pString );
			List.AddTail(pToken);
			pString[Len] = cTemp;
		}
		pString += Len;
		Count++;
	}

	return Count;
}
