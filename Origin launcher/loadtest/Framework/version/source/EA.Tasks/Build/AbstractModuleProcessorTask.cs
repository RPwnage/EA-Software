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

using NAnt.Core;
using NAnt.Core.Logging;
using NAnt.Core.Util;

using EA.Eaconfig.Modules;
using EA.Eaconfig.Modules.Tools;
using EA.FrameworkTasks.Model;
using EA.Tasks.Model;

namespace EA.Eaconfig.Core
{
	public abstract class AbstractModuleProcessorTask : Task, IBuildModuleTask, IModuleProcessor
	{
		public ProcessableModule Module { get; set; }

		public Tool Tool { get; set; }

		public OptionSet BuildTypeOptionSet { get; set; }

		protected override void ExecuteTask()
		{
			if (Tool != null)
			{
				ProcessTool(Tool);
			}
			else
			{
				Module.Apply(this);
				foreach(var tool in Module.Tools)
				{
					ProcessTool(tool);
				}
				ProcessModuleCustomTools(); // iterates over files in Module.Tools, do this last as previous steps (.Apply and .ProcessTool) made have modified these filesets

			}
		}

		private void ProcessModuleCustomTools()
		{
			// native modules can have custom options on asm and cc files, custombuildfiles are handled in Module.Tools
			Module_Native nativeModule = Module as Module_Native;
			if (nativeModule != null)
			{
				if (nativeModule.Cc != null && nativeModule.Cc.SourceFiles != null)
				{
					foreach (CcCompiler customCcCompiler in GetCustomTools(nativeModule.Cc, nativeModule.Cc.SourceFiles))
					{
						ProcessCustomTool(customCcCompiler);
					}
				}
				if (nativeModule.Asm != null && nativeModule.Asm.SourceFiles != null)
				{
					foreach (AsmCompiler customAsmCompiler in GetCustomTools(nativeModule.Asm, nativeModule.Asm.SourceFiles))
					{
						ProcessCustomTool(customAsmCompiler);
					}
				}
			}

			// dot net modules can have a Csc or Fsc compiler and we only deal in concrete types, handle each with explicit override
			// of ProcessCustomTool
			Module_DotNet dotNetModule = Module as Module_DotNet;
			if (dotNetModule != null)
			{
				if (dotNetModule.Compiler != null && dotNetModule.Compiler.SourceFiles != null)
				{
					foreach (DotNetCompiler customDotNetCompiler in GetCustomTools(dotNetModule.Compiler, dotNetModule.Compiler.SourceFiles))
					{
						CscCompiler customCscCompiler = customDotNetCompiler as CscCompiler;
						FscCompiler customFscCompiler = customDotNetCompiler as FscCompiler;
						if (customCscCompiler != null)
						{
							ProcessCustomTool(customCscCompiler);
						}
						else if (customFscCompiler != null)
						{
							ProcessCustomTool(customFscCompiler);
						}
						else
						{
							throw new BuildException(String.Format("Unknown DotNetCompiler type '{0}'!", customDotNetCompiler.GetType().ToString()));
						}
					}
				}
			}
		}

		protected void AddDefaultPlatformIndependentDotNetCoreAssemblies(Module_DotNet module)
		{
			if (module.TargetFrameworkFamily == DotNetFrameworkFamilies.Core)
			{
				// adding mscorlib explicitly, mimicing VS default arguments
				module.Compiler.Options.Add("/nostdlib+");

				// this list was derrived from all the default assemblies visualstudio / msbuild will pass to .net core projects on windows
				module.Compiler.Assemblies.Include("Microsoft.CSharp.dll", asIs: true);
				module.Compiler.Assemblies.Include("Microsoft.VisualBasic.Core.dll", asIs: true);
				module.Compiler.Assemblies.Include("Microsoft.VisualBasic.dll", asIs: true);
				module.Compiler.Assemblies.Include("Microsoft.Win32.Primitives.dll", asIs: true);
				module.Compiler.Assemblies.Include("mscorlib.dll", asIs: true);
				module.Compiler.Assemblies.Include("netstandard.dll", asIs: true);
				module.Compiler.Assemblies.Include("System.AppContext.dll", asIs: true);
				module.Compiler.Assemblies.Include("System.Buffers.dll", asIs: true);
				module.Compiler.Assemblies.Include("System.Collections.Concurrent.dll", asIs: true);
				module.Compiler.Assemblies.Include("System.Collections.dll", asIs: true);
				module.Compiler.Assemblies.Include("System.Collections.Immutable.dll", asIs: true);
				module.Compiler.Assemblies.Include("System.Collections.NonGeneric.dll", asIs: true);
				module.Compiler.Assemblies.Include("System.Collections.Specialized.dll", asIs: true);
				module.Compiler.Assemblies.Include("System.ComponentModel.Annotations.dll", asIs: true);
				module.Compiler.Assemblies.Include("System.ComponentModel.DataAnnotations.dll", asIs: true);
				module.Compiler.Assemblies.Include("System.ComponentModel.dll", asIs: true);
				module.Compiler.Assemblies.Include("System.ComponentModel.EventBasedAsync.dll", asIs: true);
				module.Compiler.Assemblies.Include("System.ComponentModel.Primitives.dll", asIs: true);
				module.Compiler.Assemblies.Include("System.ComponentModel.TypeConverter.dll", asIs: true);
				module.Compiler.Assemblies.Include("System.Configuration.dll", asIs: true);
				module.Compiler.Assemblies.Include("System.Console.dll", asIs: true);
				module.Compiler.Assemblies.Include("System.Core.dll", asIs: true);
				module.Compiler.Assemblies.Include("System.Data.Common.dll", asIs: true);
				module.Compiler.Assemblies.Include("System.Data.DataSetExtensions.dll", asIs: true);
				module.Compiler.Assemblies.Include("System.Data.dll", asIs: true);
				module.Compiler.Assemblies.Include("System.Diagnostics.Contracts.dll", asIs: true);
				module.Compiler.Assemblies.Include("System.Diagnostics.Debug.dll", asIs: true);
				module.Compiler.Assemblies.Include("System.Diagnostics.DiagnosticSource.dll", asIs: true);
				module.Compiler.Assemblies.Include("System.Diagnostics.FileVersionInfo.dll", asIs: true);
				module.Compiler.Assemblies.Include("System.Diagnostics.Process.dll", asIs: true);
				module.Compiler.Assemblies.Include("System.Diagnostics.StackTrace.dll", asIs: true);
				module.Compiler.Assemblies.Include("System.Diagnostics.TextWriterTraceListener.dll", asIs: true);
				module.Compiler.Assemblies.Include("System.Diagnostics.Tools.dll", asIs: true);
				module.Compiler.Assemblies.Include("System.Diagnostics.TraceSource.dll", asIs: true);
				module.Compiler.Assemblies.Include("System.Diagnostics.Tracing.dll", asIs: true);
				module.Compiler.Assemblies.Include("System.dll", asIs: true);
				module.Compiler.Assemblies.Include("System.Drawing.dll", asIs: true);
				module.Compiler.Assemblies.Include("System.Drawing.Primitives.dll", asIs: true);
				module.Compiler.Assemblies.Include("System.Dynamic.Runtime.dll", asIs: true);
				module.Compiler.Assemblies.Include("System.Globalization.Calendars.dll", asIs: true);
				module.Compiler.Assemblies.Include("System.Globalization.dll", asIs: true);
				module.Compiler.Assemblies.Include("System.Globalization.Extensions.dll", asIs: true);
				module.Compiler.Assemblies.Include("System.IO.Compression.Brotli.dll", asIs: true);
				module.Compiler.Assemblies.Include("System.IO.Compression.dll", asIs: true);
				module.Compiler.Assemblies.Include("System.IO.Compression.FileSystem.dll", asIs: true);
				module.Compiler.Assemblies.Include("System.IO.Compression.ZipFile.dll", asIs: true);
				module.Compiler.Assemblies.Include("System.IO.dll", asIs: true);
				module.Compiler.Assemblies.Include("System.IO.FileSystem.dll", asIs: true);
				module.Compiler.Assemblies.Include("System.IO.FileSystem.DriveInfo.dll", asIs: true);
				module.Compiler.Assemblies.Include("System.IO.FileSystem.Primitives.dll", asIs: true);
				module.Compiler.Assemblies.Include("System.IO.FileSystem.Watcher.dll", asIs: true);
				module.Compiler.Assemblies.Include("System.IO.IsolatedStorage.dll", asIs: true);
				module.Compiler.Assemblies.Include("System.IO.MemoryMappedFiles.dll", asIs: true);
				module.Compiler.Assemblies.Include("System.IO.Pipes.dll", asIs: true);
				module.Compiler.Assemblies.Include("System.IO.UnmanagedMemoryStream.dll", asIs: true);
				module.Compiler.Assemblies.Include("System.Linq.dll", asIs: true);
				module.Compiler.Assemblies.Include("System.Linq.Expressions.dll", asIs: true);
				module.Compiler.Assemblies.Include("System.Linq.Parallel.dll", asIs: true);
				module.Compiler.Assemblies.Include("System.Linq.Queryable.dll", asIs: true);
				module.Compiler.Assemblies.Include("System.Memory.dll", asIs: true);
				module.Compiler.Assemblies.Include("System.Net.dll", asIs: true);
				module.Compiler.Assemblies.Include("System.Net.Http.dll", asIs: true);
				module.Compiler.Assemblies.Include("System.Net.HttpListener.dll", asIs: true);
				module.Compiler.Assemblies.Include("System.Net.Mail.dll", asIs: true);
				module.Compiler.Assemblies.Include("System.Net.NameResolution.dll", asIs: true);
				module.Compiler.Assemblies.Include("System.Net.NetworkInformation.dll", asIs: true);
				module.Compiler.Assemblies.Include("System.Net.Ping.dll", asIs: true);
				module.Compiler.Assemblies.Include("System.Net.Primitives.dll", asIs: true);
				module.Compiler.Assemblies.Include("System.Net.Requests.dll", asIs: true);
				module.Compiler.Assemblies.Include("System.Net.Security.dll", asIs: true);
				module.Compiler.Assemblies.Include("System.Net.ServicePoint.dll", asIs: true);
				module.Compiler.Assemblies.Include("System.Net.Sockets.dll", asIs: true);
				module.Compiler.Assemblies.Include("System.Net.WebClient.dll", asIs: true);
				module.Compiler.Assemblies.Include("System.Net.WebHeaderCollection.dll", asIs: true);
				module.Compiler.Assemblies.Include("System.Net.WebProxy.dll", asIs: true);
				module.Compiler.Assemblies.Include("System.Net.WebSockets.Client.dll", asIs: true);
				module.Compiler.Assemblies.Include("System.Net.WebSockets.dll", asIs: true);
				module.Compiler.Assemblies.Include("System.Numerics.dll", asIs: true);
				module.Compiler.Assemblies.Include("System.Numerics.Vectors.dll", asIs: true);
				module.Compiler.Assemblies.Include("System.ObjectModel.dll", asIs: true);
				module.Compiler.Assemblies.Include("System.Reflection.DispatchProxy.dll", asIs: true);
				module.Compiler.Assemblies.Include("System.Reflection.dll", asIs: true);
				module.Compiler.Assemblies.Include("System.Reflection.Emit.dll", asIs: true);
				module.Compiler.Assemblies.Include("System.Reflection.Emit.ILGeneration.dll", asIs: true);
				module.Compiler.Assemblies.Include("System.Reflection.Emit.Lightweight.dll", asIs: true);
				module.Compiler.Assemblies.Include("System.Reflection.Extensions.dll", asIs: true);
				module.Compiler.Assemblies.Include("System.Reflection.Metadata.dll", asIs: true);
				module.Compiler.Assemblies.Include("System.Reflection.Primitives.dll", asIs: true);
				module.Compiler.Assemblies.Include("System.Reflection.TypeExtensions.dll", asIs: true);
				module.Compiler.Assemblies.Include("System.Resources.Reader.dll", asIs: true);
				module.Compiler.Assemblies.Include("System.Resources.ResourceManager.dll", asIs: true);
				module.Compiler.Assemblies.Include("System.Resources.Writer.dll", asIs: true);
				module.Compiler.Assemblies.Include("System.Runtime.CompilerServices.Unsafe.dll", asIs: true);
				module.Compiler.Assemblies.Include("System.Runtime.CompilerServices.VisualC.dll", asIs: true);
				module.Compiler.Assemblies.Include("System.Runtime.dll", asIs: true);
				module.Compiler.Assemblies.Include("System.Runtime.Extensions.dll", asIs: true);
				module.Compiler.Assemblies.Include("System.Runtime.Handles.dll", asIs: true);
				module.Compiler.Assemblies.Include("System.Runtime.InteropServices.dll", asIs: true);
				module.Compiler.Assemblies.Include("System.Runtime.InteropServices.RuntimeInformation.dll", asIs: true);
				module.Compiler.Assemblies.Include("System.Runtime.InteropServices.WindowsRuntime.dll", asIs: true);
				module.Compiler.Assemblies.Include("System.Runtime.Intrinsics.dll", asIs: true);
				module.Compiler.Assemblies.Include("System.Runtime.Loader.dll", asIs: true);
				module.Compiler.Assemblies.Include("System.Runtime.Numerics.dll", asIs: true);
				module.Compiler.Assemblies.Include("System.Runtime.Serialization.dll", asIs: true);
				module.Compiler.Assemblies.Include("System.Runtime.Serialization.Formatters.dll", asIs: true);
				module.Compiler.Assemblies.Include("System.Runtime.Serialization.Json.dll", asIs: true);
				module.Compiler.Assemblies.Include("System.Runtime.Serialization.Primitives.dll", asIs: true);
				module.Compiler.Assemblies.Include("System.Runtime.Serialization.Xml.dll", asIs: true);
				module.Compiler.Assemblies.Include("System.Security.Claims.dll", asIs: true);
				module.Compiler.Assemblies.Include("System.Security.Cryptography.Algorithms.dll", asIs: true);
				module.Compiler.Assemblies.Include("System.Security.Cryptography.Csp.dll", asIs: true);
				module.Compiler.Assemblies.Include("System.Security.Cryptography.Encoding.dll", asIs: true);
				module.Compiler.Assemblies.Include("System.Security.Cryptography.Primitives.dll", asIs: true);
				module.Compiler.Assemblies.Include("System.Security.Cryptography.X509Certificates.dll", asIs: true);
				module.Compiler.Assemblies.Include("System.Security.dll", asIs: true);
				module.Compiler.Assemblies.Include("System.Security.Principal.dll", asIs: true);
				module.Compiler.Assemblies.Include("System.Security.SecureString.dll", asIs: true);
				module.Compiler.Assemblies.Include("System.ServiceModel.Web.dll", asIs: true);
				module.Compiler.Assemblies.Include("System.ServiceProcess.dll", asIs: true);
				module.Compiler.Assemblies.Include("System.Text.Encoding.CodePages.dll", asIs: true);
				module.Compiler.Assemblies.Include("System.Text.Encoding.dll", asIs: true);
				module.Compiler.Assemblies.Include("System.Text.Encoding.Extensions.dll", asIs: true);
				module.Compiler.Assemblies.Include("System.Text.Encodings.Web.dll", asIs: true);
				module.Compiler.Assemblies.Include("System.Text.Json.dll", asIs: true);
				module.Compiler.Assemblies.Include("System.Text.RegularExpressions.dll", asIs: true);
				module.Compiler.Assemblies.Include("System.Threading.Channels.dll", asIs: true);
				module.Compiler.Assemblies.Include("System.Threading.dll", asIs: true);
				module.Compiler.Assemblies.Include("System.Threading.Overlapped.dll", asIs: true);
				module.Compiler.Assemblies.Include("System.Threading.Tasks.Dataflow.dll", asIs: true);
				module.Compiler.Assemblies.Include("System.Threading.Tasks.dll", asIs: true);
				module.Compiler.Assemblies.Include("System.Threading.Tasks.Extensions.dll", asIs: true);
				module.Compiler.Assemblies.Include("System.Threading.Tasks.Parallel.dll", asIs: true);
				module.Compiler.Assemblies.Include("System.Threading.Thread.dll", asIs: true);
				module.Compiler.Assemblies.Include("System.Threading.ThreadPool.dll", asIs: true);
				module.Compiler.Assemblies.Include("System.Threading.Timer.dll", asIs: true);
				module.Compiler.Assemblies.Include("System.Transactions.dll", asIs: true);
				module.Compiler.Assemblies.Include("System.Transactions.Local.dll", asIs: true);
				module.Compiler.Assemblies.Include("System.ValueTuple.dll", asIs: true);
				module.Compiler.Assemblies.Include("System.Web.dll", asIs: true);
				module.Compiler.Assemblies.Include("System.Web.HttpUtility.dll", asIs: true);
				module.Compiler.Assemblies.Include("System.Windows.dll", asIs: true);
				module.Compiler.Assemblies.Include("System.Xml.dll", asIs: true);
				module.Compiler.Assemblies.Include("System.Xml.Linq.dll", asIs: true);
				module.Compiler.Assemblies.Include("System.Xml.ReaderWriter.dll", asIs: true);
				module.Compiler.Assemblies.Include("System.Xml.Serialization.dll", asIs: true);
				module.Compiler.Assemblies.Include("System.Xml.XDocument.dll", asIs: true);
				module.Compiler.Assemblies.Include("System.Xml.XmlDocument.dll", asIs: true);
				module.Compiler.Assemblies.Include("System.Xml.XmlSerializer.dll", asIs: true);
				module.Compiler.Assemblies.Include("System.Xml.XPath.dll", asIs: true);
				module.Compiler.Assemblies.Include("System.Xml.XPath.XDocument.dll", asIs: true);
				module.Compiler.Assemblies.Include("WindowsBase.dll", asIs: true);
			}
			else if (module.TargetFrameworkFamily == DotNetFrameworkFamilies.Standard)
			{
				throw new NotImplementedException(); // nuget-todo
			}
		}

		protected string PropGroupName(string name)
		{
			return Module.PropGroupName(name);
		}

		protected string PropGroupValue(string name, string defVal = "")
		{
			return Module.PropGroupValue(name, defVal);
		}

		protected FileSet PropGroupFileSet(string name)
		{
			return Module.PropGroupFileSet(name);
		}

		protected FileSet AddModuleFiles(Module module, string propertyname, params string[] defaultfilters)
		{
			FileSet dest = new FileSet(Project);
			if (0 < AddModuleFiles(dest, module, propertyname, defaultfilters))
			{
				return dest;
			}
			return null;
		}

		internal int AddModuleFiles(FileSet dest, Module module, string propertyname, params string[] defaultfilters)
		{
			int appended = 0;
			if (dest != null)
			{
				appended += dest.AppendWithBaseDir(PropGroupFileSet(propertyname));
				appended += dest.AppendWithBaseDir(PropGroupFileSet(propertyname + "." + module.Configuration.System));
				if (appended == 0 && defaultfilters != null)
				{
					foreach (string filter in defaultfilters)
					{
						dest.Include(Project.ExpandProperties(filter));
						appended++;
					}
				}
			}
			return appended;
		}


		public virtual void Process(Module_Native module)
		{
		}

		public virtual void Process(Module_DotNet module)
		{
		}

		public virtual void Process(Module_Utility module)
		{
		}

		public virtual void Process(Module_MakeStyle module)
		{
		}

		public virtual void Process(Module_Python module)
		{
		}

		public virtual void Process(Module_ExternalVisualStudio module)
		{
		}

		public virtual void Process(Module_Java module)
		{
		}

		public virtual void Process(Module_UseDependency module)
		{
		}

		public virtual void ProcessTool(CcCompiler cc)
		{
		}

		public virtual void ProcessCustomTool(CcCompiler cc)
		{
		}

		public virtual void ProcessTool(AsmCompiler asm)
		{
		}

		public virtual void ProcessCustomTool(AsmCompiler customAsmCompiler)
		{
		}

		public virtual void ProcessTool(Linker link)
		{
		}

		public virtual void ProcessTool(Librarian lib)
		{
		}

		public virtual void ProcessTool(BuildTool buildtool)
		{
		}

		public virtual void ProcessTool(CscCompiler csc)
		{
		}

		public virtual void ProcessCustomTool(CscCompiler customCscCompiler)
		{
		}

		public virtual void ProcessTool(FscCompiler fsc)
		{
		}

		public virtual void ProcessCustomTool(FscCompiler customFscCompiler)
		{
		}

		public virtual void ProcessTool(PostLink postlink)
		{
		}

		public virtual void ProcessTool(AppPackageTool apppackage)
		{
		}

		public virtual void ProcessTool(PythonInterpreter pythonTool)
		{
		}

		private void ProcessTool(Tool tool)
		{
			if (tool is CcCompiler)
				ProcessTool(tool as CcCompiler);
			else if (tool is AsmCompiler)
				ProcessTool(tool as AsmCompiler);
			else if (tool is Linker)
				ProcessTool(tool as Linker);
			else if (tool is Librarian)
				ProcessTool(tool as Librarian);
			else if (tool is BuildTool)
				ProcessTool(tool as BuildTool);
			else if (tool is CscCompiler)
				ProcessTool(tool as CscCompiler);
			else if (tool is FscCompiler)
				ProcessTool(tool as FscCompiler);
			else if (tool is PostLink)
				ProcessTool(tool as PostLink);
			else if (tool is AppPackageTool)
				ProcessTool(tool as AppPackageTool);
			else if (tool is PythonInterpreter)
				ProcessTool(tool as PythonInterpreter);
			else if (tool == null)
			{
				Log.Error.WriteLine("INTERNAL ERROR: module '{0}', task '{1}' null tool", Module.Name, Name);
			}
			else
				Log.Warning.WriteLine("module '{0}', task '{1}' unknown tool '{2}'", Module.Name, Name, tool.GetType().Name);
		}

		// search a fileset for any includes that have an associated tool that is not the same as the default module's
		// tool, for example sourcefiles that have a custom compiler
		private static ToolType[] GetCustomTools<ToolType>(ToolType defaultTool, FileSet toolFiles) where ToolType : Tool
		{
			HashSet<ToolType> customTools = new HashSet<ToolType>();
			foreach (FileItem file in toolFiles.FileItems)
			{
				FileData fileData = file.Data as FileData;
				if (fileData != null)
				{
					ToolType customTool = fileData.Tool as ToolType;
					if (customTool != null && customTool != defaultTool)
					{
						customTools.Add(customTool);
					}
				}
			}
			return customTools.ToArray();
		}
	}
}
