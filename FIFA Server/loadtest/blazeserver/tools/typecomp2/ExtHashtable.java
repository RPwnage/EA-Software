import java.util.*;

public class ExtHashtable extends HashMap<Object, Object>
{
    
    /**
     * 
     */
    private static final long serialVersionUID = 1L;

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

    public <T> void putInList(String listName, T obj)
    {
        List<T> list = getT(listName);
        if (list == null)
        {
            list = new ArrayList<T>();
            put(listName, list);
        }
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
   
    public <T> T getT(Object key)
    {
        return (T) get(key);
    }
    
    public <T> T getT(Object key, T def)
    {
        T result = (T)getT(key);
        return result != null ? result : def;
    }
    
    public String getString(Object key)
    {
        return (String) get(key);
    }
    
    
    public ExtHashtable() {}
    public ExtHashtable(ExtHashtable other) 
    {
        put(other);
    }
}
