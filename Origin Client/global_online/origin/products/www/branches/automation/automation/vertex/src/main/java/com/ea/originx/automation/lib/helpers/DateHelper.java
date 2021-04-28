package com.ea.originx.automation.lib.helpers;

import java.text.ParseException;
import java.text.SimpleDateFormat;
import java.util.Calendar;
import java.util.Date;
import java.util.TimeZone;

/**
 * Helper for getting the date. Will format dates before you get them.
 *
 * @author glivingstone
 * @author mdobre
 */
public class DateHelper {

    /**
     * Get the current day of the month, normalized to GMT
     *
     * @return the numbered day of the month in GMT
     */
    public static int getDayOfMonthGMT() {
        SimpleDateFormat dayFormat = new SimpleDateFormat("dd");
        dayFormat.setTimeZone(TimeZone.getTimeZone("GMT"));
        return Integer.parseInt(dayFormat.format(Calendar.getInstance().getTime()));
    }

    /**
     * Get the month as a word, normalized to GMT
     *
     * @return the month in GMT
     */
    public static String getMonthNameGMT() {
        SimpleDateFormat monthFormat = new SimpleDateFormat("MMMM");
        monthFormat.setTimeZone(TimeZone.getTimeZone("GMT"));
        return monthFormat.format(Calendar.getInstance().getTime());
    }

    /**
     * Get the next month as a word, normalized to GMT
     *
     * @return the next month in GMT
     */
    public static String getNextMonthNameGMT() {
        SimpleDateFormat monthFormat = new SimpleDateFormat("MMMM");
        Calendar cal = Calendar.getInstance();
        monthFormat.setTimeZone(TimeZone.getTimeZone("GMT"));
        int nextMonth = cal.get(Calendar.MONTH) + 1;
        cal.set(Calendar.MONTH, nextMonth);
        return monthFormat.format(cal.getTime());
    }

    /**
     * Get the year, normalized to GMT
     *
     * @return the year in GMT
     */
    public static int getYearGMT() {
        SimpleDateFormat yearFormat = new SimpleDateFormat("YYYY");
        yearFormat.setTimeZone(TimeZone.getTimeZone("GMT"));
        return Integer.parseInt(yearFormat.format(Calendar.getInstance().getTime()));
    }

    /**
     * Get the the year in n years, normalized to GMT
     *
     * @param n the number of years ahead you would like
     * @return the year in GMT
     */
    public static int getYearPlusGMT(int n) {
        return getYearGMT() + n;
    }

    /**
     * Get the the date advanced one year in MMMM dd, YYYY+1 format, normalized
     * to GMT
     *
     * @return the current date in GMT
     */
    public static String getDatePlusOneYearGMT() {
        return getMonthNameGMT() + " " + getDayOfMonthGMT() + ", " + getYearPlusGMT(1);
    }

    /**
     * Get the current date in MMMM dd, YYYY format, normalized to GMT
     *
     * @return the current date in GMT
     */
    public static String getCurrentDateGMT() {
        return getMonthNameGMT() + " " + getDayOfMonthGMT() + ", " + getYearGMT();
    }
    
    /**
     * Verify a given date matches an expected format
     * @param date the date we need to check
     * @param expectedDateFormat the format the date needs to match
     * @return true if the date matches the format, false otherwise
     */
    public static boolean verifyDateMatchesExpectedFormat(String date, String expectedDateFormat) {
        try {
            SimpleDateFormat simpleDateFormat = new SimpleDateFormat(expectedDateFormat);
            Date dateGiven = simpleDateFormat.parse(date);
        } catch (ParseException e) {
            return false;
        }
        return true;
    }
}
