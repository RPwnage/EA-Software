package com.esn.sonartest.VoiceEdgeTest.jobs;

import com.esn.sonar.core.Message;
import com.esn.sonar.core.Protocol;
import com.esn.sonartest.VoiceEdgeTest.VoiceEdgeTestDirector;
import com.esn.sonartest.VoiceEdgeTest.VoiceServer;
import com.esn.sonartest.director.AbstractJob;
import com.esn.sonartest.director.JobTrigger;
import org.jboss.netty.channel.ChannelFuture;
import org.jboss.netty.channel.ChannelFutureListener;

/**
 * Created by IntelliJ IDEA.
 * User: jonas
 * Date: 2011-07-06
 * Time: 13:57
 * To change this template use File | Settings | File Templates.
 */
public class ReviveServer extends AbstractJob
{
    private VoiceEdgeTestDirector director;
    private VoiceServer voiceServer;

    public ReviveServer(JobTrigger trigger, VoiceEdgeTestDirector director)
    {
        super(trigger);
        this.director = director;
    }

    @Override
    protected boolean onStart() throws Exception
    {
        this.voiceServer = director.disconnectServerPool.acquire();

        if (voiceServer == null)
        {
            return false;
        }

        return true;
    }

    @Override
    protected void onRun() throws Exception
    {
        voiceServer.setRegistrationError(new VoiceServer.MessageCallback()
        {
            public void onMessage(Message msg)
            {
                if (msg.getArgument(1).equals(Protocol.Errors.ChallengeExpired))
                {
                    voiceServer.close().addListener(new ChannelFutureListener()
                    {
                        public void operationComplete(ChannelFuture future) throws Exception
                        {
                            director.disconnectServerPool.add(voiceServer);
                        }
                    });

                    return;
                }

                throw new RuntimeException();
            }
        });

        voiceServer.setRegistrationSuccess(new VoiceServer.MessageCallback()
        {
            public void onMessage(Message msg)
            {
                signalDone();
                director.connectedServerPool.add(voiceServer);
            }
        });

        voiceServer.connect();
    }

}
