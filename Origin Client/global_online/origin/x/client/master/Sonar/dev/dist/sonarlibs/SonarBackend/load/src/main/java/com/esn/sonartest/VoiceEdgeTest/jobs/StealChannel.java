package com.esn.sonartest.VoiceEdgeTest.jobs;

import com.esn.sonar.core.Maintenance;
import com.esn.sonartest.VoiceEdgeTest.VoiceChannel;
import com.esn.sonartest.VoiceEdgeTest.VoiceEdgeTestDirector;
import com.esn.sonartest.VoiceEdgeTest.VoiceServer;
import com.esn.sonartest.director.AbstractJob;
import com.esn.sonartest.director.JobTrigger;
import org.jboss.netty.util.Timeout;
import org.jboss.netty.util.TimerTask;

/**
 * Created by IntelliJ IDEA.
 * User: jonas
 * Date: 2011-07-08
 * Time: 09:31
 * To change this template use File | Settings | File Templates.
 */
public class StealChannel extends AbstractJob
{
    private VoiceEdgeTestDirector director;
    private VoiceServer serverVictim;
    private VoiceServer serverTarget;


    public StealChannel(JobTrigger trigger, VoiceEdgeTestDirector director)
    {
        super(trigger);
        this.director = director;
    }

    @Override
    protected boolean onStart() throws Exception
    {
        this.serverVictim = director.connectedServerPool.acquire();
        this.serverTarget = director.connectedServerPool.acquire();

        if (serverVictim == null || serverTarget == null)
        {
            if (serverTarget == null) director.connectedServerPool.release(serverTarget);
            if (serverVictim == null) director.connectedServerPool.release(serverVictim);

            return false;
        }

        return true;
    }

    @Override
    protected void onRun() throws Exception
    {
        VoiceChannel voiceChannel = serverVictim.getVoiceChannel();

        String userId = String.format("THIEF-%08x", Thread.currentThread().getId() * 31337);
        String userDesc = String.format("Description of %s", userId);

        final Timeout timeout = Maintenance.getInstance().scheduleTask(5, new TimerTask()
        {
            public void run(Timeout timeout) throws Exception
            {
                throw new RuntimeException();
            }
        });

        serverTarget.expectClientUnregister(userId, voiceChannel.getChannelId(), new Runnable()
        {
            public void run()
            {
                timeout.cancel();
                director.connectedServerPool.release(serverTarget);
                director.connectedServerPool.release(serverVictim);
                signalDone();
            }
        });

        serverTarget.writeClientRegisteredToChannel(userId, userDesc, voiceChannel.getChannelId(), voiceChannel.getChannelDesc(), 1);


    }

}
