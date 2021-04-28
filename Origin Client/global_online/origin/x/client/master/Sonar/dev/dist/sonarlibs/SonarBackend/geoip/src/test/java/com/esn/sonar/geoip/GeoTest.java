package com.esn.sonar.geoip;

import com.esn.geoip.*;
import com.esn.geoip.util.LocationGenerator;
import com.esn.geoip.util.Validator;
import org.junit.Test;

import java.io.IOException;
import java.util.ArrayList;
import java.util.Collection;
import java.util.List;

import static org.junit.Assert.assertTrue;

/**
 * User: ronnie
 * Date: 2011-06-22
 * Time: 10:54
 */
public class GeoTest
{
    private static final List<Class<? extends GeoMapper<Positionable>>> implementations = new ArrayList<Class<? extends GeoMapper<Positionable>>>();

    static
    {
        implementations.add(GeoGridV1.class);
        //implementations.add(GeoGridV2.class);
        implementations.add(SpatialMapper.class);
    }

    public static final int CANDIDATES_SIZE = 25;

    private final Validator<Positionable> validator = new Validator<Positionable>()
    {
        public boolean verify(Positionable item)
        {
            return true;
        }
    };

    private GeoLiteCity geopIpProvider = null;
    private LocationGenerator locationGenerator;

    public GeoTest()
    {
    }

    private void setUp() throws IOException
    {
        this.geopIpProvider = new GeoLiteCity();
        this.geopIpProvider.loadFromJar(this.getClass(), "/geoip");
        this.locationGenerator = new LocationGenerator(geopIpProvider);
    }

    private GeoMapper<Positionable> setUpGrid(int size, Class<? extends GeoMapper<Positionable>> implementation) throws Exception
    {
        final GeoMapper<Positionable> grid = implementation.newInstance();

        for (int index = 0; index < size; index++)
        {
            String ip = locationGenerator.getRandomIP();

            Position position = geopIpProvider.getPosition(ip);
            grid.mapObject(new StringWrapper(ip, position));
        }

        return grid;
    }

    @Test
    public void dummy()
    {
        System.out.println("GeoTest.dummy");
    }

    private void runIterations(int count) throws Exception
    {
        int gridSize = count / 10;
        runIterations(count, gridSize);
    }

    private void runIterations(int count, int gridSize) throws Exception
    {
        final Collection<String> candidateIps = locationGenerator.getRandomIPs(count);

        for (Class<? extends GeoMapper<Positionable>> implementation : implementations)
        {
            final GeoMapper<Positionable> grid = setUpGrid(gridSize, implementation);

            System.gc();

            long start = System.currentTimeMillis();

            for (String candidateIp : candidateIps)
            {
                Position position = geopIpProvider.getPosition(candidateIp);
                // Positionable randomFromNearest = grid.getRandomFromNearest(position.getLongitude(), position.getLatitude(), 3, validator);
                List<Positionable> result = grid.findNearest(position.getLongitude(), position.getLatitude(), 3, validator);

                boolean success = result.size() == 3; // randomFromNearest != null; // result.size() >= Math.min(CANDIDATES_SIZE, gridSize);
                if (!success)
                {
                    System.out.println("bomb");
                }
                assertTrue(success);
            }

            long elapsed = System.currentTimeMillis() - start;
            double rps = (double) count / ((double) elapsed / 1000d);

            System.out.println(String.format("%s: %d queries %d servers in %d ms (%.2f rps)", grid.getClass().getSimpleName(), count, gridSize, elapsed, rps));
        }
    }

    private static class StringWrapper implements Positionable
    {
        private final String string;
        private final Position position;

        private StringWrapper(String string, Position position)
        {
            this.string = string;
            this.position = position;
        }

        public Position getPosition()
        {
            return position;
        }

        public String getString()
        {
            return string;
        }
    }

    public static void main(String[] args) throws Exception
    {
        GeoTest test = new GeoTest();
        test.setUp();
        
        /*
Results with GeoGridV1:

Done with 1800510 custom IPs, evenly spread from areas [US-Fort Lauderdale, GB-London, SE-Stockholm, US-Los Angeles, GB-Manchester, US-New York, US-Houston, US-Atlanta, KR-Seoul, TW-Taipei, SE-Uppsala, FR-Paris, US-Seattle, JP-Tokyo, FR-Lyon, US-Philadelphia, DE-Frankfurt, DE-Berlin, US-Palo Alto]
GeoGridV1: 100 users 10 servers in 2159 ms (46,32 rps)
GeoGridV1: 1000 users 100 servers in 778 ms (1285,35 rps)
GeoGridV1: 10000 users 1000 servers in 480 ms (20833,33 rps)
GeoGridV1: 100000 users 10000 servers in 2853 ms (35050,82 rps)
GeoGridV1: 100000 users 1000 servers in 3069 ms (32583,90 rps)
GeoGridV1: 100000 users 100 servers in 77841 ms (1284,67 rps)
GeoGridV1: 100000 users 10 servers in 1184168 ms (84,45 rps)

Results with current GeoGridV2:

100 users 10 servers in 1 ms (100000,00 rps)
1000 users 100 servers in 38 ms (26315,79 rps)
10000 users 1000 servers in 4082 ms (2449,78 rps)
100000 users 10000 servers in 463877 ms (215,57 rps)
         */

        test.runIterations(10000, 10);
        test.runIterations(10000, 25);
        test.runIterations(10000, 50);
        test.runIterations(10000, 75);
        test.runIterations(10000, 100);
        test.runIterations(10000, 250);
        test.runIterations(10000, 500);
        test.runIterations(10000, 750);

        test.runIterations(10000, 1000);
        test.runIterations(10000, 2500);
        test.runIterations(10000, 5000);
        test.runIterations(10000, 7500);
        test.runIterations(10000, 10000);

        test.runIterations(10000, 15000);
        test.runIterations(10000, 20000);
        test.runIterations(10000, 25000);
        test.runIterations(10000, 30000);
    }
}
