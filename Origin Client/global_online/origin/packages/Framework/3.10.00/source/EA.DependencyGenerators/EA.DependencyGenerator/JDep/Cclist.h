/*

	ccList Class

*/


#ifndef CCLIST_H
#define CCLIST_H

#include "StringClass.h"

#ifndef BOOL
#define BOOL int
#endif
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif
#ifndef NULL
#define NULL 0
#endif

class ccMinNode;
class ccMinList;
class ccList;
class ccNode;
class ccHashNode;

#define LIST_MAGIC 0xABadCafe


class ccMinNode
{
protected:
	ccMinNode	*next, *prev;

public:
	ccMinNode() { next = (ccMinNode *)LIST_MAGIC; prev = (ccMinNode *)LIST_MAGIC; }
	ccMinNode( const ccMinNode& ) { next = (ccMinNode *)LIST_MAGIC; prev = (ccMinNode *)LIST_MAGIC; }
	virtual ~ccMinNode() {;}

	inline ccMinNode *GetNext(void) const { return next; }
	inline ccMinNode *GetPrev(void) const { return prev; }

	friend class ccMinList;
};


unsigned long CalcHash(const char *String);


class ccNode : public ccMinNode
{
protected:
	String name;
	unsigned long nameHash;

public:
	ccNode() : nameHash(0) {;}
	virtual ~ccNode() {;}
	void SetName(const char *theName) {name = theName; nameHash = CalcHash(theName);}
	void SetName(const char *theName, int Len) {name.Set(theName, Len); nameHash = CalcHash(theName);}

	inline const char *GetName(void) const { return(name); }
	inline unsigned long GetHash(void) const {return nameHash;}

	inline ccNode *GetNext(void) const { return (ccNode *)ccMinNode::GetNext(); }
	inline ccNode *GetPrev(void) const { return (ccNode *)ccMinNode::GetPrev(); }

	friend class ccList;
};




//*********************************************************************

class ccMinList
{
private:
	unsigned long numElements;

	BOOL	IsInList(ccMinNode *n) const;

protected:
	ccMinNode	*head, *tail;

public:
	ccMinList() { head = 0; tail = 0; numElements = 0; }
	virtual ~ccMinList() { Purge(); }

	ccMinNode *RemHead(void);
	ccMinNode *RemTail(void);
	ccMinNode *RemNode(ccMinNode *n);

	void Append(ccMinList &AppList);
	long ElementNumber(ccMinNode *node);

	inline ccMinNode *GetHead(void) const { return head; }
	inline ccMinNode *GetTail(void) const { return tail; }
	inline void AddHead(ccMinNode *n) { AddNode(0,n);}

	void AddTail(ccMinNode *n) { AddNode(tail,n);}
	void AddNode(ccMinNode *insertPoint, ccMinNode *n);
	ccMinNode *FindNode(unsigned long ordinalnumber) const;

	inline void Purge(void)
	{
		ccMinNode *cmn;
		cmn = RemHead();
		while( cmn )
		{
			delete cmn;
			cmn = RemHead();
		}
	}

	inline BOOL IsListEmpty() const {return(head ? 0 : 1);}
	inline unsigned long GetNumElements() const { return(numElements); }

	typedef BOOL (*CheckFlipFunc)(ccMinNode *n, ccMinNode *nn);

	void Sort ( CheckFlipFunc CheckFlip );
};

class ccList : public ccMinList
{
public:
	ccList() { };
	virtual ~ccList() { };
	ccNode *FindNode(const char *name) const;
	ccNode *FindNode(const char *name, ccNode *node) const;
	inline ccNode *FindNode(unsigned long ordinalnumber) const
		{ return (ccNode *)ccMinList::FindNode(ordinalnumber); }

	void SortAlpha();

	inline ccNode *GetHead(void) const { return (ccNode *)ccMinList::GetHead(); }
	inline ccNode *GetTail(void) const { return (ccNode *)ccMinList::GetTail(); }
	inline ccNode *RemHead(void) { return (ccNode *)ccMinList::RemHead(); }
	inline ccNode *RemTail(void) { return (ccNode *)ccMinList::RemTail(); }
	inline ccNode *RemNode(ccMinNode *n)  { return(ccNode *)ccMinList::RemNode(n); }
};

//-------------------------------------------------------------------
// Hash values are maintained for faster searches
//-------------------------------------------------------------------

class ccHashNodePtr;
class ccHashNode;
class ccHashList;

class ccHashNodePtr : public ccMinNode
{
private:
	ccHashNode	*NodePtr;

public:
	inline ccHashNodePtr(ccHashNode *PointTo) {NodePtr = PointTo;}
	inline ccHashNode *GetNodePtr(void) {return NodePtr;}

	friend class ccHashList;
};

class ccHashNode : public ccMinNode
{
private:
	String	Name;
	unsigned long	Hash;
	ccHashNodePtr	*myPtr;

protected:
public:
	ccHashNode();
	ccHashNode( ccHashNode& hashnode );
	virtual ~ccHashNode();

	void SetName(const char *newName);

	inline const char *GetName(void) const { return Name; }

	inline ccHashNode *GetNext(void) const { return (ccHashNode *)ccMinNode::GetNext(); }
	inline ccHashNode *GetPrev(void) const { return (ccHashNode *)ccMinNode::GetPrev(); }

	friend class ccHashList;
};


class ccHashList
{
private:
	long		TableSize;
	ccMinList	*HashTable;
	ccMinList	NodeList;

	void AddHashEntry(ccHashNode *n);
	void RemHashEntry(ccHashNode *n);

public:

	ccHashList();
	ccHashList(long NewTableSize);
	virtual ~ccHashList();

	void SetTableSize(long NewTableSize);
	long GetTableSize() const {return TableSize;}

	void Purge(void);

	void AddHead(ccHashNode *n);
	void AddTail(ccHashNode *n);
	void AddNode(ccHashNode *insertPoint, ccHashNode *n);

	void RemNode(ccHashNode *n);

	ccHashNode *Find(const char *FindName);
	void Rename(ccHashNode *n, const char *NewName);
	inline unsigned long GetNumElements(void) const {return NodeList.GetNumElements();}

	inline ccHashNode *GetHead(void) const { return (ccHashNode *)NodeList.GetHead(); }
	inline ccHashNode *GetTail(void) const { return (ccHashNode *)NodeList.GetTail(); }
};

#endif
