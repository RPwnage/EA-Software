#ifndef _CAPSLOCKPROXY_H
#define _CAPSLOCKPROXY_H

/**********************************************************************************************************
 * This class is part of Origin's JavaScript bindings and is not intended for use from C++
 *
 * All changes to this class should be reflected in the documentation in jsinterface/doc
 *
 * See http://developer.origin.com/documentation/display/EBI/Working+with+Web+Widgets for more information
 * ********************************************************************************************************/

#include <QObject>

#include "services/plugin/PluginAPI.h"

namespace Origin
{
namespace Client
{
namespace JsInterface
{

class ORIGIN_PLUGIN_API CapsLockProxy : public QObject
{
    Q_OBJECT
public:    
    Q_PROPERTY(bool capsLockActive READ capsLockActive);
    bool capsLockActive();

};

}
}
}

#endif
