package com.esn.sonartest.verifier;

/**
 * User: ronnie
 * Date: 2011-06-20
 * Time: 16:14
 */
public class ExpectationTimeout extends RuntimeException
{
    public ExpectationTimeout()
    {
    }

    public ExpectationTimeout(String message)
    {
        super(message);
    }

    public ExpectationTimeout(String message, Throwable cause)
    {
        super(message, cause);
    }

    public ExpectationTimeout(Throwable cause)
    {
        super(cause);
    }
}
