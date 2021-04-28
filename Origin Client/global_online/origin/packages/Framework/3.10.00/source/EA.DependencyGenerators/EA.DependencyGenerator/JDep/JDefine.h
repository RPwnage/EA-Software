/*

	JDefine.h - Class for holding and parsing #define commands

*/

#ifndef __JDEFINE_H__
#define __JDEFINE_H__

#include "ccList.h"
#include <Assert.h>

class JDefine : public ccNode
{
public:
	JDefine() : bIsValue(false), bEvaluated(false), Value(0) {;}

	bool bIsValue, bEvaluated;
	int Value;			// If this define equates to a value, this is it

	void AddToken(const char *pTok);
	ccList	Tokens;		// Only used if the object hasn't been evaluated yet

	JDefine *GetNext(void) const {return (JDefine *)ccMinNode::GetNext();}
	JDefine *GetPrev(void) const {return (JDefine *)ccMinNode::GetPrev();}
};


class JDefineList
{
public:
	JDefineList();
	JDefineList(int Size);
	~JDefineList();

	void Purge(void);

	void Add(JDefine *pNode);
	JDefine *Remove(JDefine *pNode);
	JDefine *GetHead(void) {assert(TableSize == 1); return (JDefine *)pList[0].GetHead();}

	JDefine *Find(const char *pName);

private:
	int		TableSize;
	ccList	*pList;
};

#endif
