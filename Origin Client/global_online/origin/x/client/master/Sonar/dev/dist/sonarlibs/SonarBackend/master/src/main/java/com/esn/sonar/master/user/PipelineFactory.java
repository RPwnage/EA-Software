package com.esn.sonar.master.user;

import org.jboss.netty.channel.ChannelPipeline;
import org.jboss.netty.channel.ChannelPipelineFactory;

import static com.esn.sonar.core.util.Utils.getTabProtocolPipeline;

public class PipelineFactory implements ChannelPipelineFactory
{
    private final UserManager userManager;

    public PipelineFactory(UserManager userManager)
    {
        this.userManager = userManager;
    }

    public ChannelPipeline getPipeline() throws Exception
    {
        return getTabProtocolPipeline(new UserEdgeConnection(userManager));
    }
}
