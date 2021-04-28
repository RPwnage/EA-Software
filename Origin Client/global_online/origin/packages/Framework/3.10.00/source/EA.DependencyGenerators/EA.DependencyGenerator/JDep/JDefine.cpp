/*

	JDefine.cpp - Class for holding and parsing #define commands

*/

#include "StdAfx.h"
#include "JDefine.h"


void JDefine::AddToken(const char *pTok)
{
	ccNode *pNode = new ccNode;
	pNode->SetName(pTok);
	Tokens.AddTail(pNode);
}

JDefineList::JDefineList()
{
	TableSize = 1;
	pList = new ccList[TableSize];
}

JDefineList::JDefineList(int Size)
{
	TableSize = Size;
	pList = new ccList[TableSize];
}

JDefineList::~JDefineList()
{
	delete [] pList;
}

void JDefineList::Purge(void)
{
	for(int i=0; i<TableSize; i++)
		pList[i].Purge();
}

void JDefineList::Add(JDefine *pNode)
{
	int ListIndex = pNode->GetHash() % TableSize;
	pList[ListIndex].AddTail(pNode);
}

JDefine *JDefineList::Find(const char *pName)
{
	unsigned long Hash = CalcHash(pName);
	int ListIndex = Hash % TableSize;

	ccNode *pNode = pList[ListIndex].GetHead();
	while(pNode)
	{
		if(pNode->GetHash() == Hash && strcmp(pNode->GetName(), pName) == 0)
			return (JDefine *)pNode;

		pNode = pNode->GetNext();
	}

	return 0;
}

JDefine *JDefineList::Remove(JDefine *pNode)
{
	int ListIndex = pNode->GetHash() % TableSize;
	return (JDefine *)pList[ListIndex].RemNode(pNode);
}
