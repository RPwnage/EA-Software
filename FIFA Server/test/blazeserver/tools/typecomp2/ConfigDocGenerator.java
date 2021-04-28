/* Config Documentation Generator */
/*   
 *  Generates the html file used to document Blaze configuration
 *  
 *  This document serves as a reference for Blaze configuration. For a full guide to using the Blaze configuration TDF.
 *  Please see Confluence: http://online.ea.com/documentation/display/blaze/How+To+Read+Configuration+Into+TDF+Class+Object

 */
import java.io.File;
import java.io.PrintStream;
import java.io.FileNotFoundException;
import java.util.*;

public class ConfigDocGenerator
{
    private static PrintStream printStream = null;
    
    private static void indexPreGenerate(String fileName)
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
            e.printStackTrace();
        }
            
        //Add Document header
        printStream.println("<html>");
        printStream.println("<head>");
        printStream.println("  <title> Index </title>");
        printStream.println("  <link rel=\"stylesheet\" type=\"text/css\" name=\"cfgdoc\" href=\"wadl.css\" />");
        printStream.println("</head>");
        printStream.println("<body>");
        printStream.println("  <h1><a href=\"index.html\">Blaze Configuration TDF</a> :: index</h1>");
        printStream.println("  <p><i>This document serves as a reference for the configuration TDF classes. ");
        printStream.println("For a full guide to using the configuration TDF classes, ");
        printStream.println("<a href=\"http://online.ea.com/documentation/display/blaze/How+To+Read+Configuration+Into+TDF+Class+Object\"> see Confluence</a></i></p>");
        printStream.println("<p>Blaze component index. Select a resource below to view  HTML documentation.</p>");
        printStream.println("<ul>");
        printStream.println("<li>");
        printStream.println("<b>Blaze</b>");
        printStream.println("<ul>");
    }
    
    // Generate index html for config docs
    public static void generateIndex(String indexFileName) throws Exception
    {
    	ConfigDocGenerator.indexPreGenerate(indexFileName);
       
        printStream.println("<li><a href=\"index_component.html\">Components</a></li>");
        printStream.println("<li><a href=\"framework/frameworkconfig.html\">Framework</a></li>");
        printStream.println("<li><a href=\"framework/loggingconfig.html\">Logging</a></li>");
        printStream.println("<li><a href=\"framework/bootconfig.html\">Boot</a></li>");
       
        ConfigDocGenerator.indexPostGenerate();
    }
    
    private static void indexPostGenerate() throws Exception
    {
        printStream.println("</ul>");
        printStream.println("</li>");
        printStream.println("</ul>");
        printStream.println("</body>");
        printStream.println("</html>");
        printStream.close();
    }
}
