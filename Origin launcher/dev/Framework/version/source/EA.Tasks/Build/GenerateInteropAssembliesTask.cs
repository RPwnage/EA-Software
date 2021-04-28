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
using System.IO;
using System.Runtime.InteropServices;
using System.Runtime.InteropServices.ComTypes;

using NAnt.Core;
using NAnt.Core.Util;
using NAnt.Core.Attributes;
using NAnt.Core.Tasks;

namespace EA.Eaconfig
{
	/// <summary>Helper task to generate interop assemblies for a list of COM DLL's in a module.</summary>
	[TaskName("task-generatemoduleinteropassemblies")]
	public class GenerateModuleInteropAssembliesTask : Task
	{
		/// <summary>The name of the module</summary>
		[TaskAttribute("module", Required = true)]
		public string ModuleName { get; set; }

		/// <summary>The name of the group that the module is part of, defaults to runtime</summary>
		[TaskAttribute("group")]
		public string GroupName { get; set; } = "runtime";

		[TaskAttribute("generated-assemblies-fileset")]
		public string GeneratedAssembliesFilesetName { get; set; }

		public GenerateModuleInteropAssembliesTask() : base() { }

		public GenerateModuleInteropAssembliesTask(Project project, string module, string group = null, string generateAssembliesFileset = null)
			: base()
		{
			Project = project;
			ModuleName = module;
			GroupName = group;
			GeneratedAssembliesFilesetName = generateAssembliesFileset;
		}

		public static string GetTlbimpExe(Project project)
		{
			string tlbimpExe = project.GetPropertyOrDefault("tlbimp.exe", null);
			if (tlbimpExe == null)
			{
				tlbimpExe = project.GetPropertyOrDefault("package.WindowsSDK.tools.tlbimp", null);
				if (tlbimpExe == null)
				{
					int majorVersion = 0;
					if (int.TryParse(project.GetPropertyOrDefault("package.WindowsSDK.MajorVersion", "0"), out majorVersion) && majorVersion >= 8)
					{
						tlbimpExe = project.ExpandProperties("${package.WindowsSDK.dotnet.tools.dir}/tlbimp.exe");
					}
					else if (project.Properties.Contains("package.WindowsSDK.appdir"))
					{
						tlbimpExe = project.ExpandProperties("${package.WindowsSDK.appdir}/bin/tlbimp.exe");
					}
				}
				project.Properties["tlbimp.exe"] = tlbimpExe;
			}
			return tlbimpExe;
		}

		/* should we "regsvr32 -s ${comref}" to register the COM assembly?
		Pro: consistent with Visual Studio behaviour (I believe).
		Con: This only solves the problem on the build machine, not client machines.
		Decision: don't bother*/
		protected override void ExecuteTask()
		{
			string groupAndModule = GroupName + "." + ModuleName;

			FileSet generateAssembliesFileset = new FileSet();
			Project.NamedFileSets[GeneratedAssembliesFilesetName] = generateAssembliesFileset;

			string tlbimpExe = GetTlbimpExe(Project);
			if (!File.Exists(tlbimpExe))
			{
				throw new BuildException(string.Format("Unable to find tlbimp.exe in '{0}'", tlbimpExe));
			}

			bool moduleCopyLocal = Project.Properties.GetBooleanPropertyOrDefault(groupAndModule + ".copylocal", false);
			FileSet assemblies = Project.NamedFileSets[groupAndModule + ".assemblies"];
			FileSet comassemblies = Project.NamedFileSets[groupAndModule + ".comassemblies"];
			foreach (FileItem comref in comassemblies.FileItems)
			{
				string comrefBasename = Path.GetFileNameWithoutExtension(comref.FullPath);
				Project.Properties["comref-basename"] = comrefBasename;
				string netref = Project.ExpandProperties("${nant.project.temproot}/${comref-basename}.net.dll/${config}/${comref-basename}.net.dll");
				Project.Properties["netref"] = netref;

				// Generate .NET wrapper assembly.
				new SlnRuntimeGenerateInteropAssemblyTask(Project, comDll: comref.FullPath, interopDll: netref, _namespace: comrefBasename,
					message: string.Format("Generating interop assembly for {0} ...", comref.FullPath)).Execute();

				// Add generated interop assembly to the build.
				assemblies.Include(netref);
				generateAssembliesFileset.Include(netref);

				// Copy the COM DLL to the build folder.
				if (moduleCopyLocal)
				{
					new CopyTask(Project, file: comref.FileName, todir: Project.Properties["package.configbindir"]);
				}
			}
		}
	}

	/// <summary>Generates a single interop assembly from a COM DLL. Dependency-checking is performed.</summary>
	[TaskName("slnruntime-generateinteropassembly")]
	public class SlnRuntimeGenerateInteropAssemblyTask : Task
	{
		[TaskAttribute("comdll", Required = true)]
		public string ComDll { get; set; }

		[TaskAttribute("interopdll", Required = true)]
		public string InteropDll { get; set; }

		[TaskAttribute("namespace", Required = true)]
		public string Namespace { get; set; }

		[TaskAttribute("keyfile")]
		public string KeyFile { get; set; } = string.Empty;

		[TaskAttribute("message")]
		public string Message { get; set; } = string.Empty;

		public SlnRuntimeGenerateInteropAssemblyTask() : base() { }

		public SlnRuntimeGenerateInteropAssemblyTask(Project project, string comDll, string interopDll, string _namespace,
			string keyFile = "", string message = "")
			: base()
		{
			Project = project;
			ComDll = comDll;
			InteropDll = interopDll;
			Namespace = _namespace;
			KeyFile = keyFile;
			Message = message;
		}

		protected override void ExecuteTask()
		{
			string tlbimpExe = GenerateModuleInteropAssembliesTask.GetTlbimpExe(Project);
			if (!File.Exists(tlbimpExe))
			{
				throw new BuildException(string.Format("Unable to find tlbimp.exe in '{0}'", tlbimpExe));
			}

			FileSet dependsInputs = new FileSet();
			dependsInputs.Include(ComDll);
			FileSet dependsOutputs = new FileSet();
			dependsOutputs.Include(InteropDll, failonmissing: false);
			new DependsTask(Project, "needs-generating", dependsInputs, dependsOutputs).Execute();

			bool verbose = Project.Properties.GetBooleanPropertyOrDefault("nant.verbose", false);
			if (Project.Properties.GetBooleanPropertyOrDefault("needs-generating", false))
			{
				// Need 'package.DotNet.referencedir', DotNet is probably already depended on by this point if you are calling this task.
				TaskUtil.Dependent(Project, "DotNet");

				try
				{
					// We probably don't need to use trycatch block after we added the do.once block. But keeping the trycatch block as is for now.
					DoOnce.Execute(Project, string.Format("tlbimp.exe_building_{0}", InteropDll), () =>
					{
						string verbosityArg = "/silent";
						if (verbose) verbosityArg = "/verbose";

						// determine whether or not a keyfile has been specified
						string keyFileArg = "";
						if (!KeyFile.IsNullOrEmpty())
						{
							keyFileArg = string.Format("\"/keyfile:{0}\"", KeyFile);
						}

						new GetComLibraryNameTask(Project, ComDll, Namespace + ".real_namespace").Execute();

						ArgumentSet args = new ArgumentSet();
						args.Add(ComDll);
						args.Add("/out:" + InteropDll);
						args.Add("/namespace:" + Project.Properties[Namespace + ".real_namespace"]);
						args.Add("/nologo");
						args.Add(keyFileArg);
						args.Add(verbosityArg);

						// We notice that using the /verbose switch requires us to add the .NET GAC assembly folder to the PATH environment, otherwise
						// tlbimp.exe would fail to load tlbref.dll.  Not sure why this happens.  Maybe because we are using non-proxy WindowsSDK?
						OptionSet env = new OptionSet();
						env.Options.Add("PATH", Project.ExpandProperties("${sys.env.PATH};${package.DotNet.bindir}"));

						new ExecTask(Project, program: tlbimpExe, silent: !verbose, args: args, env: env).Execute();
					}, isblocking: true);

					if (!File.Exists(InteropDll))
					{
						throw new BuildException(string.Format("Failed to create '{0}'. " +
							"You might have a previous thread created the interop dll but then got deleted by another thread. " +
							"This interop dll creation is only executed once in the same process because of the 'do.once' instruction. ",
							InteropDll));
					}
				}
				catch (Exception ex)
				{
					// If file already exists, most likely another thread got here trying to create the same interop assembly 
					// at the same time and locked access to this file. Just ignore this error!
					if (!File.Exists(InteropDll))
					{
						throw new BuildException(string.Format("Unable to execute tlbimp.exe to generate '{0}'. Error was: {1}", InteropDll, ex.Message), ex);
					}
				}
			}
			else if (verbose)
			{
				Log.Status.WriteLine("Interop DLL {0} is up-to-date.", InteropDll);
			}
		}
	}

	[TaskName("GetComLibraryName")]
	public class GetComLibraryNameTask : Task
	{
		[TaskAttribute("libpath", Required = true)]
		public string LibPath { get; set; }

		/// <summary>The property where the result will be stored</summary>
		[TaskAttribute("Result", Required = true)]
		public string Result { get; set; }

		public GetComLibraryNameTask() : base() { }

		public GetComLibraryNameTask(Project project, string libPath, string result) 
			: base()
		{
			Project = project;
			LibPath = libPath;
			Result = result;
		}

		public enum RegKind
		{
			RegKind_Default = 0,
			RegKind_Register = 1,
			RegKind_None = 2
		}

		[DllImport("oleaut32.dll", CharSet = CharSet.Unicode, PreserveSig = false)]
		private static extern void LoadTypeLibEx(string strTypeLibName, RegKind regKind, out ITypeLib typeLib);

		// separated into a separate method mainly for testing convienience.
		protected virtual ITypeLib LoadTypeLib(string strTypeLibName, RegKind regKind)
		{
			ITypeLib typeLib = null;
			try
			{
				LoadTypeLibEx(strTypeLibName, regKind, out typeLib);
			}
			catch (Exception ex)
			{
				new BuildException(String.Format("Unable to generate interop assembly for COM reference '{0}'.", LibPath), ex);
			}
			if (typeLib == null)
			{
				new BuildException(String.Format("Unable to generate interop assembly for COM reference '{0}'.", LibPath));
			}
			return typeLib;
		}

		protected override void ExecuteTask()
		{
			if (!File.Exists(LibPath))
			{
				throw new BuildException(String.Format("Unable to generate interop assembly for COM reference, file '{0}' not found", LibPath));
			}

			ITypeLib typeLib = LoadTypeLib(LibPath, RegKind.RegKind_None);
#if NETFRAMEWORK
			Project.Properties[Result] = Marshal.GetTypeLibName(typeLib);
#else
			// copied from reference source...
			typeLib.GetDocumentation(-1, out string libName, out string unused1, out int unused2, out string unused3);
			Project.Properties[Result] = libName;
#endif
		}
	}
}
