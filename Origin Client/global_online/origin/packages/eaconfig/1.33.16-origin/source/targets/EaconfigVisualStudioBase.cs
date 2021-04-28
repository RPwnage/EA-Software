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

    public class ProjBuildData : BuildData
    {        
    };

    public abstract class EaconfigVisualStudioBase : EaconfigTargetBase
    {

        public EaconfigVisualStudioBase(Project project, string groupname, string buildModule)
            : base(project, groupname, buildModule)
        {
        }

        public EaconfigVisualStudioBase()
            : base()
        {
        }

        protected override BuildData BuildData
        {
            get { return _projBuildData;  }
        }

        protected ProjBuildData ProjBuildData
        {
            get { return _projBuildData; }
        }

        protected override string BuildProjectName
        {
            get
            {
                if (String.IsNullOrEmpty(_buildProjectName))
                {
                    _buildProjectName = GetProjName(BuildData.BuildType.IsCSharp ? ".csproj-name-cross" : ".vcproj-name-cross");
                }
                return _buildProjectName;
            }
        }

        protected abstract string PreBuildTargetProperty
        {
            get;
        }

        protected abstract bool IsValidProjectType
        {
            get;
        }

        protected  abstract void SetupProjectInput();


        protected override bool Prebuild()
        {
            if (-1 != Array.IndexOf(UNSUPPORTED_PLATFORMS, ConfigSystem))
            {
                Error.Throw(Project, Location, Name, "nantToVStools doesn't support '{0}' platform.", ConfigSystem);
            }

            if (!IsValidProjectType)
            {
                return false;
            }



            //Hook to allow users to do a module-specific "pre-build" step for vcproj/csproj
            TargetUtil.ExecuteTargetIfExists(Project, Properties[GroupName + PreBuildTargetProperty], true);

            //If "webreferences" fileset exists wsdl.exe is used to generate the corresponding managed wrappers.
            if (OptionSetUtil.OptionSetExists(Project, PropGroupName("webreferences")))
            {
                TaskUtil.ExecuteScriptTask(Project, "task-generatemodulewebreferences", "module", BuildModule);
            }

            // need to inform eaconfig.spu-elfs_to_embed_target that it is trying to call nantToVsTools 
            Properties["eaconfig.spu-elfs_to_embed_target.isRunningNantToVsTools"] = "true";

            BuildNantTovstools();

            _projBuildData = new ProjBuildData();

            cleartOutputBinary = !PropertyExists("outputbinary");

            return true;

        }

        protected override bool BeforeBuildStep()
        {
            // Execute any platform specific custom target that SDK might define:

            TargetUtil.ExecuteTargetIfExists(Project, Project.Properties["eaconfig." + ConfigPlatform + PreBuildTargetProperty], true);

            return true;
        }

        protected override bool PrepareBuild()
        {
            ValidateModuleDependencies(BuildData);

            SetupProjectInput();

            SetupComAssembliesAssemblies();

            return true;
        }


        protected void SetupReferences()
        {
            FileSet CopylocalReferences = new FileSet();
            FileSet References = new FileSet();
            References.FailOnMissingFile = false;
            CopylocalReferences.FailOnMissingFile = false;

            OptionSet CopylocalProjectReferences = null;
            OptionSet ProjectReferences = null;

            switch (BuildData.CopyLocal)
            {
                case BaseBuildData.CopyLocalType.True:
                    FileSetUtil.FileSetAppend(CopylocalReferences, BuildData.BuildFileSets["build.vsproj.assemblies.all"]);
                    break;
                case BaseBuildData.CopyLocalType.Slim:
                    FileSetUtil.FileSetAppend(CopylocalReferences, BuildData.BuildFileSets["build.vsproj.assemblies.all"]);
                    FileSetUtil.FileSetAppend(References, BuildData.BuildFileSets["build.assemblies"]);
                    break;
                default:
                    FileSetUtil.FileSetAppend(References, BuildData.BuildFileSets["build.vsproj.assemblies.all"]);
                    break;
            }

            if (PropertyExists("package.eaconfig.isusingvc8"))
            {
                CopylocalProjectReferences = OptionSetUtil.GetOptionSet(Project, "__private_" + GroupName + "_copylocal_projectreferences");
                ProjectReferences = OptionSetUtil.GetOptionSet(Project, "__private_" + GroupName + "_projectreferences");
            }

            if (CopylocalProjectReferences == null)
            {
                CopylocalProjectReferences = new OptionSet();
            }
            if (ProjectReferences == null)
            {
                ProjectReferences = new OptionSet();
            }

            Project.NamedOptionSets["__private_" + GroupName + "_copylocal_projectreferences"] = CopylocalProjectReferences;
            Project.NamedOptionSets["__private_" + GroupName + "_projectreferences"] = ProjectReferences;

            BuildData.BuildFileSets["__private_copylocal_references"] = CopylocalReferences;
            BuildData.BuildFileSets["__private_references"] = References;
        }

        private void SetupComAssembliesAssemblies()
        {
            if (FileSetUtil.FileSetExists(Project, PropGroupName("comassemblies")))
            {
                if (BuildData.BuildType.IsCLR)
                {
                    IDictionary<string, string> taskParameters = new Dictionary<string, string>()
                    {
                        {"module", BuildModule},
                        {"group", BuildGroupName},
                        {"generated-assemblies-fileset", "__private_com_net_wrapper_references"}
                    };

                    TaskUtil.ExecuteScriptTask(Project, "task-generatemoduleinteropassemblies", taskParameters);

                    string referencesFilesetName = (BuildData.CopyLocal == BuildData.CopyLocalType.True) ?
                                                    "__private_copylocal_references" :
                                                    "__private_references";

                    FileSetUtil.FileSetAppend(BuildData.BuildFileSets[referencesFilesetName], Project.NamedFileSets["__private_com_net_wrapper_references"]);
                    FileSetFunctions.FileSetUndefine(Project, "__private_com_net_wrapper_references");

                }
                else
                {
                    BuildData.BuildFileSets["build.comassemblies.all"] = FileSetUtil.GetFileSet(Project, PropGroupName("comassemblies"));
                }
            }
            if(!BuildData.BuildFileSets.ContainsKey("build.comassemblies.all"))
            {
                BuildData.BuildFileSets["build.comassemblies.all"] = new FileSet();
            }
        }


        private void BuildNantTovstools()
        {
            PropertyUtil.SetPropertyIfMissing(Project, "skip-build-nanttovstools", "false");

            if (PropertyUtil.GetBooleanProperty(Project, "skip-build-nanttovstools"))
            {
                TargetUtil.ExecuteTarget(Project, "vcproj-use-nantToVSTools", false);
            }
            else
            {
                TargetUtil.ExecuteTarget(Project, "vcproj-build-nantToVSTools", false);
            }
        }

        private void ValidateModuleDependencies(BaseBuildData basebuildData)
        {

            SlnVisualStudioDependent.Execute(Project);

			string vsVersion = Project.Properties["package.VisualStudio.version"];

			if (Object.ReferenceEquals(vsVersion, null))
			{
				Error.Throw(Project, Location, "EaconfigVcproj", "Visual studio version 'package.VisualStudio.version' not defined.");
			}
			else if (0 > StringUtil.StrCompareVersions(vsVersion, "8.0.0"))
            {
                Error.Throw(Project, Location, "EaconfigVcproj", "Visual studio older than VS 2005 are not supported.");
            }

            SetupAssemblyAndLibReferences(false);
        }

        private string GetProjName(string crossPropName)
        {
            string proj_name = PropertyUtil.GetPropertyOrDefault(Project, BuildModule + crossPropName, BuildModule);

            //To avoid single module name clashes, prefix the project name with groupname if it is not a runtime module
            if (BuildGroupName != "runtime" && !PropertyExists(PropBuildGroup("buildmodules")))
            {
                if (PropertyExists(BuildGroupName + "-" + PackageName + crossPropName))
                {
                    proj_name = Properties[BuildGroupName + "-" + PackageName + crossPropName];
                }
                else
                {
                    proj_name = BuildGroupName + "-" + PackageName;
                }
            }
            return proj_name;
        }

        private void SaveBuildDataToProject(BuildData builddata)
        {
            Project.NamedOptionSets[BuildTypeName] = builddata.BuildOptionSet;

            foreach (KeyValuePair<string, string> de in builddata.BuildProperties)
            {
                Project.Properties[de.Key] = (de.Value == null) ? String.Empty : de.Value;
            }

            foreach (KeyValuePair<string, FileSet> de in builddata.BuildFileSets)
            {
                Project.NamedFileSets[de.Key] = (de.Value == null) ? new FileSet() : de.Value;
            }
        }
        
        private static readonly string[] UNSUPPORTED_PLATFORMS = { "psp", "ds" };

        private ProjBuildData _projBuildData;

        private string _buildProjectName;

        protected bool cleartOutputBinary = true; 

    }
}
