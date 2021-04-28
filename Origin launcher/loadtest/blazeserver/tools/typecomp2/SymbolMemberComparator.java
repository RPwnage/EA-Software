import java.util.Comparator;
import java.lang.String;

class SymbolMemberComparator implements Comparator<TdfSymbolRef>
{
    public SymbolMemberComparator(String sortAttribute)
    {
        mSortAttribute = sortAttribute;
    }
    
    public int compare(TdfSymbolRef arg0, TdfSymbolRef arg1)
    {
        if (arg0 == null && arg1 == null)
            return 0;
        else if (arg0 == null)
            return 1;
        else if (arg1 == null)
            return -1;
    
        String val0 = (String)arg0.getSymbol().getT(mSortAttribute);
        String val1 = (String)arg1.getSymbol().getT(mSortAttribute);
        
        if (val0 == null && val1 == null)
            return 0;
        else if (val0 == null)
            return 1;
        if (val1 == null)
            return -1;        
        
        return val0.compareToIgnoreCase(val1);
    }
    
    private String mSortAttribute;
}
