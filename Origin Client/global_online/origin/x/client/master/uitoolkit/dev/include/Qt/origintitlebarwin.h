#ifndef ORIGINTITLEBARWIN_H
#define ORIGINTITLEBARWIN_H

#include "origintitlebarbase.h"
#include "uitoolkitpluginapi.h"

class QPushButton;
class Ui_OriginTitlebarWin;

namespace Origin {
namespace UIToolkit {

class OriginWindow;

/// \brief Provides a user interface for Origin dialog scrollable widget.
/// Used in OriginWindow. Shouldn't be referenced directly.
class UITOOLKIT_PLUGIN_API OriginTitlebarWin : public OriginTitlebarBase
{
    Q_OBJECT

public:
    /// \brief Constructor
    /// \param parent Pointer to the QWidget that is the parent of this titlebarr
    /// \param partner TBD.
	OriginTitlebarWin(QWidget* parent = 0, OriginWindow* partner = 0);
    /// \brief Destructor
    ~OriginTitlebarWin();

    void showMaximized();
    void showNormal();
    void setMenuBarVisible(const bool& visible);
    QLayout* menuBarLayout();
    void setSystemMenuEnabled(const bool& enable);
    void setEnvironmentLabel(const QString& lbl);
    void setTitleBarText(const QString& title);
    void setTitleBarContent(QWidget* content);
    bool isRestoreButtonVisible();
    // Maybe we shouldn't do it like this. I'll ask for feedback later.
    void setupButtonFunctionality(const bool& hasIcon, const bool& hasMinimize, 
                                  const bool& hasMaximize, const bool& hasClose);
    QVector<QObject*> buttons() const;
    void setActiveWindowStyle(const bool& isActive);
    /// \brief Sets whether or not to show the tooltips. Is on by default.
    void setToolTipVisible(const bool&);

#if defined(Q_OS_WIN)
    bool showWindowsSystemMenu(long *result, int xPos = -1, int yPos = -1);
#endif


protected:
    bool eventFilter(QObject* obj, QEvent* event);

protected slots:
    /// \brief Called when the icon titlebar button is clicked.
    /// Shows the MS Window's icon menu. Calls showWindowsSystemMenu()
    bool showSystemMenu(bool show = true, int xPos = -1, int yPos = -1);


private:
    Ui_OriginTitlebarWin* uiTitlebar;
};
}
}
#endif