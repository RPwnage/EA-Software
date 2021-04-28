#ifndef _ORIGINPAGEVISIBILITYPROXY_H
#define _ORIGINPAGEVISIBILITYPROXY_H

/**********************************************************************************************************
 * This class is part of Origin's JavaScript bindings and is not intended for use from C++
 *
 * All changes to this class should be reflected in the documentation in jsinterface/doc
 *
 * See http://developer.origin.com/documentation/display/EBI/Working+with+Web+Widgets for more information
 * ********************************************************************************************************/

#include <QObject>
#include <QPointer>

#include "ExposedWidgetDetector.h"
#include "services/plugin/PluginAPI.h"

namespace Origin
{
namespace Client
{

class ExposedWidgetDetector;

namespace JsInterface
{

class ORIGIN_PLUGIN_API OriginPageVisibilityProxy : public QObject
{
    Q_OBJECT
public:
    OriginPageVisibilityProxy(ExposedWidgetDetector *detector, QObject *parent);

    Q_PROPERTY(QString visibilityState READ visibilityState);
    QString visibilityState();

    Q_PROPERTY(bool hidden READ hidden);
    bool hidden();

signals:
    void visibilityChanged();

protected:
    QPointer<ExposedWidgetDetector> mDetector;
};

}
}
}

#endif

