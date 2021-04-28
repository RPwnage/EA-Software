package com.esn.sonar.useredge;

import com.esn.sonar.core.StatsGauge;

/**
 * User: ronnie
 * Date: 2011-07-07
 * Time: 10:15
 */
public enum UserEdgeGauges implements StatsGauge
{
    NUM_OPERATORS
            {
                public Long get()
                {
                    return (long) UserEdgeServer.getInstance().getUserEdgeManager().getSphereCount();
                }
            },
    NUM_CLIENTS
            {
                public Long get()
                {
                    return (long) UserEdgeServer.getInstance().getUserEdgeManager().getClientCount();
                }
            }
}
