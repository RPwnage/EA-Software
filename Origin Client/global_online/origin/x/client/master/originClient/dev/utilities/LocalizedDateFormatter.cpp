#include <QtGlobal>

#include <QDebug>
#include "LocalizedDateFormatter.h"

namespace
{
    using namespace Origin::Client;
	struct UnitInformation
	{
		UnitInformation(LocalizedDateFormatter::IntervalUnit newUnit, quint64 newSeconds, const QByteArray &newName) :
			unit(newUnit),
			seconds(newSeconds),
			name(newName)
		{
		}

        LocalizedDateFormatter::IntervalUnit unit;
		quint64 seconds;
		QByteArray name;
	};
}

namespace Origin
{

namespace Client
{

LocalizedDateFormatter::LocalizedDateFormatter(const QLocale &locale) : mLocale(locale)
{
}

QString LocalizedDateFormatter::format(const QDateTime& datetime, FormatType type) const
{
	QDateTime localDateTime = datetime.toLocalTime();
	return format(localDateTime.date(), type) + " " + format(localDateTime.time());
}

QString LocalizedDateFormatter::format(const QTime& time, bool includeSeconds) const
{
	switch(mLocale.country())
	{
	case QLocale::UnitedStates:
		return time.toString(includeSeconds ? "h:mm:ss AP" : "h:mm AP");

	case QLocale::Denmark:
	case QLocale::Sweden:
		return time.toString(includeSeconds ? "HH.mm.ss" : "HH.mm");

	case QLocale::Hungary:
		return time.toString(includeSeconds ? "H.mm.ss" : "H.mm");

	case QLocale::UnitedKingdom:
	case QLocale::France:
	case QLocale::Germany:
	case QLocale::Italy:
	case QLocale::Norway:
	case QLocale::Poland:
		return time.toString(includeSeconds ? "HH:mm:ss" : "HH:mm");

	case QLocale::Netherlands:
	case QLocale::Brazil:
	case QLocale::Portugal:
	case QLocale::RussianFederation:
	case QLocale::Spain:
	case QLocale::Mexico:
	case QLocale::Finland:
	case QLocale::RepublicOfKorea:
	case QLocale::Japan:
	case QLocale::China:
	case QLocale::Taiwan:
	case QLocale::Greece:
    default:
        return time.toString(includeSeconds ? "H:mm:ss" : "H:mm");
	}
}

QString LocalizedDateFormatter::format(const QDate& date, FormatType type) const
{
	if (type == ShortFormat)
	{
		return formatShortDate(date);
	}
	else if (type == LongFormat)
	{
		return formatLongDate(date);
	}
    else // if (type == LongFormatWithWeekday)
    {
        return QString("%1 %2").arg(mLocale.dayName(date.dayOfWeek(), QLocale::LongFormat)).arg(formatLongDate(date));
    }
}

QString LocalizedDateFormatter::formatShortDate(const QDate& date) const
{
	switch(mLocale.country())
	{
	case QLocale::UnitedStates:
	case QLocale::China:
		return date.toString("MM/dd/yy");

	case QLocale::CzechRepublic:
	case QLocale::Slovakia:
	case QLocale::Netherlands:
	case QLocale::Denmark:
	case QLocale::Finland:				
	case QLocale::Germany:
	case QLocale::Norway:
	case QLocale::Poland:
	case QLocale::RussianFederation:
		return date.toString("dd.MM.yyyy");
	
	case QLocale::Sweden:
		return date.toString("yyyy-MM-dd");

	case QLocale::Hungary:				
	case QLocale::Japan:				
	case QLocale::RepublicOfKorea:
	case QLocale::Taiwan:
		return date.toString("yyyy/MM/dd");

	default:
		return date.toString("dd/MM/yyyy");
	}
}

QString LocalizedDateFormatter::formatLongDate(const QDate& date) const
{
	QString month = mLocale.monthName(date.month(), QLocale::LongFormat);

	switch(mLocale.country())
	{
	case QLocale::Brazil:		
	case QLocale::Portugal:
		return date.toString("d 'de' %1 'de' yyyy").arg(month);

	case QLocale::Spain:			
	case QLocale::Mexico:		
		return date.toString("dd 'de' %1 'de' yyyy").arg(month);

    case QLocale::Netherlands:
    case QLocale::Thailand:
		return date.toString("d %1 yyyy").arg(month);

	case QLocale::Germany:				
	case QLocale::Norway:				
	case QLocale::Denmark:				
		return date.toString("d'.' %1 yyyy").arg(month);

	case QLocale::Finland:				
		return date.toString("d'.' %1 yyyy").arg(month);

	case QLocale::Sweden:				
		return date.toString("d %1 yyyy").arg(month);

	case QLocale::Italy:				
	case QLocale::Greece:					
		return date.toString("dd %1 yyyy").arg(month);

	case QLocale::France:
		return date.toString("d%1 %2 yyyy").arg((date.day() == 1) ? "er": "").arg(month);

	case QLocale::RussianFederation:
	case QLocale::Poland:	
		return date.toString("d  %1  yyyy 'r.'").arg(month);

	case QLocale::Hungary:				
		return date.toString("yyyy'.' %1 d'.'").arg(month);

	case QLocale::CzechRepublic:
	case QLocale::Slovakia:
		return date.toString("d'.' %1 yyyy").arg(month);
		
	case QLocale::Taiwan:
	case QLocale::China:
	case QLocale::RepublicOfKorea:
    case QLocale::Japan:
        // Chinese and Korean appear to prefer purely numeric long dates
		return date.toString("yyyy/MM/dd");

	default:
		return date.toString("%1 d',' yyyy").arg(month);
	}
}
	
QString LocalizedDateFormatter::formatInterval(quint64 seconds, IntervalUnit minimumUnit, IntervalUnit maximumUnit, const bool& caps)
{
	QList<UnitInformation> allUnits;

	// These are all the units we know about
	// This should be in descending size order
	allUnits.append(UnitInformation(Days, 24 * 60 * 60, "day"));
	allUnits.append(UnitInformation(Hours, 60 * 60, "hour"));
	allUnits.append(UnitInformation(Minutes, 60, "minute"));
	allUnits.append(UnitInformation(Seconds, 1, "second"));

	// These are the units the caller has specified they want
	QList<UnitInformation> availableUnits;
	for(QList<UnitInformation>::ConstIterator it = allUnits.constBegin();
		it != allUnits.constEnd();
		it++)
	{
		if ((it->unit > maximumUnit) ||
			(it->unit < minimumUnit))
		{
			// Not an available unit
			continue;
		}

		availableUnits.append(*it);
	}

	if (availableUnits.isEmpty())
	{
		// Nope
		return QString();
	}

	for(QList<UnitInformation>::ConstIterator it = availableUnits.constBegin();
		it != availableUnits.constEnd();
		it++)
	{
		const UnitInformation &unit = *it;

		if (seconds >= unit.seconds)
		{
			// Use this unit
			int roundedUnits = qRound(double(seconds) / it->seconds);
			QByteArray stringName;

			if (roundedUnits == 1)
			{
				stringName = "ebisu_client_interval_" + it->name + "_singular";
			}
			else
			{
				stringName = "ebisu_client_interval_" + it->name + "_plural";
			}

            if(caps)
            {
                stringName += "_caps";
            }

			return QObject::tr(stringName).arg(roundedUnits);
		}
	}

	const UnitInformation &smallestUnit = availableUnits.last();
	QByteArray stringName("ebisu_client_interval_less_than_" + smallestUnit.name + (caps ? "_caps" : ""));

	return QObject::tr(stringName);
}

QString LocalizedDateFormatter::formatShortInterval(quint64 seconds)
{
    QLocale locale;   // Use the default locale, which should be set during app startup and if the locale changes at runtime.
    QString time;

    int hour = seconds / 3600;
    int minute = (seconds % 3600) / 60;
    int second = seconds % 60;

    switch(locale.country())
    {
    case QLocale::Denmark:
        time.sprintf("%.2u.%.2u.%.2u", hour, minute, second);
        break;

    case QLocale::UnitedKingdom:
    case QLocale::France:
    case QLocale::Sweden:
    case QLocale::Italy:
    case QLocale::Germany:
    case QLocale::Norway:
    case QLocale::Poland:
        time.sprintf("%.2u:%.2u:%.2u", hour, minute, second);
        break;

    default:
        time.sprintf("%u:%.2u:%.2u", hour, minute, second);
    }

    return time;
}

QString LocalizedDateFormatter::formatShortInterval_hs(quint64 seconds)
{
    QString time;
    const int hour = seconds / 3600;
    const int minute = (seconds % 3600) / 60;
    return QObject::tr("ebisu_x_hours_minutes").arg(hour).arg(minute);
}

}
}
