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
using System.Linq;
using System.IO;
#if NETFRAMEWORK
using System.Web.Razor;
#else
using Microsoft.AspNetCore.Razor.Language;
#endif
using System.Reflection;
using System.CodeDom;

using NAnt.Core;
using NAnt.Core.Util;
using NAnt.Core.Logging;
using NAnt.Core.Reflection;

using Model = EA.FrameworkTasks.Model;

namespace EA.Razor
{
	public class Razor<TTemplate, TModel> where TTemplate : RazorTemplate<TModel>
	{
		private readonly Log Log;
		private readonly Project Project;

		private static readonly IEnumerable<string> DefaultImports;

		private static readonly FileSet DefaultReferences;

		static Razor()
		{
			DefaultImports = new List<string>()
			{
				"System",
				"System.Text",
				"System.Collections.Generic",
				"System.Linq",
				"System.IO",

				"NAnt.Core",
				"NAnt.Core.Util",
				"NAnt.Core.Logging",
				"EA.FrameworkTasks.Model",
				"EA.Eaconfig",
				"EA.Eaconfig.Core",
				"EA.Eaconfig.Build",
				"EA.Eaconfig.Modules",
				"EA.Eaconfig.Modules.Tools",
				"EA.Eaconfig.Backends.Text"
			};

			DefaultReferences = new FileSet();
			DefaultReferences.Include("Microsoft.CSharp.dll", asIs: true);
		}



		public Razor(Project project)
		{
			Project = project;
			Log = project.Log;
		}


		public string RenderTemplate(PathString templateFile, TModel data, Model.IModule module, FileSet references = null, IEnumerable<string> imports = null)
		{
			if (!File.Exists(templateFile.Path))
			{
				throw new BuildException("Razor template file: '" + templateFile.Path + "' does not exist.");
			}
			var template = new StringReader(File.ReadAllText(templateFile.Path));

			// Expand static DefaultReferences fileset in a thread safe manner to avoid race conditions.
			lock (DefaultReferences)
			{
				VoidInt(DefaultReferences.FileItems.Count);
			}

			var referencedAssemblies = new FileSet();
			referencedAssemblies.Include(DefaultReferences);
			referencedAssemblies.Include(references);

			return RenderTemplate(template, data, module, referencedAssemblies, imports, templateFile.Path);
		}

		public string RenderTemplate(TextReader template,  TModel data, Model.IModule module, FileSet references = null, IEnumerable<string> imports = null, string templatePath=null)
		{
			string templateTxt = template.ReadToEnd().TrimWhiteSpace();

			GetNamespaceAndClass(templateTxt, out string namesp, out string classname);

			var templatevirtualpath = GetVirtualPath(templatePath, namesp, classname);
			if (!string.IsNullOrEmpty(templatePath))
			{
				// Make sure this dummy dll name correspond to a class with identical file content 
				// (classname is auto hash generated from file content).  If the "templatePath" is
				// null, the templatevirtualpath will already have the classname as the filename.
				templatevirtualpath += classname;
			}
			templatevirtualpath += ".dll";

			// The AssemblyLoader's TryGetCached and Add function uses dll filename as key.  However, the CompileAssembly
			// function creates auto-generated random assembly name that is different every time.  So we need to specify our
			// dummy dll filename to be used as key.  Otherwise, by default the Add function will use the random generated
			// Assembly name as key and we would end up keep compiling the same file again and again.
			string cachedAssemblyDll = Path.GetFileName(templatevirtualpath);

			if (!AssemblyLoader.TryGetCached(cachedAssemblyDll, out Assembly assembly))
			{
				assembly = CompileAssembly(new StringReader(templateTxt), references, GetAllImports(imports), templatevirtualpath, namesp, classname);

				if (assembly == null)
				{
					// Adding this to get better error checking.  We're already throwing a build exception below.
					// Like to check if the build exception is caused by compile failure.
					Log.Status.WriteLine("ERROR: Failed to compile Razor template {0} - this may be a .NET core upgrade issue", templatePath ?? templatevirtualpath);
				}

				AssemblyLoader.Add(assembly, templatevirtualpath, cachedAssemblyDll);

				// Try to get back the cached assembly. We can actually have multiple instances trying to do the same
				// compile but only one got added to the ConcurrentDictionary. So try to get back the one that actually get cached.
				Assembly cachedAssem;
				if (AssemblyLoader.TryGetCached(cachedAssemblyDll, out cachedAssem))
				{
					// There are possibility that we'll be throwing away the one we just compiled.  But if another thread
					// just added the same assembly compile, just use the one added so that the whole build will be consistent.
					assembly = cachedAssem;
				}
			}

			using (var generatedtemplate = CreateGeneratedTemplateObject(assembly, namesp, classname))
			{

			if (generatedtemplate == null)
			{
				throw new BuildException(String.Format("Failed to create Razor template"));
			}

			generatedtemplate.Model = data;
			generatedtemplate.Module = module;
			generatedtemplate.Project = Project;

			try
			{
				generatedtemplate.Execute();
			}
			catch (Exception ex)
			{
				var msg = String.Format("Failed execute template '{0}'{1}{2}{1} ---- ERROR ----", templatevirtualpath, Environment.NewLine, generatedtemplate.Result);

				throw new BuildException(msg, ex);
			}

			return generatedtemplate.Result;
			}
		}

		private void GetNamespaceAndClass(string template, out string  namesp, out string classname)
		{
			namesp = String.Format("EA.{0}.{1}", GetSafeTypeName(typeof(TTemplate)), GetSafeTypeName(typeof(TModel)));

			classname = String.Format("Template_{0}", Hash.MakeGUIDfromString(template).ToString("N"));
		}

		private string GetSafeTypeName(Type type)
		{
			var name = type.Name;

			var ind = String.IsNullOrEmpty(name) ? -1 : name.IndexOf('`');

			return ind < 0 ? name : name.Substring(0, ind);
		}

		private string GetVirtualPath(string templatePath, string namesp, string classname)
		{
			if(String.IsNullOrEmpty(templatePath))
			{
				return String.Format("templatepath://{0}/{1}", namesp, classname);
			}
			return templatePath;
		}

		IEnumerable<string> GetAllImports(IEnumerable<string> imports)
		{
			if (imports == null)
			{
				return DefaultImports;
			}

			return DefaultImports.Union(imports).OrderedDistinct();
		}

		private Assembly CompileAssembly(TextReader template, FileSet references, IEnumerable<string> imports, string virtualPath, string namesp, string className)
		{
#if NETFRAMEWORK
			var result = CreateRazor(namesp, className, imports).GenerateCode(template, className, namesp, virtualPath);
			return CompileGeneratedCode(result.GeneratedCode, references);
#else
			// Razor outside of ASPNetCore is a pain in .NET Core for some reason
			var fs = RazorProjectFileSystem.Create(".");;

			RazorProjectEngine engine = RazorProjectEngine.Create(RazorConfiguration.Default, fs, (builder) =>
			{
				builder.AddDefaultImports(imports.ToArray());
				builder.SetNamespace(namesp); // define a namespace for the Template class

				// old code looked like this: host.DefaultBaseClass = typeof(RazorTemplate<TModel>).FullName;
				// we can't use that because this looks like "EA.Razor.RazorTemplate`1[[EA.Eaconfig.Backends.VisualStudio.RazorModel_Manifest, EA.Tasks, Version=8.4.5.0, Culture=neutral, PublicKeyToken=null]]"
				// This is NOT a great solution, and can only deal with TModel being a nongeneric type (see https://stackoverflow.com/questions/2579734/get-the-type-name)
				builder.SetBaseType($"EA.Razor.RazorTemplate<{typeof(TModel).FullName}>");
				builder.ConfigureClass((doc, decl) =>
				{
					decl.ClassName = className;
				});
			});

			string defaultImports = string.Join("\n", imports.Select(x => $"using {x};")) + "\n";

			RazorSourceDocument document = RazorSourceDocument.Create(template.ReadToEnd(), virtualPath);

			RazorCodeDocument codeDocument = engine.Process(document, null, new RazorSourceDocument[0], new TagHelperDescriptor[0]);

			RazorCSharpDocument razorCSharpDocument = codeDocument.GetCSharpDocument();

			bool debug = false;
			Location location = Location.UnknownLocation;
			using (var compiler = new ScriptCompiler(Project, debug, location))
			{
				var codereferences = compiler.GetReferenceAssemblies(references);

				var code = defaultImports + razorCSharpDocument.GeneratedCode;

				return compiler.CompileAssemblyFromCode(code);
			}
#endif
		}

#if NETFRAMEWORK
		private RazorTemplateEngine CreateRazor(string namesp, string className, IEnumerable<string> imports)
		{
			var host = new RazorEngineHost(new CSharpRazorCodeLanguage());

			host.DefaultBaseClass = typeof(RazorTemplate<TModel>).FullName;
			host.DefaultClassName = className;
			host.DefaultNamespace = namesp;

			foreach (string ns in imports)
			{
				 host.NamespaceImports.Add(ns);
			}
			
			return new RazorTemplateEngine(host);
		}

		Assembly CompileGeneratedCode(CodeCompileUnit compileunit, FileSet additionalreferences)
		{
			Assembly assembly = null;

			bool debug = false;
			Location location = Location.UnknownLocation;

			using (var compiler = new ScriptCompiler(Project, debug, location))
			{
				var references = compiler.GetReferenceAssemblies(additionalreferences);

				var code = compiler.GenerateCode(compileunit);

				assembly = compiler.CompileAssemblyFromCode(code);
			}

			return assembly;
		}
#endif
		RazorTemplate<TModel> CreateGeneratedTemplateObject(Assembly assembly, string namesp, string className)
		{
			Type razorType = GetTemplateType(assembly, namesp, className);

			// We got some mysterious crash from QA.  Adding more debug log output if things went wrong!  We will
			// eventually get an error message after exiting this function.  But like to have more info on 
			// which step failed!
			if (razorType == null)
			{
				Log.Status.WriteLine("ERROR: Failed to get Razor template type {0}.{1} from Assembly {2}.", namesp, className, assembly.GetName().Name);
			}

			object objInstance = Activator.CreateInstance(razorType);

			if (objInstance == null)
			{
				Log.Status.WriteLine("ERROR: Failed to create Razor instance from razorType {0}", razorType.FullName);
			}

			// If we failed to cast as RazorTemplate class, the caller of this function is catching the returned object
			// being NULL.
			RazorTemplate<TModel> retval = objInstance as RazorTemplate<TModel>;
			if (retval == null)
			{
				Log.Status.WriteLine("ERROR: failed to cast Razor instance to RazorTemplate<{0}> from {1}", typeof(TModel).FullName, objInstance.GetType().FullName);
			}
			return retval;
		}

		Type GetTemplateType(Assembly assembly, string namesp, string className)
		{
			if (String.IsNullOrEmpty(namesp) || String.IsNullOrEmpty(className))
			{
				return assembly.GetTypes().FirstOrDefault();
			}
			return assembly.GetType(namesp + "." + className, true, false);
		}

		// To get rid of compiler warnings
		private void VoidInt(int i) { }
	}
}
