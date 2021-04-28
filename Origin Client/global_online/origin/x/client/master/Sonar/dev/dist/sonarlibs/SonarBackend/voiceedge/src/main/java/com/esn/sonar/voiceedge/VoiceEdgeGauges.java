package com.esn.sonar.voiceedge;

import com.esn.sonar.core.StatsGauge;

/**
 * User: ronnie
 * Date: 2011-07-07
 * Time: 10:13
 */
public enum VoiceEdgeGauges implements StatsGauge
{
    NUM_CLIENTS
            {
                public Long get()
                {
                    return (long) VoiceEdgeServer.getInstance().getVoiceEdgeManager().getNumClients();
                }
            }
}
