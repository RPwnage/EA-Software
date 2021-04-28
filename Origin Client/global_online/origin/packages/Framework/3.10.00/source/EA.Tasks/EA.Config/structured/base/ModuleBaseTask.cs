using NAnt.Core;
using NAnt.Core.Attributes;
using NAnt.Core.Tasks;
using NAnt.Core.Logging;
using EA.FrameworkTasks;

using System;
using System.Collections;
using System.Collections.Generic;
using System.Text;
using System.Xml;
using System.Linq;

using EA.Eaconfig.Modules;
using EA.Eaconfig.Build;

namespace EA.Eaconfig.Structured
{
    abstract public class ModuleBaseTask : ConditionalTaskContainer
    {
        private string _basebuildtype;

        internal class DataEntry
        {
            internal string Name;
            internal object Data;

            internal static void Add(List<DataEntry> dataList, string name, object data)
            {
                if (data != null)
                {
                    foreach (DataEntry entry in dataList)
                    {
                        if (entry.Name == name && entry.Data.GetType() == data.GetType())
                        {
                            entry.Data = data;
                            return;
                        }
                    }
                    DataEntry newEntry = new DataEntry();
                    newEntry.Name = name;
                    newEntry.Data = data;

                    dataList.Add(newEntry);
                }
            }
        }

        internal List<DataEntry> debugData = new List<DataEntry>();

        protected ModuleBaseTask(string buildtype)
        {
            _group = "runtime";
            _basebuildtype = buildtype;
        }
        
        /// <summary></summary>
        [TaskAttribute("debug", Required = false)]
        public bool Debug
        {
            get { return _debug; }
            set { _debug = value;}
        } 
        private bool _debug = false;

        /// <summary></summary>       
        [TaskAttribute("comment", Required = false)]
        public string Comment
        {
            get { return _comment; }
            set { _comment = value; }
        } private string _comment;

        /// <summary>The name of this module.</summary>       
        [TaskAttribute("name", Required = true)]
        public virtual string ModuleName
        {
            get { return _name; }
            set {
                _name = value; 
                if(_basePartialName==null)
                    _basePartialName = value; 
            }
        } private string _basePartialName;

        /// <summary>The name of the build group this module is a part of. The default is 'runtime'.</summary>        
        [TaskAttribute("group", Required = false)]
        public string Group
        {
            get { return _group; }
            set { _group = value; }
        }private string _group;

        /// <summary></summary>        
        [TaskAttribute("frompartial", Required = false)]
        public virtual string PartialModuleName
        {
            get { return PartialModuleNameProp.Value; }
            set { PartialModuleNameProp.Value = value; }
        }

        /// <summary>Used to explicitly state the build type. By default 
        /// structured xml determines the build type from the structured xml tag.
        /// This field is used to override that value.</summary>        
        [TaskAttribute("buildtype", Required = false)]
        public string BuildType
        {
            get 
            {
                string explicitType = ExplicitBuildType;

                if (!String.IsNullOrEmpty(explicitType))
                {
                    return explicitType;
                }
                return _basebuildtype;
            }
            set { _buildtype = value; }

        }private string _buildtype;

        /// <summary></summary>        
        [Property("frompartial", Required = false)]
        public ConditionalPropertyElement PartialModuleNameProp
        {
            get { return _partialModuleNameProp; }
        }private ConditionalPropertyElement _partialModuleNameProp = new ConditionalPropertyElement();

        /// <summary></summary>
        [BuildElement("script", Required = false)]
        public NantScript NantScript
        {
            get { return _nantScript; }
        }private NantScript _nantScript = new NantScript();

        protected string ExplicitBuildType
        {
            get
            {
                if (!String.IsNullOrEmpty(_buildTypeProp.Value) && Project != null)
                {
                    _buildTypeProp.Value = Project.ExpandProperties(_buildTypeProp.Value);
                    _buildtype = StringUtil.Trim(_buildTypeProp.Value);
                    _buildTypeProp.Value = String.Empty;
                    return _buildtype;
                }
                else if (!String.IsNullOrEmpty(_buildtype))
                {
                    if (!String.IsNullOrEmpty(_buildTypeProp.Value) && (StringUtil.Trim(_buildTypeProp.Value) != _buildtype))
                    {
                        Error.Throw(Project, Location, ModuleName, "Defining buildtype twice is not allowed: bothth '{0}' and '{1}' were defined", _buildtype, StringUtil.Trim(_buildTypeProp.Value));
                    }
                    return _buildtype;
                }
                return String.Empty;
            }
        }

        /// <summary></summary>        
        [Property("buildtype", Required = false)]
        public BuildTypePropertyElement BuildTypeProp
        {
            get { return _buildTypeProp; }
        }private BuildTypePropertyElement _buildTypeProp = new BuildTypePropertyElement();

        protected string PackageName
        {
            get { return Project.Properties["package.name"]; }
        }

        /// <summary>Sets the dependencies for a project</summary>
        [BuildElement("dependencies", Required = false)]
        public DependenciesPropertyElement Dependencies
        {
            get { return _dependencies; }
        }private DependenciesPropertyElement _dependencies = new DependenciesPropertyElement();

        protected override void InitializeAttributes(XmlNode elementNode)
        {
           // Package package = Package.GetPackage(Project, Project.Properties["package.name"]);

            _group = BuildGroupBaseTask.GetGroupName(Project);

            var initializer = NAnt.Core.Reflection.ElementInitHelper.GetElementInitializer(this);

            long foundRequiredAttributes = 0;
            long foundUnknownAttributes = 0;
            long foundRequiredElements = 0;

            for (int i = 0; i < elementNode.Attributes.Count; i++)
            {
                XmlAttribute attr = elementNode.Attributes[i];

                initializer.InitAttribute(this, attr.Name, attr.Value, ref foundRequiredAttributes, ref foundUnknownAttributes);
            }

            initializer.CheckRequiredAttributes(elementNode, foundRequiredAttributes);
            initializer.CheckUnknownAttributes(elementNode, foundUnknownAttributes);


            NantScript.InitializeFromCode(Project, elementNode);

            if (!IsPartial)
            {
                NantScript.Execute(NantScript.Order.before);

                // Execute partial before build steps:

                //Add property to use in partial modules processing
                Project.Properties.AddLocal("module.name", ModuleName);

                foreach (string partialBaseModule in StringUtil.ToArray(PartialModuleName))
                {
                    foreach (PartialModuleTask partialModule in PartialModuleTask.GetPatialModules(Project))
                    {
                        if (partialModule.ModuleName == partialBaseModule)
                        {
                            var partialScript = new NantScript();
                            partialScript.InitializeFromCode(Project, partialModule.Code);
                            partialScript.Execute(NantScript.Order.before);
                        }
                    }
                }
            }

            if (initializer.HasNestedElements)
            {
                foreach (XmlNode childNode in elementNode.ChildNodes)
                {
                    initializer.InitNestedElement(this, childNode, true, ref foundRequiredElements);
                }

                initializer.CheckRequiredNestedElements(elementNode, foundRequiredElements);
            }

            //base.InitializeAttributes(elementNode);
        }

        protected override void ExecuteTask()
        {
            base.ExecuteTask();

            InitModule();

            //Add property to use in scripts
            Project.Properties.AddLocal("module.name", ModuleName);

            if (!IsPartial)
            {
                if (Debug)
                {
                    Log.Status.WriteLine();
                    if (!String.IsNullOrEmpty(Comment))
                    {
                        Log.Status.WriteLine("{0}--- Module name=\"{1}\" comment=\"{2}\"", LogPrefix, ModuleName, Comment);
                    }
                    else
                    {
                        Log.Status.WriteLine("{0}--- Module name=\"{1}\"", LogPrefix, ModuleName);
                    }
                    Log.Status.WriteLine();
                }

                SetProperty(Group + ".buildmodules", ModuleName, true);

                SetBuildType();
            }

            ProcessPartialModules();

            SetModuleProperty("scriptfile", Project.CurrentScriptFile, append: false);

            SetupModule();

            FinalizeModule();

            NantScript.Execute(NantScript.Order.after);

            if (!IsPartial)
            {
                PrintModule();
            }
            Project.Properties.Remove("module.name");
        }


        protected bool IsPartial
        {
            get { return this is PartialModuleTask.PartialModule || this is PartialModuleTask.DotNetPartialModule; }
        }

        protected bool HasPartial
        {
            get { return _hasPartial; }
        } private bool _hasPartial = false;


        protected virtual void SetBuildType()
        {
            string existingBuildType =  GetModuleProperty("buildtype");

            string finalBuildType = String.Empty;

            if (!String.IsNullOrEmpty(existingBuildType))
            {
                string explicitBuildType = ExplicitBuildType;

                if (!String.IsNullOrEmpty(explicitBuildType))
                {
                    if (explicitBuildType != existingBuildType)
                    {
                        string msg = Error.Format(Project, ModuleName, "WARNING", "Conflicting build types defined '{0}' and '{1}'. Will use buildtype='{1}'", existingBuildType, explicitBuildType);
                        Log.Warning.WriteLine(msg);
                    }
                    finalBuildType = explicitBuildType;
                }
                else
                {
                    finalBuildType = existingBuildType;
                }
            }
            else
            {
                finalBuildType = BuildType;
            }
            SetModuleProperty("buildtype", finalBuildType);

            _buildTypeInfo = Project.CreateBuildType(Group + "." + ModuleName);
        }

        protected BuildType BuildTypeInfo
        {
            get 
            {
                if (_buildTypeInfo == null)
                {
                    Error.Throw(Project, Location, Name, "INTERNAL ERROR: BuildTypeInfo only available after buildtype property is set.");
                }
                return _buildTypeInfo; 

            } 
        } private BuildType _buildTypeInfo;


        protected void ProcessPartialModules()
        {
            if (!String.IsNullOrEmpty(PartialModuleName))
            {
                //Add property to use in partial modules processing
                Project.Properties.AddLocal("module.name", ModuleName);

                foreach (string partialBaseModule in StringUtil.ToArray(PartialModuleName))
                {
                    foreach (PartialModuleTask partialModule in PartialModuleTask.GetPatialModules(Project))
                    {
                        if (partialModule.ModuleName == partialBaseModule)
                        {
                            if (Debug)
                            {
                                if (!String.IsNullOrEmpty(partialModule.Comment))
                                {
                                    Log.Status.WriteLine("{0}    Applying PartialModule name=\"{1}\" comment=\"{2}\"", LogPrefix, partialModule.ModuleName, partialModule.Comment);
                                }
                                else
                                {
                                    Log.Status.WriteLine("{0}    Applying PartialModule name=\"{1}\"", LogPrefix, partialModule.ModuleName);
                                }
                                
                            }

                            partialModule.ApplyPartialModule(this);
                            _hasPartial = true;
                        }
                    }
                }
                if (!_hasPartial)
                {
                    string msg = Error.Format(Project, Name, "WARNING", "No definition of partial module \"{1}\" found", LogPrefix, PartialModuleName);
                    Log.Warning.WriteLine(msg);
                }
            }
        }


        protected void SetModuleProperty(string name, ConditionalPropertyElement value)
        {
            if (value != null)
            {
                SetModuleProperty(name, value, value.Append);
            }
        }

        protected void SetModuleProperty(string name, PropertyElement value, bool append)
        {
            if (value != null)
            {
                SetModuleProperty(name, value.Value, append);
            }
        }

        protected void SetProperty(string name, string value, bool append)
        {
            if (value != null)
            {
                value = Project.ExpandProperties(value);

                if (append)
                {
                    string oldVal = Project.Properties[name];
                    if (!String.IsNullOrEmpty(oldVal))
                    {
                        value = oldVal + Environment.NewLine + value;
                    }
                }
                Project.Properties[name] = value;

                if(Debug)
                {
                    DataEntry.Add(debugData, name, value);
                }
            }
        }

        public void SetModuleProperty(string name, string value)
        {
            SetModuleProperty(name, value, false);
        }

        public void SetModuleProperty(string name, string value, bool append)
        {
            SetProperty(Group + "." + ModuleName + "." + name, value, append);
        }

        public string GetModuleProperty(string name)
        {
            return Project.Properties[Group + "." + ModuleName + "." + name];
        }

        public FileSet GetModuleFileset(string name)
        {
            string fullName = Group + "." + ModuleName + "." + name;

            return Project.NamedFileSets.GetOrAdd(fullName, new FileSet());
        }

        public string SetModuleFileset(string name, StructuredFileSet value)
        {
            string fullName = Group + "." + ModuleName + "." + name;

            if (value != null && !String.IsNullOrEmpty(value.Suffix))
            {
                fullName += "." + value.Suffix;
            }

            if (HasPartial || IsPartial)
            {
                value = AppendPartialFileSet(value, fullName);
            }

            if (value != null)
            {
                if (value.Includes.Count > 0 || value.Excludes.Count > 0)
                {
                    Project.NamedFileSets[fullName] = value;

                    if (Debug)
                    {
                        DataEntry.Add(debugData, fullName, value);
                    }
                }
            }
            return fullName;
        }

        public void SetModuleOptionSet(string name, StructuredOptionSet value)
        {
            string fullName = Group + "." + ModuleName + "." + name;
            if (HasPartial || IsPartial)
            {
                value = AppendPartialOptionSet(value, fullName);
            }

            if (value != null)
            {
                if(value.Options.Count > 0)
                {
                    Project.NamedOptionSets[fullName] = value;
                }

                if (Debug)
                {
                    DataEntry.Add(debugData, fullName, value);
                }
            }
        }

        private StructuredFileSet AppendPartialFileSet(StructuredFileSet fileSet, string fromfileSetName)
        {
            if (fileSet != null && !fileSet.AppendBase)
            {
                return fileSet;
            }

            FileSet fromfileSet = FileSetUtil.GetFileSet(Project, fromfileSetName);
            if (fromfileSet != null)
            {
                var combined = new StructuredFileSet();
                combined.FailOnMissingFile = false;
                combined.Append(fromfileSet);
                combined.BaseDirectory = fromfileSet.BaseDirectory;

                if (fileSet != null)
                {
                    // Prepend items with custom optionsets:
                    combined.Includes.PrependRange(fileSet.Includes.Where(fi=>!String.IsNullOrEmpty(fi.OptionSet)));
                    combined.Includes.AddRange(fileSet.Includes.Where(fi=>String.IsNullOrEmpty(fi.OptionSet)));
                    combined.Excludes.AddRange(fileSet.Excludes);
                    combined.BaseDirectory = fileSet.BaseDirectory;
                }
                return combined;
            }
            return fileSet;
        }

        private StructuredOptionSet AppendPartialOptionSet(StructuredOptionSet optionSet, string fromoptionSetName)
        {
            if (optionSet != null && !optionSet.AppendBase)
            {
                return optionSet;
            }

            OptionSet fromoptionSet = OptionSetUtil.GetOptionSet(Project, fromoptionSetName);
            if (fromoptionSet != null)
            {
                if (fromoptionSet != null)
                {
                    optionSet.Append(fromoptionSet);
                }
            }
            return optionSet;
        }

        protected string GetValueOrDefault(string value, string defaultVal)
        {
            if (String.IsNullOrEmpty(value))
            {
                value = defaultVal;
            }
            return value;
        }

        protected void SetModuleTarget(BuildTargetElement target, string propertyName)
        {
            if ((target.Target.HasTargetElement || !String.IsNullOrEmpty(target.TargetName)))
            {
                string targetName = target.TargetName;

                if (target.Target.HasTargetElement)
                {
                    targetName = Group + "." + _basePartialName + "." + target.Target.TargetName;
                    target.Target.Execute(targetName);

                    target.TargetName = target.Target.TargetName;
                }

                SetModuleProperty(propertyName, targetName, true);
                SetModuleProperty(propertyName + "." + targetName + ".nantbuildonly", target.NantBuildOnly.ToString().ToLowerInvariant(), true);
            }
        }
        
        protected void SetupDependencies()
        {
            // Use
            SetModuleDependencies(0, "interfacedependencies", Dependencies.UseDependencies.Interface_1_Link_0 + Environment.NewLine + Dependencies.InterfaceDependencies.Interface_1_Link_0);
            SetModuleDependencies(1, "usedependencies", Dependencies.UseDependencies.Interface_1_Link_1 + Environment.NewLine + Dependencies.UseDependencies.Interface_1_Link_1);            
            SetModuleDependencies(2, "uselinkdependencies", Dependencies.UseDependencies.Interface_0_Link_1 + Environment.NewLine + Dependencies.InterfaceDependencies.Interface_0_Link_1);
            // Build
            SetModuleDependencies(3, "builddependencies", Dependencies.BuildDependencies.Interface_1_Link_1 + Environment.NewLine + Dependencies.LinkDependencies.Interface_1_Link_1);
            SetModuleDependencies(4, "linkdependencies", Dependencies.BuildDependencies.Interface_0_Link_1 + Environment.NewLine + Dependencies.LinkDependencies.Interface_0_Link_1);
            SetModuleDependencies(5, "buildinterfacedependencies", Dependencies.BuildDependencies.Interface_1_Link_0 + Environment.NewLine + Dependencies.LinkDependencies.Interface_1_Link_0);
            SetModuleDependencies(6, "buildonlydependencies", Dependencies.BuildDependencies.Interface_0_Link_0 + Environment.NewLine + Dependencies.LinkDependencies.Interface_0_Link_0);
            // Auto Build/Use
            SetModuleDependencies(7, "autodependencies", Dependencies.Dependencies.Interface_1_Link_1);
            SetModuleDependencies(8, "autointerfacedependencies", Dependencies.Dependencies.Interface_1_Link_0);
            SetModuleDependencies(9, "autolinkdependencies", Dependencies.Dependencies.Interface_0_Link_1);


            // Set Copy Local dependencies:

            var copylocalitems = String.Concat(
                Dependencies.BuildDependencies.CopyLocalValue, Environment.NewLine,
                Dependencies.UseDependencies.CopyLocalValue, Environment.NewLine,
                Dependencies.InterfaceDependencies.CopyLocalValue, Environment.NewLine,
                Dependencies.LinkDependencies.CopyLocalValue, Environment.NewLine,
                Dependencies.Dependencies.CopyLocalValue).ToArray().Distinct().ToString(Environment.NewLine);

            if (!String.IsNullOrEmpty(copylocalitems))
            {
                SetModuleProperty("copylocal.dependencies", copylocalitems);
            }

        }


        protected void SetModuleDependencies(int mappingInd, string dependencytype, string value)
        {
            if (!String.IsNullOrEmpty(value))
            {
                var processedpackages = new HashSet<string>(); // To avoid duplicate entries

                var moduleDependenciesName = Mappings.ModuleDependencyPropertyMapping[mappingInd].Key;
                var moduleConstraintsName = Mappings.ModuleDependencyConstraintsPropertyMapping[mappingInd].Key;

                foreach (string dep in StringUtil.ToArray(value))
                {
                    IList<string> depDetails = StringUtil.ToArray(dep, "/");
                    switch (depDetails.Count)
                    {
                        case 1: // Package name

                            if(!processedpackages.Contains(depDetails[0]))
                            {
                                processedpackages.Add(depDetails[0]);
                                SetModuleProperty(dependencytype, depDetails[0], true);
                            }
                            break;

                        case 2: // Package name + module name

                            if (depDetails[0] == PackageName) // Moduledependencies:
                            {
                                    SetModuleProperty("runtime" + moduleDependenciesName, depDetails[1], true);
                            }
                            else //Constraining dependencies
                            {
                                if (!processedpackages.Contains(depDetails[0]))
                                {
                                    processedpackages.Add(depDetails[0]);
                                    SetModuleProperty(dependencytype, depDetails[0], true);
                                }
                                SetModuleProperty(depDetails[0] + ".runtime" + moduleConstraintsName, depDetails[1], true);

                                if (depDetails[0] == ModuleName)
                                {
                                    Log.Warning.WriteLine(Error.Format(Project, Name, "WARNING", "Module dependency format is 'PackageName/group/ModuleName', is appears that first entry '{0}' in '{1}' is a module name, not a package name", depDetails[0], dep));
                                }
                            }
                            break;

                        case 3: //Package name + group name + module name;

                            if (depDetails[0] == PackageName) // Moduledependencies:
                            {
                                    SetModuleProperty(depDetails[1] + moduleDependenciesName, depDetails[2], true);
                            }
                            else //Constraining dependencies
                            {
                                if (!processedpackages.Contains(depDetails[0]))
                                {
                                    processedpackages.Add(depDetails[0]);
                                    SetModuleProperty(dependencytype, depDetails[0], true);
                                }
                                SetModuleProperty(depDetails[0] + "." + depDetails[1] + moduleConstraintsName, depDetails[2], true);

                                if (depDetails[0] == ModuleName)
                                {
                                    Log.Warning.WriteLine(Error.Format(Project, Name, "WARNING", "Module dependency format is 'PackageName/group/ModuleName', is appears that first entry '{0}' in '{1}' is a module name, not a package name", depDetails[0], dep));
                                }
                            }
                            break;
                        default:
                            Error.Throw(Project, Location, Name, "Ivalid dependency description: {0}, valid values are 'package_name' or 'package_name/module_name' or 'package_name/group/module_name', where group is one of 'runtime, test, example, tool'", dep);
                            break;
                    }
                }
            }
        }

        protected override void InitializeParentTask(System.Xml.XmlNode taskNode, Element parent)
        {
            base.InitializeParentTask(taskNode, null);
        }

        protected void PrintModule()
        {
            if (Debug)
            {
                foreach (DataEntry key in debugData)
                {
                    if (key.Data is string)
                    {
                        PrintProperty(key.Name, key.Data as string);
                    }
                    else if (key.Data is FileSet)
                    {
                        PrintFileset(key.Name, key.Data as FileSet);
                    }
                    else if (key.Data is OptionSet)
                    {
                        PrintOptionset(key.Name, key.Data as OptionSet);
                    }
                }

                StringBuilder sb = new StringBuilder("     --- Build Options ---");
                sb.AppendLine();
                sb.AppendLine();
                PrintOptionset(sb, GetModuleProperty("buildtype"));
                Log.Status.WriteLine();
                Log.Status.WriteLine();
                Log.Status.WriteLine(sb.ToString());
                Log.Status.WriteLine("{0}--- End Module \"{1}\"", LogPrefix, ModuleName);
                Log.Status.WriteLine();
            }
        }

        protected void PrintProperty(string name, string val)
        {
            if (Debug)
            {
                StringBuilder sb = new StringBuilder("     <property name=\""+name+"\"");

                IList<string> values = StringUtil.ToArray(val, _delimArray);

                if (values.Count <= 1)
                {
                    sb.AppendFormat(" value=\"{0}\"", val);
                    sb.Append("/>");
                }
                else
                {
                    sb.AppendLine(">");
                    foreach (string v in values)
                    {
                        sb.AppendLine("          "+v);
                    }
                    sb.Append("     </property>");
                }

                Log.Status.WriteLine("{0}", sb.ToString());
            }
        }

        protected void PrintOptionset(string name, OptionSet optionset)
        {
            StringBuilder sb = new StringBuilder();
            PrintOptionset(sb, name);
            Console.Write(sb.ToString());
        }

        protected void PrintOptionset(StringBuilder sb,  string name)
        {
            OptionSet set = OptionSetUtil.GetOptionSet(Project, name);

            sb.Append("     <optionset name=\"" + name + "\"");
            if (set != null)
            {
                sb.AppendLine(">");
                foreach (KeyValuePair<string, string> entry in set.Options)
                {
                    if (!String.IsNullOrEmpty(entry.Value as string))
                    {
                        IList<string> values = StringUtil.ToArray(entry.Value as string, _delimArray);

                        sb.Append("       <option name=\"" + entry.Key as string + "\"");

                        if (values.Count <= 1)
                        {
                            sb.AppendFormat(" value=\"{0}\"", entry.Value as string);
                            sb.AppendLine("/>");
                        }
                        else
                        {
                            sb.AppendLine(">");
                            foreach (string v in values)
                            {
                                sb.AppendLine("           " + v);
                            }
                            sb.AppendLine("       </option>");
                        }
                    }
                }
            }
            else
            {
                sb.AppendLine(" />");
            }
        }

        protected void PrintFileset(string name, FileSet fileset)
        {
            if (Debug)
            {
                Log.Status.WriteLine();
                StringBuilder sb = new StringBuilder("     <fileset name=\"" + name + "\"");

                if (!String.IsNullOrEmpty(fileset.FromFileSetName))
                {
                    sb.AppendFormat(" fromfileset=\"{0}\"", fileset.FromFileSetName);
                }

                if(!String.IsNullOrEmpty(fileset.BaseDirectory))
                {
                    sb.AppendFormat(" basedir=\"{0}\"", fileset.BaseDirectory);
                }
                sb.AppendLine(">");

                foreach (FileSetItem item in fileset.Includes)
                {
                    sb.Append("        <includes ");
                    if (!String.IsNullOrEmpty(item.Pattern.Value))
                    {
                        sb.AppendFormat(" name=\"{0}\"", item.Pattern.Value);
                    }
                    if (item.AsIs)
                    {
                        sb.Append(" asis=\"true\"");
                    }
                    if (!item.Pattern.FailOnMissingFile)
                    {
                        sb.Append(" failonmissing=\"false\"");
                    }
                    if (item.Pattern.BaseDirectory != fileset.BaseDirectory)
                    {
                        sb.AppendFormat(" basedir=\"{0}\"", item.Pattern.BaseDirectory);
                    }

                    if (String.IsNullOrEmpty(item.OptionSet))
                    {
                        sb.AppendLine("/>");
                    }
                    else
                    {
                        sb.AppendLine(">");
                        //sb.AppendLine("         OptionSet=\""+item.OptionSet+"\"");
                        PrintOptionset(sb, item.OptionSet);
                        sb.AppendLine("     </includes>");
                    }
                }
                foreach (FileSetItem item in fileset.Excludes)
                {
                    sb.Append("     <includes ");
                    if (!String.IsNullOrEmpty(item.Pattern.Value))
                    {
                        sb.AppendFormat(" name=\"{0}\"", item.Pattern.Value);
                    }
                    if (item.AsIs)
                    {
                        sb.Append(" asis=\"true\"");
                    }
                    if (!item.Pattern.FailOnMissingFile)
                    {
                        sb.Append(" failonmissing=\"false\"");
                    }
                    if (item.Pattern.BaseDirectory != fileset.BaseDirectory)
                    {
                        sb.AppendFormat(" basedir=\"{0}\"", item.Pattern.BaseDirectory);
                    }

                    if (String.IsNullOrEmpty(item.OptionSet))
                    {
                        sb.AppendLine("/>");
                    }
                    else
                    {
                        sb.AppendLine(">");
                        sb.AppendLine("         OptionSEt=\"" + item.OptionSet + "\"");
                        sb.AppendLine("     </includes>");
                    }
                }
                sb.Append("     </fileset>");

                Log.Status.WriteLine(sb.ToString());
            }
        }

        protected abstract void SetupModule();

        protected abstract void InitModule();

        protected abstract void FinalizeModule();

        string _name;

        private static char[] _delimArray = { 
            '\x000a', // newline
            '\x000d', // carriage return
        };
    }

    
    public class NantScript : Element
    {
        public enum Order { before, after };

        private List<ScriptEntry> _scripts = new List<ScriptEntry>();

        private bool _initialized = false;

        class ScriptEntry
        {
            internal Order Order;
            internal XmlNode XmlNode;
        }

        /// <summary>The directory to create.</summary>
        [TaskAttribute("order", Required = true)]
        public Order ExecutionOrder 
        { 
            get 
            { 
                return _order; 
            } 
            set 
            {
                _order = value; 
            } 
        }
        private Order _order = Order.before;


        /// <summary>Add all the child option elements.</summary>
        /// <param name="elementNode">Xml node to initialize from.</param>
        protected override void InitializeElement(XmlNode elementNode)
        {
            if (!_initialized)
            {
                var script = new ScriptEntry();
                script.Order = ExecutionOrder;
                script.XmlNode = elementNode;

                _scripts.Add(script);

                _order = Order.before;
            }
        }

        public void InitializeFromCode(Project project, XmlNode elementNode)
        {
            Project = project;

            foreach (XmlNode childNode in elementNode)
            {
                if (childNode.NodeType == XmlNodeType.Element && childNode.NamespaceURI.Equals(elementNode.NamespaceURI))
                {
                    InitializeCodeElement(project, childNode);
                }
            }
            _initialized = true;
        }

        private void InitializeCodeElement(Project project, XmlNode elementNode)
        {
            if (elementNode.NodeType != XmlNodeType.Comment)
            {
                if (elementNode.Name == "do")
                {
                    var task = new SectionTask();

                    task.Project = Project;
                    task.Parent = null;
                    task.Initialize(elementNode);

                    if (task.IfDefined && !task.UnlessDefined)
                    {
                        foreach (XmlNode childNode in elementNode)
                        {
                            if (childNode.NodeType == XmlNodeType.Element && childNode.NamespaceURI.Equals(elementNode.NamespaceURI))
                            {
                                InitializeCodeElement(project, childNode);
                            }
                        }
                    }
                }
                else if (elementNode.Name == "script")
                {
                    Initialize(elementNode);
                }
            }
        }


        public void Execute(Order order)
        {
            foreach (var script in _scripts)
            {
                if (script.Order == order)
                {
                    var task = new ExecScript();
                    task.Project = Project;
                    task.Initialize(script.XmlNode);

                    task.Execute();
                }
            }
        }

        class ExecScript : TaskContainer
        {
            [TaskAttribute("order", Required = false)]
            public Order Dummy { set {} }

        }

    }


}
