package com.esn.sonartest.verifier;

import com.esn.sonar.core.Message;

/**
 * User: ronnie
 * Date: 2011-06-17
 * Time: 10:35
 */
public class ResultVerifier extends Verifier<Long>
{
    private static final ResultVerifier instance = new ResultVerifier();

    public static ResultVerifier getInstance()
    {
        return instance;
    }

    public void addExpectation(long id, Expectation verification)
    {
        Expectation oldResult = expectations.put(id, verification);
        if (oldResult != null)
        {
            throw new RuntimeException("There already exists an expectation for given id (" + id + ")");
        }
    }

    public void onResult(Message message)
    {
        Expectation expectation = expectations.remove(message.getId());
        if (expectation == null)
        {
            throw new RuntimeException("Expectation not found (" + message.getId() + ")");
        }

        // Check timestamp
        long elapsedMillis = expectation.checkTimeout();

        // Check expectation
        expectation.verify(message);
    }
}
