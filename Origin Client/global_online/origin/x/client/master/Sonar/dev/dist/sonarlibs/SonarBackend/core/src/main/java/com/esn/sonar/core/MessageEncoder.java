package com.esn.sonar.core;

import org.jboss.netty.buffer.ChannelBuffers;
import org.jboss.netty.channel.Channel;
import org.jboss.netty.channel.ChannelHandlerContext;
import org.jboss.netty.handler.codec.oneone.OneToOneEncoder;

public class MessageEncoder extends OneToOneEncoder
{
    public MessageEncoder()
    {
    }

    @Override
    protected Object encode(ChannelHandlerContext channelHandlerContext, Channel channel, Object o) throws Exception
    {
        Message msg = (Message) o;
        return ChannelBuffers.wrappedBuffer(msg.getBytes());
    }
}
