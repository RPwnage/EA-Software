package com.esn.sonartest.verifier;

import com.esn.sonar.core.Message;

/**
 * User: ronnie
 * Date: 2011-06-17
 * Time: 10:37
 */
public abstract class Expectation
{
    protected final long tsCreated;
    private String info;

    protected Expectation(String info)
    {
        this.tsCreated = System.nanoTime();
        this.info = info;
    }

    public abstract void verify(Message message);

    /**
     * Checks how long this expectation has lived.
     *
     * @return the time elapsed since creation (milliseconds)
     * @throws ExpectationTimeout if is has lived longer than <tt>timeoutMillis</tt>
     */
    public final long checkTimeout()
    {
        long timeoutMillis = 600000;

        long elapsedMillis = (System.nanoTime() - tsCreated) / 1000000;
        if (elapsedMillis > timeoutMillis)
        {
            throw new ExpectationTimeout("Expectation '" + info + "' timed out. Timeout: " + timeoutMillis + " ms, elapsed: " + elapsedMillis + " ms");
        }
        return elapsedMillis;
    }
}
