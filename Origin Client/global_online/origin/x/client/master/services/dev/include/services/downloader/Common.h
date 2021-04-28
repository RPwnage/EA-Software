#ifndef COMMON_H
#define COMMON_H

#include "services/debug/DebugService.h"
#include "services/plugin/PluginAPI.h"

#include <QObject>
#include <QMetaObject>
#include <QThread>
#include <QString>

#ifdef DEBUG
#define ASYNC_INVOKE_GUARD \
    if (QThread::currentThread() != this->thread()) \
    { \
        ORIGIN_ASSERT(QMetaObject::invokeMethod(this, Origin::_stripNamespaces(__FUNCTION__), Qt::QueuedConnection)); \
        return; \
    }

#define ASYNC_INVOKE_GUARD_ARGS(...) \
    if (QThread::currentThread() != this->thread()) \
    { \
        ORIGIN_ASSERT(QMetaObject::invokeMethod(this, Origin::_stripNamespaces(__FUNCTION__), Qt::QueuedConnection, __VA_ARGS__)); \
        return; \
    }

#else
#define ASYNC_INVOKE_GUARD \
    if (QThread::currentThread() != this->thread()) \
    { \
        QMetaObject::invokeMethod(this, Origin::_stripNamespaces(__FUNCTION__), Qt::QueuedConnection); \
        return; \
    }

#define ASYNC_INVOKE_GUARD_ARGS(...) \
    if (QThread::currentThread() != this->thread()) \
    { \
        QMetaObject::invokeMethod(this, Origin::_stripNamespaces(__FUNCTION__), Qt::QueuedConnection, __VA_ARGS__); \
        return; \
    }
#endif

namespace Origin
{
    ORIGIN_PLUGIN_API const char * _stripNamespaces(const char * string);
}

#endif // COMMON_H
