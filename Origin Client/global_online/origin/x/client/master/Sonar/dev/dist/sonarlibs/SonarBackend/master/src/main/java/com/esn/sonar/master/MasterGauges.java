package com.esn.sonar.master;

import com.esn.sonar.core.StatsGauge;

import java.util.Collection;

/**
 * User: ronnie
 * Date: 2011-07-07
 * Time: 10:16
 */
public enum MasterGauges implements StatsGauge
{
    NUM_OPERATORS
            {
                public Long get()
                {
                    return (long) MasterServer.getInstance().getOperatorManager().getOperators().size();
                }
            },
    NUM_VOICE_SERVERS
            {
                public Long get()
                {
                    return (long) MasterServer.getInstance().getVoiceManager().getServerCount();
                }
            },
    NUM_VOICE_CHANNELS
            {
                public Long get()
                {
                    long count = 0;
                    Collection<Operator> operators = MasterServer.getInstance().getOperatorManager().getOperators();
                    for (Operator operator : operators)
                    {
                        count += operator.getChannelManager().getChannelCount();
                    }
                    return count;
                }
            },
    NUM_VOICECACHE_CHANNELS
            {
                public Long get()
                {
                    long count = 0;
                    Collection<Operator> operators = MasterServer.getInstance().getOperatorManager().getOperators();
                    for (Operator operator : operators)
                    {
                        count += operator.getCache().getChannelCount();
                    }
                    return count;
                }
            },
    NUM_VOICECACHE_USERS
            {
                public Long get()
                {
                    long count = 0;
                    Collection<Operator> operators = MasterServer.getInstance().getOperatorManager().getOperators();
                    for (Operator operator : operators)
                    {
                        count += operator.getCache().getUserCount();
                    }
                    return count;
                }
            },
    NUM_JOINCHANNELFUTURE
            {
                public Long get()
                {
                    return (long) MasterServer.getInstance().getUserManager().getJoinChannelFutureCount();
                }
            }
}
