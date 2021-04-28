import java.util.Comparator;

class CustomAttributeMemberInstanceComparator implements Comparator<HashToken>
{
    public CustomAttributeMemberInstanceComparator(String sortAttribute)
    {
        mSortAttribute = sortAttribute;
    }
    
    public int compare(HashToken arg0, HashToken arg1)
    {        
        return ((String)(arg0.getT(mSortAttribute))).compareTo((String) (arg1.getT(mSortAttribute))); 
    }
    
    private String mSortAttribute;
}
