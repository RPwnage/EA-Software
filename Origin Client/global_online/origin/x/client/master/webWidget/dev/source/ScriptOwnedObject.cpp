#include <QVariant>

#include "ScriptOwnedObject.h"

namespace WebWidget
{
    ScriptOwnedObject::ScriptOwnedObject(QObject *parent) :
        QObject(parent)
    {
        transferToScriptOwnership(this);
    }
        
    void ScriptOwnedObject::transferToScriptOwnership(QObject *obj)
    {
        obj->setProperty("_origin_script_ownership", QVariant(true));
    }
}
