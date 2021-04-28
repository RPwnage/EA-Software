///////////////////////////////////////////////////////////////////////////////
// DateTime.h
//
// Copyright (c) 2004 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#ifndef DATE_TIME_H
#define DATE_TIME_H

#include <QDate>
#include <QString>
#include <QTime>

namespace Origin
{
namespace Downloader
{
/// \brief The minimum valid year.
const int DATE_MIN_YEAR = 1900;

/// \brief The maximum valid hour.
const int DATE_MAX_HOUR = 24;

/// \brief The maximum valid minute.
const int DATE_MAX_MINUTE = 59;

/// \brief The maximum valid second.
const int DATE_MAX_SECOND = 59;

/// \ingroup Date
/// \brief Enumeration of errors that may occur when setting the date or time.
enum DateError
{
    DATE_SUCCESS = 0, ///< The date or time was set successfully.
    DATE_FAIL = -1, ///< The date or time was not set for an unspecified reason.
    DATE_ERR_MONTH_OUT_OF_RANGE = -2, ///< The date was not set because the month was not valid.
    DATE_ERR_DAY_OUT_OF_RANGE = -3, ///< The date was not set because the day was not valid.
    DATE_ERR_YEAR_OUT_OF_RANGE = -4, ///< The date was not set because the year was not valid.
    DATE_ERR_HOUR_OUT_OF_RANGE = -5, ///< The time was not set because the hour was not valid.
    DATE_ERR_MINUTE_OUT_OF_RANGE = -6, ///< The time was not set because the minute was not valid.
    DATE_ERR_SECOND_OUT_OF_RANGE = -7 ///< The time was not set because the second was not valid.
};

/// \brief Enumeration of months in the year.
enum DateMonth
{
	DATE_TIME_MONTH_JAN = 1, ///< January
	DATE_TIME_MONTH_FEB, ///< February
	DATE_TIME_MONTH_MAR, ///< March
	DATE_TIME_MONTH_APR, ///< April
	DATE_TIME_MONTH_MAY, ///< May
	DATE_TIME_MONTH_JUN, ///< June
	DATE_TIME_MONTH_JUL, ///< July
	DATE_TIME_MONTH_AUG, ///< August
	DATE_TIME_MONTH_SEP, ///< September
	DATE_TIME_MONTH_OCT, ///< October
	DATE_TIME_MONTH_NOV, ///< November
	DATE_TIME_MONTH_DEC  ///< December

};


// forward decl
class DateTimeRange;

/// \brief Holds information about a specific date.
/// \note Unless specified, all values are in UTC.
class Date
{

public:
    /// \brief The Date constructor.
	Date();

	/// \brief Sets the date.
    /// \param month Downloader::DateMonth value representing the month.
    /// \param day The day.  Must be a valid day for the given month in the given year.
    /// \param year The year.  Must be greater than Downloader::DATE_MIN_YEAR.
    /// \return DATE_SUCCESS if the date is successfully set, otherwise a DateError error code.
	int SetDate(DateMonth month, int day, int year);

	/// \brief Decodes and sets a date in a string formatted as YYYY-MM-DD (1999-05-31).
    /// \param sDate The string representing the date.
    /// \return DATE_SUCCESS if the date is successfully set, otherwise a DateError error code.
	int SetDate(const QString sDate);

    /// \brief Gets the month.
    /// \return The DateMonth value representing the month.
	DateMonth GetMonth() const
	{
		return mMonth;
	}
    
    /// \brief Gets the day.
    /// \return An integer representing the day.
	int GetDay() const
	{
		return mDay;
	}
    
    /// \brief Gets the year.
    /// \return An integer representing the year.
	int GetYear() const
	{
		return mYear;
	}
    
    /// \brief Overridden "==" operator.
    /// \param leftDate The first Date object to compare.
    /// \param rightDate The second Date object to compare.
    /// \return True if the stored date and the given date are equal.
	friend bool operator==(const Date& leftDate, const Date& rightDate);
    
    /// \brief Overridden "<" operator.
    /// \param leftDate The first Date object to compare.
    /// \param rightDate The second Date object to compare.
    /// \return True if the stored date occurs before the given date.
	friend bool operator<( const Date& leftDate, const Date& rightDate);
    
    /// \brief Overridden ">" operator.
    /// \param leftDate The first Date object to compare.
    /// \param rightDate The second Date object to compare.
    /// \return True if the stored date occurs after the given date.
	friend bool operator>( const Date& leftDate, const Date& rightDate);

    /// \brief Overridden "!=" operator.
    /// \param leftDate The first Date object to compare.
    /// \param rightDate The second Date object to compare.
    /// \return True if the stored date and the given date are NOT equal.
    friend bool operator!=( const Date& leftDate, const Date& rightDate);
    
    /// \brief Overridden ">=" operator.
    /// \param leftDate The first Date object to compare.
    /// \param rightDate The second Date object to compare.
    /// \return True if the stored date occurs after the given date or they are equal.
	friend bool operator>=( const Date& leftDate, const Date& rightDate);
    
    /// \brief Overridden "<=" operator.
    /// \param leftDate The first Date object to compare.
    /// \param rightDate The second Date object to compare.
    /// \return True if the stored date occurs before the given date or they are equal.
	friend bool operator<=( const Date& leftDate, const Date& rightDate);

private:
	/// \brief Sets the month.
    /// \param month Downloader::DateMonth value representing the month.
    /// \return DATE_SUCCESS if the date is successfully set, otherwise a DateError error code.
	int SetMonth(DateMonth month);
    
	/// \brief Sets the day.
    /// \param day The day.  Must be a valid day for the given month in the given year.
    /// \return DATE_SUCCESS if the date is successfully set, otherwise a DateError error code.
	int SetDay(int day);
    
	/// \brief Sets the year.
    /// \param year The year.  Must be greater than Downloader::DATE_MIN_YEAR.
    /// \return DATE_SUCCESS if the date is successfully set, otherwise a DateError error code.
	int SetYear(int year);

    
	/// \brief Downloader::DateMonth value representing the month.
	DateMonth mMonth;
    
	/// \brief The day.
	int mDay;
    
	/// \brief The year.
	int mYear;
};


/// \brief Holds information about a specific date and time.
/// \note Unless specified, all values are in UTC.
class DateTime
{
public:
    /// \brief The DateTime constructor.
	DateTime();

    /// \brief The DateTime destructor.
	~DateTime();
    
	/// \brief Checks if the date has been set.
    /// \return True if the date has been set.
	bool IsValid() const		{ return mHasBeenSet; }
    
	/// \brief Marks that we have (or have not) set the date.
    /// \param value True if we have set the date.
	void IsValid(bool value)	{ mHasBeenSet = value; }
    
	/// \brief Sets the date and time.
    /// \param month Downloader::DateMonth value representing the month.
    /// \param day The day.  Must be a valid day for the given month in the given year.
    /// \param year The year.  Must be greater than Downloader::DATE_MIN_YEAR.
    /// \param hour The hour.  Must be less than or equal to Downloader::DATE_MAX_HOUR and greater than or equal to 0.
    /// \param min The minute.  Must be less than or equal to Downloader::DATE_MAX_MINUTE and greater than or equal to 0.
    /// \param sec The second.  Must be less than or equal to Downloader::DATE_MAX_SECOND and greater than or equal to 0.
    /// \return DATE_SUCCESS if the date and time are successfully set, otherwise a DateError error code.
	int SetDate( DateMonth month, int day, int year, int hour = 0, int min = 0, int sec = 0);

	/// \brief Sets the date using the number of seconds since Unix epoch (POSIX/Unix standard).
    /// \param timeInSeconds The number of seconds since epoch.
    /// \return DATE_SUCCESS if the date and time are successfully set, otherwise a DateError error code.
	int SetDate(qint64 timeInSeconds);

    /// \brief Converts the date and time to number of seconds since Unix epoch.
    /// \return Number of seconds since Unix epoch.
	qint64 ConvertToSeconds() const;

    /// \brief Sets the time and date to the current system time and date.
	void SetToNow();
    
	/// \brief Gets the number of seconds remaining from now until the stored date and time.
	/// \return The number of seconds remaining.
	qint64 GetCountdownSeconds() const;

    /// \brief Gets the number of months, days, and hours remaining from now until the stored date and time.
	/// \param nMonths This function sets this value to the number of months remaining.
    /// \param nDays This function sets this value to the number of days remaining.
    /// \param nHours This function sets this value to the number of hours remaining.
    /// \note Once the specified date and time has passed, all parameters will be set to 0.
	/// \note nHours is always rounded up (i.e. 1 hour and 10 seconds becomes 2 hours).
	void GetCountdownMDH( qint32 & nMonths, qint32 & nDays, qint32 & nHours );
    
    /// \brief Gets the number of days and hours remaining from now until the stored date and time.
    /// \param nDays This function sets this value to the number of days remaining.
    /// \param nHours This function sets this value to the number of hours remaining.
    /// \note Once the specified date and time has passed, all parameters will be set to 0.
	/// \note nHours is always rounded up (i.e. 1 hour and 10 seconds becomes 2 hours).
	void GetCountdownDH( qint32 & nDays, qint32 & nHours );
    
    /// \brief Gets the number of days, hours, and minutes remaining from now until the stored date and time.
    /// \param nDays This function sets this value to the number of days remaining.
    /// \param nHours This function sets this value to the number of hours remaining.
	/// \param nMinutes This function sets this value to the number of minutes remaining.
    /// \note Once the specified date and time has passed, all parameters will be set to 0.
	void GetCountdownDHM( qint32 & nDays, qint32 & nHours, qint32 & nMinutes );

	/// \brief Gets the number of days in the month of the currently stored date.
    /// \return Number of days in the month.
	int GetDaysInMonth() const;
    
	/// \brief Gets the month of the currently stored date.
    /// \return A Downloader::DateMonth value representing the stored month.
	DateMonth GetMonth() const       { return mMonth; }
    
	/// \brief Gets the day of the currently stored date.
    /// \return An integer representing the day.
	int GetDay() const	             { return mDay; }
    
	/// \brief Gets the year of the currently stored date.
    /// \return An integer representing the year.
	int GetYear() const              { return mYear; }
    
	/// \brief Gets the hour of the currently stored time.
    /// \return An integer representing the hour.
	int GetHour() const              { return mHour; }
    
	/// \brief Gets the minute of the currently stored time.
    /// \return An integer representing the minute.
	int GetMin() const               { return mMin; }
    
	/// \brief Gets the second of the currently stored time.
    /// \return An integer representing the second.
	int GetSecond() const            { return mSec; }

    /// \brief Overridden "=" operator.
    /// \param rhs The DateTime object to should copy from.
    /// \return The DateTime object copied to.
	const DateTime& operator=( const DateTime& rhs );
    
    /// \brief Overridden "<" operator.
    /// \param rhs The DateTime object to compare against.
    /// \return True if the stored date and time occur before the given date and time.
	bool operator<( const DateTime& rhs ) const;
    
    /// \brief Overridden "<=" operator.
    /// \param rhs The DateTime object to compare against.
    /// \return True if the stored date and time occur before the given date and time or they are equal.
	bool operator<=( const DateTime& rhs ) const;
    
    /// \brief Overridden ">" operator.
    /// \param rhs The DateTime object to compare against.
    /// \return True if the stored date and time occur after the given date and time.
    bool operator>( const DateTime& rhs ) const;
    
    /// \brief Overridden ">=" operator.
    /// \param rhs The DateTime object to compare against.
    /// \return True if the stored date and time occur after the given date and time or they are equal.
    bool operator>=( const DateTime& rhs ) const;
    
    /// \brief Overridden "==" operator.
    /// \param rhs The DateTime object to compare against.
    /// \return True if the stored date and time and the given date and time are equal.
	bool operator==( const DateTime& rhs ) const;
    
    /// \brief Overridden "!=" operator.
    /// \param rhs The DateTime object to compare against.
    /// \return True if the stored date and time and the given date and time are NOT equal.
	bool operator!=( const DateTime& rhs ) const;
    
	/// \brief Checks if the stored date and time falls within the given range.
    /// \param range A DateTimeRange object representing the range.
    /// \return True if the stored date and time falls within the given range.
    bool InRange(const DateTimeRange& range) const;
    
	/// \brief Checks if the stored date and time falls before the given range.
    /// \param range A DateTimeRange object representing the range.
    /// \return True if the stored date and time falls before the given range.
    bool BeforeRange(const DateTimeRange& range) const;
    
	/// \brief Checks if the stored date and time falls after the given range.
    /// \param range A DateTimeRange object representing the range.
    /// \return True if the stored date and time falls after the given range.
	bool AfterRange(const DateTimeRange& range) const;

	/// \brief Adjusts the stored date and time to compensate for local clock skew.
    /// \param reference A DateTime object that contains the reference clock.
    /// \return True if our internal date and time was modified.
	bool AdjustUsingReferenceClock(const DateTime &reference);

    /// \brief Converts an MS DOS time to a QTime.
    /// \param msDosTime The time given in MS DOS format.
    /// \return A QTime object that represents the same time as the MS DOS time.
	static QTime MsDosTimeToQTime(ushort msDosTime);
    
    /// \brief Converts an MS DOS date to a QDate.
    /// \param msDosDate The date given in MS DOS format.
    /// \return A QDate object that represents the same date as the MS DOS date.
	static QDate MsDosDateToQDate(ushort msDosDate);
    
    /// \brief Converts a QTime object to an MS DOS time.
    /// \param qTime A QTime object that represents the time.
    /// \return The time in MS DOS format.
	static ushort QTimeToMsDosTime(const QTime& qTime);
    
    /// \brief Converts a QDate object to an MS DOS date.
    /// \param qDate A QDate object that represents the date.
    /// \return The date in MS DOS format.
	static ushort QDateToMsDosDate(const QDate& qDate);

protected:
	/// \brief Sets the month.
    /// \param month Downloader::DateMonth value representing the month.
    /// \return DATE_SUCCESS if the date is successfully set, otherwise a DateError error code.
	int SetMonth(DateMonth month);
    
	/// \brief Sets the day.
    /// \param day The day.  Must be a valid day for the given month in the given year.
    /// \return DATE_SUCCESS if the date is successfully set, otherwise a DateError error code.
	int SetDay(int day);
    
	/// \brief Sets the year.
    /// \param year The year.  Must be greater than Downloader::DATE_MIN_YEAR.
    /// \return DATE_SUCCESS if the date is successfully set, otherwise a DateError error code.
	int SetYear(int year);
    
	/// \brief Sets the hour.
    /// \param hour The hour.  Must be greater than 0 and less than or equal to Downloader::DATE_MAX_HOUR.
    /// \return DATE_SUCCESS if the time is successfully set, otherwise a DateError error code.
	int SetHour(int hour);
    
	/// \brief Sets the minute.
    /// \param min The minute.  Must be greater than 0 and less than or equal to Downloader::DATE_MAX_MINUTE.
    /// \return DATE_SUCCESS if the time is successfully set, otherwise a DateError error code.
	int SetMin(int min);
    
	/// \brief Sets the second.
    /// \param sec The second.  Must be greater than 0 and less than or equal to Downloader::DATE_MAX_SECOND.
    /// \return DATE_SUCCESS if the time is successfully set, otherwise a DateError error code.
	int SetSec(int sec);


protected:	
    /// \brief Downloader::DateMonth value representing the month.
	DateMonth mMonth;
    
	/// \brief The day.
	int mDay;
    
	/// \brief The year.
	int mYear;
    
	/// \brief The hour.
	int mHour;
    
	/// \brief The minute.
	int mMin;
    
	/// \brief The second.
	int mSec;
    
	/// \brief True if the stored date and time were set.
	bool mHasBeenSet;
};

/// \brief Holds information about a range of dates and times.
/// \note Unless specified, all values are in UTC.
class DateTimeRange
{
public:
    /// \brief The DateTimeRange constructor.
    DateTimeRange();

    /// \brief The DateTimeRange constructor.
    /// \param start A pointer to a DateTime object representing the start of the range.
    /// \param end A pointer to a DateTime object representing the end of the range.
    DateTimeRange(const DateTime *start, const DateTime *end);
   
    /// \brief The DateTimeRange destructor.
    ~DateTimeRange();
 
    /// \brief Gets the starting date and time.
    /// \return A DateTime object representing the starting date and time.
    const DateTime& GetStart() const { return mStart; }
    
    /// \brief Gets the ending date and time.
    /// \return A DateTime object representing the ending date and time.
    const DateTime& GetEnd() const   { return mEnd; }
   
    /// \brief Checks if the given date and time are within the stored range.
    /// \param time A DateTime object representing the date and time to check.
    /// \return 0 if within range (inclusive), -1 if before range, 1 if after range.
    int OutsideRange(const DateTime &time) const;

protected:
	/// \brief True if DateTimeRange::mStart is valid.
    bool mHasStart;

    /// \brief The starting date and time.
    DateTime mStart;
   
	/// \brief True if DateTimeRange::mEnd is valid.
    bool mHasEnd;

    /// \brief The ending date and time.
    DateTime mEnd;
};

/// \brief Holds information about a range of dates.
/// \note Unless specified, all values are in UTC.
class DateRange
{
public:
    /// \brief The DateRange constructor.
	DateRange();
    
    /// \brief The DateRange constructor.
    /// \param start A pointer to a Date object representing the start of the range.
    /// \param end A pointer to a Date object representing the end of the range.
	DateRange(const Date *start, const Date *end);

    /// \brief Sets the starting date.
    /// \param start A Date object representing the starting date.
	void SetStartDate(const Date *start);
    
    /// \brief Gets the starting date.
    /// \return A pointer to a Date object representing the starting date.
	const Date *GetStartDate() const;
    
    /// \brief Sets the ending date.
    /// \param end A Date object representing the ending date.
	void SetEndDate(const Date *end);

    /// \brief Gets the ending date.
    /// \return A pointer to a Date object representing the ending date.
	const Date *GetEndDate() const;

    /// \brief Overridden "==" operator.
    /// \param leftDateRange The first DateRange to compare.
    /// \param rightDateRange The second DateRange to compare.
    /// \return True if both ranges are the same.
	friend bool operator==(const DateRange& leftDateRange, const DateRange& rightDateRange);

private:
    /// \brief The starting date.
	Date mStart;
    
    /// \brief The ending date.
	Date mEnd;
};

} // namespace Downloader
} // namespace Origin

#endif	// DATE_TIME_H
