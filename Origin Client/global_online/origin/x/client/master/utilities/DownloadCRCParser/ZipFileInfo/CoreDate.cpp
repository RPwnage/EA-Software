//
//	date class.
//
//	Copyright 2004 Electronic Arts Inc.  All rights reserved.
//

#include "CoreDate.h"
#include "time.h"

#define FALSE 0
#define TRUE 1
#define CORE_ASSERT(x) ((void)0);

namespace Utilities
{
	__int64 GetCurrentSeconds()
	{
		__time64_t curTime;
		_time64(&curTime);
		return (__int64)curTime;
	}
}

//namespace Core
//{

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

	CoreDate::CoreDate()
	{
		SetDate(CORE_DATE_MONTH_JAN, 1, CORE_BASE_YEAR);
	}


	int CoreDate::SetDate(CoreDateMonth month, int day, int year)
	{
		int err = DATE_SUCCESS;

		err = SetMonth(month);
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

	int CoreDate::SetMonth(CoreDateMonth month)
	{
		if (month > 0 && month < 13)
		{
			mMonth = month;
			return DATE_SUCCESS;
		}
		return DATE_ERR_MONTH_OUT_OF_RANGE;
	}


	int CoreDate::SetDay(int day)
	{
		int maxMonthDay = 0;
		switch(mMonth)
		{
		case CORE_DATE_MONTH_FEB:
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
		case CORE_DATE_MONTH_APR:
		case CORE_DATE_MONTH_JUN:
		case CORE_DATE_MONTH_SEP:
		case CORE_DATE_MONTH_NOV:
			{
				maxMonthDay = 30;
				break;
			}
		case CORE_DATE_MONTH_JAN:
		case CORE_DATE_MONTH_MAR:
		case CORE_DATE_MONTH_MAY:
		case CORE_DATE_MONTH_JUL:
		case CORE_DATE_MONTH_AUG:
		case CORE_DATE_MONTH_OCT:
		case CORE_DATE_MONTH_DEC:
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

	int CoreDate::SetYear(int year)
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
	int CoreDate::SetDate(const QString sDate)
	{
		int day, year, month, itemsParsed;
		day = 1;
		year = 1900;
		month = 1;
        itemsParsed = swscanf_s((const wchar_t*)sDate.utf16(), L"%d-%2d-%2d", &year, &month, &day);
		if (itemsParsed != 3)
		{
			//EBILOGERROR << L"Only parsed " << itemsParsed << L" items in date " << sDate;
			CORE_ASSERT(FALSE);
		}

		return SetDate(static_cast<CoreDateMonth>(month), day, year);
	}

	/**
	* Set the date/time from the Unix epoch time format (in UTC)
	*
	*	See CoreDateTime::ConvertToSeconds
	*/
	int CoreDateTime::SetDate(__int64 timeInSeconds)
	{
		struct tm dateTime;

		__time64_t secs = (__time64_t)timeInSeconds;
		if (_gmtime64_s(&dateTime, &secs))  // Non-zero on error
			//	likely because time was negative for some reason.
			return DATE_FAIL;

		SetDate((CoreDateMonth)(dateTime.tm_mon+1), dateTime.tm_mday, dateTime.tm_year+1900, 
			dateTime.tm_hour, dateTime.tm_min, dateTime.tm_sec);

		return DATE_SUCCESS;
	}

	// Decodes an ISO 8601 date and time
	// This is in the format YYYY-MM-DDTHH:MM:SS and will ignore the
	// optional time zone component
	int CoreDateTime::SetISO8601DateTime(const QString sDate, bool bAllowDateOnly)
	{
		unsigned int year, month, day, 
					 hour, minute, seconds;

		year = 1900;
		month = 1;
		day = 1;
		hour = 0;
		minute = 0;
		seconds = 0;

        int itemsParsed = swscanf_s((const wchar_t*)sDate.utf16(), L"%u-%2u-%2uT%u:%2u:%2u",
				&year, &month, &day,
				&hour, &minute, &seconds);

		// Datetime is 6 items and just date is 3
		if (!((itemsParsed == 6) || ((itemsParsed == 3) && bAllowDateOnly)))
		{
			CORE_ASSERT(FALSE);
		}

		return SetDate(static_cast<CoreDateMonth>(month), day, year, hour, minute, seconds); 
	}

	/**
	* Returns the number of seconds since the Unix Epoch (UTC)
	*
	*	See CoreDateTime::SetDate(__int64 timeInSeconds)
	*/
	__int64 CoreDateTime::ConvertToSeconds() const
	{
		//	using C runtime mktime.
		struct tm dateTime;

		dateTime.tm_year = mYear-1900;
		dateTime.tm_mon = mMonth-1;
		dateTime.tm_mday = mDay;
		dateTime.tm_hour = mHour;
		dateTime.tm_min = mMin;
		dateTime.tm_sec = mSec;
		dateTime.tm_isdst = 0;		// standard time (no daylight savings time conversion.)

		__time64_t secs = _mktime64(&dateTime);
		// mktime converts to the local zone, so we need to convert back to UTC.
		long timezoneAdj;
		_get_timezone(&timezoneAdj);
		secs -= timezoneAdj;

		return (__int64)secs;
	}

	bool operator==(const CoreDate& leftDate, const CoreDate& rightDate)
	{
		return ((leftDate.mMonth == rightDate.mMonth) &&
			(leftDate.mDay == rightDate.mMonth) &&
			(leftDate.mYear == rightDate.mYear));
	}


	bool operator!=( const CoreDate& leftDate, const CoreDate& rightDate)
	{
		return !(leftDate == rightDate);
	}


	bool operator<( const CoreDate& leftDate, const CoreDate& rightDate)
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

	bool operator<=( const CoreDate& leftDate, const CoreDate& rightDate)
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


	bool operator>( const CoreDate& leftDate, const CoreDate& rightDate)
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


	bool operator>=( const CoreDate& leftDate, const CoreDate& rightDate)
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
	CoreDateTime::CoreDateTime() : 
	mMonth(CORE_DATE_MONTH_JAN),
		mDay(1),
		mYear(CORE_BASE_YEAR),
		mHour(0),
		mMin(0),
		mSec(0),
		mHasBeenSet(false)
	{
	}

	CoreDateTime::~CoreDateTime()
	{
	}

	int CoreDateTime::SetDate( CoreDateMonth month, int day, int year, int hour, int min, int sec )
	{
		mHasBeenSet = false;
		int err = DATE_SUCCESS;

		err = SetMonth(month);
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
	void CoreDateTime::SetToNow()
	{
		__int64 curTime = Utilities::GetCurrentSeconds();
		SetDate(curTime);
	}

	int CoreDateTime::GetDaysInMonth( void ) const
	{
		int nResult = 0;

		switch ( mMonth )
		{
		case CORE_DATE_MONTH_FEB:
			{
				if ( IsLeapYear( mYear ) ) { nResult = 29; }
				else					   { nResult = 28; }
				break;
			}
		case CORE_DATE_MONTH_APR:
		case CORE_DATE_MONTH_JUN:
		case CORE_DATE_MONTH_SEP:
		case CORE_DATE_MONTH_NOV: { nResult = 30; break; }
		case CORE_DATE_MONTH_JAN:
		case CORE_DATE_MONTH_MAR:
		case CORE_DATE_MONTH_MAY:
		case CORE_DATE_MONTH_JUL:
		case CORE_DATE_MONTH_AUG:
		case CORE_DATE_MONTH_OCT:
		case CORE_DATE_MONTH_DEC: { nResult = 31; break; }
		default:				  { nResult = DATE_ERR_MONTH_OUT_OF_RANGE; break; }
		}

		return nResult;
	}

	// return the number of seconds remaining from 'now' until 'datetime'.
	// once the datetime has passed, the countdown will return zero.
	// is not aware of the timezone setting (assumes that this datetime is UTC)
	__int64 CoreDateTime::GetCountdownSeconds() const
	{
		__int64 curTime = Utilities::GetCurrentSeconds();
		__int64 thisTime = ConvertToSeconds();
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
	void CoreDateTime::GetCountdownMDH( long & nMonths, long & nDays, long & nHours )
	{
		const long NHOURS_IN_DAY   = 24;
		const long NMONTHS_IN_YEAR = 12;

		long nNowYear,     nNowMonth,     nNowDay,     nNowHour,	nNowDaysInMonth;
		long nEndYear,     nEndMonth,     nEndDay,     nEndHour;
		long nRawYearDiff, nRawMonthDiff, nRawDayDiff, nRawHourDiff;

		long nExtraHours  = 1; // +1 hour off the bat for rounding up.
		long nExtraDays   = 0; // Additions from hours overflow.  
		long nExtraMonths = 0; // Additions from days overflow.

		CoreDateTime now;

		// Only compute differences if *now* is less than *this* date.
		if ( now < *this )
		{
			// ------------------------------
			//  Get the date parts of *now*.
			//

			now.SetToNow();
			nNowYear  = now.GetYear();
			nNowMonth = ( long ) now.GetMonth();
			nNowDay   = now.GetDay();
			nNowHour  = now.GetHour();
			nNowDaysInMonth = now.GetDaysInMonth();


			// ------------------------------------
			//  Get the date parts of *this* date.
			//

			nEndYear  = GetYear();
			nEndMonth = ( long ) GetMonth();
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

			if ( ( nNowYear == nEndYear ) )
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
	void CoreDateTime::GetCountdownDH( long & nDays, long & nHours )
	{
		const long NHOURS_IN_DAY = 24;
		const long NSECS_IN_MIN  = 60;
		const long NMINS_IN_HOUR = 60;

		__int64 nSeconds    = GetCountdownSeconds();
		long    nExtraHours = 1; // +1 hour off the bat for rounding up.

		// Defaults.
		nDays   = 0;
		nHours  = 0;

		// Only compute differences if *now* is less than *this* date.
		if ( nSeconds )
		{
			// ---------------------------------
			//  Calculate the hours difference.
			//

			nHours = ( long ) ( nSeconds / NSECS_IN_MIN / NMINS_IN_HOUR ) + nExtraHours;


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

	void CoreDateTime::GetCountdownDHM( long & nDays, long & nHours, long & nMinutes )
	{
		static const long NHOURS_IN_DAY = 24;
		static const long NSECS_IN_MIN  = 60;
		static const long NMINS_IN_HOUR = 60;

		__int64 nSeconds = GetCountdownSeconds();

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
			nMinutes = ( long ) ( nSeconds / NSECS_IN_MIN );

			// Round to at least one minute
			if ( nMinutes == 0 )
				nMinutes = 1;

			// ---------------------------------
			//  Calculate the hours difference.
			//
			if ( nMinutes >= NMINS_IN_HOUR )
			{
				nHours = ( long ) ( nMinutes / NMINS_IN_HOUR );
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

	int CoreDateTime::SetMonth(CoreDateMonth month)
	{
		if (month > 0 && month < 13)
		{
			mMonth = month;
			return DATE_SUCCESS;
		}
		return DATE_ERR_MONTH_OUT_OF_RANGE;
	}


	int CoreDateTime::SetDay(int day)
	{
		int maxMonthDay = 0;
		switch(mMonth)
		{
		case CORE_DATE_MONTH_FEB:
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
		case CORE_DATE_MONTH_APR:
		case CORE_DATE_MONTH_JUN:
		case CORE_DATE_MONTH_SEP:
		case CORE_DATE_MONTH_NOV:
			{
				maxMonthDay = 30;
				break;
			}
		case CORE_DATE_MONTH_JAN:
		case CORE_DATE_MONTH_MAR:
		case CORE_DATE_MONTH_MAY:
		case CORE_DATE_MONTH_JUL:
		case CORE_DATE_MONTH_AUG:
		case CORE_DATE_MONTH_OCT:
		case CORE_DATE_MONTH_DEC:
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

	int CoreDateTime::SetYear(int year)
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

	int CoreDateTime::GetMonthFromStr( const char *monthStr, CoreDateMonth& month )
	{
		if (_stricmp(monthStr, "JAN") == 0)
		{
			month = CORE_DATE_MONTH_JAN;
		}
		else if (_stricmp(monthStr, "FEB") == 0)
		{
			month = CORE_DATE_MONTH_FEB;
		}
		else if(_stricmp(monthStr, "MAR") == 0)
		{
			month = CORE_DATE_MONTH_MAR;
		}
		else if(_stricmp(monthStr, "APR") == 0)
		{
			month = CORE_DATE_MONTH_APR;
		}
		else if(_stricmp(monthStr, "MAY") == 0)
		{
			month = CORE_DATE_MONTH_MAY;
		}
		else if(_stricmp(monthStr, "JUN") == 0)
		{
			month = CORE_DATE_MONTH_JUN;
		}
		else if(_stricmp(monthStr, "JUL") == 0)
		{
			month = CORE_DATE_MONTH_JUL;
		}
		else if(_stricmp(monthStr, "AUG") == 0)
		{
			month = CORE_DATE_MONTH_AUG;
		}
		else if(_stricmp(monthStr, "SEP") == 0)
		{
			month = CORE_DATE_MONTH_SEP;
		}
		else if(_stricmp(monthStr, "OCT") == 0)
		{
			month = CORE_DATE_MONTH_OCT;
		}
		else if(_stricmp(monthStr, "NOV") == 0)
		{
			month = CORE_DATE_MONTH_NOV;
		}
		else if(_stricmp(monthStr, "DEC") == 0)
		{
			month = CORE_DATE_MONTH_DEC;
		}
		else 
		{
			return DATE_ERR_MONTH_OUT_OF_RANGE;
		}

		return DATE_SUCCESS;
	}


	// Decodes a date in a string formatted as "YYYY-MM-DD HH:MM:SS GMT" (ex: 2008-05-01 00:00:00 GMT)
	// This is proxy server GetContent response's date/time format.
	//
	int CoreDateTime::SetDate(const QString sDate)
	{
		mHasBeenSet = false;

		// elements must be defaulted, because date in string may vary.
		int day = 1;
		int month = 1;
		int year = 1900;
		int hour = 0;
		int min = 0;
		int sec = 0;
		wchar_t buf[256];
		int itemsParsed = 0;

        itemsParsed = swscanf_s((const wchar_t*)sDate.utf16(), L"%d-%2d-%2d %2d:%2d:%2d %s", &year, &month, &day, &hour, &min, &sec, &buf, _countof(buf));

		if (itemsParsed != 7)
		{
			//EBILOGERROR << L"Only parsed " << itemsParsed << L" items in date " << sDate);
			CORE_ASSERT(FALSE);
		}

		return SetDate( static_cast<CoreDateMonth>(month), day, year, hour, min, sec);
	}

	const CoreDateTime& CoreDateTime::operator=( const CoreDateTime& rhs )
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

	bool CoreDateTime::operator<( const CoreDateTime& rhs ) const
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


	bool CoreDateTime::operator<=( const CoreDateTime& rhs ) const
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


	bool CoreDateTime::operator>( const CoreDateTime& rhs ) const
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


	bool CoreDateTime::operator>=( const CoreDateTime& rhs ) const
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

	bool CoreDateTime::operator==( const CoreDateTime& rhs ) const
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


	bool CoreDateTime::operator!=( const CoreDateTime& rhs ) const
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


	int CoreDateTime::SetHour(int hour)
	{
		if ( (hour >= 0) && (hour <= DATE_MAX_HOUR))
		{
			mHour = hour;
			return DATE_SUCCESS;
		}

		return DATE_ERR_HOUR_OUT_OF_RANGE;
	}

	int CoreDateTime::SetMin(int min)
	{
		if ( (min >= 0) && (min <= DATE_MAX_MINUTE))
		{
			mMin = min;
			return DATE_SUCCESS;
		}

		return DATE_ERR_MINUTE_OUT_OF_RANGE;
	}

	int CoreDateTime::SetSec(int sec)
	{
		if ( (sec >= 0) && (sec <= DATE_MAX_SECOND))
		{
			mSec = sec;
			return DATE_SUCCESS;
		}

		return DATE_ERR_SECOND_OUT_OF_RANGE;
	}

	bool CoreDateTime::InRange(const CoreDateTimeRange& range) const
	{
		return (range.OutsideRange(*this) == 0);
	}

	bool CoreDateTime::BeforeRange(const CoreDateTimeRange& range) const
	{
		return (range.OutsideRange(*this) < 0);
	}

	bool CoreDateTime::AfterRange(const CoreDateTimeRange& range) const
	{
		return (range.OutsideRange(*this) > 0);
	}

	bool CoreDateTime::AdjustUsingReferenceClock(const CoreDateTime &ref)
	{
		CoreDateTime local;
		local.SetToNow();

		if (!ref.IsValid() || !local.IsValid())
		{
			return false;
		}

		// Calculate our clock skew
		// Negative means we're slow, positive means we're fast
		__int64 iClockSkew = local.ConvertToSeconds() - ref.ConvertToSeconds();

		// Re-set our date using the clock kew
		SetDate(ConvertToSeconds() + iClockSkew);

		return true;
	}

	void CoreDateTime::ToString(char *buf, size_t bufSize) const
	{
		_snprintf_s(buf, bufSize, bufSize, "%d-%d-%d %02d:%02d:%02d", GetMonth(), 
			GetDay(), GetYear(), GetHour(), GetMin(), GetSecond());
	}

	void CoreDateTime::ToString(wchar_t *buf, size_t bufSize) const
	{
		_snwprintf_s(buf, bufSize, bufSize, L"%d-%d-%d %02d:%02d:%02d", GetMonth(), 
			GetDay(), GetYear(), GetHour(), GetMin(), GetSecond());
	}

	QTime CoreDateTime::MsDosTimeToQTime(unsigned short msDosTime)
	{
		// Bits		Description
		// 0-4		Seconds divided by 2
		// 5-10		Minutes (0-59)
		// 11-15	Hours (0-23)
		// http://msdn.microsoft.com/en-us/library/windows/desktop/ms693800(v=vs.85).aspx

		unsigned short secDiv2 = msDosTime & 31;
		unsigned short min = (msDosTime >> 5) & 63;
		unsigned short hour = (msDosTime >> 11) & 31;

		return QTime(hour, min, secDiv2 * 2);
	}

	QDate CoreDateTime::MsDosDateToQDate(unsigned short msDosDate)
	{
		// Bits		Description
		// 0-4		Days of the month (1-31)
		// 5-8		Months (1 = January, 2 = February, and so forth)
		// 9-15		Year offset from 1980 (add 1980 to get actual year)
		// http://msdn.microsoft.com/en-us/library/windows/desktop/ms693800(v=vs.85).aspx

		unsigned short day = msDosDate & 31;
		unsigned short month = (msDosDate >> 5) & 15;
		unsigned short year = (msDosDate >> 9) & 127;

		return QDate(1980 + year, month, day);
	}

	unsigned short CoreDateTime::QTimeToMsDosTime(const QTime& qTime)
	{
		// Bits		Description
		// 0-4		Seconds divided by 2
		// 5-10		Minutes (0-59)
		// 11-15	Hours (0-23)
		// http://msdn.microsoft.com/en-us/library/windows/desktop/ms693800(v=vs.85).aspx

		unsigned short sec = static_cast<unsigned short>(qTime.second()) / 2;
		unsigned short min = static_cast<unsigned short>(qTime.minute());
		unsigned short hour = static_cast<unsigned short>(qTime.hour());

		unsigned short msDosTime = hour;
		msDosTime = (msDosTime << 6) | min;
		msDosTime = (msDosTime << 5) | sec;

		return msDosTime;
	}

	unsigned short CoreDateTime::QDateToMsDosDate(const QDate& qDate)
	{
		// Bits		Description
		// 0-4		Days of the month (1-31)
		// 5-8		Months (1 = January, 2 = February, and so forth)
		// 9-15		Year offset from 1980 (add 1980 to get actual year)
		// http://msdn.microsoft.com/en-us/library/windows/desktop/ms693800(v=vs.85).aspx

		unsigned short day = static_cast<unsigned short>(qDate.day());
		unsigned short month = static_cast<unsigned short>(qDate.month());
		unsigned short year = static_cast<unsigned short>(qDate.year()) - 1980;

		unsigned short msDosDate = year;
		msDosDate = (msDosDate << 4) | month;
		msDosDate = (msDosDate << 5) | day;

		return msDosDate;
	}

	// default is a range from -INF -> + INF
	CoreDateTimeRange::CoreDateTimeRange()
	{
		mHasStart = mHasEnd = false;
	}

	// either start or end may be NULL, implying a time of +/- INF
	// otherwise the time is copied into the range
	CoreDateTimeRange::CoreDateTimeRange(const CoreDateTime *start, const CoreDateTime *end)
	{
		mHasStart = (start != NULL);

		if(mHasStart)
			mStart = *start;

		mHasEnd = (end != NULL);
		if(mHasEnd)
			mEnd = *end;
	}

	CoreDateTimeRange::~CoreDateTimeRange()
	{
	}

	// returns:
	// 0 if within range (inclusive)
	// -1 if before range
	//  1 if after range
	// NOTE: should probably define a BEFORE_RANGE, IN_RANGE, AFTER_RANGE enumeration...
	int CoreDateTimeRange::OutsideRange(const CoreDateTime &time) const
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
	DateRange::DateRange(const CoreDate *start, const CoreDate *end)
	{
		SetStartDate(start);
		SetEndDate(end);
	}

	void DateRange::SetStartDate(const CoreDate *start)
	{
		mStart = *start;
	}

	const CoreDate *DateRange::GetStartDate() const
	{
		return &mStart;
	}

	void DateRange::SetEndDate(const CoreDate *end)
	{
		mEnd = *end;
	}

	const CoreDate *DateRange::GetEndDate() const
	{
		return &mEnd;
	}

	bool operator==(const DateRange& leftDateRange, const DateRange& rightDateRange)
	{
		return ((leftDateRange.mStart == rightDateRange.mStart) &&
			(leftDateRange.mEnd == rightDateRange.mEnd));
	}
//}
