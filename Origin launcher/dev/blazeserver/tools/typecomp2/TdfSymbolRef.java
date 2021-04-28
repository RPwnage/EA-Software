import java.util.*;

public class TdfSymbolRef
{
    TdfSymbol mSymbol;
    public TdfSymbol getSymbol() { return mSymbol; }
    public TdfSymbol getActualSymbol() { return mSymbol.getActualSymbol(); }

    public List<String> getSymbolName() { return mSymbol.getFullNameList(); } 
    
    public List<String> getScopedSymbolName() { return mScope.getScopedName(mSymbol, true); }
    public List<String> getSymbolScopeName() { return mScope.getScopedName(mSymbol, false); }
    public List<String> getActualSymbolScopeName() { return mScope.getScopedName(getActualSymbol(), false); }

    public ExtHashtable getScopedAttributes() { return getScopedAttributesForSymbol(getSymbol()); }
    public ExtHashtable getActualScopedAttributes() { return getScopedAttributesForSymbol(getSymbol().getActualSymbol()); }
    
    ExtHashtable getScopedAttributesForSymbol(TdfSymbol symbol) 
    {
        if (symbol == null)
            return null;

        ExtHashtable result = new ExtHashtable();           
        for (Map.Entry<Object, Object> e : symbol.entrySet())
        {
            if (e.getValue().getClass() == TdfSymbol.class)
            {
                result.put(e.getKey(), mScope.getScopedName((TdfSymbol)e.getValue(), true));
            }
            else
            {
                result.put(e.getKey(), e.getValue());
            }
        }

        return result;
    }


    TdfSymbol mScope;
    public TdfSymbolRef(TdfSymbol scope, TdfSymbol symbol) throws NullPointerException
    {
        mScope = scope;
        mSymbol = symbol;
        if (symbol == null)
        	throw new java.lang.NullPointerException();
    }

 
    public String toString()
    {
        String result = null;
        
        for (String s : getSymbolName())
        {
            if (result == null)
            {
               result = s;
            }
            else
            {
               result += ("::" + s);
            }
        }

	return result;
    }
}

