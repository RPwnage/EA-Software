import java.util.Comparator;

class CustomAttributeMemberInstanceComparator implements Comparator<HashToken>
{
    public CustomAttributeMemberInstanceComparator(String sortAttribute)
    {
        mSortAttribute = sortAttribute;
    }
    
    public int compare(HashToken arg0, HashToken arg1)
    {        
        return ((arg0.getString(mSortAttribute))).compareTo((arg1.getString(mSortAttribute))); 
    }
    
    private String mSortAttribute;
}
