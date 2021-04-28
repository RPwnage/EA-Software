//    Copyright © 2011-2012, Electronic Arts
//    All rights reserved.
//    Author: Hans van Veenendaal

#ifndef XMLUTIL_H
#define XMLUTIL_H

#include <limits>
#include <QDateTime>
#include <QString>
#include <QStringList>

#include "services/plugin/PluginAPI.h"

// Helper functions to read parameters out an XML document.
//
//<Doc>
//    <Param>403940</Param>
//</Doc>
class QDomDocument;
class QDomElement;
namespace Origin
{
    namespace Util
    {
        namespace XmlUtil
        {
            ORIGIN_PLUGIN_API bool    getBool(const QDomDocument &doc, QString name, bool _default);
            ORIGIN_PLUGIN_API bool    getBool(const QDomElement &elem, QString name, bool _default);

            ORIGIN_PLUGIN_API QString getString(const QDomDocument &doc, const QString& name, const QString& _default = QString());
            ORIGIN_PLUGIN_API QString getString(const QDomElement &elem, const QString& name, const QString& _default = QString());

            ORIGIN_PLUGIN_API int     getInt(const QDomDocument &doc, QString name, int _default = int());
            ORIGIN_PLUGIN_API int     getInt(const QDomElement &elem, QString name, int _default = int());

            ORIGIN_PLUGIN_API qint64  getLongLong(const QDomDocument &doc, QString name, qint64 _default = quint64());
            ORIGIN_PLUGIN_API qint64  getLongLong(const QDomElement &elem, QString name, qint64 _default = quint64());

            ORIGIN_PLUGIN_API QDateTime getDateTime(const QDomDocument &doc, QString name, QString format = QString("yyyy-MM-dd hh:mm:ss"), QDateTime _default = QDateTime());
            ORIGIN_PLUGIN_API QDateTime getDateTime(const QDomElement &elem, QString name, QString format = QString("yyyy-MM-dd hh:mm:ss"), QDateTime _default = QDateTime());

            ORIGIN_PLUGIN_API QStringList getStringList(const QDomDocument &doc, QString name);
            ORIGIN_PLUGIN_API QStringList getStringList(const QDomElement &elem, QString name);

            ORIGIN_PLUGIN_API QDateTime getDateTimeFromAttribute(const QDomElement &elem, QString name, QString format = QString("yyyy-MM-dd hh:mm:ss"), QDateTime _default = QDateTime());


            // Set functions
            ORIGIN_PLUGIN_API QDomElement newElement(QDomElement &elem, QString name);
            ORIGIN_PLUGIN_API void setBool(QDomElement &elem, QString name, bool value);
            ORIGIN_PLUGIN_API void setString(QDomElement &elem, const QString& name, const QString& value);
            ORIGIN_PLUGIN_API void setInt(QDomElement &elem, QString name, int value);
            ORIGIN_PLUGIN_API void setLongLong(QDomElement &elem, QString name, qint64 value);
            ORIGIN_PLUGIN_API void setDateTime(QDomElement &elem, QString name, const QDateTime &value, QString format = QString("yyyy-MM-dd hh:mm:ss") );
            ORIGIN_PLUGIN_API void setStringList(QDomElement &elem, QString name, const QStringList &values);
        }
    }
}
#endif // XMLUTIL_H
