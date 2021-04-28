package com.esn.geoip;

/**
 * User: ronnie
 * Date: 2011-06-22
 * Time: 13:48
 */
public interface Position
{
    float getLatitude();

    float getLongitude();

    String getAddress();
}
