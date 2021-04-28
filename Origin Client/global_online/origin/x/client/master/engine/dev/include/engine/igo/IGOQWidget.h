///////////////////////////////////////////////////////////////////////////////
// IGOQWidget.h
// 
// Created by Thomas Bruckschlegel
// Copyright (c) 2012 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#ifndef IGOQWIDGET_H
#define IGOQWIDGET_H

#include <QPointer>
#include <QWidget>

#include "IIGOWindowManager.h"

#include "engine/igo/IIGOCommandController.h"
#include "services/plugin/PluginAPI.h"

namespace Origin
{
namespace Engine
{

class ORIGIN_PLUGIN_API IGOQWidget : public QWidget
{
    Q_OBJECT

public:

    IGOQWidget(QWidget *parent = 0);
    virtual ~IGOQWidget(void);

    virtual bool eventFilter(QObject *obj, QEvent *event);

    virtual bool isAlwaysVisible() { return false; };
    virtual uint32_t layout() { return IIGOWindowManager::WINDOW_ALIGN_NONE; }
    virtual bool canBringToFront() { return true; }

    void storeRealParent(QWidget *parent);
    QWidget *getRealParent() { return internalParentReference.data(); };

    QSize sizeHint() const; 

// Disable optimisations, so that our little squish helper is not optimized away!
#ifdef _MSC_VER
    #pragma optimize("", off)
#endif
    Q_PROPERTY( bool isVisibleInIGO READ isVisibleInIGO WRITE setVisibleInIGO )

    // Notify whether IGO is visible - this is different from mVisibleInIGO, which specifies
    // whether the control is shown at all
    virtual void setIGOVisible(bool visible) { mIGOVisible = visible; }
    bool isIGOVisible() const { return mIGOVisible; }
    
public slots:

    void setVisibleInIGO(bool s) { mVisibleInIGO = s; }    // a helper for squish

public:

    bool isVisibleInIGO() const { return mVisibleInIGO; };    // a helper for squish

#ifdef _MSC_VER
    #pragma optimize("", on) // See comments above regarding this optimization change.
#endif

    void allowClose(bool close) { mAllowClose = close; }
    void allowMovable(bool movable) { mAllowMovable = movable; }

signals:

    // List of valid commands
    // "NewBrowser"
    // "ShowFriends"
    // "CloseIGO"

    void igoCommand(int cmd, Origin::Engine::IIGOCommandController::CallOrigin callOrigin);
    void igoCloseWidget(bool);
    void igoDirtyWindow();
    void igoMove(const QPoint &pt);
    void igoResize(const QSize &);
    void igoDestroy();
    void igoUpdateWindow(uint);
    void igoOpacity(qreal);

private:
    bool mAllowClose;
    bool mAllowMovable;
    bool mIGOVisible;
    QPointer<QWidget> internalParentReference;
    QSize mHintSize;

public: // a helper for squish

    bool mVisibleInIGO;
    void setSizeHint(QSize s);
};

} // Engine
} // Origin

#endif
