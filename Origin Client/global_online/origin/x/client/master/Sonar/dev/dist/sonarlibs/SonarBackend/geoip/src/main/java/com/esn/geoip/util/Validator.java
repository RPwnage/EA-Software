package com.esn.geoip.util;

import com.esn.geoip.Positionable;

/**
 * User: ronnie
 * Date: 2011-06-24
 * Time: 10:33
 */
public interface Validator<T extends Positionable>
{
    public boolean verify(T item);
}
