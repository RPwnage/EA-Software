package com.esn.sonar.useredge;

import org.jboss.netty.channel.ChannelPipeline;
import org.jboss.netty.channel.ChannelPipelineFactory;

import static com.esn.sonar.core.util.Utils.getTabProtocolPipeline;

public class PipelineFactory implements ChannelPipelineFactory
{
    private UserEdgeConfig config;

    public PipelineFactory(UserEdgeConfig config)
    {
        this.config = config;
    }

    public ChannelPipeline getPipeline() throws Exception
    {
        return getTabProtocolPipeline(new UserConnection(config));
    }
}
