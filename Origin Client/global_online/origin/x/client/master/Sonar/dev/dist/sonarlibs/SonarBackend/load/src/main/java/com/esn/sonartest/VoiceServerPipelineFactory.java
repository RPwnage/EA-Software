package com.esn.sonartest;

import com.esn.sonar.core.MessageDecoder;
import com.esn.sonar.core.MessageEncoder;
import com.esn.sonar.core.TabProtocolFrameDecoder;
import org.jboss.netty.channel.ChannelPipeline;
import org.jboss.netty.channel.ChannelPipelineFactory;
import org.jboss.netty.channel.SimpleChannelUpstreamHandler;
import org.jboss.netty.channel.StaticChannelPipeline;

public class VoiceServerPipelineFactory implements ChannelPipelineFactory
{
    private final SimpleChannelUpstreamHandler handler;
    static final MessageDecoder messageDecoder = new MessageDecoder();
    static final MessageEncoder messageEncoder = new MessageEncoder();

    public VoiceServerPipelineFactory(SimpleChannelUpstreamHandler handler)
    {
        this.handler = handler;
    }

    public ChannelPipeline getPipeline() throws Exception
    {
        return new StaticChannelPipeline(new TabProtocolFrameDecoder(65536), messageDecoder, messageEncoder, handler);
    }
}
