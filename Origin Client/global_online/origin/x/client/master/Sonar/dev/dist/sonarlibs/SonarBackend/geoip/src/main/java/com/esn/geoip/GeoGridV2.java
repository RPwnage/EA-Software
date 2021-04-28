package com.esn.geoip;

import com.esn.geoip.util.SearchProcessor;

import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;
import java.util.Set;
import java.util.concurrent.ConcurrentSkipListSet;

public class GeoGridV2 extends GeoMapper<Positionable>
{
    private static final Comparator<ItemPosition> comparator = new ItemPositionComparator();

    private Set<Positionable> itemSet = new ConcurrentSkipListSet<Positionable>(new PositionSorter());

    public void mapObject(Positionable item)
    {
        //TODO: Check return value? Maybe LoT? (Log or Throw)
        itemSet.add(item);
    }

    public void unmapObject(Positionable item)
    {
        //TODO: Check return value? Maybe LoT? (Log or Throw)
        itemSet.remove(item);
    }

    public void processNearest(Position position, SearchProcessor<Positionable> processor)
    {
        processNearest(processor, position.getLongitude(), position.getLatitude());
    }

    private void processNearest(SearchProcessor<Positionable> processor, float longitude, float latitude)
    {
        List<ItemPosition> deltaItems = new ArrayList<ItemPosition>();

        for (Positionable item : itemSet)
        {
            deltaItems.add(new ItemPosition(
                    item.getPosition().getLongitude() - longitude,
                    item.getPosition().getLatitude() - latitude, item));
        }

        Collections.sort(deltaItems, comparator);

        for (ItemPosition deltaItem : deltaItems)
        {
            if (!processor.execute((Positionable) deltaItem.getItem())) return;
        }
    }

    public int size()
    {
        return this.itemSet.size();
    }

    private static class ItemPosition implements Positionable, Position
    {
        private final Object item;
        private float longitude;
        private float latitude;

        private ItemPosition(float longitude, float latitude, Object item)
        {
            this.item = item;
            this.longitude = longitude;
            this.latitude = latitude;
        }

        public Position getPosition()
        {
            return this;
        }

        public Object getItem()
        {
            return item;
        }

        public String getAddress()
        {
            return "";
        }

        public float getLongitude()
        {
            return longitude;
        }

        public float getLatitude()
        {
            return latitude;
        }
    }

    private static class ItemPositionComparator implements Comparator<ItemPosition>
    {
        public int compare(ItemPosition o1, ItemPosition o2)
        {
            /*
            double l1 = Math.sqrt(((VertexItem) o1).getX() * ((VertexItem) o1).getY());
            double l2 = Math.sqrt(((VertexItem) o2).getX() * ((VertexItem) o2).getY());
            */

            /*
            TODO: Is this a real assumption? Can we really cheat Pythagoras this easily?
             */

            double l1 = Math.abs(o1.getLongitude()) + Math.abs(o1.getLatitude());
            double l2 = Math.abs(o2.getLongitude()) + Math.abs(o2.getLatitude());

            if (l1 > l2)
            {
                return 1;
            } else if (l1 < l2)
            {
                return -1;
            }

            return 0;
        }
    }

    private static class PositionSorter implements Comparator<Positionable>
    {
        public int compare(Positionable o1, Positionable o2)
        {
            return o1.hashCode() - o2.hashCode();
        }
    }
}
