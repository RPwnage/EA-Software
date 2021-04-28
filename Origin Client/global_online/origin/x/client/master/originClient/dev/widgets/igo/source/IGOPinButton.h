#ifndef IGO_PIN_BUTTON_H
#define IGO_PIN_BUTTON_H

#include <QtWidgets/QPushButton>
#include "UIScope.h"

#include "engine/igo/IGOController.h"
#include "services/plugin/PluginAPI.h"

namespace Origin
{
namespace Engine
{
    class IGOQtContainer;
}
    
namespace Client
{
    class IGOTooltipSliderWidget;

class ORIGIN_PLUGIN_API IGOPinButton : public QPushButton, public Origin::Engine::IPinButtonViewController
{
    Q_OBJECT

public: 
    IGOPinButton(const Origin::Client::UIScope& context, QWidget* parent = NULL);
    ~IGOPinButton();

    // IPinButtonViewController
    virtual QWidget* ivcView() { return NULL; }
    virtual bool ivcPreSetup(Origin::Engine::IIGOWindowManager::WindowProperties* properties, Origin::Engine::IIGOWindowManager::WindowBehavior* behavior) { return true; }
    virtual void ivcPostSetup() {}

    virtual void setPinned(bool pinned);
    virtual bool pinned() const;
    virtual void setVisibility(bool visible);
    virtual void setSliderValue(int percent);
    virtual int  sliderValue() const;

    
public slots:
    virtual void raiseSlider();
    virtual void hideSlider();
    virtual void showSlider();

signals:
    void pinningFinished();
    void pinningStarted();
    void pinningToggled();
     void sliderChanged(const int& val);

protected:
    bool event(QEvent*);


private:
    IGOTooltipSliderWidget* mIGOTooltipSlider;
    const Origin::Client::UIScope mContext;
    int mTimerID;

private slots:
    void onToggle(const bool&);
    void onSliderRequestVisibilityChange(const bool&);
    void onSliderActive();
};
    
    
} // Client
} // Origin

#endif