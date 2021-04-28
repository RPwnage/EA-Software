#include "stdafx.h"
#include <sstream>
#include <iosfwd>
#include "rapidxmlparser.h"

using namespace origin_parser::rapidxml;

RapidXmlParser::RapidXmlParser() : 
	m_doc(m_defaultDoc)
{
	m_parent = NULL;
	m_current = NULL;
	m_currentAttribute = NULL;
}

RapidXmlParser::RapidXmlParser(xml_document<> &doc) :
	m_doc(doc)
{
	m_parent = NULL;
	m_current = NULL;
	m_currentAttribute = NULL;
}

RapidXmlParser::~RapidXmlParser()
{

}

	// Parse XML from string.
bool RapidXmlParser::Parse(char *pSource)
{
	m_doc.parse<0>(pSource);

	m_parent = NULL;
	m_currentAttribute = NULL;

	m_current = m_doc.first_node();

	return true;
}

	// Parse XML from string.
bool RapidXmlParser::Parse(const char *pSource)
{
	return this->Parse(m_doc.allocate_string(pSource));
}

	// Functions for navigating
bool RapidXmlParser::Root()
{
	m_parent = NULL;
	m_currentAttribute = NULL;

	m_current = m_doc.first_node();

	return m_current != NULL;
}

bool RapidXmlParser::FirstChild()
{
	m_parent = m_current;
	m_currentAttribute = NULL;

	if(m_current != NULL)
		m_current = m_parent->first_node();

	return m_current != NULL;
}

bool RapidXmlParser::FirstChild(const char *pName)
{
	m_parent = m_current;
	m_currentAttribute = NULL;

	if(m_current != NULL)
		m_current = m_parent->first_node(pName);

	return m_current != NULL;
}

bool RapidXmlParser::NextChild()
{
	m_currentAttribute = NULL;

	if(m_current != NULL)
		m_current = m_current->next_sibling();
	else
		return Root();

	return m_current != NULL;
}
	
bool RapidXmlParser::NextChild(const char *pName)
{
	m_currentAttribute = NULL;

	if(m_current != NULL)
		m_current = m_current->next_sibling(pName);
	else
	{
		if(Root())
		{
			if(m_current != NULL)
			{
				if(strcmp(GetName(), pName) != 0)
				{
					return NextChild(pName);
				}
				else
				{
					return true;
				}
			}
		}
		return false;
	}


	return m_current != NULL;
}
	
bool RapidXmlParser::Parent()
{
	m_current = m_parent;
	m_currentAttribute = NULL;

	if(m_current != NULL)
		m_parent = m_current->parent();

	return m_current != NULL;
}

bool RapidXmlParser::FirstAttribute()
{
	if(m_current != NULL)
		m_currentAttribute = m_current->first_attribute();

	return m_currentAttribute != NULL;
}

bool RapidXmlParser::NextAttribute()
{
	if(m_currentAttribute != NULL)
		m_currentAttribute = m_currentAttribute->next_attribute();
	else
		return FirstAttribute();

	return m_currentAttribute != NULL;
}

bool RapidXmlParser::GetAttribute(const char* name)
{
	if(m_current != NULL)
	{
		m_currentAttribute = m_current->first_attribute(name);

		if(m_currentAttribute != NULL)
		{
			return true;
		}
	}
	return false;
}


	// Functions for querying current state.
const char* RapidXmlParser::GetName()
{
	if(m_current != NULL)
		return m_current->name();

	return NULL;
}

const char* RapidXmlParser::GetValue()
{
	if(m_current != NULL)
		return m_current->value();

	return NULL;
}
	
const char* RapidXmlParser::GetAttributeValue(const char* name)
{
	if(m_current != NULL)
	{
		m_currentAttribute = m_current->first_attribute(name);

		if(m_currentAttribute != NULL)
		{
			return m_currentAttribute->value();
		}
	}

	return NULL;
}

const char * RapidXmlParser::GetAttributeName()
{
	if(m_currentAttribute != NULL)
	{
		return m_currentAttribute->name();
	}
	return NULL;
}

const char * RapidXmlParser::GetAttributeValue()
{
	if(m_currentAttribute != NULL)
	{
		return m_currentAttribute->value();
	}
	return NULL;
}

	// Functions for creating XML
bool RapidXmlParser::AddChild(const char *Name)
{
	if(Name == NULL)
		return false;

	char *node_name = m_doc.allocate_string(Name);
	xml_node<> * node = m_doc.allocate_node(node_element,  node_name);

	if(m_current != NULL)
	{
		m_current->append_node(node);
		m_parent = m_current;
	}
	else
	{
		m_doc.append_node(node);
		m_parent = NULL;
		m_current = node;
	}

	m_current = node;
	m_currentAttribute = NULL;

	return true;
}

bool RapidXmlParser::AddChild(const char *Name, const char *Value)
{
	if(Name == NULL || Value == NULL)
		return false;

	char *node_name = m_doc.allocate_string(Name);
	char *node_text = m_doc.allocate_string(Value);

	xml_node<> * node = m_doc.allocate_node(node_element,  node_name, node_text);

	if(m_current != NULL)
	{
		m_current->append_node(node);
		m_parent = m_current;
	}
	else
	{
		m_doc.append_node(node);
		m_parent = NULL;
		m_current = node;
	}

	m_current = node;
	m_currentAttribute = NULL;

	return true;
}

bool RapidXmlParser::AddAttribute(const char *Name, const char *Value)
{
	if(m_current == NULL || Name == NULL || Value == NULL)
		return false;

	char *node_name = m_doc.allocate_string(Name);
	char *node_text = m_doc.allocate_string(Value);

	xml_attribute<> * node = m_doc.allocate_attribute(node_name, node_text);

	m_current->append_attribute(node);
	m_currentAttribute = node;

	return true;
}
bool RapidXmlParser::SetValue(const char *Value)
{
	if(m_current == NULL || Value == NULL)
		return false;

	char *node_text = m_doc.allocate_string(Value);
	m_current->value(node_text);

	return true;
}



	// Parse XML to string.
const Origin::AllocString & RapidXmlParser::GetXml()
{
	std::basic_ostringstream<char, std::char_traits<char>, Origin::Allocator<char> > result;
	result << *m_current;

	str = result.str().c_str();

	return str;
}

	// Dispose the XmlDocument
void RapidXmlParser::Release()
{
	delete this;
}

void * RapidXmlParser::operator new(size_t size)
{
	return Origin::AllocFunc(size);
}
void RapidXmlParser::operator delete(void *p)
{
	Origin::FreeFunc(p);
}