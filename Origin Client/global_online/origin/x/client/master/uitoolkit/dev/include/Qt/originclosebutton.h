#ifndef ORIGINCLOSEBUTTON_H_
#define ORIGINCLOSEBUTTON_H_

#include <QPushButton>
#include "originuiBase.h"
#include "uitoolkitpluginapi.h"

namespace Origin
{
namespace UIToolkit
{
/// \brief Close button for Origin
///
/// <A HREF="https://developer.origin.com/documentation/display/EBI/Origin+UI+Toolkit#OriginUIToolkit-OriginCloseButton">Click here for more OriginCloseButton documentation</A>

class UITOOLKIT_PLUGIN_API OriginCloseButton : public QPushButton, public OriginUIBase
{
    Q_OBJECT
    Q_ENUMS(ButtonSize)
    Q_PROPERTY(ButtonSize buttonSize READ buttonSize WRITE setButtonSize)

public:
    enum ButtonSize
    {
        ButtonInvalid,
        ButtonLarge,
        ButtonSmall
    };

    OriginCloseButton(QWidget* parent = 0);
    ~OriginCloseButton();

    /// \brief Sets the type of button
    /// 
    /// Sets what type of button it is and accepts only the following values or else it defaults to "ButtonTertiary": 
    /// <ul>
    /// <li>ButtonLarge</li>
    /// <li>ButtonSmall</li>
    /// </ul>
    /// \param ButtonSize the type of button to set
    void setButtonSize(const ButtonSize& buttonSize);

    /// \brief Gets the type of button
    /// 
    /// Gets what type of button it is which must be one of the following: 
    /// <ul>
    /// <li>ButtonLarge</li>
    /// <li>ButtonSmall</li>
    /// </ul>
    /// \return ButtonSize the type of button to set
    ButtonSize buttonSize() const { return mButtonSize; }

protected:
     void paintEvent(QPaintEvent* event);
     bool event(QEvent* event);
     QPushButton* getSelf() {return this;}
     QString&  getElementStyleSheet();
     void prepForStyleReload();

private:
    static QString mPrivateStyleSheet;
    ButtonSize mButtonSize;
};

}
}

#endif