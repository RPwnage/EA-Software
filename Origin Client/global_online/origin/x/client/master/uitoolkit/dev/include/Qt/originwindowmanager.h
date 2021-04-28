#ifndef ORIGINWINDOWMANAGER_H
#define ORIGINWINDOWMANAGER_H

class QWidget;
class QEvent;
class QMouseEvent;

#include <QObject>
#include <QMargins>
#include <QSize>
#include <QPoint>
#include <QRect>
#include "originpushbutton.h"
#include "uitoolkitpluginapi.h"

namespace Origin {
    namespace UIToolkit {
/// \brief Controller of a window object.
class UITOOLKIT_PLUGIN_API OriginWindowManager : public QObject
{
    Q_OBJECT

public:
    /// \brief Enum of functionality that the object will be able to do
    enum Functionality
    {
          NoFunction = 0
        , Movable = 1
        , Resizable = 2
    };
    Q_DECLARE_FLAGS(Functionalities, Functionality)

    /// \brief Constructor
    /// \param managed Pointer to the (will be) managed object
    /// \param function The functionality that the object will be able to do (e.g. Move, Resize)
    /// \param borderwidths The border of the managed object. Used for resizing.
    /// \param titleBar The titlebar of the managed object.
    OriginWindowManager(QWidget* managed, const Functionalities& function, 
                       const QMargins& borderwidths, QWidget* titleBar = NULL);
    /// \brief Destructor
    ~OriginWindowManager();

    /// \brief Sets a pointer to the titlebar of the managed object
    /// \param titleBar The titlebar of the managed object
    void setTitleBar(QWidget* titleBar);
    /// \return TitleBar The titlebar of the managed object.
    QWidget* titleBar() const {return mTitleBar;}

    /// \brief Sets the individual borders to the specified width
    /// \param size - Uniform size of the border.
    void setBorderWidth(const unsigned int& size);
    /// \brief Sets the individual borders to the specified width
    /// \param margins - Specified width of the borders
    void setBorderWidths(const QMargins& margins) {mBorderWidths = margins;}
    /// \brief Returns the border width of the managed object
    const QMargins& borderWidths() const {return mBorderWidths;}

    /// \brief Sets the managed object
    /// \param widget - Pointer to the (will be) managed object
    void setManagedWidget(QWidget* widget);

    /// \return The QWidget being managed 
    QWidget* managedWidget() { return mManaged; }
    
    /// \brief Sets the functionalities of the managed object
    /// \param functionalities - the functionality that the object will be able to do (e.g. Move, Resize)
    void setManagedFunctionalities(const Functionalities& functionalities) {mFunctionality = functionalities;}
    /// \brief Returns if the managed object is maximized.
    const bool isManagedMaximized() {return mState & Maximized;}
    /// \brief Call from the managed object when the object is to be maximized
    void onManagedMaximized();
    /// \brief Call from the managed object when the object is to be minimized
    void onManagedMinimized();
    /// \brief Call from the managed object when the object is to be restored/normal
    void onManagedRestored();
    /// \brief OSX - Call from the managed object when the object is to be zoomed
    void onManagedZoomed();
    /// \brief Returns if the managed object is zommed.
    const bool isManagedZoomed() {return mState & Zoomed;}
    /// \brief Centers the window on the current screen it's on.
    /// screenNumber = -1 means the window should center on the screen it's currently on.
    void centerWindow(const int& screenNumber = -1);

    /// 
    /// have the windowmanager setup the button focus. Should be called once it has all the widgets.
    /// 
    void setupButtonFocus();

    /// should be done if a new default button has been set. Assumes that setupButtonFocus
    /// has already been called
    void resetDefaultButton(OriginPushButton* defaultBtn);

    /// \brief Set up list of Resizable Widgets.
    /// on Mavericks we are getting the inner and outer frames in our resize event
    /// \param widget - Pointer to the resizable object
    void addResizableItem(QWidget*);
    
public slots:
    void buttonFocusIn(OriginPushButton*);
    void buttonFocusOut(OriginPushButton*);
    /// \brief Adjusts the managed window so it would be on screen
    void ensureOnScreen();
    void ensureOnScreen(bool forceUpdate);

signals:
    /// \brief Emitted when the window should zoom. Used for a window to customize
    /// the size that it will be zoomed. Called from inside showZoomed()
    void customizeZoomedSize();

protected:
    enum ManagedState
    {
          NoState = 0x0
        , Resizing = 0x01
        , MouseDown = 0x02
        , Maximized = 0x04
        , Zoomed = 0x08
    };

    Q_DECLARE_FLAGS(ManagedStates, ManagedState)

    bool eventFilter(QObject* obj, QEvent* event);
    void mousePressEvent(QMouseEvent*);
    void mouseMoveEvent(QMouseEvent* event, const QPoint& offset=QPoint(0,0));
    void mouseMoveToResizeEvent(QMouseEvent*);
    void mouseReleaseEvent(QMouseEvent*);


private:
    void borderHitTest(const QPoint& mousePox, bool& top, bool& right, bool& bottom, bool& left);
    /**
     * whether or not you hit the border and which side you hit. 
     *
     *  returns true if it hit a border, false if it didn't
     */
    bool hitBorder(const QPoint& mousePos, bool& top, bool& right, bool& bottom, bool& left);
    
    QRect totalWorkspaceGeometry();
#ifdef Q_OS_MAC
    QRect totalRelevantWorkspaceGeometry();
#endif
    bool isTitlebarOnscreen();

    void saveCurrentGeometry();
    
    bool isManaged(QWidget*);

    QMargins mBorderWidths;
    QPoint mClickPos;

    QWidget* mManaged;
    QWidget* mTitleBar;

    ManagedStates mState;
    Functionalities mFunctionality;
    QRect mPreMinMaxGeometry;
    QRect mPreZoomGeometry;
#ifdef Q_OS_MAC
    QRect mScreenAvailGeometryCache;
#endif
    bool mInMove;
    bool mHasMoved;

    // Hack - to get the window to not move when we are
    // changing the left or top during while it's minimum size
    QPoint mBottomRightPos;

    Origin::UIToolkit::OriginPushButton* mDefaultButton;
    Origin::UIToolkit::OriginPushButton* mCurrentFocusButton;
    QList<QWidget*> mResizableObjects;
};
}
}


#endif
