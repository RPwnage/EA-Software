package com.esn.sonar.core;

import com.esn.sonar.core.wheeltimer.SortListTimer;
import org.jboss.netty.util.Timeout;
import org.jboss.netty.util.Timer;
import org.jboss.netty.util.TimerTask;

import java.util.concurrent.TimeUnit;

public class Maintenance
{
    private final Timer timer;
    private static Maintenance instance = new Maintenance(100);

    public static void configure(int resolutionMSEC)
    {
        instance = new Maintenance(resolutionMSEC);
    }

    private Maintenance(int resolutionMSEC)
    {
        timer = new SortListTimer(resolutionMSEC);

        instance = this;
    }

    public Timeout scheduleTask(int secondsToRun, TimerTask task)
    {
        return timer.newTimeout(task, (long) secondsToRun, TimeUnit.SECONDS);
    }

    public Timeout scheduleTaskMilli(int millisecondsToRun, TimerTask task)
    {
        return timer.newTimeout(task, (long) millisecondsToRun, TimeUnit.MILLISECONDS);
    }

    public static Maintenance getInstance()
    {
        return instance;
    }

    public void reset()
    {
        timer.stop();
    }
}
