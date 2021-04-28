package com.esn.sonar.master;

import org.apache.log4j.Logger;
import org.jboss.netty.channel.*;

import java.net.InetSocketAddress;

@ChannelHandler.Sharable
public class LoggingHandler implements ChannelUpstreamHandler, ChannelDownstreamHandler
{
    private final static Logger log = Logger.getLogger(LoggingHandler.class);

    public void handleDownstream(ChannelHandlerContext ctx, ChannelEvent e) throws Exception
    {
        //log(e, " << ");
        ctx.sendDownstream(e);
    }

    public void handleUpstream(ChannelHandlerContext ctx, ChannelEvent e) throws Exception
    {
        //log(e, " >> ");
        ctx.sendUpstream(e);
    }

    private void log(ChannelEvent e, String direction)
    {
        if (!(e instanceof MessageEvent))
            return;

        String hostname = ((InetSocketAddress) e.getChannel().getRemoteAddress()).getAddress().getHostAddress();
        int port = ((InetSocketAddress) e.getChannel().getRemoteAddress()).getPort();

        Object o = ((MessageEvent) e).getMessage();
        log.debug(hostname + ":" + port + direction + o);
    }
}
