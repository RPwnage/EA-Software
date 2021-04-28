package com.esn.sonar.master.api.event;

import org.jboss.netty.channel.ChannelPipeline;
import org.jboss.netty.channel.ChannelPipelineFactory;

import static com.esn.sonar.core.util.Utils.getTabProtocolPipeline;

public class EventServicePipelineFactory implements ChannelPipelineFactory
{
    public ChannelPipeline getPipeline() throws Exception
    {
        return getTabProtocolPipeline(new EventConnection());
    }
}
