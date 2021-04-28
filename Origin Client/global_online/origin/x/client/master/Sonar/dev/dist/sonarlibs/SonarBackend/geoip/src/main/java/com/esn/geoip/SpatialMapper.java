package com.esn.geoip;

import com.esn.geoip.util.SearchProcessor;
import com.infomatiq.jsi.Point;
import com.infomatiq.jsi.Rectangle;
import com.infomatiq.jsi.SpatialIndex;
import com.infomatiq.jsi.rtree.RTree;
import gnu.trove.TIntProcedure;

import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;

/**
 * User: ronnie
 * Date: 2011-06-22
 * Time: 15:42
 */
public class SpatialMapper extends GeoMapper<Positionable>
{
    private Map<Integer, Positionable> map = new ConcurrentHashMap<Integer, Positionable>();

    private final SpatialIndex index;

    public SpatialMapper()
    {
        index = new RTree();
        index.init(null);
    }

    public int size()
    {
        return index.size();
    }

    public void mapObject(Positionable item)
    {
        Position position = item.getPosition();
        int id = item.hashCode();
        map.put(id, item);
        index.add(createRectangle(position), id);
    }

    public void unmapObject(Positionable item)
    {
        Position position = item.getPosition();
        int id = item.hashCode();
        map.remove(id);
        index.delete(createRectangle(position), id);
    }

    @Override
    public void processNearest(Position position, final SearchProcessor<Positionable> searchProcessor)
    {
        index.nearestN(createPoint(position.getLongitude(), position.getLatitude()), new TIntProcedure()
        {
            public boolean execute(int i)
            {
                Positionable object = map.get(i);
                if (object == null)
                {
                    // should not happen..
                    System.err.println("Warning: unable to lookup object with id " + i + " (map size=" + map.size() + ")");
                    return false;
                }
                return searchProcessor.execute(object);
            }
        }, Integer.MAX_VALUE, Float.POSITIVE_INFINITY);
    }

    private Rectangle createRectangle(Position position)
    {
        return createRectangle(position.getLongitude(), position.getLatitude());
    }

    private Rectangle createRectangle(float x, float y)
    {
        float xRound = Math.round(x);
        float yRound = Math.round(y);

        float size = 1.0f;

        float x1 = xRound - size;
        float y1 = yRound - size;
        float x2 = xRound + size;
        float y2 = yRound + size;

        return new Rectangle(x1, y1, x2, y2);
    }

    private Point createPoint(float longitude, float latitude)
    {
        return new Point(Math.round(longitude), Math.round(latitude));
    }
}
