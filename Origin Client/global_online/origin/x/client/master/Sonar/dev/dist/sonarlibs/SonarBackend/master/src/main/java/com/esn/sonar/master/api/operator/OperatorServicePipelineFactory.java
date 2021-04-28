package com.esn.sonar.master.api.operator;

import org.jboss.netty.channel.ChannelPipeline;
import org.jboss.netty.channel.ChannelPipelineFactory;

import static com.esn.sonar.core.util.Utils.getTabProtocolPipeline;

public class OperatorServicePipelineFactory implements ChannelPipelineFactory
{
    private final OperatorService operatorService;

    public OperatorServicePipelineFactory(OperatorService operatorService)
    {
        this.operatorService = operatorService;
    }

    public ChannelPipeline getPipeline() throws Exception
    {
        return getTabProtocolPipeline(new OperatorServiceConnection(operatorService));
    }
}