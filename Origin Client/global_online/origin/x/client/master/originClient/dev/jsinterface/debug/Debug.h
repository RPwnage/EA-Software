// Copyright 2014 Electronic Arts Inc.

#ifndef DEBUG_H
#define DEBUG_H 1

#include <QObject>

namespace Origin
{
namespace Client
{
namespace JsInterface
{

/// \brief Javascript bridge object for the Origin client log.
///
/// The object gets registered as "originConsole" with the Javascript runtime.
/// Sample usage:
///
///     originConsole.log_debug('*** smoot, id=' + id);
///
class Console : public QObject
{
    Q_OBJECT

public:

    Q_INVOKABLE void log_action(const QString& msg) const;
    Q_INVOKABLE void log_debug(const QString& msg) const;
    Q_INVOKABLE void log_error(const QString& msg) const;
    Q_INVOKABLE void log_event(const QString& msg) const;
    Q_INVOKABLE void log_warning(const QString& msg) const;
};


}
}
}

#endif // DEBUG_H
