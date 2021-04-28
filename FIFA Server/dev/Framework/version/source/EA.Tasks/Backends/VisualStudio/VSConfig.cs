// Originally based on NAnt - A .NET build tool
// Copyright (C) 2003-2018 Electronic Arts Inc.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
// 
// As a special exception, the copyright holders of this software give you 
// permission to link the assemblies with independent modules to produce 
// new assemblies, regardless of the license terms of these independent 
// modules, and to copy and distribute the resulting assemblies under terms 
// of your choice, provided that you also meet, for each linked independent 
// module, the terms and conditions of the license of that module. An 
// independent module is a module which is not derived from or based 
// on these assemblies. If you modify this software, you may extend 
// this exception to your version of the software, but you are not 
// obligated to do so. If you do not wish to do so, delete this exception 
// statement from your version. 
// 
// Electronic Arts (Frostbite.Team.CM@ea.com)

using System;
using System.Collections.Generic;
using System.Collections.Concurrent;
using System.Linq;
using System.IO;
using System.Xml;
using System.Text.RegularExpressions;

using NAnt.Core;
using NAnt.Core.PackageCore;
using EA.FrameworkTasks.Model;

namespace EA.Eaconfig.Backends.VisualStudio
{
	internal class VSConfig 
	{
		internal enum CharacterSetTypes { Unknown, Unicode, MultiByte }

		internal struct SwitchInfo
		{
			//switchInfo struct information:
			//Switch -			A regular expression specifying the command line switch format. ie. new Regex("^[-/]nodefaultlib:\"(.+)\"$")
			//
			//XMLName -			The name of the XML attribute that the switch controls ie. "IgnoreDefaultLibraryNames"
			//					if the name is empty, the switch is ignored
			//
			//TypeSpec -		A comma delimited listing of types (int, string, hex) that specify the types of the
			//					$1...$n in the regular expression
			//					int - $n is an integer
			//					hex - $n is a hex number, even without the "0x" prefix
			//					int-hex - $n is a hex number if it is prefixed with "0x", integer otherwise
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
			//
			//MatchOnce -		If there is a regex matching this switch, don't attempt to match against other regex's
			//					Default: false (for backwards compatibility - some settings assume a match more than once)
			//
			//condition -		Only evaluate this rule if condition is true.

			internal SwitchInfo(Regex Switch, String TypeSpec, String XMLName, String XMLResultString, bool SetOnce, bool MatchOnce, string Condition)
			{
				this.Switch = Switch;
				this.TypeSpec = TypeSpec;
				this.XMLName = XMLName;
				this.XMLResultString = XMLResultString;
				this.SetOnce = SetOnce;
				this.MatchOnce = MatchOnce;
				this.Condition = Condition;
			}

			internal Regex Switch { get; set; }

			internal String XMLName { get; set; }

			internal String TypeSpec { get; set; }

			internal String XMLResultString { get; set; }

			internal bool SetOnce { get; set; }

			internal bool MatchOnce { get; set; }

			internal String Condition { get; set; }
		}

		internal class ParseDirectives
		{
			internal List<SwitchInfo> General = new List<SwitchInfo>();
			internal List<SwitchInfo> Cc = new List<SwitchInfo>();
			internal List<SwitchInfo> Masm = new List<SwitchInfo>();
			internal List<SwitchInfo> Link = new List<SwitchInfo>();
			internal List<SwitchInfo> Lib = new List<SwitchInfo>();
			internal List<SwitchInfo> Ximg = new List<SwitchInfo>();
			internal List<SwitchInfo> Midl = new List<SwitchInfo>();
			internal List<SwitchInfo> FxCompile = new List<SwitchInfo>();
			internal List<SwitchInfo> WavePsslc = new List<SwitchInfo>();
		}

		internal static CharacterSetTypes GetCharacterSetFromDefines(IEnumerable<string> defines)
		{
			return (defines != null && null != defines.FirstOrDefault(def => def == "UNICODE" | def == "_UNICODE")) ? CharacterSetTypes.Unicode : CharacterSetTypes.MultiByte;
		}

		internal static ParseDirectives GetParseDirectives(IModule module)
		{
			return GetParseDirectives(module.Configuration.System, module.Configuration.Compiler, module.Package.Project);
		}

		internal static ParseDirectives GetParseDirectives(string platform, string compiler, Project project)
		{
			string key = platform + "#" + compiler;

			var directives = _parseDirectiveMap.GetOrAdd(key, k => CreateParseDirectives(platform, compiler, project));
			
			return directives;
		}

		#region Private

		private static ParseDirectives CreateParseDirectives(string platform, string compiler, Project project)
		{
			ParseDirectives parse = new ParseDirectives();

			GetRulesAndAdd(project, parse.General, "/rules/general/rule");

			GetRulesAndAdd(project, parse.Cc, "/rules/compiler/" + compiler + "/rule", "/rules/compiler/" + compiler + "/" + platform + "/rule");
			GetRulesAndAdd(project, parse.Cc, "/rules/compiler/" + platform + "/rule", "/rules/compiler/" + platform + "/" + compiler + "/rule");

			GetRulesAndAdd(project, parse.Masm, "/rules/masm/" + compiler + "/rule", "/rules/masm/" + compiler + "/" + platform + "/rule");
			GetRulesAndAdd(project, parse.Masm, "/rules/masm/" + platform + "/rule", "/rules/masm/" + platform + "/" + compiler + "/rule");

			GetRulesAndAdd(project, parse.Link, "/rules/linker/" + compiler + "/rule", "/rules/linker/" + compiler + "/" + platform + "/rule");
			GetRulesAndAdd(project, parse.Link, "/rules/linker/" + platform + "/rule", "/rules/linker/" + platform + "/" + compiler + "/rule");

			GetRulesAndAdd(project, parse.Lib, "/rules/librarian/" + compiler + "/rule", "/rules/librarian/" + compiler + "/" + platform + "/rule");
			GetRulesAndAdd(project, parse.Lib, "/rules/librarian/" + platform + "/rule", "/rules/librarian/" + platform + "/" + compiler + "/rule");

			GetRulesAndAdd(project, parse.Ximg, "/rules/imagebuilder/" + compiler + "/rule", "/rules/imagebuilder/" + compiler + "/" + platform + "/rule");
			GetRulesAndAdd(project, parse.Ximg, "/rules/imagebuilder/" + platform + "/rule", "/rules/imagebuilder/" + platform + "/" + compiler + "/rule");

			GetRulesAndAdd(project, parse.Midl, "/rules/midl/" + compiler + "/rule", "/rules/midl/" + compiler + "/" + platform + "/rule");
			GetRulesAndAdd(project, parse.Midl, "/rules/midl/" + platform + "/rule", "/rules/midl/" + platform + "/" + compiler + "/rule");

			GetRulesAndAdd(project, parse.FxCompile, "/rules/fxcompile/" + compiler + "/rule", "/rules/fxcompile/" + compiler + "/" + platform + "/rule");
			GetRulesAndAdd(project, parse.FxCompile, "/rules/fxcompile/" + platform + "/rule", "/rules/fxcompile/" + platform + "/" + compiler + "/rule");

			GetRulesAndAdd(project, parse.WavePsslc, "/rules/psslc/" + compiler + "/rule", "/rules/psslc/" + compiler + "/" + platform + "/rule");
			GetRulesAndAdd(project, parse.WavePsslc, "/rules/psslc/" + platform + "/rule", "/rules/psslc/" + platform + "/" + compiler + "/rule");

			return parse;
		}

		private static IEnumerable<XmlNodeList> GetPlatformSpecificAndGenericLists(Project project, string rulePath, string specificRulePath)
		{
			XmlNodeList specificRuleNodes = null;
			XmlNodeList genericRuleNodes = null;

			XmlDocument specificDoc = GetPlatformOptionsXmlDoc(project);

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
			string ruleCondition;
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

						ruleCondition = string.Empty;
						
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
							roptions |= RegexOptions.IgnoreCase;
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

						if (oNodeList.Item(x).Attributes["condition"] != null)
							ruleCondition = oNodeList.Item(x).Attributes["condition"].InnerText;

                        /* compiling these regexes is faster but when doing a large number of compiled regexs in parallel
                        (can happen with many projects and certain parallel execution constraints) it seems like the 
                        compiler instance is shared under the hood and can crash in native underlying code when accessed
                        rapidly in parallel */
                        // roptions |= RegexOptions.Compiled;

						parseDirectives.Add(new SwitchInfo(new Regex(regex, roptions), typespec, xmlname, xmlvalue, bSetOnce, bMatchOnce, ruleCondition));
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
						XmlDocument temp_xmlDoc = new XmlDocument();

						string dataPath = Path.GetFullPath(Path.Combine(PackageMap.Instance.GetFrameworkRelease().Path, @"data\options_msbuild.xml"));

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
				bool explictPath = true;
				string optionFilename = project.Properties.GetPropertyOrDefault("visualstudio.msbuild-options.file", null);
				if (optionFilename == null)
				{
					explictPath = false;
					optionFilename = "options_msbuild.xml";
				}

				string platformConfigPackage = project.Properties["nant.platform-config-package-name"].TrimWhiteSpace();
				if (!string.IsNullOrEmpty(platformConfigPackage))
				{
					return _platformDocs.GetOrAdd(platformConfigPackage, platformConfigPackageKey =>
					{				
						string dataDir = Path.GetFullPath(Path.Combine(project.Properties["package." + platformConfigPackageKey + ".dir"], "config", "data"));
						string dataPath = Path.Combine(dataDir, optionFilename);
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
						else if (explictPath)
						{
							throw new BuildException($"Cannot find MSBuild options file '{optionFilename}' specified by 'visualstudio.msbuild-options.file' property. " 
								+ $"File must be located in the config package config\\data directory: {dataPath}.");
						}
						return null;
					});
				}
				else if (explictPath)
				{
					throw new BuildException($"Property 'visualstudio.msbuild-options.file' has value '{optionFilename}' but this platform does not use a specific platform config package. "
						+ $"Options should be merged into Framework's 'data\\options_msbuild.xml' file.");
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
