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
using System.Web.Script.Serialization;
#else
using Newtonsoft.Json;
#endif

using NAnt.Core;
using NAnt.Core.Util;
using NAnt.Core.Attributes;
using NAnt.Core.Writers;
using EA.FrameworkTasks.Model;
using EA.Eaconfig.Modules.Tools;
using EA.Eaconfig.Core;
using EA.Eaconfig.Modules;
using EA.Eaconfig.Build;

namespace EA.Eaconfig
{
	/// <summary>Generates a Clang Compilation Database</summary>
	[TaskName("ClangCompilationDatabase")]
	public class ClangCompilationDatabase : Task
	{
		[TaskAttribute("outputdir", Required = false)]
		public string OutputDir
		{
			get
			{
				return !String.IsNullOrEmpty(_outputdir) ? _outputdir : Path.Combine(Project.GetFullPath(Project.ExpandProperties(Properties[Project.NANT_PROPERTY_PROJECT_BUILDROOT])), "buildinfo", "compilecommands");
			}
			set { _outputdir = value; }
		}
		private string _outputdir;

		private bool _isFirstLine;

		public ClangCompilationDatabase() : base("ClangCompilationDatabase") { }

		protected override void ExecuteTask()
		{
			Log.Minimal.Write(LogPrefix + "clang-compilation-database task running!\n");

			var timer = new Chrono();

			foreach (var topModulesByConfig in Project.BuildGraph().TopModules.GroupBy(m => m.Configuration.Name))
			{
				Log.Status.WriteLine(LogPrefix + "  {0} Config : {1}", timer.ToString(), topModulesByConfig.Key);
				WriteOneConfig(topModulesByConfig, PathString.MakeCombinedAndNormalized(OutputDir, topModulesByConfig.Key));
			}

			Log.Minimal.WriteLine(LogPrefix + "  {0} buildinfo directory : {1}", timer.ToString(), OutputDir.Quote());
		}

		public void WriteOneConfig(IEnumerable<IModule> topmodules, PathString dir)
		{
			var allmodules = topmodules.Union(topmodules.SelectMany(m => m.Dependents.FlattenAll(DependencyTypes.Build), (m, d) => d.Dependent).Where(m => !(m is Module_UseDependency)));

			using (var compileCommandsWriter = new MakeWriter(writeBOM: false))
			using (var cppWriter = new MakeWriter(writeBOM: false))
			using (var cppClrWriter = new MakeWriter(writeBOM: false))
			using (var csWriter = new MakeWriter(writeBOM: false))
			using (var generatedWriter = new MakeWriter(writeBOM: false))
			{
				Log.Status.WriteLine(LogPrefix + "    Writing:");
				compileCommandsWriter.FileName = Path.Combine(dir.Path, "compile_commands.json");
				Log.Status.WriteLine(LogPrefix + "      " + compileCommandsWriter.FileName);
				cppWriter.FileName = Path.Combine(dir.Path, "cpp_files.lst");
				Log.Status.WriteLine(LogPrefix + "      " + cppWriter.FileName);
				cppClrWriter.FileName = Path.Combine(dir.Path, "cpp_clr_files.lst");
				Log.Status.WriteLine(LogPrefix + "      " + cppClrWriter.FileName);
				csWriter.FileName = Path.Combine(dir.Path, "cs_files.lst");
				Log.Status.WriteLine(LogPrefix + "      " + csWriter.FileName);
				generatedWriter.FileName = Path.Combine(dir.Path, "generated_files.lst");
				Log.Status.WriteLine(LogPrefix + "      " + generatedWriter.FileName);

				compileCommandsWriter.WriteLine("[");
				_isFirstLine = true;
				foreach (var module in allmodules)
				{
					Log.Status.WriteLine(LogPrefix + "      Package : {0}", module.Package.Name);

					WriteModule(module as Module, compileCommandsWriter, cppWriter, cppClrWriter, csWriter, generatedWriter);
				}
				compileCommandsWriter.WriteLine("]");
			}
		}

		private void WriteModule(Module module, IMakeWriter compileCommandsWriter, IMakeWriter cppWriter, IMakeWriter cppClrWriter, IMakeWriter csWriter, IMakeWriter generatedWriter)
		{
			// Want to be able to apply ownership, so really don't like when we
			// don't have a .build file with a full path.
			var scriptFile = module.ScriptFile != null ?
				module.ScriptFile.ToString().Replace('\\', '/').ToLower() :
				module.Package.Name;
			foreach (var tool in module.Tools)
			{
				WriteTool(compileCommandsWriter, cppWriter, cppClrWriter, csWriter, generatedWriter, tool, module.OutputDir, scriptFile);
			}
		}

		private class CompileStep
		{
			public string directory;
			public string command;
			public string file;
		}

		private static CcCompiler GetCompiler(CcCompiler globalCompiler, FileItem fileItem)
		{
			CcCompiler compiler = null;

			if (fileItem != null && fileItem.OptionSetName != null)
			{
				compiler = fileItem.GetTool() as CcCompiler;
			}
			return compiler ?? globalCompiler;
		}

		private void WriteTool(IMakeWriter compileCommandsWriter, IMakeWriter cppWriter, IMakeWriter cppClrWriter, IMakeWriter csWriter, IMakeWriter generatedWriter, Tool tool, PathString directory, string scriptFile)
		{
			if (tool is CcCompiler)
			{
				WriteTool(compileCommandsWriter, cppWriter, cppClrWriter, tool as CcCompiler, directory, scriptFile);
			}
			else if (tool is CscCompiler)
			{
				WriteTool(csWriter, tool as CscCompiler, scriptFile);
			}
			else if (tool is BuildTool)
			{
				WriteTool(generatedWriter, tool as BuildTool, scriptFile);
			}
		}

		private void WriteTool(IMakeWriter compileCommandsWriter, IMakeWriter cppWriter, IMakeWriter cppClrWriter, CcCompiler cc, PathString directory, string scriptFile)
		{
			Directory.CreateDirectory(directory.Path); // clang-tools require the path included in the json directory property exists.
#if NETFRAMEWORK
			var serializer = new JavaScriptSerializer();
#endif

			string intrinsicsIncludeFile = OutputDir.Replace('\\', '/') + "/ps4_clang_intrinsics.h";
			bool usingDefaultPs4IntrinsicsInclude = true;

			if (Project.Properties["config-system"] == "kettle" && Project.Properties.Contains("ps4_clang_intrinsics_hdr"))
			{
				intrinsicsIncludeFile = Project.Properties["ps4_clang_intrinsics_hdr"];
				usingDefaultPs4IntrinsicsInclude = false;
			}

			var unixDirectory = directory.Path.Replace('\\', '/');

			foreach (var fi in cc.SourceFiles.FileItems)
			{
				if (fi.Path.Path.EndsWith(".c"))
				{
					// Skip plain C files.
					// Why do we do this, yet include them in refactor.py?
					continue;
				}

				var filecompiler = GetCompiler(cc, fi); // custom compiler options can be associated with individual file items

				var compilestep = new CompileStep();
				compilestep.directory = unixDirectory;
				compilestep.file = fi.Path.Path.Replace('\\', '/');
				compilestep.command = cc.Executable.Path.Quote().Replace('\\', '/');
				bool isClr = false;
				foreach (var v in filecompiler.Options)
				{
					// Skip MSVC arguments that Clang chokes on.
					if (   v == "/permissive" 
						|| v == "/Zf"
						|| v == "/Zc:char8_t-"
						|| v == "-MP")
					{
						continue;
					}

					if (v == "-clr")
					{
						isClr = true;
					}

					compilestep.command += " " + v;
				}

				if (isClr)
				{
					cppClrWriter.WriteLine(scriptFile + "," + fi.Path.Path);
				}
				else
				{
					// Don't prepend "scriptFile," here since when we use Clang
					// to parse through files, we will find all sorts of
					// #include:d constructs which actually belong to other
					// .build files.
					// gen_data_api.py instead uses a trie of build files paths.
					cppWriter.WriteLine(fi.Path.Path);
				}

				// Hack for kettle-clang
				if (Project.Properties["config-system"] == "kettle")
				{
					compilestep.command += " -target x86_64-scei-ps4 -D__ORBIS__";
					compilestep.command += " -include " + intrinsicsIncludeFile;
				}

				foreach (var v in filecompiler.Defines)
				{
					compilestep.command += " -D" + v;
				}

				foreach (var v in filecompiler.IncludeDirs)
				{
					compilestep.command += " -I" + v.Path.Quote().Replace('\\', '/');
				}

				compilestep.command += " " + compilestep.file;

				if (PropertyUtil.PropertyExists(Project, "clang-additional-cmds"))
				{
					string additionalCmds = Project.Properties["clang-additional-cmds"];
					compilestep.command += " " + additionalCmds;
				}

				if (_isFirstLine)
				{
					_isFirstLine = false;
				}
				else 
				{
					compileCommandsWriter.WriteLine(",");
				}

#if NETFRAMEWORK
				compileCommandsWriter.WriteLine(serializer.Serialize(compilestep));
#else
				compileCommandsWriter.WriteLine(JsonConvert.SerializeObject(compilestep, Formatting.Indented));
#endif


			}

			if (Project.Properties["config-system"] == "kettle" && usingDefaultPs4IntrinsicsInclude)
			{
				// write out the intrinsics file
				WritePS4IntrinsicsFile(intrinsicsIncludeFile);
			}
		}

		// write out the intrinsics file that fools the Win32 compiler into working properly with PS4 code
		private void WritePS4IntrinsicsFile(string filename)
		{
			string fileContents = @"
typedef float __m128 __attribute__ ((__vector_size__ (16)));
typedef double __m128d __attribute__ ((__vector_size__ (16)));
typedef long long __m128i __attribute__ ((__vector_size__ (16)));
typedef float __m256 __attribute__ ((__vector_size__ (32)));
typedef double __m256d __attribute__((__vector_size__(32)));
typedef long long __m256i __attribute__((__vector_size__(32)));
typedef double __v2df __attribute__ ((__vector_size__(16)));
typedef long long __v2di __attribute__ ((__vector_size__ (16)));
typedef int __v4si __attribute__((__vector_size__(16)));
typedef float __v4sf __attribute__((__vector_size__(16)));
typedef long long __v2di __attribute__ ((__vector_size__ (16)));
typedef char __v16qi __attribute__((__vector_size__(16)));
typedef long long __v1di __attribute__((__vector_size__(8)));
typedef int __v2si __attribute__((__vector_size__(8)));
typedef short __v4hi __attribute__((__vector_size__(8)));
typedef char __v8qi __attribute__((__vector_size__(8)));
typedef short __v8hi __attribute__((__vector_size__(16)));
typedef short __v16hi __attribute__ ((__vector_size__ (32)));
typedef float __v8sf __attribute__ ((__vector_size__ (32)));

typedef double __v2df __attribute__ ((__vector_size__ (16)));
typedef long long __v2di __attribute__ ((__vector_size__ (16)));
typedef short __v8hi __attribute__((__vector_size__(16)));
typedef char __v16qi __attribute__((__vector_size__(16)));
typedef char __v32qi __attribute__ ((__vector_size__ (32)));
typedef unsigned long long __v2du __attribute__ ((__vector_size__ (16)));
typedef unsigned short __v8hu __attribute__((__vector_size__(16)));
typedef unsigned char __v16qu __attribute__((__vector_size__(16)));
typedef signed char __v16qs __attribute__((__vector_size__(16)));
typedef long long __v4di __attribute__ ((__vector_size__ (32)));
typedef double __v4df __attribute__ ((__vector_size__ (32)));

__m256i __builtin_ia32_vpcmov_256(__v4di, __v4di, __v4di);
__m128i __builtin_ia32_vpcmov(__v2di, __v2di, __v2di);




__m128 __builtin_ia32_vfmsubps(__v4sf, __v4sf, __v4sf);
__m128d __builtin_ia32_vfmsubpd(__v2df, __v2df, __v2df);
__m128 __builtin_ia32_vfmsubss(__v4sf, __v4sf, __v4sf);
__m128 __builtin_ia32_vfmaddss(__v4sf, __v4sf, __v4sf);
__m128d __builtin_ia32_vfmsubsd(__v2df, __v2df, __v2df);
__m128d __builtin_ia32_vfmaddsd(__v2df, __v2df, __v2df);
__m128 __builtin_ia32_vfnmaddps(__v4sf, __v4sf, __v4sf);
__m128 __builtin_ia32_vfmaddps(__v4sf, __v4sf, __v4sf);
__m128d __builtin_ia32_vfnmaddpd(__v2df, __v2df, __v2df);
__m128d __builtin_ia32_vfmaddpd(__v2df, __v2df, __v2df);
__m128 __builtin_ia32_vfnmaddss(__v4sf, __v4sf, __v4sf);
__m128 __builtin_ia32_vfmaddss(__v4sf, __v4sf, __v4sf);
__m128d __builtin_ia32_vfnmaddsd(__v2df, __v2df, __v2df);
__m128d __builtin_ia32_vfmaddsd(__v2df, __v2df, __v2df);
__m128 __builtin_ia32_vfnmsubps(__v4sf, __v4sf, __v4sf);
__m128d __builtin_ia32_vfnmsubpd(__v2df, __v2df, __v2df);
__m128 __builtin_ia32_vfnmsubss(__v4sf, __v4sf, __v4sf);
__m128 __builtin_ia32_vfmaddss(__v4sf, __v4sf, __v4sf);
__m128d __builtin_ia32_vfnmsubsd(__v2df, __v2df, __v2df);
__m128d __builtin_ia32_vfmaddsd(__v2df, __v2df, __v2df);
__m128 __builtin_ia32_vfmsubaddps(__v4sf, __v4sf, __v4sf);
__m128 __builtin_ia32_vfmaddps(__v4sf, __v4sf, __v4sf);
__m128d __builtin_ia32_vfmsubaddpd(__v2df, __v2df, __v2df);
__m128d __builtin_ia32_vfmaddpd(__v2df, __v2df, __v2df);
__m256 __builtin_ia32_vfmsubps256(__v8sf, __v8sf, __v8sf);
__m256i __builtin_ia32_movntdqa256(const __v4di*);
__m128i __builtin_ia32_movntdqa (const __v2di*);

__v16qi __builtin_ia32_pavgb128 (__v16qi, __v16qi);
__v8hi __builtin_ia32_pavgw128 (__v8hi, __v8hi);
__v32qi __builtin_ia32_pavgb256 (__v32qi,__v32qi);
__v16hi __builtin_ia32_pavgw256 (__v16hi,__v16hi);
__v16qi __builtin_ia32_pcmpeqb128 (__v16qi, __v16qi);
__v8hi __builtin_ia32_pcmpeqw128 (__v8hi, __v8hi);
__v4si __builtin_ia32_pcmpeqd128 (__v4si, __v4si);
__v16qi __builtin_ia32_pcmpgtb128 (__v16qi, __v16qi);
__v8hi __builtin_ia32_pcmpgtw128 (__v8hi, __v8hi);
__v4si __builtin_ia32_pcmpgtd128 (__v4si, __v4si);
__v16qi __builtin_ia32_pmaxub128 (__v16qi, __v16qi);
__v8hi __builtin_ia32_pmaxsw128 (__v8hi, __v8hi);
__v16qi __builtin_ia32_pminub128 (__v16qi, __v16qi);
__v8hi __builtin_ia32_pminsw128 (__v8hi, __v8hi);

__v2di __builtin_ia32_movntdqa (__v2di *);
__v16qi __builtin_ia32_pavgb128 (__v16qi, __v16qi);

__m256d __builtin_ia32_cvtps2pd256(__m128);
__m128d __builtin_ia32_cvtps2pd(__m128d);
__m128d __builtin_ia32_cvtdq2pd(__m128i);
__m128 __builtin_ia32_cvtdq2ps(__m128i);
__m256 __builtin_ia32_cvtdq2ps256(__v8sf);
//__m256d __builtin_ia32_cvttpd2dq256(__m256d);
//__m256i __builtin_ia32_cvttps2dq256(__m256i);

void __builtin_ia32_storeups(float*, __m128);
void __builtin_ia32_storeupd(double*, __m128);
void __builtin_ia32_storedqu(char* __p, __m128i);
void __builtin_ia32_storedqu256(char *, __m256i);
void __builtin_ia32_storeupd256(double*, __m256d);
void __builtin_ia32_storeups256(float*, __m256);

__m128i 	__builtin_ia32_cvttps2dq(__m128i);
__m128i 	__builtin_ia32_pmovsxdq128(__m128i); 
__m128i 	__builtin_ia32_pmovsxbq128(__m128i); 
__m128i		__builtin_ia32_pmovsxbw128(__m128i);
int 		__builtin_ia32_pmovmskb128(__m128i);
__m128i 	__builtin_ia32_pmovsxbd128(__m128i);
  __m128i 	__builtin_ia32_pmovsxwd128(__m128i); 
  __m128i 	__builtin_ia32_pmovsxwq128(__m128i); 
  __m128i 	__builtin_ia32_pmovzxbd128(__m128i); 
  __m128i 	__builtin_ia32_pmovzxbq128(__m128i); 
  __m128i 	__builtin_ia32_pmovzxbw128(__m128i); 
  __m128i 	__builtin_ia32_pmovzxdq128(__m128i); 
  __m128i 	__builtin_ia32_pmovzxwd128(__m128i); 
  __m128i 	__builtin_ia32_pmovzxwq128(__m128i); 

  __m256d 	__builtin_ia32_cvtdq2pd256(__m128i);


short   __builtin_popcounts(unsigned short);
__m256d __builtin_ia32_pd256_pd(__v2df);
__m256i __builtin_ia32_si256_si(__v4si);
__m256  __builtin_ia32_ps256_ps(__v4sf); 

__m256  __builtin_ia32_vinsertf128_pd256(__m256, __m128d, const int o);
__m256d __builtin_ia32_vinsertf128_ps256(__m256d, __m128d, const int o);
__m256i __builtin_ia32_vinsertf128_si256(__m256i, __m128, const int o);

__m128  __builtin_ia32_vextractf128_pd256(__m256, const int o);
__m128d __builtin_ia32_vextractf128_ps256(__m256d, const int o);
__m128i __builtin_ia32_vextractf128_si256(__m256i, const int o);


// BMI
unsigned int __builtin_ia32_andn_u32(unsigned int, unsigned int);
unsigned int __builtin_ia32_blsi_u32(unsigned int);
unsigned int __builtin_ia32_blsmsk_u32(unsigned int);
unsigned int __builtin_ia32_blsr_u32(unsigned int);
unsigned long long __builtin_ia32_andn_u64(unsigned long long, unsigned long long);
unsigned long long __builtin_ia32_blsi_u64(unsigned long long, unsigned long long);
unsigned long long __builtin_ia32_blsmsk_u64(unsigned long long);
unsigned long long __builtin_ia32_blsr_u64(unsigned long long);
unsigned long long __builtin_ia32_blsi_u64(unsigned long long);

// AVX
__m128 __builtin_ia32_vbroadcastss(float const*);
__m128d __builtin_ia32_vbroadcastss(double const*);
__m256 __builtin_ia32_vbroadcastss256(float const*);
__m256d __builtin_ia32_vbroadcastsd256(double const*);
__m256 __builtin_ia32_vbroadcastf128_ps256(__v4sf const*);
__m256 __builtin_ia32_vbroadcastf128_pd256(__v2df const*);
__m256 __builtin_ia32_vfnmaddps256(__v8sf, __v8sf, __v8sf);
__m256d __builtin_ia32_vfnmaddpd256(__v4df, __v4df, __v4df);

__m128i __builtin_ia32_vpcmov(__v2di, __v2di, __v2di);
__m256i __builtin_ia32_vpcmov_256(__v4di, __v4di, __v4di);
__m256i __builtin_ia32_movntdqa256(const __v4di *);
__m256i __builtin_ia32_movntdqa256(const __v4di *);
__m256d __builtin_ia32_vfmsubpd256(__v4df, __v4df, __v4df);
__m256 __builtin_ia32_vfnmsubps256(__v8sf, __v8sf, __v8sf);
__m256d __builtin_ia32_vfnmsubpd256(__v4df, __v4df, __v4df);
__m256 __builtin_ia32_vfmsubaddps256(__v8sf, __v8sf, __v8sf);
__m256d __builtin_ia32_vfmsubaddpd256(__v4df, __v4df, __v4df);
";
			Directory.CreateDirectory(Path.GetDirectoryName(filename));

			// write the actual file out, should be in the top level of the compilecommands directory
			File.WriteAllText(filename, fileContents);
		}

		private void WriteTool(IMakeWriter writer, CscCompiler csc, string scriptFile)
		{
			foreach (var fi in csc.SourceFiles.FileItems)
			{
				if (!fi.Path.Path.EndsWith(".cs"))
				{
					// Skip non-C# files and semi-C# files like .XAML
					continue;
				}

				writer.WriteLine(scriptFile + "," + fi.Path.Path.ToLower().Replace('\\', '/'));
			}
		}

		private void WriteTool(IMakeWriter writer, BuildTool buildTool, string scriptFile)
		{
			writer.WriteLine(scriptFile + "," + ShortenToGenerated(buildTool.IntermediateDir.Path.ToLower().Replace('\\', '/')) + "," + buildTool.ToolName);
			foreach (var fi in buildTool.OutputDependencies.FileItems)
			{
				writer.WriteLine(scriptFile + "," + ShortenToGenerated(fi.Path.Path.ToLower().Replace('\\', '/')));
			}
		}

		private string ShortenToGenerated(string path)
		{
			// This is to handle stuff like
			// "d:/dev/dev-na/tnt/local/build/blazesdk/15.1.1.9.4-fb/pc64-vc-dll-release/.."
			// Go 2 directories deeper than "tnt/local/build/" and then ignore
			// the platform-specific parts.
			int localBuildIndex = path.IndexOf("tnt/local/build/");
			if (localBuildIndex >= 0)
			{
				int slashPos = path.IndexOf('/', localBuildIndex + "tnt/local/build/".Length);
				if (slashPos >= 0)
				{
					slashPos = path.IndexOf('/', slashPos + 1);
					if (slashPos >= 0)
					{
						path = path.Substring(0, slashPos + 1);
					}
				}
			}

			int genIndex = path.IndexOf("/_gen_/");
			if (genIndex >= 0)
				return path.Substring(0, genIndex + 1);
			genIndex = path.IndexOf("/gen/");
			if (genIndex >= 0)
				return path.Substring(0, genIndex + 1);
			genIndex = path.IndexOf("/__ldfgen/");
			if (genIndex >= 0)
				return path.Substring(0, genIndex + 1);

			int lastSlash = path.LastIndexOf('/');
			if (lastSlash >= 0)
				return path.Substring(0, lastSlash + 1);
			else
				return path;
		}
	}
}
