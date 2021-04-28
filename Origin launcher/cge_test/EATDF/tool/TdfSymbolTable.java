import java.util.*;

public class TdfSymbolTable
{
    Map<TdfSymbol.SymbolType, HashMap<String,TdfSymbol>> mNameTable = new HashMap<TdfSymbol.SymbolType, HashMap<String,TdfSymbol>>();

    public void addSymbol(TdfSymbol symbol, TdfSymbol.SymbolType type)
    {
        HashMap<String, TdfSymbol> map = mNameTable.get(type);
        if (map == null)
        {
            map = new HashMap<String, TdfSymbol>();
            mNameTable.put(type, map);
        }
        
        map.put(symbol.getFullName(), symbol);
    }
    
    public TdfSymbol getSymbol(String name, TdfSymbol.SymbolType type) 
    {
        TdfSymbol symbol = null;
        HashMap<String, TdfSymbol> map = mNameTable.get(type);
        if (map != null)
        {
            symbol = map.get(name);
        }
        return symbol;
    }

    TdfSymbol mRootSymbol;
    public TdfSymbol getRootScope()  { return mRootSymbol; }
    public TdfSymbol getRootSymbol()  { return mRootSymbol; }

    public TdfSymbolTable()
    {
        mRootSymbol = new TdfSymbol(this);
    }
}
