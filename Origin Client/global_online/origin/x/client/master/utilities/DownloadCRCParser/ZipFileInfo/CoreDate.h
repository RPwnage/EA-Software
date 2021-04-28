//
//	Header file for dates.
//
//	Copyright 2004 Electronic Arts Inc.  All rights reserved.
//

#ifndef CORE_DATE_H
#define CORE_DATE_H

#include <QDate>
#include <QString>
#include <QTime>

//namespace Core
//{

// NOTE: Unless specified all time/date values are in UTC.

// constants
const int DATE_SUCCESS = 0;
const int DATE_FAIL = -1;
const int DATE_ERR_MONTH_OUT_OF_RANGE = -2;
const int DATE_ERR_DAY_OUT_OF_RANGE = -3;
const int DATE_ERR_YEAR_OUT_OF_RANGE = -4;
const int DATE_ERR_HOUR_OUT_OF_RANGE = -5;
const int DATE_ERR_MINUTE_OUT_OF_RANGE = -6;
const int DATE_ERR_SECOND_OUT_OF_RANGE = -7;

const int DATE_MIN_YEAR = 1900;
const int DATE_MAX_HOUR = 24;
const int DATE_MAX_MINUTE = 59;
const int DATE_MAX_SECOND = 59;

const int DATE_STRING_LEN = 11; // JUN-25-1979
const int DATE_TIME_STRING_LEN = 24; // JUN-25-1979 12:01:01 UTC

//types
typedef enum
{
	CORE_DATE_MONTH_JAN = 1,
	CORE_DATE_MONTH_FEB,
	CORE_DATE_MONTH_MAR,
	CORE_DATE_MONTH_APR,
	CORE_DATE_MONTH_MAY,
	CORE_DATE_MONTH_JUN,
	CORE_DATE_MONTH_JUL,
	CORE_DATE_MONTH_AUG,
	CORE_DATE_MONTH_SEP,
	CORE_DATE_MONTH_OCT,
	CORE_DATE_MONTH_NOV,
	CORE_DATE_MONTH_DEC

}CoreDateMonth;


// forward decl
class CoreDateTimeRange;

class CoreDate
{

public:
	CoreDate();

	//Returns constant DATE_SUCCESS is the date is successfully set.
	//Othewise, it  returns an error code.
	int SetDate(CoreDateMonth month, int day, int year);

	// updated to use new proxy server date format
	int SetDate(const QString sDate);


	CoreDateMonth GetMonth() const
	{
		return mMonth;
	}

	int GetDay() const
	{
		return mDay;
	}

	int GetYear() const
	{
		return mYear;
	}

	friend bool operator==(const CoreDate& leftDate, const CoreDate& rightDate);
	friend bool operator<( const CoreDate& leftDate, const CoreDate& rightDate);
	friend bool operator>( const CoreDate& leftDate, const CoreDate& rightDate);
    friend bool operator!=( const CoreDate& leftDate, const CoreDate& rightDate);
	friend bool operator>=( const CoreDate& leftDate, const CoreDate& rightDate);
	friend bool operator<=( const CoreDate& leftDate, const CoreDate& rightDate);

private:
	int SetMonth(CoreDateMonth month);
	int SetDay(int day);
	int SetYear(int year);
	CoreDateMonth mMonth;
	int mDay;
	int mYear;
};


class CoreDateTime
{
public:
	CoreDateTime();
	~CoreDateTime();

	bool IsValid() const		{ return mHasBeenSet; }
	void IsValid(bool value)	{ mHasBeenSet = value; }

	//Returns constant DATE_SUCCESS is the date is successfully set.
	//Othewise, it  returns an error code.
	int SetDate( CoreDateMonth month, int day, int year, int hour = 0, int min = 0, int sec = 0);
 
	// Parses just the date and time; time zone will be ignored
	int SetISO8601DateTime(const QString sDate, bool bAllowDateOnly = false);

	// updated to use new proxy server date format
	int SetDate(const QString sDate);

	//Set the date using time_t 64-bit format (POSIX/Unix standard)
	int SetDate(__int64 timeInSeconds);
	__int64 ConvertToSeconds() const;

	// return the number of seconds remaining from 'now' until 'datetime'.
	// once the datetime has passed, the countdown will return zero.
	void SetToNow();
	__int64 GetCountdownSeconds() const;

	// retrieves the number of months, days, and hours remaining from 'now' until 'datetime'.
	// once the datetime has passed, the countdown will return zero.
	void GetCountdownMDH( long & nMonths, long & nDays, long & nHours );

	// retrieves the number of days and hours remaining from 'now' until 'datetime'.
	// once the datetime has passed, the countdown will return zero.
	void GetCountdownDH( long & nDays, long & nHours );
	void GetCountdownDHM( long & nDays, long & nHours, long & nMinutes );

	//	accessors
	int GetDaysInMonth( void ) const;
	CoreDateMonth GetMonth() const   { return mMonth; } // Gets the number of days in the month of the currently stored date.
	int GetDay() const	             { return mDay; }
	int GetYear() const              { return mYear; }
	int GetHour() const              { return mHour; }
	int GetMin() const               { return mMin; }
	int GetSecond() const            { return mSec; }

	/**
	 * Writes the contents of the date to a string in a readable format:
	 * MM-DD-YYYY HH:MM:SS
	 *
	 * @param *buf  String buffer to write to.
	 * @param bufSize  Size of passed in buffer.
	 */
	void ToString(char *buf, size_t bufSize) const;
	void ToString(wchar_t *buf, size_t bufSize) const;

	const CoreDateTime& operator=( const CoreDateTime& rhs );
	bool operator<( const CoreDateTime& rhs ) const;
	bool operator<=( const CoreDateTime& rhs ) const;
    bool operator>( const CoreDateTime& rhs ) const;
    bool operator>=( const CoreDateTime& rhs ) const;
	bool operator==( const CoreDateTime& rhs ) const;
	bool operator!=( const CoreDateTime& rhs ) const;

    bool InRange(const CoreDateTimeRange& range) const;
    bool BeforeRange(const CoreDateTimeRange& range) const;
	bool AfterRange(const CoreDateTimeRange& range) const;
	/*
	 * Adjusts the datetime to compensate for local clock skew assuming:
	 * - The reference clock is accurate
	 * - The reference clock was recently sampled
	 */
	bool AdjustUsingReferenceClock(const CoreDateTime &reference);

	static QTime MsDosTimeToQTime(unsigned short msDosTime);
	static QDate MsDosDateToQDate(unsigned short msDosDate);

	static unsigned short QTimeToMsDosTime(const QTime& qTime);
	static unsigned short QDateToMsDosDate(const QDate& qDate);

protected:
	int SetMonth(CoreDateMonth month);
	int SetDay(int day);
	int SetYear(int year);
	int SetHour(int hour);
	int SetMin(int min);
	int SetSec(int sec);

	// Get month from string and return error code
	int GetMonthFromStr( const char *monthStr, CoreDateMonth& month );

protected:
	CoreDateMonth mMonth;
	int mDay;
	int mYear;
	int mHour;
	int mMin;
	int mSec;

	bool mHasBeenSet;
};

class CoreDateTimeRange
{
public:
   CoreDateTimeRange();
   CoreDateTimeRange(const CoreDateTime *start, const CoreDateTime *end);
   ~CoreDateTimeRange();

   const CoreDateTime& GetStart() const { return mStart; }
   const CoreDateTime& GetEnd() const   { return mEnd; }

   //returns 0 if within range (inclusive), -1 if before range, 1 if after range
   int OutsideRange(const CoreDateTime &time) const;

protected:
   bool mHasStart;
   CoreDateTime mStart;

   bool mHasEnd;
   CoreDateTime mEnd;
};

//This helper class encapsulates a date range
class DateRange
{
public:
	DateRange();
	DateRange(const CoreDate *start, const CoreDate *end);

	void SetStartDate(const CoreDate *start);
	const CoreDate *GetStartDate() const;

	void SetEndDate(const CoreDate *end);
	const CoreDate *GetEndDate() const;

	friend bool operator==(const DateRange& leftDateRange, const DateRange& rightDateRange);

private:
	CoreDate mStart;
	CoreDate mEnd;
};

//}  // namespace Fesl

#endif	// FESL_DATE_H
