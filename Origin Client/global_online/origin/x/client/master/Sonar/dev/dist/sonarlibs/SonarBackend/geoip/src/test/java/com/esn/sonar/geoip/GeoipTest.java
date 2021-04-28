package com.esn.sonar.geoip;

import com.esn.geoip.GeoGridV2;
import com.esn.geoip.GeoLiteCity;
import com.esn.geoip.GeoipProvider;
import com.esn.geoip.Position;
import com.esn.geoip.Positionable;
import com.esn.geoip.util.Validator;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;

import java.io.IOException;
import java.util.List;

import static org.junit.Assert.*;

public class GeoipTest
{
    private static final Validator<Positionable> validator = new Validator<Positionable>()
    {
        public boolean verify(Positionable item)
        {
            return true;
        }
    };
    GeoipProvider provider;
    GeoGridV2 geoGrid = new GeoGridV2();

    private class TestItem implements Positionable
    {
        private String address;
        private Position position;

        public TestItem(Position pos, String address)
        {
            this.position = pos;
            this.address = address;

        }

        public Position getPosition()
        {
            return position;
        }

        @Override
        public String toString()
        {
            return address;
        }

        @Override
        public boolean equals(Object obj)
        {
            if (obj instanceof String)
            {
                return address.equals(((String) obj));
            } else if (obj instanceof TestItem)
            {
                return address.equals(((TestItem) obj).address);
            }
            return false;
        }
    }

    @Before
    public void start() throws IOException
    {
        if (provider == null)
        {
            provider = new GeoLiteCity();
            provider.loadFromJar(this.getClass(), "/geoip");
        }
    }

    @After
    public void stop()
    {
    }

    @Test
    public void lookup()
    {
        Position pos;

        long tsStart = System.currentTimeMillis();

        pos = provider.getPosition("130.244.1.1");
        assertNotNull(pos);
        assertEquals(pos.getAddress(), "SE-");

        pos = provider.getPosition("195.198.215.210");
        assertNotNull(pos);
        assertEquals(pos.getAddress(), "SE-Uppsala");

        pos = provider.getPosition("9.31.4.1");
        assertNotNull(pos);
        assertEquals(pos.getAddress(), "US-White Plains");

        pos = provider.getPosition("11.31.4.255");
        assertNotNull(pos);
        assertEquals(pos.getAddress(), "US-Columbus");

        pos = provider.getPosition("213.115.179.10");
        assertNotNull(pos);
        assertEquals(pos.getAddress(), "SE-");

        pos = provider.getPosition("11.0.0.0");
        assertNotNull(pos);
        assertEquals(pos.getAddress(), "US-Columbus");
    }

    private static boolean findByAddress(List<Positionable> list, String address)
    {
        for (Positionable item : list)
        {
            if (item.toString().equals(address))
            {
                return true;
            }
        }

        return false;
    }

    @Test
    //@Ignore("Ignored because the random generation has issues in GeoGrid implementation. We need to figure this out")
    public void gridLookup()
    {
        Position pos;
        pos = provider.getPosition("9.31.4.1");
        geoGrid.mapObject(new TestItem(pos, "US1"));
        pos = provider.getPosition("11.31.4.255");
        geoGrid.mapObject(new TestItem(pos, "US2"));

        pos = provider.getPosition("130.244.1.1");
        geoGrid.mapObject(new TestItem(pos, "SE1"));
        pos = provider.getPosition("195.198.215.210");
        geoGrid.mapObject(new TestItem(pos, "SE2"));

        pos = provider.getPosition("213.115.179.10");

        List<Positionable> nearest = geoGrid.findNearest(pos.getLongitude(), pos.getLatitude(), 1, validator);
        assertEquals(1, nearest.size());
        assertTrue(findByAddress(nearest, "SE1"));


        nearest = geoGrid.findNearest(pos.getLongitude(), pos.getLatitude(), 2, validator);
        assertEquals(2, nearest.size());


        assertTrue(findByAddress(nearest, "SE1"));
        assertTrue(findByAddress(nearest, "SE2"));

        pos = provider.getPosition("11.0.0.0");

        nearest = geoGrid.findNearest(pos.getLongitude(), pos.getLatitude(), 1, validator);
        assertEquals(1, nearest.size());
        assertEquals(nearest.iterator().next(), "US2");

        nearest = geoGrid.findNearest(pos.getLongitude(), pos.getLatitude(), 2, validator);
        assertEquals(2, nearest.size());

        assertTrue(findByAddress(nearest, "US1"));
        assertTrue(findByAddress(nearest, "US2"));

    }

}
