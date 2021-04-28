import java.util.*;

public class JavaTemplateConfig extends TemplateConfig
{
    static String[][] mTdfTemplates = {
        {"javatdfclass", "javagentdfs/ea/%2$s/tdf/%1$s.java", "javagen"},
        {"javaconstants", "%1$s_constants.txt", "javagen"},
        {"javatdfregistration", "%1$s_tdfregistration.txt", "javagen"},
		{"tdfdependencyfile", "tdf/%1$s.d", null, "false"} 
    };

	boolean hasTdfTemplates()
	{
		return true;
	}
	
	String[][] getTdfTemplates() {
		return mTdfTemplates;
	}		

    boolean useSimpleOutput(String templateName) {
        if(templateName == "javatdfregistration") {
            return true;
        }
        else {
            return false;
        } 
    }

    boolean hasCaseSensitiveFilenames()
    {
        return true;
    }

    boolean skipEmptyGeneration(String filename)
    {
        return filename.endsWith(".java");
    }

    String buildOutputFileBase(String resolvedFileName)
    {
        return resolvedFileName.replace('\\', '/').replace("/gen/","/tdf/");
    }
}
