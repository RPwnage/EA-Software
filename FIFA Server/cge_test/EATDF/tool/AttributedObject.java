

@SuppressWarnings("serial")
public abstract class AttributedObject extends ExtHashtable
{
    
    
    class ParseHelper extends AttributedObject
    {
        AttributedObject mInnerObject;
        ParseHelper(AttributedObject o)
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
    
    
    public boolean containsKey(Object key)
    {
        //our special string parsing keyword
        if (key.toString().equals("DOT"))
        {
            return true;
        }
        return get(key) != null;
    }
      
    
    public Object get(Object key)
    {
        //our special string parsing keyword
        if (key.toString().equals("DOT"))
        {
            return new ParseHelper(this);
        }
    
        //Check the super class to see if the key is defined
        Object superKey = super.get(key);
        if (superKey != null)
        {
            return superKey;
        } 
        
        return null;
    }    
    
    public AttributedObject() 
    {
        //One oddity of the ST library is that it will treat our object as 
        //"empty" or non existant if it doesn't have anything in it.  We 
        //put a default value in the map as a quick way of ensuring the map
        //is always size() > 1
        put("DEFAULTKEY_FOR_ATTRIBUTED_OBJECT", true);
    }
    
    public AttributedObject(DefineHelper other) 
    {
        put("DEFAULTKEY_FOR_ATTRIBUTED_OBJECT", true);        
        put(other);
    }
}
