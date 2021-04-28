// NAnt - A .NET build tool
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
// Sergey Chaban (serge@wildwestsoftware.com)
// Gerry Shaw (gerry_shaw@yahoo.com)

using System;
using System.Xml;
using System.IO;
using System.Reflection;
using System.Collections;
using System.Collections.Specialized;
using System.CodeDom;
using System.CodeDom.Compiler;
using System.Text;

using NAnt.Core;
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
    /// <para>A static entry point named <c>ScriptMain</c> is required.   It must have a single
    /// <see cref="Project"/> parameter.</para>
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
    /// <include file='Examples/Script/vb.example' path='example'/>
    /// <include file='Examples/Script/Property.example' path='example'/>
    [TaskName("script")]
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

        string _language = "Unknown";
        FileSet _references = new FileSet();
        XmlPropertyElement _codeProperty = new XmlPropertyElement();
        string _mainClass = String.Empty;
        List<string> _imports = new List<string>(_defaultNamespaces);

        string[] _classNames = null;
        XmlNode[] _scriptNodes = null;
        bool _compileScript = false;

        /// <summary>The language of the script block (C# or VB).</summary>
        [TaskAttribute("language", Required = true)]
        public string Language
        {
            get { return _language; }
            set { _language = value; }
        }

        /// <summary>Required assembly references to link with.</summary>
        [FileSet("references")]
        public FileSet References
        {
            get { return _references; }
        }

        /// <summary>The script to execute. It's required that the script be put in a CDATA element. This is
        /// because the potential use of XML reserved characters in the script. See example below.</summary>
        [Property("code")]
        public XmlPropertyElement Code
        {
            get { return _codeProperty; }
        }

        // Add this to force documentation on imports
        /// <summary>Namespaces to import.</summary>
        /// <remarks>
        /// This causes a subtle problem in Element.ProcessBuildElement(3):
        /// Element childElement = (Element)propertyInfo.GetValue(this, null);
        /// This line expects the property is derived from Element.
        /// So comment out following property
        /// </remarks>
        //[Property("imports")]
        public List<string> Imports
        {
            get { return _imports; }
        }

        /// <summary>The name of the main class containing the static <c>ScriptMain</c> entry point.</summary>
        [TaskAttribute("mainclass", Required = false)]
        public string MainClass
        {
            get { return _mainClass; }
            set { _mainClass = value; }
        }

        /// <summary>The flag of compiling code into a saved assembly or not. The default value is false. Given the file
        /// containing the &lt;script&gt; is named package.build, the assembly will be named package.build.dll</summary>
        [TaskAttribute("compile", Required = false)]
        public bool Compile
        {
            get { return _compileScript; }
            set { _compileScript = value; }
        }

        protected override void InitializeTask(XmlNode taskNode)
        {
            if (String.IsNullOrEmpty(_mainClass))
            {
                _mainClass = /*Target.Name*/ "xx" + "_script_" + Hash.MakeGUIDfromString(Location.ToString()).ToString().Replace('-', '_');
            }

            List<XmlNode> importsElList = taskNode.GetChildElementsByName("imports");
            foreach (XmlNode imports in importsElList)
            {
                List<XmlNode> importElList = imports.GetChildElementsByName("import");
                foreach (XmlNode import in importElList)
                {
                    _imports.Add(import.Attributes["name"].InnerText);
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

        private static void LoadReferencedAssemblies(FileSet references, string basedir)
        {
            if (references != null && references.Includes.Count > 0)
            {
                try
                {
                    if (references.BaseDirectory == null)
                    {
                        references.BaseDirectory = basedir;
                    }

                    foreach (FileItem assemblyItem in references.FileItems)
                    {
                        if (!String.IsNullOrEmpty(assemblyItem.FileName))
                        {
                            AssemblyLoader.Get(assemblyItem.FileName);
                        }
                    }
                }
                catch(Exception)
                {
                    // Ignore any errors now. WE will get these errors when trying to run an assembly
                }

            }
        }

        protected override void ExecuteTask()
        {
            var timer = new Chrono();

            Assembly assembly = null;

            var outputAssembly = this.GetOutputAssemblyPath();

            if (!AssemblyLoader.TryGetCached(outputAssembly, out assembly))
            {
                assembly = LoadOrCompileAssembly(outputAssembly);
            }

            Type mainType = assembly.GetType(_mainClass);
            if (mainType == null)
            {
                throw new BuildException("Invalid mainclass.", Location);
            }

            CallScriptMain(mainType);

                Log.Info.WriteLine(LogPrefix + "{0} time used={1} ms, compile={2}",
                    Location, timer.ToString(), _compileScript);
        }

        private string GetOutputAssemblyPath()
        {
            //If we don't know the build root location just create the dll file in where the build file is located.

            var buildpath = Project.Properties[Project.NANT_PROPERTY_PROJECT_TEMPROOT];

            String fileName = String.Format("{0}-{1}-{2}.dll", Location.FileName, Location.LineNumber, Hash.MakeGUIDfromString(_codeProperty.Value));

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

                // Create the dll inside the build root if possible.
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
                        if (!_imports.Contains(imp))
                        {
                            _imports.Add(imp);
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

            if (_compileScript)
            {
                //ParseScripts(Location.FileName);
                // Load Referenced assemblies. This will Update Assembly resolver data as well
                //LoadReferencedAssemblies(References, Project.BaseDirectory);

                if (NeedCompile(path))
                {
                    CompileScripts(path);
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

            using (var compiler = new ScriptCompiler(Project, false, Location, GetScriptLanguage()))
            {
                var references = compiler.GetReferenceAssemblies(References);

                var code = compiler.GenerateCode(_mainClass, _codeProperty.Value, _imports);

                compiled = compiler.CompileAssemblyFromCode(code, outputAssembly);
            }

/*
            string[] classCodes = new string[_scriptNodes.Length];
            for (int i = 0; i < _scriptNodes.Length; i++)
                classCodes[i] = _scriptNodes[i].InnerText;

            string codes = compilerInfo.GenerateCodes(_classNames, classCodes, _imports);
            CompilerResults results = compilerInfo.Provider.CompileAssemblyFromSource(options, codes);
*/
            return compiled;
        }

        bool NeedCompile(string scriptPath)
        {
            // Does the dll exist?
            if (!File.Exists(scriptPath))
                return true;
            // Compare timestamps of build file and assembly file
            FileInfo fi1 = new FileInfo(Location.FileName.Replace("file:///",""));
            FileInfo fi2 = new FileInfo(scriptPath);
            return fi1.LastWriteTime > fi2.LastWriteTime;
        }

        private ScriptCompiler.Language GetScriptLanguage()
        {
            if (String.Equals(Language, "C#", StringComparison.OrdinalIgnoreCase) ||
               String.Equals(Language, "CSHARP", StringComparison.OrdinalIgnoreCase))
            {
                return ScriptCompiler.Language.CS;
            }
            if (String.Equals(Language, "VB", StringComparison.OrdinalIgnoreCase) ||
               String.Equals(Language, "VISUALBASIC", StringComparison.OrdinalIgnoreCase))
            {
                return ScriptCompiler.Language.CS;
            }

            throw new BuildException("Unknown language '" + _language + "'.", Location);

        }

    }
}
