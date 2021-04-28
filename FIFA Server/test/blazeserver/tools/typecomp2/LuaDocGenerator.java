/* Lua Documentation Generator */
/*   
 *  Generates the WADL file used to document Blaze web access layer
 *  
 *  This document serves as a reference for Lua stress testing types. For a full guide to using the Lua stress testing types.
 *  Please see Confluence: http://online.ea.com/confluence/display/blaze/Stress+Testing

 */
import java.io.File;
import java.io.PrintStream;
import java.io.FileNotFoundException;
import java.util.*;

public class LuaDocGenerator
{
    private static PrintStream printStream = null;
    
    private static void preGenerate(String fileName)
    {
        //Create the output path if it doesn't exist.
        File outputFile = new File(fileName);
        outputFile.getParentFile().mkdirs();
        
        try 
        {
            printStream = new PrintStream(outputFile);
        } 
        catch (FileNotFoundException e)
        {
            // TODO Auto-generated catch block
            e.printStackTrace();
        }
            
        //Add Document header
        printStream.println("<html>");
        printStream.println("<head>");
        printStream.println("  <title> index </title>");
        printStream.println("  <link rel=\"stylesheet\" type=\"text/css\" name=\"luadoc\" href=\"wadl.css\" />");
        printStream.println("</head>");
        printStream.println("<body>");
        printStream.println("  <h1><a href=\"index.html\">Lua stress testing types</a> :: index</h1>");
        printStream.println("  <p><i>This document serves as a reference for Lua stress testing types.");
        printStream.println("For a full guide to using the Lua stress testing types, ");
        printStream.println("<a href=\"http://online.ea.com/confluence/display/blaze/Stress+Testing\"> see Confluence</a></i></p>");
        printStream.println("<p>Blaze component index. Select a resource below to view  HTML documentation.</p>");
        printStream.println("<ul>");
        printStream.println("<li>");
        printStream.println("<b>Components</b>");
        printStream.println("<ul>");
    }
    
    public static void generate(ArrayList<String> outputFileNames, String fileName) throws Exception
    {
       LuaDocGenerator.preGenerate(fileName);
       
       for (String filename : outputFileNames)
       {
           int startPos = Math.max(filename.lastIndexOf("/"), filename.lastIndexOf("\\"));
           startPos++;
       
           String relativeFileName = filename.substring(startPos);
           int endPos = relativeFileName.lastIndexOf(".");
       
           if (endPos == -1)
               endPos = relativeFileName.length();
               
           printStream.println("<li><a href=\"" + filename + "\">" + relativeFileName.substring(0, endPos) + "</a></li>");
       }
       
       LuaDocGenerator.postGenerate();
    }
    
    private static void postGenerate() throws Exception
    {
        printStream.println("</ul>");
        printStream.println("</li>");
        printStream.println("</ul>");
        printStream.println("</body>");
        printStream.println("</html>");
        printStream.close();
    }
}
