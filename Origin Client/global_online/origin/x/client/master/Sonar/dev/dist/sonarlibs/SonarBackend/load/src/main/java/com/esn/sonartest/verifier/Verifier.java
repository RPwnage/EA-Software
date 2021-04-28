package com.esn.sonartest.verifier;

import com.esn.sonar.core.util.Processor;

import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;

/**
 * User: ronnie
 * Date: 2011-06-20
 * Time: 15:27
 */
public abstract class Verifier<K>
{
    protected final Map<K, Expectation> expectations = new ConcurrentHashMap<K, Expectation>();

    public void processExpectations(Processor<Expectation> processor)
    {

        for (Expectation expectation : expectations.values())
        {
            if (!processor.execute(expectation))
            {
                break;
            }
        }
    }

    public int size()
    {
        return expectations.size();
    }

    public void reset()
    {
        expectations.clear();
    }

}
