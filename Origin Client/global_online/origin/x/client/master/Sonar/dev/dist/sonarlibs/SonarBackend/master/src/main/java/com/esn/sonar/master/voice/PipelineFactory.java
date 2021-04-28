package com.esn.sonar.master.voice;

import org.jboss.netty.channel.ChannelPipeline;
import org.jboss.netty.channel.ChannelPipelineFactory;

import static com.esn.sonar.core.util.Utils.getTabProtocolPipeline;

public class PipelineFactory implements ChannelPipelineFactory
{
    private VoiceManager voiceManager;

    public PipelineFactory(VoiceManager voiceManager)
    {
        this.voiceManager = voiceManager;
    }

    public ChannelPipeline getPipeline() throws Exception
    {
        return getTabProtocolPipeline(new VoiceEdgeConnection(voiceManager));
    }
}
