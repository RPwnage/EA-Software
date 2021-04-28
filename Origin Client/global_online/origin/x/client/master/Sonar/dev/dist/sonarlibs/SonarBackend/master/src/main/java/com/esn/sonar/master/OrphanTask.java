package com.esn.sonar.master;


import com.esn.sonar.core.Maintenance;
import com.esn.sonar.master.voice.VoiceChannelManager;
import org.jboss.netty.util.Timeout;
import org.jboss.netty.util.TimerTask;

import java.util.Collection;

public class OrphanTask implements TimerTask
{
    private final int taskIntervalSec;
    private final MasterServer masterServer;
    private final int maxOrphanedCount;
    private volatile Timeout timeout;

    public OrphanTask(MasterServer masterServer, int taskIntervalSec, int maxOrphanedCount)
    {
        this.masterServer = masterServer;
        this.taskIntervalSec = taskIntervalSec;
        this.maxOrphanedCount = maxOrphanedCount;
        this.timeout = Maintenance.getInstance().scheduleTask(taskIntervalSec, this);
    }

    private void task()
    {
        Collection<Operator> operators = masterServer.getOperatorManager().getOperators();

        MasterServer.masterLog.info("Running Orphan task");

        for (Operator operator : operators)
        {
            Collection<VoiceChannelManager.VoiceChannel> voiceChannels = operator.getChannelManager().getVoiceChannels();

            if (MasterServer.masterLog.isTraceEnabled())
                MasterServer.masterLog.info(String.format("Doing operator %s with %d voice channels", operator.getId(), voiceChannels.size()));

            for (VoiceChannelManager.VoiceChannel voiceChannel : voiceChannels)
            {
                int i = 0;

                if (masterServer.getVoiceManager().getVoiceServerById(voiceChannel.getServerId()) == null)
                {
                    i = voiceChannel.increaseOrphanCount();
                } else
                {
                    if (operator.getCache().getUserCount() == 0)
                    {
                        i = voiceChannel.increaseOrphanCount();
                    }
                }


                if (i <= maxOrphanedCount)
                {
                    // Have not reached limit yet
                    continue;
                }

                // Orphan the channel
                operator.getChannelManager().unlinkChannel(voiceChannel.getChannelId());
                operator.getCache().destroyChannel(voiceChannel.getChannelId());
            }
        }

    }

    public void run(Timeout timeout) throws Exception
    {

        try
        {
            task();
        } finally
        {
            this.timeout = Maintenance.getInstance().scheduleTask(taskIntervalSec, this);
        }
    }


    public void cancel()
    {
        if (timeout != null)
        {
            timeout.cancel();
        }
    }
}
