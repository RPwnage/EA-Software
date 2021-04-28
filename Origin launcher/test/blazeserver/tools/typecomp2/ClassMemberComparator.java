import java.util.Comparator;

class ClassMemberComparator implements Comparator<HashToken>
{
    public ClassMemberComparator(String sortAttribute)
    {
        mSortAttribute = sortAttribute;
    }
    
    public int compare(HashToken arg0, HashToken arg1)
    {
        return (Integer) (arg0.getT(mSortAttribute)) - (Integer) (arg1.getT(mSortAttribute));
    }
    
    private String mSortAttribute;
}
