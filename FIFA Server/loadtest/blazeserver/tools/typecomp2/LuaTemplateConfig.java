
public class LuaTemplateConfig extends TemplateConfig
{
    static String[][] mLuaScriptTemplates = {
        {"luabt3luaconstants", "etc/bt3/bt3constants.lua", null}
    };
	
	boolean hasRpcCentralTemplates()
	{
		return true;
	}
	
	String[][] getRpcCentralTemplates()
	{
		return mLuaScriptTemplates;
	}
        
    String buildOutputFileBase(String resolvedFileName)
    {
        return resolvedFileName.replace('\\', '/').replace("/gen/","/tdf/");
    }
}

