import org.antlr.runtime.CharStream;
import org.antlr.runtime.Token;

public class HashToken extends AttributedObject implements Token
{
    private static final long serialVersionUID = 1L;
    
    int mChannel;
    public int getChannel() { return mChannel; }
    public void setChannel(int channel) { mChannel = channel; }
    
    int mCharPositionInLine;
    public int getCharPositionInLine() { return mCharPositionInLine; }
    public void setCharPositionInLine(int pos) { mCharPositionInLine = pos; }
    
    int mLine;
    public int getLine() { return mLine; }
    public void setLine(int line) { mLine = line; }
    
    String mText;
    public String getText() { return mText; }
    public void setText(String text) { mText = text; }
  
    int mTokenIndex;
    public int getTokenIndex() { return mTokenIndex; }
    public void setTokenIndex(int index) { mTokenIndex = index; }
       
    int mType;
    public int getType() { return mType; }
    public void setType(int type) { mType = type; }
    
    public HashToken put(Object key, Object value)
    {
        return (HashToken) super.put(key, value);
    }

    public HashToken(int type)
    {
        mType = type;
    }

    public HashToken(int type, String text)
    {
        mType = type;
        mText = text;
    }

    public HashToken(int type, Token token)
    {
        setFromToken(token);
        mType = type;
        if (token.getClass() == HashToken.class)
        {
            put((HashToken)token);
        }
    }

    public HashToken(Token token)
    {
        setFromToken(token);
        if (token.getClass() == HashToken.class)
        {
            put((HashToken)token);
        }
    }

    public HashToken(int type, Token token, ExtHashtable table)
    {
        setFromToken(token);
        mType = type;
        put(table);
    }

    public HashToken(int token, String text, ExtHashtable table)
    {
        mType = token;
        mText = text;
        put(table);
    }

    private void setFromToken(Token token)
    {
        mType = token.getType();
        mChannel = token.getChannel();
        mCharPositionInLine = token.getCharPositionInLine();
        mLine = token.getLine();
        mText = token.getText();
        mTokenIndex = token.getTokenIndex();
    }
    
    CharStream mCharStream;
    public CharStream getInputStream() { return mCharStream; }
    public void setInputStream(CharStream val) { mCharStream = val; }
    
}
