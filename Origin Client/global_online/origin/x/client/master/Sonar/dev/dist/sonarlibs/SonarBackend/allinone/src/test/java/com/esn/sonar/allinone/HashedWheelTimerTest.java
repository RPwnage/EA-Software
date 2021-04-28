package com.esn.sonar.allinone;

import com.esn.sonar.core.wheeltimer.WheelTimer;
import org.jboss.netty.util.Timeout;
import org.jboss.netty.util.Timer;
import org.jboss.netty.util.TimerTask;
import org.junit.Test;

import java.util.Set;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicInteger;

import static org.junit.Assert.assertTrue;

public class HashedWheelTimerTest
{
    private static class TestTimeout implements TimerTask
    {
        protected final AtomicInteger execCount = new AtomicInteger(0);

        public void run(Timeout timeout) throws Exception
        {
            Thread.sleep(1000);

            execCount.incrementAndGet();
            timeout.getTimer().newTimeout(this, 100, TimeUnit.MILLISECONDS);
        }
    }

    @Test
    public void testExpire() throws InterruptedException
    {
        Timer wheelTimer = new WheelTimer(100, 4096);//HashedWheelTimer(100, TimeUnit.MILLISECONDS);

        TestTimeout tt = new TestTimeout();

        wheelTimer.newTimeout(tt, 100, TimeUnit.MILLISECONDS);

        Thread.sleep(10000);

        Set<Timeout> stop = wheelTimer.stop();

        assertTrue(tt.execCount.get() > 1);

    }
}
