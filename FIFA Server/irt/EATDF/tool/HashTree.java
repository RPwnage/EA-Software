import org.antlr.runtime.tree.CommonTree;
import org.antlr.runtime.Token;


public class HashTree extends CommonTree 
{
    //Hashtoken passthroughs
    public Object get(Object key) { return getHashToken().get(key);}
    public String getString(Object key) { return getHashToken().getString(key); }
    public void put(Object key, Object value) { getHashToken().put(key, value); }
    public void putInList(String listName, Object value) { getHashToken().putInList(listName, value); }
    public boolean containsKey(Object key) { return getHashToken().containsKey(key); }
    
    public HashToken getHashToken()
    {
        return (HashToken) super.getToken();  
    }

    public HashTree(HashTree tree)
    {
        super(tree);
    }

    public HashTree(Token token)
    {
        super(token);
    }

    public HashTree()
    {
    }
}
