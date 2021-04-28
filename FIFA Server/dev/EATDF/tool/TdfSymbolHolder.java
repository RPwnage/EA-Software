import org.antlr.runtime.*;

@SuppressWarnings("serial")
public class TdfSymbolHolder extends TdfSymbol 
{
    TdfSymbol mRef = null;
    TdfSymbol getRef() { return mRef; }
    void setRef(TdfSymbol ref) { mRef = ref; }
    
    public TdfSymbolHolder(int tokenVal, Token token, String name,  
            Category category, ExtHashtable attribs)
    {
        super(tokenVal, token, name, category, attribs);
    }
    
    public TdfSymbol getBaseSymbol()
    {
        if(mRef == null)
        {
            System.out.println("Base symbol called with empty symbol ref.");
            
            return null;
        }
        else
        {
            return mRef.getBaseSymbol();
        }
    }
}
