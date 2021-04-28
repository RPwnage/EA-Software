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

using EA.Eaconfig.Structured.ObjectModel;

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
        
        [NAnt.Core.Attributes.Description("")]
        [TaskAttribute("debug", Required = false)]
        public bool Debug
        {
            get { return _debug; }
            set { _debug = value;}
        } 
        private bool _debug = false;

        [NAnt.Core.Attributes.Description("")]       
        [TaskAttribute("comment", Required = false)]
        public string Comment
        {
            get { return _comment; }
            set { _comment = value; }
        } private string _comment;


        [NAnt.Core.Attributes.Description("")]       
        [TaskAttribute("name", Required = true)]
        public string ModuleName
        {
            get { return _module.Name; }
            set {
                _module.Name = value; 
                if(_basePartialName==null)
                    _basePartialName = value; 
            }
        } private string _basePartialName;

        [NAnt.Core.Attributes.Description("")]        
        [TaskAttribute("group", Required = false)]
        public string Group
        {
            get { return _group; }
            set { _group = value; }
        }private string _group;

        [NAnt.Core.Attributes.Description("")]        
        [TaskAttribute("frompartial", Required = false)]
        public virtual string PartialModuleName
        {
            get { return PartialModuleNameProp.Value; }
            set { PartialModuleNameProp.Value = value; }
        }

        [NAnt.Core.Attributes.Description("")]        
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

        [NAnt.Core.Attributes.Description("")]        
        [Property("frompartial", Required = false)]
        public ConditionalPropertyElement PartialModuleNameProp
        {
            get { return _partialModuleNameProp; }
        }private ConditionalPropertyElement _partialModuleNameProp = new ConditionalPropertyElement();


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

        [NAnt.Core.Attributes.Description("")]        
        [Property("buildtype", Required = false)]
        public ConditionalPropertyElement BuildTypeProp
        {
            get { return _buildTypeProp; }
        }private ConditionalPropertyElement _buildTypeProp = new ConditionalPropertyElement();

        protected string PackageName
        {
            get { return Project.Properties["package.name"]; }
        }

        protected override void InitializeAttributes(XmlNode elementNode)
        {
            _group = BuildGroupBaseTask.GetGroupName(Project);

            base.InitializeAttributes(elementNode);
        }

        protected override void ExecuteTask()
        {
            base.ExecuteTask();

            if (!IsPartial)
            {
                if (Debug)
                {
                    Log.WriteLine();
                    if (!String.IsNullOrEmpty(Comment))
                    {
                        Log.WriteLine("{0}--- Module name=\"{1}\" comment=\"{2}\"", LogPrefix, ModuleName, Comment);
                    }
                    else
                    {
                        Log.WriteLine("{0}--- Module name=\"{1}\"", LogPrefix, ModuleName);
                    }
                    Log.WriteLine();
                }

                Package package = Package.GetPackage(Project);
                package.Modules.Add(_module.Name, _module);

                SetProperty(Group + ".buildmodules", ModuleName, true);

                SetBuildType();
            }

            ProcessPartialModules();

            SetupModule();

            if (!IsPartial)
            {
                PrintModule();
            }
        }

        protected bool IsPartial
        {
            get { return this is PartialModuleTask; }
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
                        Log.WriteLine(msg);
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

            _buildTypeInfo = ComputeBaseBuildData.GetBuldType(Project, finalBuildType);
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
                                    Log.WriteLine("{0}    Adding PartialModule name=\"{1}\" comment=\"{2}\"", LogPrefix, partialModule.ModuleName, partialModule.Comment);
                                }
                                else
                                {
                                    Log.WriteLine("{0}    Adding PartialModule name=\"{1}\"", LogPrefix, partialModule.ModuleName);
                                }
                                partialModule.Debug = true;
                            }

                            partialModule.ApplyPartialModule(this);
                            _hasPartial = true;
                        }
                    }
                }
                if (!_hasPartial)
                {
                    string msg = Error.Format(Project, Name, "WARNING", "No definition of partial module \"{1}\" found", LogPrefix, PartialModuleName);
                    Log.WriteLine(msg);
                }
                else
                {
                    Log.WriteLine();
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

        protected void SetModuleProperty(string name, string value)
        {
            SetModuleProperty(name, value, false);
        }

        protected void SetModuleProperty(string name, string value, bool append)
        {
            SetProperty(Group + "." + ModuleName + "." + name, value, append);
        }

        protected string GetModuleProperty(string name)
        {
            return Project.Properties[Group + "." + ModuleName + "." + name];
        }

        protected string SetModuleFileset(string name, StructuredFileSet value)
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

        protected void SetModuleOptionSet(string name, StructuredOptionSet value)
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
                if (fileSet != null)
                {
                    fileSet.FailOnMissingFile = false;
                    fileSet.Append(fromfileSet);
                }
                else
                {
                    fileSet = new StructuredFileSet(fromfileSet);
                }
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
            }

            if (!IsPartial)
            {
                // If multiple postbuildtargets are defined - merge them together:
                IList<string> targetNames = StringUtil.ToArray(GetModuleProperty(propertyName));

                if (targetNames.Count > 1)
                {
                    string targetSetName = Group + "." + ModuleName + "." + propertyName + "set";
                    DynamicTarget targetSet = new DynamicTarget();
                    targetSet.BaseDirectory = Project.BaseDirectory;
                    targetSet.Name = targetSetName;
                    targetSet.Hidden = true;
                    targetSet.Project = Project;
                    targetSet.DependencyList = StringUtil.ArrayToString(targetNames, " ");

                    XmlDocument xmldoc = new InMemoryLineInfoXmlDoc(target.Location);
                    XmlElement targetEl = xmldoc.CreateElement("target");
                    targetEl.SetAttribute("name", targetSetName);

                    targetSet.SetXmlNode(targetEl);

                    Project.Targets.Add(targetSet);

                    SetModuleProperty(propertyName, targetSetName, false);
                }
            }
        }

        protected void SetModuleDependencies(string dependencytype, string value)
        {
            if (!String.IsNullOrEmpty(value))
            {
                foreach (string dep in StringUtil.ToArray(value))
                {
                    IList<string> depDetails = StringUtil.ToArray(dep, "/");
                    switch (depDetails.Count)
                    {
                        case 1: // Package name
                            SetModuleProperty(dependencytype, depDetails[0], true);
                            break;

                        case 2: // Package name + module name

                            if (depDetails[0] == PackageName) // Moduledependencies:
                            {
                                SetModuleProperty("runtime.moduledependencies", depDetails[1], true);
                            }
                            else //Constraining dependencies
                            {
                                SetModuleProperty(dependencytype, depDetails[0], true);
                                SetModuleProperty(depDetails[0] + "runtime.buildmodules", depDetails[1], true);

                                if (depDetails[0] == ModuleName)
                                {
                                    Log.WriteLine(Error.Format(Project, Name, "WARNING", "Module dependency format is 'PackageName/group/ModuleName', is appears that first entry '{0}' in '{1}' is a module name, not a package name", depDetails[0], dep));
                                }
                            }
                            break;

                        case 3: //Package name + group name + module name;

                            if (depDetails[0] == PackageName) // Moduledependencies:
                            {
                                SetModuleProperty(depDetails[1] + ".moduledependencies", depDetails[2], true);
                            }
                            else //Constraining dependencies
                            {
                                SetModuleProperty(dependencytype, depDetails[0], true);
                                SetModuleProperty(depDetails[0] + "." + depDetails[1] + ".buildmodules", depDetails[2], true);

                                if (depDetails[0] == ModuleName)
                                {
                                    Log.WriteLine(Error.Format(Project, Name, "WARNING", "Module dependency format is 'PackageName/group/ModuleName', is appears that first entry '{0}' in '{1}' is a module name, not a package name", depDetails[0], dep));
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
                Log.WriteLine();
                Log.WriteLine();
                Log.Write(sb.ToString());
                Log.WriteLine();
                Log.WriteLine("{0}--- End Module \"{1}\"", LogPrefix, ModuleName);
                Log.WriteLine();
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

                Log.WriteLine("{0}", sb.ToString());
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
                foreach (DictionaryEntry entry in set.Options)
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
                Log.WriteLine();
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

                Log.WriteLine(sb.ToString());
            }
        }

        protected abstract void SetupModule();

        Module _module = new Module();

        private static char[] _delimArray = { 
            '\x000a', // newline
            '\x000d', // carriage return
        };

    }

}
