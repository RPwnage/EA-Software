import java.util.Comparator;
import java.lang.String;

class HashTokenMemberComparator implements Comparator<HashToken>
{
    public HashTokenMemberComparator(String sortAttribute)
    {
        mSortAttribute = sortAttribute;
    }
    
    public int compare(HashToken arg0, HashToken arg1)
    {
        if (arg0 == null && arg1 == null)
            return 0;
        else if (arg0 == null)
            return 1;
        else if (arg1 == null)
            return -1;
            
        String val0 = (String)arg0.getT(mSortAttribute);
        String val1 = (String)arg1.getT(mSortAttribute);
        
        if (val0 == null && val1 == null)
            return 0;
        if (val0 == null)
            return 1;
        if (val1 == null)
            return -1;        
            
        return val0.compareToIgnoreCase(val1);
    }
    
    private String mSortAttribute;
}
