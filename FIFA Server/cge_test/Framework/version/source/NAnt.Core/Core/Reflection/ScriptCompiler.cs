// Originally based on NAnt - A .NET build tool
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
// Gerry Shaw (gerry_shaw@yahoo.com)
// Ian MacLean (ian_maclean@another.com)
// Electronic Arts (Frostbite.Team.CM@ea.com)

using System;
using System.CodeDom.Compiler;
using System.CodeDom;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Text;

using NAnt.Core.Tasks;
using NAnt.Core.Util;
using NAnt.Core.Logging;
using Microsoft.CodeAnalysis.CSharp;
using Microsoft.CodeAnalysis.Text;
using Microsoft.CodeAnalysis;
using Microsoft.CodeAnalysis.Emit;
using System.Runtime.Loader;

namespace NAnt.Core.Reflection
{
	public sealed class ScriptCompiler : IDisposable
	{
		private readonly Project m_project;
		private readonly bool _debug;
		private readonly string _version;
		private readonly string _keyfile;
		private readonly Location _location;
		private readonly CodeDomProvider m_codeProvider;
		private List<string>  _referenceassemblies;

		private bool m_disposed = false;

		private static readonly string[] s_defaultReferences = new string[]
		{
			"System.Core.dll",
			"System.dll",
			"System.Xml.dll",
			"System.Runtime.dll",
			"netstandard.dll"
		};

		/* setting up mono facade dir as search paths seems to work for now, but keeping this
		in case I need to fall back to it (remember to remove system.runtime and netstandard from
		above if resurecting this code)

		// DAVE-FUTURE-REFACTOR-TODO: the mono rules below aren't based off of any rational
		// basis they are simply "what works" - better research into what we need to do 
		// under mono context is needed (or we should move to dot net core)

		// only to be referenced where not running under mono
		private static readonly string[] s_nonMonoReferences = new string[]
		{
			"System.Runtime.dll",
			"netstandard.dll"
		};

		// force exclude under mono, even if user has requested this assembly
		private static readonly string[] s_monoForceExcludeReferences = new string[]
		{
			"System.Runtime.dll"
		};*/

		public ScriptCompiler(Project project, bool debug, Location location, string version = null, string keyfile = null)
		{
			m_project = project;
			_debug = debug;
			_location = location;
			_version = version;
			_keyfile = keyfile;
			
			if (PlatformUtil.IsMonoRuntime)
			{
				string compilerPath = project.GetPropertyValue("package.mono.tools.compiler");
				m_codeProvider = CodeDomProvider.CreateProvider("cs", new Dictionary<string, string> {
					{ "CompilerDirectoryPath", Path.GetDirectoryName(compilerPath) }
				});
			}
			else
			{
#if NETFRAMEWORK
				InternalQuickDependency.Dependent(project, "VisualStudio");
				string compilerPath = project.GetPropertyValue("package.VisualStudio.csc.exe");
				if (compilerPath != null)
				{
					m_codeProvider = CodeDomProvider.CreateProvider("cs", new Dictionary<string, string> {
						{ "CompilerDirectoryPath", Path.GetDirectoryName(compilerPath) }
					});
				}
				else
				{
					throw new BuildException("Script compiler could not obtain the path to csc.exe " +
						"from the visual studio package and as a result is unable to compile taskdefs.");
				}
#else
				// we still actually need the code provider to generate code from a DOM, even if we have a different C# compiler
				if (!String.IsNullOrEmpty(_version))
				{
					m_codeProvider = new CSharpCodeProvider(new Dictionary<string, string>() { { "CompilerVersion", _version } });
				}
				else
				{
					m_codeProvider = new CSharpCodeProvider();
				}
#endif
			}
		}

		public List<string> GetReferenceAssemblies(FileSet references)
		{
			bool useLegacyReferences = m_project.Properties.GetBooleanPropertyOrDefault("framework.script-compiler.use-legacy-references", false);
			if (useLegacyReferences)
			{
				// legacy references gatering is disabled by default as it takes all the currently loaded assemblies
				// in the domain and makes them references - this can mean script compilation is dependent on 
				// a) the dependenices of nant, which may change
				// b) the order in which nant dependenices / other taskdefs are processed and thus what is loaded
				// we want to avoid these and make users explicitly refernce any dependecnies which aren't clearly
				// going to be needed (i.e user shouldn't need to refernce nant's core assemblies)
				m_project.Log.ThrowDeprecation
				(
					Log.DeprecationId.LegacyScriptCompilerRefernces, Log.DeprecateLevel.Normal,
					new object[] { "framework.script-compiler.use-legacy-references" }, // just use string as key - this only needs to be thrown once
					"Property framework.script-compiler.use-legacy-references is set to true. This is unsafe as scripts resolving references can unexpectedly " +
					"break during Framework upgrades or if assembly load order is changed due to script changes. It is highly recommend you don't set this " +
					"property and instead fix any missing references in Script or Taskdef tasks."
				);

				// framework 7 would have loaded these assemblies by this point - this doesn't happen in FW8 so explicitly load them
				// just in case they are referenced by the taskdef
				Assembly.Load("Microsoft.Build");
				Assembly.Load("Microsoft.Build.Framework");
				Assembly.Load("Microsoft.Build.Utilities.Core");

				if (_referenceassemblies == null)
				{
					references = references ?? new FileSet();
					if (references.Includes.Count > 0)
					{
						// References to all loaded DLLs are added automatically. To avoid double references exclude anything from framework and eaconfig:
						if (m_project != null)
						{
							var eaconfig_path = m_project.Properties["package.eaconfig.dir"];

							if (!String.IsNullOrEmpty(eaconfig_path))
							{
								references.Excludes.Add(PatternFactory.Instance.CreatePattern(eaconfig_path + "/bin/eaconfig.dll", references.BaseDirectory, false));
							}
						}
						references.Excludes.Add(PatternFactory.Instance.CreatePattern(Project.NantLocation + "/bin/*.dll", references.BaseDirectory, false));
						// Reset internal _initialized flag to false
						references.Includes.AddRange(new List<FileSetItem>());
					}

					List<string> allLoadedReferences = AppDomain.CurrentDomain.GetAssemblies().Where(a => !a.IsDynamic && !String.IsNullOrEmpty(a.Location)).Select(a => a.Location).ToList();
					foreach (string loadedReference in allLoadedReferences)
					{
						references.Include(loadedReference);
					}

					SetupDefaultSystemReferences(ref references);

					_referenceassemblies = ResolveReferences(references);
				}
			}
			else
			{
				if (_referenceassemblies == null)
				{
					FileSet finalReferences = references ?? new FileSet();

					SetupDefaultSystemReferences(ref finalReferences);

					foreach (string defaultFrameworkReference in AssemblyLoader.WellKnownAssemblies.Select(a => a.Location))
					{
						finalReferences.Include(defaultFrameworkReference);
					}

					_referenceassemblies = ResolveReferences(finalReferences);
				}
			}

			return _referenceassemblies;
		}

		private static void SetupDefaultSystemReferences(ref FileSet references)
		{
			foreach (string defaultSystemReference in s_defaultReferences)
			{
				references.Include(defaultSystemReference, asIs: true);
			}

			// see commented out s_nonMonoReferences
			/*if (false && PlatformUtil.IsMonoRuntime)
			{
				FileSetItem[] removals = references.Includes
					.Where
					(
						include => s_monoForceExcludeReferences.Any
						(
							exclude => include.Pattern.Value.Equals(exclude, StringComparison.OrdinalIgnoreCase)
						)
					)
					.ToArray();

				foreach (FileSetItem removal in removals)
				{
					references.Includes.Remove(removal);
				}
			}
			else
			{
				foreach (string defaultSystemReference in s_nonMonoReferences)
				{
					references.Include(defaultSystemReference, asIs: true);
				}
			}*/
		}

		private List<string> ResolveReferences(FileSet fileSet)
		{
			return fileSet.FileItems.Select(item => item.Path.Path).OrderedDistinct(FileNameEqualityComparer.Instance).ToList();
		}

#if NETFRAMEWORK
#else
		private Lazy<List<MetadataReference>> m_constantMetadataRefs = new Lazy<List<MetadataReference>>(() =>
		{
			Func<Assembly, IEnumerable<MetadataReference>> walkRefs = (asm) =>
			{
				return new[] { MetadataReference.CreateFromFile(asm.Location) }.Concat(asm.GetReferencedAssemblies().Select(x => MetadataReference.CreateFromFile(Assembly.Load(x).Location)));
			};

			// Get _all_ the "trusted" .NET Core assemblies (overkill, I know)
			var trustedAssembliesPaths = ((string)AppContext.GetData("TRUSTED_PLATFORM_ASSEMBLIES")).Split(Path.PathSeparator);

			var sysRefAssemblies = trustedAssembliesPaths
				.Select(p => MetadataReference.CreateFromFile(p))
				.ToList();

			// Get the nant direct references
			var coreReferences = sysRefAssemblies.Concat(walkRefs(Assembly.GetExecutingAssembly()).Concat(walkRefs(Assembly.GetEntryAssembly())));

			return coreReferences.ToList();
		});

		private Dictionary<string, MetadataReference> m_cachedMetadataReferences = new Dictionary<string, MetadataReference>();

		public Assembly CompileAssemblyInternal(IEnumerable<(SourceText source, string path)> sources, out bool warningsThrown, string assemblyFileName = null, Task task = null, bool includeDebugInformation = true)
		{
			Assembly assembly = null;
			warningsThrown = false;

			bool generateInMemory = string.IsNullOrEmpty(assemblyFileName);
			if (generateInMemory)
				assemblyFileName = Path.GetRandomFileName();
			else
				PrepareOutputDir(assemblyFileName);

			Microsoft.CodeAnalysis.CSharp.CSharpCompilationOptions options =
				new Microsoft.CodeAnalysis.CSharp.CSharpCompilationOptions(Microsoft.CodeAnalysis.OutputKind.DynamicallyLinkedLibrary).WithOverflowChecks(true);

			if (_debug)
				options = options.WithOptimizationLevel(Microsoft.CodeAnalysis.OptimizationLevel.Debug);
			else
				options = options.WithOptimizationLevel(Microsoft.CodeAnalysis.OptimizationLevel.Release);

			options = options.WithWarningLevel(Math.Max(0, Math.Min(3, (int)m_project.Log.WarningLevel)));

			if (!String.IsNullOrEmpty(_keyfile))
			{
				options = options.WithCryptoKeyFile(_keyfile);
			}

			options = options.WithSpecificDiagnosticOptions(new[] { new KeyValuePair<string, Microsoft.CodeAnalysis.ReportDiagnostic>("CS1668", Microsoft.CodeAnalysis.ReportDiagnostic.Suppress) });

			List<string> defines = new List<string>();

			if (_debug)
			{
				defines.Add("DEBUG");
			}
			defines.Add("TRACE");
			defines.Add("FRAMEWORK3");

			var parseOptions = new Microsoft.CodeAnalysis.CSharp.CSharpParseOptions
			(
				Microsoft.CodeAnalysis.CSharp.LanguageVersion.Latest,
				Microsoft.CodeAnalysis.DocumentationMode.None,
				Microsoft.CodeAnalysis.SourceCodeKind.Regular,
				defines
			);

			List<SyntaxTree> parsedSources = new List<SyntaxTree>();
			foreach (var src in sources)
			{
				parsedSources.Add(SyntaxFactory.ParseSyntaxTree(src.source, parseOptions, src.path));
			}

			List<MetadataReference> specificReferences = new List<MetadataReference>();
			foreach (var r in _referenceassemblies)
			{
				MetadataReference metadata = null;
				lock (m_cachedMetadataReferences)
				{
					if (!m_cachedMetadataReferences.TryGetValue(r, out metadata))
					{
						if (File.Exists(r))
							metadata = MetadataReference.CreateFromFile(r);
						else
							metadata = null;
						m_cachedMetadataReferences.Add(r, metadata);
					}
				}
				if (metadata != null)
					specificReferences.Add(metadata);
			}
			var references = m_constantMetadataRefs.Value.Concat(specificReferences);

			Microsoft.CodeAnalysis.CSharp.CSharpCompilation compiler = Microsoft.CodeAnalysis.CSharp.CSharpCompilation.Create(Path.GetFileNameWithoutExtension(assemblyFileName), parsedSources, references, options);

			EmitOptions emitOptions = null;

			if (includeDebugInformation)
				emitOptions = new EmitOptions(debugInformationFormat: DebugInformationFormat.Embedded);

			MemoryStream ms = new MemoryStream();
			var result = compiler.Emit(ms, options: emitOptions);
			ProcessCompilerResults(result, out warningsThrown, assemblyFileName, task);
			if (result.Success)
			{
				if (generateInMemory)
				{
					assembly = Assembly.Load(ms.ToArray());
				}
				else
				{
					using (var fs = File.Create(assemblyFileName))
						ms.WriteTo(fs);
					assembly = AssemblyLoader.Get(assemblyFileName, fromMemory: !PlatformUtil.IsMonoRuntime); // loading from memory on mono costs us the ability to see C# callstacks
				}
			}

			return assembly;
		}
#endif

		private Lazy<List<MetadataReference>> m_constantMetadataRefs = new Lazy<List<MetadataReference>>(() =>
		{
			Func<Assembly, IEnumerable<MetadataReference>> walkRefs = (asm) =>
			{
				return new[] { MetadataReference.CreateFromFile(asm.Location) }.Concat(asm.GetReferencedAssemblies().Select(x => MetadataReference.CreateFromFile(Assembly.Load(x).Location)));
			};

			// Get _all_ the "trusted" .NET Core assemblies (overkill, I know)
			var trustedAssembliesPaths = ((string)AppContext.GetData("TRUSTED_PLATFORM_ASSEMBLIES")).Split(Path.PathSeparator);

			var sysRefAssemblies = trustedAssembliesPaths
				.Select(p => MetadataReference.CreateFromFile(p))
				.ToList();

			// Get the nant direct references
			var coreReferences = sysRefAssemblies.Concat(walkRefs(Assembly.GetExecutingAssembly()).Concat(walkRefs(Assembly.GetEntryAssembly())));

			return coreReferences.ToList();
		});

		private Dictionary<string, MetadataReference> m_cachedMetadataReferences = new Dictionary<string, MetadataReference>();
#if NETFRAMEWORK
#else
		public Assembly CompileAssemblyInternal(IEnumerable<(SourceText source, string path)> sources, out bool warningsThrown, string assemblyFileName = null, Task task = null, bool includeDebugInformation = true)
		{
			Assembly assembly = null;
			warningsThrown = false;

			bool generateInMemory = string.IsNullOrEmpty(assemblyFileName);
			if (generateInMemory)
				assemblyFileName = Path.GetRandomFileName();
			else
				PrepareOutputDir(assemblyFileName);

			CSharpCompilationOptions options =
				new CSharpCompilationOptions(OutputKind.DynamicallyLinkedLibrary).WithOverflowChecks(true);

			if (_debug)
				options = options.WithOptimizationLevel(OptimizationLevel.Debug);
			else
				options = options.WithOptimizationLevel(OptimizationLevel.Release);

			options = options.WithWarningLevel(Math.Max(0, Math.Min(3, (int)m_project.Log.WarningLevel)));

			if (!String.IsNullOrEmpty(_keyfile))
			{
				options = options.WithCryptoKeyFile(_keyfile);
			}

			options = options.WithSpecificDiagnosticOptions(new[] { new KeyValuePair<string, Microsoft.CodeAnalysis.ReportDiagnostic>("CS1668", Microsoft.CodeAnalysis.ReportDiagnostic.Suppress) });

			List<string> defines = new List<string>();

			if (_debug)
			{
				defines.Add("DEBUG");
			}
			defines.Add("TRACE");
			defines.Add("FRAMEWORK3");

			var parseOptions = new CSharpParseOptions
			(
				LanguageVersion.Latest,
				DocumentationMode.None,
				SourceCodeKind.Regular,
				defines
			);

			List<SyntaxTree> parsedSources = new List<SyntaxTree>();
			foreach (var src in sources)
			{
				parsedSources.Add(SyntaxFactory.ParseSyntaxTree(src.source, parseOptions, src.path));
			}

			List<MetadataReference> specificReferences = new List<MetadataReference>();
			foreach (var r in _referenceassemblies)
			{
				MetadataReference metadata = null;
				lock (m_cachedMetadataReferences)
				{
					if (!m_cachedMetadataReferences.TryGetValue(r, out metadata))
					{
						if (File.Exists(r))
							metadata = MetadataReference.CreateFromFile(r);
						else
							metadata = null;
						m_cachedMetadataReferences.Add(r, metadata);
					}
				}
				if (metadata != null)
					specificReferences.Add(metadata);
			}
			var references = m_constantMetadataRefs.Value.Concat(specificReferences);

			CSharpCompilation compiler = CSharpCompilation.Create(Path.GetFileNameWithoutExtension(assemblyFileName), parsedSources, references, options);

			EmitOptions emitOptions = null;

			if (includeDebugInformation)
				emitOptions = new EmitOptions(debugInformationFormat: DebugInformationFormat.Embedded);

			MemoryStream ms = new MemoryStream();
			var result = compiler.Emit(ms, options: emitOptions);
			ProcessCompilerResults(result, out warningsThrown, assemblyFileName, task);
			if (result.Success)
			{
				if (generateInMemory)
				{
					assembly = Assembly.Load(ms.ToArray());
				}
				else
				{
					using (var fs = File.Create(assemblyFileName))
						ms.WriteTo(fs);
					assembly = AssemblyLoader.Get(assemblyFileName, fromMemory: !PlatformUtil.IsMonoRuntime); // loading from memory on mono costs us the ability to see C# callstacks
				}
			}

			return assembly;
		}
#endif

		public Assembly CompileAssemblyFromFiles(FileSet sources, out bool warningsThrown, string assemblyFileName = null, Task task = null, bool includeDebugInformation = true)
		{
			Assembly assembly = null;
			warningsThrown = false;
			if (sources != null)
			{
#if NETFRAMEWORK
				int warningLevel = Math.Max(0, Math.Min(4, (int)m_project.Log.WarningLevel)); // our warning levels map pretty well onto .net compilers - but limit range for futureproofing
				var parameters = GetCompilerParameters(assemblyFileName, warningLevel);

				PrepareOutputDir(assemblyFileName);

				var results = m_codeProvider.CompileAssemblyFromFile(parameters, sources.FileItems.Select(fi => fi.Path.Path).ToArray());

				ProcessCompilerResults(results, out warningsThrown, assemblyFileName, task);

				if (!String.IsNullOrEmpty(assemblyFileName) && File.Exists(assemblyFileName))
				{
					assembly = AssemblyLoader.Get(assemblyFileName, fromMemory: !PlatformUtil.IsMonoRuntime); // loading from memory on mono costs us the ability to see C# callstacks
				}
				else
				{
					assembly = results.CompiledAssembly;
				}
#else
				List<(SourceText, string)> sourceTree = new List<(SourceText, string)>();
				foreach (var fi in sources.FileItems)
				{
					using (var fileStream = File.OpenRead(fi.Path.Path))
					{
						var source = SourceText.From(fileStream);
						sourceTree.Add((source, fi.FullPath));
					}
				}
				assembly = CompileAssemblyInternal(sourceTree, out warningsThrown, assemblyFileName, task, includeDebugInformation);
#endif
			}

			return assembly;
		}

		public Assembly CompileAssemblyFromCode(string code, string assemblyFileName = null, Task task = null)
		{
			Assembly assembly = null;
			if (!String.IsNullOrEmpty(code))
			{
#if NETFRAMEWORK
				int warningLevel = Math.Max(0, Math.Min(4, (int)m_project.Log.WarningLevel)); // our warning levels map pretty well onto .net compilers - but limit range for futureproofing
				var parameters = GetCompilerParameters(assemblyFileName, warningLevel);

				PrepareOutputDir(assemblyFileName);

				var results = m_codeProvider.CompileAssemblyFromSource(parameters, code);

				ProcessCompilerResults(results, out bool warningsThrown, assemblyFileName, task);

				if (!String.IsNullOrEmpty(assemblyFileName) && File.Exists(assemblyFileName))
				{
					assembly = AssemblyLoader.Get(assemblyFileName, fromMemory: !PlatformUtil.IsMonoRuntime); // loading from memory on mono costs us the ability to see C# callstacks
				}
				else
				{
					assembly = results.CompiledAssembly;
				}
#else
				List<(SourceText, string)> sourceTree = new List<(SourceText, string)>();
				assembly = CompileAssemblyInternal(new[] { (SourceText.From(code, System.Text.Encoding.Default), "script") }, out bool warningsThrown, assemblyFileName, task);
#endif
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
			var typeDecl = new CodeTypeDeclaration(typeName)
			{
				IsClass = true,
				TypeAttributes = TypeAttributes.Public
			};

			// Add the new type to the namespace type collection.
			codenamesp.Types.Add(typeDecl);

			typeDecl.Members.Add(new CodeSnippetTypeMember(codeBody));

			using (var sw = new StringWriter())
			{
				m_codeProvider.GenerateCodeFromCompileUnit(compileUnit, sw, null);

				return sw.ToString();
			}
		}

		public string GenerateCode(CodeCompileUnit compileUnit)
		{
			using (var sw = new StringWriter())
			{
				m_codeProvider.GenerateCodeFromCompileUnit(compileUnit, sw, null);

				return sw.ToString();
			}
		}

		public void LoadReferencedAssemblies()
		{
			// DAVE-FUTURE-REFACTOR-TODO: lifetime and setting of _referenceassemblies is non-intuitive, clean up
			try
			{
				foreach (string assemblyPath in _referenceassemblies.Where(path => Path.IsPathRooted(path))) // don't explicitly load relative paths, let normal resolution figure them out
				{
					try
					{
						AssemblyLoader.Get(assemblyPath);
					}
					catch (Exception)
					{
						// Ignore any errors now. WE will get these errors when trying to run an assembly
					}
				}
			}
			catch (Exception)
			{
				// Ignore any errors now. WE will get these errors when trying to run an assembly
			}
		}

		public void Dispose()
		{
			if (!m_disposed)
			{
				m_disposed = true;
				m_codeProvider?.Dispose();

			}
		}

		private CompilerParameters GetCompilerParameters(string assemblyFileName, int warningLevel)
		{
			bool generateInMemory = String.IsNullOrEmpty(assemblyFileName);

			var parameters = new CompilerParameters(_referenceassemblies == null ? new string[0] : _referenceassemblies.ToArray(), assemblyFileName, includeDebugInformation: !generateInMemory)
			{
				GenerateExecutable = false,
				GenerateInMemory = generateInMemory,
				TreatWarningsAsErrors = false,
				WarningLevel = warningLevel
			};

			StringBuilder options = new StringBuilder();
			if (!String.IsNullOrEmpty(_keyfile))
			{
				options.Append(" /keyfile:" + _keyfile);
			}
			if (_debug)
			{
				options.Append(" /define:DEBUG");
			}
			options.Append(" /define:TRACE");
			options.Append(" /define:FRAMEWORK3");
			options.Append(" /nowarn:1668"); // dont throw warnings about invalid paths in LIB env var
			if (!_debug)
			{
				options.Append(" /optimize");
			}

			// Framework is now being built using a lot of system assemblies that actually come from
			// NuGet (and embedded in Framework's bin folder).  So we need to add executing nant.exe's folder
			// to script compile's search path so that we are using consistent version of system assemblies
			// and also just in case user system doesn't have the NuGet version that Framework is using
			// (which can happen on linux system running under mono).
			options.Append(" /lib:" + Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location));

			if (PlatformUtil.IsMonoRuntime)
			{
				options.Append(" /lib:" + Path.Combine(RuntimeEnvironment.GetRuntimeDirectory(), "Facades"));
			}

			parameters.CompilerOptions += options.ToString();

			return parameters;

		}

		private void PrepareOutputDir(string assemblyFileName)
		{
			if (!String.IsNullOrEmpty(assemblyFileName))
			{
				MkDirTask mkdir = new MkDirTask
				{
					Project = m_project,
					Dir = Path.GetDirectoryName(assemblyFileName)
				};
				mkdir.Execute();

				if (File.Exists(assemblyFileName))
				{
					try
					{
						File.SetAttributes(assemblyFileName, File.GetAttributes(assemblyFileName) & ~FileAttributes.ReadOnly);
					}
					catch { }
				}

				string pdbfilename = Path.ChangeExtension(assemblyFileName, "pdb");
				if (File.Exists(pdbfilename))
				{
					try
					{
						File.SetAttributes(pdbfilename, File.GetAttributes(pdbfilename) & ~FileAttributes.ReadOnly);
					}
					catch { }
				}
			}
		}

		private void ProcessCompilerResults(CompilerResults results, out bool warningsThrown, string assemblyFileName = null, Task task = null)
		{
			string taskPrefix = String.Empty;
			if (task != null)
			{
				taskPrefix = $"[{task.Name}] ";
			}

			string locationDescription = String.Empty;
			if (!String.IsNullOrEmpty(assemblyFileName))
			{
				locationDescription = $"[Assembly '{assemblyFileName}'] ";
			}
			else if (task != null)
			{
				locationDescription = $"[{task.Location.ToString().TrimEnd(':')}] "; // Location ToString adds a ':' we don't want
			}

			string failureDesciption = results.Errors.HasErrors ? "failed" : "warnings";

			if (results.Errors.HasErrors || results.Errors.HasWarnings)
			{
				StringBuilder message = new StringBuilder();
				message.AppendLine($"{taskPrefix}{locationDescription}compilation {failureDesciption}:");
				foreach (CompilerError err in results.Errors)
				{
					if (!results.Errors.HasErrors || !err.IsWarning)
					{
						message.AppendLine(err.ToString());
					}
				}

				if (results.Errors.HasErrors)
				{
					throw new BuildException(message.ToString(), _location);
				}
				else if (PlatformUtil.IsMonoRuntime)
				{
					// Mono version can be using an older System.dll "GAC" assembly which doesn't work well dealing with new system.* assemblies that is
					// download from NuGet.  For example:
					// On PC,  CSharpCodeProvider came from C:\WINDOWS\Microsoft.Net\assembly\GAC_MSIL\System\v4.0_4.0.0.0__b77a5c561934e089\System.dll
					// On OSX, CSharpCodeProvider came from /Library/Frameworks/Mono.framework/Versions/6.10.0/lib/mono/gac/System/4.0.0.0__b77a5c561934e089/System.dll
					// This difference is enough to throw a warning when there is a System.*.dll NuGet download such as System.Memory.
					// Which will result with warning message like:
					// "CS1685  The predefined type 'System.Span' is defined multiple times.  Using definition from 'mscorlib.dll'.
					// So for now, if it is under mono, we'll suppress warning as error but still show the warnings.
					if (m_project.Log.IsWarningEnabled(Logging.Log.WarningId.CSharpWarnings, Logging.Log.WarnLevel.Normal))
					{
						m_project.Log.Warning.WriteLine($"[W{((int)(object)Logging.Log.WarningId.CSharpWarnings).ToString().PadLeft(4, '0')}] {message.ToString()}");
						warningsThrown = true;
						return;
					}
				}
				else
				{
					m_project.Log.ThrowWarning(Logging.Log.WarningId.CSharpWarnings, Logging.Log.WarnLevel.Normal, message.ToString());

					warningsThrown = m_project.Log.IsWarningEnabled(Logging.Log.WarningId.CSharpWarnings, Logging.Log.WarnLevel.Normal, out bool asError) && !asError;
					return;
				}
			}
			warningsThrown = false;
		}
#if NETFRAMEWORK
#else

		private void ProcessCompilerResults(EmitResult results, out bool warningsThrown, string assemblyFileName = null, Task task = null)
		{
			string taskPrefix = String.Empty;
			if (task != null)
			{
				taskPrefix = $"[{task.Name}] ";
			}

			string locationDescription = String.Empty;
			if (!String.IsNullOrEmpty(assemblyFileName))
			{
				locationDescription = $"[Assembly '{assemblyFileName}'] ";
			}
			else if (task != null)
			{
				locationDescription = $"[{task.Location.ToString().TrimEnd(':')}] "; // Location ToString adds a ':' we don't want
			}

			var hasErrors = results.Diagnostics.Any(x => x.Severity == DiagnosticSeverity.Error);
			var hasWarnings = results.Diagnostics.Any(x => x.Severity == DiagnosticSeverity.Warning);

			string failureDesciption = hasErrors ? "failed" : "warnings";

			if (hasErrors || hasWarnings)
			{
				StringBuilder message = new StringBuilder();
				message.AppendLine($"{taskPrefix}{locationDescription}compilation {failureDesciption}:");
				if (hasErrors)
				{
					foreach (var err in results.Diagnostics.Where(x => x.Severity == DiagnosticSeverity.Error))
					{
						message.AppendLine(err.ToString());
					}
				}
				else if (hasWarnings)
				{
					foreach (var err in results.Diagnostics.Where(x => x.Severity == DiagnosticSeverity.Warning))
					{
						message.AppendLine(err.ToString());
					}
				}

				if (hasErrors)
				{
					throw new BuildException(message.ToString(), _location);
				}
				else if (PlatformUtil.IsMonoRuntime)
				{
					// Mono version can be using an older System.dll "GAC" assembly which doesn't work well dealing with new system.* assemblies that is
					// download from NuGet.  For example:
					// On PC,  CSharpCodeProvider came from C:\WINDOWS\Microsoft.Net\assembly\GAC_MSIL\System\v4.0_4.0.0.0__b77a5c561934e089\System.dll
					// On OSX, CSharpCodeProvider came from /Library/Frameworks/Mono.framework/Versions/6.10.0/lib/mono/gac/System/4.0.0.0__b77a5c561934e089/System.dll
					// This difference is enough to throw a warning when there is a System.*.dll NuGet download such as System.Memory.
					// Which will result with warning message like:
					// "CS1685  The predefined type 'System.Span' is defined multiple times.  Using definition from 'mscorlib.dll'.
					// So for now, if it is under mono, we'll suppress warning as error but still show the warnings.
					if (m_project.Log.IsWarningEnabled(Logging.Log.WarningId.CSharpWarnings, Logging.Log.WarnLevel.Normal))
					{
						m_project.Log.Warning.WriteLine($"[W{((int)(object)Logging.Log.WarningId.CSharpWarnings).ToString().PadLeft(4, '0')}] {message.ToString()}");
						warningsThrown = true;
						return;
					}
				}
				else
				{
					m_project.Log.ThrowWarning(Logging.Log.WarningId.CSharpWarnings, Logging.Log.WarnLevel.Normal, message.ToString());

					warningsThrown = m_project.Log.IsWarningEnabled(Logging.Log.WarningId.CSharpWarnings, Logging.Log.WarnLevel.Normal, out bool asError) && !asError;
					return;
				}
			}
			warningsThrown = false;
		}
#endif
	}
}
