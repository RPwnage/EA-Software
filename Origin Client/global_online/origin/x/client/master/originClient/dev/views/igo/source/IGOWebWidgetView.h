#ifndef _IGOWEBWIDGETVIEW_H
#define _IGOWEBWIDGETVIEW_H

#include <QWidget>
#include "WebWidget/ForwardedWidgetPageView.h"
#include "services/plugin/PluginAPI.h"

class QGraphicsItem;
class QGraphicsWebView;
class QResizeEvent;

namespace Origin
{
namespace Client
{

class ORIGIN_PLUGIN_API IGOWebWidgetView : public QWidget, public WebWidget::ForwardedWidgetPageView
{
    Q_OBJECT
    
public:
    IGOWebWidgetView();
    ~IGOWebWidgetView();
    QGraphicsItem *item() const;

private:
    void resizeEvent(QResizeEvent *);

    QGraphicsWebView *mWebView;
};

} // Client
} // Origin

#endif

