#include <QLocale>
#include "DateFormatProxy.h"
#include "LocalizedDateFormatter.h"
#include "utilities/LocalizedContentString.h"

namespace
{
    using namespace Origin::Client;

    LocalizedDateFormatter::IntervalUnit intervalUnitFromString(const QString &string, LocalizedDateFormatter::IntervalUnit defaultUnit)
    {
        if (string == "SECONDS")
        {
            return LocalizedDateFormatter::Seconds;
        }
        else if (string == "MINUTES")
        {
            return LocalizedDateFormatter::Minutes;
        }
        else if (string == "HOURS")
        {
            return LocalizedDateFormatter::Hours;
        }
        else if (string == "DAYS")
        {
            return LocalizedDateFormatter::Days;
        }

        return defaultUnit;
    }
}

namespace Origin
{
namespace Client
{
namespace JsInterface
{

QString DateFormatProxy::formatTime(const QDateTime &datetime, bool includeSeconds)
{
	return LocalizedDateFormatter().format(datetime.toLocalTime().time(), includeSeconds);
}

QString DateFormatProxy::formatShortDate(const QDateTime &datetime)
{
    QDateTime localDateTime = datetime.toLocalTime();
	return LocalizedDateFormatter().format(localDateTime.date(), LocalizedDateFormatter::ShortFormat);
}

QString DateFormatProxy::formatLongDate(const QDateTime &datetime)
{
    QDateTime localDateTime = datetime.toLocalTime();
    
	return LocalizedDateFormatter().format(localDateTime.date(), LocalizedDateFormatter::LongFormat);
}

QString DateFormatProxy::formatLongDateWithWeekday(const QDateTime &datetime)
{
    QDateTime localDateTime = datetime.toLocalTime();
    
	return LocalizedDateFormatter().format(localDateTime.date(), LocalizedDateFormatter::LongFormatWithWeekday);
}

QString DateFormatProxy::formatShortDateTime(const QDateTime &datetime)
{
	return LocalizedDateFormatter().format(datetime, LocalizedDateFormatter::ShortFormat);
}

QString DateFormatProxy::formatLongDateTime(const QDateTime &datetime)
{
	return LocalizedDateFormatter().format(datetime, LocalizedDateFormatter::LongFormat);
}

QString DateFormatProxy::formatLongDateTimeWithWeekday(const QDateTime &datetime)
{
	return LocalizedDateFormatter().format(datetime, LocalizedDateFormatter::LongFormatWithWeekday);
}

QString DateFormatProxy::formatInterval(quint64 seconds, const QString &minimumUnit, const QString &maximumUnit)
{
	return LocalizedDateFormatter().formatInterval(seconds, 
        intervalUnitFromString(minimumUnit, LocalizedDateFormatter::Hours), 
        intervalUnitFromString(maximumUnit, LocalizedDateFormatter::Days));
}

QString DateFormatProxy::formatShortInterval(quint64 seconds)
{
    return LocalizedDateFormatter().formatShortInterval(seconds);
}


QString DateFormatProxy::formatBytes(const qulonglong& bytes)
{
    return LocalizedContentString().formatBytes(bytes);
}

}
}
}
