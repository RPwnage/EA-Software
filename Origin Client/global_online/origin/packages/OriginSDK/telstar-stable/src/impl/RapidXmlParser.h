#ifndef __RAPID_XML_PARSER_H__
#define __RAPID_XML_PARSER_H__

#include "rapidXml/rapidxml.hpp"
#include "rapidXml/rapidxml_print.hpp"

#include "XmlDocument.h"

class RapidXmlParser : public INodeDocument
{
public:
	RapidXmlParser();
	RapidXmlParser(origin_parser::rapidxml::xml_document<> &doc);
	virtual ~RapidXmlParser();

public:
	void *operator new(size_t size);
	void operator delete(void *p);

	// Parse XML from string.
	virtual bool Parse(char *pSource);
	virtual bool Parse(const char *pSource);

	// Functions for navigating
	virtual bool Root();
	virtual bool FirstChild();
	virtual bool FirstChild(const char *pName);
	virtual bool NextChild();
	virtual bool NextChild(const char *pName);
	virtual bool Parent();
	virtual bool FirstAttribute();
	virtual bool NextAttribute();
	virtual bool GetAttribute(const char* name);

	// Functions for querying current state.
	virtual const char * GetName();
	virtual const char * GetValue();
	virtual const char * GetAttributeValue(const char* name);
	virtual const char * GetAttributeName();
	virtual const char * GetAttributeValue();

	// Functions for creating XML
	virtual bool AddChild(const char *Name);
	virtual bool AddChild(const char *Name, const char *Value);
	virtual bool AddAttribute(const char *Name, const char *Value);
	virtual bool SetValue(const char *Value);

	// Parse XML to string.
	virtual const Origin::AllocString & GetXml();

	// Dispose the XmlDocument
	virtual void Release();

	RapidXmlParser(const RapidXmlParser &) = delete;
	RapidXmlParser(RapidXmlParser &&) = delete;
	RapidXmlParser & operator = (const RapidXmlParser &) = delete;
	RapidXmlParser & operator = (RapidXmlParser &&) = delete;

private:
	Origin::AllocString str;

	origin_parser::rapidxml::xml_document<> & m_doc;
    origin_parser::rapidxml::xml_document<> m_defaultDoc;

	origin_parser::rapidxml::xml_node<> * m_parent;
	origin_parser::rapidxml::xml_node<> * m_current;
	origin_parser::rapidxml::xml_attribute<> * m_currentAttribute;
};



#endif //__RAPID_XML_PARSER_H__