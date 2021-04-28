#ifndef __XMLPARSER_H__
#define __XMLPARSER_H__

#include <string>
#include "../../impl/OriginSDKMemory.h"

class INodeDocument
{
public:
	// Parse XML from string.
	virtual bool Parse(char *pSource) = 0;
	virtual bool Parse(const char *pSource) = 0;

	// Functions for navigating
	virtual bool Root() = 0;
	virtual bool FirstChild() = 0;
	virtual bool FirstChild(const char *pName) = 0;
	virtual bool NextChild() = 0;
	virtual bool NextChild(const char *pName) = 0;
	virtual bool Parent() = 0;
	virtual bool FirstAttribute() = 0;
	virtual bool NextAttribute() = 0;
	virtual bool GetAttribute(const char* name) = 0;

	// Functions for querying current state.
	virtual const char * GetName() = 0;
	virtual const char * GetValue() = 0;
	virtual const char * GetAttributeValue(const char* name) = 0;
	virtual const char * GetAttributeName() = 0;
	virtual const char * GetAttributeValue() = 0;

	// Functions for creating XML
	virtual bool AddChild(const char *Name) = 0;
	virtual bool AddChild(const char *Name, const char *Value) = 0;
	virtual bool AddAttribute(const char *Name, const char *Value) = 0;
	virtual bool SetValue(const char *Value) = 0;

	// Parse XML to string.
	virtual const Origin::AllocString & GetXml() = 0;

	// Dispose the XmlDocument
	virtual void Release() = 0;
};

INodeDocument * CreateRapidXmlDocument();

#endif // __XMLPARSER_H__