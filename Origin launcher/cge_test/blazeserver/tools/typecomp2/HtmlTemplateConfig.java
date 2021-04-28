import org.antlr.stringtemplate.language.*;

public class HtmlTemplateConfig extends TemplateConfig
{
    static String[][] mRpcTemplates = { 
        {"luarpcdoc", "doc/%1$sluarpcs.html", null}
    };
    
    static String[][] mRpcDocTemplates = { 
        {"waldoc", "wadl/%1$s.html", null}
    };
    
    static String[][] mRpcDocCentralTemplates = { 
        {"walindex", "wadl/index.html", null}
    };

    static String[][] mTdfTemplates = {
        {"luatdfdoc", "doc/%1$sluatypes.html", null},
		{"tdfdependencyfile", "tdf/%1$s.d", null, "false"} 
    };
    
    static String[][] mTdfDocComponentTemplates = { 
        {"configtdfdoc", "tdf/component/%1$s.html", null}
    };
    static String[][] mTdfDocFrameworkTemplates = { 
        {"configtdfdocclass", "tdf/framework/%1$s.html", null}
    };
    static String[][] mTdfDocComponentCentralTemplates = {
        {"configtdfdocindex", "tdf/index_component.html", null}
    };
    
    java.lang.Class getStringTemplateLexerClass()
    {
        return DefaultTemplateLexer.class;
    }

	boolean hasRpcTemplates()
	{
		return true;
	}
	
	String[][] getRpcTemplates()
	{
		return mRpcTemplates;
	}

	boolean hasTdfTemplates()
	{
		return true;
	}
	
	String[][] getTdfTemplates()
	{
		return mTdfTemplates;
	}		

	boolean hasTdfDocComponentTemplates()
	{
		return true;
	}
	
	String[][] getTdfDocComponentTemplates()
	{
		return mTdfDocComponentTemplates;
	}	
    
    boolean hasRpcDocTemplates()
	{
		return true;
	}
    
    String[][] getRpcDocTemplates()
	{
		return mRpcDocTemplates;
	}
	
	boolean hasRpcDocCentralTemplates()
	{
		return true;
	}
    
    String[][] getRpcDocCentralTemplates()
	{
		return mRpcDocCentralTemplates;
	}	

	boolean hasTdfDocFrameworkTemplates()
	{
		return true;
	}
	
	String[][] getTdfDocFrameworkTemplates()
	{
		return mTdfDocFrameworkTemplates;
	}

	boolean hasTdfDocComponentCentralTemplates()
	{
		return true;
	}
	
	String[][] getTdfDocComponentCentralTemplates()
	{
		return mTdfDocComponentCentralTemplates;
	}	

    String buildOutputFileBase(String resolvedFileName)
    {
        return resolvedFileName.replace('\\', '/').replace("/gen/","/doc/").replace(".rpc", "").replace(".tdf", "");
    }
}
