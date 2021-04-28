package com.esn.sonar.master.api.rest;

import com.esn.sonar.core.JsonStatisticsHandler;
import com.esn.sonar.core.StatisticsHandler;
import com.sun.jersey.api.container.ContainerFactory;
import com.sun.jersey.api.core.ClassNamesResourceConfig;
import com.sun.jersey.api.core.ResourceConfig;
import com.sun.jersey.server.impl.container.netty.NettyHandlerContainer;
import org.jboss.netty.channel.ChannelHandler;
import org.jboss.netty.channel.ChannelPipeline;
import org.jboss.netty.channel.ChannelPipelineFactory;
import org.jboss.netty.handler.codec.http.HttpRequestDecoder;
import org.jboss.netty.handler.codec.http.HttpResponseEncoder;
import se.cgbystrom.netty.http.FileServerHandler;
import se.cgbystrom.netty.http.router.RouterHandler;

import java.util.HashMap;
import java.util.LinkedHashMap;
import java.util.Map;

import static org.jboss.netty.channel.Channels.pipeline;

public class HttpPipelineFactory implements ChannelPipelineFactory
{
    private final LinkedHashMap<String, ChannelHandler> routes = new LinkedHashMap<String, ChannelHandler>();

    public HttpPipelineFactory(StatisticsHandler statsHandler)
    {
        NettyHandlerContainer jerseyHandler = getJerseyHandler();
        routes.put("startsWith:/api", jerseyHandler);

        String path = "classpath:///webapp";
        if (System.getProperty("livereload") != null)
            path = "master/src/main/resources/webapp";

        routes.put("equals:/_statistics", statsHandler);
        routes.put("equals:/_statistics/json", new JsonStatisticsHandler());
        routes.put("startsWith:/", new FileServerHandler(path, 3600));
    }

    public ChannelPipeline getPipeline() throws Exception
    {
        ChannelPipeline pipeline = pipeline();
        pipeline.addLast("decoder", new HttpRequestDecoder());
        pipeline.addLast("encoder", new HttpResponseEncoder());
        pipeline.addLast("router", new RouterHandler(routes));
        return pipeline;
    }

    private NettyHandlerContainer getJerseyHandler()
    {
        Map<String, Object> props = new HashMap<String, Object>();
        props.put(ClassNamesResourceConfig.PROPERTY_CLASSNAMES, "com.esn.sonar.master.api.rest.JsonExceptionMapper;com.esn.sonar.master.api.rest.RestApi");
        props.put(NettyHandlerContainer.PROPERTY_BASE_URI, "http://localhost:8080/");
        ResourceConfig rcf = new ClassNamesResourceConfig(props);
        return ContainerFactory.createContainer(NettyHandlerContainer.class, rcf);
    }
}