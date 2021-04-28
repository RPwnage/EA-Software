package com.esn.geoip;

import java.util.Map;
import java.util.concurrent.ConcurrentSkipListMap;

public class BitTree<E>
{
    private final ConcurrentSkipListMap<Long, E> map = new ConcurrentSkipListMap<Long, E>();

    public E find(long value)
    {
        Map.Entry<Long, E> lowEntry = map.floorEntry(value);
        Map.Entry<Long, E> highEntry = map.ceilingEntry(value);

        if (lowEntry == null || highEntry == null)
        {
            return null;
        }

        if (lowEntry.getValue() == highEntry.getValue())
        {
            return lowEntry.getValue();
        }

        return null;
    }

    public int size()
    {
        return map.size();
    }


    public void add(long start, long end, E value)
    {
        map.put(start, value);
        if (start != end)
        {
            map.put(end, value);
        }
    }
}
