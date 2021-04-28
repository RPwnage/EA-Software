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


    public abstract class EaconfigTargetBase : EaconfigBuildTask
    {

        public EaconfigTargetBase(Project project, string groupname, string buildModule)
            : base(project, groupname, buildModule)
        {
        }

        public EaconfigTargetBase()
            : base()
        {
        }


        protected abstract BuildData BuildData
        {
            get;
        }

        protected abstract string BuildProjectName
        {
            get;
        }

        protected BuildOutput BuildOutput
        {
            get { return _buildOutput; }
        }

        protected abstract string BuildTypeName
        {
            get;
        }

        protected  abstract bool Prebuild();

        protected abstract bool PrepareBuild();

        protected abstract bool BeforeBuildStep();

        protected abstract bool Build();

        protected abstract void PostBuild();


        /// <summary>Execute the task.</summary>
        protected override void ExecuteTask()
        {
            bool ret = Prebuild();

            if (!ret)
            {
                return;
            }
            
            PrivateSetupVariables.Execute(Project, GroupName, BuildModule, BuildData);

            _buildOutput = BuildOutput.GetBuildOutput(Project, GroupName, BuildData.BuildOptionSet, BuildData.BuildType, BuildProjectName);

            SetCustomOutputToOptions();

            BuildData.BuildProperties["build.imgbldoptions"] = GetImgBldOptions();

            ret = PrepareBuild();

            if (!ret)
            {
                return;
            }
            
            SaveBuildDataToProject(BuildData);

            ret = BeforeBuildStep();

            if (!ret)
            {
                return;
            }

            ret = Build();

            if (!ret)
            {
                return;
            }

            PostBuild();
        }

        private void SetCustomOutputToOptions()
        {
            //  If there is a user-defined groupname.outputdir, then replace the existing output location with it
            //  and update the optionset 'build.buildtype.vcproj' with the new location. We do not mess up with the real 
            //  build.buildtype optionset since it might be used by other modules in the same build
            string groupOutputDir = Properties[PropGroupName("outputdir")];

            string outputbinary = PropertyUtil.GetPropertyOrDefault(Project, "outputbinary", BuildOutput.OutputName);

            if (!String.IsNullOrEmpty(groupOutputDir))
            {
                if (BuildData.BuildType.BaseName != "StdLibrary")
                {
                    string[] OPTION_NAME_LIST = new string[] { "linkoutputname", "linkoutputpdbname", "linkoutputmapname", "imgbldoutputname", "linkoutputnameself", "linkoutputnamesprx" };

                    foreach (string option in OPTION_NAME_LIST)
                    {
                        string optionVal = BuildData.BuildOptionSet.Options[option];
                        if (!String.IsNullOrEmpty(optionVal))
                        {
                            string ext = Path.GetExtension(optionVal);
                            BuildData.BuildOptionSet.Options[option] = groupOutputDir + "/" + outputbinary + ext;
                        }
                    }
                }
                else
                {
                    BuildData.BuildOptionSet.Options["lib.options"] = BuildData.BuildOptionSet.Options["lib.options"].Replace("%outputdir%", groupOutputDir);
                }
            }

            // Replace the tokens (%linkoutputname% %linkoutputmapname% %linkoutputpdbname%) in link.options with the corresponding option value 
            if (BuildData.BuildType.BaseName != "StdLibrary")
            {
                string link_options = BuildData.BuildOptionSet.Options["link.options"];
                if (!String.IsNullOrEmpty(link_options))
                {
                    string[] OPTION_NAME_LIST = new string[] { "linkoutputname", "linkoutputmapname", "linkoutputpdbname" };
                    foreach (string option in OPTION_NAME_LIST)
                    {
                        link_options = link_options.Replace("%" + option + "%", BuildData.BuildOptionSet.Options[option]);
                    }

                    BuildData.BuildOptionSet.Options["link.options"] = link_options;
                }
            }


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


        protected void SetupAssemblyAndLibReferences(bool isNative)
        {
            // Iterate through each of the module-dependencies for the current build
            // module.  Add assembly references or LIBs for each of those modules to
            // the libs of assemblies and libraries for the current module.
            
            if(BuildData.BuildType.IsMakeStyle)
            {
                return;
            }
            if(BuildData.BuildFileSets["build.sourcefiles.all"].FileItems.Count == 0 &&
               BuildData.BuildFileSets["build.asmsourcefiles.all"].FileItems.Count == 0)
            {
                if(BuildData.BuildType.BaseName != "Utility")
                {
                    string msg = Error.Format(Project, "SetupAssemblyAndLibReferences", "WARNING", "Build style target ({0}) specified no source files to build (module name: {1})", Properties["target.name"], BuildModule);
                    Log.WriteLine(msg);
                }
                return;
            }

            string[] groups = (BuildGroupName == "runtime") ? new string[] { BuildGroupName } : new string[] { "runtime", BuildGroupName };

            string currentCC = BuildData.BuildOptionSet.Options["cc"];

            foreach (string group in groups)
            {
                string moduledependents = PropertyUtil.GetPropertyOrDefault(Project, PropGroupName(group + ".moduledependencies"), "") +
                                          "\n" +
                                          PropertyUtil.GetPropertyOrDefault(Project, PropGroupName(group + ".moduledependencies." + ConfigSystem), "");

                // SlnGenerator alters (group + ".buildmodules") and stores original value in (group + ".buildmodules.temp")
                string buildModules = PropertyUtil.GetPropertyOrDefault(Project, group + ".buildmodules.temp", Properties[group + ".buildmodules"]);

                foreach (string dependent in StringUtil.ToArray(moduledependents))
                {
                    if (String.IsNullOrEmpty(buildModules))
                    {
                        // if runtime.buildmodules doesn't exist, then the module dependent must
                        // be unavailable in the current config or unknown, so we ignore it for now
                        if (PropertyExists(group + ".buildtype") || PropertyExists(group + ".builddependencies"))
                        {
                            if (!dependent.Contains("cross") && !Config.Contains("cross"))
                            {
                                if (PackageName != dependent)
                                {
                                    Error.Throw(Project, Location, "EaconfigBuild", "Module '{0}' does not exist in group '{1}' of package '{2}' in config '{3}'!", dependent, group, PackageName, Config);
                                }
                            }
                            else if (dependent.Contains("cross") && Config.Contains("cross"))
                            {
                                if ("cross-" + PackageName != dependent)
                                {
                                    Error.Throw(Project, Location, "EaconfigBuild", "Module '{0}' does not exist in group '{1}' of package '{2}' in config '{3}'!", dependent, group, PackageName, Config);
                                }
                            }
                        }
                    }
                    else
                    {
                        string moduleBuildTypeProp = group + "." + dependent + ".buildtype";

                        if (StringUtil.ArrayContains(buildModules, dependent))
                        {
                            // if the module dependent is one of the current runtime modules
                            if (PropertyExists(moduleBuildTypeProp))
                            {
                                // if the module dependent has buildtype, then try to get to attach it to the build.libs.all
                                BuildType dependentBuildType = ComputeBaseBuildData.GetBuldType(Project, Properties[moduleBuildTypeProp]);
                                if (dependentBuildType.IsCLR)
                                {
                                    FileSet dependentAssemblies = Project.NamedFileSets["__private_" + group + "." + dependent + ".assembly"];
                                    if (dependentAssemblies != null)
                                    {
                                        if (isNative)
                                        {
                                            FileSetUtil.FileSetAppend(BuildData.BuildFileSets["build.assemblies.all"], dependentAssemblies);

                                            if (BuildData.CopyLocal != BuildData.CopyLocalType.False &&
                                               !GroupName.StartsWith("runtime") && group == "runtime")
                                            {
                                                //Copy to the bin\<group> folder for <group>-to-runtime modules
                                                foreach (FileItem file in dependentAssemblies.FileItems)
                                                {
                                                    try
                                                    {
                                                        File.Copy(file.FileName, Path.Combine(BuildOutput.OutputBinDir, Path.GetFileName(file.FileName)), true);
                                                    }
                                                    catch (Exception ex)
                                                    {
                                                        Error.Throw(Project, Location, "EaconfigBuild", "Failed to perform 'CopyLocal' for file '{0}', target directory '{1}'\nDetails: {2}", file.FileName, BuildOutput.OutputBinDir, ex.Message);
                                                    }
                                                }
                                            }
                                        }
                                    }
                                    else if (dependentBuildType.IsEASharp)
                                    {
                                        FileSet dependentlibs = Project.NamedFileSets["__private_" + group + "." + dependent + ".lib"];
                                        if (dependentlibs != null)
                                        {
                                            FileSetUtil.FileSetAppend(BuildData.BuildFileSets["build.libs.all"], dependentlibs);
                                        }
                                        else
                                        {
                                            Error.Throw(Project, Location, "EaconfigBuild", "FileSet '{0}' is missing!", "__private_" + group + "." + dependent + ".lib");
                                        }
                                    }
                                    else if(isNative)
                                    {
                                        Error.Throw(Project, Location, "EaconfigBuild", "FileSet '{0}' is missing!", "__private_" + group + "." + dependent + ".assembly");
                                    }

                                }
                                else if (dependentBuildType.BaseName == "StdLibrary" || dependentBuildType.BaseName == "DynamicLibrary")
                                {
                                    if (isNative)
                                    {
                                        FileSet dependentLibraries = Project.NamedFileSets["__private_" + group + "." + dependent + ".lib"];
                                        FileSet dependentDlls = Project.NamedFileSets["__private_" + group + "." + dependent + ".dll"];
                                        if (dependentLibraries != null)
                                        {
                                            switch (ConfigSystem)
                                            {
                                                case "ps3":

                                                    if (currentCC == OptionSetUtil.GetOption(Project, dependentBuildType.Name, "cc"))
                                                    {
                                                        FileSetUtil.FileSetAppend(BuildData.BuildFileSets["build.libs.all"], dependentLibraries);
                                                    }
                                                    break;
                                                default:

                                                    FileSetUtil.FileSetAppend(BuildData.BuildFileSets["build.libs.all"], dependentLibraries);
                                                    break;
                                            }
                                        }
                                        else if (dependentDlls != null)
                                        {
                                            FileSetUtil.FileSetAppend(BuildData.BuildFileSets["build.dlls.all"], dependentDlls);
                                        }
                                        else
                                        {
                                            Error.Throw(Project, Location, "EaconfigBuild", "For dependent module '{0}' [type={1}, base type={2}] FileSet '{3}.lib [or .dll]' is missing!", dependent, dependentBuildType.Name, dependentBuildType.BaseName, "__private_" + group + "." + dependent);
                                        }
                                    }
                                }
                            }
                            else
                            {
                                Error.Throw(Project, Location, "EaconfigBuild", "Module '{0}' does not have build type defined.", dependent);
                            }
                        }
                        else if (PropertyExists(moduleBuildTypeProp))
                        {
                            Error.Throw(Project, Location, "EaconfigBuild", "Module '{0}' has build type defined, but is not in '{1}.buildmodules'='{2}'!", dependent, group, StringUtil.EnsureSeparator(buildModules, ' '));
                        }
                        else if ((!dependent.Contains("cross") && !Config.Contains("cross")) ||
                                (dependent.Contains("cross") && Config.Contains("cross")))
                        {
                            Error.Throw(Project, Location, "EaconfigBuild", "Module '{0}' is not in '{1}.buildmodules'='{2}'!", dependent, group, StringUtil.EnsureSeparator(buildModules, ' '));
                        }
                    }
                }
            }
        }

        private string GetImgBldOptions()
        {
            string imgBldOptions = String.Empty;
            if (ConfigSystem == "xenon")
            {
                StringBuilder sb = new StringBuilder();
                sb.AppendFormat("-out:\"{0}\"\n", BuildData.BuildOptionSet.Options["imgbldoutputname"]);
                sb.AppendFormat("-in:\"{0}\"\n", BuildData.BuildOptionSet.Options["linkoutputname"]);

                string options = Project.Properties[PropGroupName("imgbld.options")];
                if (!String.IsNullOrEmpty(options))
                {
                    sb.AppendLine(options);
                }

                string defaults = Project.Properties[PropGroupName("imgbld.projectdefaults")];
                if (!String.IsNullOrEmpty(defaults))
                {
                    sb.AppendFormat("-config:{0}", StringUtil.Trim(defaults));
                }
                imgBldOptions = sb.ToString();
            }
            return imgBldOptions;
        }

        private BuildOutput _buildOutput;
    }
}
