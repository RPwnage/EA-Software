/*

	JFileFilter.cpp - Preprocessor filter class to simplify parsing

*/

#include "StdAfx.h"
#include "JFileFilter.h"
//#include "Timer.h"
#include <StdIO.h>
#include <CType.h>
#include <xmmIntrin.h>
#include <Assert.h>

static char ClassifyChar(char Char);

JFileFilter::JFileFilter()
{
    CurrentLineLength = 256;
    CurrentLine = new char[CurrentLineLength];

	bAllocd = false;
	pBuf = 0;
	Pos = Len = 0;
}

JFileFilter::~JFileFilter()
{
	if(bAllocd)
		delete [] pBuf;

    delete[] CurrentLine;
}


bool JFileFilter::Open(const char *pFilename)
{
	FILE *pFile = fopen(pFilename, "rb");
	if(pFile == 0)
		return false;

	fseek(pFile, 0, SEEK_END);
	Len = ftell(pFile);
	fseek(pFile, 0, SEEK_SET);

	bAllocd = true;
	pBuf = new char[Len+9];		// Allow for one char + a full MMX read
	fread(pBuf, 1, Len, pFile);
	//memset(pBuf + Len, 0, 9);	// Fill the padding with 0's to stop any out-of-bounds accesses
	pBuf[Len] = 0;
	fclose(pFile);
	Pos = 0;

	return true;
}

bool JFileFilter::Open(const char *pBuffer, long Length)
{
	bAllocd = false;
	pBuf = (char *)pBuffer;
	Len = Length;
	Pos = 0;
	return true;
}

bool JFileFilter::BuildLineTokens(ccList &Tokens)
{
	if(Pos >= Len) return false;

	ReadFilteredLine();

	// Now tokenize the line
	int TokenType;
	int TokenLen, TokenStart;
	int LinePos = 0;

	while(CurrentLine[LinePos])
	{
		TokenStart = LinePos;
		TokenType = ClassifyChar( CurrentLine[LinePos++] );
		while( CurrentLine[LinePos] && ClassifyChar(CurrentLine[LinePos]) == TokenType )
			LinePos++;

		if(TokenType == Char_String)
		{
			// Find the terminating double quote
			while(CurrentLine[LinePos])
			{
				if(CurrentLine[LinePos] == '\"')
					break;

				if(CurrentLine[LinePos] == '\\' && CurrentLine[LinePos+1] != 0)
					LinePos+=2;
				else
					LinePos++;
			};

			if(CurrentLine[LinePos])	// Include trailing quote
				LinePos++;
		}
		else if(TokenType == Char_Character)
		{
			// Find the terminating single quote
			while(CurrentLine[LinePos])
			{
				if(CurrentLine[LinePos] == '\'')
					break;

				if(CurrentLine[LinePos] == '\\' && CurrentLine[LinePos+1] != 0)
					LinePos+=2;
				else
					LinePos++;
			};

			if(CurrentLine[LinePos])	// Include trailing quote
				LinePos++;
		}
		else if(TokenType == Char_Group)		// bracket chars are considered singular tokens
			LinePos = TokenStart+1;
		else if(CurrentLine[TokenStart] == '<')	// <string.h> considered single token
		{
			ccNode *pPrevToken = Tokens.GetTail();
			if(pPrevToken && pPrevToken->GetName() && (
                                                        strcmp(pPrevToken->GetName(), "include") == 0 ||
                                                        strcmp(pPrevToken->GetName(), "import") == 0 ||
                                                        strcmp(pPrevToken->GetName(), "using") == 0
                                                      ))
			{
				while(CurrentLine[LinePos] && CurrentLine[LinePos] != '>')
					LinePos++;

				if(CurrentLine[LinePos] == '>')
					LinePos++;
			}
		}

		TokenLen = LinePos - TokenStart;

		char TempChar = CurrentLine[LinePos];
		CurrentLine[LinePos] = 0;
		ccNode *pToken = new ccNode;
		pToken->SetName(CurrentLine + TokenStart, TokenLen);
		Tokens.AddTail(pToken);

		CurrentLine[LinePos] = TempChar;

		while( CurrentLine[LinePos] && isspace(CurrentLine[LinePos]) )
			LinePos++;
	}

	ccNode *pToken = new ccNode;	// Unnamed token = terminator
	pToken->SetName("");
	Tokens.AddTail(pToken);
	return true;
}

bool IsEOL(int c)
{
	return (c == 0x0a) || (c == 0x0d);
}

void JFileFilter::ReadFilteredLine(void)
{
	int LineLen = 0;
	char c;
	CurrentLine[0] = 0;

	do {
		do {
			c = ReadFilteredChar();
		} while(c == ' ' && Pos < Len);

		if(c == '#')	// Found a preprocessor line
			break;

		// Look for the line end
		if( !IsEOL(c) && Pos < Len)
		{
			SkipToEOL();
			while( pBuf[Pos] && IsEOL(pBuf[Pos]) )
				Pos++;
		}
	} while(Pos < Len);

	if(Pos >= Len)	// EOF?
		return;

	// Skip any whitespace after the # character
	while(isspace(pBuf[Pos]) && !IsEOL(pBuf[Pos]) && Pos < Len)
		Pos++;

	bool bKeepReading;
	do {
		bKeepReading = false;
		do {
            c = ReadFilteredChar();

            if (LineLen >= CurrentLineLength) {
                GrowCurrentLine();
            }

			CurrentLine[LineLen++] = c;


		} while (!IsEOL(c) && Pos < Len);
		CurrentLine[--LineLen] = 0;

		if(LineLen && CurrentLine[LineLen-1] == '\\')
		{
			bKeepReading = true;
			CurrentLine[LineLen-1] = ' ';
		}
	} while(bKeepReading);
}

void JFileFilter::GrowCurrentLine() 
{
    int oldCurrentLineLength = CurrentLineLength;
    const char* pOldCurrentLine = CurrentLine;

    CurrentLineLength = CurrentLineLength * 2;
    CurrentLine = new char[CurrentLineLength];
    memcpy(CurrentLine, pOldCurrentLine, oldCurrentLineLength);
    delete[] pOldCurrentLine;
}

// Read a single character from the token stream, compressing whitespace and ignoring comments
char JFileFilter::ReadFilteredChar(void)
{
	if(isalnum(pBuf[Pos]))
		return pBuf[Pos++];

	if(isspace(pBuf[Pos]))
	{
		// Is it an EOL space?
		if( IsEOL(pBuf[Pos]) )
		{
			if( Pos > 0  &&  pBuf[Pos-1] == '\\' )
			{
				//
				// Do not collapse newlines if previous line had a
				// continuation character.  Otherwise constructs like below
				// will be treated as one line:
				//
				// #define FOO \
				//
				// #include "bar.h"
				//
				if( Pos < Len - 1)
				{
					// skip alternate CR/LF character properly
					if( pBuf[Pos] != pBuf[Pos+1] && IsEOL(pBuf[Pos+1]) )
						++Pos ;
				}
				++Pos ;
				return '\n' ;
			}
			while(IsEOL(pBuf[Pos]) && Pos < Len)
				Pos++;
			return '\n';
		}

		while(isspace(pBuf[Pos]) && !IsEOL(pBuf[Pos]) && Pos < Len)
			Pos++;

		return ' ';
	}

	if(pBuf[Pos] == '/')
	{
		if(pBuf[Pos+1] == '/')	// C++ style comment - Scan for EOL
		{
			Pos += 2;
			//while(!IsEOL(pBuf[Pos]) && Pos < Len)
			//	Pos++;

			SkipToEOL();
			while(IsEOL(pBuf[Pos]) && Pos < Len)
				Pos++;

			return '\n';
		}

		if(pBuf[Pos+1] == '*')	// C Style comment - scan for */
		{
			Pos += 2;
			while(Pos < Len && !(pBuf[Pos] == '*' && pBuf[Pos+1] == '/') )
				Pos++;

			Pos += 2;
			while(isspace(pBuf[Pos]) && !IsEOL(pBuf[Pos]) && Pos < Len)
				Pos++;

			if( Pos < Len && IsEOL(pBuf[Pos]) )
			{
				while(Pos < Len && IsEOL(pBuf[Pos]))
					Pos++;
				return '\n';
			}
			return ' ';
		}
	}

	return pBuf[Pos++];
}

void JFileFilter::SkipToEOL(void)
{
	if(Pos >= Len)
		return;

	while(pBuf[Pos] && !IsEOL(pBuf[Pos]))
	    Pos++;
}


static char ClassifyChar(char Char)
{
	if( isalnum(Char) )
		return Char_AlphaNum;

	switch(Char)
	{
	case '_':
	case '#':	return Char_AlphaNum;

	case ',':	return Char_Comma;

	case '(':
	case ')':	return Char_Group;
	case '\'':	return Char_Character;
	case '\"':	return Char_String;

	case '~':
	case '!':
	case '%':
	case '^':
	case '&':
	case '*':
	case '-':
	case '+':
	case '=':
	case '<':
	case '>':
	case '|':	return Char_Expr;
	};

	return Char_None;
}
