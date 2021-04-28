#ifndef _DATEFORMATPROXY_H
#define _DATEFORMATPROXY_H

/**********************************************************************************************************
 * This class is part of Origin's JavaScript bindings and is not intended for use from C++
 *
 * All changes to this class should be reflected in the documentation in jsinterface/doc
 *
 * See http://developer.origin.com/documentation/display/EBI/Working+with+Web+Widgets for more information
 * ********************************************************************************************************/

#include <QDateTime>
#include <QObject>
#include <QString>

#include "services/plugin/PluginAPI.h"

namespace Origin
{
namespace Client
{
namespace JsInterface
{

class ORIGIN_PLUGIN_API DateFormatProxy : public QObject
{
	Q_OBJECT
public:
	// JS doesn't distinguish between dates, times and datetimes so we have to use this instead of overloads
	Q_INVOKABLE QString formatTime(const QDateTime &, bool includeSeconds = false);

	Q_INVOKABLE QString formatShortDateTime(const QDateTime &);
	Q_INVOKABLE QString formatLongDateTime(const QDateTime &);
    Q_INVOKABLE QString formatLongDateTimeWithWeekday(const QDateTime &);
	
	Q_INVOKABLE QString formatLongDate(const QDateTime &);
    Q_INVOKABLE QString formatLongDateWithWeekday(const QDateTime &);
	Q_INVOKABLE QString formatShortDate(const QDateTime &);
	
    Q_INVOKABLE QString formatInterval(quint64 seconds, const QString &minimumUnit = QString(), const QString &maximumUnit = QString());
	Q_INVOKABLE QString formatShortInterval(quint64 seconds);

    /// \brief Give the JS a helper to use a locaized byte format. Some locales use , instead of .
    /// note: this function shouldn't be here. It should be in a more generic proxy. We don't have something like that atm.
    Q_INVOKABLE QString formatBytes(const qulonglong& bytes);
};

}
}
}

#endif
