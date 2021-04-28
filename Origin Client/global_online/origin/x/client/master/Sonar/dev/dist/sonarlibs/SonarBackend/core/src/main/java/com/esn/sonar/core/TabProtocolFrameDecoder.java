package com.esn.sonar.core;

import org.jboss.netty.buffer.ChannelBuffer;
import org.jboss.netty.channel.Channel;
import org.jboss.netty.channel.ChannelHandlerContext;
import org.jboss.netty.handler.codec.frame.FrameDecoder;
import org.jboss.netty.util.CharsetUtil;


public class TabProtocolFrameDecoder extends FrameDecoder
{
    private final int maxFrameLength;
    static final byte delimiter = "\n".getBytes(CharsetUtil.US_ASCII)[0];

    //TODO: Test so that the maxFrameLength really works. A cheap way out could be to cut the channel if the maxFrameLength is overrun

    public TabProtocolFrameDecoder(int maxFrameLength)
    {
        this.maxFrameLength = maxFrameLength;
    }

    @Override
    protected Object decode(ChannelHandlerContext ctx, Channel channel, ChannelBuffer buffer) throws Exception
    {
        int length = -1;

        int start = buffer.readerIndex();
        int end = buffer.writerIndex();

        for (int index = start; index < end; index++)
        {
            if (buffer.getByte(index) == delimiter)
            {
                length = index - buffer.readerIndex();
                break;
            }
        }

        if (length < 1)
        {
            return null;
        }

        if (length > maxFrameLength)
        {
            buffer.skipBytes(length + 1);
            return null;
        }

        ChannelBuffer frame = buffer.slice(buffer.readerIndex(), length);
        buffer.skipBytes(length);
        buffer.skipBytes(1);
        return frame;
    }
}
