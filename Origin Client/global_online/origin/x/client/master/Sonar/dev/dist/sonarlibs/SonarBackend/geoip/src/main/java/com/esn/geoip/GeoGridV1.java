package com.esn.geoip;

import com.esn.geoip.util.SearchProcessor;

import java.util.HashSet;
import java.util.Random;
import java.util.Set;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.concurrent.locks.ReentrantReadWriteLock;

public class GeoGridV1 extends GeoMapper<Positionable>
{
    private static Random random = new Random();

    private int lonRes;
    private int latRes;

    private float latS;
    private float lonS;

    private AtomicInteger itemCount = new AtomicInteger(0);
    private ReentrantReadWriteLock rwLock = new ReentrantReadWriteLock();
    private int maxScanWidth;

    public GeoGridV1()
    {
        this(256, 256, 256 / 8);
    }

    public GeoGridV1(int longRes, int latRes, int maxScanWidth)
    {
        this.maxScanWidth = maxScanWidth;

        this.lonRes = longRes;
        this.latRes = latRes;

        this.lonS = this.lonRes / 360.0f;
        this.latS = this.latRes / 180.0f;

        geoGrid = new Dot[longRes + 1][latRes + 1];

        for (int x = 0; x < longRes + 1; x++)
        {
            for (int y = 0; y < latRes + 1; y++)
            {
                geoGrid[x][y] = new Dot();
            }
        }

    }

    public int size()
    {
        return itemCount.get();
    }

    public void mapObject(Positionable item)
    {
        getDot(longitudeToIndex(item.getPosition().getLongitude()), latitudeToIndex(item.getPosition().getLatitude())).add(item);
    }

    public void unmapObject(Positionable item)
    {
        getDot(longitudeToIndex(item.getPosition().getLongitude()), latitudeToIndex(item.getPosition().getLatitude())).remove(item);
    }

    public void processNearest(Position position, SearchProcessor<Positionable> processor)
    {
        processNearest(processor, longitudeToIndex(position.getLongitude()), latitudeToIndex(position.getLatitude()));
    }

    private class Dot
    {
        private Set<Positionable> set = new HashSet<Positionable>(0);

        public void add(Positionable value)
        {
            rwLock.writeLock().lock();
            try
            {
                if (set.add(value))
                {
                    itemCount.incrementAndGet();
                }
            } finally
            {
                rwLock.writeLock().unlock();
            }
        }

        public Positionable remove(Positionable value)
        {
            rwLock.writeLock().lock();
            try
            {
                if (set.remove(value))
                {
                    itemCount.decrementAndGet();
                    return value;
                }

            } finally
            {
                rwLock.writeLock().unlock();
            }

            return null;
        }

        public boolean processItems(SearchProcessor<Positionable> processor)
        {
            rwLock.readLock().lock();

            try
            {
                for (Positionable item : set)
                {
                    if (!processor.execute(item)) return false;
                }
                return true;
            } finally
            {
                rwLock.readLock().unlock();
            }
        }


    }

    private Dot geoGrid[][];

    int sx[] = new int[]{-1, 1, 1, -1};
    int sy[] = new int[]{-1, -1, 1, 1};
    int dx[] = new int[]{1, 0, -1, 0};
    int dy[] = new int[]{0, 1, 0, -1};


    private int latitudeToIndex(float latitude)
    {
        /* -90 + +90 */
        latitude += 90.0f;
        float fi = latS * latitude;
        int i = Math.round(fi);

        return i;
    }

    private int longitudeToIndex(float longitude)
    {
        /* -180 + +180 */
        longitude += 180.0f;
        float fi = lonS * longitude;
        int i = Math.round(fi);

        return i;
    }

    private Dot getDot(int x, int y)
    {
        if (x > lonRes)
        {
            x %= lonRes;
        } else if (x < 0)
        {
            x = lonRes + (x % lonRes);
        }

        if (y > latRes)
        {
            y %= latRes;
        } else if (y < 0)
        {
            y = latRes + (y % latRes);
        }

        return geoGrid[x][y];
    }

    private boolean processDotItems(int x, int y, SearchProcessor<Positionable> processor)
    {

        if (x % lonRes == 0)
        {
            if (y % latRes == 0)
            {
                if (!getDot(0, 0).processItems(processor)) return false;
                if (!getDot(0, latRes).processItems(processor)) return false;
                if (!getDot(lonRes, 0).processItems(processor)) return false;
                if (!getDot(lonRes, latRes).processItems(processor)) return false;
            } else
            {
                if (!getDot(0, y).processItems(processor)) return false;
                if (!getDot(latRes, y).processItems(processor)) return false;
            }
        } else if (y % latRes == 0)
        {
            if (!getDot(x, 0).processItems(processor)) return false;
            if (!getDot(x, latRes).processItems(processor)) return false;
        } else
        {
            if (!getDot(x, y).processItems(processor)) return false;
        }
        return true;
    }

    //TODO: Make more complete tests"

    private void processNearest(SearchProcessor<Positionable> processor, int xc, int yc)
    {
        int left = xc;
        int right = xc;
        int top = yc;
        int bottom = yc;

        if (!processDotItems(xc, yc, processor)) return;

        while (true)
        {
            left--;
            right++;
            top--;
            bottom++;

            int w = right - left;
            int h = bottom - top;

            if (w > maxScanWidth)
            {
                return;
            }


            if (w > lonRes)
            {
                return;
            }

            if (h > latRes)
            {
                return;
            }

            int c = w * 4;
            int d = -1 + (random.nextInt(2) * 2);
            int r = random.nextInt(c);
            int hw = w / 2;

            for (int index = 0; index < c; index++)
            {
                int p = (r + (index * d)) % c;

                if (p < 0)
                {
                    p = c + p;
                }

                int s = p / w;
                int o = p % w;

                int x = xc + (sx[s] * (hw)) + (dx[s] * o);
                int y = yc + (sy[s] * (hw)) + (dy[s] * o);

                if (!processDotItems(x, y, processor)) return;
            }
        }
    }
}
