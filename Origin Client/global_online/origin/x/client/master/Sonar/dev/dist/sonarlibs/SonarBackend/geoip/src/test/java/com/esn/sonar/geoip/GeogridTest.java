package com.esn.sonar.geoip;

import com.esn.geoip.GeoGridV2;
import com.esn.geoip.GeoMapper;
import com.esn.geoip.Position;
import com.esn.geoip.Positionable;
import com.esn.geoip.util.Validator;
import org.junit.After;
import org.junit.Before;
import org.junit.Ignore;
import org.junit.Test;

import java.util.List;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;

public class GeogridTest
{
    GeoMapper<Positionable> geoGrid;
    private static final Validator<Positionable> validator = new Validator<Positionable>()
    {
        public boolean verify(Positionable item)
        {
            return true;
        }
    };
    //TODO: We don't test the Validator

    @Before
    public void start()
    {
        geoGrid = new GeoGridV2();
    }

    @After
    public void stop()
    {
    }

    @Test
    public void stupid()
    {
        List<Positionable> nearest = geoGrid.findNearest(-180.0f, -90.0f, 1, validator);
    }

    private static class TestObject implements Positionable, Position
    {

        final String address;
        final float longitude;
        final float latitude;
        private int value;

        private TestObject(int value, String address, float latitude, float longitude)
        {
            this.address = address;
            this.latitude = latitude;
            this.longitude = longitude;
            this.value = value;
        }

        public int getValue()
        {
            return value;
        }

        public Position getPosition()
        {
            return this;
        }

        public String getAddress()
        {
            return address;
        }

        public float getLatitude()
        {
            return latitude;
        }

        public float getLongitude()
        {
            return longitude;
        }
    }

    @Test
    public void findSpotOn()
    {
        geoGrid.mapObject(new TestObject(0, "", 31337.0f, 31337.0f));
        List<Positionable> nearest = geoGrid.findNearest(0.0f, 0.0f, 1, validator);
        assertEquals(nearest.size(), 1);
    }

    @Test
    public void maxResults()
    {
        geoGrid.mapObject(new TestObject(0, "", 31337.0f, 31337.0f));
        geoGrid.mapObject(new TestObject(0, "", 31337.0f, 31337.0f));
        geoGrid.mapObject(new TestObject(0, "", 31337.0f, 31337.0f));
        geoGrid.mapObject(new TestObject(0, "", 31337.0f, 31337.0f));
        geoGrid.mapObject(new TestObject(0, "", 31337.0f, 31337.0f));

        List<Positionable> nearest = geoGrid.findNearest(0.0f, 0.0f, 4, validator);
        assertEquals(nearest.size(), 4);
    }


    @Test
    public void itemCount()
    {
        TestObject to = new TestObject(1, "1", 0.0f, 0.0f);
        geoGrid.mapObject(to);
        assertEquals(geoGrid.size(), 1);
        geoGrid.unmapObject(to);
        assertEquals(geoGrid.size(), 0);
    }


    @Test
    @Ignore("This specific type of random order is not needed and/or is up for discussion")
    public void randomOrder()
    {
        TestObject ul = new TestObject(0, "", 127.0f, 127.0f);
        TestObject ur = new TestObject(1, "", 129.0f, 127.0f);
        TestObject bl = new TestObject(2, "", 127.0f, 129.0f);
        TestObject br = new TestObject(3, "", 129.0f, 129.0f);

        geoGrid.mapObject(ul);
        geoGrid.mapObject(ur);
        geoGrid.mapObject(bl);
        geoGrid.mapObject(br);

        int firstCount[] = new int[4];

        for (int index = 0; index < 400; index++)
        {
            List<Positionable> nearest = geoGrid.findNearest(128.0f, 128.0f, 4, validator);

            TestObject first = (TestObject) nearest.iterator().next();
            firstCount[first.getValue()]++;
        }


        for (int index = 0; index < 4; index++)
        {
            assertTrue(firstCount[index] >= 70 && firstCount[index] <= 130);
        }


    }


}
