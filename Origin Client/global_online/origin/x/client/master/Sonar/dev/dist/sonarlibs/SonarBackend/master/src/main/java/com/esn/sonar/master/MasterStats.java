package com.esn.sonar.master;

import com.esn.sonar.core.StatsManager;

/**
 * User: ronnie
 * Date: 2011-07-06
 * Time: 15:17
 */
public class MasterStats
{
    private static final StatsManager<MasterMetrics, MasterGauges> statsManager = new StatsManager<MasterMetrics, MasterGauges>("Master", MasterMetrics.class, MasterGauges.class);

    private MasterStats()
    {
    }

    public static StatsManager<MasterMetrics, MasterGauges> getStatsManager()
    {
        return statsManager;
    }
}
