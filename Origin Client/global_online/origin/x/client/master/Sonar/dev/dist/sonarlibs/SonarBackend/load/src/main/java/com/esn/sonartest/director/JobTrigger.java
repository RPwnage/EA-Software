package com.esn.sonartest.director;

import java.util.concurrent.ExecutorService;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.concurrent.atomic.AtomicLong;


public abstract class JobTrigger implements Runnable
{
    private final AtomicLong totalJobTime = new AtomicLong();
    private final AtomicBoolean paused = new AtomicBoolean(false);
    private final String name;

    private double ratePerSec;
    private final AtomicInteger startCount = new AtomicInteger();
    private final AtomicInteger doneCount = new AtomicInteger();
    private final AtomicInteger avgCount = new AtomicInteger();
    private AtomicInteger abortedCount = new AtomicInteger();
    private final ExecutorService threadPool;

    public JobTrigger(ExecutorService threadPool, String name, double ratePerSec)
    {
        this.name = name;
        this.ratePerSec = ratePerSec;
        this.threadPool = threadPool;
    }

    synchronized void updateLatency()
    {
        totalJobTime.set(totalJobTime.get() / 2);
        avgCount.set((avgCount.get() / 2) + 1);
    }

    synchronized void resetLatency()
    {
        totalJobTime.set(1);
        avgCount.set(1);
    }


    public void signalJobDone(long runtime)
    {
        doneCount.incrementAndGet();
        avgCount.incrementAndGet();


        if (totalJobTime.addAndGet(runtime) > 30000)
        {
            updateLatency();
        }
    }

    public void signalJobAborted()
    {
        abortedCount.incrementAndGet();
    }

    public float getAverageJobTime()
    {
        return (float) totalJobTime.get() / (float) avgCount.get();
    }

    public void run()
    {
        createJob().run();
    }

    public void update(long tsNow) throws Exception
    {
        double expected = getExpected(tsNow);

        while (startCount.get() < expected)
        {
            if (!paused.get())
            {
                threadPool.execute(this);
                startCount.incrementAndGet();
            }
        }
    }

    public String getName()
    {
        return name;
    }

    public int getStartCount()
    {
        return startCount.get();
    }

    public double getExpected(long tsNow)
    {
        return (((double) tsNow / 1000.0) * (double) ratePerSec);
    }

    public abstract AbstractJob createJob();

    public int getDoneCount()
    {
        return doneCount.get();
    }

    public double getRate()
    {
        return ratePerSec;
    }


    public void setRate(double rate)
    {
        if (rate <= 0.0)
        {
            rate = 1.0;
        }

        this.ratePerSec = rate;

        updateLatency();
    }


    public int getAbortCount()
    {
        return abortedCount.get();
    }

    public void pause(boolean pause)
    {
        paused.set(pause);
    }
}
