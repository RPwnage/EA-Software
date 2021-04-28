package com.esn.sonar.core.util;

/**
 * User: ronnie
 * Date: 2011-06-22
 * Time: 13:54
 */
public interface Validator<T>
{
    public boolean verify(T item);
}
