package com.esn.geoip.util;

/**
 * User: ronnie
 * Date: 2011-06-20
 * Time: 15:29
 */
public interface SearchProcessor<T>
{
    /**
     * Process an object
     *
     * @param t the object
     * @return <tt>true</tt> if processing should continue, <tt>false</tt> otherwise</tt>
     */
    boolean execute(T t);
}
