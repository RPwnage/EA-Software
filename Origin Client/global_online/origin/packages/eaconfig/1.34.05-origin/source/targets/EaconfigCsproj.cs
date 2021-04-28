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
    [TaskName("EaconfigCsproj")]
    public class EaconfigCsproj : EaconfigVisualStudioBase
    {

        public static void Execute(Project project, string groupname, string buildModule)
        {
            EaconfigCsproj task = new EaconfigCsproj(project, groupname, buildModule);
            task.Execute();
        }

        public EaconfigCsproj(Project project, string groupname, string buildModule)
            : base(project, groupname, buildModule)
        {
        }

        public EaconfigCsproj()
            : base()
        {
        }

        protected string DotNetVersion
        {
            get { return NantToVSTools.GetDotNetVersion(Project, "3.5"); }
        }

        protected override string PreBuildTargetProperty
        {
            get { return ".csproj.prebuildtarget"; }
        }

        protected override bool IsValidProjectType
        {
            get
            {
                return ComputeBaseBuildData.GetBuldType(Project, Properties[PropGroupName("buildtype")]).IsCSharp;
            }
        }

        protected override string BuildTypeName
        {
            get { return "build.buildtype.csproj"; }
        }

        protected override bool Build()
        {
            TargetUtil.ExecuteTarget(Project, "eaconfig-csproj-nanttocsproj-invoker", true);
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
            FileSetUtil.FileSetAppendWithBaseDir(fs, Project.NamedFileSets[PropGroupName(".csproj.excludedbuildfiles")]);
            FileSetUtil.FileSetAppendWithBaseDir(fs, Project.NamedFileSets[PropGroupName(".csproj.excludedbuildfiles." + ConfigSystem)]);
            BuildData.BuildFileSets["build.csproj.excludedbuildfiles.all"] = fs;
        }

        private void SetupProperties()
        {
            string cscTarget = BuildData.BuildOptionSet.Options["csc.target"];
            BuildData.BuildProperties["cscTarget"] = cscTarget;
            BuildData.BuildProperties["build.debug"] = (BuildData.BuildOptionSet.Options["debugflags"] == "on" || BuildData.BuildOptionSet.Options["debugflags"] == "custom") ? "true" : "false";

            BuildData.BuildProperties["cscDefinesAll"] = StringUtil.EnsureSeparator(BuildData.BuildProperties["build.defines.all"], ';') + ";EA_DOTNET2";
            BuildData.BuildProperties["post-build-step"] = Properties[PropGroupName("csproj.post-build-step")];
            BuildData.BuildProperties["pre-build-step"] = Properties[PropGroupName("csproj.pre-build-step")];
            BuildData.BuildProperties["runpostbuildevent"] = Properties[PropGroupName("csproj.runpostbuildevent")];
            BuildData.BuildProperties["rootnamespace"] = PropertyUtil.GetPropertyOrDefault(Project, PropGroupName("csproj.rootnamespace"), BuildModule);
            BuildData.BuildProperties["package.perforceintegration"] = PropertyUtil.GetPropertyOrDefault(Project, "package.perforceintegration", "false");
            BuildData.BuildProperties["__private_allowunsafe"] = PropertyUtil.GetPropertyOrDefault(Project, PropGroupName("allowunsafe"), "false");
            BuildData.BuildProperties["__private_enableunmanageddebugging"] = PropertyUtil.GetPropertyOrDefault(Project, PropGroupName("csproj.enableunmanageddebugging"), "false");
            BuildData.BuildProperties["__private_disablevshosting"] = PropertyUtil.GetPropertyOrDefault(Project, PropGroupName("csproj.disablevshosting"), "false");
            BuildData.BuildProperties["__private_targetframeworkversion"] = PropertyUtil.GetPropertyOrDefault(Project, PropGroupName("csproj.targetframeworkversion"), "");
            BuildData.BuildProperties["workingdir"] = Properties[PropGroupName(".workingdir")];
            BuildData.BuildProperties["remotemachine"] = Properties[PropGroupName(".remotemachine")];
            BuildData.BuildProperties["commandprogram"] = Properties[PropGroupName(".commandprogram")];
            BuildData.BuildProperties["commandargs"] = Properties[PropGroupName(".commandargs")];
            BuildData.BuildProperties["keyfilename"] = Properties[PropGroupName(".keyfile")];
            BuildData.BuildProperties["application-manifest"] = Properties[PropGroupName("csproj.application-manifest")];

            BuildData.BuildProperties["csproj-name"] = BuildProjectName;
            BuildData.BuildProperties["__private_output_filename"] = BuildOutput.FullOutputName;

            string csproj_root = "___private_" + PackageName + "." + BuildModule + ".csproj_projroot";

            if (PropertyUtil.GetBooleanProperty(Project, "package." + PackageName + ".designermode"))
            {
                BuildData.BuildProperties[csproj_root] = Properties["build.resourcefiles.basedir"];
            }
            else
            {
                BuildData.BuildProperties[csproj_root] = Properties["package.builddir"] + "/" + BuildProjectName;
            }

            string cscPlatform = Properties[PropGroupName(".platform")];

            if (String.IsNullOrEmpty(cscPlatform))
            {
                if (cscTarget == "exe" || cscTarget == "winexe")
                {
                    switch (ConfigSystem)
                    {
                        case "pc64":
                            cscPlatform = "x64";
                            break;
                        case "pc":
                            cscPlatform = "Win32";
                            break;
                    }
                }
                if (String.IsNullOrEmpty(cscPlatform))
                {
                    cscPlatform = "AnyCPU";
                }
            }
            BuildData.BuildProperties["cscPlatform"] = cscPlatform;

            BuildData.BuildProperties["__private_csproj_warnaserrors"] = PropertyUtil.GetPropertyOrDefault(Project, PropGroupName(".warningsaserrors"), "false");
            BuildData.BuildProperties["__private_csproj_warnaserrors.list"] = StringUtil.EnsureSeparator(PropertyUtil.GetPropertyOrDefault(Project, PropGroupName(".warningsaserrors.list"), ""), ',');
            BuildData.BuildProperties["__private_csproj_suppresswarnings"] = StringUtil.EnsureSeparator(PropertyUtil.GetPropertyOrDefault(Project, PropGroupName(".suppresswarnings"), ""), ',');
        }


    }
}
