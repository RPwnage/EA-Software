package com.esn.sonar.voiceedge;

import org.jboss.netty.channel.ChannelPipeline;
import org.jboss.netty.channel.ChannelPipelineFactory;

import static com.esn.sonar.core.util.Utils.getTabProtocolPipeline;

public class PipelineFactory implements ChannelPipelineFactory
{
    private VoiceEdgeConfig config;

    public PipelineFactory(VoiceEdgeConfig config)
    {
        this.config = config;
    }

    public ChannelPipeline getPipeline() throws Exception
    {
        return getTabProtocolPipeline(new VoiceConnection(config));
    }
}
