import org.antlr.stringtemplate.language.*;

public class TemplateConfig
{
	static TemplateConfig GetLanguageConfig(String language)
	{
		if(language.equalsIgnoreCase("cpp"))
		{
			return new CppTemplateConfig();
		}
		else if(language.equalsIgnoreCase("java"))
		{
			return new JavaTemplateConfig();
		}
		else if(language.equalsIgnoreCase("html"))
		{
			return new HtmlTemplateConfig();
		}
		else if(language.equalsIgnoreCase("lua"))
		{
			return new LuaTemplateConfig();
		}
		else
		{
			return null;
		}
	}
	
    java.lang.Class getStringTemplateLexerClass()
    {
        return AngleBracketTemplateLexer.class;
    }

	boolean useSimpleOutput(String templateName)
	{
		return false;
	}
		
    boolean hasCaseSensitiveFilenames()
    {
        return false;
    }

	boolean generateDepFile()
	{
		return true;
	}
	
	boolean hasArsonRpcTemplates()
	{
		return false;
	}

	String[][] getArsonRpcTemplates()
	{
		return null;
	}
	
	boolean hasArsonClientTdfTemplates()
	{
		return false;
	}
	
	String[][] getArsonClientTdfTemplates()
	{
		return null;
	}
		
	boolean hasClientRpcTemplates()
	{
		return false;
	}
	
	String[][] getClientRpcTemplates()
	{
		return null;
	}
	
	boolean hasClientTdfTemplates()
	{
		return false;
	}
	
	String[][] getClientTdfTemplates()
	{
		return null;
	}
	
	boolean hasRpcTemplates()
	{
		return false;
	}
	
	String[][] getRpcTemplates()
	{
		return null;
	}
	
	boolean hasTdfTemplates()
	{
		return false;
	}
	
	String[][] getTdfTemplates()
	{
		return null;
	}		
		
	boolean hasRpcLuaTemplates()
	{
		return false;
	}
	
	String[][] getRpcLuaTemplates()
	{
		return null;
	}

	boolean hasTdfLuaTemplates()
	{
		return false;
	}

	String[][] getTdfLuaTemplates()
	{
		return null;
	}		
    
    boolean hasRpcProxyTemplates()
	{
		return false;
	}

	String[][] getRpcProxyTemplates()
	{
		return null;
	}

	boolean hasRpcProxyCentralTemplates()
	{
		return false;
	}
	
	String[][] getRpcProxyCentralTemplates()
	{
		return null;
	}
    
    boolean hasArsonRpcCentralTemplates()
	{
		return false;
	}
	
	String[][] getArsonRpcCentralTemplates()
	{
		return null;
	}		
		
	boolean hasClientRpcCentralTemplates()
	{
		return false;
	}
	
	String[][] getClientRpcCentralTemplates()
	{
		return null;
	}			
		
	boolean hasRpcCentralTemplates()
	{
		return false;
	}
	
	String[][] getRpcCentralTemplates()
	{
		return null;
	}	
	
	boolean hasLuaRpcCentralTemplates()
	{
		return false;
	}
	
	String[][] getLuaRpcCentralTemplates()
	{
		return null;
	}

	boolean hasTdfDocComponentTemplates()
	{
		return false;
	}
	
	String[][] getTdfDocComponentTemplates()
	{
		return null;
	}

	boolean hasRpcDocTemplates()
	{
		return false;
	}
    
	String[][] getRpcDocTemplates()
	{
		return null;
	}
	
	boolean hasRpcDocCentralTemplates()
	{
		return false;
	}
    
    String[][] getRpcDocCentralTemplates()
	{
		return null;
	}
    
	boolean hasTdfDocFrameworkTemplates()
	{
		return false;
	}
	
	String[][] getTdfDocFrameworkTemplates()
	{
		return null;
	}
	
	boolean hasTdfDocComponentCentralTemplates()
	{
		return false;
	}
	
	String[][] getTdfDocComponentCentralTemplates()
	{
		return null;
	}	

	String[][] getTimeStampTemplates()
	{
		return null;
	}

    boolean skipEmptyGeneration(String filename)
    {
        return false;
    }

    String buildOutputFileBase(String resolvedFileName)
    {
        return resolvedFileName.replace('\\', '/');
    }
}
