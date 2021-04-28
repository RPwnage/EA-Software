#ifndef ORIGINPUSHBUTTON_H
#define ORIGINPUSHBUTTON_H

#include <QtWidgets/QPushButton>
#include "originuiBase.h"
#include "uitoolkitpluginapi.h"

namespace Origin {
    namespace UIToolkit {

/// \brief Provides a push button control.
/// 
/// The push button is perhaps the most commonly used widget in any graphical user interface. Push (click) a button to command the computer to perform some action, or to answer a question. Typical buttons are OK, Apply, Cancel, Close, Yes, No and Help. A command button is rectangular and typically displays a text label describing its action.
///
/// <A HREF="https://developer.origin.com/documentation/display/EBI/Origin+UI+Toolkit#OriginUIToolkit-OriginPushButton">Click here for more OriginPushButton documentation</A>

class UITOOLKIT_PLUGIN_API OriginPushButton : public QPushButton, public OriginUIBase
{
    Q_OBJECT
    Q_ENUMS(ButtonType);
    Q_PROPERTY(ButtonType mButtonType READ getButtonType WRITE setButtonType DESIGNABLE true)
    Q_PROPERTY(bool default READ isDefault WRITE setDefault)

public:

    /// \brief Constructor
    /// \param parent Pointer to the parent widget, defaults to NULL    
    OriginPushButton(QWidget *parent = 0);
	/// \brief Destructor
    ~OriginPushButton() { this->removeElementFromChildList(this);}

    /// \brief Defines the type of button
    enum ButtonType {
        ButtonPrimary=1, ///< A primary button is also the default button of a dialog. Only one button at a time can be primary.
        ButtonSecondary, ///< The rest of the main command buttons (those at the bottom of a dialog) will be secondary buttons. There may be more than one secondary button at a time.
        ButtonTertiary,  ///< Tertiary buttons appear as actionable buttons within the dialog content itself. The main command buttons (those at the bottom of a dialog) should never be tertiary. Tertiary buttons are also slightly smaller in height than primary and secondary buttons.
		ButtonAccept,	 ///< Accept buttons are used for the actionable notifications to accpet the current request ex: incomming VoIP call or Group Invite
        ButtonDecline,   ///< Decline buttons are used for the actionable notifications to decline the current request ex: incomming VoIP call or Group Invite
		ButtonDark       ///< Dark button used mostly for navigation items on the main client
    };

    /// \brief Sets the type of button
    /// 
    /// Sets what type of button it is and accepts only the following values or else it defaults to "ButtonTertiary": 
    /// <ul>
    /// <li>ButtonPrimary</li>
    /// <li>ButtonSecondary</li>
    /// <li>ButtonTertiary</li>
    /// </ul>
    /// \param buttonType the type of button to set
    void setButtonType(ButtonType buttonType);
    /// \brief Gets the type of button
    /// 
    /// Gets what type of button it is which must be one of the following: 
    /// <ul>
    /// <li>ButtonPrimary</li>
    /// <li>ButtonSecondary</li>
    /// <li>ButtonTertiary</li>
    /// </ul>
    /// \return buttonType the type of button to set
    ButtonType getButtonType() const { return mButtonType; }

    /// \brief Sets if this is the default button
    /// 
    /// Whether or not the button is selected by default when the user presses "enter" on the page. If default, then its click event is fired.
    /// \param bool True if default, false if not
    void setDefault(bool);

signals:
    /// \brief Signal for when the button loses focus
    /// \param btn Pointer to this OriginPushButton
    void focusOut(OriginPushButton* btn);
    /// \brief Signal for when the button gains focus
    /// \param bth Pointer to this OriginPushButton
    void focusIn(OriginPushButton* bth);

protected:
     void paintEvent(QPaintEvent* event);
     bool event(QEvent* event);

     QPushButton* getSelf() {return this;}
     QString&  getElementStyleSheet();


     void changeLang(QString newLang);
     QString& getCurrentLang();
     void prepForStyleReload();


private:
     static QString mPrivateStyleSheetPrimary;
     static QString mPrivateStyleSheetSecondary;
     static QString mPrivateStyleSheetTertiary;
	 static QString mPrivateStyleSheetAccept;
	 static QString mPrivateStyleSheetDecline;
	 static QString mPrivateStyleSheetDark;
     QString mCurrentStyleSheet;
     ButtonType mButtonType;
     bool mFocusRect;
};
    }
}
#endif
