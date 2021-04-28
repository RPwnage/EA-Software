import java.util.*;

@SuppressWarnings("serial")
public class TdfSymbolName extends ArrayList<String> 
{
    public TdfSymbolName()
    {
    }
    
    public TdfSymbol resolve(TdfSymbol scope, TdfSymbol.SymbolType type)
    {
        TdfSymbol result = null;
        for (int i = scope.getScopeHeirarchy().size() - 1; result == null && i >= 0; --i)
        {
            Iterator<String> itr = iterator();
            if (itr.hasNext())
            {
                result = scope.getScopeHeirarchy().get(i).findSymbol(itr, type);
            }
        }
            
        return result;
    }
}
