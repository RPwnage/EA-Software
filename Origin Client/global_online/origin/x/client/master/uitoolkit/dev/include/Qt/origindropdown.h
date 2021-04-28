#ifndef ORIGINDROPDOWN_H
#define ORIGINDROPDOWN_H

#include <QtWidgets/QComboBox>
#include "originuiBase.h"
#include "uitoolkitpluginapi.h"

namespace Origin {
namespace UIToolkit {
class UITOOLKIT_PLUGIN_API OriginDropdown : public QComboBox, public OriginUIBase
{
    Q_OBJECT

public:
    OriginDropdown(QWidget *parent = 0);
    ~OriginDropdown();

    // Moved from ebisudropdown to make settingsdialog work. 
    // TODO: Do something better than this.
    /// \brief Lets parent container to know if the last event was a wheel event
    bool isWheel() const { return mWheel; }
    void showPopup();

protected:
     void paintEvent(QPaintEvent* event);
     bool event(QEvent* event);

     QComboBox* getSelf() {return this;}
     QString&  getElementStyleSheet();
     void prepForStyleReload();

private:
     static QString mPrivateStyleSheet;
     // Moved from ebisudropdown to make settingsdialog work.
     // TODO: Do something better than this.
     /// \brief Set to true if there was a wheel event
     bool mWheel;
};
}
}
#endif