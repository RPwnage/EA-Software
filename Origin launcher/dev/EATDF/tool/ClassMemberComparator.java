import java.util.Comparator;
import java.lang.Long;

class ClassMemberComparator implements Comparator<HashToken>
{
    public ClassMemberComparator(String sortAttribute)
    {
        mSortAttribute = sortAttribute;
    }
    
    public int compare(HashToken arg0, HashToken arg1)
    {
        return ((Long)(arg0.get(mSortAttribute))).compareTo((Long)(arg1.get(mSortAttribute)));
    }
    
    private String mSortAttribute;
}
