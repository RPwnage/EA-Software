package com.esn.sonartest.MasterTest.jobs;

import com.esn.sonar.core.Maintenance;
import com.esn.sonartest.MasterTest.MasterTestDirector;
import com.esn.sonartest.OperatorClient;
import com.esn.sonartest.director.AbstractJob;
import com.esn.sonartest.director.JobTrigger;
import org.jboss.netty.channel.ChannelFuture;
import org.jboss.netty.channel.ChannelFutureListener;
import org.jboss.netty.util.Timeout;
import org.jboss.netty.util.TimerTask;

public class OperatorClientConnectionCycle extends AbstractJob
{
    private MasterTestDirector masterTestDirector;
    private OperatorClient operatorClient;

    public OperatorClientConnectionCycle(JobTrigger trigger, MasterTestDirector masterTestDirector)
    {
        super(trigger);
        this.masterTestDirector = masterTestDirector;
    }

    @Override
    protected boolean onStart() throws Exception
    {
        operatorClient = masterTestDirector.createOperatorClient();

        return true;
    }


    @Override
    protected void onRun() throws Exception
    {
        operatorClient.connect().addListener(new ChannelFutureListener()
        {
            public void operationComplete(ChannelFuture future) throws Exception
            {
                Maintenance.getInstance().scheduleTask(3, new TimerTask()
                {
                    public void run(Timeout timeout) throws Exception
                    {
                        operatorClient.close().addListener(new ChannelFutureListener()
                        {
                            public void operationComplete(ChannelFuture future) throws Exception
                            {
                                signalDone();
                            }
                        });
                    }
                });

            }
        });


    }

}
