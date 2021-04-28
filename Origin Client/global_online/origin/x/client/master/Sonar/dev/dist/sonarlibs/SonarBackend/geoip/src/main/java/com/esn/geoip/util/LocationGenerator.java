package com.esn.geoip.util;

import com.esn.geoip.GeoLiteCity;

import java.util.ArrayList;
import java.util.Collection;
import java.util.Random;

/**
 * User: ronnie
 * Date: 2011-06-20
 * Time: 09:13
 */
public class LocationGenerator
{
    private GeoLiteCity geopIpProvider;
    private final ArrayList<GeoLiteCity.Network> arrayList;

    private final static Random random = new Random(System.nanoTime());

    public LocationGenerator(GeoLiteCity geopIpProvider)
    {
        Collection<GeoLiteCity.Network> networks = geopIpProvider.getNetworks();

        arrayList = new ArrayList<GeoLiteCity.Network>(networks);
    }

    public String getRandomIP()
    {
        GeoLiteCity.Network network = arrayList.get(random.nextInt(arrayList.size()));

        long addr;

        if (network.getLength() == 0)
        {
            addr = network.getStart();
        } else
        {
            addr = network.getStart() + (random.nextLong() % network.getLength());
        }
        return IpUtils.longToIPV4(addr);
    }

    public Collection<String> getRandomIPs(int count)
    {
        Collection<String> retList = new ArrayList<String>(count);

        for (int index = 0; index < count; index++)
        {
            retList.add(getRandomIP());
        }

        return retList;
    }
}
