package com.esn.sonar.allinone;

import com.esn.sonar.core.AbstractConfig;
import com.esn.sonar.master.MasterConfig;
import com.esn.sonar.useredge.UserEdgeConfig;
import com.esn.sonar.voiceedge.VoiceEdgeConfig;

import java.io.IOException;


public class AllInOneConfig
{
    public final MasterConfig masterConfig;
    public final VoiceEdgeConfig voiceEdgeConfig;
    public final UserEdgeConfig userEdgeConfig;

    public AllInOneConfig() throws IOException, AbstractConfig.ConfigurationException
    {
        masterConfig = new MasterConfig(System.getProperty("sonar.config"));
        voiceEdgeConfig = new VoiceEdgeConfig(System.getProperty("sonar.config"));
        userEdgeConfig = new UserEdgeConfig(System.getProperty("sonar.config"));
    }
}
