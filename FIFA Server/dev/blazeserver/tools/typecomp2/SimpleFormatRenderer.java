import java.io.File;

import org.antlr.stringtemplate.AttributeRenderer;


public class SimpleFormatRenderer implements AttributeRenderer 
{
    public String toString(Object o) { return o.toString(); }
    public String toString(Object o, String formatName)  
    {
        if (formatName.equals("toUpper") || formatName.equals("tu")) 
        {
            return o.toString().toUpperCase();
        } 
        else if (formatName.equals("toLower") || formatName.equals("tl")) 
        {
            return o.toString().toLowerCase();
        }
        else if (formatName.equals("pascal"))
        {
            String temp = o.toString();
            return temp.substring(0, 1).toUpperCase() + temp.substring(1);
        }
        else if (formatName.equals("java"))
        {
            String temp = o.toString();
            return temp.substring(0, 1).toLowerCase() + temp.substring(1);
        }
        else if (formatName.equals("guard"))
        {
            return o.toString().toUpperCase().replaceAll("[^A-Z0-9_]", "_");
        }
        else if (formatName.equals("baseFile"))
        {
            File file = new File(o.toString());
            String baseName = o.toString();
            int index = file.getName().lastIndexOf('.');
            if ((index > 0) && (index <= file.getName().length() - 2)) 
            {
                baseName = file.getName().substring(0, index);
            }  
            return baseName;
        }
        else if (formatName.equals("basePath"))
        {
            String baseName = o.toString();
            int index = baseName.lastIndexOf('.');
            if ((index > 0) && (index <= baseName.length() - 2)) 
            {
                baseName = baseName.substring(0, index);
            }  
            return baseName;
        }        
        else if (formatName.equals("hex"))
        {
            if (o instanceof Integer)
            {
                return "0x" + Integer.toHexString(((Integer) o).intValue());
            }
            return "";
        }
        else if (formatName.equals("stripnl"))
        {
            return o.toString().replace("\n", "").replace("\r", "");
        }
        else if (formatName.charAt(0) == '@')
        {
            String formatString = formatName.substring(1);
            
            return String.format(formatString, o);
        }
        else 
        {
            throw new IllegalArgumentException("Unsupported format name");
        }
    }
}
