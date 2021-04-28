package com.esn.sonar.useredge;

import com.esn.sonar.core.StatsManager;

/**
 * User: ronnie
 * Date: 2011-07-06
 * Time: 15:44
 */
public class UserEdgeStats
{
    private static final StatsManager<UserEdgeMetrics, UserEdgeGauges> statsManager = new StatsManager<UserEdgeMetrics, UserEdgeGauges>("User Edge", UserEdgeMetrics.class, UserEdgeGauges.class);

    private UserEdgeStats()
    {
    }

    public static StatsManager<UserEdgeMetrics, UserEdgeGauges> getStatsManager()
    {
        return statsManager;
    }
}
