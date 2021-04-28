#ifndef _WEBWIDGET_SCRIPTOWNEDOBJECT_H
#define _WEBWIDGET_SCRIPTOWNEDOBJECT_H

#include <QObject>

#include "WebWidgetPluginAPI.h"

namespace WebWidget
{
    ///
    /// Base class for objects owned by the JavaScript garbage collector
    ///
    /// This is intended to be used by QObjects returned by native intefaces 
    /// that should have their lifecycles determined by JavaScript. When there
    /// are no more references to a script owned object in the JavaScript
    /// context the object will be eventually deleted. 
    /// 
    /// A script owned object must only be returned to JavaScript once. It must
    /// not be deleted from C++ while the JavaScript context still exists. 
    ///
    class WEBWIDGET_PLUGIN_API ScriptOwnedObject : public QObject
    {
    public:
        ///
        /// Converts an existing object to JavaScript ownership
        ///
        /// Passing an object to this function is equivalent to having that
        /// object derive from ScriptOwnedObject. See the  documentation for
        /// ScriptOwnedObject and ScriptOwnedObject::ScriptOwnedObject for
        /// for more information.
        ///
        /// \param  obj  Object to transfer to script ownership
        ///
        static void transferToScriptOwnership(QObject *obj); 

    protected:
        ///
        /// Creates a script owned object
        ///
        /// \param  parent  Parent object of the script owned object. The
        ///                 parent object must have a lifetime longer than the
        ///                 JavaScript context the script owned object is being
        ///                 returned to. Script owned objects must not have
        ///                 other script owned objects as parents.
        ///
        ScriptOwnedObject(QObject *parent);
    };
}

#endif
