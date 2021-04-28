
public class CppTemplateConfig extends TemplateConfig
{
    //This is a list of all the templates to run for an rpc file
    static String[][] mRpcTemplates = { 
        {"componentinterfaceheader", "rpc/%1$s%2$s.h", "%1$s%2$s", "BLAZE_%1$s_%1$s%2$s_H"},
        {"componentinterfacesource", "rpc/%1$s%2$s.cpp", "%1$s%2$s"},
        {"stuberrorheader", "rpc/%1$s%2$s_errorstub.h", "%1$s%2$s", "BLAZE_%1$s_%1$s%2$s_ERRORSTUB_H"},
        {"stubheaderslave", "rpc/%1$s%2$s_stub.h", "%1$s%2$s", "BLAZE_%1$s_%1$s%2$s_STUB_H"},
        {"stubheadermaster", "rpc/%1$s%2$s_stub.h", "%1$s%2$s", "BLAZE_%1$s_%1$s%2$s_STUB_H"},
        {"stubsourcemaster", "rpc/%1$s%2$s_stub.cpp", "%1$s%2$s"},
        {"stubsourceslave", "rpc/%1$s%2$s_stub.cpp", "%1$s%2$s"},
        {"componentdefinitionheader", "rpc/%1$s_defines.h", "%1$s", "BLAZE_%1$s_DEFINES_H"},
        {"componentdefinitionsource", "rpc/%1$s_defines.cpp", "%1$s"},
        {"commandheader", "rpc/%1$s%2$s/%3$s_stub.h", "%1$s%2$s", "BLAZE_%1$s_%3$s_STUB_H"}
    };

    static String[][] mRpcProxyTemplates = { 
        {"componentinterfaceheader", "rpc/%1$s%2$s.h", "%1$s%2$s", "BLAZE_%1$s_%1$s%2$s_H"},
        {"componentinterfacesource", "rpc/%1$s%2$s.cpp", "%1$s%2$s"},
        {"componentdefinitionheader", "rpc/%1$s_defines.h", "%1$s", "BLAZE_%1$s_DEFINES_H"},
        {"componentdefinitionsource", "rpc/%1$s_defines.cpp", "%1$s"}
    };

    static String[][] mRpcProxyCentralTemplates = { 
        {"proxysource", "blazerpcproxy.cpp", "blazerpc"}
    };

    static String[][] mRpcCentralTemplates = { 
        {"errorheader", "blazerpcerrors.h", "blazerpc", "BLAZE_BLAZERPCERRORS_H"},
        {"errorsource", "blazerpcerrors.cpp", "blazerpc"},
        {"rpcallcommandscommon", "../gengeneratedcommandscommon.cpp", "btgen"}
    };
    
    static String[][] mTdfTemplates = {
        {"tdfheaderfile", "tdf/%1$s.h", null},
        {"tdfsource", "tdf/%1$s.cpp", null},
        {"tdfdependencyfile", "tdf/%1$s.d", null, "false"} 
    };

    static String[][] mClientTdfTemplates = { 
        {"tdfheaderclient", "tdf/%1$s.h", null},
        {"tdfsourceclient", "tdf/%1$s.cpp", null} 
    };
    
    static String[][] mArsonClientTdfTemplates = { 
        {"tdfheaderclientarson", "%1$s.h", null},
        {"tdfsourceclientarson", "%1$s.cpp", null} 
    };

    static String[][] mClientRpcTemplates = { 
        {"rpcclientcomponentheader", "component/%1$scomponent.h", null, "BLAZE_COMPONENT_%1$s_H"},
        {"rpcclientcomponentsource", "component/%1$scomponent.cpp", null} 
    };
    
    static String[][] mClientRpcCentralTemplates = { 
        {"rpcclienterrortypeheader", "blazeerrortype.h", "blazerpc", "BLAZE_BLAZERPCERRORTYPE_H"},
        {"rpcclienterrorheader", "blazeerrors.h", "blazerpc", "BLAZE_BLAZERPCERRORS_H"},
        {"rpcclientcomponentmanagerheader", "componentmanager.h", "blazerpc", "BLAZE_BLAZECOMPONENTMANAGER_H"},
        {"rpcclientcomponentmanagersource", "componentmanager.cpp", "blazerpc"}
    };
    
    static String[][] mArsonRpcTemplates = { 
        {"rpcarsoncommandheader", "commands/blaze/%1$s%2$s/%3$s_stub.h", null, "TESTER_ARSON_%1$s_%3$s_H"}
    };
    
    static String[][] mArsonRpcCentralTemplates = { 
        {"rpcarsongeneratedcommandsheader", "commands/blaze/generatedcommands.h", null, "ARSON_COMMANDS_HEADER_H"},
        {"rpcarsonservercommandshelperheader", "commands/servercommandshelper.h", null, "ARSON_SERVER_COMMANDS_HELPER_H"},
        {"rpcarsonservercommandshelpersource", "commands/servercommandshelper.cpp", null, null },
		{"rpcarsonservercommandshelpersource1", "commands/servercommandshelper1.cpp", null, null },
		{"rpcarsonservercommandshelpersource2", "commands/servercommandshelper2.cpp", null, null },
        {"rpcarsonclientcommandshelperheader", "actions/clientcommandshelper.h", null, "ARSON_CLIENT_COMMANDS_HELPER_H"},
        {"rpcarsonclientcommandshelpersource", "actions/clientcommandshelper.cpp", null, null },
        {"rpcarsonblazeerrorssource", "actions/blaze/arsonblazeerrors.cpp", null, null }
    };

	static String[][] mRpcLuaTemplates = {		
		{"luastressrpcsheader", "rpc/%1$s_luarpcs.h", null, "BLAZE_%1$s_%1$s%2$s_STRESS_H"},
		{"luastressrpcssource", "rpc/%1$s_luarpcs.cpp", null},
		{"luabt3rpcsheader", "rpc/%1$s_bt3luarpcs.h", null, "BLAZE_%1$s_%1$s%2$s_BT3_H"},
		{"luabt3rpcssource", "rpc/%1$s_bt3luarpcs.cpp", null},
	};

	static String[][] mTdfLuaTemplates = {
		{"luastresstypesheader", "tdf/%1$s_luatypes.h", null, "BLAZE_STRESS_%2$s_H"},
        {"luastresstypessource", "tdf/%1$s_luatypes.cpp", null},
		{"luabt3typesheader", "tdf/%1$s_bt3luatypes.h", null, "BLAZE_BT3_%2$s_H"},
        {"luabt3typessource", "tdf/%1$s_bt3luatypes.cpp", null},
	};
    
    static String[][] mLuaRpcCentralTemplates = { 
        {"luastresscommonheader", "tools/stress/lua/luacommon.h", null, "BLAZE_STRESS_LUA_COMMON_H"},
        {"luastresscommonsource", "tools/stress/lua/luacommon.cpp", null, null},
        {"luabt3commonheader", "tools/bt3/lua/luacommon.h", null, "BLAZE_BT3_LUA_COMMON_H"},
        {"luabt3commonsource", "tools/bt3/lua/luacommon.cpp", null, null}
    };

	static String[][] mTimeStampTemplates = { 
        {"timestamp", "%1$s.d", null, null},
    };

	boolean generateDepFile()
	{
		return true;
	}
		
	boolean hasArsonRpcTemplates()
	{
		return true;
	}

	String[][] getArsonRpcTemplates()
	{
		return mArsonRpcTemplates;
	}
	
	boolean hasArsonClientTdfTemplates()
	{
		return true;
	}
	
	String[][] getArsonClientTdfTemplates()
	{
		return mArsonClientTdfTemplates;
	}
	
	boolean hasClientRpcTemplates()
	{
		return true;
	}

    String[][] getClientRpcTemplates()
    {
        return mClientRpcTemplates;
    }
	
	boolean hasClientTdfTemplates()
	{
		return true;
	}
	
	String[][] getClientTdfTemplates()
	{
		return mClientTdfTemplates;
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
		
	boolean hasRpcLuaTemplates()
	{
		return true;
	}
	
	String[][] getRpcLuaTemplates()
	{
		return mRpcLuaTemplates;
	}
			
	boolean hasTdfLuaTemplates()
	{
		return true;
	}
	
	String[][] getTdfLuaTemplates()
	{
		return mTdfLuaTemplates;
	}		
		
	boolean hasRpcProxyTemplates()
	{
		return true;
	}
	
	String[][] getRpcProxyTemplates()
	{
		return mRpcProxyTemplates;
	}

	boolean hasRpcProxyCentralTemplates()
	{
		return true;
	}
	
	String[][] getRpcProxyCentralTemplates()
	{
		return mRpcProxyCentralTemplates;
	}
			
	boolean hasArsonRpcCentralTemplates()
	{
		return true;
	}
	
	String[][] getArsonRpcCentralTemplates()
	{
		return mArsonRpcCentralTemplates;
	}		
		
	boolean hasClientRpcCentralTemplates()
	{
		return true;
	}
	
	String[][] getClientRpcCentralTemplates()
	{
		return mClientRpcCentralTemplates;
	}			
		
	boolean hasRpcCentralTemplates()
	{
		return true;
	}
	
	String[][] getRpcCentralTemplates()
	{
		return mRpcCentralTemplates;
	}	
	
	boolean hasLuaRpcCentralTemplates()
	{
		return true;
	}
	
	String[][] getLuaRpcCentralTemplates()
	{
		return mLuaRpcCentralTemplates;
	}

	String[][] getTimeStampTemplates()
	{
		return mTimeStampTemplates;
	}

    boolean skipEmptyGeneration(String filename)
    {
        return filename.endsWith(".cpp");
    }

    String buildOutputFileBase(String resolvedFileName)
    {
        // Existing behavior from original TdfComp.java
        return resolvedFileName.replace('\\', '/').replace("/gen/","/tdf/");
    }
}

