package com.esn.sonar.allinone;

import org.junit.Test;

import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.locks.ReentrantReadWriteLock;

import static org.junit.Assert.*;

/**
 * Created by IntelliJ IDEA.
 * User: jonas
 * Date: 2010-nov-23
 * Time: 14:57:06
 * To change this template use File | Settings | File Templates.
 */
public class StupidTests
{
    @Test
    public void putIfAbsentTest()
    {
        ConcurrentHashMap<String, String> testMap = new ConcurrentHashMap<String, String>();

        String key = "test";
        String value1 = "value1";
        String value2 = "value2";

        assertNull("Put if absent first time should be null", testMap.putIfAbsent(key, value1));
        assertEquals("Value1 should still be stored", value1, testMap.putIfAbsent(key, value2));
    }

    static void functionThatReturns(ReentrantReadWriteLock lock)
    {
        try
        {
            lock.writeLock().lock();
            return;
        } finally
        {
            lock.writeLock().unlock();
        }
    }

    static void functionThatThrows(ReentrantReadWriteLock lock) throws InterruptedException
    {
        try
        {
            lock.writeLock().lock();
            throw new InterruptedException("Test");
        } finally
        {
            lock.writeLock().unlock();
        }
    }


    @Test
    public void readWriteLockFunctionReturn()
    {
        ReentrantReadWriteLock lock = new ReentrantReadWriteLock();

        functionThatReturns(lock);
        assertFalse("Write lock should have been unlocked", lock.writeLock().isHeldByCurrentThread());
    }

    @Test
    public void readWriteLockException()
    {
        ReentrantReadWriteLock lock = new ReentrantReadWriteLock();

        try
        {
            functionThatThrows(lock);
        } catch (InterruptedException e)
        {
        }

        assertFalse("Write lock should have been unlocked", lock.writeLock().isHeldByCurrentThread());
    }


    private void writeLockUpgrade(ReentrantReadWriteLock rwl, boolean throwInReadLock, boolean throwInWriteLock) throws Exception
    {
        boolean value = false;

        rwl.readLock().lock();

        try
        {

            if (!value)
            {
                rwl.readLock().unlock();   // must unlock first to obtain writelock
                rwl.writeLock().lock();

                try
                {

                    if (throwInWriteLock)
                    {
                        throw new RuntimeException("Inside writelock");
                    }

                    if (!value)
                    {
                        value = true;
                    }

                } finally
                {
                    rwl.readLock().lock();  // reacquire read without giving up write lock
                    rwl.writeLock().unlock(); // unlock write, still hold read
                }
            }

            if (throwInReadLock)
            {
                throw new RuntimeException("Inside readlock");
            }
        } finally
        {
            rwl.readLock().unlock();
        }

    }

    @Test
    public void readWriteLockUpgrade()
    {
        ReentrantReadWriteLock rwl = new ReentrantReadWriteLock();
        try
        {
            writeLockUpgrade(rwl, false, false);
        } catch (Exception e)
        {
        }
        assertEquals("Read lock", 0, rwl.getReadHoldCount());
        assertEquals("Write lock", 0, rwl.getWriteHoldCount());

        try
        {
            writeLockUpgrade(rwl, false, true);
        } catch (Exception e)
        {
        }
        assertEquals("Read lock", 0, rwl.getReadHoldCount());
        assertEquals("Write lock", 0, rwl.getWriteHoldCount());

        try
        {
            writeLockUpgrade(rwl, true, false);
        } catch (Exception e)
        {
        }
        assertEquals("Read lock", 0, rwl.getReadHoldCount());
        assertEquals("Write lock", 0, rwl.getWriteHoldCount());

        try
        {
            writeLockUpgrade(rwl, true, true);
        } catch (Exception e)
        {
        }
        assertEquals("Read lock", 0, rwl.getReadHoldCount());
        assertEquals("Write lock", 0, rwl.getWriteHoldCount());

    }
}
