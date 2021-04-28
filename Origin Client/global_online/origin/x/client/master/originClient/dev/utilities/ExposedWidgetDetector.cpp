#include "ExposedWidgetDetector.h"

#include <QEvent>
#include <QWidget>
#include <QAbstractScrollArea>

namespace Origin
{
namespace Client
{

ExposedWidgetDetector::ExposedWidgetDetector(QWidget *watched) :
	QObject(watched),
    mPaintWidget(watched),
    mVisibilityWidget(watched),
    // We don't know if we've been painted yet so assume we have
    mExposed(true)
{
    init();
}

ExposedWidgetDetector::ExposedWidgetDetector(QAbstractScrollArea *watched) :
	QObject(watched),
    // The viewport actually receives the paint events
    mPaintWidget(watched->viewport()),
    mVisibilityWidget(watched),
    mExposed(true)
{
    init();
}

void ExposedWidgetDetector::init()
{ 
    // Keep track of our window
    mWindow = mVisibilityWidget->window();

    // If we're hidden right now clear our exposed flag
    if (isHiddenOrMinimized())
    {
        setExposed(false);
    }

    // Filter paint events to the view
    mPaintWidget->installEventFilter(this);

    // Track visibility changes to the actual widget
    mVisibilityWidget->installEventFilter(this);

    // Track visibility, state changes to the window
    mWindow->installEventFilter(this);
}

bool ExposedWidgetDetector::isExposed() const
{
    return mExposed;
}
    
void ExposedWidgetDetector::setExposed(bool nowExposed)
{
    // Don't fire needlessly
    if (nowExposed != mExposed)
    {
        mExposed = nowExposed;
        emit exposureChanged(nowExposed);

        if (nowExposed)
        {
            emit exposed();
        }
        else
        {
            emit unexposed();
        }
    }
}
    
bool ExposedWidgetDetector::isHiddenOrMinimized() const
{
    return (!mVisibilityWidget->isVisible() || mVisibilityWidget->window()->isMinimized());
}

bool ExposedWidgetDetector::eventFilter(QObject *target, QEvent *e)
{
    // Watch window state changes for both the widget and its target
    if (e->type() == QEvent::Hide ||
         e->type() == QEvent::WindowStateChange)
    {
        if (isHiddenOrMinimized())
        {
            setExposed(false);
        }
    }
    else if ((target == mPaintWidget) &&
             (e->type() == QEvent::Paint || e->type() == QEvent::Show))
    {
        // If we've been asked to paint then mark us as exposed
        if (!isHiddenOrMinimized())
        {
            setExposed(true);
        }
    }
    else if (e->type() == QEvent::ParentChange)
    {
        // Track changes to our parent window
        // This will track both us being reparented and our window being reparented
        QWidget *currentWindow = mVisibilityWidget->window();

        if (currentWindow != mWindow)
        {
            // Kill the old event filter
            mWindow->removeEventFilter(this);
            
            currentWindow->installEventFilter(this);
            mWindow = currentWindow; 
        }
    }

    return QObject::eventFilter(target, e);
}

}
}
