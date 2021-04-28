import java.lang.reflect.*;

public class AttributedObject extends ExtHashtable
{
    
    /**
     * 
     */
    private static final long serialVersionUID = 1L;
    
    
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
		
		//First see if this is a method on our class
        if (super.containsKey(key))
        {
            return true;
        }
        try
        {
            if (this.getClass().getMethod("get" + key.toString()) != null)
                return true;
        }
        catch (NoSuchMethodException e) {}

        try
        {       
            if (this.getClass().getMethod("is" + key.toString()) != null)
                return true;
        }
        catch (NoSuchMethodException e) {}
        
        return false;
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

        try
        {
            Method m = null;
            try
            {
                m = this.getClass().getMethod("get" + key.toString());
            }
            catch (NoSuchMethodException e)
            {
                m = this.getClass().getMethod("is" + key.toString());
            }
            if (m != null)
            {
                return m.invoke(this);
            }
        }
        catch (Exception e) {}  
        
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
