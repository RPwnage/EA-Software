package com.esn.sonar.allinone;

import com.esn.sonar.core.wheeltimer.SortListTimer;
import org.jboss.netty.util.Timeout;
import org.jboss.netty.util.Timer;
import org.jboss.netty.util.TimerTask;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;

import java.util.Random;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicInteger;

import static junit.framework.Assert.assertEquals;
import static junit.framework.Assert.assertTrue;

/**
 * Created by IntelliJ IDEA.
 * User: Jonas
 * Date: 2011-07-07
 * Time: 11:02
 * To change this template use File | Settings | File Templates.
 */
public class TimerTest
{
    private Timer timer;

    private final static int TICK_LENGTH = 100;
    private static final int TIMER_COUNT = 2000;


    Timer timerFactory(int tickLength, int wheelTicks)
    {
        return new SortListTimer(TICK_LENGTH);//SortListTimer(tickLength);
    }

    @Before
    public void setUp() throws InterruptedException
    {
        this.timer = timerFactory(TICK_LENGTH, 4096);

        Thread.sleep(new Random().nextInt(2000));
    }

    @Test
    public void testScheduleOneTick() throws Exception
    {
        ExpectedRunTimeout ts1 = new ExpectedRunTimeout();
        timer.newTimeout(ts1, TICK_LENGTH, TimeUnit.MILLISECONDS);
        ts1.await(TICK_LENGTH * 2);
    }

    @Test
    public void testScheduleZeroTick() throws Exception
    {
        ExpectedRunTimeout ts1 = new ExpectedRunTimeout();
        timer.newTimeout(ts1, 0, TimeUnit.MILLISECONDS);
        ts1.await(TICK_LENGTH * 2);
    }

    @Test
    public void testScheduleOneTurn() throws Exception
    {
        ExpectedRunTimeout ts1 = new ExpectedRunTimeout();

        timer.stop();
        timer = timerFactory(1, 500);
        timer.newTimeout(ts1, 750, TimeUnit.MILLISECONDS);
        ts1.await(1000);
    }

    @Test
    public void testScheduleManyTurns() throws Exception
    {
        ExpectedRunTimeout ts1 = new ExpectedRunTimeout();

        timer.stop();
        timer = timerFactory(1, 10);
        Timeout timeout = timer.newTimeout(ts1, 750, TimeUnit.MILLISECONDS);
        ts1.await(1000);

        assertEquals("Timeout should be expired", true, timeout.isExpired());
        assertEquals("Timeout should be expired", false, timeout.isCancelled());
    }

    @Test
    public void testCancel() throws Exception
    {
        ExpectedRunTimeout ts1 = new ExpectedRunTimeout();

        Timeout timeout = timer.newTimeout(ts1, 3, TimeUnit.SECONDS);
        Thread.sleep(1);
        timeout.cancel();
        Thread.sleep(4);

        assertEquals("Timeout should be cancelled", true, timeout.isCancelled());
        assertEquals("Timeout should not have expired", false, timeout.isExpired());
    }

    @Test
    public void testRescheduleFromWithinAndSleep() throws Exception
    {
        final AtomicInteger execCount = new AtomicInteger();

        Timeout timeout = timer.newTimeout(new TimerTask()
        {
            public void run(Timeout timeout) throws Exception
            {
                execCount.incrementAndGet();
                Thread.sleep(1000);
                timeout.getTimer().newTimeout(this, 100, TimeUnit.MILLISECONDS);
            }
        }, 100, TimeUnit.MILLISECONDS);

        Thread.sleep(5000);

        assertTrue("", execCount.get() >= 3);
    }

    @Test
    public void testAllMustExpire() throws Exception
    {
        final AtomicInteger expired = new AtomicInteger(TIMER_COUNT);

        timer.stop();
        timer = timerFactory(1, 1000);

        for (int index = 0; index < TIMER_COUNT; index++)
        {
            timer.newTimeout(new TimerTask()
            {
                public void run(Timeout timeout) throws Exception
                {
                    expired.decrementAndGet();
                }
            }, index, TimeUnit.MILLISECONDS);
        }

        Thread.sleep(5000);

        assertTrue("", expired.get() == 0);
    }

    @Test
    public void testPrecisionSeconds() throws Exception
    {
        timer.stop();
        timer = timerFactory(100, 4096);

        final AtomicInteger execCount = new AtomicInteger();

        Timeout timeout = timer.newTimeout(new TimerTask()
        {
            public void run(Timeout timeout) throws Exception
            {
                Thread.sleep(1000);
                execCount.incrementAndGet();
                timeout.getTimer().newTimeout(this, 1, TimeUnit.SECONDS);
            }
        }, 1, TimeUnit.SECONDS);

        Thread.sleep(10000);

        assertTrue("", (execCount.get() >= 4 && execCount.get() < 6));
    }

    @Test
    public void testOrder() throws Exception
    {
        AtomicInteger orderCounter = new AtomicInteger();

        timer.stop();
        timer = timerFactory(1, 4096);


        timer.newTimeout(new OrderTimerTask(orderCounter, 5), 1000, TimeUnit.MILLISECONDS);
        timer.newTimeout(new OrderTimerTask(orderCounter, 6), 1200, TimeUnit.MILLISECONDS);
        timer.newTimeout(new OrderTimerTask(orderCounter, 7), 1400, TimeUnit.MILLISECONDS);
        timer.newTimeout(new OrderTimerTask(orderCounter, 8), 1600, TimeUnit.MILLISECONDS);

        timer.newTimeout(new OrderTimerTask(orderCounter, 1), 200, TimeUnit.MILLISECONDS);
        timer.newTimeout(new OrderTimerTask(orderCounter, 2), 400, TimeUnit.MILLISECONDS);
        timer.newTimeout(new OrderTimerTask(orderCounter, 3), 600, TimeUnit.MILLISECONDS);
        timer.newTimeout(new OrderTimerTask(orderCounter, 4), 800, TimeUnit.MILLISECONDS);

        OrderTimerTask lastTask = (OrderTimerTask) timer.newTimeout(new OrderTimerTask(orderCounter, 9), 1800, TimeUnit.MILLISECONDS).getTask();

        lastTask.await(2500);
    }

    public void testSameExpire() throws Exception
    {
        AtomicInteger orderCounter = new AtomicInteger();

        ExpectedRunTimeout t1 = (ExpectedRunTimeout) timer.newTimeout(new ExpectedRunTimeout(), 100, TimeUnit.MILLISECONDS).getTask();
        ExpectedRunTimeout t2 = (ExpectedRunTimeout) timer.newTimeout(new ExpectedRunTimeout(), 100, TimeUnit.MILLISECONDS).getTask();

        t1.await(1000);
        t2.await(1000);
    }


    @After
    public void tearDown()
    {
        this.timer.stop();
    }

    private class ExpectedRunTimeout implements TimerTask
    {
        private CountDownLatch latch = new CountDownLatch(1);

        public void run(Timeout timeout) throws Exception
        {
            if (latch.getCount() == 0)
            {
                throw new RuntimeException();
            }

            latch.countDown();
        }

        void await(int msecTimeout) throws InterruptedException
        {
            if (!latch.await(msecTimeout, TimeUnit.MILLISECONDS))
            {
                throw new RuntimeException("Did not fire");
            }
        }
    }

    private class OrderTimerTask extends ExpectedRunTimeout
    {
        private final AtomicInteger orderCounter;
        private final int orderIndex;

        private OrderTimerTask(AtomicInteger orderCounter, int orderIndex)
        {
            this.orderCounter = orderCounter;
            this.orderIndex = orderIndex;
        }

        @Override
        public void run(Timeout timeout) throws Exception
        {
            if (orderIndex != orderCounter.incrementAndGet())
            {
                throw new RuntimeException("Out of order");
            }
            super.run(timeout);

        }
    }
}
