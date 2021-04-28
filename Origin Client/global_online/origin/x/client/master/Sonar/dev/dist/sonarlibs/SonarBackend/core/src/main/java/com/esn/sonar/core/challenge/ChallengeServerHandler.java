package com.esn.sonar.core.challenge;

import org.jboss.netty.channel.ChannelHandlerContext;
import org.jboss.netty.channel.ExceptionEvent;
import org.jboss.netty.channel.MessageEvent;
import org.jboss.netty.channel.SimpleChannelUpstreamHandler;

public class ChallengeServerHandler extends SimpleChannelUpstreamHandler
{
    private final ChallengeConfirmHandler confirmCandler;

    public ChallengeServerHandler(ChallengeConfirmHandler confirmHandler)
    {
        this.confirmCandler = confirmHandler;
    }

    @Override
    public void exceptionCaught(ChannelHandlerContext ctx, ExceptionEvent e) throws Exception
    {
        super.exceptionCaught(ctx, e);
    }

    @Override
    public void messageReceived(ChannelHandlerContext ctx, MessageEvent e) throws Exception
    {
        ChallengePacket packet = (ChallengePacket) e.getMessage();
        confirmCandler.confirmChallenge(packet.getChallengeId());
    }
}
