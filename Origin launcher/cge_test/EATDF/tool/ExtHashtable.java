import java.util.*;

@SuppressWarnings("serial")
public class ExtHashtable extends HashMap<Object, Object>
{
    
    class ParseHelper extends ExtHashtable
    {
        ExtHashtable mInnerObject;
        ParseHelper(ExtHashtable o)
        {
            super();
            mInnerObject = o;
        }

        public boolean containsKey(Object key)
        {
            return mInnerObject.containsKey(key);
        }

        public Object get(Object key)
        {
            Object o = mInnerObject.get(key);
            if (o != null)
            {
                return java.util.Arrays.asList(o.toString().split("\\."));
            }

            return null;
        }

    }
 
    ExtHashtable mParentTable;
    public ExtHashtable getParentTable() { return mParentTable; }
    public ExtHashtable setParentTable(ExtHashtable parent)
    {
        mParentTable = parent;
        return this;
    }

    public ExtHashtable put(Object key, Object value) 
    {
        if (value != null)
        {
            super.put(key, value);
        }
        
        return this;
    }
    
    public ExtHashtable putIfNotExists(Object key, Object value) 
    {
        if (!super.containsKey(key))
        {
            super.put(key, value);
        }
        return this;
    }

    public ExtHashtable put(ExtHashtable m)
    {
        if (m != null)
        {
            Set<Map.Entry<Object, Object>> set = m.entrySet();        
            for (Map.Entry<Object, Object> e : set)
            {
                Object k = e.getKey();
                //Our parent doesn't matter here
                if (!super.containsKey(k))
                {
                    put(k, e.getValue());
                }
            }
        }

        return this;
    }

    @SuppressWarnings("unchecked")
    public void putInList(String listName, Object obj)
    {
        ArrayList<Object> list = null;
        Object item = get(listName);
        if (item == null || !(item instanceof ArrayList<?>))
        {
            list = new ArrayList<Object>();
            put(listName, list);
        }
        else
            list = (ArrayList<Object>) item;
        list.add(obj);
    }
    
    public boolean containsKey(Object key)
    {
        //our special string parsing keyword
        // "DOT" allows a dotted string to be iterated by string templates
        // For example, if "foo.bar" is "A.B.C", "foo.DOT.bar" will be a group of ["A", "B", "C"]
        if (key.toString().equals("DOT"))
        {
            return true;
        }

        return (super.containsKey(key) || ((mParentTable != null) && mParentTable.containsKey(key))); 
    }
    
    public Object get(Object key)
    {
        //our special string parsing keyword
        if (key.toString().equals("DOT"))
        {
            return new ParseHelper(this);
        }

        Object val = super.get(key);
        if (val == null && mParentTable != null)
        {
            val = mParentTable.get(key);
        }
        
        return val;
    }
    
    public String getString(Object key)
    {
        Object obj = this.get(key);
        return (obj != null) ? obj.toString() : null;
    }
    
    
    public ExtHashtable() {}
    public ExtHashtable(ExtHashtable other) 
    {
        put(other);
    }
}
