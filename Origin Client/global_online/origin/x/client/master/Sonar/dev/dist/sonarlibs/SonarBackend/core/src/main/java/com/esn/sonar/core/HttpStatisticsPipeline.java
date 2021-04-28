package com.esn.sonar.core;

import org.jboss.netty.channel.ChannelPipeline;
import org.jboss.netty.channel.ChannelPipelineFactory;
import org.jboss.netty.handler.codec.http.HttpRequestDecoder;
import org.jboss.netty.handler.codec.http.HttpResponseEncoder;

import static org.jboss.netty.channel.Channels.pipeline;

public class HttpStatisticsPipeline implements ChannelPipelineFactory
{
    private StatsManager[] statsManagers;

    public HttpStatisticsPipeline(StatsManager[] statsManagers)
    {
        this.statsManagers = statsManagers;
    }

    public ChannelPipeline getPipeline() throws Exception
    {
        ChannelPipeline pipeline = pipeline();
        pipeline.addLast("decoder", new HttpRequestDecoder());
        pipeline.addLast("encoder", new HttpResponseEncoder());
        pipeline.addLast("", new StatisticsHandler(statsManagers));
        return pipeline;
    }
}
