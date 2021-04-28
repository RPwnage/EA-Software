///////////////////////////////////////////////////////////////////////////////
// IGOQWidget.cpp
// 
// Created by Thomas Bruckschlegel
// Copyright (c) 2012 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#include "IGOQWidget.h"

#include <qevent.h>

#include "engine/igo/IGOController.h"
#include "services/log/LogService.h"


namespace Origin
{
namespace Engine
{

// Disable optimisations, so that our little squish helper is not optimized away!
#ifdef _MSC_VER
    #pragma optimize("", off) 
#endif

IGOQWidget::IGOQWidget(QWidget *parent) :
    QWidget(parent),
    mAllowClose(false),
    mAllowMovable(false),
    mIGOVisible(false),
    internalParentReference(0),
    mHintSize(-1,-1),
    mVisibleInIGO(false)
{
    if (IGOController::instance()->extraLogging())
        ORIGIN_LOG_ACTION << "IGOQWidget - NEW = " << this;
}

#ifdef _MSC_VER
    #pragma optimize("", on) // See comments above regarding this optimization change.
#endif

IGOQWidget::~IGOQWidget()
{
    if (IGOController::instance()->extraLogging())
        ORIGIN_LOG_ACTION << "IGOQWidget - DELETE = " << this << ", RealParent = " << getRealParent();

    storeRealParent(NULL);
}

bool IGOQWidget::eventFilter(QObject *obj, QEvent *event)
{
    QEvent::Type type = event->type();

    switch (type)
    {
    case QEvent::Destroy:
        {
            if (IGOController::instance()->extraLogging())
                ORIGIN_LOG_ACTION << "IGOQWidget - DESTROY MSG = " << this << ", RealParent = " << getRealParent();

            // Here we don't want anybody (ie the IGOQtContainer) to access the real parent anymore since it was deleted - And we don't want to do anything
            // directly with it either (ie don't call storeRealParent(NULL)
            internalParentReference = NULL;
            emit igoDestroy();
        }
        break;

    case QEvent::DeferredDelete:    // We want to delete the widget wrapper/proxy BEFORE the widget destructor is called. Otherwise we may be trying to access
                                    // obsolete objects (for example when focusing out of items) when deleting the wrapper on the 'Destroy' signal
                                    // (at which point some of the wrapped widget hierarchy destructors may have been called)
                                    // Calling deleteLater is your friend here!
        {
            if (IGOController::instance()->extraLogging())
                ORIGIN_LOG_ACTION << "IGOQWidget - DEFERRED DELETE MSG = " << this << ", RealParent = " << getRealParent();

            storeRealParent(NULL);
            emit igoDestroy();
        }
        break;

    case QEvent::Close:
        {
            if (IGOController::instance()->extraLogging())
                ORIGIN_LOG_ACTION << "IGOQWidget - CLOSE MSG = " << this << ", AllowClose = " << mAllowClose;

            emit igoCloseWidget(mAllowClose);

            if (!mAllowClose)
            {
                event->ignore();
                return true;
            }
        }
        break;

    case QEvent::DragEnter:
    case QEvent::DragLeave:
        {
            event->ignore();
            return true;
        }
        break;

    case QEvent::Move:
        {
            // don't allow move
            QMoveEvent *moveEvent = (QMoveEvent *)event;
            QPoint pt = moveEvent->pos();
            if (pt != QPoint(0,0))
            {
                QWidget *widget = (QWidget *)obj;
                widget->move(0,0);
                event->ignore();
                return true;
            }
        }
        break;
    
    default:
        break;
    }

    return QWidget::eventFilter(obj, event);
}

void IGOQWidget::setSizeHint(QSize s) 
{
    mHintSize = s;
}

QSize IGOQWidget::sizeHint() const 
{
    if (!mHintSize.isValid())
        return QWidget::sizeHint();

    return mHintSize;
}
    
void IGOQWidget::storeRealParent(QWidget *parent)
{
    if (IGOController::instance()->extraLogging())
        ORIGIN_LOG_ACTION << "IGOQWidget - STORE REAL PARENT = " << parent << ", previous = " << internalParentReference;

    if (internalParentReference != parent)
    {
        if (internalParentReference)
        {
            internalParentReference->removeEventFilter(this);
            if (internalParentReference->parent() == this)
                internalParentReference->setParent(NULL);
        }
        
        internalParentReference = parent;
    }
}
    
} // Engine
} // Origin

