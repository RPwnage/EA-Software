using NAnt.Core;
using NAnt.Core.Util;
using NAnt.Core.Attributes;
using NAnt.Core.Functions;
using NAnt.Core.Logging;

using EA.nantToVSTools;

using System;
using System.Collections;
using System.Collections.Generic;
using System.Text;
using System.IO;

namespace EA.Eaconfig
{

    public class NativeBuildData : BuildData
    {

    };

    [TaskName("EaconfigBuild")]
    public class EaconfigBuild : EaconfigTargetBase
    {
        public static void Execute(Project project, string groupname, string buildModule)
        {
            EaconfigBuild task = new EaconfigBuild(project, groupname, buildModule);
            task.Execute();
        }

        public EaconfigBuild(Project project, string groupname, string buildModule)
            : base(project, groupname, buildModule)
        {
        }

        public EaconfigBuild()
            : base()
        {
        }

        protected override string BuildTypeName
        {
            get { return "__private_build.buildtype.nativenant"; }
        }

        protected override string BuildProjectName
        {
            get
            {
                return BuildModule;
            }
        }


        protected override BuildData BuildData
        {
            get { return _projBuildData; }
        }

        protected NativeBuildData NativeBuildData
        {
            get { return _projBuildData; }
        }


        protected override bool Prebuild()
        {
            if(FileSetUtil.FileSetExists(Project, PropGroupName("resourcefiles")))
            {
                Properties["build-res-done"] = "false";
            }

            TargetUtil.ExecuteTarget(Project, "__private_call_prebuild_target", true);

            // Execute any platform specific custom target that SDK might define:

            TargetUtil.ExecuteTargetIfExists(Project, Project.Properties["eaconfig." + ConfigPlatform + ".prebuildtarget"], true);


            _projBuildData = new NativeBuildData();

            return true;
        }

        protected override bool BeforeBuildStep()
        {
            string targetName = Project.Properties[PropGroupName(".beforebuildsteptarget")];

            if(String.IsNullOrEmpty(targetName))
            {                
                targetName = PropGroupName(".beforebuildsteptarget");
            }

            TargetUtil.ExecuteTargetIfExists(Project, targetName, true);

            targetName = Project.Properties[PropGroupName(".beforebuildtarget")];

            if (String.IsNullOrEmpty(targetName))
            {
                targetName = PropGroupName(".beforebuildtarget");
            }

            TargetUtil.ExecuteTargetIfExists(Project, targetName, true);


            // Execute any platform specific custom target that SDK might define:

            TargetUtil.ExecuteTargetIfExists(Project, Project.Properties["eaconfig." + ConfigPlatform + ".beforebuildsteptarget"], true);

            return true;

        }


        protected override bool PrepareBuild()
        {
            SetupUsePackageLibs();

            if (!(BuildData.BuildType.Name == "Utility" || BuildData.BuildType.Name == "MakeStyle") &&
                BuildData.BuildFileSets["build.sourcefiles.all"].FileItems.Count == 0 &&
                BuildData.BuildFileSets["build.asmsourcefiles.all"].FileItems.Count == 0)
            {
                string msg = Error.Format(Project, "EaconfigBuild", "WARNING", "Build style target ({0}) specified no source files to build (module name: {1})", Properties["target.name"], BuildModule);
                Log.Write(msg);                
            }
            else if (BuildData.BuildType.Name != "MakeStyle")
            {
                SetupAssemblyAndLibReferences(true);
            }

            if(!(BuildData.BuildType.IsCSharp || BuildData.BuildType.IsMakeStyle))
            {
                string pchFile = PropertyUtil.GetPropertyOrDefault(Project, PropGroupName("pch-file"), "%outputdir%\\stdPCH.pch");
                string pchHeaderFile = PropertyUtil.GetPropertyOrDefault(Project, PropGroupName("pch-header-file"), String.Empty);
                string outFolder = BuildOutput.OutputBuildDir;
                Resolve_PCH_Templates.Execute(Project, BuildData.BuildOptionSet, pchFile, pchHeaderFile, outFolder, BuildData.BuildFileSets["build.sourcefiles.all"]);
            }

            // Untill other targets are converted to C# store some properties:
            Properties["outputpath"] = BuildOutput.OutputBuildDir;
            BuildData.BuildProperties["__private_output_filename"] = BuildOutput.FullOutputName;

            // To prevent relinking 
            // need to replace the outputdir and outputname tokens if linkoutputname contains them
            // because build.outputdir.override will be used after lib task expands those tokens
            if(BuildData.BuildType.IsMakeStyle)
            {
                if(Properties.Contains("build.outputdir.override"))
                    Properties.Remove("build.outputdir.override");
            }
            else if (BuildData.BuildType.BaseName != "StdLibrary")
            {
                Properties["build.outputdir.override"] = Path.GetDirectoryName(BuildOutput.FullOutputName);
            }
            else
            {
                string groupOutputDir = Properties[PropGroupName("outputdir")];
                if (!String.IsNullOrEmpty(groupOutputDir) && groupOutputDir != "%outputdir%")
                {
                    Properties["build.outputdir.override"] = groupOutputDir;
                }
            }
            return true;
        }

        protected override bool Build()
        {
            if (BuildData.BuildType.IsMakeStyle)
            {
                TargetUtil.ExecuteTarget(Project, "eaconfig-buildmakestyle", true);  
            }
            else if (BuildData.BuildType.IsCSharp)
            {
                TargetUtil.ExecuteTarget(Project, "eaconfig-buildcsc", true);
                TargetUtil.ExecuteTarget(Project, "build-copy", true);
            }
            else if (BuildData.BuildType.IsFSharp)
            {
                TargetUtil.ExecuteTarget(Project, "eaconfig-buildfsc", true);
                TargetUtil.ExecuteTarget(Project, "build-copy", true);
            }
            else
            {
                TargetUtil.ExecuteTarget(Project, "eaconfig-buildcpp", true);
                TargetUtil.ExecuteTarget(Project, "build-copy", true);
            }
            return true;
        }

        protected override void PostBuild()
        {
            // Execute post build
            string post_build_target = PropertyUtil.GetPropertyOrDefault(Project, PropGroupName("postbuildtarget"), PropGroupName("postbuildtarget"));
            TargetUtil.ExecuteTargetIfExists(Project, post_build_target, true);

            // Create application package:
            // Execute any platform specific custom target that SDK might define:
            if (!TargetUtil.ExecuteTargetIfExists(Project, Project.Properties["eaconfig." + ConfigPlatform + ".application-package-target"], true))
            {
                // If non platform specific target is defined - execute it
                string application_package_target = PropertyUtil.GetPropertyOrDefault(Project, PropGroupName("application-package-target"), PropGroupName("application-package-target"));
                TargetUtil.ExecuteTargetIfExists(Project, application_package_target, true);
            }

            // Call the deployment task.
            DeployTask deployTask = new DeployTask(GroupName, BuildModule, ConfigSystem);
            deployTask.Project = Project;
            deployTask.Execute();
        }

        private void SetupUsePackageLibs()
        {
            //This is a really horrible legacy feature, which predates support for
            //module-dependencies in eaconfig.   Basically "test" and "example"
            //modules have ALL the "runtime" libraries added to their link line
            //automatically.

            //TODO:   Need to put together a deprecation plan for this.  Any
            //packages which support SLN generation have to use
            //module-dependencies anyway, so as soon as we've got everything
            //using module-dependencies, this feature will be obsolete.

            if (!PropertyUtil.GetBooleanProperty(Project, "eaconfig."+PropBuildGroup("usepackagelibs")))
            {
                return;
            }

            // We don't need to link libraries
            if (BuildData.BuildType.BaseName == "StdLibrary")
            {
                return;
            }

            if (ConfigSystem == "ps3")
            {
                if (PropertyExists("runtime.buildmodules"))
                {
                    string current_module_cc = OptionSetUtil.GetOptionOrDefault(BuildData.BuildOptionSet, "cc", String.Empty);
                    foreach (string module in StringUtil.ToArray(Properties["runtime.buildmodules"]))
                    {
                        string module_cc = OptionSetUtil.GetOptionOrDefault(OptionSetUtil.GetOptionSet(Project, Properties["runtime." + module + ".buildtype"]), "cc", String.Empty);

                        if (0 == String.Compare(current_module_cc, module_cc))
                        {
                            FileSetUtil.FileSetAppend(BuildData.BuildFileSets["build.libs.all"], Project.NamedFileSets["__private_runtime." + module + ".lib"]);
                        }
                    }

                }
                else if (PropertyExists("runtime.buildtype"))
                {
                    FileSetUtil.FileSetAppend(BuildData.BuildFileSets["build.libs.all"], Project.NamedFileSets["__private_runtime." + PackageName + ".lib"]);
                }                 
            }
            else
            {
                FileSetUtil.FileSetInclude(BuildData.BuildFileSets["build.libs.all"], Properties["package.configlibdir"] + "/*" + Properties["lib-suffix"]);
            }
        }



        private NativeBuildData _projBuildData;



    }
}
