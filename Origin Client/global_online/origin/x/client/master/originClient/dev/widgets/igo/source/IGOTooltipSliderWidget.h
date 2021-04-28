#ifndef IGO_TOOLTIP_SLIDER_WIDGET_H
#define IGO_TOOLTIP_SLIDER_WIDGET_H

#include <QWidget>

#include "services/plugin/PluginAPI.h"

namespace Ui{
    class IGOTooltipSliderWidget;
}

namespace Origin
{
namespace Client
{

class ORIGIN_PLUGIN_API IGOTooltipSliderWidget : public QWidget
{
    Q_OBJECT

public: 
    IGOTooltipSliderWidget(QWidget* parent = NULL);
    ~IGOTooltipSliderWidget();

    void setSliderValue(const int& percent);
    int sliderValue() const;

protected:
    virtual void paintEvent(QPaintEvent*);
    virtual bool event(QEvent *e);
    virtual bool eventFilter(QObject* obj, QEvent *e);

signals:
    void sliderChanged(const int& val);
    void requestVisibilityChange(const bool&);
    void active();
    void mousePressed();
    void mouseReleased();

private slots:
    void onSliderChanged();

private:
    Ui::IGOTooltipSliderWidget* ui;
};

} // Client
} // Origin

#endif