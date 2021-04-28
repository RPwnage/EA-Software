package com.esn.sonartest.director;


import java.util.ArrayList;
import java.util.Collection;
import java.util.LinkedList;

public class Pool<T>
{
    private final LinkedList<T> itemList;

    public Pool(Collection<T> initialPool)
    {
        this.itemList = new LinkedList<T>(initialPool);
    }

    public Pool()
    {
        this.itemList = new LinkedList<T>();
    }

    public synchronized Collection<T> acquireAll()
    {
        ArrayList<T> retList = new ArrayList<T>(itemList);

        itemList.clear();

        return retList;
    }

    public synchronized Collection<T> acquireCount(int count)
    {
        ArrayList<T> retList = new ArrayList<T>(count);

        while (!itemList.isEmpty() && retList.size() < count)
        {
            retList.add(itemList.removeFirst());
        }

        return retList;
    }

    public synchronized T acquire()
    {
        if (itemList.size() == 0)
        {
            return null;
        }

        return itemList.removeFirst();
    }

    public synchronized T acquireSpecific(T item)
    {
        return itemList.remove(item) ? item : null;
    }

    public synchronized void release(T item)
    {
        itemList.addLast(item);
    }

    public synchronized void add(T item)
    {
        itemList.addLast(item);
    }

    public synchronized int size()
    {
        return itemList.size();
    }
}
