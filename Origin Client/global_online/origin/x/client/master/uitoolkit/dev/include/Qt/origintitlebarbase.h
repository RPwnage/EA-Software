#ifndef ORIGINTITLEBARBASE_H
#define ORIGINTITLEBARBASE_H

#include <QWidget>
#include "originuiBase.h"
#include "uitoolkitpluginapi.h"

namespace Origin {
namespace UIToolkit {

class OriginWindow;

/// \brief Provides interface for window titlebar. Used inside OriginWindow
class UITOOLKIT_PLUGIN_API OriginTitlebarBase : public QWidget, public OriginUIBase
{
    Q_OBJECT

public:
    /// \brief Constructor
    /// \param parent Pointer to the QWidget that is the parent of this titlebar.r
    /// \param partner TBD.
    OriginTitlebarBase(QWidget* parent = 0, OriginWindow* partner = 0);
    /// \brief Destructor
    ~OriginTitlebarBase();

	/// \brief Sets the text of the label visible to the left of the 
    /// close buttons.
    virtual void setEnvironmentLabel(const QString& lbl) = 0;
	
	/// \brief Sets the text displayed in the middle of the titlebar.
    virtual void setTitleBarText(const QString& ) = 0;

    /// \brief Adds a custom widget to the title bar.
    virtual void setTitleBarContent(QWidget *content) = 0;
	
	/// \brief Sets the items that are shown on the titlebar. 
#if defined(Q_OS_MAC)
	/// hasMaximize works for the zoom button for OSX
    virtual void setupButtonFunctionality(const bool& hasIcon, const bool& hasMinimize, 
                                          const bool& hasMaximize, const bool& hasClose,
                                          const bool& hasFullScreen) = 0;
#else
    virtual void setupButtonFunctionality(const bool& hasIcon, const bool& hasMinimize,
                                          const bool& hasMaximize, const bool& hasClose) = 0;
#endif

    /// \brief Returns the set of actionable buttons
    virtual QVector<QObject*> buttons() const = 0;
    
	/// \brief Enable/Disabled system menu - enabled by default.
    /// System menu appears if the Icon on the titlebar is clicked.
    virtual void setSystemMenuEnabled(const bool& enable) = 0;
	
	/// \brief Sets whether or not to show the style that appears when the
	/// window is active.
    virtual void setActiveWindowStyle(const bool& hasFocus) = 0;

    /// \brief Sets whether or not to show the tooltips.
    virtual void setToolTipVisible(const bool&) {}

signals:
	/// \brief Singals when the window should be closed
    void rejectWindow();
	/// \brief Singals when the minimize button is clicked
    void minimizeBtnClicked();
	/// \brief Win Only: Singals when the restore button is clicked
    void restoreBtnClicked();
	/// \brief Win Only: Singals when the maximize button is clicked
    void maximizeBtnClicked();
	/// \brief OSX Only: Singals when the zoom button is clicked
    void zoomBtnClicked();
	/// \brief Singals when the close button is clicked
    void closeBtnClicked();


protected:
    void paintEvent(QPaintEvent* event);
    bool event(QEvent* event);
    QWidget* getSelf() {return this;}
    QString& getElementStyleSheet();
    void prepForStyleReload();
    static QString mPrivateStyleSheet;
    QString mTitle;
    OriginWindow* mWindowPartner;
    bool mSystemMenuEnabled;
    QWidget *mCustomContent;

protected slots:
    /// \brief Called when the icon titlebar button is clicked.
    /// Shows the MS Window's icon menu. Calls showWindowsSystemMenu()
    virtual bool showSystemMenu(bool show = true, int xPos = -1, int yPos = -1) = 0;
};
}
}
#endif
