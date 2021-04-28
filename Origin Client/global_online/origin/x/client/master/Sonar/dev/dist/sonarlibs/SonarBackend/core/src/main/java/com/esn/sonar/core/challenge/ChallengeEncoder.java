package com.esn.sonar.core.challenge;

import org.jboss.netty.buffer.LittleEndianHeapChannelBuffer;
import org.jboss.netty.channel.Channel;
import org.jboss.netty.channel.ChannelHandlerContext;
import org.jboss.netty.handler.codec.oneone.OneToOneEncoder;

public class ChallengeEncoder extends OneToOneEncoder
{
    @Override
    protected Object encode(ChannelHandlerContext ctx, Channel channel, Object msg) throws Exception
    {
        ChallengePacket packet = (ChallengePacket) msg;
        LittleEndianHeapChannelBuffer msgBuffer = new LittleEndianHeapChannelBuffer(64);
        msgBuffer.writeByte(0x03); // Flags
        msgBuffer.writeShort(0x0000); // Source
        msgBuffer.writeByte(0x02); // cmd
        msgBuffer.writeShort(0x0000); // Sequence
        msgBuffer.writeInt(packet.getChallengeId());

        return msgBuffer;
    }
}
