//    Copyright © 2011-2012, Electronic Arts
//    All rights reserved.
//    Author: Hans van Veenendaal

#include <limits>
#include <QDateTime>
#include <QVariant>
#include <QDomNodeList>

#include "services/common/XmlUtil.h"

namespace Origin
{
    namespace Util
    {
        namespace XmlUtil
        {
            bool getBool(const QDomDocument &doc, QString name, bool _default)
            {
                QDomNodeList list = doc.elementsByTagName(name);

                if(!list.isEmpty())
                {
                    return list.item(0).toElement().text().compare("true", Qt::CaseInsensitive) == 0;
                }
                return _default;
            }

            bool getBool(const QDomElement &elem, QString name, bool _default)
            {
                QDomElement node = elem.firstChildElement(name);

                if(!node.isNull())
                {
                    return node.text().compare("true", Qt::CaseInsensitive) == 0;
                }
                return _default;
            }

            QString getString(const QDomDocument &doc, const QString& name, const QString& _default)
            {
                QDomNodeList list = doc.elementsByTagName(name);

                if(!list.isEmpty())
                {
                    return list.item(0).toElement().text();
                }
                return _default;
            }

            QString getString(const QDomElement &elem, const QString& name, const QString& _default)
            {
                QDomElement node = elem.firstChildElement(name);

                if(!node.isNull())
                {
                    return node.text();
                }
                return _default;
            }


            int getInt(const QDomDocument &doc, QString name, int _default)
            {
                QDomNodeList list = doc.elementsByTagName(name);

                if(!list.isEmpty())
                {
                    return QVariant(list.item(0).toElement().text()).toInt();
                }
                return _default;
            }

            int getInt(const QDomElement &elem, QString name, int _default)
            {
                QDomElement node = elem.firstChildElement(name);

                if(!node.isNull())
                {
                    return node.text().toInt();
                }
                return _default;
            }

            qint64 getLongLong(const QDomDocument &doc, QString name, qint64 _default)
            {
                QDomNodeList list = doc.elementsByTagName(name);

                if(!list.isEmpty())
                {
                    return list.item(0).toElement().text().toLongLong();
                }
                return _default;
            }

            qint64 getLongLong(const QDomElement &elem, QString name, qint64 _default)
            {
                QDomElement node = elem.firstChildElement(name);

                if(!node.isNull())
                {
                    return node.text().toLongLong();
                }
                return _default;
            }

            QDateTime getDateTime(const QDomDocument &doc, QString name, QString format, QDateTime _default)
            {
                QDomNodeList list = doc.elementsByTagName(name);

                if(!list.isEmpty())
                {
                    QDateTime parsedTime(QDateTime::fromString(list.item(0).toElement().text(), format));
                    // Always use UTC
                    parsedTime.setTimeSpec(Qt::UTC);
                    return parsedTime;
                }
                return _default;
            }

            QDateTime getDateTime(const QDomElement &elem, QString name, QString format, QDateTime _default)
            {
                QDomElement node = elem.firstChildElement(name);

                if(!node.isNull())
                {
                    QDateTime parsedTime(QDateTime::fromString(node.text(), format));
                    // Always use UTC
                    parsedTime.setTimeSpec(Qt::UTC);
                    return parsedTime;
                }
                return _default;
            }

            QDateTime getDateTimeFromAttribute(const QDomElement &elem, QString name, QString format, QDateTime _default)
            {
                QString date = elem.attribute(name);
                if(!date.isEmpty())
                {
                    QDateTime parsedTime(QDateTime::fromString(date, format));
                    // Always use UTC
                    parsedTime.setTimeSpec(Qt::UTC);
                    return parsedTime;
                }
                return _default;
            }

            QStringList getStringList(const QDomDocument &doc, const QString name)
            {
                QDomNodeList list = doc.elementsByTagName(name);
                QStringList strings;

                if(!list.isEmpty())
                {
                    for(int i=0; i<list.size(); i++)
                    {
                        strings.push_back(list.at(i).toElement().text());
                    }
                }
                return strings;
            }

            QStringList getStringList(const QDomElement &elem, QString name)
            {
                QDomElement node = elem.firstChildElement(name);
                QStringList strings;

                while(!node.isNull())
                {
                    strings.push_back(node.text());            
                    node = node.nextSiblingElement(name);
                }
                return strings;
            }

            QDomElement newElement(QDomElement &elem, QString name)
            {
                QDomElement e = elem.ownerDocument().createElement(name);
                elem.appendChild(e);
                return e;
            }

            void setString(QDomElement &elem, const QString& name, const QString& value)
            {
                QDomElement e = newElement(elem, name);
                QDomText t = elem.ownerDocument().createTextNode(value);
                e.appendChild(t);
            }

            void setBool(QDomElement &elem, QString name, bool value)
            {
                setString(elem, name, QVariant(value).toString());
            }

            void setInt(QDomElement &elem, QString name, int value)
            {
                setString(elem, name, QVariant(value).toString());
            }

            void setLongLong(QDomElement &elem, QString name, qint64 value)
            {
                setString(elem, name, QVariant(value).toString());
            }

            void setDateTime(QDomElement &elem, QString name, const QDateTime &value, QString format)
            {
                setString(elem, name, value.toString(format));
            }

            void setStringList(QDomElement &elem, QString name, const QStringList &values)
            {
                foreach(QString value, values)
                {
                    setString(elem, name, value);
                }
            }
        }
    }
}