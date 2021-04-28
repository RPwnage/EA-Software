#ifndef ORIGINTITLEBAROSX_H
#define ORIGINTITLEBAROSX_H

#include "origintitlebarbase.h"
#include "uitoolkitpluginapi.h"

class Ui_OriginTitlebarOSX;

namespace Origin {
namespace UIToolkit {

/// \brief Provides a user interface for Origin dialog scrollable widget.
/// Used in OriginWindow. Shouldn't be referenced directly.
class UITOOLKIT_PLUGIN_API OriginTitlebarOSX : public OriginTitlebarBase
{
    Q_OBJECT

public:
    /// \brief Constructor
    /// \param parent Pointer to the QWidget that is the parent of this titlebarr
    /// \param partner TBD.
	OriginTitlebarOSX(QWidget* parent = 0, OriginWindow* partner = 0);
    /// \brief Destructor
    ~OriginTitlebarOSX();

    void setEnvironmentLabel(const QString& lbl);
    void setTitleBarText(const QString& title);
    void setTitleBarContent(QWidget* content);
    void setupButtonFunctionality(const bool& hasIcon, const bool& hasMinimize, 
                                  const bool& hasMaximize, const bool& hasClose,
                                  const bool& hasFullScreen);
    QVector<QObject*> buttons() const;
    void setSystemMenuEnabled(const bool& enable);
    void setActiveWindowStyle(const bool& isActive);
    void setButtonContainerHoverStyle(const bool& isHovering);
    static bool isHoveringInButtonContainer(QWidget* w);

signals:
	/// \brief Signals when the window should be displayed in fullscreen mode
    void fullScreenBtnClicked();

protected:
    bool eventFilter(QObject* obj, QEvent* event);

protected slots:
    bool showSystemMenu(bool show = true, int xPos = -1, int yPos = -1);
    // HACK: For some reason the titlebar buttons don't want to remove their
    // hover state. Force the zoom to do it. I am forcing the other buttons in
    // the event filter.
    void removeZoomHover();


private:
    Ui_OriginTitlebarOSX* uiTitlebar;
};
}
}
#endif
