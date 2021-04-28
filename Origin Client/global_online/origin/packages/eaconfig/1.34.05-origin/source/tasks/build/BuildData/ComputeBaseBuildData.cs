using NAnt.Core;
using NAnt.Core.Attributes;
using NAnt.Core.Functions;
using NAnt.Core.Logging;
using NAnt.Core.Tasks;
using NAnt.Core.Util;

using System;
using System.Collections;
using System.Collections.Generic;
using System.Text;
using System.IO;

namespace EA.Eaconfig
{
    public class BuildDataDef
    {
        // These properties initialized using ${property} + ${property.${config-system}}
        protected static readonly string[][] BUILD_PROPERTIES = 
        {
         //             -- target name                  ---  source name            ---  default patterns
         new string[] { "build.usedependencies",        ".usedependencies"      },
         new string[] { "build.interfacedependencies",  ".interfacedependencies"},
         new string[] { "build.builddependencies.all",  ".builddependencies"    },
         new string[] { "build.linkdependencies.all",   ".linkdependencies"     },
         new string[] { "build.defines.all",            ".defines"              },
         new string[] { "build.warningsuppression.all", ".warningsuppression"   },
         new string[] { "build.includedirs.all",        ".includedirs",             "${package.dir}/include", "${package.dir}/${eaconfig.${eaconfig.build.group}.sourcedir}" },
         new string[] { "build.resourcefiles.basedir",  ".resourcefiles.basedir" },
         new string[] { "build.resourcefiles.prefix",   ".resourcefiles.prefix",    "${build.module}"  },
         new string[] { "build.args",                   ".csc-args"              },
         new string[] { "build.win32icon",              ".win32icon"             }
        };

        // These properties are setup using custom rules, but we include them in the cleanup
        protected static readonly string[] CUSTOM_BUILD_PROPERTIES = 
        {
            "build.elfs_to_embed_target",
            // --- C# ---,
            "build.doc"
        };

        protected static readonly string[][] BUILD_FILESETS = 
        {
         //             -- target name                 ---  source name        ---  default patterns
         new string[] { "build.filedependencies.all",  ".filedependencies" },
         new string[] { "build.headerfiles.all",       ".headerfiles",         "${package.dir}/include/**/*.h", "${package.dir}/${groupsourcedir}/**/*.h"  },
         new string[] { "build.sourcefiles.all",       ".sourcefiles"      },
         new string[] { "build.asmsourcefiles.all",    ".asmsourcefiles"   },
         new string[] { "build.libs.all",              ".libs"             },
         new string[] { "build.dlls.all",              ".dlls"             },
         new string[] { "build.objects.all",           ".objects"          },
         new string[] { "build.resourcefiles.all",     ".resourcefiles",       "${package.dir}/${groupsourcedir}/**/*.ico" },
         new string[] { "build.assemblies.all",        ".assemblies"       },
         new string[] { "build.modules.all",           ".modules",             "${package.dir}/${groupsourcedir}/**/*.module" }
        };
        //More to clean:

        //build.bulkbuild.sourcefiles.all

        // Clear all properties and file sets in the project.
        // This can be removed when everything is moved into C#
        protected void Undefine(Project project, string configSystem)
        {
            foreach (string[] propDef in BUILD_PROPERTIES)
            {
                string targetName = propDef[0];

                if (project.Properties.Contains(targetName))
                {
                    project.Properties.Remove(targetName);
                }
            }

            foreach (string propName in CUSTOM_BUILD_PROPERTIES)
            {
                if (project.Properties.Contains(propName))
                {
                    project.Properties.Remove(propName);
                }
            }

            IDictionary filesets = (IDictionary)project.NamedFileSets;
            foreach (string[] filesetDef in BUILD_FILESETS)
            {
                if (filesets[filesetDef[0]] != null)
                {
                    filesets.Remove(filesetDef[0]);
                }
            }
        }
    }

    public class ComputeBaseBuildData : BuildDataDef
    {
        public readonly BaseBuildData BuildData;

        protected readonly Project Project;
        protected readonly string ConfigSystem;
        protected readonly string ConfigCompiler;
        protected readonly string GroupName;
        protected readonly string BuildGroup;
        protected string _buildTypeName;


        public ComputeBaseBuildData(Project project, string configSystem, string configCompiler, string buildTypeName, string groupName, BaseBuildData buildData)
        {
            Project = project;
            ConfigSystem = configSystem;
            ConfigCompiler = configCompiler;
            _buildTypeName = buildTypeName;
            GroupName = groupName;

            BuildData = buildData;

            BuildGroup = Project.Properties["eaconfig.build.group"];
        }

        public static BuildType GetBuldType(Project project, string buildTypeName)
        {
            if (String.IsNullOrEmpty(buildTypeName))
            {
                buildTypeName = "Program";
            }

            // Init common common properties required for the build:
            return GetModuleBaseType.Execute(project, buildTypeName);
        }

        public void UndefineTempNantData()
        {
            base.Undefine(Project, ConfigSystem);
        }

        public void Init()
        {
            // Init common common properties required for the build:
            BuildData.BuildType = GetBuldType(Project, _buildTypeName);

            if (String.IsNullOrEmpty(_buildTypeName))
            {
                Project.Properties[PropGroupName("buildtype")] = BuildData.BuildType.Name;
            }

            // This shoul be alredy set in the build caller.
            Project.Properties["build.buildtype"] = BuildData.BuildType.Name;
            Project.Properties["build.buildtype.base"] = BuildData.BuildType.BaseName;
            Project.Properties["subsystem"] = BuildData.BuildType.SubSystem;

            BuildData.GroupOutputDir = Project.Properties["eaconfig." + BuildGroup + ".outputfolder"];

            OptionSet buildOptionSet = OptionSetUtil.GetOptionSetOrFail(Project, BuildData.BuildType.Name);

            DoDelayedInit(buildOptionSet.Options["delayedinit"]);

            // Create copy of the option set
            BuildData.BuildOptionSet.Append(buildOptionSet);

            BuildData.BulkBuild = PropertyUtil.GetBooleanProperty(Project, "bulkbuild");

            if (Project.Properties[PropGroupName("copylocal")] == null)
            {
                Project.Properties[PropGroupName("copylocal")] = "false";
            }
            string copylocal = PropertyUtil.GetPropertyOrDefault(Project, PropGroupName("copylocal"), "false");
            if (copylocal.Equals("true", StringComparison.InvariantCultureIgnoreCase))
            {
                BuildData.CopyLocal = BaseBuildData.CopyLocalType.True;
            }
            else if (copylocal.Equals("slim", StringComparison.InvariantCultureIgnoreCase))
            {
                BuildData.CopyLocal = BaseBuildData.CopyLocalType.Slim;
            }

            VerifyBulkBuildSetup();

            UndefineTempNantData();

        }

        public void Compute()
        {
            // Setup common build properties and filesets
            SetupBuildProperties();

            SetupBuildFilesets();

            // Depends on SetupBuildFilesets
            SetupUseDependencies();

            SetupBuildDependencies();

            BuildData.BuildFileSets["build.vsproj.assemblies.all"] = new FileSet(BuildData.BuildFileSets["build.assemblies.all"]);

            SetupBulkBuild();

            SetupBuildOptions();

            SetupElfsToEmbed();
        }

        public void SaveNantVariables()
        {
            foreach (KeyValuePair<string, string> de in BuildData.BuildProperties)
            {
                Project.Properties[de.Key] = de.Value;
            }

            foreach (KeyValuePair<string, FileSet> de in BuildData.BuildFileSets)
            {
                Project.NamedFileSets[de.Key] = de.Value;
            }
        }

        protected string PropGroupName(string name)
        {
            if (name.StartsWith("."))
            {
                return GroupName + name;
            }
            return GroupName + "." + name;
        }

        private void DoDelayedInit(string targetName)
        {
            TargetUtil.ExecuteTargetIfExists(Project, targetName, false);
        }

        private void SetupBuildOptions()
        {
            FileSet buildResources;

            if (BuildData.BuildFileSets.TryGetValue("build.resourcefiles.all", out buildResources))
            {
                if (ConfigCompiler == "vc")
                {
                    string linkOptions = BuildData.BuildOptionSet.Options["link.options"];
                    if (linkOptions != null)
                    {
                        StringBuilder sb = new StringBuilder(linkOptions);
                        sb.AppendLine();
                        foreach (FileItem file in buildResources.FileItems)
                        {
                            sb.AppendFormat("-ASSEMBLYRESOURCE:\"{0}\"\n", file.FileName);
                        }

                        BuildData.BuildOptionSet.Options["link.options"] = sb.ToString();
                    }
                }
            }

            //For Wii builds, we offer the option of setting the "sdata" and "sdata2" threshold globally across all modules.
            string private_sdata_options = String.Empty;
            if (ConfigSystem == "rev")
            {

                string sdataVal = Project.Properties["rev.mw.sdata"];
                // If optionset exists overwrite
                if (BuildData.BuildOptionSet.Options.Contains("rev.mw.sdata"))
                {
                    sdataVal = BuildData.BuildOptionSet.Options["rev.mw.sdata"];
                }
                string sdata2Val = Project.Properties["rev.mw.sdata2"];
                // If optionset exists overwrite
                if (BuildData.BuildOptionSet.Options.Contains("rev.mw.sdata2"))
                {
                    sdata2Val = BuildData.BuildOptionSet.Options["rev.mw.sdata2"];
                }
                if (sdataVal != null || sdata2Val != null)
                {
                    StringBuilder sb = new StringBuilder(); // (BuildData.BuildOptionSet.Options["cc.options"]);
                    sb.AppendLine();
                    if (sdataVal != null)
                    {
                        sdataVal = StringUtil.Trim(sdataVal);
                        if (String.IsNullOrEmpty(sdataVal))
                            sdataVal = "0";
                        sb.AppendFormat("-sdata {0}\n", sdataVal);
                    }
                    if (sdata2Val != null)
                    {
                        sdata2Val = StringUtil.Trim(sdata2Val);
                        if (String.IsNullOrEmpty(sdata2Val))
                            sdata2Val = "0";
                        sb.AppendFormat("-sdata2 {0}\n", sdata2Val);
                    }
                    //BuildData.BuildOptionSet.Options["cc.options"] = sb.ToString();
                    private_sdata_options = sb.ToString();
                }
            }
            Project.Properties["__private_sdata_option"] = private_sdata_options;

            bool isvc8Transition = false;
            if (PropertyUtil.PropertyExists(Project, PropGroupName("vc8transition")))
            {
                isvc8Transition = PropertyUtil.GetBooleanProperty(Project, PropGroupName("vc8transition"));
            }
            else if (PropertyUtil.PropertyExists(Project, Project.Properties["package.name"] + ".vc8transition"))
            {
                isvc8Transition = PropertyUtil.GetBooleanProperty(Project, Project.Properties["package.name"] + ".vc8transition");
            }

            if (isvc8Transition && BuildData.BuildType.IsManaged)
            {
                OptionSetUtil.ReplaceOption(BuildData.BuildOptionSet, "cc.options", Project.Properties["package.eaconfig.clrFlag"], "-clr:oldSyntax");

                string buildDefines = BuildData.BuildProperties["build.defines.all"];

                if (!String.IsNullOrEmpty(buildDefines) && !buildDefines.Contains("_CRT_SECURE_NO_DEPRECATE"))
                {
                    BuildData.BuildProperties["build.defines.all"] = buildDefines + "\n" + "_CRT_SECURE_NO_DEPRECATE";
                }
            }

            // We default the location of stub libs for Dlls to ${config}\lib folder
            if (BuildData.BuildType.BaseName == "DynamicLibrary" && ConfigCompiler == "vc")
            {
                string impliboutputname = BuildData.BuildOptionSet.Options["impliboutputname"];

                if (impliboutputname == "%outputdir%/%outputname%.lib")
                {
                    impliboutputname = String.Format(" -implib:\"{0}{1}\\{2}.lib\"",
                                                    Project.Properties["package.configlibdir"],
                                                    Project.Properties["eaconfig." + Project.Properties["eaconfig.build.group"] + ".outputfolder"],
                                                    Project.Properties["build.outputname"]);
                }

                if (!String.IsNullOrEmpty(impliboutputname))
                {
                    // Need to create directory if it does not exist
                    impliboutputname = impliboutputname.Replace("%outputdir%", Project.Properties["outputpath"]);
                    impliboutputname = impliboutputname.Replace("%outputname%", Project.Properties["build.outputname"]);

                    string path = impliboutputname.Replace("-implib:", String.Empty).Trim(new char[] { '\"', ' ', '\r', '\n', '\t' });
                    path = Path.GetDirectoryName(path);
                    // TODO- move creation to the build targets
                    if (!Directory.Exists(path))
                    {
                        Directory.CreateDirectory(path);
                    }

                    BuildData.BuildOptionSet.Options["link.options"] += impliboutputname;
                }
            }

            // Getting package level debug level defines into the build
            if (BuildData.BuildOptionSet.Options["debugflags"] != "off")
            {
                //     Turn autogeneration ONLY when debuglevel is defined
                //        <do if="'@{OptionSetGetValue('build.buildtype.vcproj', 'debugflags')}' != 'off'" >
                //           <property name="__private_debuglevel_define_value" value="1" />
                //        </do>

                string debugLevel = PropertyUtil.GetPropertyOrDefault(Project, "package.debuglevel", "0");
                debugLevel = PropertyUtil.GetPropertyOrDefault(Project, "package." + Project.Properties["package.name"] + ".debuglevel", debugLevel);

                if (debugLevel != "0")
                {
                    string debuglevel_define = Project.Properties["package.name"].ToUpper() + "_DEBUG";
                    string defines = BuildData.BuildOptionSet.Options["cc.defines"];
                    string prop_defines = BuildData.BuildProperties["build.defines.all"];
                    if (defines == null)
                    {
                        defines = String.Empty;
                    }

                    if (!(defines.Contains(debuglevel_define) || prop_defines.Contains(debuglevel_define)))
                    {
                        if (!BuildData.BuildType.IsCSharp && !BuildData.BuildType.IsFSharp)
                        {
                            BuildData.BuildProperties["build.defines.all"] = String.Format("{0}\n{1}={2}", prop_defines, debuglevel_define, debugLevel);
                        }
                        else
                        {
                            BuildData.BuildProperties["build.defines.all"] = String.Format("{0}\n{1}", prop_defines, debuglevel_define);
                        }
                    }
                }
            }

            // For PS3 PPU program type, linker may produce fself directly
            if (ConfigSystem == "ps3" && !BuildData.BuildOptionSet.Options.Contains("ps3-spu"))
            {
                if (BuildData.BuildType.BaseName == "StdProgram" ||
                    BuildData.BuildType.BaseName == "DynamicLibrary")
                {
                    string link_options = BuildData.BuildOptionSet.Options["link.options"];

                    string oformat_secure = String.Empty;
                    string linkoutput_secure = String.Empty;
                    string suffix = String.Empty;
                    string suffix_secure = String.Empty;

                    if (BuildData.BuildType.BaseName == "StdProgram")
                    {
                        oformat_secure = "-oformat=fself";
                        linkoutput_secure = "linkoutputnameself";
                        suffix = Project.Properties["exe-suffix"];
                        suffix_secure = Project.Properties["secured-exe-suffix"];
                    }
                    else // Dll
                    {
                        oformat_secure = "-oformat=fsprx";
                        linkoutput_secure = "linkoutputnamesprx";
                        suffix = Project.Properties["dll-suffix"];
                        suffix_secure = Project.Properties["secured-dll-suffix"];

                    }

                    if (link_options.Contains(oformat_secure))
                    {
                        string linkoutputname = BuildData.BuildOptionSet.Options["linkoutputname"];
                        string linkoutputnamesecure = BuildData.BuildOptionSet.Options[linkoutput_secure];
                        String standard_linkoutputname = @"%outputdir%/%outputname%" + suffix;
                        String standard_linkoutputname1 = @"%outputdir%\%outputname%" + suffix;

                        if (String.IsNullOrEmpty(linkoutputname) || (linkoutputname.Contains(standard_linkoutputname) || linkoutputname.Contains(standard_linkoutputname1)))
                        {
                            BuildData.BuildOptionSet.Options["linkoutputname"] = BuildData.BuildOptionSet.Options[linkoutput_secure];
                        }
                        else
                        {
                            BuildData.BuildOptionSet.Options["linkoutputname"] = linkoutputname.Replace(suffix, suffix_secure);
                        }
                    }
                }
            }

            // Create SNtemp directory
            if (ConfigSystem == "ps3" && ConfigCompiler == "sn")
            {
                string ccoptions = OptionSetUtil.GetOption(BuildData.BuildOptionSet, "cc.options");
                if (!String.IsNullOrEmpty(ccoptions))
                {
                    int ind0 = ccoptions.IndexOf("-td=");
                    if (ind0 > -1 && ind0 + 5 < ccoptions.Length)
                    {
                        ind0 += 4;
                        int ind1 = ccoptions.IndexOfAny(new char[] { '\n', '\r' }, ind0);
                        if (ind1 < 0) ind1 = ccoptions.Length;
                        string snTempPath = ccoptions.Substring(ind0, ind1 - ind0);

                        snTempPath = PathNormalizer.Normalize(PathNormalizer.StripQuotes(StringUtil.Trim(snTempPath)));

                        if (!Directory.Exists(snTempPath))
                        {
                            Directory.CreateDirectory(snTempPath);
                        }
                    }
                }
            }

            UpdateLinkOptionsWithBaseAddAndDefFile();
        }

        private void UpdateLinkOptionsWithBaseAddAndDefFile()
        {
            if (ConfigSystem == "pc" || ConfigSystem == "pc64" || ConfigSystem == "Xenon")
            {
                StringBuilder link_options = new StringBuilder();

                if (BuildData.BuildType.BaseName == "StdProgram" || BuildData.BuildType.BaseName == "DynamicLibrary")
                {
                    string defFile = Project.Properties[PropGroupName(".definition-file." + ConfigSystem)];
                    if (String.IsNullOrEmpty(defFile))
                    {
                        defFile = Project.Properties[PropGroupName(".definition-file")];
                    }
                    if (!String.IsNullOrEmpty(defFile))
                    {
                        link_options.AppendFormat("-DEF:{0}\n", defFile);
                    }

                    string baseAddr = Project.Properties[PropGroupName(".base-address." + ConfigSystem)];
                    if (String.IsNullOrEmpty(baseAddr))
                    {
                        baseAddr = Project.Properties[PropGroupName(".base-address")];
                    }
                    if (!String.IsNullOrEmpty(baseAddr))
                    {
                        link_options.AppendFormat("-BASE:{0}\n", baseAddr);
                    }

                    if (link_options.Length > 0)
                    {
                        BuildData.BuildOptionSet.Options["link.options"] += "\n" + link_options.ToString();
                    }
                }
            }
        }

        private void VerifyBulkBuildSetup()
        {
            // BulkBuild features aren't available to C# or F# modules.
            if (BuildData.BuildType.IsCSharp || BuildData.BuildType.IsFSharp)
            {
                if (FileSetUtil.FileSetExists(Project, PropGroupName("bulkbuild.manual.sourcefiles")))
                {
                    Error.Throw(Project, "SetupBulkBuild", "BulkBuild features aren't available to C# or F# modules, but '{0}.bulkbuild.manual.sourcefiles' was specified", GroupName);
                }
                if (FileSetUtil.FileSetExists(Project, PropGroupName("bulkbuild.manual.sourcefiles." + ConfigSystem)))
                {
                    Error.Throw(Project, "SetupBulkBuild", "BulkBuild features aren't available to C# or F# modules, but '{0}.bulkbuild.manual.sourcefiles.{1}' was specified", GroupName, ConfigSystem);
                }
                if (PropertyUtil.PropertyExists(Project, PropGroupName("bulkbuild.filesets")))
                {
                    Error.Throw(Project, "SetupBulkBuild", "BulkBuild features aren't available to C# or F# modules, but '{0}.bulkbuild.filesets' was specified", GroupName);
                }
            }

            if (BuildData.BulkBuild)
            {
                if (FileSetUtil.FileSetExists(Project, PropGroupName("bulkbuild.sourcefiles")) &&
                    PropertyUtil.PropertyExists(Project, PropGroupName("bulkbuild.filesets")))
                {
                    Error.Throw(Project, "VerifyBulkBuildSetup", "You cannot have both '{0}' and '{1}' defined.", PropGroupName("bulkbuild.sourcefiles"), PropGroupName("bulkbuild.filesets"));
                }
                if (PropertyUtil.PropertyExists(Project, PropGroupName("bulkbuild.filesets")))
                {
                    foreach (string filesetName in StringUtil.ToArray(Project.Properties[PropGroupName("bulkbuild.filesets")]))
                    {
                        if (!FileSetUtil.FileSetExists(Project, PropGroupName("bulkbuild." + filesetName + ".sourcefiles")))
                        {
                            Error.Throw(Project, "VerifyBulkBuildSetup", "FileSet: '{0} was specified in '{1} but it doesn't exits..",
                                        PropGroupName("bulkbuild." + filesetName + ".sourcefiles"),
                                        PropGroupName("bulkbuild.filesets"));
                        }
                    }
                }
            }
        }

        private void SetupBuildProperties()
        {
            foreach (string[] propDef in BUILD_PROPERTIES)
            {
                string targetName = propDef[0];
                string sourceName = propDef[1];

                string propertyValue = String.Empty;

                // Set default values
                if (propDef.Length > 2)
                {
                    // None found, and have default initializers:
                    string sep = String.Empty;
                    for (int i = 2; i < propDef.Length; i++)
                    {
                        propertyValue += sep + Project.ExpandProperties(propDef[i]);
                        sep = "\n";
                    }
                }

                string val = Project.Properties[PropGroupName(sourceName)];

                if (!String.IsNullOrEmpty(val))
                {
                    propertyValue = val;
                }

                val = Project.Properties[PropGroupName(sourceName + "." + ConfigSystem)];

                if (!String.IsNullOrEmpty(val))
                {
                    if (!String.IsNullOrEmpty(propertyValue))
                    {
                        propertyValue += "\n" + val;
                    }
                    else
                    {
                        propertyValue = val;
                    }
                }

                BuildData.BuildProperties[targetName] = propertyValue;
            }

            // Normalize and strip duplicate includedirs in build.includedirs.all
            string newNormalizedPaths = EA.Eaconfig.PropertyUtil.AddNonDuplicatePathToNormalizedPathListProp(null, BuildData.BuildProperties["build.includedirs.all"]);
            if (!String.IsNullOrEmpty(newNormalizedPaths))
            {
                BuildData.BuildProperties["build.includedirs.all"] = newNormalizedPaths;
            }

            // ******** CUSTOM CASES ***************

            // ------ linkdependencies ------

            // Append link dependencies to build dependencies.

            string linkDependencies = BuildData.BuildProperties["build.linkdependencies.all"];
            if (!String.IsNullOrEmpty(linkDependencies))
            {
                BuildData.BuildProperties["build.builddependencies.all"] += "\n" + linkDependencies;
            }

            // ------ resourcefiles -----

            if (String.IsNullOrEmpty(BuildData.BuildProperties["build.resourcefiles.basedir"]))
            {
                if (Project.NamedFileSets[GroupName + ".sourcefiles"] != null)
                {
                    BuildData.BuildProperties["build.resourcefiles.basedir"] = FileSetFunctions.FileSetGetBaseDir(Project, GroupName + ".sourcefiles");
                }
            }

            // ------ build.doc ----- 

            string cscDoc = Project.Properties[GroupName + ".csc-doc"];
            if (cscDoc != null)
            {
                BuildData.BuildProperties["build.doc"] = Project.ExpandProperties("${package.configbuilddir}${eaconfig.${eaconfig.build.group}.outputfolder}/${build.module}/${build.module}.xml");
                if (!(cscDoc == "true" || cscDoc == "false"))
                {
                    String msg = Error.Format(Project, "BaseBuildData", "WARNING", "property '{0}.csc-doc'={1}, it should only be either 'true' or 'false', Other string values are considered invalid!", GroupName, cscDoc);
                    Log.WriteLine(msg);
                }
            }
            else
            {
                BuildData.BuildProperties["build.doc"] = String.Empty;
            }

            // ------ defines ----- 

            //If the end-user has specified a "winver" property, then use that to add WINVER and _WIN32_WINNT
            //#defines.   Otherwise, we default to the settings for Windows XP (0x501).
            //
            //See this blog entry for more information on these defines.
            //
            //"What's the difference between WINVER, _WIN32_WINNT, _WIN32_WINDOWS, and _WIN32_IE?"
            //http://blogs.msdn.com/oldnewthing/archive/2007/04/11/2079137.aspx 
            //
            //Apprarently microsoft has removed some of the forced _WIN32_IE define in windowssdk, which used to exist in VC8 platformsdk, see the following lines existing in the original platformsdk but not in the new windowssdk
            //  C:\Program Files\Microsoft Visual Studio 8\VC\PlatformSDK\Include\CommCtrl.h   
            //      #ifndef _WINRESRC_
            //      #ifndef _WIN32_IE
            //      #define _WIN32_IE 0x0501
            //      #else
            //      #if (_WIN32_IE < 0x0400) && defined(_WIN32_WINNT) && (_WIN32_WINNT >= 0x0500)
            //      #error _WIN32_IE setting conflicts with _WIN32_WINNT setting
            //      #endif
            //      #endif
            //      #endif

            if ((ConfigSystem == "pc" || ConfigSystem == "pc64") && !BuildData.BuildType.IsCSharp && !BuildData.BuildType.IsFSharp)
            {
                // This is not applicable for C# modules-->
                string winver = Project.Properties[PropGroupName("defines.winver")];
                if (String.IsNullOrEmpty(winver))
                {
                    winver = Project.Properties["eaconfig.defines.winver"];
                }
                if (String.IsNullOrEmpty(winver))
                {
                    if (ConfigSystem == "pc64")
                    {
                        winver = "0x0601";
                    }
                    else
                    {
                        winver = "0x0501";
                    }
                }
                winver = winver.Trim();

                string defines = "\n" + "_WIN32_WINNT=" + winver + "\n" + "WINVER=" + winver;

                if (PropertyUtil.PropertyExists(Project, "package.eaconfig.isusingvc9"))
                {
                    defines += "\n" + "_WIN32_IE=" + winver;
                }

                BuildData.BuildProperties["build.defines.all"] += defines + "\n";

            }


        }

        private void SetupBuildFilesets()
        {
            // --- Create a default sourcefiles property if the package does not define it
            // TODO: can't we use default initializer instead of defining this in <group>.sourcefiles? need this for bulkbuild setup?
            if (!(FileSetUtil.FileSetExists(Project, PropGroupName("sourcefiles")) || FileSetUtil.FileSetExists(Project, PropGroupName("sourcefiles." + ConfigSystem))))
            {
                FileSet sourceFiles = FileSetUtil.CreateFileSetInProject(Project, PropGroupName("sourcefiles"), Project.Properties["package.dir"] + "/" + Project.Properties["groupsourcedir"]);
                FileSetUtil.FileSetInclude(sourceFiles, "**/*.cpp");
            }

            // --- Merge all files

            foreach (string[] fileSetdef in BUILD_FILESETS)
            {
                int appended = 0;
                string targetName = fileSetdef[0];
                string sourceName = fileSetdef[1];

                FileSet targetFileSet = new FileSet();
                targetFileSet.FailOnMissingFile = false;

                if (sourceName == ".sourcefiles")
                {
                    appended += FileSetUtil.FileSetAppendWithBaseDir(targetFileSet,
                                                    FileSetUtil.GetFileSet(Project, PropGroupName(sourceName)));
                    appended += FileSetUtil.FileSetAppendWithBaseDir(targetFileSet,
                                                    FileSetUtil.GetFileSet(Project, PropGroupName(sourceName + "." + ConfigSystem)));
                }
                else
                {
                    appended += FileSetUtil.FileSetAppend(targetFileSet,
                                                    FileSetUtil.GetFileSet(Project, PropGroupName(sourceName)));
                    appended += FileSetUtil.FileSetAppend(targetFileSet,
                                                    FileSetUtil.GetFileSet(Project, PropGroupName(sourceName + "." + ConfigSystem)));
                }

                if (appended == 0 && fileSetdef.Length > 2)
                {
                    // None found, and have default initializers:
                    for (int i = 2; i < fileSetdef.Length; i++)
                    {
                        FileSetUtil.FileSetInclude(targetFileSet, Project.ExpandProperties(fileSetdef[i]));
                    }
                }
                BuildData.BuildFileSets[targetName] = targetFileSet;
            }

            //TODO - do I need this at all?
            //Is tis done to distingush managed/unmanaged? (build.resourcefiles does not include default "${package.dir}/${groupsourcedir}/**/*.ico"
            BuildData.BuildFileSets["build.resourcefiles"] = new FileSet();
            FileSetUtil.FileSetAppend(BuildData.BuildFileSets["build.resourcefiles"],
                                      FileSetUtil.GetFileSet(Project, PropGroupName("resourcefiles")));

            if (BuildData.BuildType.IsCSharp || BuildData.BuildType.IsFSharp)
            {
                FileSet libs = FileSetUtil.GetFileSet(Project, PropGroupName("libs"));
                if (libs != null)
                {
                    Log.WriteLine(Error.Format(Project, "SetupBuildFilesets", "WARNING", "Use of <groupname>.libs fileset for C# or F# modules is deprecated.  Please use <groupname>.assemblies instead."));
                    FileSetUtil.FileSetAppendWithBaseDir(BuildData.BuildFileSets["build.assemblies.all"], libs);
                }
            }

            if (BuildData.CopyLocal == BaseBuildData.CopyLocalType.Slim)
            {
                BuildData.BuildFileSets["build.assemblies"] = BuildData.BuildFileSets["build.assemblies.all"];
                BuildData.BuildFileSets["build.assemblies.all"] = new FileSet();
            }

            BuildData.BuildFileSets["build.vsproj.assemblies.all"] = new FileSet(BuildData.BuildFileSets["build.assemblies.all"]);

        }

        private void SetupElfsToEmbed()
        {
            string elfToEmbedTarget = Project.Properties[PropGroupName("elfs_to_embed_target")];

            if (!String.IsNullOrEmpty(elfToEmbedTarget))
            {
                if (TargetUtil.TargetExists(Project, elfToEmbedTarget))
                {
                    BuildData.BuildProperties["build.elfs_to_embed_target"] = elfToEmbedTarget;
                }
            }
            else if (TargetUtil.TargetExists(Project, GroupName + ".elfs_to_embed_target"))
            {
                BuildData.BuildProperties["build.elfs_to_embed_target"] = elfToEmbedTarget = PropGroupName("elfs_to_embed_target");
            }
            else
            {
                BuildData.BuildProperties["build.elfs_to_embed_target"] = elfToEmbedTarget = "eaconfig.spu-elfs_to_embed_target";
            }

            if (!String.IsNullOrEmpty(elfToEmbedTarget) && TargetUtil.TargetExists(Project, elfToEmbedTarget))
            {
                FileSet elfFiles = FileSetUtil.GetFileSet(Project, PropGroupName("elfs_to_embed"));
                if (elfFiles != null)
                {
                    BuildData.BuildFileSets["build.elfs_to_embed"] = new FileSet(elfFiles);

                    TargetUtil.ExecuteTarget(Project, elfToEmbedTarget, true);
                }
            }

        }

        private void SetupBulkBuild()
        {
            bool isReallyBulkBuild = false;

            string bulkbuildOutputdir = Project.ExpandProperties("${package.configbuilddir}${eaconfig.${eaconfig.build.group}.outputfolder}/source");

            FileSet source = new FileSet();
            FileSet bulkBuildSource = new FileSet();

            source.FailOnMissingFile = false;
            bulkBuildSource.FailOnMissingFile = false;

            if (BuildData.BulkBuild)
            {
                // As of eaconfig-1.21.01, unless explicitly specified, we're now NOT enabling partial
                // BulkBuild.  Metrics gathered on Def Jam and C&C show that it's just TOO slow. 
                bool enablePartialBulkBuild = PropertyUtil.GetBooleanProperty(Project, "package.enablepartialbulkbuild");

                FileSet manualSource = FileSetUtil.GetFileSet(Project, PropGroupName("bulkbuild.manual.sourcefiles"));
                FileSet manualSourceCfg = FileSetUtil.GetFileSet(Project, PropGroupName("bulkbuild.manual.sourcefiles." + ConfigSystem));

                if (manualSource != null || manualSourceCfg != null)
                {
                    isReallyBulkBuild = true;

                    FileSetUtil.FileSetAppendWithBaseDir(source, manualSource);
                    FileSetUtil.FileSetAppendWithBaseDir(source, manualSourceCfg);

                    FileSetUtil.FileSetAppendWithBaseDir(bulkBuildSource, manualSource);
                    FileSetUtil.FileSetAppendWithBaseDir(bulkBuildSource, manualSourceCfg);
                }

                //This is the simple case where the user wants to bulkbuild the runtime.sourcefiles.
                if (FileSetUtil.FileSetExists(Project, PropGroupName("bulkbuild.sourcefiles")))
                {
                    isReallyBulkBuild = true;

                    if (OsUtil.PlatformID == (int)OsUtil.OsUtilPlatformID.MacOSX)
                    {
                        int bbSourceCount = 0;

                        FileSet bbSourceFiles = FileSetUtil.GetFileSet(Project, PropGroupName("bulkbuild.sourcefiles"));
                        if (!enablePartialBulkBuild)
                        {
                            bbSourceCount = bbSourceFiles.FileItems.Count;
                        }
                        FileSetUtil.FileSetExclude(bbSourceFiles, bbSourceFiles.BaseDirectory + "/**.mm");
                        FileSetUtil.FileSetExclude(bbSourceFiles, bbSourceFiles.BaseDirectory + "/**.m");
                        if (!enablePartialBulkBuild && (bbSourceCount != bbSourceFiles.FileItems.Count))
                        {
                            enablePartialBulkBuild = true;
                        }
                    }

                    //Loose = common + platform-specific - bulkbuild
                    FileSet looseSourcefiles = null;
                    if (enablePartialBulkBuild)
                    {
                        looseSourcefiles = new FileSet();

                        FileSetUtil.FileSetInclude(looseSourcefiles, FileSetUtil.GetFileSet(Project, PropGroupName("sourcefiles")));
                        FileSetUtil.FileSetInclude(looseSourcefiles, FileSetUtil.GetFileSet(Project, PropGroupName("sourcefiles." + ConfigSystem)));
                        FileSetUtil.FileSetExclude(looseSourcefiles, FileSetUtil.GetFileSet(Project, PropGroupName("bulkbuild.sourcefiles")));
                    }


                    string outputFile = "BB_" + GroupName + ".cpp";
                    string outputPath = bulkbuildOutputdir + "/" + outputFile;

                    string maxBulkbuildSize = PropertyUtil.GetPropertyOrDefault(Project, PropGroupName("max-bulkbuild-size"),
                                                                                PropertyUtil.GetPropertyOrDefault(Project, "eaconfig.max-bulkbuild-size", String.Empty));


                    // Generate bulkbuild CPP file

                    FileSet bbFiles = EA.GenerateBulkBuildFiles.generatebulkbuildfiles.Execute(Project, GroupName + ".bulkbuild.sourcefiles", outputFile, bulkbuildOutputdir, maxBulkbuildSize);

                    //Add bulkbuild CPP to source fileset
                    FileSetUtil.FileSetAppend(source, bbFiles);
                    FileSetUtil.FileSetAppend(bulkBuildSource, bbFiles);


                    //Add loose files to source fileset 
                    if (enablePartialBulkBuild)
                    {
                        FileSetUtil.FileSetAppend(source, looseSourcefiles);
                    }
                }

                //This is the case where the user supplies bulkbuild filesets. This means that the runtime.bulkbuild.fileset Property exists.
                if (PropertyUtil.PropertyExists(Project, PropGroupName("bulkbuild.filesets")))
                {
                    isReallyBulkBuild = true;

                    if (OsUtil.PlatformID == (int)OsUtil.OsUtilPlatformID.MacOSX)
                    {
                        int bbSourceCount = 0;

                        foreach (string bbFileSetName in StringUtil.ToArray(Project.Properties[PropGroupName("bulkbuild.filesets")]))
                        {
                            FileSet bbSourceFiles = FileSetUtil.GetFileSet(Project, PropGroupName("bulkbuild." + bbFileSetName + ".sourcefiles"));
                            if (bbSourceFiles != null)
                            {
                                if (!enablePartialBulkBuild)
                                {
                                    bbSourceCount = bbSourceFiles.FileItems.Count;
                                }
                                FileSetUtil.FileSetExclude(bbSourceFiles, bbSourceFiles.BaseDirectory + "/**.mm");
                                FileSetUtil.FileSetExclude(bbSourceFiles, bbSourceFiles.BaseDirectory + "/**.m");

                                if (!enablePartialBulkBuild && (bbSourceCount != bbSourceFiles.FileItems.Count))
                                {
                                    enablePartialBulkBuild = true;
                                }
                            }
                            else
                            {
                                Error.Throw(Project, "SetupBulkBuild", "Fileset '{0}' specified in property '{1}'='{2}' does not exist ", PropGroupName("bulkbuild." + bbFileSetName + ".sourcefiles"), PropGroupName("bulkbuild.filesets"), Project.Properties[PropGroupName("bulkbuild.filesets")]);
                            }
                        }
                    }

                    //Loose = common + platform-specific - bulkbuild(multiple ones)
                    FileSet looseSourcefiles = null;
                    if (enablePartialBulkBuild)
                    {
                        looseSourcefiles = new FileSet();

                        FileSetUtil.FileSetInclude(looseSourcefiles, FileSetUtil.GetFileSet(Project, PropGroupName("sourcefiles")));
                        FileSetUtil.FileSetInclude(looseSourcefiles, FileSetUtil.GetFileSet(Project, PropGroupName("sourcefiles." + ConfigSystem)));


                        foreach (string bbFileSetName in StringUtil.ToArray(Project.Properties[PropGroupName("bulkbuild.filesets")]))
                        {
                            FileSetUtil.FileSetExclude(looseSourcefiles, FileSetUtil.GetFileSet(Project, PropGroupName("bulkbuild." + bbFileSetName + ".sourcefiles")));
                        }
                    }

                    // Generate bulkbuild CPP file
                    foreach (string bbFileSetName in StringUtil.ToArray(Project.Properties[PropGroupName(".bulkbuild.filesets")]))
                    {
                        string outputFile = "BB_" + GroupName + "_" + bbFileSetName + ".cpp";
                        string outputPath = bulkbuildOutputdir + "/" + outputFile;

                        string maxBulkbuildSize = PropertyUtil.GetPropertyOrDefault(Project, PropGroupName(bbFileSetName + ".max-bulkbuild-size"),
                                                           PropertyUtil.GetPropertyOrDefault(Project, PropGroupName("max-bulkbuild-size"),
                                                           PropertyUtil.GetPropertyOrDefault(Project, "eaconfig.max-bulkbuild-size", String.Empty)));

                        FileSet bbFiles = EA.GenerateBulkBuildFiles.generatebulkbuildfiles.Execute(Project, GroupName + ".bulkbuild." + bbFileSetName + ".sourcefiles", outputFile, bulkbuildOutputdir, maxBulkbuildSize);

                        //Add bulkbuild CPP to source fileset
                        FileSetUtil.FileSetAppend(source, bbFiles);
                        FileSetUtil.FileSetAppend(bulkBuildSource, bbFiles);
                    }

                    //Add loose files to source fileset 
                    if (enablePartialBulkBuild)
                    {
                        FileSetUtil.FileSetAppend(source, looseSourcefiles);
                    }
                }
            }

            if (isReallyBulkBuild)
            {
                BuildData.BuildFileSets["build.bulkbuild.sourcefiles.all"] = bulkBuildSource;
            }
            else
            {
                //This is the non-bulkbuild case. Set up build sourcefiles.
                FileSetUtil.FileSetAppendWithBaseDir(source, FileSetUtil.GetFileSet(Project, PropGroupName("sourcefiles")));
                FileSetUtil.FileSetAppendWithBaseDir(source, FileSetUtil.GetFileSet(Project, PropGroupName("sourcefiles." + ConfigSystem)));

                if (ConfigSystem == "rev" && BuildData.BuildType.BaseName == "DynamicLibrary")
                {
                    string global_destructor_src = PropertyUtil.GetPropertyOrDefault(Project, "rso.global_destructor.src", String.Empty);
                    if (!String.IsNullOrEmpty(global_destructor_src))
                    {
                        FileSetUtil.FileSetInclude(source, Project.ExpandProperties(global_destructor_src));
                    }
                }

                BuildData.BuildFileSets["build.bulkbuild.sourcefiles.all"] = new FileSet();
            }

            BuildData.BuildFileSets["build.sourcefiles.all"] = source;
        }

        private void SetupUseDependencies()
        {
            string interfacedep = BuildData.BuildProperties["build.interfacedependencies"];
            string usedep = BuildData.BuildProperties["build.usedependencies"];
            string packageName = Project.Properties["package.name"];

            // --- Set interfacedependencies ---

            foreach (string depName in StringUtil.ToArray(interfacedep))
            {
                if (packageName == depName)
                {
                    Error.Throw(Project, "SetupUseDependencies", "Package '{0}' cannot depend on itself as usedependencies.", packageName);
                }

                // --- use the <dependent> task to do the actual work

                TaskUtil.Dependent(Project, depName, Project.TargetStyleType.Use);

                AppendDependentIncludeDirs(depName);
            }

            // --- Set usedependencies ---

            foreach (string depName in StringUtil.ToArray(usedep))
            {
                if (packageName == depName)
                {
                    Error.Throw(Project, "SetupUseDependencies", "Package '{0}' cannot depend on itself as usedependencies.", packageName);
                }

                // --- use the <dependent> task to do the actual work

                TaskUtil.Dependent(Project, depName, Project.TargetStyleType.Use);

                AppendDependentIncludeDirs(depName);

                if (BuildData.BuildType.IsCLR)
                {
                    // We probably shouldn't call the following task if we're currently building a solution file. 
                    if (BuildData.CopyLocal == BaseBuildData.CopyLocalType.True || BuildData.CopyLocal == BaseBuildData.CopyLocalType.Slim)
                    {
                        CopyUseDependentAssemblies(depName);
                    }

                    FileSetUtil.FileSetAppend(BuildData.BuildFileSets["build.assemblies.all"],
                                              FileSetUtil.GetFileSet(Project, "package." + depName + ".assemblies"));
                }

                // --- Add in all the usedependencies libs.
                //     The builddependencies libs will be added only for <build>, since VS already implicitly includes them
                AppendDependentLibraries(depName);
            }
        }

        private void SetupBuildDependencies()
        {
            string buildDependencies = BuildData.BuildProperties["build.builddependencies.all"];
            string linkDependencies = BuildData.BuildProperties["build.linkdependencies.all"];
            Dictionary<string, string> linkDepHash = null;
            if (!String.IsNullOrEmpty(linkDependencies))
            {
                ICollection<string> linkPackages = StringUtil.ToArray(linkDependencies);
                if (linkPackages.Count > 0)
                {
                    linkDepHash = new Dictionary<string, string>(linkPackages.Count);

                    foreach (string dep in linkPackages)
                    {
                        linkDepHash[dep] = dep;
                    }
                }
            }

            foreach (string depName in StringUtil.ToArray(buildDependencies))
            {
                TaskUtil.Dependent(Project, depName);

                // If we list the package as build dependent, the dependent 
                // package should be a framework 2 package.
                if (PropertyUtil.IsPropertyEqual(Project, "package." + depName + ".frameworkversion", "1"))
                {
                    string depPkgNameWithVer = depName + "-" + Project.Properties["package." + depName + ".version"];
                    string depPkgManifest = Project.Properties["package." + depName + ".dir"] + Path.DirectorySeparatorChar + "Manifest.xml";
                    if (!File.Exists(depPkgManifest))
                    {
                        Log.WriteLine(Error.Format(Project, "SetupBuildDependencies", "WARNING", "The build dependent package ({0}) is missing Manifest.xml file.  If this is a FW1 pre-built package, please set it up as 'usedependencies' in your build script.", depPkgNameWithVer));
                    }
                    else
                    {
                        Log.WriteLine(Error.Format(Project, "SetupBuildDependencies", "WARNING", "The build dependent package ({0}) appears to be a FW1 package.  Please set it up as 'usedependencies' in your build script.", depPkgNameWithVer));
                    }
                }

                if (!(linkDepHash != null && linkDepHash.ContainsKey(depName)))
                {
                    AppendDependentIncludeDirs(depName);
                }

                if (BuildData.BuildType.IsCLR)
                {
                    // We probably shouldn't call the following task if we're currently building a solution file. 
                    if (BuildData.CopyLocal == BaseBuildData.CopyLocalType.True || BuildData.CopyLocal == BaseBuildData.CopyLocalType.Slim)
                    {
                        CopyLocalDependentAssemblies(depName);
                    }

                    FileSetUtil.FileSetAppend(BuildData.BuildFileSets["build.assemblies.all"],
                        FileSetUtil.GetFileSet(Project, "package." + depName + ".assemblies"));
                }
            }
        }

        private void AppendDependentIncludeDirs(string dependent)
        {
            string depIncludes = String.Empty;
            if (String.IsNullOrEmpty(BuildData.BuildType.SubSystem))
            {
                depIncludes = Project.Properties["package." + dependent + ".includedirs"];
            }
            else
            {
                depIncludes = Project.Properties["package." + dependent + ".includedirs" + BuildData.BuildType.SubSystem];
                if (depIncludes == null)
                {
                    depIncludes = Project.Properties["package." + dependent + ".includedirs"];
                }
            }
            if (!String.IsNullOrEmpty(depIncludes))
            {
                string newIncludeDirs = EA.Eaconfig.PropertyUtil.AddNonDuplicatePathToNormalizedPathListProp(BuildData.BuildProperties["build.includedirs.all"], depIncludes);
                if (!String.IsNullOrEmpty(newIncludeDirs))
                {
                    BuildData.BuildProperties["build.includedirs.all"] = newIncludeDirs;
                }
            }
        }

        private void AppendDependentLibraries(string dependent)
        {
            // bool isDllFramework1 = false;

            // If a package is built for DLL configs, and has Framework 1 dependent packages like EAThread, 
            // then translate pc-vc71 config-system to pcdll. This is because eaconfig maps pc-vc to pc-vc71, 
            // but doesn't map pc-vc-dll to pcdll. And it doesn't help if the package overrides config-system
            // in <framework1overrides> in its masterconfig. eaconfig expects that the dependent be built with
            // pc-vc71 configs in addition to pcdll-vc71, because it uses the pc-vc71 libs to construct the one
            // for pcdll.
            if (PropertyUtil.GetBooleanProperty(Project, "Dll"))
            {
                FileSet depLibs = FileSetUtil.GetFileSet(Project, "package." + dependent + ".libs");
                if (depLibs != null)
                {
                    if (Project.Properties["package." + dependent + ".frameworkversion"] == "1")
                    {
                        // isDllFramework1 = true;

                        FileSet libsNofailonmissing = new FileSet(depLibs);
                        libsNofailonmissing.FailOnMissingFile = false;
                        if (libsNofailonmissing.FileItems.Count > 0)
                        {
                            FileSet libsRenamed = new FileSet();
                            foreach (FileItem file in libsNofailonmissing.FileItems)
                            {
                                FileSetUtil.FileSetInclude(libsRenamed, file.FileName.Replace("pc-vc71", "pcdll"));
                            }
                            Project.NamedFileSets["package." + dependent + ".libs"] = libsRenamed;
                        }
                    }
                }

            }

            if (BuildData.BuildType.BaseName != "StdLibrary")
            {
                FileSet libsAll = BuildData.BuildFileSets["build.libs.all"];
                FileSet dllsAll = BuildData.BuildFileSets["build.dlls.all"];

                //FileSetUtil.FileSetAppend(libsAll,
                //      FileSetUtil.GetFileSet(Project, "package." + dependent + ".libs" + BuildData.BuildType.SubSystem));

                // If package exports per-module libraries - use them
                int perModuleLibCount = 0;
                foreach (string depModule in StringUtil.ToArray(Project.Properties[PropGroupName(dependent + "." + BuildGroup + ".buildmodules")]))
                {
                    perModuleLibCount += FileSetUtil.FileSetAppend(libsAll, FileSetUtil.GetFileSet(Project, "package." + dependent + "." + depModule + ".libs" + BuildData.BuildType.SubSystem));
                    perModuleLibCount += FileSetUtil.FileSetAppend(dllsAll, FileSetUtil.GetFileSet(Project, "package." + dependent + "." + depModule + ".dlls" + BuildData.BuildType.SubSystem));
                }

                // If there are no per-module libraries exported use package libs
                if (perModuleLibCount == 0)
                {
                    FileSetUtil.FileSetAppend(libsAll,
                        FileSetUtil.GetFileSet(Project, "package." + dependent + ".libs" + BuildData.BuildType.SubSystem));

                    FileSetUtil.FileSetAppend(dllsAll,
                        FileSetUtil.GetFileSet(Project, "package." + dependent + ".dlls" + BuildData.BuildType.SubSystem));

                }

                FileSetUtil.FileSetAppend(libsAll,
                      FileSetUtil.GetFileSet(Project, "package." + dependent + ".libs" + BuildData.BuildType.SubSystem + ".external"));


                FileSetUtil.FileSetAppend(dllsAll,
                      FileSetUtil.GetFileSet(Project, "package." + dependent + ".dlls" + BuildData.BuildType.SubSystem + ".external"));
            }
        }

        private void CopyUseDependentAssemblies(string dependent)
        {
            string outputDir = ComputeBuildOutput.GetOutputDir(Project, GroupName, BuildData.BuildType);

            FileSet fs = FileSetUtil.GetFileSet(Project, "package." + dependent + ".assemblies");
            if (fs != null)
            {
                foreach (FileItem fileItem in fs.FileItems)
                {
                    string fileName = Path.GetFileName(fileItem.FileName);

                    CopyTask task = new CopyTask();
                    task.Project = Project;
                    task.SourceFile = fileItem.FileName;
                    task.ToFile = Path.Combine(outputDir, fileName);

                    task.Execute();
                }
            }
        }

        private void CopyLocalDependentAssemblies(string dependent)
        {
            CopyTask task = new CopyTask();
            task.Project = Project;

            // All non-runtime groups always depend on runtime 
            string dependent_build_group = "runtime";
            task.CopyFileSet.BaseDirectory = Project.ExpandProperties("${package." + dependent + ".builddir}/${config}/bin${eaconfig." + dependent_build_group + ".outputfolder}");

            FileSetUtil.FileSetInclude(task.CopyFileSet, "*.dll*");
            FileSetUtil.FileSetInclude(task.CopyFileSet, "*.pdb*");
            FileSetUtil.FileSetInclude(task.CopyFileSet, "*.map*");
            FileSetUtil.FileSetInclude(task.CopyFileSet, "*.exe*");

            task.ToDirectory = ComputeBuildOutput.GetOutputDir(Project, GroupName, BuildData.BuildType);

            task.Execute();
        }

    }
}

