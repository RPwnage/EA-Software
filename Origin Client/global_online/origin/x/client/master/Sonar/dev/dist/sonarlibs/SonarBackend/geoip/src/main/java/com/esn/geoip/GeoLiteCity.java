package com.esn.geoip;

import com.esn.geoip.util.IpUtils;
import com.esn.geoip.util.StringUtils;

import java.io.*;
import java.util.Collection;
import java.util.HashMap;
import java.util.LinkedList;

public class GeoLiteCity implements GeoipProvider
{
    private Position[] locations;
    private final BitTree<Network> bitTree = new BitTree<Network>();

    private static final String LOCATION_FILE = "/GeoLiteCity-Location.csv";
    private static final String BLOCKS_FILE = "/GeoLiteCity-Blocks.csv";
    private Collection<Network> networkList = new LinkedList<Network>();

    public Collection<Network> getNetworks()
    {
        return networkList;
    }


    public static class Network
    {
        private final long start;
        private final int length;
        private final Position position;

        public Network(long start, int length, Position position)
        {
            this.start = start;
            this.length = length;
            this.position = position;
        }

        public long getStart()
        {
            return start;
        }

        public int getLength()
        {
            return length;
        }

        public Position getPosition()
        {
            return position;
        }

        @Override
        public String toString()
        {
            return String.format("Network %s-%s @ %s", IpUtils.longToIPV4(start), IpUtils.longToIPV4(start + length), String.valueOf(position));
        }
    }

    private final Position nullPosition = new GeoLiteLocation(0.0f, 0.0f, "Null");


    public String getLicense()
    {
        return "This product includes GeoLite data created by MaxMind, available from http://maxmind.com/";
    }

    public Position getPosition(String address)
    {
        String[] split = StringUtils.tokenize(address, '.');

        if (split == null || split.length != 4)
        {
            return nullPosition;
        }

        long addr = 0;

        addr |= Integer.parseInt(split[0]);
        addr <<= 8;
        addr |= Integer.parseInt(split[1]);
        addr <<= 8;
        addr |= Integer.parseInt(split[2]);
        addr <<= 8;
        addr |= Integer.parseInt(split[3]);

        Network network = bitTree.find(addr);

        if (network == null)
        {
            return nullPosition;
        }

        return network.position;
    }

    private void loadBlocks(InputStream is) throws IOException
    {
        BufferedReader in = new BufferedReader(new InputStreamReader(is));

        try
        {
            in.readLine();
            in.readLine();

            long count = 0;


            System.err.printf("Parsing blocks");

            while (true)
            {
                String line = in.readLine();

                if (line == null)
                {
                    break;
                }

                String[] tokens = StringUtils.tokenize(line, ',');

                if (tokens == null || tokens.length != 3)
                {
                    continue;
                }

                long start = Long.parseLong(tokens[0].substring(1, tokens[0].length() - 1));
                long end = Long.parseLong(tokens[1].substring(1, tokens[1].length() - 1));
                int locId = Integer.parseInt(tokens[2].substring(1, tokens[2].length() - 1));

                Position location = locations[locId];

                Network net = new Network(start, (int) (end - start), location);

                networkList.add(net);

                bitTree.add(start, end, net);

                count++;

                if (count % 100000 == 0)
                {
                    System.err.printf(".");
                }

            }

            System.err.printf("\n");
        } finally
        {
            try
            {
                in.close();
            } catch (IOException e)
            {
                // ignore, at least we tried
            }
        }
    }

    private static class Location
    {
        private final int id;
        private final float latitude;
        private final float longitude;
        private final String address;

        private Location(int id, float latitude, float longitude, String address)
        {
            this.id = id;
            this.latitude = latitude;
            this.longitude = longitude;
            this.address = address;
        }
    }

    private static class GeoLiteLocation implements Position
    {
        private final String address;
        private final float longitude;
        private final float latitude;

        public GeoLiteLocation(float longitude, float latitude, String address)
        {
            if (longitude > 180.0f ||
                    longitude < -180.0f ||
                    latitude > 90.0f ||
                    latitude < -90.0f)
            {
                throw new IllegalArgumentException("Longitude and latitude must be in range (-180, +180), (-90, +90)");
            }
            this.address = address;
            this.longitude = longitude;
            this.latitude = latitude;
        }

        public float getLongitude()
        {
            return longitude;
        }

        public float getLatitude()
        {
            return latitude;
        }


        public String getAddress()
        {
            return this.address;
        }

        @Override
        public String toString()
        {
            return String.format("%s %f, %f", getAddress(), getLongitude(), getLatitude());
        }

        @Override
        public boolean equals(Object o)
        {
            if (this == o) return true;
            if (!(o instanceof GeoLiteLocation)) return false;

            GeoLiteLocation that = (GeoLiteLocation) o;

            if (Float.compare(that.latitude, latitude) != 0) return false;
            if (Float.compare(that.longitude, longitude) != 0) return false;
            if (address != null ? !address.equals(that.address) : that.address != null) return false;

            return true;
        }

        @Override
        public int hashCode()
        {
            int result = address != null ? address.hashCode() : 0;
            result = 31 * result + (longitude != +0.0f ? Float.floatToIntBits(longitude) : 0);
            result = 31 * result + (latitude != +0.0f ? Float.floatToIntBits(latitude) : 0);
            return result;
        }
    }

    private void loadLocations(InputStream is) throws IOException
    {
        BufferedReader in = new BufferedReader(new InputStreamReader(is), 1024 * 1024 * 10);


        try
        {
            in.readLine();
            in.readLine();

            HashMap<Integer, Location> locationMap = new HashMap<Integer, Location>();

            int maxId = -1;


            while (true)
            {
                String line = in.readLine();

                if (line == null)
                {
                    break;
                }

                String[] tokens = StringUtils.tokenize(line, ',');

                if (tokens == null || tokens.length != 9)
                {
                    continue;
                }

                int id = Integer.parseInt(tokens[0]);

                if (id > maxId)
                {
                    maxId = id;
                }

                StringBuilder address = new StringBuilder();

                address.append(tokens[1].substring(1, tokens[1].length() - 1));
                address.append("-");
                address.append(tokens[3].substring(1, tokens[3].length() - 1));

                Location loc = new Location(id, Float.parseFloat(tokens[5]), Float.parseFloat(tokens[6]), address.toString());
                locationMap.put(id, loc);
            }

            System.err.printf("Locations: %d\n", maxId);

            this.locations = new Position[maxId + 1];

            for (Location location : locationMap.values())
            {
                Position pos = new GeoLiteLocation(location.longitude, location.latitude, location.address);
                locations[location.id] = pos;
            }
        } finally
        {
            try
            {
                in.close();
            } catch (IOException e)
            {
                // ignore, at least we tried
            }
        }
    }


    public File[] getFiles(String basePath)
    {
        File[] files = new File[2];

        files[0] = new File(basePath, LOCATION_FILE);
        files[1] = new File(basePath, BLOCKS_FILE);
        return files;
    }

    public void loadFromJar(Class cls, String basePath) throws IOException
    {
        InputStream locationStream = cls.getResourceAsStream(basePath + LOCATION_FILE);
        InputStream blockStream = cls.getResourceAsStream(basePath + BLOCKS_FILE);

        load(locationStream, blockStream);
    }

    public void loadFromDisk(String basePath) throws IOException
    {
        InputStream locationStream = new FileInputStream(new File(basePath, LOCATION_FILE));
        InputStream blockStream = new FileInputStream(new File(basePath, BLOCKS_FILE));

        load(locationStream, blockStream);
    }

    private void load(InputStream locationStream, InputStream blockStream) throws IOException
    {
        long start = System.nanoTime();

        System.err.printf("Loading MaxMind GeoData database...\n");
        loadLocations(new BufferedInputStream(locationStream));
        loadBlocks(new BufferedInputStream(blockStream));
        System.err.printf("Blocks: %d\n", bitTree.size());

        long elapsed = System.nanoTime() - start;

        System.err.printf("Done! Elapsed: %d ms\n", elapsed / 1000000);
    }

}
