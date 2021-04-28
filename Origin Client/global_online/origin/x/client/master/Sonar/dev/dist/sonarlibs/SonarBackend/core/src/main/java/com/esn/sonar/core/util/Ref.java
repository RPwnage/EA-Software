package com.esn.sonar.core.util;

/**
 * User: ronnie
 * Date: 2011-06-15
 * Time: 14:35
 */
public class Ref<T>
{
    private T myValue;

    public Ref()
    {
    }

    public Ref(T value)
    {
        myValue = value;
    }

    public boolean isNull()
    {
        return myValue == null;
    }

    public T get()
    {
        return myValue;
    }

    public void set(T value)
    {
        myValue = value;
    }

    public static <T> Ref<T> create(T value)
    {
        return new Ref<T>(value);
    }

    public String toString()
    {
        return getClass().getSimpleName() + "<" + myValue + ">";
    }
}
