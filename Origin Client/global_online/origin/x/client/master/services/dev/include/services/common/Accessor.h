//    Copyright © 2011-2012, Electronic Arts
//    All rights reserved.
//    Author: Hans van Veenendaal

#ifndef ORIGINACCESSOR_H
#define ORIGINACCESSOR_H

#include <QPointer>

#define ACCESSOR(t, tLower)\
private:\
    QPointer<QObject> m_##t;\
public:\
    void set##t(class t * object){ m_##t = (QObject *)object; }\
    class t * tLower() const {return (class t * ) m_##t.data();}

#define ACCESSOR_CREATE_IF_NOT_EXIST(t, tLower)\
private:\
    QPointer<QObject> m_##t;\
public:\
    void set##t(class t * object){ m_##t = (QObject *)object; }\
    class t * tLower();

#define IMPLEMENT_ACCESSOR_CREATE_IF_NOT_EXIST(cls, t, tLower)\
    t * cls::tLower()\
    {\
        if(m_##t.isNull())\
        {\
            m_##t = t::create(this);\
        }\
        return (class t * ) m_##t.data();\
    }


#endif // ORIGINACCESSOR_H
