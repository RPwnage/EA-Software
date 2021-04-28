#ifndef __NODE_DOCUMENT_H__
#define __NODE_DOCUMENT_H__

#include <QByteArray>

class INodeDocument
{
public:
    virtual ~INodeDocument() {}

public:
    // Parse data from string.
    virtual bool Parse(char *pSource) = 0;
    virtual bool Parse(const char *pSource) = 0;

    // Functions for navigating
    virtual bool Root() = 0;
    virtual bool EnterArray(const char *pName) = 0;
    virtual bool FirstChild() = 0;
    virtual bool FirstChild(const char *pName) = 0;
    virtual bool NextChild() = 0;
    virtual bool NextChild(const char *pName) = 0;
    virtual bool Parent() = 0;
    virtual bool FirstAttribute() = 0;
    virtual bool NextAttribute() = 0;
    virtual bool GetAttribute(const char* name) = 0;

    // Navigating array values
    virtual bool FirstValue() = 0;
    virtual bool NextValue() = 0;

    // Functions for querying current state.
    virtual const char * GetName() = 0;
    virtual const char * GetValue() = 0;
    virtual const char * GetAttributeValue(const char* name) = 0;
    virtual const char * GetAttributeName() = 0;
    virtual const char * GetAttributeValue() = 0;

    // Functions for creating Nodes
    virtual bool AddArray(const char *Name) = 0;
    virtual void AddToArray() = 0;
    virtual bool LeaveArray() = 0;
    virtual bool AddChild(const char *Name) = 0;
    virtual bool AddChild(const char *Name, const char *Value) = 0;
    virtual bool AddAttribute(const char *Name, const char *Value) = 0;
    virtual bool SetValue(const char *Value) = 0;

	// Insert a document at the current node.
	virtual bool AddDocument(INodeDocument *pDoc) = 0;

    // Parse Nodes to string.
    virtual QByteArray Print() const = 0;

    // Dispose of the NodeDocument
    virtual void Release() = 0;
};


extern INodeDocument * CreateJsonDocument();
extern INodeDocument * CreateXmlDocument();
extern INodeDocument * CreateXmlDocument(class QDomDocument doc);
extern INodeDocument * CreateXmlDocument(class QDomDocument doc, class QDomNode current);


#endif // __NODE_DOCUMENT_H__