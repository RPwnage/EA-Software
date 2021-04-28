import org.antlr.runtime.tree.CommonTreeAdaptor;
import org.antlr.runtime.Token;


public class HashTreeAdaptor extends CommonTreeAdaptor 
{
    public Token createToken(int tokenType, String text)
    {
        return new HashToken(tokenType, text);
    }

    public Token CreateToken(Token fromToken)
    {
        return new HashToken(fromToken);
    }

    public Object create(Token payload)
    {
        return new HashTree(payload);
    }

    public Object nil()
    {
        return new HashTree();
    }

    public Object dupTree(Object tree)
    {
        return new HashTree((HashTree)tree);
    }

    public Object dupNode(Object tree)
    {
        return new HashTree((HashTree)tree);
    }
}
