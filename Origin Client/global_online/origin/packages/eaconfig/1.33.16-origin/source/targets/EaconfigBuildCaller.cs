using NAnt.Core;
using NAnt.Core.Attributes;

using System;
using System.Collections;
using System.Collections.Generic;
using System.Text;

namespace EA.Eaconfig
{
    [TaskName("EaconfigBuildCaller")]
    public class EaconfigBuildCaller : EAConfigTask
    {

        [TaskAttribute("build-target-name", Required = true)]
        public string BuildTargetName
        {
            get { return _buildTargetName; }
            set { _buildTargetName = value; }
        }

/*
        public static void Execute(Project project, string buildTargetName)
        {
            EaconfigBuildCaller task = new EaconfigBuildCaller(project, buildTargetName);
            task.Execute();
        }
*/
        public EaconfigBuildCaller(Project project, string buildTargetName)
            : base(project)
        {
            BuildTargetName = buildTargetName;
        }

        public EaconfigBuildCaller()
            : base()
        {
        }

        /// <summary>Execute the task.</summary>
        protected override void ExecuteTask()
        {
            string buildGroup = Project.ExpandProperties("eaconfig.${eaconfig.build.group}");
            string groupName = Properties[buildGroup + ".groupname"];

            if (DoModules(groupName))
            {
                BuildWithModules(buildGroup, groupName);
            }
            else
            {
                BuildWithoutModules(buildGroup, groupName);
            }

            // Ensure that the default settings for these two "modes" are reset after this run.
            Properties["eaconfig-build-caller.supportmodules"] = "true";
        }


        private void BuildWithModules(string buildGroup, string groupName)
        {
            string private_buildmodules = Properties[groupName + ".buildmodules"];

            List<string> buildModules = SortBuildModules.Execute(Project, groupName, private_buildmodules);

            foreach (string module in buildModules)
            {
                BuilTargetParameters parameters;

                parameters.GroupName = groupName + "." + module;
                parameters.Module = module;
                parameters.GroupSourceDir = Properties[buildGroup + ".sourcedir"] + "/" + module;
                parameters.OutputName = Properties[parameters.GroupName + ".outputname"];

                PropertyUtil.SetPropertyIfMissing(Project, parameters.GroupName + ".buildtype", "Library");

                InvokeBuildTarget(parameters);
            }
        }

        private void BuildWithoutModules(string buildGroup, string groupName)
        {

            BuilTargetParameters parameters;

            parameters.GroupName = groupName;
            parameters.Module = Properties["package.name"];
            parameters.GroupSourceDir = Properties[buildGroup + ".sourcedir"];
            parameters.OutputName = Properties[parameters.GroupName + ".outputname"];


            // If <group>.buildtype isn't defined then we assume there are
            // no modules in that build group.  Before this check was added
            // there was no way to specify that there were 0 modules in a
            // particular group - it was always assumed that there was at
            // least one module in each group, with default source filesets
            // and properties.  This would lead to build failures when calling
            // the build targets for groups containing 0 modules.  This check
            // provides an explicit way to check for (and ignore) 0 module groups.

            // We also check for <group>.builddependencies, for backwards
            // compatibility.  There are several instances of end-users setting
            // up build files which are really programs in disguise, so we
            // look for <group>.builddependencies to catch that case.  When
            // this change was made, we also re-added the code which defaults
            // the buildtype to Program if it wasn't previously set.  Again,
            // this is for backwards-compatibility.

            if (Properties.Contains(parameters.GroupName + ".buildtype") ||
                Properties.Contains(parameters.GroupName + ".builddependencies"))
            {
                InvokeBuildTarget(parameters);

            }

            // eaconfig-build-caller.domodules can also be false if eaconfig-build-caller.supportmodules is set to false
            // so eaconfig.${eaconfig.build.group}.groupname}.buildmodules might exist. Target 'doc' is an example where
            // eaconfig-build-caller.supportmodules is explicitly set to false.            
            // If falling in here, it means it's not doing anything related to build modules, like 'doc' target. so 
            // we do not need to construct properties like 'build.module' etc.

            else if (!String.IsNullOrEmpty(Properties[groupName + ".buildmodules"]))                
            {
                InvokeBuildTarget(parameters);
            }
        }


        private void InvokeBuildTarget(BuilTargetParameters parameters )
        {
            if(String.IsNullOrEmpty(parameters.OutputName))
            {
                parameters.OutputName = parameters.Module;
            }

            Properties["groupname"] = parameters.GroupName;
            Properties["build.module"] = parameters.Module;
            Properties["groupsourcedir"] = parameters.GroupSourceDir;
            Properties["build.outputname"] = parameters.OutputName;

            bool isEasharp = false;
            if (Properties.Contains(parameters.GroupName + ".buildtype"))
            {
                string realBuildType = GetRealBuildType(Properties[parameters.GroupName + ".buildtype"]);
                isEasharp = OptionSetUtil.GetBooleanOption(Project, realBuildType, "easharp");
            }
            if (isEasharp)
            {
                TargetUtil.ExecuteTarget(Project, "easharp-build-native-from-managed", true);
            }
            else
            {
                TargetUtil.ExecuteTarget(Project, BuildTargetName, true);
            }
        }

        struct BuilTargetParameters
        {
            public String GroupName;
            public String Module;
            public String GroupSourceDir;
            public String OutputName;
        }

        bool DoModules(string groupName)
        {
            bool doModules = false;
            if (String.IsNullOrEmpty(Properties[groupName + ".buildmodules"]))
            {
                doModules = false;
            }
            else
            {
                string support = Properties["eaconfig-build-caller.supportmodules"];
                if (String.IsNullOrEmpty(support))
                {
                    doModules = true;
                }
                else
                {
                    if (support.Equals("true", StringComparison.InvariantCultureIgnoreCase))
                    {
                        doModules = true;
                    }

                }
            }
            return doModules;
        } 

        private string _buildTargetName = String.Empty;
    }
}
