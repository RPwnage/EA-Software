/**
 *  TimeValue.java
 *
 *  (c) Electronic Arts. All Rights Reserved.
 *
 */
package ea.blaze.tdf;

import java.util.Calendar;
import java.util.Date;
import java.util.GregorianCalendar;

/**
 * Represents a time value, stored internally as microseconds 
 */
public class TimeValue 
{
	
	/**
	 * The TimeFormat.
	 */
	public enum TimeFormat {
		
		TIME_FORMAT_INTERVAL
	}
	
	/**
	 * Instantiates a new time value with 0 microsecnds.
	 */
	public TimeValue() {
		mTime = 0;
	}
	
	/**
	 * Instantiates a new time value from string containing the time in format TIME_FORMAT_INTERVAl
	 */
	public TimeValue(String timeInterval) {
		mTime = 0;
		parseTimeInterval(timeInterval);
	}
	
	/**
	 * Instantiates a new time value from a string containing the time interval in the specified format.
	 * 
	 * @param timeInterval
	 *            the time interval
	 * @param format
	 *            the format
	 */
	public TimeValue(String timeInterval, TimeFormat format) {
		mTime = 0;
	    switch (format)
	    {
	        case TIME_FORMAT_INTERVAL:
	            parseTimeInterval(timeInterval);
	        break;
	        default: //nothing 
	        break;        
	    };
	}
	
	/**
	 * Instantiates a new time value from the specified value of microseconds.
	 * 
	 * @param time_us
	 *            the microseconds for this timevalue.
	 */
	public TimeValue(long time_us) {
		mTime = time_us;
	}
	
	/**
	 * Instantiates a new time value from the java Date class.
	 * 
	 * @param d
	 *            the date to instantiate this TimeValue with.
	 */
	public TimeValue(Date d) {
		setDate(d);
	}
	
	/**
	 * Instantiates a new time value from an existing TimeValue.
	 * 
	 * @param rhs
	 *            the time value to instantiate from.
	 */
	public TimeValue(TimeValue rhs) {
		mTime = rhs.mTime;
	}
	
	/**
	 * Sets the time value to the existing time value.
	 * 
	 * @param rhs
	 *            the time value to set.
	 */
	public void set(TimeValue rhs) {
		mTime = rhs.mTime;
	}
	
	/**
	 * Sets the time value to the specified microseconds.
	 * 
	 * @param time_us
	 *            the microseconds to set.
	 */
	public void set(long time_us) {
		mTime = time_us;
	}
	
	/**
	 * Gets the Java Date class for this time value.
	 * 
	 * @return the date represented by this time value.
	 */
	public Date getDate() {
		long time_ms = mTime/1000; // convert to milliseconds
		return new Date(time_ms);
	}
	
	/**
	 * Sets this time value to the value in the Java Date class.
	 * 
	 * @param d
	 *            the new date to set this time value to.
	 */
	public void setDate(Date d) {
		long time_ms = d.getTime(); // Milliseconds
		mTime = time_ms * 1000; // convert to microseconds;
	}
	
    /**
	 * Gets the micro seconds for this time value.
	 * 
	 * @return the micro seconds
	 */
    public long getMicroSeconds() { return mTime; }
    
    /**
	 * Gets the milliseconds for this time value.
	 * 
	 * @return the milliseconds
	 */
    public long getMillis() { return mTime / 1000; }
    
    /**
	 * Gets the seconds for this time value.
	 * 
	 * @return the seconds
	 */
    public long getSec() { return mTime / 1000000; }

	
	/* (non-Javadoc)
	 * @see java.lang.Object#toString()
	 */
	public String toString()
	{
	    long year = 0L, month = 0L, day = 0L, hour = 0L, minute = 0L, second = 0L, millis = 0L;
	    
		GregorianCalendar cal = new GregorianCalendar();
		long time_millis = mTime/1000; // convert usec to millis.
		cal.setTime(new Date(time_millis));
		
		year   = (long) cal.get(Calendar.YEAR);
		month  = (long) cal.get(Calendar.MONTH)+1;
		day    = (long) cal.get(Calendar.DAY_OF_MONTH);
		hour   = (long) cal.get(Calendar.HOUR_OF_DAY);
		minute = (long) cal.get(Calendar.MINUTE);
		second = (long) cal.get(Calendar.SECOND);
		millis = (long) cal.get(Calendar.MILLISECOND);
	    
	    String ret = String.format("%d/%02d/%02d-%02d:%02d:%02d.%03d", year, month, day,
	        hour, minute, second, millis);
	    return ret;
	}
	
	/**
	 *  Parses time interval string in format like: "300d:17h:33m:59s:200ms".
	 *  
	 *  Caller can provide any combination of time components:
	 *      300d:17h:33m
	 *      300d:17h
	 *      33m:59s
	 *      300d
	 *      17h
	 *      33m
	 *      59s
	 *      200ms
	 *      
	 *  Finally the caller can 'overflow' the component's natural modulo if it is convenient:
	 *      48h
	 *      600m
	 *      
	 *  NOTE: The maximum valid number of digits in a time component is 10, i.e.: 1234567890s
	 *  
	 *  @param str The input string
	 *  @return True if successfully parsed.
	 */
	public boolean parseTimeInterval(String str)
	{
	    long  val, days, hours, minutes, seconds, milliseconds;
	    long max = Long.MAX_VALUE / 10;
	    long rem = Long.MAX_VALUE % 10;
	    val = days = hours = minutes = seconds = milliseconds = 0;
	    char ch;
	    int curIndex = 0;
	    while (curIndex < str.length())
	    {
	        ch = str.charAt(curIndex++);
	        if ((ch >= '0') && (ch <= '9'))
	        {
	            ch -= '0';
	            if ((val < max) || ((val == max) && (ch <= rem)))
	            {
	                val = (val * 10) + ch;
	                continue;
	            }
	            // overflow detected
	            return false;
	        }
	        
	        switch(ch)
	        {
	        case 'd':
	            days = val;
	            break;
	        case 'h':
	            hours = val;
	            break;
	        case 'm':
	            if(str.length() > curIndex && str.charAt(curIndex) == 's')
	            {
	                curIndex++; // consume 's'
	                milliseconds = val;
	            }
	            else
	                minutes = val;
	            break;
	        case 's':
	            seconds = val;
	            break;
	        default:
	            return false; // encountered unhandled token, or early '\0'
	        }
	        
	        if (curIndex >= str.length())
	            break;
	        
	        val = 0; // reset the value
	        
	        if(str.charAt(curIndex) == ':')
	            curIndex++; // consume ':'
	    }
	    
	    mTime = ((((days*24L + hours)*60L + minutes)*60L + seconds)*1000L + milliseconds)*1000L;
	    
	    return true;
	}

	
	/** The time in microseconds. */
	private long mTime;
}
