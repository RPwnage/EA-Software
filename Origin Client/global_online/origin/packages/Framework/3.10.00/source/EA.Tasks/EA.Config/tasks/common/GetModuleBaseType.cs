using NAnt.Core;
using NAnt.Core.Attributes;
using NAnt.Core.Logging;

using System;
using System.Collections;
using System.Collections.Generic;
using System.Text;

namespace EA.Eaconfig
{
    /// <summary>
    /// Evaluates base buildtype name for the given buildtype optionset.
    /// </summary>
    /// <remarks>
    /// Base build type name can be one of the following
    /// <list type="bullet">
    /// <item>Library</item>
    /// <item>Program</item>
    /// <item>DynamicLibrary</item>
    /// <item>WindowsProgram</item>
    /// <item>CSharpLibrary</item>
    /// <item>CSharpProgram</item>
    /// <item>CSharpWindowsProgram</item>
    /// <item>FSharpLibrary</item>
    /// <item>FSharpProgram</item>
    /// <item>FSharpWindowsProgram</item>
    /// <item>ManagedCppAssembly</item>
    /// <item>ManagedCppProgram</item>
    /// <item>MakeStyle</item>
    /// <item>VisualStudioProject</item>
    /// <item>Utility</item>
    /// </list>
    /// </remarks>
    [TaskName("GetModuleBaseType")]
    public class GetModuleBaseType : EAConfigTask
    {
        /// <summary>
        /// Name of the buildtype optionset to examine
        /// </summary>
        [TaskAttribute("buildtype-name", Required = true)]
        public string BuildTypeName
        {
            get { return _buildTypeName; }
            set { _buildTypeName = value; }
        }

        /// <summary>
        /// Evaluated buildtype. Saved in property "GetModuleBaseType.RetVal"
        /// </summary>
        [TaskAttribute("base-buildtype", Required = false)]
        public string BaseBuildType
        {
            get { return _buildType.BaseName; }
        }

        public BuildType BuildType
        {
            get { return _buildType; }
        }


        public static BuildType Execute(Project project, string buildTypeName)
        {
            GetModuleBaseType task = new GetModuleBaseType(project, buildTypeName);
            task.ExecuteTask();
            return task.BuildType;
        }

        public GetModuleBaseType(Project project, string buildTypeName)
            : base(project)
        {
            BuildTypeName = buildTypeName;

            if (String.IsNullOrEmpty(BuildTypeName))
            {
                Error.Throw(Project, Location, Name, "'BuildTypeName' parameter is empty.");
            }
        }

        public GetModuleBaseType()
            : base()
        {
        }

        /// <summary>Execute the task.</summary>
        protected override void ExecuteTask()
        {
            string baseBuildType = String.Empty;

            if (BuildTypeName == "Program")
            {
                BuildTypeName = "StdProgram";
            }
            else if (BuildTypeName == "Library")
            {
                BuildTypeName = "StdLibrary";
            }
            OptionSet buildOptionSet = OptionSetUtil.GetOptionSetOrFail(Project, BuildTypeName);

            if (OptionSetUtil.GetBooleanOption(buildOptionSet, "buildset.protobuildtype"))
            {
                Error.Throw(Project, Location, "GetModuleBaseType", "Buildset '{0}' is a protobuild set and can not be used for build.\nUse 'GenerateBuildOptionset' task to create final build type", BuildTypeName);
            }

            string buildTasks = OptionSetUtil.GetOption(buildOptionSet, "build.tasks");


            if (buildTasks == null)
            {
                Error.Throw(Project, Location, "GetModuleBaseType", "BuildType '{0}' does not have 'build.tasks' option", BuildTypeName);
            }

            if (buildTasks.Contains("lib"))
            {
                baseBuildType = "StdLibrary";
            }
            else if (buildTasks.Contains("csc"))
            {
                switch (OptionSetUtil.GetOption(buildOptionSet,"csc.target"))
                {
                    case "library":
                        baseBuildType = "CSharpLibrary";
                        break;
                    case "exe":
                        baseBuildType = "CSharpProgram";
                        break;
                    case "winexe":
                        baseBuildType = "CSharpWindowsProgram";
                        break;
                }
            }
            else if (buildTasks.Contains("fsc"))
            {
                // baseBuildType must be set here...
                switch (OptionSetUtil.GetOption(buildOptionSet, "fsc.target"))
                {
                    case "library":
                        baseBuildType = "FSharpLibrary";
                        break;
                    case "exe":
                        baseBuildType = "FSharpProgram";
                        break;
                    case "winexe":
                        baseBuildType = "FSharpWindowsProgram";
                        break;
                }
            }
            else if (buildTasks.Contains("link"))
            {
                string ccDefines = OptionSetUtil.GetOption(buildOptionSet, "cc.defines");
                string ccOptions = OptionSetUtil.GetOption(buildOptionSet, "cc.options");
                string linkOptions = OptionSetUtil.GetOption(buildOptionSet, "link.options");

                if (ccOptions.Contains("clr"))
                {
                    if (linkOptions.Contains("-dll"))
                    {
                        baseBuildType = "ManagedCppAssembly";
                    }
                    else
                    {
                        baseBuildType = "ManagedCppProgram";
                    }
                }
                else if (OptionSetUtil.GetBooleanOption(buildOptionSet, "generatedll") ||

                         linkOptions.Contains("-dll") ||
                         linkOptions.Contains("-mprx") ||
                         linkOptions.Contains("-oformat=fsprx"))
                {
                    baseBuildType = "DynamicLibrary";
                }
                else if (ccDefines.Contains("_WINDOWS"))
                {
                    baseBuildType = "WindowsProgram";
                }
                else
                {
                    baseBuildType = "StdProgram";
                }
            }
            else if (buildTasks.Contains("makestyle"))
            {
                baseBuildType = "MakeStyle";
            }
            else if (buildTasks.Contains("visualstudio") || BuildTypeName == "VisualStudioProject")
            {
                baseBuildType = "VisualStudioProject";
            }
            else if (buildTasks.Contains("pythonprogram") || BuildTypeName == "PythonProgram")
            {
                baseBuildType = "PythonProgram";
            }
            else if (String.IsNullOrEmpty(buildTasks) || BuildTypeName == "Utility")
            {
                baseBuildType = "Utility";  // Seems that's what eaconf expects for Util
            }
            else
            {
                baseBuildType = "StdLibrary";
            }

            string subsystem = String.Empty;
            if (ConfigSystem == "ps3")
            {
                // Two ways to check for a spu build?  The second one is required by the SpuMakeStyle build type used in the spubuildpipe package used by ant...
                // All of the rest seem to have the ps3-spu option set...
                if (buildOptionSet.Options.Contains("ps3-spu"))
                {
                    subsystem = ".spu";
                }
                else
                {
                    // See SlnMaker.cs in old nantToVSTools and how it determines whether the current module is a SPU module or not.
                    string currCompiler = Project.Properties["cc"];
                    if (Project.NamedOptionSets[BuildTypeName].Options.Contains("cc"))
                    {
                        currCompiler = Project.NamedOptionSets[BuildTypeName].Options["cc"];
                    }
                    string spuCompiler = Project.NamedOptionSets["ps3spu"].Options["cc"];
                    if (currCompiler.Equals(spuCompiler))
                    {
                        subsystem = ".spu";
                    }
                }
            }

            bool isEasharp = OptionSetUtil.GetBooleanOption(buildOptionSet, "easharp");
            bool ismakestyle = baseBuildType == "MakeStyle";

            _buildType = new BuildType(BuildTypeName, baseBuildType, subsystem, isEasharp, ismakestyle);

            // For compatibility with C# script
            Properties["GetModuleBaseType.RetVal"] = baseBuildType;
        }


        private string _buildTypeName;
        BuildType _buildType;

    }
}
