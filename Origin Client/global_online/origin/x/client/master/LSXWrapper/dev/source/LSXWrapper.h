#ifndef __EADM_XML_WRAPPER_H__
#define __EADM_XML_WRAPPER_H__

#include <QDomDocument>
#include "NodeDocument.h"
#include <string>

class LSXWrapper : public INodeDocument
{
public:
    LSXWrapper();
    LSXWrapper(QDomDocument doc);
    LSXWrapper(QDomDocument doc, QDomNode current);
    virtual ~LSXWrapper();

public:
    // Not implemented. Parse XML from string.
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

    // Functions for creating XML
    virtual bool AddArray(const char *Name);
    virtual void AddToArray(){}
    virtual bool LeaveArray();
    virtual bool AddChild(const char *Name);
    virtual bool AddChild(const char *Name, const char *Value);
    virtual bool AddAttribute(const char *Name, const char *Value);
    virtual bool SetValue(const char *Value);

	virtual bool AddDocument(INodeDocument *pDoc);

    virtual QByteArray Print() const;

    QDomNode GetRootNode();

    // Dispose the XmlDocument
    virtual void Release();

private:
    QDomDocument    m_doc;		
    QDomNode		m_parent;
    QDomNode		m_current;
    QByteArray      m_temp;
};


extern INodeDocument * CreateXmlDocument();
extern INodeDocument * CreateXmlDocument(class QDomDocument doc);
extern INodeDocument * CreateXmlDocument(class QDomDocument doc, class QDomNode current);


#endif //__EADM_QXML_WRAPPER_H__