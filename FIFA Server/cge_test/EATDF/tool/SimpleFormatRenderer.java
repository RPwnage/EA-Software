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
        else if (formatName.equals("_guard"))
        {
            String original = o.toString();
            
            String formatted = "";
            formatted += original.charAt(0);
            
            for (int i = 1; i < original.length(); ++i)
            {
                char c = original.charAt(i);  
                if (Character.isUpperCase(c))
                {
                    formatted += "_" + c;
                }
                else
                {
                    formatted += c;
                }
            }
                
            return formatted.toUpperCase().replaceAll("[^A-Z0-9_]", "_");
        }
        else if (formatName.equals("objcPrefix"))
        {
            return o.toString().toUpperCase().replaceAll("[^A-Z0-9_]", "_").concat("_");
        }
        else if (formatName.equals("javaPackage"))
        {
            String javaPackage = "com.ea." + o.toString().toLowerCase();
            
            if (javaPackage.startsWith("com.ea.eadp"))
              javaPackage = javaPackage.replaceFirst("com.ea.eadp", "com.ea.gs");
            
            return javaPackage;
        }
        else if (formatName.equals("goPackage"))
        {
            String goPackage = o.toString().toLowerCase().replaceAll("[^a-z0-9_]", "_");
            return goPackage;
        }
        else if (formatName.equals("goPackageImportPath"))
        {
            String goModule ="gitlab.ea.com/eadp-gameplay-services/blaze-support/blazegoprotos";
            String goPackage = o.toString().toLowerCase().replaceAll("[^a-z0-9_]", "_");
            return goModule + "/" + goPackage.replaceAll("_", "/");
        }
        else if (formatName.equals("baseFile"))
        {
            String baseFile = getBaseNameWithoutSuffix(o);
            return baseFile;
        }
        else if (formatName.equals("basePath"))
        {
            String basePath = getFullPathWithoutSuffix(o);
            return basePath;
        }
        else if (formatName.equals("unixPath"))
        {
            return o.toString().replace('\\', '/');
        }
        else if (formatName.equals("hex"))
        {
            if (o instanceof Integer)
            {
                return "0x" + Integer.toHexString(((Integer) o).intValue());
            }
            else if (o instanceof Long)
            {
                return "0x" + Long.toHexString(((Long) o).longValue());
            }
            return "";
        }
        else if (formatName.equals("stripnl"))
        {
            // remove newlines (and their extra indendation)
            return o.toString().replaceAll("[\n\r]{1,2}[ \t]*", " ");
        }
        else if (formatName.charAt(0) == '@')
        {
            String formatString = formatName.substring(1);
            
            return String.format(formatString, o);
        }
        else if (formatName.equals("includeproto"))
        {
            String includeFile = o.toString();
            
            if (includeFile.endsWith(".h"))
            {
                return "// TdfToProto INFO: Ignore " + includeFile;
            }
       
            String protoIncludeFile = o.toString().replace(".tdf",".proto");
            return "import \"" + protoIncludeFile + "\";";
        }
        else if (formatName.equals("protoFullyQualified"))
        {
            String type = o.toString().replace("::",".");
                        
            if (type.startsWith("Blaze"))
              type = type.replaceFirst("Blaze", "eadp.blaze");
            
            return type;
        }
        else if (formatName.equals("dotToColon"))
        {
            return o.toString().replace(".","::");
        }
        else if (formatName.equals("tabToSpaces"))
        {
            // remove newlines (and their extra indendation)
            return o.toString().replaceAll("\t", "    ");
        }
        else if (formatName.equals("escapeQuotes"))
        {
            return o.toString().replace("\"","\\\"").replace("\'","\\\'");
        }
        else if (formatName.equals("protoPrimitiveValue"))
        {
            if ((o instanceof Number) || (o instanceof Boolean))
            {
                return o.toString();
            }
            // Handle string. Enclose in double quotes, after removing newlines (and their extra indendation)
            return "\"" +  o.toString().replaceAll("[\n\r]{1,2}[ \t]*", " ") +  "\"";
        }
        else if (formatName.equals("formatUriMetrics"))
        {
            String result = o.toString();
            result = result.replaceAll("([?/*\\\\_])", "-");
            result = result.replaceAll("(^-+|-+$|\\{|\\})", "");
            return result;
        }
        else 
        {
            throw new IllegalArgumentException("Unsupported format name: " + formatName +" for " +o.toString() );
        }
    }

    public String getBaseNameWithoutSuffix(Object o)
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

    public String getFullPathWithoutSuffix(Object o)
    {
        String filePath = o.toString();
        int index = filePath.lastIndexOf('.');
        if ((index > 0) && (index <= filePath.length() - 2))
        {
            filePath = filePath.substring(0, index);
        }
        return filePath;
    }

    public String getDirPath(Object o)
    {
        String dirPath = o.toString();
        int index = dirPath.lastIndexOf('/');
        if ((index > 0) && (index <= dirPath.length() - 2))
        {
            dirPath = dirPath.substring(0, index);
        }
        return dirPath;
    }
}
