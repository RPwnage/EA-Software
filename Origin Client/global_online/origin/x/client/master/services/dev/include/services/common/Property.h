//    Copyright © 2011-2012, Electronic Arts
//    All rights reserved.
//    Author: Hans van Veenendaal

#ifndef ORIGINPROPERTY_H
#define ORIGINPROPERTY_H

#include <QVariant>
#include <QReadWriteLock>

////////////////////////////////////////////////////////////////
// Declaration for data in the pimple
// Add types as needed. Only use types that can be converted to strings.

#define DECLARE_PROPERTY(type, name, conversion, _default, setName)\
public:\
    type name() const\
    {\
        QReadLocker locker(m_propertyLock);\
        auto i = m_propertyOverrides.find(#name);\
        if(i != m_propertyOverrides.end())\
            return i.value().conversion();\
        i = m_properties.find(#name);\
        if(i != m_properties.end())\
            return i.value().conversion();\
        return _default;\
    }\
    void setName(const type& value) {\
        QWriteLocker locker(m_propertyLock);\
        m_properties[#name] = value;\
    }\
    void setName##Override(const type& value) {\
        QWriteLocker locker(m_propertyLock);\
        m_propertyOverrides[#name] = value;\
    }

#define DECLARE_PROPERTY_BOOL( name, _default, setName ) DECLARE_PROPERTY(bool, name, toBool, _default, setName)
#define DECLARE_PROPERTY_INT(name, _default, setName) DECLARE_PROPERTY(int, name, toInt, _default, setName)
#define DECLARE_PROPERTY_FLOAT(name, _default, setName) DECLARE_PROPERTY(float, name, toFloat, _default, setName)
#define DECLARE_PROPERTY_DOUBLE(name, _default, setName) DECLARE_PROPERTY(double, name, toDouble, _default, setName)
#define DECLARE_PROPERTY_STRING(name, _default, setName) DECLARE_PROPERTY(QString, name, toString, _default, setName)
#define DECLARE_PROPERTY_STRINGLIST(name, setName) DECLARE_PROPERTY(QStringList, name, toStringList, QStringList(), setName)
#define DECLARE_PROPERTY_LONGLONG(name, _default, setName) DECLARE_PROPERTY(qint64, name, toLongLong, _default, setName)
#define DECLARE_PROPERTY_ULONGLONG(name, _default, setName) DECLARE_PROPERTY(quint64, name, toULongLong, _default, setName)
#define DECLARE_PROPERTY_MAP(name, _default, setName) DECLARE_PROPERTY(QVariantMap, name, toMap, _default, setName)
#define DECLARE_PROPERTY_BYTEARRAY(name, _default, setName) DECLARE_PROPERTY(QByteArray, name, toByteArray, _default, setName)

#define DECLARE_PROPERTY_ENUM(_enum, name, _default, setName)\
public:\
    _enum name() const\
    {\
        QReadLocker locker(m_propertyLock);\
        auto i = m_propertyOverrides.find(#name);\
        if(i != m_propertyOverrides.end())\
            return static_cast<_enum>(i.value().toInt());\
        i = m_properties.find(#name);\
        if(i != m_properties.end())\
            return static_cast<_enum>(i.value().toInt());\
        else\
            return _default;\
    }\
    void setName(_enum value) {\
        QWriteLocker locker(m_propertyLock);\
        m_properties[#name] = (int)value;\
    }\
    void setName##Override(_enum value) {\
        QWriteLocker locker(m_propertyLock);\
        m_propertyOverrides[#name] = (int)value;\
    }


#define QDATESTRING_FORMAT "yyyy-mm-dd"
#define QDATESTRING_FUTURE_DEFAULTVALUE "2999-01-01"
#define QDATESTRING_PAST_DEFAULTVALUE "2012-01-01"

#define DECLARE_PROPERTY_DATETIME(name, _default, format, setName)\
public:\
    QDateTime name() const\
    {\
        QReadLocker locker(m_propertyLock);\
        auto i = m_propertyOverrides.find(#name);\
        if(i != m_propertyOverrides.end())\
            return i.value().value<QDateTime>();\
        i = m_properties.find(#name);\
        if(i != m_properties.end())\
            return i.value().value<QDateTime>();\
        else\
            return QDateTime::fromString(_default, format);\
    }\
    void setName(const QDateTime &value) {\
        QWriteLocker locker(m_propertyLock);\
        m_properties[#name] = value;\
    }\
    void setName##Override(const QDateTime &value) {\
        QWriteLocker locker(m_propertyLock);\
        m_propertyOverrides[#name] = value;\
    }

#define DECLARE_PROPERTY_CONTAINER(cls)\
private:\
    void setproperties(const cls &rhs) {\
        if (this != &rhs) {\
            QReadLocker readLocker(rhs.m_propertyLock);\
            QWriteLocker writeLocker(m_propertyLock);\
            m_properties = rhs.m_properties;\
            m_propertyOverrides = rhs.m_propertyOverrides;\
        }\
    }\
    QVariantHash m_properties;\
    QVariantHash m_propertyOverrides;\
    class CopyableReadWriteLock {\
    public:\
        CopyableReadWriteLock() { }\
        CopyableReadWriteLock(const CopyableReadWriteLock &) { }\
        CopyableReadWriteLock &operator =(const CopyableReadWriteLock &) { return *this; }\
        operator QReadWriteLock *() { return &m_lock; }\
    private:\
        QReadWriteLock m_lock;\
    };\
    mutable CopyableReadWriteLock m_propertyLock;\
public:\
    cls();\
    QVariantHash &properties(){return m_properties;}\
    const QVariantHash &properties() const {return m_properties;}\
    void clearOverrides() { m_propertyOverrides.clear(); }

//////////////////////////////////////////////////
// Implementation defines for the pimple data structure.
//
#define IMPLEMENT_PROPERTY_CONTAINER(cls)\
    cls::cls()

#endif // ORIGINPROPERTY_H
