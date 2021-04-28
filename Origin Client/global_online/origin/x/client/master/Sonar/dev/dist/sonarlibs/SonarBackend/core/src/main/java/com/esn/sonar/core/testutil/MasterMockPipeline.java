package com.esn.sonar.core.testutil;

import com.esn.sonar.core.util.Utils;
import org.jboss.netty.channel.ChannelPipeline;
import org.jboss.netty.channel.ChannelPipelineFactory;

public class MasterMockPipeline implements ChannelPipelineFactory
{
    public ChannelPipeline getPipeline() throws Exception
    {
        return Utils.getTabProtocolPipeline(new MasterMockHandler());

    }
}
