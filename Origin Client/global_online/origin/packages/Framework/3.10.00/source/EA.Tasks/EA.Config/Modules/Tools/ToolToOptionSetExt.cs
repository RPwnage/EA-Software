using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

using NAnt.Core;
using NAnt.Core.Util;
using EA.FrameworkTasks.Model;
using EA.Eaconfig.Core;

namespace EA.Eaconfig.Modules.Tools
{
    internal static class ToolToOptionSetExt
    {
        internal static void ToOptionSet<T>(this Tool tool, OptionSet set, T module) where T : Module
        {
            if (tool != null)
            {
                if (tool is CcCompiler)
                    ((CcCompiler)tool).ToOptionSet(set, module);
                else if (tool is AsmCompiler)
                    ((AsmCompiler)tool).ToOptionSet(set, module);
                else if (tool is Linker)
                    ((Linker)tool).ToOptionSet(set, module);
                else if (tool is Librarian)
                    ((Librarian)tool).ToOptionSet(set, module);
                else if (tool is PostLink)
                    ((PostLink)tool).ToOptionSet(set, module);
            }
        }

        internal static void ToOptionSet<T>(this CcCompiler tool, OptionSet set, T module) where T : Module
        {
            InitSet(tool, set);

            if (tool != null)
            {
                Module_Native native_module = module as Module_Native;

                set.Options[tool.ToolName + ".defines"] = tool.Defines.ToString(Environment.NewLine);

                set.Options[tool.ToolName + ".objfile.extension"] = tool.ObjFileExtension;

                foreach(var define in tool.CompilerInternalDefines)
                {
                    set.Options[tool.ToolName+".compilerinternaldefines"] = define;
                }

                //Assign this option to use in nant compiler task  dependency 
                if (tool.PrecompiledHeaders == CcCompiler.PrecompiledHeadersAction.CreatePch)
                {
                    set.Options["create-pch"] = "on";
                    set.Options["pch-file"] = (native_module != null) ? native_module.PrecompiledFile.Path : String.Empty;
                }
            }
        }


        internal static void ToOptionSet<T>(this AsmCompiler tool, OptionSet set, T module) where T : Module
        {
            InitSet(tool, set, "as");

            if (tool != null)
            {
                set.Options["as.defines"] = tool.Defines.ToString(Environment.NewLine);
                set.Options["as.objfile.extension"] = tool.ObjFileExtension;
            }
        }

        internal static void ToOptionSet<T>(this Linker tool, OptionSet set, T module) where T : Module
        {
            InitSet(tool, set);

            if (tool != null)
            {
                set.Options[tool.ToolName + ".librarydirs"] = StringUtil.ArrayToString(tool.LibraryDirs.Select<PathString, string>(item => item.Normalize().Path), Environment.NewLine);
            }
        }

        internal static void ToOptionSet<T>(this PostLink tool, OptionSet set, T module)  where T : Module
        {
            if (tool != null)
            {
                set.Options["link.postlink.program"] = tool.Executable.Path;
                set.Options["link.postlink.commandline"] = StringUtil.ArrayToString(tool.Options, Environment.NewLine);
            }
        }


        internal static void ToOptionSet<T>(this Librarian tool, OptionSet set, T module)
        {
            InitSet(tool, set);
        }


        private static void InitSet(Tool tool, OptionSet set, string altToolName = null)
        {
            if (tool != null)
            {
                var toolName = altToolName ?? tool.ToolName;

                foreach (string option_name in option_names)
                {
                    set.Options[toolName + option_name] = null;
                }

                foreach (var entry in tool.Templates)
                {
                    set.Options[entry.Key] = entry.Value;
                }

                if (!tool.WorkingDir.IsNullOrEmpty())
                {
                    set.Options[toolName + ".workingdir"] = tool.WorkingDir.Path;
                }

                set.Options[toolName] = tool.Executable.Path;
                set.Options[toolName + ".options"] = StringUtil.ArrayToString(tool.Options, Environment.NewLine);
            }
        }

        private static List<string> option_names = new List<string>{ ".defines", ".gccdefines", ".includedirs", ".usingdirs", ".objfile.extension" };
    }
}
