package com.esn.sonar.master;

import java.util.ArrayList;
import java.util.Collection;
import java.util.HashMap;
import java.util.concurrent.locks.ReentrantReadWriteLock;

public class OperatorManager
{
    private final HashMap<String, Operator> operatorMap = new HashMap<String, Operator>();
    private final ReentrantReadWriteLock rwl = new ReentrantReadWriteLock();

    public Operator getOperator(String operatorId)
    {
        rwl.readLock().lock();

        Operator sphere;
        try
        {
            sphere = operatorMap.get(operatorId);

            if (sphere == null)
            {
                // upgrade lock manually
                rwl.readLock().unlock();   // must unlock first to obtain writelock

                rwl.writeLock().lock();
                try
                {
                    sphere = operatorMap.get(operatorId);

                    if (sphere == null)
                    {
                        sphere = new Operator(operatorId);
                        operatorMap.put(operatorId, sphere);
                    }
                } finally
                {
                    // downgrade lock
                    rwl.readLock().lock();  // reacquire read without giving up write lock
                    rwl.writeLock().unlock(); // unlock write, still hold read
                }
            }
        } finally
        {
            rwl.readLock().unlock();
        }


        return sphere;
    }

    public Collection<Operator> getOperators()
    {
        rwl.readLock().lock();

        try
        {
            return new ArrayList<Operator>(operatorMap.values());
        } finally
        {
            rwl.readLock().unlock();
        }
    }
}
