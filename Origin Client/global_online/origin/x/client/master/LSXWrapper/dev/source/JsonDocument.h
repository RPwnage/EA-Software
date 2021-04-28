#include "NodeDocument.h"

#ifndef __JSON_DOCUMENT_H__
#define __JSON_DOCUMENT_H__

#include <string>
#include "libjson.h"

/*
 * Note: The JsonDocument wrapper over the libjson library doesn't allow access to array elements.
 */
 

class JsonDocument : public INodeDocument
{
public:
    JsonDocument();
    virtual ~JsonDocument() {}

public:
    // Parse data from string.
    virtual bool Parse(char *pSource);
    virtual bool Parse(const char *pSource);

    // Functions for navigating
    virtual bool Root();
    virtual bool EnterArray(const char *Name);
    virtual bool FirstChild();
    virtual bool FirstChild(const char *pName);
    virtual bool NextChild();
    virtual bool NextChild(const char *pName);
    virtual bool Parent();
    virtual bool FirstAttribute();
    virtual bool NextAttribute();
    virtual bool GetAttribute(const char* name);

    // Navigating array values
    virtual bool FirstValue();
    virtual bool NextValue();

    // Functions for querying current state.
    virtual const char * GetName();
    virtual const char * GetValue();
    virtual const char * GetAttributeValue(const char* name);
    virtual const char * GetAttributeName();
    virtual const char * GetAttributeValue();

    // Functions for creating Nodes
    virtual bool AddArray(const char *Name);
    virtual void AddToArray(){currentIndex = -1;}
    virtual bool LeaveArray();
    virtual bool AddChild(const char *Name);
    virtual bool AddChild(const char *Name, const char *Value);
    virtual bool AddAttribute(const char *Name, const char *Value);
    virtual bool SetValue(const char *Value);

	// Add a node document to the current node.
	virtual bool AddDocument(INodeDocument *pDoc);

    // Parse Nodes to string.
    virtual QByteArray Print() const;

    // Dispose of the NodeDocument
    virtual void Release();

private:
    libjson::json_object * CurrentObject();
    libjson::json_pair * CurrentPair();
    libjson::json_pair * CurrentAttribute();


private:

    libjson::json doc;
    std::vector<libjson::json_object *> parents;
    int currentPair;        // Navigating an object list
    int currentAttribute;   // Navigating an attribute list
    int currentIndex;       // Navigating an array
    std::string temp;
};

#endif // __JSON_DOCUMENT_H__