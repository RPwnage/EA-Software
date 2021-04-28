package com.esn.sonar.core;

import org.jboss.netty.buffer.ChannelBuffer;
import org.jboss.netty.channel.Channel;
import org.jboss.netty.channel.ChannelHandlerContext;
import org.jboss.netty.handler.codec.oneone.OneToOneDecoder;
import org.jboss.netty.util.CharsetUtil;

import java.util.Arrays;

import static com.esn.sonar.core.util.Utils.tokenize;

public class MessageDecoder extends OneToOneDecoder
{
    public MessageDecoder()
    {
    }

    protected Object decode(ChannelHandlerContext channelHandlerContext, Channel channel, Object o) throws Exception
    {
        ChannelBuffer cb = (ChannelBuffer) o;

        String str = cb.toString(CharsetUtil.UTF_8);

        if (str == null)
        {
            return null;
        }

        String args[] = tokenize(str, '\t');

        if (args.length < 2)
        {
            return null;
        }

        try
        {
            long id = Long.valueOf(args[0]);
            String command = args[1];

            return new Message(id, command, Arrays.copyOfRange(args, 2, args.length));
        } catch (NumberFormatException exception)
        {
            return null;
        }
    }
}
