using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;
using System.Reflection;

using System.CodeDom;
using System.CodeDom.Compiler;
using Microsoft.CSharp;
using Microsoft.VisualBasic;

using NAnt.Core.Util;
using NAnt.Core.Tasks;

namespace NAnt.Core.Reflection
{
    public class ScriptCompiler : IDisposable
    {
        private readonly Project _project;
        private readonly bool _debug;
        private readonly Language _language;
        private readonly string _version;
        private readonly string _keyfile;
        private readonly Location _location;
        private readonly CodeDomProvider _codeProvider;
        private List<string>  _referenceassemblies;

        public enum Language { CS, VB }

        public ScriptCompiler(Project project, bool debug, Location location, Language language = Language.CS, string version = null, string keyfile = null)
        {
            _project = project;
            _debug = debug;
            _location = location;
            _language = language;
            _version = version;
            _keyfile = keyfile;
            _codeProvider = GetCodeProvider();
        }

        public List<string> GetReferenceAssemblies(FileSet references)
        {
            if(_referenceassemblies == null)
            {
            if (references!= null && references.Includes.Count > 0)
            {
                // References to all loaded Dlls are added automatically. To avoid double references exclude anything from framework and eaconfig:
                if (_project != null)
                {
                    var eaconfig_path = _project.Properties["package.eaconfig.dir"];

                    if (!String.IsNullOrEmpty(eaconfig_path))
                    {
                        references.Excludes.Add(PatternFactory.Instance.CreatePattern(eaconfig_path + "/bin/eaconfig.dll", references.BaseDirectory, false));
                    }
                }
                references.Excludes.Add(PatternFactory.Instance.CreatePattern(Project.NantLocation + "/bin/*.dll", references.BaseDirectory, false));
                // Reset internal _initialized flag to false
                references.Includes.AddRange(new List<FileSetItem>());
            }

            var finalreferences = AppDomain.CurrentDomain.GetAssemblies().Where(a => !a.IsDynamic && !String.IsNullOrEmpty(a.Location)).Select(a => a.Location).ToList();

            finalreferences.AddRange(references.FileItems.Select(fi => fi.Path.Path));

            // Remove duplicate references using assembly file name).
            _referenceassemblies = finalreferences.OrderedDistinct<string>(FileNameEqualityComparer.Instance).ToList();
            }

            return _referenceassemblies;
        }


        public Assembly CompileAssemblyFromFiles(FileSet sources, string assemblyFileName = null)
        {
            Assembly assembly = null;
            if (sources != null)
            {
                var parameters = GetCompilerParameters(assemblyFileName);

                PrepareOputputDir(assemblyFileName);

                var results = _codeProvider.CompileAssemblyFromFile(parameters, sources.FileItems.Select(fi => fi.Path.Path).ToArray());

                ProcessCompilerResults(results, assemblyFileName);

                if (!String.IsNullOrEmpty(assemblyFileName) && File.Exists(assemblyFileName))
                {
                    assembly = AssemblyLoader.Get(assemblyFileName, fromMemory: true);
                }
                else
                {
                    assembly = results.CompiledAssembly;
                }
            }

            return assembly;
        }

        public Assembly CompileAssemblyFromCode(String code, string assemblyFileName = null)
        {
            Assembly assembly = null;
            if (!String.IsNullOrEmpty(code))
            {
                var parameters = GetCompilerParameters(assemblyFileName);

                PrepareOputputDir(assemblyFileName);

                var results = _codeProvider.CompileAssemblyFromSource(parameters, code);

                ProcessCompilerResults(results, assemblyFileName);

                if (!String.IsNullOrEmpty(assemblyFileName) && File.Exists(assemblyFileName))
                {
                    assembly = AssemblyLoader.Get(assemblyFileName, fromMemory: true);
                }
                else
                {
                    assembly = results.CompiledAssembly;
                }
            }

            return assembly;
        }



        public string GenerateCode(string typeName, string codeBody, IEnumerable<string> imports)
        {
            var compileUnit = new CodeCompileUnit();

            // Add code namespace
            var codenamesp = new CodeNamespace();
            compileUnit.Namespaces.Add(codenamesp);

            // Add Import namespaces
            foreach (string imp in imports.OrderedDistinct())
            {
                codenamesp.Imports.Add(new CodeNamespaceImport(imp));
            }
            // Declare a new type
            var typeDecl = new CodeTypeDeclaration(typeName);
            typeDecl.IsClass = true;
            typeDecl.TypeAttributes = TypeAttributes.Public;

            // Add the new type to the namespace type collection.
            codenamesp.Types.Add(typeDecl);

            typeDecl.Members.Add(new CodeSnippetTypeMember(codeBody));

            StringWriter sw = new StringWriter();

            _codeProvider.GenerateCodeFromCompileUnit(compileUnit, sw, null);

            return sw.ToString(); 
        }



        private CodeDomProvider GetCodeProvider()
        {
            CodeDomProvider codeProvider = null;

            switch (_language)
            {
                case Language.VB:
                    if (!String.IsNullOrEmpty(_version))
                    {
                        codeProvider = new VBCodeProvider(new Dictionary<string, string>() { { "CompilerVersion", _version } });
                    }
                    else
                    {
                        codeProvider = new VBCodeProvider();
                    }
                    break;
                case Language.CS:
                default:
                    if (!String.IsNullOrEmpty(_version))
                    {
                        codeProvider = new CSharpCodeProvider(new Dictionary<string, string>() { { "CompilerVersion", _version } });
                    }
                    else
                    {
                        codeProvider = new CSharpCodeProvider();
                    }
                    break;
            }
            return codeProvider;

        }

        private CompilerParameters GetCompilerParameters(string assemblyFileName)
        {
            var parameters = new CompilerParameters(_referenceassemblies.ToArray(), assemblyFileName, _debug);

            parameters.GenerateExecutable = false;
            parameters.GenerateInMemory = String.IsNullOrEmpty(assemblyFileName);
            parameters.TreatWarningsAsErrors = false;

            if (!String.IsNullOrEmpty(_keyfile))
            {
                parameters.CompilerOptions += " /keyfile:" + _keyfile;
            }

            var defines = new StringBuilder();
#if DEBUG
            defines.Append(" /define:DEBUG");
#endif
#if TRACE
            defines.Append(" /define:TRACE");
#endif
            defines.Append(" /define:FRAMEWORK3");

            parameters.CompilerOptions += defines.ToString();

            return parameters;

        }

        private void PrepareOputputDir(string assemblyFileName)
        {
            if (!String.IsNullOrEmpty(assemblyFileName))
            {

                MkDirTask mkdir = new MkDirTask();
                mkdir.Project = _project;
                mkdir.Dir = Path.GetDirectoryName(assemblyFileName);
                mkdir.Execute();

                if (File.Exists(assemblyFileName))
                {
                    try
                    {
                        File.SetAttributes(assemblyFileName, File.GetAttributes(assemblyFileName) & ~FileAttributes.ReadOnly);
                    }
                    catch { }
                }
            }
        }


        private void ProcessCompilerResults(CompilerResults results, string assemblyFileName)
        {            
            if (results.Errors.HasErrors || results.Errors.HasWarnings)
            {
                StringBuilder sb = new StringBuilder();
                sb.AppendFormat("[taskdef] Assembly '{0}] compilation {1}:{2}{2}", assemblyFileName, results.Errors.HasErrors ? "failed" : "warnings", Environment.NewLine);

                foreach (CompilerError err in results.Errors)
                {
                    sb.AppendLine(err.ToString());
                }
                if (results.Errors.HasErrors)
                {
                    throw new BuildException(Environment.NewLine + sb.ToString(), _location);
                }
                else
                {
                    _project.Log.Status.WriteLine(sb.ToString());
                }
            }
        }


        public void Dispose()
        {
            Dispose(true);
            GC.SuppressFinalize(this);
        }

        void Dispose(bool disposing)
        {
            if (!this._disposed)
            {
                if (disposing)
                {
                    _codeProvider.Dispose();
                }
            }
            _disposed = true;
        }
        private bool _disposed = false;
    }
}
