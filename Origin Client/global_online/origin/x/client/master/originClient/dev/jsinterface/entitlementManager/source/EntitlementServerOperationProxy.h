#ifndef _ENTITLEMENTSERVEROPERATIONPROXY_H
#define _ENTITLEMENTSERVEROPERATIONPROXY_H

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

class ORIGIN_PLUGIN_API EntitlementServerOperationProxy : public QObject
{
public:
	explicit EntitlementServerOperationProxy(QObject *parent = NULL) : QObject(parent)
	{
	}

	virtual void pause() = 0;
	virtual void resume() = 0;

	virtual QVariant progress() = 0;

	virtual void cancel() = 0;

    virtual bool pausable() = 0;
    virtual bool resumable() = 0;
};

}
}
}

#endif
