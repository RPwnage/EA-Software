using NAnt.Core;
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
    [TaskName("EaconfigFsproj")]
    public class EaconfigFsproj : EaconfigVisualStudioBase
    {

        public static void Execute(Project project, string groupname, string buildModule)
        {
            EaconfigFsproj task = new EaconfigFsproj(project, groupname, buildModule);
            task.Execute();
        }

        public EaconfigFsproj(Project project, string groupname, string buildModule)
            : base(project, groupname, buildModule)
        {
        }

        public EaconfigFsproj()
            : base()
        {
        }

        protected string DotNetVersion
        {
            get { return NantToVSTools.GetDotNetVersion(Project, "3.5"); }
        }

        protected override string PreBuildTargetProperty
        {
            get { return ".fsproj.prebuildtarget"; }
        }

        protected override bool IsValidProjectType
        {
            get
            {
                return ComputeBaseBuildData.GetBuldType(Project, Properties[PropGroupName("buildtype")]).IsFSharp;
            }
        }

        protected override string BuildTypeName
        {
            get { return "build.buildtype.fsproj"; }
        }

        protected override bool Build()
        {
            TargetUtil.ExecuteTarget(Project, "eaconfig-fsproj-nanttofsproj-invoker", true);
            return true;
        }

        protected override void PostBuild()
        {
        }

        protected override void SetupProjectInput()
        {
            SetupProperties();

            SetupReferences();

            if (!PropertyUtil.IsPropertyEqual(Project, PropGroupName("usedefaultassemblies"), "false"))
            {
                FileSet references = BuildData.BuildFileSets["__private_references"];

                FileSetUtil.FileSetInclude(references, "System.dll", true);
                FileSetUtil.FileSetInclude(references, "System.Data.dll", true);
                FileSetUtil.FileSetInclude(references, "System.XML.dll", true);
                FileSetUtil.FileSetInclude(references, "System.Drawing.dll", true);
                FileSetUtil.FileSetInclude(references, "System.Drawing.Design.dll", true);
                FileSetUtil.FileSetInclude(references, "System.Windows.Forms.dll", true);

                if (PropertyExists("package.eaconfig.isusingvc9"))
                {
                    switch (DotNetVersion)
                    {
                        //Do not add System.Core.dll for unsupported .net framework version
                        case "v2.0":
                        case "v3.0":
                            break;
                        default:
                            FileSetUtil.FileSetInclude(references, "System.Core.dll", true);
                            break;
                    }
                }
            }


            FileSet fs = new FileSet();
            fs.BaseDirectory = Properties["package.dir"];
            FileSetUtil.FileSetAppendWithBaseDir(fs, Project.NamedFileSets[PropGroupName(".fsproj.excludedbuildfiles")]);
            FileSetUtil.FileSetAppendWithBaseDir(fs, Project.NamedFileSets[PropGroupName(".fsproj.excludedbuildfiles."+ConfigSystem)]);
            BuildData.BuildFileSets["build.fsproj.excludedbuildfiles.all"] = fs;
        }

        private void SetupProperties()
        {
            string fscTarget = BuildData.BuildOptionSet.Options["fsc.target"];
            BuildData.BuildProperties["fscTarget"] = fscTarget;
            BuildData.BuildProperties["build.debug"] = (BuildData.BuildOptionSet.Options["debugflags"] == "on" || BuildData.BuildOptionSet.Options["debugflags"] == "custom") ? "true" : "false";

            BuildData.BuildProperties["fscDefinesAll"] = StringUtil.EnsureSeparator(BuildData.BuildProperties["build.defines.all"], ';') + ";EA_DOTNET2";
            BuildData.BuildProperties["post-build-step"] = Properties[PropGroupName("fsproj.post-build-step")];
            BuildData.BuildProperties["pre-build-step"] = Properties[PropGroupName("fsproj.pre-build-step")];
            BuildData.BuildProperties["runpostbuildevent"] = Properties[PropGroupName("fsproj.runpostbuildevent")];
            BuildData.BuildProperties["rootnamespace"] = PropertyUtil.GetPropertyOrDefault(Project, PropGroupName("fsproj.rootnamespace"), BuildModule);
            BuildData.BuildProperties["package.perforceintegration"] = PropertyUtil.GetPropertyOrDefault(Project, "package.perforceintegration", "false");
            BuildData.BuildProperties["__private_allowunsafe"] = PropertyUtil.GetPropertyOrDefault(Project, PropGroupName("allowunsafe"), "false");
            BuildData.BuildProperties["__private_enableunmanageddebugging"] = PropertyUtil.GetPropertyOrDefault(Project, PropGroupName("fsproj.enableunmanageddebugging"), "false");
            BuildData.BuildProperties["__private_disablevshosting"] = PropertyUtil.GetPropertyOrDefault(Project, PropGroupName("fsproj.disablevshosting"), "false");
            BuildData.BuildProperties["__private_targetframeworkversion"] = PropertyUtil.GetPropertyOrDefault(Project, PropGroupName("fsproj.targetframeworkversion"), "");
            BuildData.BuildProperties["workingdir"] = Properties[PropGroupName(".workingdir")];
            BuildData.BuildProperties["remotemachine"] = Properties[PropGroupName(".remotemachine")];
            BuildData.BuildProperties["commandprogram"] = Properties[PropGroupName(".commandprogram")];
            BuildData.BuildProperties["commandargs"] = Properties[PropGroupName(".commandargs")];
            BuildData.BuildProperties["keyfilename"] = Properties[PropGroupName(".keyfile")];

            BuildData.BuildProperties["fsproj-name"] = BuildProjectName;
            BuildData.BuildProperties["__private_output_filename"] = BuildOutput.FullOutputName;

            string fsproj_root = "___private_" + PackageName + "." + BuildModule + ".fsproj_projroot";

            if (PropertyUtil.GetBooleanProperty(Project, "package." + PackageName + ".designermode"))
            {
                BuildData.BuildProperties[fsproj_root] = Properties["build.resourcefiles.basedir"];
            }
            else
            {
                BuildData.BuildProperties[fsproj_root] = Properties["package.builddir"] + "/" + BuildProjectName;
            }

            string fscPlatform = Properties[PropGroupName(".platform")];

            if (String.IsNullOrEmpty(fscPlatform))
            {
                if (fscTarget == "exe" || fscTarget == "winexe")
                {
                    switch (ConfigSystem)
                    {
                        case "pc64":
                            fscPlatform = "x64";
                            break;
                        case "pc":
                            fscPlatform = "Win32";
                            break;
                    }
                }
                if (String.IsNullOrEmpty(fscPlatform))
                {
                    fscPlatform = "AnyCPU";
                }
            }
            BuildData.BuildProperties["fscPlatform"] = fscPlatform;

            BuildData.BuildProperties["__private_fsproj_warnaserrors"] = PropertyUtil.GetPropertyOrDefault(Project, PropGroupName(".warningsaserrors"), "false");
            BuildData.BuildProperties["__private_fsproj_warnaserrors.list"] = StringUtil.EnsureSeparator(PropertyUtil.GetPropertyOrDefault(Project, PropGroupName(".warningsaserrors.list"), ""), ',');
            BuildData.BuildProperties["__private_fsproj_suppresswarnings"] = StringUtil.EnsureSeparator(PropertyUtil.GetPropertyOrDefault(Project, PropGroupName(".suppresswarnings"), ""), ',');
        }


    }
}
