// ObjectBase.h
// make base class equivalent to .NET's Object

#ifndef __OBJECTBASE_H
#define __OBJECTBASE_H

#include <qobject.h>
#include <qstring.h>

namespace RTF2HTML
{
    // ------------------------------------------------------------------------
    class ObjectBase: public QObject
    {
        public:
            ObjectBase(QObject* parent = 0);
            virtual ~ObjectBase();

            virtual bool Equals (ObjectBase* obj);
            virtual QString& GetType();
            virtual QString& ToString ();

            QString objName;
    };
} // namespace Itenso.Rtf.Model
#endif //__OBJECTBASE_H
// -- EOF -------------------------------------------------------------------
