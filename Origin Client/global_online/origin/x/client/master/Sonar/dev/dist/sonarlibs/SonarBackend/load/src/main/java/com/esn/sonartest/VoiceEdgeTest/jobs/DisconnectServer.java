package com.esn.sonartest.VoiceEdgeTest.jobs;

import com.esn.sonartest.VoiceEdgeTest.VoiceEdgeTestDirector;
import com.esn.sonartest.VoiceEdgeTest.VoiceServer;
import com.esn.sonartest.director.AbstractJob;
import com.esn.sonartest.director.JobTrigger;
import org.jboss.netty.channel.ChannelFuture;
import org.jboss.netty.channel.ChannelFutureListener;

public class DisconnectServer extends AbstractJob
{
    private VoiceEdgeTestDirector director;
    private VoiceServer voiceServer;

    public DisconnectServer(JobTrigger trigger, VoiceEdgeTestDirector director)
    {
        super(trigger);
        this.director = director;
    }

    @Override
    protected boolean onStart() throws Exception
    {
        this.voiceServer = director.connectedServerPool.acquire();

        if (voiceServer == null)
        {
            return false;
        }

        return true;
    }

    @Override
    protected void onRun() throws Exception
    {
        voiceServer.close().addListener(new ChannelFutureListener()
        {

            public void operationComplete(ChannelFuture future) throws Exception
            {
                director.disconnectServerPool.add(voiceServer);
                signalDone();
            }
        });

    }

}
