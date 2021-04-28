package com.esn.sonartest.VoiceEdgeTest;

import com.esn.sonar.core.AbstractConfig;
import com.esn.sonar.core.Maintenance;
import com.esn.sonar.core.util.Utils;
import com.esn.sonartest.VoiceEdgeTest.jobs.ConnectServer;
import com.esn.sonartest.VoiceEdgeTest.jobs.DisconnectServer;
import com.esn.sonartest.VoiceEdgeTest.jobs.ReviveServer;
import com.esn.sonartest.VoiceEdgeTest.jobs.StealChannel;
import com.esn.sonartest.director.AbstractJob;
import com.esn.sonartest.director.Director;
import com.esn.sonartest.director.JobTrigger;
import com.esn.sonartest.director.Pool;

public class VoiceEdgeTestDirector extends Director
{
    private VoiceEdgeTestConfig config;
    public Pool<VoiceServer> disconnectServerPool = new Pool<VoiceServer>();
    public Pool<VoiceServer> connectedServerPool = new Pool<VoiceServer>();
    public Pool<VoiceServer> uncommittedServersPool = new Pool<VoiceServer>();
    public Pool<VoiceChannel> committedChannelsPool = new Pool<VoiceChannel>();

    public Pool<VoiceChannel> uncommittedChannels = new Pool<VoiceChannel>();
    public Pool<User> uncommittedUsers = new Pool<User>();

    public VoiceEdgeTestDirector(VoiceEdgeTestConfig config)
    {
        Utils.create();
        Maintenance.configure(100);
        this.config = config;
    }

    @Override
    protected void onStart() throws Exception
    {
        String publicAddress = AbstractConfig.guessLocalAddress();

        for (int index = 0; index < config.getVoiceServerCount(); index++)
        {
            VoiceServer voiceServer = new VoiceServer(getBootstrap(), config.getVoiceEdgeAddress(), datagramChannelFactory, publicAddress, 1000, "OPER");
            uncommittedServersPool.add(voiceServer);
        }

        int userCount = config.getChannelCount() * config.getUsersPerChannel();

        for (int index = 0; index < userCount; index++)
        {
            String userId = String.format("UID-%08x", index);
            String userDesc = String.format("Description of %s", userId);

            uncommittedUsers.add(new User(userId, userDesc));
        }

        for (int index = 0; index < config.getChannelCount(); index++)
        {
            String channelId = String.format("CID-%08x", index);
            String channelDesc = String.format("Description of %s", channelId);

            uncommittedChannels.add(new VoiceChannel(channelId, channelDesc));
        }
    }

    @Override
    protected void onStop() throws Exception
    {
    }

    @Override
    protected JobTrigger[] setupJobTriggers()
    {
        return new JobTrigger[]{
                new JobTrigger(workerThreadPool, "ConnectServer", config.getConnectServer())
                {
                    @Override
                    public AbstractJob createJob()
                    {
                        return new ConnectServer(this, VoiceEdgeTestDirector.this, config.getChannelCount() / config.getVoiceServerCount(), config.getUsersPerChannel());
                    }
                },

                new JobTrigger(workerThreadPool, "DisconnectServer", config.getDisconnectServer())
                {
                    @Override
                    public AbstractJob createJob()
                    {
                        return new DisconnectServer(this, VoiceEdgeTestDirector.this);
                    }
                },

                new JobTrigger(workerThreadPool, "ReviveServer", config.getReviveServer())
                {
                    @Override
                    public AbstractJob createJob()
                    {
                        return new ReviveServer(this, VoiceEdgeTestDirector.this);
                    }
                },

                new JobTrigger(workerThreadPool, "StealServer", config.getStealServer())
                {
                    @Override
                    public AbstractJob createJob()
                    {
                        return new StealChannel(this, VoiceEdgeTestDirector.this);
                    }
                }

        };
    }

    public static void main(String[] args) throws Exception, AbstractConfig.ConfigurationException
    {
        VoiceEdgeTestConfig config = new VoiceEdgeTestConfig(System.getProperty("config"));

        new VoiceEdgeTestDirector(config).start();


    }
}
