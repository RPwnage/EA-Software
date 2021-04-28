// NAnt - A .NET build tool
// Copyright (C) 2003-2015 Electronic Arts Inc.
// Copyright (C) 2001-2003 Gerry Shaw
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
//
//
// Electronic Arts (Frostbite.Team.CM@ea.com)
// Sergey Chaban (serge@wildwestsoftware.com)
// Gerry Shaw (gerry_shaw@yahoo.com)

using System;
using System.Xml;
using System.IO;
using System.Reflection;

using NAnt.Core.Util;
using NAnt.Core.Attributes;
using NAnt.Core.Logging;
using NAnt.Core.Reflection;
using System.Collections.Generic;

namespace NAnt.Core.Tasks
{

	/// <summary>Executes the code contained within the task.</summary>
	/// <remarks>
	///
	/// <para>The <c>script</c> element must contain a single <c>code</c> element, which in turn
	/// contains the script code.</para>
	/// <para>A static entry point named <c>ScriptMain</c> is required. It must have a single
	/// Nant.Core.Project parameter.</para>
	///
	/// <para>The following namespaces are loaded by default:</para>
	/// <list type="bullet">
	///     <item>System</item>
	///     <item>System.Collections</item>
	///     <item>System.Collections.Specialized</item>
	///     <item>System.IO</item>
	///     <item>System.Text</item>
	///     <item>System.Text.RegularExpressions</item>
	///     <item>NAnt.Core</item>
	/// </list>
	/// The <b>&lt;imports></b> element can be used to import more namespaces; see example below.
	/// You may need to add <c>&lt;references></c> to load assemblies containing those namespaces.
	/// </remarks>
	/// <include file='Examples/Script/csharp.example' path='example'/>
	/// <include file='Examples/Script/Property.example' path='example'/>
	[TaskName("script", NestedElementsCheck = true)]
	public class ScriptTask : Task
	{
		private static readonly string[] _defaultNamespaces =  {
			"System",
			"System.Collections",
			"System.Collections.Specialized",
			"System.IO",
			"System.Text",
			"System.Text.RegularExpressions",
			"NAnt.Core",
		};
		string[] _classNames = null;
		XmlNode[] _scriptNodes = null;

		/// <summary>The language of the script block (C# or VB).</summary>
		[TaskAttribute("language")]
		public string Language { get; set; } = null;

		/// <summary>Required assembly references to link with.</summary>
		[FileSet("references")]
		public FileSet References { get; } = new FileSet();

		/// <summary>The script to execute. It's required that the script be put in a CDATA element. This is
		/// because the potential use of XML reserved characters in the script. See example below.</summary>
		[Property("code")]
		public XmlPropertyElement Code { get; } = new XmlPropertyElement();

		// Add this to force documentation on imports
		/// <summary>Namespaces to import.</summary>
		/// <remarks>
		/// Should contain elements:
		///  &lt;import name="namespace name"/&gt;
		/// </remarks>
		[Property("imports")]
		public PropertyElement DummyImports
		{
			get { return new PropertyElement(); }
		}
		public List<string> Imports { get; } = new List<string>(_defaultNamespaces);

		/// <summary>The name of the main class containing the static <c>ScriptMain</c> entry point.</summary>
		[TaskAttribute("mainclass", Required = false)]
		public string MainClass { get; set; } = String.Empty;

		/// <summary>The flag of compiling code into a saved assembly or not. The default value is false. Given the file
		/// containing the &lt;script&gt; is named package.build, the assembly will be named package.build.dll</summary>
		[TaskAttribute("compile", Required = false)]
		public bool Compile { get; set; } = false;

		public ScriptTask() : base() { }

		public ScriptTask(Project project, string code = null, FileSet references = null, List<string> imports = null) 
			: base()
		{
			Project = project;
			Code.Value = code;
			if (references != null) References = references;
			if (imports != null) Imports.AddRange(imports);
		}

		protected override void InitializeTask(XmlNode taskNode)
		{
			List<XmlNode> importsElList = taskNode.GetChildElementsByName("imports");
			foreach (XmlNode imports in importsElList)
			{
				List<XmlNode> importElList = imports.GetChildElementsByName("import");
				foreach (XmlNode import in importElList)
				{
					Imports.Add(import.Attributes["name"].InnerText);
				}
			}
		}

		// Loads the content of a file to a byte array.
		static byte[] LoadFile(string filename)
		{
			byte[] buffer = null;
			using (FileStream fs = new FileStream(filename, FileMode.Open))
			{
				buffer = new byte[(int)fs.Length];
				fs.Read(buffer, 0, buffer.Length);
			}
			return buffer;
		}

		protected override void ExecuteTask()
		{
			if (Language != null)
			{
				if (String.Equals(Language, "C#", StringComparison.OrdinalIgnoreCase) ||
					String.Equals(Language, "CSHARP", StringComparison.OrdinalIgnoreCase))
				{
					Project.Log.ThrowDeprecation(Log.DeprecationId.ScriptTaskLanguageParameter, Log.DeprecateLevel.Normal, "{0} Language parameter is not supported for {1}. Specifying C# is unnecessary as this is now the only allowed language.", Location, Name);
				}
				else
				{
					throw new BuildException($"Language parameter is not supported for {Name}. Only C# is supported.");
				}
			}

			var timer = new Chrono();

			if (String.IsNullOrEmpty(MainClass))
			{
				MainClass = /*Target.Name*/ "xx" + "_script_" + Hash.MakeGUIDfromString(Location.ToString()).ToString().Replace('-', '_');
			}

			string outputAssemblyPath = GetOutputAssemblyPath();
			if (!AssemblyLoader.TryGetCached(outputAssemblyPath, out Assembly assembly))
			{
				assembly = LoadOrCompileAssembly(outputAssemblyPath);
			}

			Type mainType = assembly.GetType(MainClass);
			if (mainType == null)
			{
				throw new BuildException("Invalid mainclass.", Location);
			}

			CallScriptMain(mainType);

			Log.Info.WriteLine(LogPrefix + "{0} time used={1} ms, compile={2}",
				Location, timer.ToString(), Compile);
		}

		private string GetOutputAssemblyPath()
		{
			//If we don't know the build root location just create the DLL file in where the build file is located.

			var buildpath = Project.Properties[Project.NANT_PROPERTY_PROJECT_TEMPROOT];

			String fileName = String.Format("{0}-{1}-{2}.dll", Location.FileName, Location.LineNumber, Hash.MakeGUIDfromString(Code.Value));

			if (!String.IsNullOrEmpty(buildpath))
			{
				string package = String.Empty;
				string version = String.Empty;

				string path = Path.GetDirectoryName(Location.FileName);
				version = Path.GetFileName(path);
				if (0 == String.Compare(version, "scripts", true))
				{
					path = Path.GetDirectoryName(path);
					version = Path.GetFileName(path);
				}
				path = Path.GetDirectoryName(path);
				package = Path.GetFileName(path);

				fileName = Path.GetFileName(fileName);

				if (!String.IsNullOrEmpty(version) && version != ".")
				{
					fileName = version + "-" + fileName;
				}
				if (!String.IsNullOrEmpty(package) && package != ".")
				{
					fileName = package + "-" + fileName;
				}

				// Create the DLL inside the build root if possible.
				fileName = Path.Combine(buildpath, fileName);
			}
			return fileName;
		}

		void CallScriptMain(Type mainType)
		{
			MethodInfo entry = mainType.GetMethod("ScriptMain");
			if (entry == null)
				throw new BuildException("Missing entry point.", Location);

			if (!entry.IsStatic)
				throw new BuildException("Invalid entry point declaration (should be static).", Location);

			ParameterInfo[] entryParams = entry.GetParameters();
			if (entryParams.Length != 1)
				throw new BuildException("Invalid entry point declaration (wrong number of parameters).", Location);

			if (entryParams[0].ParameterType != typeof(Project))
				throw new BuildException("Invalid entry point declaration (invalid parameter type, Project expected).", Location);

			try
			{
				entry.Invoke(null, new Object[] { Project });
			}
			catch (Exception e)
			{
				// This exception is not likely to tell us much, BUT the
				// InnerException normally contains the runtime exception
				// thrown by the executed script code.
				//throw new BuildException("Script exception: " + e.InnerException, Location);
				throw new BuildException("Script exception.", Location, e.InnerException);
			}
		}

		void ParseScripts(string fname)
		{
			//int t1, t2;
			//t1 = Environment.TickCount;
			XmlDocument doc = LineInfoDocument.Load(fname);

			XmlNamespaceManager nsmgr = new XmlNamespaceManager(doc.NameTable);
			nsmgr.AddNamespace("x", doc.DocumentElement.NamespaceURI);
			XmlNodeList nodes = doc.SelectNodes("//x:script", nsmgr);

			if (String.IsNullOrWhiteSpace(nodes[0].BaseURI))
			{
				throw new BuildException("Error: Uri", Location);
			}

			_classNames = new string[nodes.Count];
			_scriptNodes = new XmlNode[nodes.Count];
			int index = 0;
			int rootScripts = 0;
			foreach (XmlNode node in nodes)
			{
				string mainClass = null;
				if (node.Attributes["mainclass"] != null)
				{
					mainClass = node.Attributes["mainclass"].Value;
					//Console.WriteLine("found <script>, using its class name " + mainClass);
				}
				else
				{
					XmlNode parent = node.ParentNode;
					if (parent.Attributes["name"] != null)
					{
						mainClass = parent.Attributes["name"].Value.Replace("-", "_");
					}
					else
					{
						mainClass = "xx_script_" + rootScripts;
						rootScripts++;
					}
					//Console.WriteLine("found <script>, setting its class name to " + mainClass);
				}
				_classNames[index] = mainClass;
				_scriptNodes[index] = node;
				index++;

				// Handle references and imports for other nodes
				List<XmlNode> importsElList = node.GetChildElementsByName("imports");
				foreach (XmlNode imports in importsElList)
				{
					List<XmlNode> importElList = imports.GetChildElementsByName("import");
					foreach (XmlNode import in importElList)
					{
						string imp = import.Attributes["name"].InnerText;
						if (!Imports.Contains(imp))
						{
							Imports.Add(imp);
						}
					}
				}

				XmlNode refNode = node.GetChildElementByName("references");
				if (refNode != null)
				{
					FileSet fs = new FileSet();
					fs.Project = Project;
					fs.Initialize(refNode);
					// May need to make sure no duplicate reference is added
					References.FailOnMissingFile = fs.FailOnMissingFile;
					References.BaseDirectory = fs.BaseDirectory;
					References.Append(fs);
				}
			}
			//t2 = Environment.TickCount;
			//Console.WriteLine("Time for parsing scripts: " + (t2-t1) + "(ms)");
		}

		Assembly LoadOrCompileAssembly(string path)
		{
			Assembly assembly = null;

			if (Compile)
			{
				using (var buildState = PackageCore.PackageAutoBuildCleanMap.AssemblyAutoBuildCleanMap.StartBuild(path, "dotnet"))
				{
					if (!buildState.IsDone())
					{
						if (NeedCompile(path))
						{
							CompileScripts(path);
						}
					}
				}

				assembly = AssemblyLoader.Get(path, true);
			}
			else
			{
				assembly = CompileScripts(null); // pass null so that compiler does not save assembly
				AssemblyLoader.Add(assembly, path, Path.GetFileName(path));
			}
			return assembly;
		}

		Assembly CompileScripts(string outputAssembly)
		{
			Assembly compiled = null;

			using (var compiler = new ScriptCompiler(Project, true, Location))
			{
				var references = compiler.GetReferenceAssemblies(References);

				var code = compiler.GenerateCode(MainClass, Code.Value, Imports);

				compiled = compiler.CompileAssemblyFromCode(code, outputAssembly, this);

				compiler.LoadReferencedAssemblies();
			}

			return compiled;
		}

		bool NeedCompile(string scriptPath)
		{
			// Does the DLL exist?
			if (!File.Exists(scriptPath))
				return true;
			// Compare timestamps of build file and assembly file
			FileInfo fi1 = new FileInfo(Location.FileName.Replace("file:///",""));
			FileInfo fi2 = new FileInfo(scriptPath);
			return fi1.LastWriteTime > fi2.LastWriteTime;
		}
	}
}
