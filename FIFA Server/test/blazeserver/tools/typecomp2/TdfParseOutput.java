import org.antlr.runtime.CommonTokenStream;
import org.antlr.runtime.tree.CommonTree;

public class TdfParseOutput
{
    public CommonTree tree;
    public CommonTokenStream tokens;
    public String packageName;
    public String resolvedFileName;
};
