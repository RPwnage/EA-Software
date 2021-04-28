package com.esn.geoip;

import com.esn.geoip.util.SearchProcessor;
import com.esn.geoip.util.Validator;

import java.util.ArrayList;
import java.util.List;
import java.util.Random;

/**
 * <p>
 * The base class for geographical indices. Subclasses need only to implement the four basic methods:
 * <ul>
 * <li>{@link #mapObject(Positionable)} </li>
 * <li>{@link #unmapObject(Positionable)} </li>
 * <li>{@link #size()} </li>
 * <li>{@link #processNearest(Position, com.esn.geoip.util.SearchProcessor)} </li>
 * </ul>
 * </p>
 * User: ronnie
 * Date: 2011-06-22
 * Time: 13:50
 */
public abstract class GeoMapper<T extends Positionable>
{
    public abstract void mapObject(T item);

    public abstract void unmapObject(T item);

    public abstract void processNearest(Position position, SearchProcessor<T> processor);

    public abstract int size();

    /**
     * Helper method for backwards compability. Finds maximum <tt>maxResults</tt> results that valid by the <tt>validator</tt>.
     *
     * @param longitude  the search position longitude (-180.0 to 180.0)
     * @param latitude   the search position latitude (-90.0 to 90.0)
     * @param maxResults the maximum number of valid results to find
     * @param validator  the verifier
     * @return the list of results, possibly empty
     */
    public List<T> findNearest(float longitude, float latitude, final int maxResults, final Validator<T> validator)
    {
        final List<T> output = new ArrayList<T>();
        processNearest(new SearchPostition(longitude, latitude), new SearchProcessor<T>()
        {
            public boolean execute(T t)
            {
                if (output.size() >= maxResults)
                {
                    // If we get here an underlying processor has not obeyed the result from this processor.
                    return false;
                }
                if (validator.verify(t))
                {
                    output.add(t);
                }
                return output.size() < maxResults;
            }
        });
        return output;
    }

    /**
     * <p>Helper method for getting a random object among the <tt>maxNearest</tt> nearest neighbours.</p>
     * <p><b>Note:</b>  as an optimization the search might stop before <tt>maxNearest</tt> neighbours have been processed.</p>
     *
     * @param longitude  the search position longitude (-180.0 to 180.0)
     * @param latitude   the search position latitude (-90.0 to 90.0)
     * @param maxNearest the maximum number of valid results to find
     * @param validator  the verifier
     * @return a random result among the <tt>maxNearest</tt> nearest, or <tt>null</tt> if no valid neighbour can be found
     */
    public T getRandomFromNearest(float longitude, float latitude, final int maxNearest, final Validator<T> validator)
    {
        RandomStopCollector<T> randomStopCollector = new RandomStopCollector<T>(maxNearest, validator);
        processNearest(new SearchPostition(longitude, latitude), randomStopCollector);
        return randomStopCollector.getResult();
    }


    private static class SearchPostition implements Position
    {
        private final float longitude;
        private final float latitude;

        private SearchPostition(float longitude, float latitude)
        {
            this.longitude = longitude;
            this.latitude = latitude;
        }

        public float getLatitude()
        {
            return latitude;
        }

        public float getLongitude()
        {
            return longitude;
        }

        public String getAddress()
        {
            return null;
        }
    }

    private static class RandomStopCollector<R extends Positionable> implements SearchProcessor<R>
    {
        private static final Random randomizer = new Random(System.currentTimeMillis());

        private final List<R> results;
        private final int maxResults;
        private R stop;
        private final Validator<R> validator;

        public RandomStopCollector(int maxResults, Validator<R> validator)
        {
            this.validator = validator;
            this.results = new ArrayList<R>(maxResults);
            this.maxResults = maxResults;
            this.stop = null;
        }

        public boolean execute(R result)
        {
            if (stop != null)
            {
                // Once a result has been set this instance will ignore consequent requests
                return false;
            }

            if (!validator.verify(result))
            {
                // If not valid, keep searching
                return true;
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
