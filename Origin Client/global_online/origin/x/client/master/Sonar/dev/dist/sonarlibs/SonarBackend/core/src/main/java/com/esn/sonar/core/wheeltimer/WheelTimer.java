package com.esn.sonar.core.wheeltimer;

import org.jboss.netty.util.Timeout;
import org.jboss.netty.util.Timer;
import org.jboss.netty.util.TimerTask;

import java.util.HashSet;
import java.util.LinkedList;
import java.util.Set;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicLong;

public class WheelTimer implements Timer, Runnable
{
    private Thread wheelThread;

    Spoke spokes[];
    private long tickLength;

    private long startTime;
    private long wheelSize;
    private volatile boolean isRunning = true;
    private AtomicLong lastTick = new AtomicLong();

    public WheelTimer(int tickLength, int wheelSize)
    {
        this.spokes = new Spoke[(int) wheelSize];
        this.tickLength = tickLength;
        this.startTime = getTime();
        this.wheelSize = wheelSize;

        for (int index = 0; index < wheelSize; index++)
        {
            spokes[index] = new Spoke();
        }

        this.wheelThread = new Thread(this);
        wheelThread.start();
        ;
    }

    private long getTime()
    {
        return TimeUnit.MILLISECONDS.convert(System.nanoTime(), TimeUnit.NANOSECONDS);
    }

    synchronized public Timeout newTimeout(TimerTask task, long delay, TimeUnit unit)
    {
        long tsNow = getTime();

        long tsTimeout = tsNow + TimeUnit.MILLISECONDS.convert(delay, unit);
        long delta = tsTimeout - startTime;

        long totalTick = delta / tickLength;

        if (totalTick < lastTick.get())
        {
            totalTick = lastTick.get();
        }

        long turns = totalTick / wheelSize;
        long ticks = totalTick - (turns * wheelSize);

        InternalTimeout timeout = new InternalTimeout(task, (int) turns);

        spokes[((int) ticks)].schedule(timeout);
        return timeout;
    }

    public Set<Timeout> stop()
    {
        this.isRunning = false;
        wheelThread.interrupt();
        try
        {
            wheelThread.join();
        } catch (InterruptedException e)
        {
        }

        return null;
    }

    private void expireTimeouts(LinkedList<InternalTimeout> expired)
    {
        for (InternalTimeout timeout : expired)
        {
            if (timeout.isCancelled())
            {
                continue;
            }

            if (timeout.isExpired())
            {
                throw new RuntimeException("Timeouts can't be expired here");
            }

            timeout.setExpired(true);

            TimerTask task = timeout.getTask();
            try
            {
                task.run(timeout);
            } catch (Exception e)
            {
            }
        }
    }

    synchronized private void timerTick()
    {
        long now = getTime();
        long tick = (now - startTime) / tickLength;

        LinkedList<InternalTimeout> expired = new LinkedList<InternalTimeout>();


        if (tick < lastTick.get())
        {
            throw new RuntimeException("Time travelling is not allowed");
        }

        long currTick = lastTick.getAndSet(tick);

        for (; currTick != tick; currTick++)
        {
            Spoke spoke = spokes[((int) (currTick % wheelSize))];
            spoke.tick(expired);
        }

        expireTimeouts(expired);
    }

    public void run()
    {
        while (isRunning)
        {
            timerTick();
            try
            {
                Thread.sleep(tickLength);
            } catch (InterruptedException e)
            {
                return;
            }
        }
    }

    private static class Spoke
    {
        synchronized public void tick(LinkedList<InternalTimeout> expired)
        {
            for (InternalTimeout timeout : timeouts)
            {
                if (timeout.decrementTurns() < 1)
                {
                    expired.add(timeout);
                }
            }

            for (InternalTimeout timeout : expired)
            {
                timeouts.remove(timeout);
            }
        }

        synchronized public void schedule(InternalTimeout timeout)
        {
            timeouts.add(timeout);
        }

        private Set<InternalTimeout> timeouts = new HashSet<InternalTimeout>();
    }

    private class InternalTimeout implements Timeout
    {
        Timer timer;
        TimerTask task;

        AtomicBoolean isExpired = new AtomicBoolean(false);
        AtomicBoolean isCancelled = new AtomicBoolean(false);
        private int turns;

        private InternalTimeout(TimerTask task, int turns)
        {
            this.task = task;
            this.turns = turns;

        }

        public Timer getTimer()
        {
            return WheelTimer.this;
        }

        public TimerTask getTask()
        {
            return task;
        }

        public boolean isExpired()
        {
            return this.isExpired.get();
        }

        public boolean isCancelled()
        {
            return this.isCancelled.get();
        }

        public void cancel()
        {
            isCancelled.set(true);
        }

        public void setExpired(boolean b)
        {
            isExpired.set(b);
        }

        public int decrementTurns()
        {
            turns--;
            return turns;
        }
    }

}
