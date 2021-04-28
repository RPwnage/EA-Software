// make base class equivalent to .NET's Object

#include <QString>
#include "objectBase.h"

namespace RTF2HTML
{
    ObjectBase::ObjectBase(QObject* parent)
        : QObject (parent)
    {
        objName = "baseObject";
    }

    ObjectBase::~ObjectBase()
    {
    }

    bool ObjectBase::Equals (ObjectBase* obj)
    {
        Q_UNUSED (obj);
        return false;
    }

    QString& ObjectBase::GetType ()
    {
        return objName;
    }

    QString& ObjectBase::ToString()
    {
        return GetType();
    }
} // namespace Itenso.Rtf.Model
// -- EOF -------------------------------------------------------------------
