package com.esn.sonar.voiceedge;

import com.esn.sonar.core.*;
import com.esn.sonar.core.challenge.ChallengeConfirmHandler;
import com.esn.sonar.core.challenge.ChallengePacket;
import com.esn.sonar.core.challenge.ChallengeServerHandler;
import com.esn.sonar.core.util.Ref;
import com.esn.sonar.core.util.Utils;
import org.jboss.netty.bootstrap.ClientBootstrap;
import org.jboss.netty.channel.Channel;
import org.jboss.netty.channel.socket.ClientSocketChannelFactory;
import org.jboss.netty.channel.socket.DatagramChannelFactory;
import org.jboss.netty.util.Timeout;
import org.jboss.netty.util.TimerTask;

import java.net.SocketAddress;
import java.security.NoSuchAlgorithmException;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.atomic.AtomicInteger;

public class VoiceEdgeManager extends ConnectionManager implements ChallengeConfirmHandler
{
    private static final Ref<VoiceEdgeManager> instance = new Ref<VoiceEdgeManager>();
    private final VoiceEdgeConfig config;
    private final MasterConnection master;
    private final Channel challengeChannel;

    final ConcurrentHashMap<String, VoiceConnection> serverMap = new ConcurrentHashMap<String, VoiceConnection>();
    final ConcurrentHashMap<Integer, Challenge> challengeMap = new ConcurrentHashMap<Integer, Challenge>();
    private final AtomicInteger challengeIdCounter = new AtomicInteger(31337);

    public VoiceEdgeManager(VoiceEdgeConfig config, ClientSocketChannelFactory factory, DatagramChannelFactory datagramFactory) throws NoSuchAlgorithmException
    {
        if (!instance.isNull() && !Utils.isUnitTestMode())
        {
            throw new IllegalStateException("There can be only one VoiceEdgeManager");
        }
        instance.set(this);
        this.config = config;
        this.master = new MasterConnection(new ClientBootstrap(factory), config.getMasterAddress());
        this.challengeChannel = Utils.createChallengeSocket(config.getChallengeBindAddress(), new ChallengeServerHandler(this), datagramFactory);
        this.master.connect();
    }

    public void shutdown()
    {
        master.close();
        challengeChannel.close().awaitUninterruptibly();
    }

    public int getNumClients()
    {
        return serverMap.size();
    }

    @Override
    public void registerClient(InboundConnection client)
    {
        VoiceConnection connection = (VoiceConnection) client;

        VoiceConnection oldConnection = serverMap.put(connection.getId(), connection);

        if (oldConnection != null)
        {
            //TODO: Technically, if this happens, do we really need to send the EdgeUserRegistered event?
            //TODO: Check if this will endup in the unregister event for this one comming after the register event for the one we are replacing. Same issue in UserEdgeConnection
            oldConnection.sendUnregisterAndClose(Protocol.Reasons.LoggedInElsewhere, "");
        }

        master.sendEdgeServerRegistered(connection.getId(), connection.getLocation(), connection.getVoipAddress(), connection.getVoipPort(), connection.getMaxClients());
        connection.setRegistered(true);

        if (VoiceEdgeServer.voiceEdgeLog.isTraceEnabled()) VoiceEdgeServer.voiceEdgeLog.trace(String.format(
                "Voice server %s registered from %s", client, client.getRemoteAddress().toString()));

        VoiceEdgeStats.getStatsManager().metric(VoiceEdgeMetrics.ClientConnects).incrementAndGet();
    }

    @Override
    public void unregisterClient(InboundConnection client, String reasonType, String reasonDesc)
    {
        VoiceConnection connection = (VoiceConnection) client;

        serverMap.remove(connection.getId());

        master.sendEdgeServerUnregistered(connection.getId());
        connection.setRegistered(false);

        if (VoiceEdgeServer.voiceEdgeLog.isTraceEnabled()) VoiceEdgeServer.voiceEdgeLog.trace(String.format(
                "Voice server %s unregistered", client));

        VoiceEdgeStats.getStatsManager().metric(VoiceEdgeMetrics.ClientDisconnect).incrementAndGet();
    }

    public void disconnectClient(String serverId, String reasonType, String reasonDesc)
    {
        VoiceConnection connection = serverMap.get(serverId);

        if (connection == null)
        {
            VoiceEdgeServer.voiceEdgeLog.warn(String.format("Voice server with id %s wasn't found at voice edge %s", serverId, connection));
            return;
        }

        connection.sendUnregisterAndClose(reasonType, reasonDesc);
        unregisterClient(connection, reasonType, reasonDesc);
    }


    public static VoiceEdgeManager getInstance()
    {
        if (instance.isNull())
        {
            throw new IllegalStateException("No VoiceEdgeManager instance registered");
        }
        return instance.get();
    }

    public VoiceEdgeConfig getConfig()
    {
        return config;
    }

    public void waitForMasterConnection() throws InterruptedException
    {
        master.initialConnectLatch.await();
    }

    public void relayEvent(Message msg)
    {
        master.write(msg);
        VoiceEdgeStats.getStatsManager().metric(VoiceEdgeMetrics.EventsRelayed).incrementAndGet();
    }

    public void relayCommand(Message msg)
    {
        String serverId = msg.getArgument(0);

        VoiceConnection connection = serverMap.get(serverId);

        if (connection != null)
        {
            connection.write(msg);
        }

        VoiceEdgeStats.getStatsManager().metric(VoiceEdgeMetrics.CommandsRelayed).incrementAndGet();
    }

    private class Challenge implements TimerTask
    {
        private final int challengeId;
        private final VoiceConnection voiceServer;
        private final SocketAddress address;
        private final Timeout timeout;

        private final long startTime;
        private long endTime;

        public Challenge(VoiceConnection voiceServer, SocketAddress address)
        {
            this.timeout = Maintenance.getInstance().scheduleTask(config.getChallengeTimeout(), this);

            this.challengeId = VoiceEdgeManager.this.challengeIdCounter.getAndIncrement();
            this.voiceServer = voiceServer;
            this.address = address;
            this.startTime = Utils.getTimestamp();

            challengeChannel.write(new ChallengePacket(challengeId), address);
        }

        @Override
        public String toString()
        {
            return "Challenge{" +
                    "challengeId=" + challengeId +
                    ", address=" + address +
                    ", timeout=" + timeout +
                    ", startTime=" + startTime +
                    ", endTime=" + endTime +
                    '}';
        }

        public void run(Timeout timeout) throws Exception
        {
            VoiceEdgeManager.this.expireChallenge(challengeId);
        }

        public void confirm()
        {
            // Cancel timer
            endTime = Utils.getTimestamp();
            timeout.cancel();
        }

        public VoiceConnection getVoiceEdgeConnection()
        {
            return voiceServer;
        }

        public long getEndTime()
        {
            return endTime;
        }

        public long getStartTime()
        {
            return startTime;
        }

        public int getChallengeId()
        {
            return challengeId;
        }
    }

    public void createChallenge(VoiceConnection voiceServer, SocketAddress address)
    {
        Challenge challenge = new Challenge(voiceServer, address);
        challengeMap.put(challenge.getChallengeId(), challenge);
    }

    public void confirmChallenge(int challengeId)
    {
        Challenge challenge = challengeMap.remove(challengeId);

        if (challenge == null)
        {
            return;
        }

        challenge.confirm();
        challenge.getVoiceEdgeConnection().finalizeRegistration();
    }

    public void expireChallenge(int challengeId)
    {
        Challenge challenge = challengeMap.remove(challengeId);

        if (challenge == null)
        {
            return;
        }

        challenge.getVoiceEdgeConnection().challengeExpired();

        VoiceEdgeStats.getStatsManager().metric(VoiceEdgeMetrics.ChallengeExpired).incrementAndGet();

    }

    synchronized public void dropAllClients()
    {
        for (VoiceConnection connection : serverMap.values())
        {
            connection.sendUnregisterAndClose(Protocol.Reasons.TryAgain, "Connection to master lost");
        }

        serverMap.clear();
    }


}
