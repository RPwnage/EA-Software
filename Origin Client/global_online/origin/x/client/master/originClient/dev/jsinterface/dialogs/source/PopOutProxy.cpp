/////////////////////////////////////////////////////////////////////////////
// PopOutProxy.cpp
//
// Copyright (c) 2015, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////
#include "PopOutProxy.h"
#include <QMetaObject>
#include <QUuid>
#include "services/debug/DebugService.h"
#include "services/log/LogService.h"
#include "PopOutController.h"

namespace Origin
{
namespace Client
{
namespace JsInterface
{


PopOutProxy::PopOutProxy(QObject* parent)
    : QObject(parent)
{
    ORIGIN_VERIFY_CONNECT(
        PopOutController::instance(), SIGNAL(closed(QString)), 
        this, SIGNAL(popOutClosed(QString))
        );
    ORIGIN_VERIFY_CONNECT(
        this, SIGNAL(popInWindow(QString)),
        PopOutController::instance(), SLOT(onPopInWindow(QString))
        );
}
}
}
}