package com.esn.sonar.core.challenge;

import org.jboss.netty.buffer.ChannelBuffer;
import org.jboss.netty.channel.Channel;
import org.jboss.netty.channel.ChannelHandlerContext;
import org.jboss.netty.handler.codec.oneone.OneToOneDecoder;

public class ChallengeDecoder extends OneToOneDecoder
{
    @Override
    protected Object decode(ChannelHandlerContext ctx, Channel channel, Object msg) throws Exception
    {
        ChannelBuffer buffer = (ChannelBuffer) msg;

        byte flags = buffer.readByte();
        short source = buffer.readShort();
        byte cmd = buffer.readByte();
        short seq = buffer.readShort();
        int challengeId = buffer.readInt();

        if (flags != (byte) 0x03) return null;
        if (source != (short) 0x0000) return null;
        if (cmd != (byte) 0x02) return null;
        if (seq != 0) return null;

        return new ChallengePacket(challengeId);
    }
}
