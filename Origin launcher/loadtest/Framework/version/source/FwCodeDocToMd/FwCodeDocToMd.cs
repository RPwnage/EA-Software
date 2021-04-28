// Copyright (C) 2019 Electronic Arts Inc.
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
using System.Linq;
using System.IO;
using System.Reflection;
using System.Text;
using System.Xml.Linq;
using System.Diagnostics;

namespace EA.Tools
{
    public static class HelperFunctions
    {
        public static Guid MakeDocGUIDfromString(string inS)
        {
            const int GUID_BYTE_NUM = 16;

            byte[] buffer = Encoding.UTF8.GetBytes(inS.ToLower());

            // MD5 is the only algo which generates 128bit (16B) hashes, just the size of a GUID
            System.Security.Cryptography.MD5 md5 = new System.Security.Cryptography.MD5CryptoServiceProvider();
            byte[] hash = md5.ComputeHash(buffer);
            if (md5.HashSize != GUID_BYTE_NUM * 8)
            {
                string msg = String.Format("MD5 hash should have exactly {0} bits ({1} bytes)", GUID_BYTE_NUM, GUID_BYTE_NUM * 8);
                throw new Exception(msg);
            }

            const char DASH_CHAR = '-';
            // Create a GUID-string representation of the hash code bytes
            // A GUID has the format: {00010203-0405-0607-0809-101112131415}
            System.Text.StringBuilder guidstr = new System.Text.StringBuilder(GUID_BYTE_NUM + 4);
            guidstr.Append('{');

            // Append each byte as a 2 character upper case hex string.
            guidstr.Append(NAnt.Core.Util.Hash.BytesToHex(hash, 0, 4));
            guidstr.Append(DASH_CHAR);
            for (int i = 0; i < 3; i++)
            {
                guidstr.Append(NAnt.Core.Util.Hash.BytesToHex(hash, 4 + i * 2, 2));
                guidstr.Append(DASH_CHAR);
            }
            guidstr.Append(NAnt.Core.Util.Hash.BytesToHex(hash, 10, 6));
            guidstr.Append('}');
            Guid guid = new Guid(guidstr.ToString());

            return guid;
        }

        public static string MakeValidFileName(string name)
        {
            string invalidChars = new string(System.IO.Path.GetInvalidPathChars());
            invalidChars = invalidChars + "#(), ";
            invalidChars = System.Text.RegularExpressions.Regex.Escape(invalidChars);
            string invalidRegStr = string.Format(@"([{0}]*\.+$)|([{0}]+)", invalidChars);

            return System.Text.RegularExpressions.Regex.Replace(name, invalidRegStr, "-").ToLower().Trim('-');
        }

        public static void WriteToFile(string path, string outText)
        {
            string outfile = HelperFunctions.MakeValidFileName(path);
            string dirPath = Path.GetDirectoryName(outfile);
            if (!string.IsNullOrWhiteSpace(dirPath) && !Directory.Exists(dirPath))
            {
                Directory.CreateDirectory(dirPath);
            }

            // Actually write out the file.
            FileInfo outfileinfo = new FileInfo(outfile);
            if (outfileinfo.Exists)
            {
                outfileinfo.IsReadOnly = false;
            }
            File.WriteAllText(outfile, outText);
        }

        static public string GetAttributeOrDefault(XElement elem, string attributeName, string defaultValue)
        {
            if (elem == null || elem.Attribute(attributeName) == null)
            {
                return defaultValue;
            }

            return elem.Attribute(attributeName).Value;
        }

        public static XAttribute AttributeAnyNS(this XElement source, string localName)
        {
            var attr = source.AttributesAnyNS(localName);
            return attr.Count() != 0 ? attr.First() : null;
        }

        public static XElement ElementAnyNS(this XElement source, string localName)
        {
            var elements = source.ElementsAnyNS(localName);
            return elements.Count() != 0 ? elements.First() : null;
        }

        public static XElement DescendantAnyNS(this XElement source, string localName)
        {
            var elements = source.DescendantsAnyNS(localName);
            return elements.Count() != 0 ? elements.First() : null;
        }

        public static IEnumerable<XAttribute> AttributesAnyNS(this XElement source, string localName)
        {
            return source.Attributes().Where(e => e.Name.LocalName == localName);
        }

        public static IEnumerable<XElement> ElementsAnyNS(this XElement source, string localName)
        {
            return source.Elements().Where(e => e.Name.LocalName == localName);
        }

        public static IEnumerable<XElement> DescendantsAnyNS(this XElement source, string localName)
        {
            return source.Descendants().Where(e => e.Name.LocalName == localName);
        }
    }
	public class MarkdownGroup
	{
		public string mPath;
		public List<string> mIncludeFilters = new List<string>();
		public List<string> mExcludeFilters = new List<string>();
	}

	class Program
	{
		// Input data
		static string sOutputRootDir;
		static string sYamlConfigFile;
		static List<string> sVsXmlFiles = new List<string>();
        static string sMapFileOut;

        // List of markdown files to create based on info from the input config file.
        static Dictionary<string, List<MarkdownGroup>> sMarkdownGroups = new Dictionary<string, List<MarkdownGroup>>();

		static int Main(string[] args)
		{
			int ret = 0;

			try
			{
				ParseCommandLine(args);
				ProcessYamlConfig();

				CreateTasksMarkdown(sVsXmlFiles, sMarkdownGroups["tasks"]);
                CreateFunctionsMarkdown(sVsXmlFiles, sMarkdownGroups["functions"]);
			}
			catch (Exception ex)
			{
				Console.WriteLine("Error: '{0}'", ex.Message);
				ret = 1;
			}

			return ret;
		}

		private static void ParseCommandLine(string[] args)
		{
			string currentInput = null;
			foreach (var arg in args)
			{
				switch (arg)
				{
					case "-config":
						// Input yaml config file specifing the .md fileset to generate and what classes to generate for.
						currentInput = "config";
						break;

					case "-xml":
						// Path to the generated NAnt.Core.xml, etc files during regular Framework build.  Use multiple times,
						// one for each file.
						currentInput = "xml";
						break;

					case "-o":
						// Output root folder.
						currentInput = "o";
						break;

                    case "-mapFileOut":
                        // Output root folder.
                        currentInput = "mapFileOut";
                        break;

                    default:
						{
							bool processed = false;
							switch (currentInput)
							{
								case "config":
									sYamlConfigFile = arg;
									processed = true;
									currentInput = null;
									break;
								case "xml":
									sVsXmlFiles.Add(arg);
									processed = true;
									currentInput = null;
									break;
								case "o":
									sOutputRootDir = arg;
									processed = true;
									currentInput = null;
									break;
                                case "mapFileOut":
                                    sMapFileOut = arg;
                                    processed = true;
                                    currentInput = null;
                                    break;
                                default:
									processed = false;
									break;
							}
							if (!processed)
							{
								throw new ArgumentException("Unexpected commandline argument.", arg);
							}
						}
						break;
				}
			}
		}

		private static void ProcessYamlConfig()
		{
			YamlDotNet.RepresentationModel.YamlStream yamlSR = new YamlDotNet.RepresentationModel.YamlStream();
			using (StreamReader sr = new StreamReader(sYamlConfigFile))
			{
				yamlSR.Load(sr);
			}
			YamlDotNet.RepresentationModel.YamlDocument yamlDoc = yamlSR.Documents.First();
			YamlDotNet.RepresentationModel.YamlMappingNode mapping = (YamlDotNet.RepresentationModel.YamlMappingNode)yamlDoc.RootNode;
			foreach (var section in mapping.Children)
			{

				//if (section.Key.ToString() == "tasks")
				{
					if (section.Value.NodeType != YamlDotNet.RepresentationModel.YamlNodeType.Sequence)
					{
						continue;
					}
					YamlDotNet.RepresentationModel.YamlSequenceNode taskItems = (YamlDotNet.RepresentationModel.YamlSequenceNode)section.Value;
					foreach (var taskItem in taskItems.Children)
					{
						if (taskItem.NodeType != YamlDotNet.RepresentationModel.YamlNodeType.Mapping)
						{
							continue;
						}
						MarkdownGroup mdFile = new MarkdownGroup();
						foreach (var node in ((YamlDotNet.RepresentationModel.YamlMappingNode)taskItem).Children)
						{
							if (node.Key.ToString() == "path" && node.Value.NodeType == YamlDotNet.RepresentationModel.YamlNodeType.Scalar)
							{
								mdFile.mPath = node.Value.ToString();
							}
							else if (node.Key.ToString() == "includes" && node.Value.NodeType == YamlDotNet.RepresentationModel.YamlNodeType.Sequence)
							{
								mdFile.mIncludeFilters.AddRange(((YamlDotNet.RepresentationModel.YamlSequenceNode)node.Value).Select(v => v.ToString()).ToList());
							}
							else if (node.Key.ToString() == "excludes" && node.Value.NodeType == YamlDotNet.RepresentationModel.YamlNodeType.Sequence)
							{
								mdFile.mExcludeFilters.AddRange(((YamlDotNet.RepresentationModel.YamlSequenceNode)node.Value).Select(v => v.ToString()).ToList());
							}
						}
						if (!String.IsNullOrEmpty(mdFile.mPath) && (mdFile.mIncludeFilters.Any()))
						{
                            List<MarkdownGroup> groups = null;
                            if (!sMarkdownGroups.TryGetValue(section.Key.ToString(), out groups))
                            {
                                groups = new List<MarkdownGroup>();
                                sMarkdownGroups.Add(section.Key.ToString(), groups);
                            }
                            groups.Add(mdFile);
						}
					}
				}
			}
		}

        private static void CreateFunctionsMarkdown(List<string> inputXmlFiles, List<MarkdownGroup> mdGroups)
        {
            Dictionary<string, XDocument> assemblyInfos;
            List<Type> allTypes;
            LoadAssemblies(out assemblyInfos, out allTypes);

            // Now create markdown file based on the filter info parsed from the config file.
            int curWeight = 1000; // start at higher weight so it's below other content
            foreach (MarkdownGroup mdGroup in mdGroups)
            {
                List<Type> typesToMd = GetTypesForMdGroup(allTypes, mdGroup);
                // Now write out these types to the mark down file.
                foreach (Type ty in typesToMd)
                {
                    NAnt.Core.Attributes.FunctionClassAttribute typeAttrib = ty.GetCustomAttribute<NAnt.Core.Attributes.FunctionClassAttribute>();
                    if (typeAttrib == null)
                        continue;

                    XElement codeDocXmlNode = GetCodeDocXmlNode(assemblyInfos, ty);
                    Debug.Assert(!string.IsNullOrEmpty(ty.FullName));

                    System.Text.StringBuilder markdownContent = new System.Text.StringBuilder();
                    //var url = HelperFunctions.MakeValidFileName(Path.Combine(mdGroup.mPath, typeAttrib.FriendlyName + "-function") + ".md");
                    //url = url.Replace('\\', '/');
                    markdownContent.AppendLine(string.Format("---\ntitle: {0}\nweight: {1}\n---", typeAttrib.FriendlyName, curWeight++));
                    //taskTomlContent.AppendLine($"\"{ty.FullName}\" = {{ url = \"{url}\", text = \"<{typeAttrib.Name}>\" }}");
                    //Add the followings if they show up in the XML.
                    if (codeDocXmlNode != null)
                    {
                        var codeDocMd = FwCodeDocToMd.XmlToMarkdown.ToMarkDown(codeDocXmlNode);
                        markdownContent.AppendLine(codeDocMd);
                    }

                    // Create function overview
                    markdownContent.AppendLine("## Overview ##");

                    markdownContent.AppendLine("| Function | Summary |");
                    markdownContent.AppendLine("| -------- | ------- |");
                    foreach (var methodInfo in ty.GetMethods())
                    {
                        if (!methodInfo.GetCustomAttributes<NAnt.Core.Attributes.FunctionAttribute>().Any())
                            continue; // Not an exposed function

                        string methodSignature;
                        XElement methodXmlNode;
                        GetMethodInfo(assemblyInfos, ty, methodInfo, out methodSignature, out methodXmlNode);
                        //Add the description if they show up in the XML.
                        var description = "";
                        if (methodXmlNode != null && methodXmlNode.ElementAnyNS("summary") != null)
                        {
                            description = FwCodeDocToMd.XmlToMarkdown.ToMarkDown(methodXmlNode.ElementAnyNS("summary"), includeSelf: false); ;
                            // Replace line breaks as we'll use this in a table
                            description = description.Trim().Replace("\r\n", "<br>").Replace("\n", "<br>");
                        }

                        var anchorValue = HelperFunctions.MakeValidFileName(methodSignature);

                        // Write out
                        markdownContent.AppendLine($"| [{methodSignature}]({{{{< relref \"#{anchorValue}\" >}}}}) | {description} |");
                    }

                    // Create function details
                    markdownContent.AppendLine("## Detailed overview ##");

                    foreach (var methodInfo in ty.GetMethods())
                    {
                        if (!methodInfo.GetCustomAttributes<NAnt.Core.Attributes.FunctionAttribute>().Any())
                            continue; // Not an exposed function

                        string methodSignature;
                        XElement methodXmlNode;
                        GetMethodInfo(assemblyInfos, ty, methodInfo, out methodSignature, out methodXmlNode);

                        if (methodXmlNode != null)
                        {
                            markdownContent.AppendLine($"#### {methodSignature} ####");
                            var methodDocs = FwCodeDocToMd.XmlToMarkdown.ToMarkDown(methodXmlNode, includeSelf: false, baseHeading: 3);
                            markdownContent.AppendLine(methodDocs);
                            markdownContent.AppendLine();
                            markdownContent.AppendLine();
                        }
                    }


                    // Write out the file.
                    HelperFunctions.WriteToFile(Path.Combine(sOutputRootDir, "content", mdGroup.mPath, typeAttrib.FriendlyName) + ".md", markdownContent.ToString());
                }
            }
        }

        private static void GetMethodInfo(Dictionary<string, XDocument> assemblyInfos, Type ty, MethodInfo methodInfo, out string methodSignature, out XElement methodXmlNode)
        {
            // Construct Name of function
            var returnType = methodInfo.ReturnType.Name;
            var funcName = methodInfo.Name;
            // Skip the first parameter as it is always "Project", which is not required to add by the user
            var parameters = methodInfo.GetParameters().Skip(1);

            string parameterString = "";
            foreach (var param in parameters)
            {
                var prefix = string.IsNullOrEmpty(parameterString) ? "" : ",";
                parameterString = parameterString + $"{prefix}{param.ParameterType.Name} {param.Name}";
            }

            methodSignature = $"{returnType} {funcName}({parameterString})";

            // Now look up the description in the xml
            methodXmlNode = GetCodeDocXmlNode(assemblyInfos, ty, methodInfo);
        }

        private static void CreateTasksMarkdown(List<string> inputXmlFiles, List<MarkdownGroup> mdGroups)
        {
            Dictionary<string, XDocument> assemblyInfos;
            List<Type> allTypes;
            LoadAssemblies(out assemblyInfos, out allTypes);

            // Collect all tasks in a task.toml so we can use shortcode in hugo to reference them
            System.Text.StringBuilder taskTomlContent = new System.Text.StringBuilder();
            // Collect all tasks in a translation dictionary to be able to covert the old documentation to md.
            var taskMap = new Dictionary<string, string>();

            Debug.Assert(HelperFunctions.MakeDocGUIDfromString("NAnt.Core.Tasks.EchoTask").ToString().Equals("9ab1a190-099a-ee3b-7fe4-5a23683017c7"));

            // Now create markdown file based on the filter info parsed from the config file.
            int curWeight = 1000; // start at higher weight so it's below other content
            foreach (MarkdownGroup mdGroup in mdGroups)
            {
                System.Text.StringBuilder markdownContent = new System.Text.StringBuilder();
                string filebasename = Path.GetFileName(mdGroup.mPath);
                markdownContent.AppendLine(string.Format("---\ntitle: {0}\nweight: {1}\n---", filebasename, curWeight++));

                Queue<Type> typesToMd = new Queue<Type>();
                foreach (var type in GetTypesForMdGroup(allTypes, mdGroup))
                {
                    typesToMd.Enqueue(type);
                }
                HashSet<Type> visited = new HashSet<Type>();

                // Now write out these types to the mark down file.
                while (typesToMd.Count > 0)
                {
                    var ty = typesToMd.Dequeue();
                    visited.Add(ty);

                    string publicName = null;
                    NAnt.Core.Attributes.TaskNameAttribute typeAttrib = ty.GetCustomAttribute<NAnt.Core.Attributes.TaskNameAttribute>();

                    if (typeAttrib != null)
                    {
                        publicName = typeAttrib.Name;
                    }
                    else if (ty.GetCustomAttributes<NAnt.Core.Attributes.ElementNameAttribute>().Any())
                    {
                        publicName = ty.Name;
                    }
                    else
                        continue;


                    XElement codeDocXmlNode = GetCodeDocXmlNode(assemblyInfos, ty);

                    Debug.Assert(!string.IsNullOrEmpty(ty.FullName));

                    var FullName = ty.FullName;

                    var key = HelperFunctions.MakeDocGUIDfromString(ty.FullName).ToString();
                    if (!taskMap.ContainsKey(key))
                        taskMap.Add(key, ty.FullName);

                    // Gather all the info we need
                    // All attributes and nested elements of this task
                    var attribSummaryList = new List<PropertyInfo>();
                    var nestedSummaryList = new List<PropertyInfo>();
                    var attribFullList = new List<PropertyInfo>();
                    var nestedFullList = new List<PropertyInfo>();
                    var properties = ty.GetProperties(BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.Instance);
                    //var xmlElementAttributes = GetXmlElementAttributes(ty);

                    // For short summary, just go down two levels max
                    List<Type> shortSummaryIncludeTypes = new List<Type>() { ty };
                    if (ty.BaseType != null)
                    {
                        shortSummaryIncludeTypes.Add(ty.BaseType);
                        if (ty.BaseType.BaseType != null)
                        {
                            shortSummaryIncludeTypes.Add(ty.BaseType.BaseType);
                        }
                    }

                    foreach (var propInfo in ty.GetProperties())
                    {
                        bool isAttrib = false;
                        bool isNested = false;
                        if (propInfo.GetCustomAttributes<NAnt.Core.Attributes.TaskAttributeAttribute>().Any())
                        {
                            attribFullList.Add(propInfo);
                            isAttrib = true;
                        }
                        else if (propInfo.GetCustomAttributes<NAnt.Core.Attributes.BuildElementAttribute>().Any()
                            || propInfo.GetCustomAttributes<NAnt.Core.Attributes.BuildAttributeAttribute>().Any()
                            || propInfo.GetCustomAttributes<NAnt.Core.Attributes.XmlElementAttribute>().Any()
                            )
                        {
                            nestedFullList.Add(propInfo);
                            isNested = true;
                        }

                        if (shortSummaryIncludeTypes.Contains(propInfo.DeclaringType))
                        {
                            if (isAttrib)
                            {
                                attribSummaryList.Add(propInfo);
                            }
                            else if (isNested)
                            {
                                nestedSummaryList.Add(propInfo);
                            }
                        }
                    }

                    // Start writing out

                    System.Text.StringBuilder taskMarkdownContent = new System.Text.StringBuilder();
                    var filePath = typeAttrib != null ?
                        HelperFunctions.MakeValidFileName(Path.Combine(mdGroup.mPath, publicName + "-task") + ".md")
                        : HelperFunctions.MakeValidFileName(Path.Combine("reference\\other", publicName) + ".md");

                    var url = filePath.Replace('\\', '/');
                    if (typeAttrib != null)
                    {
                        taskMarkdownContent.AppendLine(string.Format("---\ntitle: {0}\nweight: {1}\n---", "< " + publicName + " > Task", curWeight++));
                        taskTomlContent.AppendLine($"\"{ty.FullName}\" = {{ url = \"{url}\", text = \"<{publicName}>\" }}");
                    }
                    else
                    {
                        taskMarkdownContent.AppendLine(string.Format("---\ntitle: {0}\nweight: {1}\n---", publicName, curWeight++));
                        taskTomlContent.AppendLine($"\"{ty.FullName}\" = {{ url = \"{url}\", text = \"{publicName}\" }}");
                    }

                    // Construct this block directly from type info
                    BuildSyntaxSection(taskMarkdownContent, publicName, ty, attribSummaryList, nestedSummaryList, false);

                    //Add the followings if they show up in the XML.
                    if (codeDocXmlNode != null)
                    {
                        var codeDocMd = FwCodeDocToMd.XmlToMarkdown.ToMarkDown(codeDocXmlNode);
                        taskMarkdownContent.AppendLine(codeDocMd);
                    }

                    // Construct this block directly from type info and xmldoc info
                    // Mainly setup a table with columns: Attribute, Description, Type, Required
                    if (attribFullList.Any())
                    {
                        taskMarkdownContent.AppendLine("### Attributes");
                        taskMarkdownContent.AppendLine("| Attribute | Description | Type | Required |");
                        taskMarkdownContent.AppendLine("| --------- | ----------- | ---- | -------- |");
                        foreach (System.Reflection.PropertyInfo tyProp in attribFullList)
                        {
                            NAnt.Core.Attributes.TaskAttributeAttribute taskAttrib = tyProp.GetCustomAttribute<NAnt.Core.Attributes.TaskAttributeAttribute>();
                            string attributeName = taskAttrib.Name;
                            string attributeDesc = FwCodeDocToMd.XmlToMarkdown.MakeListSafe(GetAttributeDesc(assemblyInfos, tyProp.DeclaringType, tyProp));
                            string attributeType = tyProp.PropertyType.Name;
                            string attributeRequired = taskAttrib.Required.ToString();
                            taskMarkdownContent.AppendLine(String.Format("| {0} | {1} | {2} | {3} |", attributeName, attributeDesc, attributeType, attributeRequired));
                        }
                        taskMarkdownContent.AppendLine();
                    }

                    // Construct this block directly from type info and xmldoc info
                    if (nestedFullList.Any())
                    {
                        taskMarkdownContent.AppendLine("### Nested Elements");
                        taskMarkdownContent.AppendLine("| Name | Description | Type | Required |");
                        taskMarkdownContent.AppendLine("| ---- | ----------- | ---- | -------- |");

                        foreach (var tyProp in nestedFullList)
                        {
                            var buildElementAttribute = tyProp.GetCustomAttribute<NAnt.Core.Attributes.BuildElementAttribute>();
                            var buildAttributeAttribute = tyProp.GetCustomAttribute<NAnt.Core.Attributes.BuildAttributeAttribute>();

                            string attributeName = "", attributeDesc = "", attributeType = "", attributeRequired = "";

                            if (buildElementAttribute != null)
                            {
                                attributeName = buildElementAttribute.Name;
                                attributeDesc = FwCodeDocToMd.XmlToMarkdown.MakeListSafe(GetAttributeDesc(assemblyInfos, tyProp.DeclaringType, tyProp));
                                attributeType = tyProp.PropertyType.FullName;
                                attributeRequired = buildElementAttribute.Required.ToString();
                            }
                            else if (buildAttributeAttribute != null)
                            {
                                attributeName = buildAttributeAttribute.Name;
                                attributeDesc = FwCodeDocToMd.XmlToMarkdown.MakeListSafe(GetAttributeDesc(assemblyInfos, tyProp.DeclaringType, tyProp));
                                attributeType = tyProp.PropertyType.Name;
                                attributeRequired = buildAttributeAttribute.Required.ToString();
                            }
                            if (!visited.Contains(tyProp.GetType()) && !typesToMd.Contains(tyProp.GetType()) && !tyProp.GetType().IsAbstract )
                            {
                                typesToMd.Enqueue(tyProp.GetType());
                            }
                            taskMarkdownContent.AppendLine(String.Format("| {{{{< task \"{2}\" \"{0}\" >}}}}| {1} | {{{{< task \"{2}\" >}}}} | {3} |", attributeName, attributeDesc, attributeType, attributeRequired));
                        }

                        taskMarkdownContent.AppendLine();
                    }


                    // Construct this block directly from type info
                    BuildSyntaxSection(taskMarkdownContent, publicName, ty, attribFullList, nestedFullList, true);

                   
                    // Shoe examples (from xmldoc info) if available
                    if (codeDocXmlNode != null)
                    {
                        //foreach (System.Xml.XmlNode exampleNode in codeDocXmlNode.SelectNodes("example"))
                        //{
                        //	if (exampleNode != null)
                        //	{
                        //                          taskMarkdownContent.AppendLine("### Example");
                        //                          taskMarkdownContent.AppendLine();
                        //	}
                        //}
                    }

                    // Write out the file.
                    HelperFunctions.WriteToFile(Path.Combine(sOutputRootDir, "content", filePath), taskMarkdownContent.ToString());
                }

                HelperFunctions.WriteToFile(Path.Combine(sOutputRootDir, "content", mdGroup.mPath, "_index.md"), markdownContent.ToString());
            }

            HelperFunctions.WriteToFile(Path.Combine(sOutputRootDir, "content", "reference", "other", "_index.md"), string.Format("---\ntitle: Other Types\nweight: {0}\n---", curWeight++));

            HelperFunctions.WriteToFile(Path.Combine(sOutputRootDir, "data", "tasks.toml"), taskTomlContent.ToString());

            // Temp write out a dictionary so we can translate existing links for documentation
            if (!string.IsNullOrEmpty(sMapFileOut))
            {
                System.Text.StringBuilder taskMapContent = new System.Text.StringBuilder();
                XDocument doc = new XDocument(
                    new XElement("Root")
                );

                foreach (var task in taskMap)
                {
                    var elem = new XElement("Task");
                    elem.Add(new XAttribute("id", task.Key), new XAttribute("name", task.Value));

                    doc.Element("Root").Add(elem);
                }

                HelperFunctions.WriteToFile(sMapFileOut, doc.ToString());
            }
        }

        private static List<Type> GetTypesForMdGroup(List<Type> allTypes, MarkdownGroup mdGroup)
        {
            // Get all tasks includes for this files.
            List<Type> typesToMd = new List<Type>();
            foreach (string incPattern in mdGroup.mIncludeFilters)
            {
                if (incPattern.EndsWith("*") && incPattern.Length > 1)
                {
                    // If we are including with a pattern, check of explicit exlcude before adding to the list
                    List<Type> incTypes = allTypes.Where(
                        t => t.FullName.StartsWith(incPattern.Substring(0, incPattern.Length - 1))
                        ).ToList();
                    if (incTypes.Any())
                    {
                        if (mdGroup.mExcludeFilters != null && mdGroup.mExcludeFilters.Any())
                        {
                            foreach (string exPattern in mdGroup.mExcludeFilters)
                            {
                                if (exPattern.EndsWith("*") && exPattern.Length > 1)
                                {
                                    List<Type> typesToRemove = incTypes.Where(t => t.FullName.StartsWith(exPattern.Substring(0, exPattern.Length - 1))).ToList();
                                    foreach (Type ty in typesToRemove)
                                    {
                                        incTypes.Remove(ty);
                                    }
                                }
                                else
                                {
                                    List<Type> typesToRemove = incTypes.Where(t => t.FullName == exPattern).ToList();
                                    foreach (Type ty in typesToRemove)
                                    {
                                        incTypes.Remove(ty);
                                    }
                                }
                            }
                        }
                        if (incTypes.Any())
                        {
                            typesToMd.AddRange(incTypes);
                        }
                    }
                }
                else
                {
                    Type matchedType = allTypes.Find(ty => ty.FullName == incPattern);
                    if (matchedType != null)
                    {
                        typesToMd.Add(matchedType);
                    }
                }
            }

            return typesToMd;
        }

        private static void LoadAssemblies(out Dictionary<string, XDocument> assemblyInfos, out List<Type> allTypes)
        {
            // Load all xml files (& the associated assemblies) to get all types information first.
            string currPath = Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location);
            assemblyInfos = new Dictionary<string, XDocument>();
            allTypes = new List<Type>();
            foreach (string xmlfile in sVsXmlFiles)
            {
                XDocument xmlDoc = XDocument.Load(xmlfile);

                // Get and load the assembly
                var assemblyNode = xmlDoc.Root.DescendantAnyNS("assembly");
                if (assemblyNode == null)
                {
                    throw new Exception("Unable to extract //doc/assembly/name info from XML file: " + xmlfile);
                }
                string assemblyFile = Path.Combine(currPath, assemblyNode.ElementAnyNS("name").Value + ".dll");
                Assembly assm = Assembly.LoadFile(assemblyFile);

                assemblyInfos.Add(assm.Location, xmlDoc);

                allTypes.AddRange(assm.GetTypes());
            }
        }

        //private static List<NAnt.Core.Attributes.XmlElementAttribute> GetXmlElementAttributes(Type ty)
        //{
        //    var fullList = new List<NAnt.Core.Attributes.XmlElementAttribute>();

        //    // Fetch all the XmlElementAttributes by traversing the methods and checking their custom attributes
        //    var methods = ty.GetMethods();
        //    foreach (var method in methods)
        //    {
        //        var customAttributes = method.GetCustomAttributes<NAnt.Core.Attributes.XmlElementAttribute>();
        //        fullList.AddRange(customAttributes);
        //    }

        //    return fullList;
        //}

        private static List<PropertyInfo> GetBuildAttributes(Type ty)
        {
            var fullList = new List<PropertyInfo>();

            // Fetch all the BuildElementAttribute by traversing the methods and checking their custom attributes
            foreach (var property in ty.GetProperties())
            {
                if (property.GetCustomAttributes<NAnt.Core.Attributes.BuildElementAttribute>().Any()
                    || property.GetCustomAttributes<NAnt.Core.Attributes.BuildAttributeAttribute>().Any()
                    )
                {
                    fullList.Add(property);
                }
            }

            return fullList;
        }

        private static string GetAttributeDesc(Dictionary<string, XDocument> assemblyInfos, Type ty, System.Reflection.PropertyInfo tyProp)
        {
            var codeDocXmlNode = GetCodeDocXmlNode(assemblyInfos, ty, tyProp);
            if (codeDocXmlNode == null)
                return "";

            // Just parse the content of the summary block 
            var summaryBlock = codeDocXmlNode.ElementAnyNS("summary");
            Debug.Assert(summaryBlock != null);


            return FwCodeDocToMd.XmlToMarkdown.ToMarkDown(summaryBlock, includeSelf: false);
        }

        private static XElement GetCodeDocXmlNode(Dictionary<string, XDocument> assemblyInfos, Type ty, object subType = null)
        {
            // Get the XML code doc info to see if the info exist
            Assembly assm = Assembly.GetAssembly(ty);
            string nameAttribValue = null;
            if (subType == null)
                nameAttribValue = $"T:{ty.FullName}";
            else
            {
                if (subType is System.Reflection.PropertyInfo)
                    nameAttribValue = $"P:{ty.FullName}.{(subType as System.Reflection.PropertyInfo).Name}";
                else if (subType is System.Reflection.MethodInfo)
                    nameAttribValue = $"M:{ty.FullName}.{(subType as System.Reflection.MethodInfo).Name}";
                else
                    throw new NotImplementedException();
            }
                
            if (assm != null && assemblyInfos.ContainsKey(assm.Location))
            {
                // Try with exact name first
                foreach (XElement node in assemblyInfos[assm.Location].Root.DescendantsAnyNS("member"))
                {
                    var nameAttribute = node.AttributeAnyNS("name");
                    if (nameAttribute != null && nameAttribute.Value == nameAttribValue)
                    {
                        return node;
                    }
                }

                // Try partial match
                foreach (XElement node in assemblyInfos[assm.Location].Root.DescendantsAnyNS("member"))
                {
                    var nameAttribute = node.AttributeAnyNS("name");
                    if (nameAttribute != null && nameAttribute.Value.Contains(nameAttribValue))
                    {
                        return node;
                    }
                }
            }

            return null;
        }

        private static void BuildSyntaxSection(System.Text.StringBuilder markdownContent, string name, Type ty, List<PropertyInfo> attribList, List<PropertyInfo> nestedList, bool isFullSyntax)
        {
            if (isFullSyntax)
                markdownContent.AppendLine("## Full Syntax");
            else
                markdownContent.AppendLine("## Syntax");
            
            XElement doc = BuildTaskNameAttributeSyntax(name, ty, attribList, nestedList, isFullSyntax);
            markdownContent.AppendLine($"```xml\n{doc.ToString()}\n```");
        }

        private static XElement BuildTaskNameAttributeSyntax(string name, Type ty, List<PropertyInfo> attribList, List<PropertyInfo> nestedList, bool isFullSyntax)
        {
            XElement xElem = new XElement(name);
            foreach (System.Reflection.PropertyInfo tyProp in attribList)
            {
                var attrName = tyProp.GetCustomAttribute<NAnt.Core.Attributes.TaskAttributeAttribute>().Name;
                if (xElem.AttributeAnyNS(attrName) == null)
                    xElem.Add(new XAttribute(attrName, ""));
            }

            if (nestedList != null)
            {
                // Write nested build elements.
                foreach (System.Reflection.PropertyInfo tyProp in nestedList)
                {
                    xElem.Add(BuildElementSyntax(tyProp, attribList, nestedList, isFullSyntax));
                }
            }

            var methods = ty.GetMethods(BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.Instance);

            foreach (var method in methods)
            {
                var customAttributes = method.GetCustomAttributes<NAnt.Core.Attributes.XmlElementAttribute>();
                foreach (var attr in customAttributes)
                {
                    xElem.Add(new XElement(attr.Name));
                }
            }

            return xElem;
        }

        private static XElement BuildElementSyntax(PropertyInfo tyProp, List<PropertyInfo> attribList, List<PropertyInfo> nestedList, bool isFullSyntax, bool elementOnly = false)
        {
            List<string> hiddenAttributes = new List<string>() { "verbose", "failonerror" };
            string[] short_element_groups = { "ConfigElement", "DotNetConfigElement", "DotNetDataElement", "BuildStepsElement", "VisualStudioDataElement", "JavaDataElement" };

            if (!isFullSyntax)
            {
                hiddenAttributes.Add("if");
                hiddenAttributes.Add("unless");
            }

            var nestedElemName = tyProp.Name;
            var buildElementAttribute = tyProp.GetCustomAttribute<NAnt.Core.Attributes.BuildElementAttribute>();
            var buildAttributeAttribute = tyProp.GetCustomAttribute<NAnt.Core.Attributes.BuildAttributeAttribute>();
            var xmlAttribute = tyProp.GetCustomAttribute<NAnt.Core.Attributes.XmlElementAttribute>();

            if (buildElementAttribute != null)
                nestedElemName = buildElementAttribute.Name;
            else if (buildAttributeAttribute != null)
                nestedElemName = buildAttributeAttribute.Name;
            else if (xmlAttribute != null)
                nestedElemName = xmlAttribute.Name;

            XElement xElem = new XElement(nestedElemName);

            if (!isFullSyntax && !(short_element_groups.Contains(tyProp.PropertyType.Name) || short_element_groups.Contains(tyProp.PropertyType.BaseType.Name)))
                elementOnly = true;

            var properties = isFullSyntax ? tyProp.PropertyType.GetProperties(BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.Instance)
                : tyProp.PropertyType.GetProperties(BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.Instance | BindingFlags.DeclaredOnly);
            foreach (var prop in properties)
            {
                var taskAttr = prop.GetCustomAttribute<NAnt.Core.Attributes.TaskAttributeAttribute>();
                if (taskAttr != null && !xElem.AttributesAnyNS(taskAttr.Name).Any() && !hiddenAttributes.Contains(taskAttr.Name))
                    xElem.Add(new XAttribute(taskAttr.Name, ""));
                else if (prop.GetCustomAttributes<NAnt.Core.Attributes.BuildElementAttribute>().Any()
                    || prop.GetCustomAttributes<NAnt.Core.Attributes.BuildAttributeAttribute>().Any()
                    || prop.GetCustomAttributes<NAnt.Core.Attributes.XmlElementAttribute>().Any())
                {
                    if (!elementOnly)
                        xElem.Add(BuildElementSyntax(prop, attribList, nestedList, isFullSyntax, elementOnly: !isFullSyntax));
                }

            }

            if (!elementOnly)
            {
                var methods = isFullSyntax ? tyProp.PropertyType.GetMethods(BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.Instance)
                : tyProp.PropertyType.GetMethods(BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.Instance );


                foreach (var method in methods)
                {
                    var customAttributes = method.GetCustomAttributes<NAnt.Core.Attributes.XmlElementAttribute>();
                    foreach (var attr in customAttributes)
                    {
                        xElem.Add(new XElement(attr.Name));
                    }
                }

                if (isFullSyntax)
                {
                    Type structuredFileSet = typeof(EA.Eaconfig.Structured.StructuredFileSet);
                    if (tyProp.PropertyType == structuredFileSet || tyProp.PropertyType.IsSubclassOf(structuredFileSet))
                    {
                        xElem.Add(new XComment("Structured Fileset"));
                    }

                    Type structuredOptionSet = typeof(EA.Eaconfig.Structured.StructuredOptionSet);
                    if (tyProp.PropertyType == structuredOptionSet || tyProp.PropertyType.IsSubclassOf(structuredOptionSet))
                    {
                        xElem.Add(new XComment("Structured Optionset"));
                    }

                    Type ConditionalPropertyElement = typeof(EA.Eaconfig.Structured.ConditionalPropertyElement);
                    if (tyProp.PropertyType == ConditionalPropertyElement || tyProp.PropertyType.IsSubclassOf(ConditionalPropertyElement))
                    {
                        xElem.Add(new XComment("Text"));
                    }

                    if (nestedElemName == "do" || nestedElemName == "target" || nestedElemName == "custom-build-tool")
                    {
                        xElem.Add(new XComment("NAnt Script"));
                    }
                }
            }

            return xElem;
        }

    }
}

