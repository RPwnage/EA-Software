#ifndef ORIGINWINDOW_H_
#define ORIGINWINDOW_H_

#include "originchromebase.h"
#include "originmsgbox.h"
#include "originscrollablemsgbox.h"
#include "uitoolkitpluginapi.h"
#include <QDialogButtonBox>
#include <QPoint>
#include <QTime>
#include <QIcon>

namespace Origin {
    namespace UIToolkit {

class OriginWindowTemplateBase;
class OriginWindowManager;
class OriginWebView;
class OriginPushButton;
class OriginTitlebarBase;

/// \brief Origin main window. It holds custom widget content.
/// <A HREF="https://developer.origin.com/documentation/display/EBI/Origin+UI+Toolkit#OriginUIToolkit-OriginWindow">Click here for more OriginWindow documentation</A>
class UITOOLKIT_PLUGIN_API OriginWindow : public OriginChromeBase
{
	Q_OBJECT
    Q_ENUMS(WindowType);
    Q_PROPERTY(ChromeStyle mChromeStyle READ getChromeStyle WRITE setChromeStyle DESIGNABLE true)
    Q_PROPERTY(bool ignoreEscPress READ getIgnoreEscPress WRITE setIgnoreEscPress)

public:
    /// \brief Items that are shown on the titlebar
    enum TitlebarItem
    {
        Icon = 0x01
        , Minimize = 0x02
        , Maximize = 0x04
        , Close = 0x08
        , AllItems = 0x0F // Does not include fullscreen button
        , FullScreen = 0x10
    };
    Q_DECLARE_FLAGS(TitlebarItems, TitlebarItem)

    /// \brief The template of the window.
    /// Msgbox - sets the window's content as a OriginWindowTemplateMsgBox
    /// Scrollable - sets the window's content as a OriginWindowTemplateScrollable
    /// WebContent - sets the window's content as a OriginWebView
    /// Custom - No template will be applied
    enum ContentTemplate
    {
          MsgBox
        , Scrollable
		, ScrollableMsgBox
        , WebContent
        , Custom
    };

    /// \brief Where in the world is the window?
    /// Normal - Window is outside of the OIG. Window has no special treatment
    /// OIG - Window is in of the OIG. The window manager, tooltips, and system menu have been removed.
#include "originuiscope.h"

    /// \brief Border style of the window
    /// ChromeStyle_Dialog - a drop shadow. Used mostly for non-resizable windows
    /// ChromeStyle_Window - a dark gray 3px border. Used mostly for resizable windows
    enum ChromeStyle {ChromeStyle_Dialog=1, ChromeStyle_Window};
	
	

    /// \brief Constructor
    /// \param itemFlags Items that are shown on the titlebar. (See TitlebarItem)
    /// \param content Content of the window if the template is not set. Otherwise, the content of the template.
    /// \param contentTemplate The template of the window. (See ContentTemplate)
    /// \param buttons The buttons at the bottom of the window. Only usable if the template is a MsgBox or Scrollable.
    /// \param windowType Border style of the window.
    /// \param context TBD.
	OriginWindow(const TitlebarItems& itemFlags = AllItems, 
                QWidget* content = NULL,
                const ContentTemplate& contentTemplate = Custom, 
                QDialogButtonBox::StandardButtons buttons = QDialogButtonBox::NoButton,
                ChromeStyle windowType = ChromeStyle_Dialog, const WindowContext& context = Normal);

    /// \brief Destructor - deletes all content.
	~OriginWindow();

    /// \brief Sets the text of the label visible to the left of the 
    /// close buttons.
	void setEnvironmentLabel(const QString& lbl);

    /// \brief Returns the border with of the window. These are hard coded 
    /// values that are set in the qss. The value you get back depends on 
    /// what the ChromeStyle is set to.
    const QMargins borderWidths() const;

    /// \brief Returns the window's manager. Manager is in charge of the 
    /// non-interface part of a window. 
    /// (e.g. movement. See OriginWindowManager for more details)
    /// \return OriginWindowManager* Pointer to Origin Manager/Controller
	OriginWindowManager* manager() const {return mManager;}

    /// \brief Enable/Disable the window manager. This should only be used in OIG.
    /// By default the manager is enabled.
    void setManagerEnabled(const bool& enable);

    /// \brief Returns if the window manager is enabled. By default it is.
    bool isManagerEnabled() const;

    /// \brief Sets the border style of the window. (See ChromeStyle)
    void setChromeStyle(ChromeStyle chromeStyle);

    /// \brief Gets the border style of the window. (See ChromeStyle)
    ChromeStyle getChromeStyle() const { return mChromeStyle; }

    /// \brief Gets whether window is ignore the Esc button press.
    bool getIgnoreEscPress() const {return mIgnoreEscPress;}

    /// \brief Sets whether the window should ignore the Esc button press.
    void setIgnoreEscPress(const bool& ignore) {mIgnoreEscPress = ignore;}

    /// \brief Sets the custom content of the titlebar
    void setTitleBarContent(QWidget* content);

    /// \brief Sets the content of the window. If the window's template is 
    /// MsgBox or Scrollable the it will be the content of the msgbox or 
    /// scrollable object.
    void setContent(QWidget* content);

    /// \brief Returns the content of the window. If the window's template is 
    /// MsgBox or Scrollable the it will be the content of the msgbox or 
    /// scrollable object.
    /// \return QWidget* Pointer to Content
    QWidget* const content() const;

    /// \brief Returns the msgbox of the window.
    /// This will only return a value if the template is set to MsgBox.
    /// \return OriginMsgBox* Pointer to the template's OriginMsgBox
    OriginMsgBox* const msgBox() const;
	
	/// \brief Returns the msgbox of the window.
    /// This will only return a value if the Content Template is ScrollableMsgBox.
    OriginScrollableMsgBox* const scrollableMsgBox() const;

    /// \brief Returns the msgbox of the window.
    /// This will only return a value if the template is set to WebContent.
    /// \return OriginWebView* Pointer to the web container
    OriginWebView* const webview() const;

    // Banner
    /// \brief Ensures the window's message banner was created.
    /// This will only return a value if the template is set to WebContent.
    void ensureCreatedMsgBanner();

    /// \brief Sets the visibility of the banner.
    /// Banner shows just under the window's titlebar.
    void setBannerVisible(const bool& visible);

    /// \brief Sets the visibility of the message banner.
    /// Message banner covers the content of the window.
    void setMsgBannerVisible(const bool& visible);
    
    /// \brief Enable/Disabled system menu - enabled by default.
    /// System menu appears if the Icon on the titlebar is clicked.
    void setSystemMenuEnabled(const bool& enable);

#if defined(ORIGIN_MAC)
    /// \brief Hide/Show the titlebar - shown by default
    void setHideTitlebar(bool aHide);

    /// \brief Returns true if this window is currenly in fullscreen mode.
    /// Else returns false.
    bool isFullscreen();
#endif
    
    /// \brief Returns the titlebar item flags currently set on the window
    const TitlebarItems titlebarItems() const { return mTitlebarItems; }

    /// \brief Set the titlebar item flags currently set on the window
    void setTitlebarItems(const TitlebarItems& titlebarItems);

    /// \brief Returns the list of buttons in the titlebar
    QVector<QObject*> titlebarButtons() const;
    
    /// \brief Returns the context of the window. E.g. OIG or Normal. See enum WindowContext
    const WindowContext windowContext() const { return mWindowContext; }

    /// \brief Returns the titlebar text or msgbox title
    const QString decoratedWindowTitle();

// If-def'ed because the mac titlebar doesn't include these functions.
// If it should be taken out: Put function in OriginTitlebarBase as pure virtual
#ifdef ORIGIN_PC
    // Menu Bar area
    /// \brief Returns the layout of the menu bar container. 
    /// Menu bar container is contained in the titlebar - right of the icon.
    /// \return QLayout* Pointer to the menu layout
    QLayout* menuBarLayout();

    /// \brief Sets the visibility of the menu bar - in the titlebar.
    void setMenuBarVisible(const bool& visible);

    /// \brief Calls winEvent which handles our MS window events. 
    /// Used for the MS Window menu from clicking on the titlebar icon. Is 
    /// called from OriginTitlebarWin.
    bool msWinEvent(MSG * message, long *result);
#endif

    // MSG box UI Elements
    // Buttons
    /// \brief (Template - MsgBox & Scrollable) Adds a button to the button box
    /// \param which - The type of QDialogButtonBox::StandardButton you wish to add
    /// \return OriginPushButton* A pointer to the newly created button
    OriginPushButton* addButton(QDialogButtonBox::StandardButton which);

    // Ways of getting a button or it's info
    /// \brief (Template - MsgBox & Scrollable) Get the button associated with the type
    /// \param which - The type of button you are looking for
    /// \return OriginPushButton* Pointer to the button
    OriginPushButton* button(QDialogButtonBox::StandardButton which) const;

    /// \brief (Template - MsgBox & Scrollable) Get the type for the passed in button
    /// \param button - Pointer to the button whose type you want to know
    /// \return QDialogButtonBox::StandardButton The type of button
    QDialogButtonBox::StandardButton standardButton(OriginPushButton *button) const;

    /// \brief (Template - MsgBox & Scrollable) Get the role for the passed in button.
    /// \return QDialogButtonBox::ButtonRole The role associated with this button
    QDialogButtonBox::ButtonRole buttonRole(OriginPushButton *) const;

    // - Default
    /// \brief (Template - MsgBox & Scrollable) Sets the default button of the button box.
    /// \param which - type of button to set as the default button.
    void setDefaultButton(QDialogButtonBox::StandardButton which);

    /// \brief (Template - MsgBox & Scrollable) Sets the default button of the button box.
    /// \param button - Pointer to the button to be set to default.
    void setDefaultButton(OriginPushButton* button=NULL);

    /// \brief (Template - MsgBox & Scrollable) Gets the default button of the button box
    /// \return OriginPushButton - Pointer to the default button
    OriginPushButton* defaultButton() const;

    // - Modify Msg UI
    /// \brief (Template - MsgBox & Scrollable) Sets the text on the specified button
    /// 
    /// This is a helper method that finds the button specified by the type and sets the text
    /// on that button
    /// \param which Which button to set the text on
    /// \param text The new text of the button
    void setButtonText(QDialogButtonBox::StandardButton which, const QString &text);

    /// \brief (Template - MsgBox & Scrollable) Sets content of bottom left area of window.
    /// \param content - content that will be put in the bottom left area of the window.
    void setCornerContent(QWidget* content);

    /// \brief (Template - MsgBox & Scrollable) Get content of bottom left area of window.
    /// \return QWidget* Pointer to the content that is at bottom left area of the window.
    QWidget* cornerContent() const;

    /// \brief (Template - MsgBox & Scrollable) Toggles the enabled state of all the buttons in the button box
    /// \param enable True if all buttons are to be enabled, False if all buttons are to be disabled
    void setEnableAllButtons(bool enable);

    /// \brief (Template - MsgBox & Scrollable) Returns a pointer to the last clicked button 
    /// \return OriginPushButton* Returns a pointer to the last clicked button, if no button has been been clicked, returns NULL
    OriginPushButton* getClickedButton();

    /// \brief (Template - MsgBox & Scrollable) Resets the clicked button property
    /// 
    /// This is used in the case where the dialog is reused. It will clear the last clicked state
    // and set it to NULL
    void clearClickedButton();

    /// \brief Does the exec but with a move to the location if supplied, then shows the dialog, raises the dialog and finally an exec
    /// \return The result of the window. (e.g. QDialog::Accepted)
    int execWindow(const QPoint& pos = QPoint());

    /// \brief Returns true if the window supports fullscreen mode
    bool supportsFullscreen();

    /// \brief Configures the window for OIG. Turns off tooltips, removes window manager,
    /// and turns off System Menu.
    void configForOIG(const bool& keepWindowManagerEnabled = false);

    // Static convenience methods

    // \brief An "ok" type of 1 button alert dialog
    static void alert(OriginMsgBox::IconType icon, const QString& title, const QString& description, const QString& buttonText);

    // \brief An "ok" type of 1 button alert dialog
    static void alert(OriginMsgBox::IconType icon, const QString& title, const QString& description, const QString& buttonText,
        const QString& windowTitle, const QPoint& newPosition);

    // \brief An "ok" type of 1 button alert dialog that is non modal.
    // \return Return value is a non modal window.
    static OriginWindow* alertNonModal(OriginMsgBox::IconType icon, const QString& title, const QString& description, const QString& buttonText);

    // \brief An "ok" type of 1 button alert dialog that is non modal and scrollable.
    // \return Return value is a non modal window.
    static OriginWindow* alertNonModalScrollable(OriginScrollableMsgBox::IconType icon, const QString& title, const QString& description, const QString& buttonText);

    // \brief A "yes/no" or "ok/cancel" type of 2 button prompt dialog - Use the default title bar 'Origin'
    // \return Return value is either QDialog::Accepted or QDialog::Rejected
    static int prompt(OriginMsgBox::IconType icon, const QString& title, const QString& description, const QString& acceptText, const QString& rejectText, QDialogButtonBox::StandardButton defaultButton=QDialogButtonBox::NoButton);

    // \brief A "yes/no" or "ok/cancel" type of 2 button prompt dialog
    // \return Return value is either QDialog::Accepted or QDialog::Rejected
    static int prompt(OriginMsgBox::IconType icon, const QString& titleBar, const QString& title, const QString& description, const QString& acceptText, const QString& rejectText, QDialogButtonBox::StandardButton defaultButton=QDialogButtonBox::NoButton);

    // \brief A "yes/no" or "ok/cancel" type of 2 button prompt dialog
    // \return Return value is a non modal window.
    static OriginWindow* promptNonModal(OriginMsgBox::IconType icon, const QString& title, const QString& description, const QString& acceptText, const QString& rejectText, QDialogButtonBox::StandardButton defaultButton=QDialogButtonBox::NoButton);
    
    // \brief A "yes/no" or "ok/cancel" type of 2 button prompt dialog that's scrollable
    // \return Return value is a non modal window.
    static OriginWindow* promptNonModalScrollable(OriginScrollableMsgBox::IconType icon, const QString& title, const QString& description, const QString& yesText, const QString& noText, QDialogButtonBox::StandardButton defaultButton/*=QDialogButtonBox::NoButton*/);

    // \brief Simple getter for the time stamp of this window
    // \return Return value is the time this window was created
    QTime creationTime(){return mCreationTime;};
    
    // \brief Simple setter for the time stamp of this window, the setter is needed because we never recreate the main window thus we need to reset the timeCreated when we "unhide" it.
    void setCreationTime();

    /// \brief Sets up the ui of the window.
    void setupUiFunctionality(const TitlebarItems& itemFlags);

public slots:
    /// \brief Sets the text displayed in the middle of the titlebar.
	void setTitleBarText(const QString& title);

    /// \brief Reimplemented from QWidget. 
    /// If the window is minimized it calls onRestoreClicked.
    void raise();

    /// \brief Does the show but with an optional move (if the location is supplied), then shows the dialog and raises it
    void showWindow(const QPoint& pos = QPoint());

    /// \brief Maximizes the window. Use only if window has maximize/restore titlebar items.
    /// Updates the titlebar's UI to be in the maximize state
    void showMaximized();

    /// \brief Minimizes the window. Use only if window has the minimize titlebar item.
    void showMinimized();

    /// \brief Restores the window to it's state/position before being maximized.
    /// Typically only use if the window has maximize/restore titlebar items.
    /// Updates the titlebar's UI to be in the normal state
    void showNormal();

    /// \brief Zooms the window. Use only if window has the zoomed titlebar item.
    void showZoomed();
    
#if defined (ORIGIN_MAC)
    /// \brief Toggles fullscreen mode on the window
    void toggleFullScreen();
#endif
	
signals:
    /// \brief Emitted when the maximize titlebar button is clicked.
	void maximizeClicked();

    /// \brief Emitted when the minimize titlebar button is clicked.
	void minimizeClicked();

    /// \brief Emitted when the restore titlebar button is clicked.
	void restoreClicked();

    /// \brief Emitted when the banner link is clicked 
    /// \param const The anchor associated with the link
    void bannerLinkActivated(const QString&);

    /// \brief Emitted when the fullscreen mode changes
    void fullScreenChanged(bool isFullscreen);

    /// \brief (Template - MsgBox & Scrollable) Emitted when a button has been clicked
    /// \param button A pointer to the clicked button
    void buttonClicked(OriginPushButton* button);

    /// \brief Emitted in close event.
    void closeEventTriggered();

	/// \brief emmited when window is going to be shown. This signal is emitted from the showEvent
	void aboutToShow();

    void closeWindow();

protected:
    /// \brief Loads OriginWindow private style sheet
	QString& getElementStyleSheet();

    /// \brief Clears OriginWindow private style sheet
	void prepForStyleReload();

    /// \brief Set up and plugs in our ui files.
	void pluginCustomUi();

    /// \brief Handles our events. 
	bool event(QEvent* event);

    /// \brief HACK -
    /// For inexplicable reasons, when showMinimized() happens the button remains
    /// with it's hover image. Perhaps we are not calling showMinimized() correctly..?
    /// the minimize animation does not occur. Dialog acts like hide() is being called.
	void showEvent(QShowEvent* event);

    /// \brief Emits closeEventTriggered()
    void closeEvent(QCloseEvent* event);

    /// \brief Used for ignoring the Esc key press
    void keyPressEvent(QKeyEvent* event);
	
#ifdef ORIGIN_PC
    /// \brief Handles our MS window events. Used mostly for the MS Window menu
    /// from clicking on the titlebar icon.
	bool nativeEvent(const QByteArray & eventType, void * message, long * result);
#endif

protected slots:
    /// \brief Win Only - Called when the maximize titlebar button is clicked.
    /// Calls showMaximized()
    void onMaximizeClicked();

    /// \brief Win Only - Called when the minimize titlebar button is clicked.
    /// Calls showMinimized()
    void onMinimizeClicked();

    /// \brief Win Only - Called when the restore titlebar button is clicked.
    /// Calls showNormal()
    void onRestoreClicked();

#if defined (ORIGIN_MAC)
    /// \brief Mac Only - Called when the fullscreen titlebar button is clicked.
    void onFullScreenClicked();
#endif

private slots:
    /// \brief Adjust the size of the window. Because the resize doesn't 
    /// adjust properly until after the end of a event loop.
	Q_INVOKABLE void adjustWidgetSize (QWidget* );

private:
    /// \brief TBD
	void switchMaximizeButton(const bool& isMaximizeStyle);

    /// \brief Show/Hides the window's size grip.
    /// \param aEnable - Have the size grip visible.
    void setEnableSizeGrip(bool aEnable);

    /// \brief Round the tops top right and left part of the window. 
    /// \param rect The rect of the window
    /// \param r The roundness/radius of the corner
    /// \return QRegion Value to the new region that will be set window's mask
    QRegion roundedRectRegion(const QRect& rect, int r);

    /// \brief Round the tops top right and left part of the window. This
    /// is dependent upon whether the window is maximized or not
    void updateWindowRegion();

    OriginWindowTemplateBase* mTemplate;
	OriginScrollableMsgBox* mScrollableMsgBox;
    OriginTitlebarBase* mTitlebar;
	OriginWindowManager* mManager;
	static QString mPrivateStyleSheet;
    bool mIgnoreEscPress;
    ChromeStyle mChromeStyle;
    const ContentTemplate mContentTemplate;
    TitlebarItems mTitlebarItems;
    const WindowContext mWindowContext;
    QTime mCreationTime;
};
Q_DECLARE_OPERATORS_FOR_FLAGS(OriginWindow::TitlebarItems)
}
}
#endif
