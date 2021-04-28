package com.esn.geoip.util;

/**
 * User: ronnie
 * Date: 2011-06-27
 * Time: 16:51
 */
public class Utils
{
    public static boolean isUnitTestMode()
    {
        return Boolean.getBoolean("sonar.unittest");
    }
}
