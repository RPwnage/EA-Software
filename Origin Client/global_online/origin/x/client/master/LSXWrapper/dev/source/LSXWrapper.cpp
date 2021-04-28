#include "LSXWrapper.h"
#include <QTextStream>

INodeDocument * CreateXmlDocument()
{
    return new LSXWrapper();
}

INodeDocument * CreateXmlDocument(class QDomDocument doc, class QDomNode current)
{
    return new LSXWrapper(doc, current);
}

LSXWrapper::LSXWrapper()
{
    QDomElement root = m_doc.createElement("QLSXWrapperDummyRoot");
    m_doc.appendChild(root);
    m_current = m_doc.documentElement();
}


LSXWrapper::LSXWrapper(QDomDocument doc)
    : m_doc(doc)
    , m_current(doc.documentElement())
{
}

LSXWrapper::LSXWrapper(QDomDocument doc, QDomNode current)
    : m_doc(doc)
    , m_current(current)
{
}

LSXWrapper::~LSXWrapper()
{

}

// Parse XML from string.
bool LSXWrapper::Parse(char *pSource)
{
    m_doc.clear();
    m_doc.setContent(QByteArray(pSource));

    return Root();
}

// Parse XML from string.
bool LSXWrapper::Parse(const char *pSource)
{
    m_doc.clear();
    m_doc.setContent(QByteArray(pSource));

    return Root();
}

// Functions for navigating
bool LSXWrapper::Root()
{
    m_parent.clear();
    m_current = m_doc.documentElement();

    return !m_current.isNull();
}

bool LSXWrapper::FirstChild()
{
    m_parent = m_current;

    if(!m_current.isNull())
        m_current = m_current.firstChild();

    return !m_current.isNull();
}

bool LSXWrapper::FirstChild(const char *pName)
{
    m_parent = m_current;

    if(!m_current.isNull())
    {
        m_current = m_current.firstChildElement(pName);
    }

    return !m_current.isNull();
}

bool LSXWrapper::NextChild()
{
    if(!m_current.isNull())
        m_current = m_current.nextSiblingElement();
    else
        return Root();

    return !m_current.isNull();
}

bool LSXWrapper::NextChild(const char *pName)
{
    if(!m_current.isNull())
        m_current = m_current.nextSiblingElement(pName);
    else
    {
        if(Root())
        {
            if(!m_current.isNull())
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

    return !m_current.isNull();
}

bool LSXWrapper::Parent()
{
    m_current = m_parent;

    if(!m_current.isNull())
        m_parent = m_current.parentNode();

    return !m_current.isNull();
}

bool LSXWrapper::FirstAttribute()
{
    return false;
}

bool LSXWrapper::NextAttribute()
{
    return false;
}

bool LSXWrapper::GetAttribute(const char* name)
{
    if(m_current.attributes().contains(name))
    {
        GetAttributeValue(name);
        return true;
    }
    return false;
}

// Navigating array values
bool LSXWrapper::FirstValue()
{
    // TODO: Implement XML array navigation
    return false;
}

bool LSXWrapper::NextValue()
{
    // TODO: Implement XML array navigation
    return false;
}

// Functions for querying current state.
const char* LSXWrapper::GetName()
{
    if(!m_current.isNull())
    {
        m_temp = m_current.nodeName().toUtf8();
        return m_temp.data();
    }
    return NULL;
}

const char* LSXWrapper::GetValue()
{
    if(!m_current.isNull())
    {
        m_temp = m_current.firstChild().toText().data().toUtf8();
        return m_temp.data();
    }
    return NULL;
}

const char* LSXWrapper::GetAttributeValue(const char* name)
{
    if(!m_current.isNull())
    {
        QDomNode attr = m_current.attributes().namedItem(name);
        if(!attr.isNull())
        {
            m_temp = attr.nodeValue().toUtf8();
            return m_temp.data();
        }
    }
    return "";
}

const char * LSXWrapper::GetAttributeName()
{
    return NULL;
}

const char * LSXWrapper::GetAttributeValue()
{
    return m_temp.data();
}

bool LSXWrapper::EnterArray( const char *Name )
{
    return false;                        
}

bool LSXWrapper::AddArray( const char *Name )
{
    return false;                        
}

bool LSXWrapper::LeaveArray()
{
    return false;
}

// Functions for creating XML
bool LSXWrapper::AddChild(const char *Name)
{
    if(Name == NULL)
        return false;

    QDomNode node = m_doc.createElement(Name);

    if(!m_current.isNull())
    {
        m_current.appendChild(node);
        m_parent = m_current;
    }
    else
    {
        m_doc.appendChild(node);
        m_parent.clear();
    }

    m_current = node;

    return true;
}

bool LSXWrapper::AddChild(const char *Name, const char *Value)
{
    if(Name == NULL || Value == NULL)
        return false;

    QDomNode node = m_doc.createElement(Name);
    node.setNodeValue(Value);

    if(!m_current.isNull())
    {
        m_current.appendChild(node);
        m_parent = m_current;
    }
    else
    {
        m_doc.appendChild(node);
        m_parent.clear();
    }

    m_current = node;

    return true;
}

bool LSXWrapper::AddAttribute(const char *Name, const char *Value)
{
    if(m_current.isNull() || Name == NULL || Value == NULL)
        return false;

    QDomNode attr = m_doc.createAttribute(Name);
    attr.setNodeValue(QString::fromUtf8(Value));
    m_current.attributes().setNamedItem(attr);

    return true;
}

bool LSXWrapper::SetValue(const char *Value)
{
    if(m_current.isNull() || Value == NULL)
        return false;

    QDomText text = m_doc.createTextNode(QString::fromUtf8(Value));

    m_current.appendChild(text);

    return true;
}

bool LSXWrapper::AddDocument(INodeDocument *pDoc)
{
	return false;
}


// Parse XML to string.
QByteArray LSXWrapper::Print() const
{
	if(!m_current.isNull())
	{
		QByteArray dest;

		QTextStream stream(&dest, QIODevice::ReadWrite);
        stream.setCodec("UTF-8");

		stream << m_current.firstChildElement();

		return dest;
	}
	else
	{
		return m_doc.toByteArray(0);
	}
}

QDomNode LSXWrapper::GetRootNode()
{
    if(!m_doc.isNull() && !m_doc.firstChild().isNull())
    {
        QDomNode child = m_doc.firstChild();
        if(child.nodeName() == "QLSXWrapperDummyRoot")
            return child.firstChild();
    }
    return m_doc.firstChild();
}

// Dispose the XmlDocument
void LSXWrapper::Release()
{
    delete this;
}

