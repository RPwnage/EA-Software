package com.esn.sonartest.verifier;

import com.esn.sonar.core.util.Processor;
import org.jboss.netty.util.HashedWheelTimer;
import org.jboss.netty.util.Timeout;
import org.jboss.netty.util.TimerTask;

import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicLong;

/**
 * User: ronnie
 * Date: 2011-06-20
 * Time: 15:35
 */
public class TimeoutVerifier
{
    /**
     * Timeout for {@link Expectation} in millis. Default value is 1000 millis but will be overridden from configuration runtime.
     */
    private static AtomicLong GLOBAL_TIMEOUT = new AtomicLong(1000L);

    private final long intervalMillis;
    private final long timeoutMillis;

    private final HashedWheelTimer hashedWheelTimer = new HashedWheelTimer(1, TimeUnit.SECONDS);
    private final TimerTask statsTask = new TimerTask()
    {
        public void run(Timeout timeout) throws Exception
        {
            scheduleStatsTask();
        }
    };

    private final AtomicBoolean isRunning = new AtomicBoolean(true);
    private final Thread timerThread = new Thread(new Runnable()
    {
        public void run()
        {
            while (isRunning.get())
            {
                checkTimeouts();

                try
                {
                    Thread.sleep(intervalMillis);
                } catch (InterruptedException e)
                {
                    throw new RuntimeException(getClass().getSimpleName() + " interrupted", e);
                }
            }
        }
    });
    private final Processor<Expectation> timeoutProcessor = new Processor<Expectation>()
    {
        public boolean execute(Expectation expectation)
        {
            expectation.checkTimeout();
            return true;
        }
    };

    /**
     * Timeout for {@link Expectation} in millis. Default value is 1000 millis but will be overridden from configuration runtime.
     *
     * @return the current timeout in millis
     */
    public static long getTimeout()
    {
        return GLOBAL_TIMEOUT.get();
    }

    public TimeoutVerifier(long timeoutMillis)
    {
        GLOBAL_TIMEOUT.set(timeoutMillis);
        this.intervalMillis = timeoutMillis / 2;
        this.timeoutMillis = timeoutMillis;
    }

    public void start()
    {
        scheduleStatsTask();
        this.timerThread.start();
    }

    private void scheduleStatsTask()
    {
        this.hashedWheelTimer.newTimeout(statsTask, 5, TimeUnit.SECONDS);
    }

    public void stop()
    {
        isRunning.set(false);
        try
        {
            timerThread.join();
        } catch (InterruptedException e)
        {
            // ignore
        }
    }

    private void checkTimeouts()
    {
        EventVerifier.getInstance().processExpectations(timeoutProcessor);
        ResultVerifier.getInstance().processExpectations(timeoutProcessor);
    }
}
