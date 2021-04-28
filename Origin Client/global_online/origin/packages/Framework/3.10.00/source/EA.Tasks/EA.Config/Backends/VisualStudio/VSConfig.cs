using System;
using System.Collections.Generic;
using System.Collections.Concurrent;
using System.Linq;
using System.Text;
using System.IO;
using System.Xml;
using System.Text.RegularExpressions;

using NAnt.Core;
using NAnt.Core.Util;
using NAnt.Core.Logging;
using NAnt.Core.PackageCore;
using EA.FrameworkTasks.Model;



namespace EA.Eaconfig.Backends.VisualStudio
{
    internal class VSConfig 
    {
        internal static string config_vs_version = "10.0";

        internal enum CharacterSetTypes { Unknown, Unicode, MultiByte }

        internal struct SwitchInfo
        {
            //switchInfo struct information:
            //Switch -			A regular expression specifying the command line switch format. ie. new Regex("^[-/]nodefaultlib:\"(.+)\"$")
            //
            //XMLName -			The name of the XML attribute that the switch controls ie. "IgnoreDefaultLibraryNames"
            //					if the name is empty, the switch is ignored
            //
            //TypeSpec -		A comma delimited listing of types (int, string, hex) that specity the types of the
            //					$1...$n in the regular expression
            //					int - $n is an integer
            //					hex - $n is a hex number, even without the "0x" prefix
            //                  int-hex - $n is a hex number if it is prefixed with "0x", integer otherwise
            //					string - $n is a string
            //
            //XMLResultString -	A format specification (like String.Format uses) that specifies what to set the attribute to;
            //					{0} refers to the previous value of the attribute;
            //					{1} and up refer to the text pulled out with the reg exp match
            //					ie. (for the above setting): "{0};{1}"
            //					    appends the argument of nodefaultlib to the existing setting (separated by a ;)
            //
            //SetOnce -			If true, any modification to an XML attribute that's already set will throw an exception;
            //					is designed to prevent conflicting settings. Default: true
            //MatchOnce -		If there is a regex matching this switch, don't attempt to match against othere regex's
            //					Default: false (for backwards compatability - some settings assume a match more than once)

            internal SwitchInfo(Regex Switch, String TypeSpec, String XMLName, String XMLResultString, bool SetOnce, bool MatchOnce)
            {
                mySwitch = Switch;
                myTypeSpec = TypeSpec;
                myXMLName = XMLName;
                myXMLResultString = XMLResultString;
                mySetOnce = SetOnce;
                myMatchOnce = MatchOnce;
            }

            internal Regex Switch
            {
                get { return mySwitch; }
                set { mySwitch = value; }
            }

            internal String XMLName
            {
                get { return myXMLName; }
                set { myXMLName = value; }
            }

            internal String TypeSpec
            {
                get { return myTypeSpec; }
                set { myTypeSpec = value; }
            }

            internal String XMLResultString
            {
                get { return myXMLResultString; }
                set { myXMLResultString = value; }
            }

            internal bool SetOnce
            {
                get { return mySetOnce; }
                set { mySetOnce = value; }
            }

            internal bool MatchOnce
            {
                get { return myMatchOnce; }
                set { myMatchOnce = value; }
            }

            private Regex mySwitch;
            private String myTypeSpec;
            private String myXMLName;
            private String myXMLResultString;
            private bool mySetOnce;
            private bool myMatchOnce;
        }

        internal class ParseDirectives
        {
            internal List<SwitchInfo> General = new List<SwitchInfo>();
            internal List<SwitchInfo> Cc = new List<SwitchInfo>();
            internal List<SwitchInfo> Link = new List<SwitchInfo>();
            internal List<SwitchInfo> Lib = new List<SwitchInfo>();
            internal List<SwitchInfo> Ximg = new List<SwitchInfo>();
            internal List<SwitchInfo> Midl = new List<SwitchInfo>();
        }

        internal static CharacterSetTypes GetCharacterSetFromDefines(IEnumerable<string> defines)
        {
            return (defines != null && null != defines.FirstOrDefault(def => def == "UNICODE" | def == "_UNICODE")) ? CharacterSetTypes.Unicode : CharacterSetTypes.MultiByte;
        }

        internal static ParseDirectives GetParseDirectives(IModule module)
        {
            return GetParseDirectives(GetConfigPlatform(module.Configuration), GetConfigCompiler(module.Configuration), module.Package.Project);
        }

        internal static ParseDirectives GetParseDirectives(string platform, string compiler, Project project)
        {
            string key = platform + "#" + compiler;

            var directives = _parseDirectiveMap.GetOrAdd(key, k => CreateParseDirectives(platform, compiler, project));
            
            return directives;
        }

        internal static string CleanString(string dirtyString, bool qmarkEnclosed, bool semicolonDelim, bool trimQmarks)
        {

            return dirtyString;
        }

        #region Private

        private static string GetConfigPlatform(Configuration configuration)
        {
            return configuration.System;
        }

        private static string GetConfigCompiler(Configuration configuration)
        {
            return configuration.Compiler;
        }


        private static ParseDirectives CreateParseDirectives(string platform, string compiler, Project project)
        {
            ParseDirectives parse = new ParseDirectives();

            // $$$ HACK - See old nantToVcproj.cs.  In there sn is only set for gamecube/ps2.
            if ("ps3" == platform)
            {
                compiler = "gcc";
            }
            // $$$ ENDHACK

            GetRulesAndAdd(project, parse.General, "/rules/general/rule");

            GetRulesAndAdd(project, parse.Cc, "/rules/compiler/" + compiler + "/rule", "/rules/compiler/" + compiler + "/" + platform + "/rule");
            GetRulesAndAdd(project, parse.Cc, "/rules/compiler/" + platform + "/rule", "/rules/compiler/" + platform + "/" + compiler + "/rule");

            GetRulesAndAdd(project, parse.Link, "/rules/linker/" + compiler + "/rule", "/rules/linker/" + compiler + "/" + platform + "/rule");
            GetRulesAndAdd(project, parse.Link, "/rules/linker/" + platform + "/rule", "/rules/linker/" + platform + "/" + compiler + "/rule");

            GetRulesAndAdd(project, parse.Lib, "/rules/librarian/" + compiler + "/rule", "/rules/librarian/" + compiler + "/" + platform + "/rule");
            GetRulesAndAdd(project, parse.Lib, "/rules/librarian/" + platform + "/rule", "/rules/librarian/" + platform + "/" + compiler + "/rule");

            GetRulesAndAdd(project, parse.Ximg, "/rules/imagebuilder/" + compiler + "/rule", "/rules/imagebuilder/" + compiler + "/" + platform + "/rule");
            GetRulesAndAdd(project, parse.Ximg, "/rules/imagebuilder/" + platform + "/rule", "/rules/imagebuilder/" + platform + "/" + compiler + "/rule");

            GetRulesAndAdd(project, parse.Midl, "/rules/midl/" + compiler + "/rule", "/rules/midl/" + compiler + "/" + platform + "/rule");
            GetRulesAndAdd(project, parse.Midl, "/rules/midl/" + platform + "/rule", "/rules/midl/" + platform + "/" + compiler + "/rule");

            return parse;
        }

        private static IEnumerable<XmlNodeList> GetPlatformSpecificAndGenericLists(Project project, string rulePath, string specificRulePath)
        {
            XmlNodeList specificRuleNodes = null;
            XmlNodeList genericRuleNodes = null;

            var specificDoc = GetPlatformOptionsXmlDoc(project);

            if (specificDoc != null)
            {
                specificRuleNodes = (specificRulePath != null) ? specificDoc.DocumentElement.SelectNodes(specificRulePath) : null;
                genericRuleNodes = specificDoc.DocumentElement.SelectNodes(rulePath);
            }

            XmlDocument oXmlDoc = GetOptionsXmlDoc();

            if (specificRuleNodes == null || specificRuleNodes.Count == 0)
            {
                specificRuleNodes= (specificRulePath != null) ? oXmlDoc.DocumentElement.SelectNodes(specificRulePath): null;
            }

            if (genericRuleNodes == null || genericRuleNodes.Count == 0)
            {
                genericRuleNodes = oXmlDoc.DocumentElement.SelectNodes(rulePath);
            }

            return new XmlNodeList[] { specificRuleNodes, genericRuleNodes };
        }

        private static void GetRulesAndAdd(Project project, List<SwitchInfo> parseDirectives, string rulePath, string specificRulePath= null)
        {

            var lists = GetPlatformSpecificAndGenericLists(project, rulePath, specificRulePath);

            var specificFound = new HashSet<string>();

            XmlAttribute attrib;
            string regex;
            string typespec;
            string xmlname;
            string xmlvalue;
            bool bSetOnce, bMatchOnce;
            RegexOptions roptions;

            try
            {
                bool processingSpecific = true;

                foreach (var oNodeList in lists)
                {
                    if (oNodeList == null)
                    {
                        processingSpecific = false;
                        continue;
                    }
                    for (int x = 0; x < oNodeList.Count; x++)
                    {
                        regex = string.Empty;
                        typespec = string.Empty;
                        xmlname = string.Empty;
                        xmlvalue = string.Empty;

                        bSetOnce = true;
                        bMatchOnce = false;
                        roptions = RegexOptions.CultureInvariant;

                        if (oNodeList.Item(x).Attributes["regex"] != null)
                        {
                            regex = oNodeList.Item(x).Attributes["regex"].InnerText;
                            if (processingSpecific)
                            {
                                specificFound.Add(regex);
                            }
                            else if (specificFound.Contains(regex))
                            {
                                continue;
                            }
                        }
                        else
                        {
                            var msg = String.Format("regex is not specified for a rule '{0}' in file '{1}'", oNodeList.Item(x).OuterXml, oNodeList.Item(x).OwnerDocument.BaseURI);
                            throw new BuildException(msg);
                        }

                        if ((attrib = oNodeList.Item(x).Attributes["ignorecase"]) != null &&
                            Convert.ToBoolean(attrib.InnerText))
                        {
                            roptions = RegexOptions.IgnoreCase;
                        }

                        if (oNodeList.Item(x).Attributes["typespec"] != null)
                            typespec = oNodeList.Item(x).Attributes["typespec"].InnerText;

                        if (oNodeList.Item(x).Attributes["xmlname"] != null)
                            xmlname = oNodeList.Item(x).Attributes["xmlname"].InnerText;

                        if (oNodeList.Item(x).Attributes["xmlvalue"] != null)
                            xmlvalue = oNodeList.Item(x).Attributes["xmlvalue"].InnerText;

                        if (oNodeList.Item(x).Attributes["setonce"] != null)
                            bSetOnce = Convert.ToBoolean(oNodeList.Item(x).Attributes["setonce"].InnerText);

                        if (oNodeList.Item(x).Attributes["matchonce"] != null)
                            bMatchOnce = Convert.ToBoolean(oNodeList.Item(x).Attributes["matchonce"].InnerText);

                        parseDirectives.Add(new SwitchInfo(new Regex(regex, roptions), typespec, xmlname, xmlvalue, bSetOnce, bMatchOnce));
                    }

                    processingSpecific = false;
                }
            }
            catch (Exception e)
            {
                throw new BuildException("Failed to process " + Path.GetFileName(_xmlDoc.BaseURI) + ":", e);
            }
        }

        private static XmlDocument GetOptionsXmlDoc()
        {
            if (_xmlDoc == null)
            {
                lock (_SyncRoot)
                {
                    if (_xmlDoc == null)
                    {
                        var temp_xmlDoc = new XmlDocument();

                        string optionFilename = config_vs_version.StrCompareVersions("10.0") >= 0 ? "options_msbuild.xml" : "options.xml";

                        var dataPath = Path.GetFullPath(Path.Combine(PackageMap.Instance.GetFrameworkRelease().Path, @"data\" + optionFilename));

                        try
                        {
                            temp_xmlDoc.Load(dataPath);
                        }
                        catch (XmlException error)
                        {
                            throw new BuildException("Error trying to open " + dataPath + ":", error);

                        }
                        catch (FileNotFoundException error)
                        {
                            throw new BuildException("Error trying to open " + dataPath + ":", error);
                        }

                        _xmlDoc = temp_xmlDoc;
                    }
                }
            }
            return _xmlDoc;
        }

        private static XmlDocument GetPlatformOptionsXmlDoc(Project project)
        {
            if (project != null)
            {
                var platformConfigPackage = project.Properties["nant.platform-config-package-name"].TrimWhiteSpace();

                if (!string.IsNullOrEmpty(platformConfigPackage))
                {
                    return _platformDocs.GetOrAdd(platformConfigPackage, platformConfigPackageKey =>
                        {
                            string optionFilename = config_vs_version.StrCompareVersions("10.0") >= 0 ? "options_msbuild.xml" : "options.xml";

                            var dataPath = Path.GetFullPath(Path.Combine(project.Properties["package." + platformConfigPackageKey + ".dir"], @"config\data\" + optionFilename));
                            if (File.Exists(dataPath))
                            {
                                var xmlDoc = new XmlDocument();
                                try
                                {
                                    xmlDoc.Load(dataPath);
                                }
                                catch (XmlException error)
                                {
                                    throw new BuildException("Error trying to open " + dataPath + ":", error);

                                }
                                catch (FileNotFoundException error)
                                {
                                    throw new BuildException("Error trying to open " + dataPath + ":", error);
                                }
                                return xmlDoc;
                            }
                            return null;
                        });
                }
            }
            return null;
        }


        private static BlockingConcurrentDictionary<string, ParseDirectives> _parseDirectiveMap = new BlockingConcurrentDictionary<string, ParseDirectives>();
        private static XmlDocument _xmlDoc;
        private static ConcurrentDictionary<string, XmlDocument> _platformDocs = new ConcurrentDictionary<string, XmlDocument>();
        private static object _SyncRoot = new object();

        #endregion Private

    }
}
