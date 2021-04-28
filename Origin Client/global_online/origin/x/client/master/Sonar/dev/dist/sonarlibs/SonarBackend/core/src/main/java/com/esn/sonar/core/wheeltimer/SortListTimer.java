package com.esn.sonar.core.wheeltimer;

import org.jboss.netty.util.ThreadRenamingRunnable;
import org.jboss.netty.util.Timeout;
import org.jboss.netty.util.Timer;
import org.jboss.netty.util.TimerTask;

import java.util.Comparator;
import java.util.Iterator;
import java.util.Set;
import java.util.SortedSet;
import java.util.concurrent.ConcurrentSkipListSet;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicBoolean;

import static java.util.concurrent.Executors.newFixedThreadPool;

public class SortListTimer implements Timer, Runnable
{
    private final SortedSet<InternalTimeout> timeouts = new ConcurrentSkipListSet<InternalTimeout>(new Comparator<InternalTimeout>()
    {
        public int compare(InternalTimeout o1, InternalTimeout o2)
        {
            return (int) (o1.getExpireTick() - o2.getExpireTick());
        }
    });
    private final int resolutionMSEC;
    private boolean isRunning;
    private Thread timerThread;
    private ExecutorService executor;

    private class InternalTimeout implements Timeout, Runnable
    {
        private AtomicBoolean isExpired = new AtomicBoolean();
        private AtomicBoolean isCancelled = new AtomicBoolean();
        private long expireTime;
        private TimerTask task;

        private InternalTimeout(long expireTime, TimerTask task)
        {
            this.expireTime = expireTime;
            this.task = task;
        }

        public Timer getTimer()
        {
            return SortListTimer.this;
        }

        public TimerTask getTask()
        {
            return task;
        }

        public boolean isExpired()
        {
            return isExpired.get();
        }

        public boolean isCancelled()
        {
            return isCancelled.get();
        }

        public void cancel()
        {
            isCancelled.set(true);
        }

        public long getExpireTick()
        {
            return expireTime;
        }

        public void setExpired(boolean b)
        {
            isExpired.set(b);
        }

        public void incExpireTime()
        {
            expireTime++;
        }

        public void run()
        {
            try
            {
                getTask().run(this);
            } catch (Throwable e)
            {
                e.printStackTrace();
            }
        }
    }

    public SortListTimer(int resolutionMSEC)
    {
        this.resolutionMSEC = resolutionMSEC;
        this.isRunning = true;
        this.timerThread = new Thread(new ThreadRenamingRunnable(this, "SortListTimer"));
        this.executor = newFixedThreadPool(Runtime.getRuntime().availableProcessors());
        timerThread.start();
    }

    private long getTime()
    {
        return System.currentTimeMillis();
    }

    public Timeout newTimeout(TimerTask task, long delay, TimeUnit unit)
    {
        long expire = getTime() + TimeUnit.MILLISECONDS.convert(delay, unit);

        InternalTimeout e = new InternalTimeout(expire, task);

        while (!timeouts.add(e))
        {
            e.incExpireTime();
        }
        return e;
    }

    public Set<Timeout> stop()
    {
        isRunning = false;
        try
        {
            timerThread.join();
        } catch (InterruptedException e)
        {
        }
        executor.shutdown();
        return null;
    }

    private void update()
    {
        long now = getTime();

        Iterator<InternalTimeout> iterator = timeouts.iterator();

        while (iterator.hasNext())
        {
            InternalTimeout next = iterator.next();

            if (now > next.getExpireTick())
            {
                iterator.remove();
                if (next.isCancelled())
                {
                    continue;
                }

                next.setExpired(true);
                executor.execute(next);


            } else
            {
                break;
            }
        }

    }

    public void run()
    {
        while (isRunning)
        {
            update();

            try
            {
                Thread.sleep(resolutionMSEC);
            } catch (InterruptedException e)
            {
            }
        }
    }


}
