import java.util.*;

public class DefineHelper extends AttributedObject
{   
    public class AddHelper extends ExtHashtable
    {   
        /**
         * 
         */
        private static final long serialVersionUID = 1L;
        
        
        public boolean containsKey(Object key)
        {
            String actualKey = key.toString();
            boolean result = super.containsKey(actualKey);
            if (!result)
            {
                mParentTable.put(actualKey, true);
            }
            
            return result;
        }
          
        
        public Object get(Object key)
        {
            String actualKey = key.toString();
            Object result = super.get(actualKey);
            
            if (result == null)
            {
                mParentTable.put(actualKey, true);
            }
            
            return "";
        }    
        
        public AddHelper(ExtHashtable parent) 
        {
            super.setParentTable(parent);
        }
    }

    
    /**
     * 
     */
    private static final long serialVersionUID = 1L;     
    
    AddHelper mAddHelper;
    public AddHelper getAdd() { return mAddHelper; } 
    
    public void getClear() { this.clear(); }

    public DefineHelper() 
    {
        mAddHelper = new AddHelper(this);
    }

}
