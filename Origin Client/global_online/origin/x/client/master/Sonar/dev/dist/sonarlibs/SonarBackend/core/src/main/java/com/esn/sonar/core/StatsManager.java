package com.esn.sonar.core;

import com.twitter.common.application.ActionController;
import com.twitter.common.quantity.Amount;
import com.twitter.common.quantity.Time;
import com.twitter.common.stats.Percentile;
import com.twitter.common.stats.Rate;
import com.twitter.common.stats.RequestStats;
import com.twitter.common.stats.Stat;
import com.twitter.common.stats.Stats;
import com.twitter.common.stats.TimeSeries;
import com.twitter.common.stats.TimeSeriesRepository;
import com.twitter.common.stats.TimeSeriesRepositoryImpl;
import org.apache.log4j.Logger;

import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.EnumMap;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.concurrent.atomic.AtomicLong;

/**
 * <p>The generic stats manager. Keeps track of both RPS (read-only) and time series data for any metrics enum, {@link M} and {@link G}.</p>
 * <p>The data for {@link M} can be read and <b>incremented</b> by a direct {@link AtomicLong} reference retrieved by {@link #metric(Enum)}
 * and the RPS double value can be read by {@link #getMetricRps(Enum)}.
 * </p>
 * <p>The data for {@link G} can be read but not updated. It is sampled internally once in a minute (or on demand) by
 * calling its {@link com.google.common.base.Supplier#get() Supplier method}.</p>
 * <p>Additionally, the manager also tracks {@link RequestStats request statistics} which is updatable by achieving a reference from {@link #getRequestStats()}</p>
 * <p></p>
 * <p>
 * User: ronnie
 * Date: 2011-07-06
 * Time: 09:00
 * </p>
 */
public class StatsManager<M extends Enum<M>, G extends Enum<G> & StatsGauge>
{
    private static Logger log = Logger.getLogger(StatsManager.class);
    private static final Amount<Long, Time> samplePeriod = Amount.of(1L, Time.MINUTES);
    private static final Amount<Long, Time> retentionPeriod = Amount.of(24L, Time.HOURS);
    private static final TimeSeriesRepository timeSeriesRepository = new TimeSeriesRepositoryImpl(samplePeriod, retentionPeriod);
    /**
     * The percentiles to track
     */
    public static final int[] PERCENTILES = new int[]{10, 50, 90, 95, 96, 97, 98, 99};
    /**
     * The percent of events to sample [0, 100]
     */
    public static final int SAMPLE_PERCENT = 10;

    static
    {
        /*
            A singleton TimeSeriesRepository since it collects stats from the global stats repository
            with exported stats from every StatsManager (this class).
         */
        timeSeriesRepository.start(new ActionController());
        if (log.isDebugEnabled())
        {
            log.debug("TimeSeriesRepository started  (sample period: " + samplePeriod + " retention period: " + retentionPeriod + ").");
        }
    }

    private final Map<M, AtomicLong> metrics;
    private final Map<G, Stat<Long>> gauges;

    private final Map<String, Stat<Double>> metricsRps = new HashMap<String, Stat<Double>>();
    private final String name;
    private final Class<M> metricsClass;
    private final Class<G> gaugesClass;
    private final RequestStats requestStats;

    public StatsManager(String name, Class<M> metricsClass, Class<G> gaugesClass)
    {
        this.name = name;
        this.requestStats = new RequestStats(requestStatsName(), new Percentile<Long>(requestStatsName(), SAMPLE_PERCENT, PERCENTILES));
        this.metricsClass = metricsClass;
        this.gaugesClass = gaugesClass;
        metrics = new EnumMap<M, AtomicLong>(metricsClass);
        gauges = new EnumMap<G, Stat<Long>>(gaugesClass);

        for (final M metric : metricsClass.getEnumConstants())
        {
            AtomicLong atomicLong = Stats.STATS_PROVIDER.makeCounter(safeName(metric));
            metrics.put(metric, atomicLong);

            String rpsName = rpsName(metric);
            metricsRps.put(rpsName, Stats.export(Rate.of(rpsName, atomicLong).build()));
        }
        if (log.isDebugEnabled())
        {
            log.debug("Exported stats for metrics " + metricsClass.getName());
        }

        for (final G gauge : gaugesClass.getEnumConstants())
        {
            gauges.put(gauge, Stats.STATS_PROVIDER.makeGauge(safeName(gauge), gauge));
        }
        if (log.isDebugEnabled())
        {
            log.debug("Exported stats for gauges " + gaugesClass.getName());
        }
    }

    public String requestStatsName()
    {
        return safeName(name + "_req_stats");
    }

    public Stat[] getRequestStatsAccumulated()
    {
        // "Master_req_stats_errors"
        List<Stat> accumulated = new ArrayList<Stat>();
        Iterable<? extends Stat> variables = Stats.getVariables();
        for (Stat stat : variables)
        {
            String statName = stat.getName();
            if (statName.startsWith(requestStatsName()) && !statName.endsWith("_percentile"))
            {
                if (!statName.contains("_reconnect") && !statName.contains("_timeout") && !statName.contains("_micros_total"))
                {
                    accumulated.add(stat);
                }
            }
        }
        Collections.sort(accumulated, accumulatedComparator);
        return accumulated.toArray(new Stat[accumulated.size()]);
    }

    public Stat[] getRequestStatsPercentiles()
    {
        List<Stat> percentiles = new ArrayList<Stat>();
        Iterable<? extends Stat> variables = Stats.getVariables();
        for (Stat stat : variables)
        {
            String statName = stat.getName();
            // "Master_req_stats_10th_percentile"
            if (statName.startsWith(requestStatsName()) && statName.endsWith("_percentile"))
            {
                percentiles.add(stat);
            }
        }
        Collections.sort(percentiles, percentileComparator);
        return percentiles.toArray(new Stat[percentiles.size()]);
    }

    /**
     * Get the reference to the request stats for updating.
     *
     * @return the request stats tracker
     */
    public RequestStats getRequestStats()
    {
        return requestStats;
    }

    public M[] getMetricsConstants()
    {
        return metricsClass.getEnumConstants();
    }

    public G[] getGaugesConstants()
    {
        return gaugesClass.getEnumConstants();
    }

    /**
     * Safe naming of enum values instead of using the value name alone. This ensures
     * that two enum values but from different classes do not share the same stats reference.
     *
     * @param e the monitor enum value
     * @return the enum value name prefixed with package and class name
     */
    private String safeName(Enum e)
    {
        return safeName(e.getClass().getName() + "." + e.name());
    }

    private String safeName(String s)
    {
        return s.replace(' ', '_').replace('.', '_').replace('$', '_');
    }

    private String rpsName(Enum e)
    {
        return safeName(e) + "_per_sec";
    }

    /**
     * Get the {@link TimeSeries} of samples for the given enum value
     *
     * @param metric the monitor enum value
     * @return the collected samples over time
     */
    public TimeSeries getMetricTimeSeries(M metric)
    {
        return timeSeriesRepository.get(safeName(metric));
    }

    /**
     * Get the {@link TimeSeries} of samples for the given enum value
     *
     * @param gauge the monitor enum value
     * @return the collected samples over time
     */
    public TimeSeries getGaugeTimeSeries(G gauge)
    {
        return timeSeriesRepository.get(safeName(gauge));
    }

    /**
     * Get the most recent read-only RPS (requests per second) value from the given enum value
     *
     * @param metric the monitor enum value
     * @return the RPS value
     */
    public double getMetricRps(M metric)
    {
        return metricsRps.get(rpsName(metric)).read();
    }

    /**
     * Get a reference to the stats counter for given {@link M monitor value}
     *
     * @param metric the monitor
     * @return a reference to the internal counter
     */
    public AtomicLong metric(M metric)
    {
        return metrics.get(metric);
    }

    /**
     * Get a reading of the stats counter for given {@link G monitor value}
     *
     * @param gauge the monitor
     * @return the current value held by the gauge
     */
    public Long gauge(G gauge)
    {
        return gauges.get(gauge).read();
    }

    public long getMetricsValue(M monitorValue, int startTick, int tickCount)
    {
        long startValue = metric(monitorValue).get();
        TimeSeries timeSeries = getMetricTimeSeries(monitorValue);
        if (timeSeries == null)
        {
            if (startTick == 0 && tickCount == 1)
            {
                return startValue; // current value
            }
            return 0; // nothing recorded....
        }

        Iterator<Number> iter = timeSeries.getSamples().iterator();

        List<Number> numbers = new ArrayList<Number>();
        while (iter.hasNext())
        {
            numbers.add(iter.next());
        }
        Collections.reverse(numbers);

        if (numbers.size() > startTick)
        {
            if (startTick != 0)
            {
                startValue = numbers.get(startTick - 1).longValue();
            }
        } else
        {
            return 0; // beyond collected data
        }

        int endTick = startTick + tickCount;
        long endValue = 0;
        if (endTick < numbers.size())
        {
            endValue = numbers.get(endTick - 1).longValue();
        }


        return startValue - endValue;
    }

    public long getGaugeValue(G gaugeValue, int startTick, int tickCount)
    {
        long ret = 0;

        TimeSeries timeSeries = getGaugeTimeSeries(gaugeValue);
        if (timeSeries == null || (startTick == 0 && tickCount == 1))
        {
            // Current (not yet stored in time series) gauge value
            return gauge(gaugeValue);
        }

        Iterator<Number> iter = timeSeries.getSamples().iterator();

        List<Number> numbers = new ArrayList<Number>();
        while (iter.hasNext())
        {
            numbers.add(iter.next());
        }
        Collections.reverse(numbers);
        iter = numbers.iterator();

        int ticks = 0;
        int ticksEnd = startTick + tickCount;

        while (iter.hasNext() && ticks < ticksEnd)
        {
            long nextValue = iter.next().longValue();
            if (ticks >= startTick && nextValue != 0)
            {
                ret = nextValue;
            }
            ticks += 1;
        }

        return ret;
    }

    public final String getName()
    {
        return name;
    }

    private final Comparator<Stat> accumulatedComparator = new Comparator<Stat>()
    {
        public int compare(Stat o1, Stat o2)
        {
            String name1 = o1.getName();
            String name2 = o2.getName();
            return name1.compareTo(name2);
        }
    };

    private final Comparator<Stat> percentileComparator = new Comparator<Stat>()
    {
        public int compare(Stat o1, Stat o2)
        {
            String name1 = o1.getName();
            String name2 = o2.getName();
            int c1 = Integer.parseInt(name1.substring(requestStatsName().length() + 1, name1.indexOf("th_")));
            int c2 = Integer.parseInt(name2.substring(requestStatsName().length() + 1, name2.indexOf("th_")));
            return c1 - c2;
        }
    };
}
