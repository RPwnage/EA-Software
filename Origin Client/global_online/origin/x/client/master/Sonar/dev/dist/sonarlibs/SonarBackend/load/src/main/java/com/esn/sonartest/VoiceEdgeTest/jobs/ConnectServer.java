package com.esn.sonartest.VoiceEdgeTest.jobs;

import com.esn.sonar.core.Message;
import com.esn.sonar.core.Protocol;
import com.esn.sonartest.VoiceEdgeTest.User;
import com.esn.sonartest.VoiceEdgeTest.VoiceChannel;
import com.esn.sonartest.VoiceEdgeTest.VoiceEdgeTestDirector;
import com.esn.sonartest.VoiceEdgeTest.VoiceServer;
import com.esn.sonartest.director.AbstractJob;
import com.esn.sonartest.director.JobTrigger;
import org.jboss.netty.channel.ChannelFuture;
import org.jboss.netty.channel.ChannelFutureListener;

import java.util.Collection;
import java.util.Iterator;
import java.util.Random;

public class ConnectServer extends AbstractJob
{
    private VoiceEdgeTestDirector director;
    private VoiceServer voiceServer;
    private static Random random = new Random();
    private int channelsPerServer;
    private int usersPerChannel;

    public ConnectServer(JobTrigger trigger, VoiceEdgeTestDirector director, int channelsPerServer, int usersPerChannel)
    {
        super(trigger);
        this.director = director;
        this.channelsPerServer = channelsPerServer;
        this.usersPerChannel = usersPerChannel;
    }

    @Override
    protected boolean onStart() throws Exception
    {
        this.voiceServer = director.uncommittedServersPool.acquire();

        if (voiceServer == null)
        {
            return false;
        }

        Collection<VoiceChannel> voiceChannels = director.uncommittedChannels.acquireCount(channelsPerServer);

        if (voiceChannels.size() != channelsPerServer)
        {
            throw new RuntimeException();
        }

        int userCount = channelsPerServer * usersPerChannel;

        Collection<User> users = director.uncommittedUsers.acquireCount(userCount);

        if (users.size() != userCount)
        {
            throw new RuntimeException();
        }

        Iterator<User> userIterator = users.iterator();

        for (VoiceChannel voiceChannel : voiceChannels)
        {
            for (int index = 0; index < usersPerChannel; index++)
            {
                User user = userIterator.next();
                voiceServer.setupState(user, voiceChannel);
            }

        }


        return true;
    }

    @Override
    protected void onRun() throws Exception
    {
        voiceServer.setRegistrationSuccess(new VoiceServer.MessageCallback()
        {
            public void onMessage(Message msg)
            {
                signalDone();
                director.connectedServerPool.add(voiceServer);
            }
        });

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

        voiceServer.connect();


    }
}
