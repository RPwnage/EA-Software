import org.antlr.runtime.*;
import org.antlr.runtime.tree.*;
import org.antlr.stringtemplate.*;


import java.io.*;
import java.util.*;

public class Rpc2Tdf 
{
  
    static void parseRpcFile(String inputFilename, String outputFilename) throws Exception
    {
        ANTLRFileStream stream = new ANTLRFileStream(inputFilename);
        Rpc2TdfGrammarLexer lexer = new Rpc2TdfGrammarLexer(stream);
        TokenRewriteStream tokens = new TokenRewriteStream(lexer);
        
        //Create the parser and parse to a node stream.
        Rpc2TdfGrammarParser parser = new Rpc2TdfGrammarParser(tokens);
        StringTemplate outputTemplate = parser.file().st;
        
                     
        Writer fw = new FileWriter(outputFilename);
        fw.write(tokens.toString());
        fw.close();
    }    
    
    static StringTemplateGroupLoader mLoader;
    static StringTemplateGroup mCommonGroup;        
    
    public static void main(String[] args) throws Exception 
    {
       for (String s : args) 
       {
    	   parseRpcFile(s, s);
       }
    }
}

