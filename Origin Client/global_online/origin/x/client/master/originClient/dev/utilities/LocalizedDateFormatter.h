///////////////////////////////////////////////////////////////////////////////
// LocalizedDateFormatter.h
//
// Copyright (c) 2012 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////

#ifndef _LOCALIZEDDATEFORMATTER_H
#define _LOCALIZEDDATEFORMATTER_H

#include <QDateTime>
#include <QString>
#include <QLocale>

#include "services/plugin/PluginAPI.h"

namespace Origin
{

namespace Client 
{

/// \brief Class for formatting dates and times based on EA's locale guidelines.
///
/// Fixed or standardized date formats should use Qt's builtin toString() functionality for the relevant type. This
/// class is only intended for format user-visible strings.
class ORIGIN_PLUGIN_API LocalizedDateFormatter
{
public:
	/// \brief Creates a new DateFormatter for the passed locale.
	/// \param locale  Locale to format for. Defaults to the current locale.
	LocalizedDateFormatter(const QLocale &locale = QLocale());

	/// \brief Enumeration to specify the type of date format required.
	enum FormatType
	{
		ShortFormat, ///< Short date format (eg MM/DD/YYYY in en-US).
		LongFormat, ///< Long date format (eg March 12, 2012 in en-US).
        LongFormatWithWeekday /// (eg Thursday February 14, 2013).
	};

	/// \brief Enumeration to specify the range of units used in interval strings.
    ///
	/// Smaller units should be numerically smaller
	enum IntervalUnit
	{
		Seconds = 0, ///< Seconds.
		Minutes = 1, ///< Minutes.
		Hours = 2, ///< Hours.
		Days = 3 ///< Days.
	};

	/// \brief Format the passed time to a localized string.
	/// \param time            Time to format.
	/// \param includeSeconds  True to include seconds in the string, false otherwise
	/// \return The formatted, localized time string.
	QString format(const QTime& time, bool includeSeconds = false) const;

	/// \brief Format the passed date to a localized string.
	/// \param date Date to format.
	/// \param type The desired format.
	/// \return The formatted, localized date string.
	QString format(const QDate& date, FormatType type) const;


	/// \brief Format the passed datetime to a localized string.
	/// \param datetime Date and time to format.
	/// \param type The desired format.
	/// \return The formatted, localized date and time string.
	QString format(const QDateTime& datetime, FormatType type) const;

	/// \brief Format the passed interval to a localized string.
	/// \param seconds Length of the interval in seconds.
	/// \param minimumUnit Smallest unit of time to use. Anything less than this will use the format "less than a (unit)".
    /// \param maximumUnit Largest unit of time to use.
	/// \param caps Should the return string be caps?
	/// \return The formatted, localized interval string.
	QString formatInterval(quint64 seconds, IntervalUnit minimumUnit = Hours, IntervalUnit maximumUnit = Days, const bool& caps = false);

    /// \brief Format the passed interval to a localized string. E.g. 12:12:12
    /// \param seconds Length of the interval in seconds.
    QString formatShortInterval(quint64 seconds);

    /// \brief Format the passed interval to a localized string. E.g. 12h 12s
    /// \param seconds Length of the interval in seconds.
    QString formatShortInterval_hs(quint64 seconds);

private:
    /// \brief Formats a short date.
    /// \param date The date to format.
    /// \return The formatted, localized date string.
	QString formatShortDate(const QDate &date) const;
    
    /// \brief Formats a long date.
    /// \param date The date to format.
    /// \return The formatted, localized date string.
	QString formatLongDate(const QDate &date) const;

	QLocale mLocale; ///< The user's locale.
};

}
}

#endif
