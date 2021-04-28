package com.esn.geoip;


/**
 * User: ronnie
 * Date: 2011-06-22
 * Time: 15:47
 */
public class PositionObject<T> implements Positionable
{
    private final T object;
    private final Position position;

    public PositionObject(Position position, T object)
    {
        this.object = object;
        this.position = position;
    }

    public Position getPosition()
    {
        return position;
    }

    public T getObject()
    {
        return object;
    }

    @Override
    public boolean equals(Object o)
    {
        if (this == o) return true;
        if (!(o instanceof PositionObject)) return false;

        PositionObject that = (PositionObject) o;

        if (object != null ? !object.equals(that.object) : that.object != null) return false;
        if (position != null ? !position.equals(that.position) : that.position != null) return false;

        return true;
    }

    @Override
    public int hashCode()
    {
        int result = object != null ? object.hashCode() : 0;
        result = 31 * result + (position != null ? position.hashCode() : 0);
        return result;
    }
}
