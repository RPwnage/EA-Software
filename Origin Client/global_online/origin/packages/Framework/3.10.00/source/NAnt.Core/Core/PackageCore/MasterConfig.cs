/*
<project>
    <masterversions>
        <grouptype name="prebuilt" autobuildclean="false">
            <package name="tlconfig" version="1.00.09"/>
            <package name="Apt"      version="1.00.00"/>
        </grouptype>

        <grouptype name="buildable">
            <package name="PackageA" version="1.01.00" autobuildtarget="build-me" autocleantarget="clean-me"/>
            <package name="PackageB" version="1.00.01" autobuildclean="false"/>

            <package name="eacore" version="1.00.01">
                <exception propertyname="config-platform">
                    <condition value="xenon" version="1.02.00"/>
                </exception>
                <properties>noDirectX useRenderWare</properties>
            </package>
        </grouptype>

        <grouptype name="live">
            <!-- this exception is evaluated only once, when masterconfig.xml is parsed -->
            <exception propertyname="GAME.PRODUCT">
               <condition value="aitestbed" name="live-ai"/>
            </exception>

            <package name="aififa" version="dev"/>
    </masterversions>

    <packageroots>
        <packageroot>c:\packages </packageroot>
        <packageroot>${nant.location}\..\</packageroot>
    </packageroots>

    <config package="tlconfig" default="pc-vc-devdebug" excludes="gc-sn-devdebug gc-sn-devfast"/>
    <!-- Or
    <config package="tlconfig" default="pc-vc-devdebug" includes="pc-vc-dll*"/>
    -->
    <ondemand>true</ondemand>
    <buildroot>build</buildroot>
</project>
*/

using System;
using System.Xml;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;

using NAnt.Core;
using NAnt.Core.Util;
using NAnt.Core.Logging;


namespace NAnt.Core.PackageCore
{
    public class MasterConfig
    {
        public static readonly MasterConfig UndefinedMasterconfig = new MasterConfig();

        internal MasterConfig(string file = "")
        {
            MasterconfigFile = file;
            _packageRoots = new List<PackageRoot>();
            _buildRoot = new BuildRootInfo();
            _framework1Overrides = new List<Fw1Override>();
            _fragments = new List<MasterConfig>();
            if (String.IsNullOrEmpty(file))
            {
                _config = new ConfigInfo(null, null, String.Empty, String.Empty);
            }
        }

        public readonly string MasterconfigFile;

        public bool OnDemand
        {
            get {   return _onDemand == null? false : (bool)_onDemand; }
        }
        private bool? _onDemand;

        public string OnDemandRoot
        {
            get { return _onDemandRoot; }
        }
        private string _onDemandRoot;


        public BuildRootInfo BuildRoot
        {
            get { return _buildRoot; }

        }
        private BuildRootInfo _buildRoot;

        public List<GlobalProperty> GlobalProperties
        {
            get { return _globalProperties; }

        }
        private List<GlobalProperty> _globalProperties;

        public readonly List<GroupType> PackageGroups = new List<GroupType>();

        public readonly Dictionary<string, Package> Packages = new Dictionary<string, Package>();

        public List<PackageRoot> PackageRoots
        {
            get { return _packageRoots; }
        }
        private List<PackageRoot> _packageRoots;

        public ConfigInfo Config
        {
            get { return _config; }
        }
        private ConfigInfo _config;


        public List<Fw1Override> Framework1Overrides
        {
            get { return _framework1Overrides; }
        }
        private List<Fw1Override> _framework1Overrides;

        public List<MasterConfig> Fragments
        {
            get { return _fragments; }
        }
        private readonly List<MasterConfig> _fragments;


        public class ConfigInfo
        {
            internal ConfigInfo(string package, string defconfig, string includes, string excludes)
            {
                if (!String.IsNullOrEmpty(excludes) && !String.IsNullOrEmpty(includes))
                {
                    throw new BuildException("Error: <config> cannot have excludes and includes both in masterconfig file");
                }
                Package = package;
                Default = defconfig;
                Excludes = StringUtil.ToArray(excludes);
                Includes = StringUtil.ToArray(includes);
            }

            public readonly string Package;
            public readonly string Default;
            public readonly IList<string> Excludes;
            public readonly IList<string> Includes;
        }

        public class BuildRootInfo
        {
            public string Path 
            { 
                get { return _buildroot; } 
                set { _buildroot = value; } 
            }
            public Exception Exception 
            { 
                get { return _exception; } 
                set { _exception = value; } 
            }
            private string _buildroot;
            private Exception _exception;
        }

        public class PackageRoot
        {
            internal PackageRoot(string path, int? minlevel = null, int? maxlevel = null)
            {
                _path = path;
                MinLevel = minlevel;
                MaxLevel = maxlevel;
                if (minlevel < 0 || maxlevel < 0)
                {
                    throw new BuildException("Error: <packageroot> element in masterconfig file cannot have negative 'minlevel' or 'maxlevel' attribute values.");
                }
                if (minlevel > maxlevel)
                {
                    throw new BuildException("Error: <packageroot> element in masterconfig file cannot have 'minlevel' greater than 'maxlevel' attribute value.");
                }
            }

            public string EvaluatePackageRoot(Project project)
            {
                string path = _path;
                if (project != null)
                {
                    string dir;
                    foreach (var excp in Exceptions)
                    {
                        if (excp.EvaluateException(project, out dir))
                        {
                            path = dir;
                            break;
                        }
                    }
                    if (path != null)
                    {
                        path = project.ExpandProperties(path);
                    }
                }
                return path;
            }


            public readonly List<Exception> Exceptions = new List<Exception>();
            private readonly string _path;
            public readonly int? MinLevel;
            public readonly int? MaxLevel;
        }

        public class GroupType
        {
            internal GroupType(List<GroupType> groupTypes, string name, bool? autobuildclean = null, IList<string> groupProperties = null, string outputMapOptionSet = null)
            {
                Name = name;
                AutoBuildClean = autobuildclean;
                GroupProperties = groupProperties ?? new List<string>();
                _groupTypes = groupTypes;
                OutputMapOptionSet = outputMapOptionSet;
            }

            public readonly string Name;

            public bool? AutoBuildClean;

            public IList<string> GroupProperties;

            public readonly string OutputMapOptionSet;

            public readonly List<Exception> Exceptions = new List<Exception>();

            public GroupType EvaluateGroupType(Project project)
            {
                return EvaluateGroupTypeImpl(project, new HashSet<string>());
            }

            private GroupType EvaluateGroupTypeImpl(Project project, ISet<string> stack)
            {
                GroupType found = null;
                string name = Name;

                if (Exceptions.Count > 0)
                {
                    Condition trueCondition;

                    name = EvaluateGroupTypeName(project, out trueCondition);

                    if (stack.Contains(name))
                    {
                        throw new BuildException(String.Format("Detected circular path evaluating masterconfig grouptype exceptions for '{0}'. Full stack: '{1}, {0}'.", name, stack.ToString(", ")));
                    }

                    List<MasterConfig.GroupType> foundGroups = _groupTypes.FindAll((item) => item.Name == name && item != this);

                    if (foundGroups.Count == 0)
                    {
                        if (name == Name && (trueCondition == null || (trueCondition.AutoBuildClean == null || AutoBuildClean == trueCondition.AutoBuildClean)))
                        {
                            // referencing itself;
                            found = this;
                        }
                        else
                        {
                            var autobuildclean = (trueCondition != null && trueCondition.AutoBuildClean != null) ? trueCondition.AutoBuildClean : AutoBuildClean;
                            // This exception referes to the same grouptype, just alters the name.
                            found = new GroupType(_groupTypes, name, autobuildclean, GroupProperties, OutputMapOptionSet);
                        }
                    }
                    else
                    {
                        foreach (MasterConfig.GroupType gt in foundGroups)
                        {
                            found = gt.EvaluateGroupType(project);
                            if (found != null)
                            {
                                break;
                            }
                        }
                    }

                    if(found == null)
                    {
                            throw new BuildException(String.Format("Failed to evaluate condition: Grouptype '{0}' <exception> did not eavaluate to any valid grouptype.", Name));
                    }
                }
                else
                {
                    found = this;
                }

                return found;

            }

            private string EvaluateGroupTypeName(Project project, out Condition trueCondition)
            {
                trueCondition = null;
                foreach (MasterConfig.Exception exp in Exceptions)
                {
                    if (exp.EvaluateException(project, out trueCondition))
                    {
                        return trueCondition.Result;
                    }
                }
                return Name;
            }




            private List<GroupType> _groupTypes;

        }

        public class Exception
        {
            internal Exception(string propertyName)
            {
                PropertyName = propertyName;
            }

            public readonly string PropertyName;

            public List<Condition> Conditions = new List<Condition>();

            public bool EvaluateException(Project project, out string result)
            {
                string propVal = project.Properties[PropertyName];
                if (propVal != null)
                {
                    foreach (Condition condition in Conditions)
                    {
                        if (Expression.Evaluate(condition.Value + "==" + propVal))
                        {
                            result = condition.Result;
                            return true;
                        }
                    }

                }
                result = null;
                return false;
            }

            public bool EvaluateException(Project project, out Condition trueCondition)
            {
                string propVal = project.Properties[PropertyName];
                if (propVal != null)
                {
                    foreach (Condition condition in Conditions)
                    {
                        if (Expression.Evaluate(condition.Value + "==" + propVal))
                        {
                            trueCondition = condition;
                            return true;
                        }
                    }

                }
                trueCondition = null;
                return false;
            }

        }

        public class Condition
        {
            internal Condition(string value, string result, bool? autobuildclean)
            {
                Value = value;
                Result = result;
                AutoBuildClean = autobuildclean;
            }

            public readonly string Value;

            public readonly string Result;

            public readonly bool? AutoBuildClean;
        }

        public class Package
        {
            internal Package(GroupType grouptype, string name, string version, bool? autobuildclean, string autoBuildTarget, string autoCleanTarget)
            {
                Name = name;
                _autoBuildClean = autobuildclean;
                AutoBuildTarget = autoBuildTarget;
                AutoCleanTarget = autoCleanTarget;
                _groupType = grouptype;
                _version = version;
            }

            public readonly string Name;

            public string AutoBuildTarget;

            public string AutoCleanTarget;

            public readonly List<Exception> Exceptions = new List<Exception>();

            public IList<string> Properties = new List<string>();

            public bool ContainsMasterVersion(string version)
            {
                if (_version == version)
                    return true;

                foreach (MasterConfig.Exception excp in Exceptions)
                {
                    foreach (Condition condition in excp.Conditions)
                    {
                        if (condition.Result == version)
                            return true;
                    }
                }
                return false;
            }

            public string EvaluateMasterVersion(Project project)
            {
                string masterversion;
                foreach (MasterConfig.Exception excp in Exceptions)
                {
                    if( excp.EvaluateException(project, out masterversion))
                    {
                        return masterversion;
                    }
                }
                return _version;
            }

            internal bool? UpdateAutobuildClean(bool? autobuildclean)
            {
                bool? oldvalue = _autoBuildClean;
                _autoBuildClean = autobuildclean;
                return oldvalue;
            }

            public bool? EvaluateAutobuildClean(Project project)
            {
                bool? autobuildclean = _autoBuildClean;

                if(autobuildclean == null)
                {
                    //check for exceptions
                    GroupType gt = _groupType.EvaluateGroupType(project);
                    autobuildclean = gt.AutoBuildClean;
                }
                return autobuildclean;
            }

            public GroupType EvaluateGrouptype(Project project)
            {
                return _groupType.EvaluateGroupType(project);
            }


            // Make following properties private, since in presence of conditions we need to do deferred evaluation
            private readonly string _version;
            private bool? _autoBuildClean;
            private readonly GroupType _groupType;
        }

        public class GlobalProperty
        {
            internal GlobalProperty(string name, string value, string condition)
            {
                Name = name;
                Value = value;
                Condition = condition;
            }
            public readonly string Name;
            public readonly string Value;
            public readonly string Condition;
        }

        public static MasterConfig Load(PathString filename, bool forceAutoBuild = false, string autobuildPackages = "", string autobuildGroups = "")
        {
            MasterConfigLoader loader = new MasterConfigLoader(forceAutoBuild, autobuildPackages, autobuildGroups);

                return loader.Load(filename.Path);
        }

        public void MergeFragments(Project project)
        {
            foreach (var fragment in Fragments)
            {
                //--- merge packages ---
                foreach (var pair in fragment.Packages)
                {
                    if (Packages.ContainsKey(pair.Key))
                    {
                        //IMTODO: Need to merge (name+exceptions) instead of evaluating exceptions right away
                        if (project != null && pair.Value.EvaluateMasterVersion(project) != Packages[pair.Key].EvaluateMasterVersion(project))
                        {
                            throw new BuildException(String.Format("Fragments contain definition of package '{0}' with different versions", pair.Value.Name));
                        }
                    }
                    else
                    {
                        Packages.Add(pair.Key, pair.Value);
                    }
                }
                //--- merge groups ---
                foreach (var group in fragment.PackageGroups)
                {
                    //IMTODO: Need to merge (name+exceptions) instead of relying on names
                    var existingGroup = PackageGroups.Find(g => g.Name == group.Name);
                    if(existingGroup == null)
                    {
                        PackageGroups.Add(group);
                    }
                    else
                    {
                        if (project!= null && existingGroup.EvaluateGroupType(project).Name != group.EvaluateGroupType(project).Name)
                        {
                            throw new BuildException(String.Format("Fragments contains group '{0}' definitions with different exceptions", group.Name));
                        }
                    }
                }

                //--- merge package roots ---
                foreach (var root in fragment.PackageRoots)
                {
                    // Compute absolute package root path because relative paths would be recomputed against different masterconfig locations:
                    string path = root.EvaluatePackageRoot(project);
                    path = PathString.MakeCombinedAndNormalized(Path.GetFullPath(Path.GetDirectoryName(fragment.MasterconfigFile)), path).Path;
                    var newRoot = new PackageRoot(path, root.MinLevel, root.MinLevel);

                    PackageRoots.Add(newRoot);
                }
                //IMTODO: Detect same property with different values?

                if (fragment.GlobalProperties != null)
                {
                    if (_globalProperties == null)
                    {
                        _globalProperties = new List<GlobalProperty>();
                    }

                    _globalProperties.AddRange(fragment.GlobalProperties);
                }
            }
        }

        private class MasterConfigLoader
        {
            private MasterConfig masterconfig;

            internal MasterConfigLoader(bool forceAutoBuild, string autobuildPackages, string autobuildGroups)
            {
                ForceAutoBuildSettings = new ForcedAutobuild(forceAutoBuild, autobuildPackages, autobuildGroups);
            }

            internal MasterConfig Load(string file)
            {
                if (String.IsNullOrWhiteSpace(file))
                {
                    return MasterConfig.UndefinedMasterconfig;
                }
                LineInfoDocument doc = LineInfoDocument.Load(file);

                if (doc.DocumentElement.Name != "project")
                {
                    XmlUtil.Warning(doc.DocumentElement, String.Format("Error in masterconfig file: Expected root element is 'project', found root '{0}'.", doc.DocumentElement.Name),null);
                }

                masterconfig = new MasterConfig(file);

                foreach (XmlNode childNode in doc.DocumentElement.ChildNodes)
                {
                    if (childNode.NodeType == XmlNodeType.Element && childNode.NamespaceURI.Equals(doc.DocumentElement.NamespaceURI))
                    {
                        switch (childNode.Name)
                        {
                            case "masterversions":
                                ParseMasterversions(childNode);
                                break;
                            case "packageroots":
                                ParsePackageRoots(childNode);
                                break;
                            case "config":
                                ParseConfig(childNode);
                                break;
                            case "buildroot":
                                ParseBuildRoots(childNode);
                                break;
                            case "ondemand":
                                if (masterconfig._onDemand != null)
                                {
                                    XmlUtil.Error(childNode, "Error in masterconfig file: Element 'ondemand' is defined more than once in masterconfig file.");
                                }
                                masterconfig._onDemand = ConvertUtil.ToNullableBoolean(StringUtil.Trim(childNode.GetXmlNodeText()));
                                break;
                            case "ondemandroot":
                                if (masterconfig._onDemandRoot != null)
                                {
                                    XmlUtil.Error(childNode, "Error in masterconfig file: Element 'ondemandroot' is defined more than once in masterconfig file.");
                                }
                                masterconfig._onDemandRoot = StringUtil.Trim(childNode.GetXmlNodeText());
                                break;
                            case "globalproperties":
                                if (masterconfig._globalProperties != null)
                                {
                                    XmlUtil.Error(childNode, "Error in masterconfig file: Element 'globalproperties' is defined more than once in masterconfig file.");
                                }
                                masterconfig._globalProperties = new List<GlobalProperty>();
                                ParseGlobalProperties(childNode,  masterconfig._globalProperties);
                                break;
                            case "framework1overrides":
                                ParseFramework1Overrides(childNode);
                                break;
                            case "fragments":
                                LoadFragments(childNode, masterconfig);
                                break;

                            default:
                                // Allow extra elements in mastrconfig file
                                break;
                        }
                    }
                }

                if (masterconfig.Config == null)
                {
                    throw new BuildException("Missing <config> element in masterconfig file: " + file);
                }

                return masterconfig;
            }

            internal MasterConfig LoadFragment(string file)
            {
                if (String.IsNullOrWhiteSpace(file))
                {
                    return MasterConfig.UndefinedMasterconfig;
                }
                LineInfoDocument doc = LineInfoDocument.Load(file);

                if (doc.DocumentElement.Name != "project")
                {
                    XmlUtil.Warning(doc.DocumentElement, String.Format("Error in masterconfig fragment file: Expected root element is 'project', found root '{0}'.", doc.DocumentElement.Name), null);
                }

                masterconfig = new MasterConfig(file);

                foreach (XmlNode childNode in doc.DocumentElement.ChildNodes)
                {
                    if (childNode.NodeType == XmlNodeType.Element && childNode.NamespaceURI.Equals(doc.DocumentElement.NamespaceURI))
                    {
                        switch (childNode.Name)
                        {
                            case "masterversions":
                                ParseMasterversions(childNode);
                                break;
                            case "packageroots":
                                ParsePackageRoots(childNode);
                                break;
                            case "globalproperties":
                                if (masterconfig._globalProperties != null)
                                {
                                    XmlUtil.Error(childNode, "Error in masterconfig file: Element 'globalproperties' is defined more than once in masterconfig file.");
                                }
                                masterconfig._globalProperties = new List<GlobalProperty>();
                                ParseGlobalProperties(childNode,  masterconfig._globalProperties);
                                break;
                            default:
                                XmlUtil.Error(childNode, "Error in masterconfig fragment file '"+file+"': Unknown element <" +childNode.Name +">.");
                                break;
                        }
                    }
                }

                return masterconfig;
            }


            private void LoadFragments(XmlNode el, MasterConfig masterconfig)
            {

                 var fragments = ParseFragments(el, Path.GetDirectoryName(masterconfig.MasterconfigFile));

                 foreach (var fi in fragments.FileItems)
                 {
                     var loader = new MasterConfigLoader(forceAutoBuild: false, autobuildPackages: String.Empty, autobuildGroups: String.Empty);
                     masterconfig.Fragments.Add(loader.LoadFragment(fi.Path.Path));
                 }
            }


            private FileSet ParseFragments(XmlNode el, string basedir)
            {
                FileSet fragments = new FileSet();

                fragments.BaseDirectory = basedir;

                foreach (XmlNode childNode in el.ChildNodes)
                {
                    if (childNode.NodeType == XmlNodeType.Element && childNode.NamespaceURI.Equals(el.NamespaceURI))
                    {
                        // Can be a package or grouptype:
                        switch (childNode.Name)
                        {
                            case "include":
                                fragments.Include(XmlUtil.GetRequiredAttribute(childNode, "name"));
                                break;
                            default:
                                XmlUtil.Error(el, "Error in masterconfig file: 'fragments' element can only have 'include' child nodes, found: '" + childNode.Name + "'.");
                                break;
                        }
                    }

                }
                return fragments;
            }

            private void ParseMasterversions(XmlNode el)
            {
                GroupType mastergroup = new MasterConfig.GroupType(masterconfig.PackageGroups, "masterversions", outputMapOptionSet: XmlUtil.GetOptionalAttribute(el, "outputname-map-options", null));

                masterconfig.PackageGroups.Add(mastergroup);

                foreach (XmlNode childNode in el.ChildNodes)
                {
                    if (childNode.NodeType == XmlNodeType.Element && childNode.NamespaceURI.Equals(el.NamespaceURI))
                    {
                        // Can be a package or grouptype:
                        switch (childNode.Name)
                        {
                            case "package":
                                ParsePackage(childNode, mastergroup);
                                break;
                            case "grouptype":
                                ParseGrouptype(childNode);
                                break;
                            default:
                                XmlUtil.Error(el, "Error in masterconfig file: 'masterversions' element can only have 'package' or 'grouptype' child nodes, found: '" + childNode.Name + "'.");
                                break;

                        }
                    }

                }
            }

            private void ParseGrouptype(XmlNode el)
            {
                string name = XmlUtil.GetRequiredAttribute(el, "name");
                bool? autobuildclean = ConvertUtil.ToNullableBoolean(XmlUtil.GetOptionalAttribute(el, "autobuildclean"), el);

                if (!NAntUtil.IsMasterconfigGrouptypeStringValid(name))
                {
                    XmlUtil.Error(el, String.Format("Error in masterconfig file: Group type name '{0}' is invalid.", name));
                }

                GroupType grouptype = new MasterConfig.GroupType(masterconfig.PackageGroups, name, ForceAutoBuildSettings.IsForceUtobuildGroup(autobuildclean, name), outputMapOptionSet: XmlUtil.GetOptionalAttribute(el, "outputname-map-options", null));

                //Parse exceptions and properties
                foreach (XmlNode childNode in el.ChildNodes)
                {
                    if (childNode.NodeType == XmlNodeType.Element && childNode.NamespaceURI.Equals(el.NamespaceURI))
                    {
                        switch (childNode.Name)
                        {
                            case "package":
                                ParsePackage(childNode, grouptype);
                                break;
                            case "exception":
                                grouptype.Exceptions.Add(ParseException(childNode, "name"));
                                break;
                            case "groupproperties":

                                IList<string> groupProperties = StringUtil.ToArray(childNode.GetXmlNodeText());

                                if (grouptype.GroupProperties == null)
                                {
                                    grouptype.GroupProperties = groupProperties;
                                }
                                else if(!StringUtil.ArraysEqual(grouptype.GroupProperties, groupProperties))
                                {
                                      XmlUtil.Warning(childNode, String.Format("Error in masterconfig file: Grouptype '{0}' has conflicting definitions of groupproperties '{1}' and '{2}'", grouptype.Name, StringUtil.ArrayToString(groupProperties, ", "), StringUtil.ArrayToString(grouptype.GroupProperties, ", ")), null);
                                }

                                break;

                            default:
                                XmlUtil.Error(childNode, "Error in masterconfig file: 'grouptype' element can only have 'package' or 'exception' child nodes, found: '" + childNode.Name + "'.");
                                break;
                        }
                    }
                }


                foreach(MasterConfig.GroupType gt in masterconfig.PackageGroups.FindAll((item) => item.Name == name))
                {
                        if(grouptype.Exceptions.Count == 0)
                        {
                            if(gt.AutoBuildClean != grouptype.AutoBuildClean)
                            {
                                XmlUtil.Error(el, "Error in masterconfig file: 'grouptype' element '"+name+"' is defined more than once with different autobuildclean values.");
                            }
                        }
                        else
                        {
                        }
                    }

                    masterconfig.PackageGroups.Add(grouptype);
            }

            private void ParsePackage(XmlNode el, MasterConfig.GroupType grouptype)
            {
                string name = XmlUtil.GetRequiredAttribute(el, "name");

                if(!NAntUtil.IsPackageNameValid(name))
                {
                    XmlUtil.Error(el, String.Format("Error in masterconfig file: package  name '{0}' is invalid.", name));
                }

                string version = XmlUtil.GetRequiredAttribute(el, "version");

                if (!NAntUtil.IsPackageVersionStringValid(version))
                {
                    XmlUtil.Error(el, String.Format("Error in masterconfig file: package '{0}' version '{1}' is invalid.", name, version));
                }

                MasterConfig.Package package = new MasterConfig.Package
                    (
                        grouptype,
                        name,
                        version,
                        ForceAutoBuildSettings.IsForceUtobuildPackage(ConvertUtil.ToNullableBoolean(XmlUtil.GetOptionalAttribute(el, "autobuildclean"), el), name),
                        XmlUtil.GetOptionalAttribute(el, "autobuildtarget"),
                        XmlUtil.GetOptionalAttribute(el, "autocleantarget")
                    );

                //Parse exceptions and properties
                foreach (XmlNode childNode in el.ChildNodes)
                {
                    if (childNode.NodeType == XmlNodeType.Element && childNode.NamespaceURI.Equals(el.NamespaceURI))
                    {
                        switch (childNode.Name)
                        {
                            case "properties":
                                package.Properties = StringUtil.ToArray(childNode.GetXmlNodeText());
#if FRAMEWORK_PARALLEL_TRANSITION
#else
                                if (Log.WarningLevel >= Log.WarnLevel.Deprecation)
                                {
                                    XmlUtil.Warning(el, "Error in masterconfig file: 'properties' element in 'package' is deprecated, use 'globalproperties' instead", null);
                                }
#endif
                                break;
                            case "exception":
                                package.Exceptions.Add(ParseException(childNode, "version"));
                                break;
                            default:
                                XmlUtil.Error(el, "Error in masterconfig file: 'package' element can only have 'exception' or 'properties' child nodes, found: '" + childNode.Name + "'.");
                                break;
                        }
                    }
                }

                if(masterconfig.Packages.ContainsKey(package.Name))
                {
                    XmlUtil.Error(el, "Error in masterconfig file: more than one definition of package'" + package.Name + "' found.");
                }

                masterconfig.Packages.Add(package.Name, package);
            }

            private static void ParseGlobalProperties(XmlNode el, List<GlobalProperty> globalproperties, string condition = null)
            {
                var text = new StringBuilder();
                if (el != null)
                {
                    foreach (XmlNode child in el)
                    {
                        if ((child.NodeType == XmlNodeType.Text || child.NodeType == XmlNodeType.CDATA) && child.NamespaceURI.Equals(el.NamespaceURI))
                        {
                            text.AppendLine(child.InnerText.TrimWhiteSpace());
                        }
                        else if (child.Name == "if")
                        {
                            if (text.Length > 0)
                            {
                                ParseGlobalPropertyDefinitions(globalproperties, text.ToString().TrimWhiteSpace(), condition);
                                text.Clear();
                            }
                            ParseGlobalProperties(child, globalproperties, GetCondition(child, condition));
                        }
                        else if (child.Name == "globalproperty")
                        {
                            if (text.Length > 0)
                            {
                                ParseGlobalPropertyDefinitions(globalproperties, text.ToString().TrimWhiteSpace(), condition);
                                text.Clear();
                            }

                            globalproperties.Add(new GlobalProperty(XmlUtil.GetRequiredAttribute(child, "name"), child.GetXmlNodeText(), GetCondition(child, condition)));
                        }

                    }
                    if (text.Length > 0)
                    {
                        ParseGlobalPropertyDefinitions(globalproperties, text.ToString().TrimWhiteSpace(), condition);
                    }
                }
            }

            private static string GetCondition(XmlNode el, string condition)
            {
                string val = null;
                var attr = el.Attributes["condition"];
                if (attr != null && !String.IsNullOrEmpty(attr.Value.TrimWhiteSpace()))
                {
                    val = attr.Value.TrimWhiteSpace();

                    if (!String.IsNullOrEmpty(condition))
                    {
                        val = String.Format("({0}) AND ({1})", val, condition);
                    }
                }
                else if(!String.IsNullOrEmpty(condition))
                {
                    val = condition;
                }
                return val;
            }

            private static void ParseGlobalPropertyDefinitions(List<GlobalProperty> globalProperties, string text, string condition)
            {
                foreach (string prop in StringUtil.QuotedToArray(text))
                {
                    string propName = prop;
                    string propValue = null;
                    int splitPos = prop.IndexOf("=");
                    if (splitPos != -1)
                    {
                        propName = prop.Substring(0, splitPos);
                        propValue = splitPos < prop.Length - 1 ? prop.Substring(splitPos + 1) : String.Empty;
                        propValue = StringUtil.TrimQuotes(propValue);
                    }
                    if (String.IsNullOrEmpty(propName))
                    {
                        Console.WriteLine("There is likely an error in the Masterconfig.xml file, in <globalproperties>, spaces around assignment operator are not allowed, property '{0}'", prop);
                    }
                    else
                    {
                        globalProperties.Add(new GlobalProperty(propName, propValue, condition));
                    }
                }
            }

            private MasterConfig.Exception ParseException(XmlNode el, string attrname)
            {
                MasterConfig.Exception exception = new MasterConfig.Exception(XmlUtil.GetRequiredAttribute(el, "propertyname"));

                // Parse conditions:
                foreach (XmlNode childNode in el.ChildNodes)
                {
                    if (childNode.NodeType == XmlNodeType.Element && childNode.NamespaceURI.Equals(el.NamespaceURI))
                    {
                        switch (childNode.Name)
                        {
                            case "condition":
                                exception.Conditions.Add(new MasterConfig.Condition
                                    (
                                        XmlUtil.GetRequiredAttribute(childNode, "value"),
                                        XmlUtil.GetRequiredAttribute(childNode, attrname),
                                        ConvertUtil.ToNullableBoolean(XmlUtil.GetOptionalAttribute(childNode, "autobuildclean", null))
                                     ));
                                break;
                            default:
                                XmlUtil.Error(childNode, "Error in masterconfig file: 'exception' element can only have 'condition' child nodes, found: '" + childNode.Name + "'.");
                                break;
                        }
                    }
                }

                return exception;

            }

            private void ParseConfig(XmlNode el)
            {
                if (masterconfig._config != null)
                {
                    XmlUtil.Error(el, "Error in masterconfig file: Element 'config' is defined more than once in masterconfig file.");
                }

                masterconfig._config = new ConfigInfo
                    (
                        XmlUtil.GetRequiredAttribute(el, "package"),
                        XmlUtil.GetOptionalAttribute(el, "default"),
                        XmlUtil.GetOptionalAttribute(el, "includes", null),
                        XmlUtil.GetOptionalAttribute(el, "excludes", null)
                    );

            }

            private void ParsePackageRoots(XmlNode el)
            {
                if (masterconfig._packageRoots.Count > 0)
                {
                    XmlUtil.Error(el, "Error in masterconfig file: Element 'packageroots' is defined more than once in masterconfig file.");
                }

                int? minlevel = ConvertUtil.ToNullableInt(XmlUtil.GetOptionalAttribute(el, "minlevel", null));
                int? maxlevel = ConvertUtil.ToNullableInt(XmlUtil.GetOptionalAttribute(el, "maxlevel", null));

                if (minlevel < 0 || maxlevel < 0)
                {
                    throw new BuildException("Error: <packageroots> element in masterconfig file cannot have negative 'minlevel' or 'maxlevel' attribute values.");
                }
                if (minlevel > maxlevel)
                {
                    throw new BuildException("Error: <packageroots> element in masterconfig file cannot have 'minlevel' greater than 'maxlevel' attribute value.");
                }

                //Parse exceptions and properties
                foreach (XmlNode childNode in el.ChildNodes)
                {
                    if (childNode.NodeType == XmlNodeType.Element && childNode.NamespaceURI.Equals(el.NamespaceURI))
                    {
                        switch (childNode.Name)
                        {
                            case "packageroot":
                                {
                                    var packageroot =
                                        new PackageRoot(childNode.GetXmlNodeText().TrimWhiteSpace(),
                                            ConvertUtil.ToNullableInt(XmlUtil.GetOptionalAttribute(childNode, "minlevel", null)) ?? minlevel,
                                            ConvertUtil.ToNullableInt(XmlUtil.GetOptionalAttribute(childNode, "maxlevel", null)) ?? maxlevel);
                                    masterconfig._packageRoots.Add(packageroot);

                                    //Parse exceptions 
                                    foreach (XmlNode exceptionNode in childNode.ChildNodes)
                                    {
                                        if (exceptionNode.NodeType == XmlNodeType.Element && exceptionNode.NamespaceURI.Equals(childNode.NamespaceURI))
                                        {
                                            switch (exceptionNode.Name)
                                            {
                                                case "exception":
                                                    packageroot.Exceptions.Add(ParseException(exceptionNode, "dir"));
                                                    break;
                                                default:
                                                    XmlUtil.Error(childNode, "Error in masterconfig file: 'packageroot' element can only have 'exception'  child nodes, found: '" + exceptionNode.Name + "'.");
                                                    break;
                                            }
                                        }
                                    }
                                }
                                break;
                            default:
                                XmlUtil.Error(el, "Error in masterconfig file: 'packageroots' element can only have 'packageroot' child nodes, found: '" + childNode.Name + "'.");
                                break;
                        }
                    }
                }
            }

            private void ParseBuildRoots(XmlNode node)
            {
                if (masterconfig.BuildRoot.Path != null)
                {
                    XmlUtil.Error(node, "Error in masterconfig file: Element 'buildroot' is defined more than once in masterconfig file.");
                }

                // the buildroot has no xml childnodes (exceptions) so simply read the xmltext as the buildroot.
                var splitResult = node.GetXmlNodeText().LinesToArray();
                int splitResultLen = splitResult.Count;
                if(splitResultLen <= 0)
                {
                    masterconfig._buildRoot.Path = node.GetXmlNodeText().TrimWhiteSpace();
                }
                else
                {
                    // grab the last string in the array.
                    masterconfig._buildRoot.Path = splitResult[splitResultLen - 1];
                }


                if (String.IsNullOrEmpty(masterconfig._buildRoot.Path))
                {
                    XmlUtil.Error(node, "Error in masterconfig file: Element 'buildroot' has no text data.");
                }

                // If exceptions exist, parse and store them in the BuildRootInfo struct
                if (node.HasChildNodes)
                {
                    foreach (XmlNode childNode in node.ChildNodes)
                    {
                        if (childNode.NodeType == XmlNodeType.Element)
                        {
                            Exception excp = ParseException(childNode, "buildroot");
                            masterconfig._buildRoot.Exception = excp;
                        }
                    }
                }
            }

            private void ParseFramework1Overrides(XmlNode el)
            {
                if (masterconfig._framework1Overrides.Count > 0)
                {
                    XmlUtil.Error(el, "Error in masterconfig file: Element 'framework1overrides' is defined more than once in masterconfig file.");
                }

                foreach (XmlNode childNode in el.ChildNodes)
                {
                    if (childNode.NodeType == XmlNodeType.Element && childNode.NamespaceURI.Equals(el.NamespaceURI))
                    {
                        switch (childNode.Name)
                        {
                            case "override":
                                ParseFramework1Override(childNode);
                                break;
                            default:
                                XmlUtil.Error(el, "Error in masterconfig file: 'framework1overrides' element can only have 'override' child nodes, found: '" + childNode.Name + "'.");
                                break;
                        }
                    }
                }
            }

            private void ParseFramework1Override(XmlNode el)
            {
                Fw1Override fw1override = new Fw1Override(XmlUtil.GetRequiredAttribute(el, "name"));

                foreach (XmlNode childNode in el.ChildNodes)
                {
                    if (childNode.NodeType == XmlNodeType.Element && childNode.NamespaceURI.Equals(el.NamespaceURI))
                    {
                        switch (childNode.Name)
                        {
                            case "item":
                                fw1override.Items.Add(new Fw1OverrideItem(XmlUtil.GetRequiredAttribute(childNode, "from"), XmlUtil.GetRequiredAttribute(childNode, "to")));
                                break;
                            default:
                                XmlUtil.Error(el, "Error in masterconfig file:  'override' element can only have 'item' child nodes, found: '" + childNode.Name + "'.");
                                break;
                        }
                    }
                }
                masterconfig._framework1Overrides.Add(fw1override);
            }

            private readonly ForcedAutobuild ForceAutoBuildSettings;

            private class ForcedAutobuild
            {
                internal ForcedAutobuild(bool autobuildAll, string autobuildPackage, string autobuildGroups)
                {
                    AutobuildAll = autobuildAll;
                    AutobuildPackages = new HashSet<string>(autobuildPackage.TrimQuotes().ToArray());
                    AutobuildGroups = new HashSet<string>(autobuildGroups.TrimQuotes().ToArray());
                }

                internal bool? IsForceUtobuildPackage(bool? attribValue, string packageName)
                {
                    if (AutobuildAll || AutobuildPackages.Contains(packageName))
                    {
                        return true;
                    }
                    return attribValue;
                }

                internal bool? IsForceUtobuildGroup(bool? attribValue, string groupName)
                {
                    if (attribValue != null && attribValue == false)
                    {
                        if (AutobuildAll || AutobuildGroups.Contains(groupName))
                        {
                            return true;
                        }
                    }
                    return attribValue;
                }

                private readonly bool AutobuildAll;
                private readonly ISet<string> AutobuildPackages;
                private readonly ISet<string> AutobuildGroups;
            }

        }
    }

    public class Fw1Override
    {
        internal Fw1Override(string name)
        {
            Name = name;
        }

        public readonly string Name;

        public readonly List<Fw1OverrideItem> Items = new List<Fw1OverrideItem>();
    }

    public class Fw1OverrideItem
    {
        internal Fw1OverrideItem(string from, string to)
        {
            From = from;
            To = to;
        }
        public readonly string From;

        public readonly string To;
    }



}
