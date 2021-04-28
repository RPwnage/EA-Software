import java.lang.reflect.*;

public class MapFunctor extends ExtHashtable
{
    
    /**
     * 
     */
    private static final long serialVersionUID = 1L;
    
    
    public boolean containsKey(Object key)
    {
    	//We always return true because we'll handle all objects.
       return true;
    }
      
    
    public Object get(Object key)
    {        
        try
        {
        	return mMethod.invoke(mObject, key);
        }
        catch (Exception e) {}  
        
        return null;
    }    
    
    Object mObject;
    Method mMethod;
    
    public MapFunctor(Object obj, String methodName) throws NoSuchMethodException
    {
        //One oddity of the ST library is that it will treat our object as 
        //"empty" or non existant if it doesn't have anything in it.  We 
        //put a default value in the map as a quick way of ensuring the map
        //is always size() > 1
        put("DEFAULTKEY_FOR_MAP_FUNCTOR", true);
        
        mObject = obj;
        mMethod = mObject.getClass().getMethod(methodName, String.class);
    }
    
}
