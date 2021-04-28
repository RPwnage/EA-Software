package com.esn.sonar.allinone.client;

import com.esn.sonar.allinone.AllInOneConfig;
import com.esn.sonar.core.MessageDecoder;
import com.esn.sonar.core.MessageEncoder;
import com.esn.sonar.core.TabProtocolFrameDecoder;
import com.esn.sonar.core.Token;
import org.jboss.netty.bootstrap.ClientBootstrap;
import org.jboss.netty.channel.ChannelHandler;
import org.jboss.netty.channel.ChannelPipeline;
import org.jboss.netty.channel.ChannelPipelineFactory;
import org.jboss.netty.channel.Channels;
import org.jboss.netty.channel.socket.DatagramChannelFactory;
import org.jboss.netty.channel.socket.nio.NioClientSocketChannelFactory;
import org.jboss.netty.channel.socket.nio.NioDatagramChannelFactory;
import org.jboss.netty.channel.socket.nio.NioServerSocketChannelFactory;

import java.util.concurrent.Executors;

public class SonarClientFactory
{
    final public NioClientSocketChannelFactory socketChannelFactory = new NioClientSocketChannelFactory(Executors.newCachedThreadPool(), Executors.newCachedThreadPool());
    final public NioServerSocketChannelFactory serverSocketFactory = new NioServerSocketChannelFactory(Executors.newCachedThreadPool(), Executors.newCachedThreadPool());
    final public DatagramChannelFactory datagramChannelFactory = new NioDatagramChannelFactory(Executors.newCachedThreadPool());

    static int refCount = 0;

    public SonarClientFactory()
    {
        refCount++;
    }

    public void release()
    {
        socketChannelFactory.releaseExternalResources();
        datagramChannelFactory.releaseExternalResources();
        serverSocketFactory.releaseExternalResources();
        refCount--;
    }

    private static class SonarPipelineFactory implements ChannelPipelineFactory
    {
        private final ChannelHandler clientHandler;

        private SonarPipelineFactory(ChannelHandler clientHandler)
        {
            this.clientHandler = clientHandler;
        }

        public ChannelPipeline getPipeline() throws Exception
        {
            ChannelPipeline pipeline = Channels.pipeline();
            pipeline.addLast("frame-decoder", new TabProtocolFrameDecoder(65536));
            pipeline.addLast("decoder-msg", new MessageDecoder());
            pipeline.addLast("encoder-msg", new MessageEncoder());
            pipeline.addLast("handler", clientHandler);
            return pipeline;

        }
    }

    public FakeUserClient newUserClient(Token controlToken, final AllInOneConfig config, boolean skipRegistration, boolean doubleRegistration)
    {
        ClientBootstrap bootstrap = new ClientBootstrap(socketChannelFactory);
        final FakeUserClient clientHandler = new FakeUserClient(bootstrap, controlToken, config, skipRegistration, doubleRegistration);
        bootstrap.setPipelineFactory(new SonarPipelineFactory(clientHandler));
        return clientHandler;
    }

    public FakeVoiceServerClient newVoiceServerClient(final AllInOneConfig config, boolean invalidChallenge, boolean dropChallenge, boolean skipRegistration, boolean doubleRegistration, boolean ignoreKeepalive, int maxClients)
    {
        ClientBootstrap bootstrap = new ClientBootstrap(socketChannelFactory);
        final FakeVoiceServerClient clientHandler = new FakeVoiceServerClient(datagramChannelFactory, bootstrap, config, invalidChallenge, dropChallenge, skipRegistration, doubleRegistration, ignoreKeepalive, maxClients);
        bootstrap.setPipelineFactory(new SonarPipelineFactory(clientHandler));
        return clientHandler;
    }

    public FakeOperatorClient newFakeOperatorClient(final AllInOneConfig config)
    {
        ClientBootstrap bootstrap = new ClientBootstrap(socketChannelFactory);
        final FakeOperatorClient clientHandler = new FakeOperatorClient(bootstrap, config);
        bootstrap.setPipelineFactory(new SonarPipelineFactory(clientHandler));
        return clientHandler;
    }

    public FakeEventClient newEventClient(AllInOneConfig config, String operatorId)
    {
        ClientBootstrap bootstrap = new ClientBootstrap(socketChannelFactory);
        final FakeEventClient clientHandler = new FakeEventClient(bootstrap, config, operatorId);
        bootstrap.setPipelineFactory(new SonarPipelineFactory(clientHandler));
        return clientHandler;
    }


}

