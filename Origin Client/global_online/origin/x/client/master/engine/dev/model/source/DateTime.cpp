//
//	date class.
//
//	Copyright 2004 Electronic Arts Inc.  All rights reserved.
//

#include "DateTime.h"
#include "services/debug/DebugService.h"

#include <QDate>

namespace Origin
{
	namespace Utilities
	{
		qint64 GetCurrentSeconds()
		{
			return QDateTime::currentMSecsSinceEpoch() / qint64(1000);
		}
	}

namespace Downloader
{

	//	Using Unix Epoch Time as a base, which is standard across many platforms and
	//	is the base year for platform/core services (1/1/1970 00:00:00 UTC)
	const int CORE_BASE_YEAR = 1970;

	//We have a leap year every four years, except those when a new
	//century starts that is not a multiple of 400. This is why 2000
	//was a leap year, for example, but 1900, 1800, and 1700 were not.
	static bool IsLeapYear(int year)
	{
		if ((year % 4 == 0 && year % 100 != 0) || year % 400 == 0)
		{
			return true;
		}
		else
		{
			return false;
		}
	}

	Date::Date()
	{
		SetDate(DATE_TIME_MONTH_JAN, 1, CORE_BASE_YEAR);
	}


	int Date::SetDate(DateMonth month, int day, int year)
	{
		int err = SetMonth(month);
		if (err != DATE_SUCCESS)
		{
			return err;
		}

		err = SetYear(year);

		if (err != DATE_SUCCESS)
		{
			return err;
		}

		return (SetDay(day));
	}

	int Date::SetMonth(DateMonth month)
	{
		if (month > 0 && month < 13)
		{
			mMonth = month;
			return DATE_SUCCESS;
		}
		return DATE_ERR_MONTH_OUT_OF_RANGE;
	}


	int Date::SetDay(int day)
	{
		int maxMonthDay = 0;
		switch(mMonth)
		{
		case DATE_TIME_MONTH_FEB:
			{
				if (IsLeapYear(mYear))
				{
					maxMonthDay = 29;
				}
				else
				{
					maxMonthDay = 28;
				}
				break;
			}
		case DATE_TIME_MONTH_APR:
		case DATE_TIME_MONTH_JUN:
		case DATE_TIME_MONTH_SEP:
		case DATE_TIME_MONTH_NOV:
			{
				maxMonthDay = 30;
				break;
			}
		case DATE_TIME_MONTH_JAN:
		case DATE_TIME_MONTH_MAR:
		case DATE_TIME_MONTH_MAY:
		case DATE_TIME_MONTH_JUL:
		case DATE_TIME_MONTH_AUG:
		case DATE_TIME_MONTH_OCT:
		case DATE_TIME_MONTH_DEC:
			{
				maxMonthDay = 31;
				break;
			}
		default:
			{
				return DATE_ERR_MONTH_OUT_OF_RANGE;
			}
		}
		if (day > 0 && day <= maxMonthDay)
		{
			mDay = day;
			return DATE_SUCCESS;
		}
		else
		{
			return DATE_ERR_DAY_OUT_OF_RANGE;
		}
	}

	int Date::SetYear(int year)
	{
		if (year >= DATE_MIN_YEAR)
		{
			mYear = year;
			return DATE_SUCCESS;
		}
		else
		{
			return DATE_ERR_YEAR_OUT_OF_RANGE;
		}
	}


	// Decodes a date in a string formatted as YYYY-MM-DD (1999-05-31)
	// This the format used in the new proxy server GetContent response
	//
	int Date::SetDate(const QString sDate)
	{	
		QDate qDate = QDate::fromString(sDate, Qt::ISODate);

		ORIGIN_ASSERT(qDate.isValid());

		return SetDate(static_cast<DateMonth>(qDate.month()), qDate.day(), qDate.year());
	}

	/**
	* Set the date/time from the Unix epoch time format (in UTC)
	*
	*	See DateTime::ConvertToSeconds
	*/
	int DateTime::SetDate(qint64 timeInSeconds)
	{	
		if (timeInSeconds < 0)
			return DATE_FAIL;

		QDateTime qDateTime = QDateTime::fromMSecsSinceEpoch(timeInSeconds * 1000);
		qDateTime = qDateTime.toUTC();

		if (!qDateTime.isValid())
			return DATE_FAIL;

		SetDate((DateMonth)(qDateTime.date().month()), qDateTime.date().day(), qDateTime.date().year(),
			qDateTime.time().hour(), qDateTime.time().minute(), qDateTime.time().second());	
		
		return DATE_SUCCESS;
	}

	qint64 DateTime::ConvertToSeconds() const
	{
		QDate qDate(mYear, mMonth, mDay);
		QTime qTime(mHour, mMin, mSec);
		QDateTime qDateTime(qDate, qTime, Qt::UTC);

		return qDateTime.toMSecsSinceEpoch() / qint64(1000);
	}

	bool operator==(const Date& leftDate, const Date& rightDate)
	{
		return ((leftDate.mMonth == rightDate.mMonth) &&
			(leftDate.mDay == rightDate.mMonth) &&
			(leftDate.mYear == rightDate.mYear));
	}


	bool operator!=( const Date& leftDate, const Date& rightDate)
	{
		return !(leftDate == rightDate);
	}


	bool operator<( const Date& leftDate, const Date& rightDate)
	{
		if (leftDate.mYear < rightDate.mYear)
			return true;
		else if (leftDate.mYear == rightDate.mYear)
		{
			if (leftDate.mMonth < rightDate.mMonth)
				return true;
			else if (leftDate.mMonth == rightDate.mMonth)
			{
				if (leftDate.mDay < rightDate.mDay)
					return true;
			}
		}

		return false;
	}

	bool operator<=( const Date& leftDate, const Date& rightDate)
	{
		if (leftDate.mYear < rightDate.mYear)
			return true;
		else if (leftDate.mYear == rightDate.mYear)
		{
			if (leftDate.mMonth < rightDate.mMonth)
				return true;
			else if (leftDate.mMonth == rightDate.mMonth)
			{
				if (leftDate.mDay <= rightDate.mDay)
					return true;
			}
		}

		return false;
	}


	bool operator>( const Date& leftDate, const Date& rightDate)
	{
		if (leftDate.mYear > rightDate.mYear)
			return true;
		else if (leftDate.mYear == rightDate.mYear)
		{
			if (leftDate.mMonth > rightDate.mMonth)
				return true;
			else if (leftDate.mMonth == rightDate.mMonth)
			{
				if (leftDate.mDay > rightDate.mDay)
					return true;
			}
		}

		return false;
	}


	bool operator>=( const Date& leftDate, const Date& rightDate)
	{
		if (leftDate.mYear > rightDate.mYear)
			return true;
		else if (leftDate.mYear == rightDate.mYear)
		{
			if (leftDate.mMonth > rightDate.mMonth)
				return true;
			else if (leftDate.mMonth == rightDate.mMonth)
			{
				if (leftDate.mDay >= rightDate.mDay)
					return true;
			}
		}

		return false;
	}


	///////////////////////////////////////////////////////////////////////////////
	DateTime::DateTime() :
	mMonth(DATE_TIME_MONTH_JAN),
		mDay(1),
		mYear(CORE_BASE_YEAR),
		mHour(0),
		mMin(0),
		mSec(0),
		mHasBeenSet(false)
	{
	}

	DateTime::~DateTime()
	{
	}

	int DateTime::SetDate( DateMonth month, int day, int year, int hour, int min, int sec )
	{
		mHasBeenSet = false;
		int err = SetMonth(month);
		if (err != DATE_SUCCESS)
		{
			return err;
		}

		err = SetYear(year);
		if (err != DATE_SUCCESS)
		{
			return err;
		}

		err = SetDay(day);
		if (err != DATE_SUCCESS)
		{
			return err;
		}

		err = SetHour(hour);
		if (err != DATE_SUCCESS)
		{
			return err;
		}

		err = SetMin(min);
		if (err != DATE_SUCCESS)
		{
			return err;
		}

		err = SetSec(sec);
		if( err != DATE_SUCCESS)
		{
			return err;
		}

		mHasBeenSet = true;
		return DATE_SUCCESS;
	}



	// set to the current system time (in UTC)
	void DateTime::SetToNow()
	{
		qint64 curTime = Utilities::GetCurrentSeconds();
		SetDate(curTime);
	}

	int DateTime::GetDaysInMonth( void ) const
	{
		int nResult = 0;

		switch ( mMonth )
		{
		case DATE_TIME_MONTH_FEB:
			{
				if ( IsLeapYear( mYear ) ) { nResult = 29; }
				else					   { nResult = 28; }
				break;
			}
		case DATE_TIME_MONTH_APR:
		case DATE_TIME_MONTH_JUN:
		case DATE_TIME_MONTH_SEP:
		case DATE_TIME_MONTH_NOV: { nResult = 30; break; }
		case DATE_TIME_MONTH_JAN:
		case DATE_TIME_MONTH_MAR:
		case DATE_TIME_MONTH_MAY:
		case DATE_TIME_MONTH_JUL:
		case DATE_TIME_MONTH_AUG:
		case DATE_TIME_MONTH_OCT:
		case DATE_TIME_MONTH_DEC: { nResult = 31; break; }
		default:				  { nResult = DATE_ERR_MONTH_OUT_OF_RANGE; break; }
		}

		return nResult;
	}

	// return the number of seconds remaining from 'now' until 'datetime'.
	// once the datetime has passed, the countdown will return zero.
	// is not aware of the timezone setting (assumes that this datetime is UTC)
	qint64 DateTime::GetCountdownSeconds() const
	{
		qint64 curTime = Utilities::GetCurrentSeconds();
		qint64 thisTime = ConvertToSeconds();
		return (thisTime - curTime);
	}

	/* ===========================================================================================
	Purpose: Retrieves the number of months, days, and hours remaning from *now* until *this*
	date instance (the assumption is that *now* < *this*.

	Parameters:	nMonths (out) - Number of months.
	nDays   (out) - Number of days.
	nHours	(out) - Number of hours.

	Notes: o Once *now* >= *this*, all parameters will return 0.
	o 'nHours' is always rounded up (ex. 1 hour and 10 seconds becomes 2 hours).
	o If the number of hours calculated is >= 24 then 'nDays' is incremented and 24 is
	subtracted from 'nHours'.
	o If the number days calculated is > the number of days in the month of *now* then
	'nMonths' is incremented and the number of days in the month of *now* is
	subtracted from 'nDays'.
	=========================================================================================== */
	void DateTime::GetCountdownMDH( qint32 & nMonths, qint32 & nDays, qint32 & nHours )
	{
		const qint32 NHOURS_IN_DAY   = 24;
		const qint32 NMONTHS_IN_YEAR = 12;

		qint32 nNowYear,     nNowMonth,     nNowDay,     nNowHour,	nNowDaysInMonth;
		qint32 nEndYear,     nEndMonth,     nEndDay,     nEndHour;
		qint32 nRawYearDiff, nRawMonthDiff, nRawDayDiff, nRawHourDiff;

		qint32 nExtraHours  = 1; // +1 hour off the bat for rounding up.
		qint32 nExtraDays   = 0; // Additions from hours overflow.
		qint32 nExtraMonths = 0; // Additions from days overflow.

		DateTime now;

		// Only compute differences if *now* is less than *this* date.
		if ( now < *this )
		{
			// ------------------------------
			//  Get the date parts of *now*.
			//

			now.SetToNow();
			nNowYear  = now.GetYear();
			nNowMonth = ( qint32 ) now.GetMonth();
			nNowDay   = now.GetDay();
			nNowHour  = now.GetHour();
			nNowDaysInMonth = now.GetDaysInMonth();


			// ------------------------------------
			//  Get the date parts of *this* date.
			//

			nEndYear  = GetYear();
			nEndMonth = ( qint32 ) GetMonth();
			nEndDay   = GetDay();
			nEndHour  = GetHour();


			// ---------------------------------
			//  Calculate the hours difference.
			//

			if ( ( nNowYear == nEndYear ) && ( nNowMonth == nEndMonth ) && ( nNowDay == nEndDay ) )
			{
				nRawHourDiff  = nEndHour  - nNowHour;
				nHours = ( nRawHourDiff > 1 ) ? ( nRawHourDiff - 1 ) : 0;
				nHours += nExtraHours;
			}
			else
			{
				nHours = nEndHour + ( NHOURS_IN_DAY - nNowHour ) - 1;
				nHours += nExtraHours;

				if ( nHours >= NHOURS_IN_DAY )
				{
					nHours -= NHOURS_IN_DAY;
					nExtraDays++;
				} // if ( nHours >= NHOURS_IN_DAY )

			} // if ( ( nNowYear == nEndYear ) && ( nNowMonth == nEndMonth ) && ( nNowDay == nEndDay ) )


			// --------------------------------
			//  Calculate the days difference.
			//

			if ( ( nNowYear == nEndYear ) && ( nNowMonth == nEndMonth ) )
			{
				nRawDayDiff = nEndDay - nNowDay;
				nDays = ( nRawDayDiff > 1 ) ? ( nRawDayDiff - 1 ) : 0;
				nDays += nExtraDays;
			}
			else
			{
				nDays = nEndDay + ( nNowDaysInMonth - nNowDay ) - 1;
				nDays += nExtraDays;

				if ( nDays > nNowDaysInMonth )
				{
					nDays -= nNowDaysInMonth;
					nExtraMonths++;
				} // if ( nDays > nNowDaysInMonth )

			} // if ( ( nNowYear == nEndYear ) && ( nNowMonth == nEndMonth ) )


			// ----------------------------------
			//  Calculate the months difference.
			//

			if ( nNowYear == nEndYear )
			{
				nRawMonthDiff = nEndMonth - nNowMonth;
				nMonths = ( nRawMonthDiff > 1 ) ? ( nRawMonthDiff - 1 ) : 0;
				nMonths += nExtraMonths;
			}
			else
			{
				nRawYearDiff = nEndYear - nNowYear;
				nMonths = nEndMonth + ( NMONTHS_IN_YEAR - nNowMonth ) - 1 + ( ( nRawYearDiff > 1 ? ( nRawYearDiff - 1 ) : 0 ) * NMONTHS_IN_YEAR );
				nMonths += nExtraMonths;
			} // if ( ( nNowYear == nEndYear ) )

		}
		else
		{
			nMonths = 0;
			nDays   = 0;
			nHours  = 0;
		} // if ( now < *this )

	}

	/* ===========================================================================================
	Purpose: Retrieves the number of days and hours remaning from *now* until *this*
	date instance (the assumption is that *now* < *this*.

	Parameters:	nDays   (out) - Number of days.
	nHours	(out) - Number of hours.

	Notes: o Once *now* >= *this*, all parameters will return 0.
	o 'nHours' is always rounded up (ex. 1 hour and 10 seconds becomes 2 hours).
	=========================================================================================== */
	void DateTime::GetCountdownDH( qint32 & nDays, qint32 & nHours )
	{
		const qint32 NHOURS_IN_DAY = 24;
		const qint32 NSECS_IN_MIN  = 60;
		const qint32 NMINS_IN_HOUR = 60;

		qint64 nSeconds    = GetCountdownSeconds();
		qint32    nExtraHours = 1; // +1 hour off the bat for rounding up.

		// Defaults.
		nDays   = 0;
		nHours  = 0;

		// Only compute differences if *now* is less than *this* date.
		if ( nSeconds )
		{
			// ---------------------------------
			//  Calculate the hours difference.
			//

			nHours = ( qint32 ) ( nSeconds / NSECS_IN_MIN / NMINS_IN_HOUR ) + nExtraHours;


			// ---------------------------------
			//  Calculate the days difference.
			//

			if ( nHours >= NHOURS_IN_DAY )
			{
				nDays  = nHours / NHOURS_IN_DAY;
				nHours = nHours % NHOURS_IN_DAY;
			} // if ( nHours >= NHOURS_IN_DAY )

		} // if ( nSeconds )

	}

	void DateTime::GetCountdownDHM( qint32 & nDays, qint32 & nHours, qint32 & nMinutes )
	{
		static const qint32 NHOURS_IN_DAY = 24;
		static const qint32 NSECS_IN_MIN  = 60;
		static const qint32 NMINS_IN_HOUR = 60;

		qint64 nSeconds = GetCountdownSeconds();

		// Defaults.
		nDays    = 0;
		nHours   = 0;
		nMinutes = 0;

		// Only compute differences if *now* is less than *this* date.
		if ( nSeconds )
		{
			// ---------------------------------
			//  Calculate the minutes difference.
			//
			nMinutes = ( qint32 ) ( nSeconds / NSECS_IN_MIN );

			// Round to at least one minute
			if ( nMinutes == 0 )
				nMinutes = 1;

			// ---------------------------------
			//  Calculate the hours difference.
			//
			if ( nMinutes >= NMINS_IN_HOUR )
			{
				nHours = ( qint32 ) ( nMinutes / NMINS_IN_HOUR );
				nMinutes = nMinutes % NMINS_IN_HOUR;

				// ---------------------------------
				//  Calculate the days difference.
				//

				if ( nHours >= NHOURS_IN_DAY )
				{
					nDays  = nHours / NHOURS_IN_DAY;
					nHours = nHours % NHOURS_IN_DAY;
				} // if ( nHours >= NHOURS_IN_DAY )
			}

		} // if ( nSeconds )
	}

	int DateTime::SetMonth(DateMonth month)
	{
		if (month > 0 && month < 13)
		{
			mMonth = month;
			return DATE_SUCCESS;
		}
		return DATE_ERR_MONTH_OUT_OF_RANGE;
	}


	int DateTime::SetDay(int day)
	{
		int maxMonthDay = 0;
		switch(mMonth)
		{
		case DATE_TIME_MONTH_FEB:
			{
				if (IsLeapYear(mYear))
				{
					maxMonthDay = 29;
				}
				else
				{
					maxMonthDay = 28;
				}
				break;
			}
		case DATE_TIME_MONTH_APR:
		case DATE_TIME_MONTH_JUN:
		case DATE_TIME_MONTH_SEP:
		case DATE_TIME_MONTH_NOV:
			{
				maxMonthDay = 30;
				break;
			}
		case DATE_TIME_MONTH_JAN:
		case DATE_TIME_MONTH_MAR:
		case DATE_TIME_MONTH_MAY:
		case DATE_TIME_MONTH_JUL:
		case DATE_TIME_MONTH_AUG:
		case DATE_TIME_MONTH_OCT:
		case DATE_TIME_MONTH_DEC:
			{
				maxMonthDay = 31;
				break;
			}
		default:
			{
				return DATE_ERR_MONTH_OUT_OF_RANGE;
			}
		}
		if (day > 0 && day <= maxMonthDay)
		{
			mDay = day;
			return DATE_SUCCESS;
		}
		else
		{
			return DATE_ERR_DAY_OUT_OF_RANGE;
		}
	}

	int DateTime::SetYear(int year)
	{
		if (year >= DATE_MIN_YEAR)
		{
			mYear = year;
			return DATE_SUCCESS;
		}
		else
		{
			return DATE_ERR_YEAR_OUT_OF_RANGE;
		}

	}


	const DateTime& DateTime::operator=( const DateTime& rhs )
	{
		mMonth = rhs.mMonth;
		mDay = rhs.mDay;
		mYear = rhs.mYear;
		mHour = rhs.mHour;
		mMin = rhs.mMin;
		mSec = rhs.mSec;
		mHasBeenSet = rhs.mHasBeenSet;
		return *this;
	}

	bool DateTime::operator<( const DateTime& rhs ) const
	{
		if ( mYear < rhs.mYear )
		{
			return true;
		}
		else if ( mYear == rhs.mYear)
		{
			if ( mMonth <  rhs.mMonth )
			{
				return true;
			}
			else if ( mMonth == rhs.mMonth )
			{
				if ( mDay < rhs.mDay )
				{
					return true;
				}
				else if ( mDay == rhs.mDay )
				{
					if ( mHour < rhs.mHour )
					{
						return true;
					}
					else if (mHour == rhs.mHour)
					{
						if (mMin < rhs.mMin)
						{
							return true;
						}
						else if (mMin == rhs.mMin)
						{
							return mSec < rhs.mSec;
						}

					}
				}
			}
		}

		return false;
	}


	bool DateTime::operator<=( const DateTime& rhs ) const
	{
		if ( mYear < rhs.mYear )
		{
			return true;
		}
		else if ( mYear == rhs.mYear)
		{
			if ( mMonth <  rhs.mMonth )
			{
				return true;
			}
			else if ( mMonth == rhs.mMonth )
			{
				if ( mDay < rhs.mDay )
				{
					return true;
				}
				else if ( mDay == rhs.mDay )
				{
					if ( mHour < rhs.mHour )
					{
						return true;
					}
					else if (mHour == rhs.mHour)
					{
						if (mMin < rhs.mMin)
						{
							return true;
						}
						else if (mMin == rhs.mMin)
						{
							return mSec <= rhs.mSec;
						}

					}
				}
			}
		}

		return false;
	}


	bool DateTime::operator>( const DateTime& rhs ) const
	{
		if ( mYear > rhs.mYear )
		{
			return true;
		}
		else if ( mYear == rhs.mYear)
		{
			if ( mMonth >  rhs.mMonth )
			{
				return true;
			}
			else if ( mMonth == rhs.mMonth )
			{
				if ( mDay > rhs.mDay )
				{
					return true;
				}
				else if ( mDay == rhs.mDay )
				{
					if ( mHour > rhs.mHour )
					{
						return true;
					}
					else if ( mHour == rhs.mHour )
					{
						if ( mMin > rhs.mMin )
						{
							return true;
						}
						else if ( mMin == rhs.mMin )
						{
							if ( mSec > rhs.mSec )
							{
								return true;
							}
						}
					}
				}
			}
		}

		return false;
	}


	bool DateTime::operator>=( const DateTime& rhs ) const
	{
		if ( mYear > rhs.mYear )
		{
			return true;
		}
		else if ( mYear == rhs.mYear)
		{
			if ( mMonth >  rhs.mMonth )
			{
				return true;
			}
			else if ( mMonth == rhs.mMonth )
			{
				if ( mDay > rhs.mDay )
				{
					return true;
				}
				else if ( mDay == rhs.mDay )
				{
					if ( mHour > rhs.mHour )
					{
						return true;
					}
					else if ( mHour == rhs.mHour )
					{
						if ( mMin > rhs.mMin )
						{
							return true;
						}
						else if ( mMin == rhs.mMin )
						{
							if ( mSec >= rhs.mSec )
							{
								return true;
							}
						}
					}
				}
			}
		}

		return false;
	}

	bool DateTime::operator==( const DateTime& rhs ) const
	{
		if ( ( mYear == rhs.mYear ) &&
			( mMonth == rhs.mMonth ) &&
			( mDay == rhs.mDay ) &&
			( mHour == rhs.mHour ) &&
			( mMin == rhs.mMin ) &&
			( mSec == rhs.mSec ) )
		{
			return true;
		}

		return false;
	}


	bool DateTime::operator!=( const DateTime& rhs ) const
	{
		if ( ( mYear != rhs.mYear ) ||
			( mMonth != rhs.mMonth ) ||
			( mDay != rhs.mDay ) ||
			( mHour != rhs.mHour ) ||
			( mMin != rhs.mMin ) ||
			( mSec != rhs.mSec ) )
		{
			return true;
		}

		return false;
	}


	int DateTime::SetHour(int hour)
	{
		if ( (hour >= 0) && (hour <= DATE_MAX_HOUR))
		{
			mHour = hour;
			return DATE_SUCCESS;
		}

		return DATE_ERR_HOUR_OUT_OF_RANGE;
	}

	int DateTime::SetMin(int min)
	{
		if ( (min >= 0) && (min <= DATE_MAX_MINUTE))
		{
			mMin = min;
			return DATE_SUCCESS;
		}

		return DATE_ERR_MINUTE_OUT_OF_RANGE;
	}

	int DateTime::SetSec(int sec)
	{
		if ( (sec >= 0) && (sec <= DATE_MAX_SECOND))
		{
			mSec = sec;
			return DATE_SUCCESS;
		}

		return DATE_ERR_SECOND_OUT_OF_RANGE;
	}

	bool DateTime::InRange(const DateTimeRange& range) const
	{
		return (range.OutsideRange(*this) == 0);
	}

	bool DateTime::BeforeRange(const DateTimeRange& range) const
	{
		return (range.OutsideRange(*this) < 0);
	}

	bool DateTime::AfterRange(const DateTimeRange& range) const
	{
		return (range.OutsideRange(*this) > 0);
	}

	bool DateTime::AdjustUsingReferenceClock(const DateTime &ref)
	{
		DateTime local;
		local.SetToNow();

		if (!ref.IsValid() || !local.IsValid())
		{
			return false;
		}

		// Calculate our clock skew
		// Negative means we're slow, positive means we're fast
		qint64 iClockSkew = local.ConvertToSeconds() - ref.ConvertToSeconds();

		// Re-set our date using the clock kew
		SetDate(ConvertToSeconds() + iClockSkew);

		return true;
	}


	QTime DateTime::MsDosTimeToQTime(ushort msDosTime)
	{
		// Bits		Description
		// 0-4		Seconds divided by 2
		// 5-10		Minutes (0-59)
		// 11-15	Hours (0-23)
		// http://msdn.microsoft.com/en-us/library/windows/desktop/ms693800(v=vs.85).aspx

		ushort secDiv2 = msDosTime & 31;
		ushort min = (msDosTime >> 5) & 63;
		ushort hour = (msDosTime >> 11) & 31;

		return QTime(hour, min, secDiv2 * 2);
	}

	QDate DateTime::MsDosDateToQDate(ushort msDosDate)
	{
		// Bits		Description
		// 0-4		Days of the month (1-31)
		// 5-8		Months (1 = January, 2 = February, and so forth)
		// 9-15		Year offset from 1980 (add 1980 to get actual year)
		// http://msdn.microsoft.com/en-us/library/windows/desktop/ms693800(v=vs.85).aspx

		ushort day = msDosDate & 31;
		ushort month = (msDosDate >> 5) & 15;
		ushort year = (msDosDate >> 9) & 127;

		return QDate(1980 + year, month, day);
	}

	ushort DateTime::QTimeToMsDosTime(const QTime& qTime)
	{
		// Bits		Description
		// 0-4		Seconds divided by 2
		// 5-10		Minutes (0-59)
		// 11-15	Hours (0-23)
		// http://msdn.microsoft.com/en-us/library/windows/desktop/ms693800(v=vs.85).aspx

		ushort sec = static_cast<ushort>(qTime.second()) / 2;
		ushort min = static_cast<ushort>(qTime.minute());
		ushort hour = static_cast<ushort>(qTime.hour());

		ushort msDosTime = hour;
		msDosTime = (msDosTime << 6) | min;
		msDosTime = (msDosTime << 5) | sec;

		return msDosTime;
	}

	ushort DateTime::QDateToMsDosDate(const QDate& qDate)
	{
		// Bits		Description
		// 0-4		Days of the month (1-31)
		// 5-8		Months (1 = January, 2 = February, and so forth)
		// 9-15		Year offset from 1980 (add 1980 to get actual year)
		// http://msdn.microsoft.com/en-us/library/windows/desktop/ms693800(v=vs.85).aspx

		ushort day = static_cast<ushort>(qDate.day());
		ushort month = static_cast<ushort>(qDate.month());
		ushort year = static_cast<ushort>(qDate.year()) - 1980;

		ushort msDosDate = year;
		msDosDate = (msDosDate << 4) | month;
		msDosDate = (msDosDate << 5) | day;

		return msDosDate;
	}

	// default is a range from -INF -> + INF
	DateTimeRange::DateTimeRange()
	{
		mHasStart = mHasEnd = false;
	}

	// either start or end may be NULL, implying a time of +/- INF
	// otherwise the time is copied into the range
	DateTimeRange::DateTimeRange(const DateTime *start, const DateTime *end)
	{
		mHasStart = (start != NULL);

		if(mHasStart)
			mStart = *start;

		mHasEnd = (end != NULL);
		if(mHasEnd)
			mEnd = *end;
	}

	DateTimeRange::~DateTimeRange()
	{
	}

	// returns:
	// 0 if within range (inclusive)
	// -1 if before range
	//  1 if after range
	// NOTE: should probably define a BEFORE_RANGE, IN_RANGE, AFTER_RANGE enumeration...
	int DateTimeRange::OutsideRange(const DateTime &time) const
	{
		if(mHasStart && time < mStart)
			return -1;

		if(mHasEnd && time > mEnd)
			return 1;

		return 0;
	}

	DateRange::DateRange()
	{
	}
	DateRange::DateRange(const Date *start, const Date *end)
	{
		SetStartDate(start);
		SetEndDate(end);
	}

	void DateRange::SetStartDate(const Date *start)
	{
		mStart = *start;
	}

	const Date *DateRange::GetStartDate() const
	{
		return &mStart;
	}

	void DateRange::SetEndDate(const Date *end)
	{
		mEnd = *end;
	}

	const Date *DateRange::GetEndDate() const
	{
		return &mEnd;
	}

	bool operator==(const DateRange& leftDateRange, const DateRange& rightDateRange)
	{
		return ((leftDateRange.mStart == rightDateRange.mStart) &&
			(leftDateRange.mEnd == rightDateRange.mEnd));
	}

} // namespace Downloader
} // namespace Origin

