// (c) Electronic Arts. All rights reserved.

using System;
using System.Collections.Generic;
using System.Linq;
using System.IO;

using NAnt.Core;
using NAnt.Core.Attributes;

using EA.Eaconfig.Core;
using EA.Eaconfig.Build;
using EA.Eaconfig.Modules;
using EA.Eaconfig.Modules.Tools;

namespace EA.Eaconfig
{
	[TaskName("iphone-postprocess")]
	class Iphone_Postprocess : AbstractModuleProcessorTask
	{
		public override void Process(Module_DotNet module)
		{
			base.Process(module);
			AddDefaultAssemblies(module);
		}

		protected virtual void AddDefaultAssemblies(Module_DotNet module)
		{
			if (module.Compiler != null && module.Package.Project.Properties.GetBooleanPropertyOrDefault(PropGroupName("usedefaultassemblies"), true))
			{
				AddDefaultPlatformIndependentDotNetCoreAssemblies(module);
			}
		}

		public override void ProcessTool(Linker linker)
		{
			base.ProcessTool(linker);

			List<string> extraFrameworkPaths = new List<string>();
			List<string> extraLibraryPaths = new List<string>();

			string extra_framework_paths = PropGroupValue("iphone-extra-framework-search-paths", null);
			if (!String.IsNullOrEmpty(extra_framework_paths))
			{
				foreach (string path in extra_framework_paths.Split(new char[] { '\n' }).Select(ln => ln.Trim()).Where(ln => !String.IsNullOrEmpty(ln)))
				{
					string unixPath = path.Replace('\\','/');
					if (!extraFrameworkPaths.Contains(unixPath))
					{
						extraFrameworkPaths.Add(unixPath);
					}
				}
			}

			string extra_frameworks = PropGroupValue("iphone-frameworks", null);
			if (!String.IsNullOrEmpty(extra_frameworks))
			{
				foreach (string fw in extra_frameworks.Split(new char [] {'\n'}).Select(ln => ln.Trim()).Where(ln => !String.IsNullOrEmpty(ln)))
				{
					string fullLinkerSwitch = fw;

					// Extract the path from the framework and add -F switch if necessary
					string fwpath = null;
					string linkerSwitch = null;
					if (fw.StartsWith("-framework "))
					{
						fwpath = fw.Substring("-framework ".Length);
						linkerSwitch = "-framework ";
					}
					else if (fw.StartsWith("-weak_framework "))
					{
						fwpath = fw.Substring("-weak_framework ".Length);
						linkerSwitch = "-weak_framework ";
					}
					else
					{
						// unrecognized switch!
						continue;
					}

					// If this framework contains a full path, extract this path and we'll add it to the
					// -F (or -L) switches when necessary.
					string basePath = Path.GetDirectoryName(fwpath.Replace('\\', '/'));

					// Also the -framework/-weak_framework switch does not allow full path.
					// It allows framework name only.  So we're removing the path info.
					string fwName = fwpath;
					if (!String.IsNullOrEmpty(basePath))
					{
						fwName = fwpath.Substring(basePath.Length);
						if (fwName.StartsWith("/"))
						{
							fwName = fwName.Substring(1);
						}
					}

					// The new iOS only ship .tbd as dynamic library stub lib and it no longer ship .dylib.
					// However, the iphonesimilator SDK still uses .dylib
					if (fwName.EndsWith(".tbd") || fwName.EndsWith(".dylib"))
					{
						if (fwName.StartsWith("lib"))
						{
							int extLen = fwName.EndsWith(".tbd") ? 4 : 6;

							if (!String.IsNullOrEmpty(basePath) && !extraLibraryPaths.Contains(basePath))
							{
								extraLibraryPaths.Add(basePath);
							}

							// Building with the standard stub lib should use linker switch -l[lib_name] or -weak-l[lib_name]
							// NOTE that -weak_l cannot be used with BITCODE turned on!
							if (linkerSwitch == "-weak_framework ")
							{
								linkerSwitch = "-weak_l";
							}
							else
							{
								linkerSwitch = "-l";
							}
							fwName = fwName.Substring(3, fwName.Length - 3 - extLen);
						}
						else // if it is not in a standard form, we need to provide the full path.  Adding -L won't work!
						{
							if (!String.IsNullOrEmpty(basePath))
							{
								fwName = basePath + "/" + fwName;
							}

							// If full path is specified, we either just specify the file or use -weak_library [full_lib_path]
							// NOTE that -weak_library cannot be used with BITCODE turned on!
							if (linkerSwitch == "-weak_framework ")
							{
								linkerSwitch = "-weak_library ";
							}
							else
							{
								linkerSwitch = "";
							}
						}
						fullLinkerSwitch = linkerSwitch + fwName;
					}
					else
					{
						if (!String.IsNullOrEmpty(basePath) && !extraFrameworkPaths.Contains(basePath))
						{
							extraFrameworkPaths.Add(basePath);
						}
						// We cannot use full path to the -framework switch.  It must be
						// a framework name.  Strip out the path and re-do the linker switch.
						if (fwName.EndsWith(".framework"))
						{
							fwName = fwName.Substring(0,fwName.Length - ".framework".Length);
						}
						fullLinkerSwitch = linkerSwitch + fwName;
					}

					if (!linker.Options.Contains(fullLinkerSwitch))
					{
						linker.Options.Add(fullLinkerSwitch);
					}
				}
			}

			foreach (string path in extraFrameworkPaths)
			{
				string pathSwitch = "-F " + path;
				if (!linker.Options.Contains(pathSwitch))
				{
					linker.Options.Add(pathSwitch);
				}
			}

			foreach (string path in extraLibraryPaths)
			{
				string pathSwitch = "-L " + path;
				if (!linker.Options.Contains(pathSwitch))
				{
					linker.Options.Add(pathSwitch);
				}
			}

			string extra_link_options = PropGroupValue("iphone-extra-link-options", null);
			if (!String.IsNullOrEmpty(extra_link_options))
			{
				foreach (string linkop in extra_link_options.Split(new char[] { '\n' }).Select(ln => ln.Trim()).Where(ln => !String.IsNullOrEmpty(ln)))
				{
					if (!linker.Options.Contains(linkop))
					{
						linker.Options.Add(linkop);
					}
				}
			}
		}

		public override void ProcessTool(CcCompiler cc)
		{
			base.ProcessTool(cc);

			if (Properties.GetBooleanPropertyOrDefault("eaconfig.enable-pch", true))
			{
				System.Text.RegularExpressions.Regex archRegex = new System.Text.RegularExpressions.Regex("-arch +.+");
				if (cc.Options.Where(op => archRegex.IsMatch(op)).Count() > 1)
				{
					// If we are building for multiple architecture, we can't have pch arguments in there as each
					// pch file is only valid for specific architecture!  We have to strip them out and just do regular
					// build.

					// Strip out the use pch compiler switch
					System.Text.RegularExpressions.Regex regex = new System.Text.RegularExpressions.Regex("^-include-pch\\s*(\"[^\"]+\")?$");
					string removeOpt = cc.Options.Where(op => regex.IsMatch(op)).FirstOrDefault();
					if (!string.IsNullOrEmpty(removeOpt))
					{
						cc.Options.Remove(removeOpt);
					}

					// For source file that create the pch, strip out that command line switch as well.
					foreach (FileItem fitm in cc.SourceFiles.FileItems)
					{
						CcCompiler fileCc = fitm.GetTool() as CcCompiler;
						if (fileCc != null)
						{
							if (fileCc.Options.Contains("-x c++-header"))
							{
								fileCc.Options.Remove("-x c++-header");
								fileCc.Options.Insert(0, "-x c++");
							}
							else if (fileCc.Options.Contains("-x c-header"))
							{
								fileCc.Options.Remove("-x c-header");
								fileCc.Options.Insert(0, "-x c");
							}
							else if (fileCc.Options.Contains("-x objective-c++-header"))
							{
								fileCc.Options.Remove("-x objective-c++-header");
								fileCc.Options.Insert(0, "-x objective-c++");
							}
							else if (fileCc.Options.Contains("-x objective-c-header"))
							{
								fileCc.Options.Remove("-x objective-c-header");
								fileCc.Options.Insert(0, "-x objective-c");
							}
						}
					}
				}
			}

			// Compiler also need the -F switch for framework paths.  Otherwise, all the #include to the Framework
			// headers won't compile.

			List<string> extraPaths = new List<string>();

			string extra_framework_paths = PropGroupValue("iphone-extra-framework-search-paths", null);
			if (!String.IsNullOrEmpty(extra_framework_paths))
			{
				foreach (string path in extra_framework_paths.Split(new char[] { '\n' }).Select(ln => ln.Trim()).Where(ln => !String.IsNullOrEmpty(ln)))
				{
					string unixPath = path.Replace('\\', '/');
					if (!extraPaths.Contains(unixPath))
					{
						extraPaths.Add(unixPath);
					}
				}
			}

			string extra_frameworks = PropGroupValue("iphone-frameworks", null);
			if (!String.IsNullOrEmpty(extra_frameworks))
			{
				foreach (string fw in extra_frameworks.Split(new char[] { '\n' }).Select(ln => ln.Trim()).Where(ln => !String.IsNullOrEmpty(ln)))
				{
					// If this framework contains a full path, extract this path and add it to the
					// -F switches for the compiler.
					string fwpath = null;
					if (fw.StartsWith("-framework "))
					{
						fwpath = fw.Substring("-framework ".Length);
					}
					else if (fw.StartsWith("-weak_framework "))
					{
						fwpath = fw.Substring("-weak_framework ".Length);
					}
					if (!String.IsNullOrEmpty(fwpath))
					{
						string basePath = Path.GetDirectoryName(fwpath.Replace('\\', '/'));
						if (!String.IsNullOrEmpty(basePath) && !extraPaths.Contains(basePath) && !fwpath.EndsWith(".tbd"))
						{
							extraPaths.Add(basePath);
						}
					}
				}
			}

			foreach (string path in extraPaths)
			{
				string pathSwitch = "-F " + path;
				if (!cc.Options.Contains(pathSwitch))
				{
					cc.Options.Add(pathSwitch);
				}
			}

			if (extraPaths.Any())
			{
				foreach (FileItem fileitem in cc.SourceFiles.FileItems)
				{
					// This post process task only get executed to the module once and the above code applied extra compiler
					// switches to the main module's compiler.  However, if individual file has different optionset
					// such as ObjectiveCLibrary, etc, the file will have it's own tool.  We need to apply these new switches
					// to those file's compiler tool as well.
					if (!String.IsNullOrWhiteSpace(fileitem.OptionSetName))
					{
						CcCompiler fileCc = fileitem.GetTool() as CcCompiler;
						if (fileCc != null)
						{
							foreach (string path in extraPaths)
							{
								string pathSwitch = "-F " + path;
								if (!fileCc.Options.Contains(pathSwitch))
								{
									fileCc.Options.Add(pathSwitch);
								}
							}
						}
					}
				}
			}
		}

	}
}
