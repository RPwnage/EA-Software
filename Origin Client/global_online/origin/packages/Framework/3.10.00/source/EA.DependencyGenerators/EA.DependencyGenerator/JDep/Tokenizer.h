/*

	Tokenizer.h - String Tokenizer class prototype

*/

#ifndef TOKENIZER_H
#define TOKENIZER_H

#include "ccList.h"

class Token : public ccNode
{
public:
	inline Token *GetNext(void) {return (Token *)ccNode::GetNext();}
	inline Token *GetPrev(void) {return (Token *)ccNode::GetPrev();}
};

// ----------------------------------------------------------------------------
// Tokenizer
//
// Parses a string for tokens.  "Whitespace" delimits tokens, unless surrounded
// by quotes.  Whitespace values default to tab, space, and newline, but can be
// specified.
//
// FindToken is case sensitive.
// ----------------------------------------------------------------------------
class Tokenizer
{
private:
	ccList	List;
	ccList	FreeList;
	char	WhiteSpace[256];	// Fast lookup (0/1) per char

	char *SkipWhite(char *pStart);
	unsigned long TokenLen(char *pStart);

public:
	Tokenizer();
	~Tokenizer();

	void ReleaseAll(void) {List.Purge(); FreeList.Purge();}
	void Purge(void); 		// Reset the tokenizer (transfers in-use to the free list)

	void SetWhiteSpace(char *pSpace);

	// Returns the number of tokens found
	int Parse(char *pString);

	unsigned long TokenCount(void) {return List.GetNumElements();}
	Token *GetFirstToken(void) {return (Token *)List.GetHead();}
	Token *GetLastToken(void) {return (Token *)List.GetTail();}
	Token *FindToken(char *pToken) {return (Token *)List.FindNode(pToken);}
	Token *RemoveToken(Token *pToken) {return (Token *)List.RemNode(pToken);}
};

#endif
