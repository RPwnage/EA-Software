/*

	JFileFilter.h - Preprocessor filter class to simplify parsing

*/

#ifndef JFILEFILTER_H_
#define JFILEFILTER_H_

#include "ccList.h"
#include <StdIO.h>

enum {
	Char_None = 0,
	Char_AlphaNum,
	Char_Comma,
	Char_Character,	// '
	Char_String,	// "
	Char_Group,		// ()
	Char_Expr,		// ~!%^&*+-=<>|
};

class JFileFilter
{
public:
	JFileFilter();
	~JFileFilter();

	bool Open(const char *pFilename);
	bool Open(const char *pBuffer, long Length);

	bool BuildLineTokens(ccList &Tokens);

private:
	void ReadFilteredLine(void);	// Read a whitespace / comment filtered line from the source stream - Put the result in szCurLine
	char ReadFilteredChar(void);	// Read a whitespace / comment filtered char from the source stream
	void SkipToEOL(void);

    void GrowCurrentLine(void);

	bool bAllocd;
	char *pBuf;
	long Pos, Len;
    int CurrentLineLength;
	char* CurrentLine;
};

#endif
