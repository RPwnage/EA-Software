package com.esn.sonar.core.util;

import java.util.ArrayList;
import java.util.List;
import java.util.Random;

/**
 * User: ronnie
 * Date: 2011-06-20
 * Time: 15:29
 */
public interface Processor<T>
{
    boolean execute(T t);


    public static class MaxResultsCollector<R> implements Processor<R>
    {
        private final List<R> results;
        private final int maxResults;

        public MaxResultsCollector(int maxResults)
        {
            this.results = new ArrayList<R>(maxResults);
            this.maxResults = maxResults;
        }

        public boolean execute(R result)
        {
            results.add(result);
            return maxResults > results.size();
        }

        public List<R> getResults()
        {
            return results;
        }
    }

    public static class RandomStopCollector<R> implements Processor<R>
    {
        private static final Random randomizer = new Random(System.currentTimeMillis());

        private final List<R> results;
        private final int maxResults;
        private R stop;

        public RandomStopCollector(int maxResults)
        {
            this.results = new ArrayList<R>(maxResults);
            this.maxResults = maxResults;
            this.stop = null;
        }

        public boolean execute(R result)
        {
            if (result != null)
            {
                // Once a result has been set this instance will ignore consequent requests
                return false;
            }
            if (randomizer.nextInt(maxResults) == 0)
            {
                // There's a 1 in maxResults chance that this result is it!
                stop = result;
                return false;
            }

            results.add(result);

            if (results.size() == maxResults)
            {
                // Maximum number of results is reached. Pick one from the gathered results.
                stop = results.get(randomizer.nextInt(results.size()));
                return false;
            }
            return true;
        }

        public R getResult()
        {
            return stop != null
                    ? stop
                    : results.isEmpty() ? null : results.get(randomizer.nextInt(results.size()));
        }
    }

}
