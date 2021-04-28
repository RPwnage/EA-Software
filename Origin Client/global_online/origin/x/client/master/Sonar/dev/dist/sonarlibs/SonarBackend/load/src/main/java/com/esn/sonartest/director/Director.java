package com.esn.sonartest.director;

import org.jboss.netty.bootstrap.ClientBootstrap;
import org.jboss.netty.channel.socket.nio.NioClientSocketChannelFactory;
import org.jboss.netty.channel.socket.nio.NioDatagramChannelFactory;

import java.util.Random;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

public abstract class Director
{
    protected final ExecutorService workerThreadPool = Executors.newFixedThreadPool(8);
    protected final NioClientSocketChannelFactory socketChannelFactory = new NioClientSocketChannelFactory(Executors.newCachedThreadPool(), Executors.newCachedThreadPool());
    protected final NioDatagramChannelFactory datagramChannelFactory = new NioDatagramChannelFactory((Executors.newCachedThreadPool()));
    protected static final Random random = new Random(System.nanoTime());
    private boolean isRunning = true;
    protected JobTrigger[] jobTriggers;
    private long tsNextPrint = 0;

    protected abstract void onStart() throws Exception;

    protected abstract void onStop() throws Exception;

    protected abstract JobTrigger[] setupJobTriggers();

    public ClientBootstrap getBootstrap()
    {
        ClientBootstrap cb = new ClientBootstrap(socketChannelFactory);
        cb.setOption("soLinger", 0);

        return cb;
    }

    private void update(long tsNow, int[] doneValues, int[] startValues) throws Exception
    {
        for (JobTrigger jobTrigger : jobTriggers)
        {
            jobTrigger.update(tsNow);
        }

        if (tsNow > tsNextPrint)
        {

            for (int index = 0; index < jobTriggers.length; index++)
            {
                JobTrigger jobTrigger = jobTriggers[index];

                int doneCount = jobTrigger.getDoneCount();
                int startCount = jobTrigger.getStartCount();
                float latency = jobTrigger.getAverageJobTime();

                System.err.printf("%s %g %d %.02f | ",
                        jobTrigger.getName(),
                        jobTrigger.getRate(),
                        doneCount - doneValues[index],
                        latency);

                doneValues[index] = doneCount;
                startValues[index] = startCount;
            }

            System.err.printf("\n");

            tsNextPrint = tsNow + 1000;
        }

    }

    public final void start() throws Exception
    {
        jobTriggers = setupJobTriggers();

        onStart();

        long tsStart = System.currentTimeMillis();

        int doneCount[] = new int[jobTriggers.length];
        int startCount[] = new int[jobTriggers.length];

        tsNextPrint = 0;

        while (isRunning)
        {
            long tsNow = System.currentTimeMillis() - tsStart;

            update(tsNow, doneCount, startCount);

            Thread.sleep(100);
        }
    }

    public final void stop() throws Exception
    {
        onStop();
    }

    public JobTrigger[] getJobTriggers()
    {
        return jobTriggers;
    }


}
