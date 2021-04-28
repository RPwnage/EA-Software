package com.esn.sonar.voiceedge;

import com.esn.sonar.core.StatsManager;

/**
 * User: ronnie
 * Date: 2011-07-06
 * Time: 16:21
 */
public class VoiceEdgeStats
{
    private static final StatsManager<VoiceEdgeMetrics, VoiceEdgeGauges> statsManager = new StatsManager<VoiceEdgeMetrics, VoiceEdgeGauges>("Voice Edge", VoiceEdgeMetrics.class, VoiceEdgeGauges.class);

    private VoiceEdgeStats()
    {
    }

    public static StatsManager<VoiceEdgeMetrics, VoiceEdgeGauges> getStatsManager()
    {
        return statsManager;
    }
}
