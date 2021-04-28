// Originally based on NAnt - A .NET build tool
// Copyright (C) 2003-2018 Electronic Arts Inc.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
// 
// As a special exception, the copyright holders of this software give you 
// permission to link the assemblies with independent modules to produce 
// new assemblies, regardless of the license terms of these independent 
// modules, and to copy and distribute the resulting assemblies under terms 
// of your choice, provided that you also meet, for each linked independent 
// module, the terms and conditions of the license of that module. An 
// independent module is a module which is not derived from or based 
// on these assemblies. If you modify this software, you may extend 
// this exception to your version of the software, but you are not 
// obligated to do so. If you do not wish to do so, delete this exception 
// statement from your version. 
// 
// Electronic Arts (Frostbite.Team.CM@ea.com)

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Xml;

using NAnt.Core;
using NAnt.Core.Attributes;
using NAnt.Core.Logging;
using NAnt.Core.Tasks;
using NAnt.Core.Util;
using NAnt.Shared.Properties;

using EA.Eaconfig.Build;
using NAnt.Core.Structured;

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
			Group = "runtime";
			_basebuildtype = buildtype;
		}

		/// <summary></summary>
		[TaskAttribute("debug", Required = false)]
		public bool Debug { get; set; } = false;

		/// <summary></summary>       
		[TaskAttribute("comment", Required = false)]
		public string Comment { get; set; }

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
		public string Group { get; set; }

		/// <summary></summary>        
		[TaskAttribute("frompartial", Required = false)]
		public virtual string PartialModuleName
		{
			get { return PartialModuleNameProp.Value; }
			set { PartialModuleNameProp.Value = value; }
		}

		/// <summary>Used to explicitly state the build type. By default 
		/// structured XML determines the build type from the structured XML tag.
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

		/// <summary>Sets the build steps for a project</summary>
		[BuildElement("buildlayout", Required = false)]
		public BuildLayoutElement BuildLayout { get; } = new BuildLayoutElement();

		/// <summary></summary>        
		[Property("frompartial", Required = false)]
		public ConditionalPropertyElement PartialModuleNameProp { get; } = new ConditionalPropertyElement();

		/// <summary></summary>
		[BuildElement("script", Required = false)]
		public NantScript NantScript { get; } = new NantScript();

		/// <summary>Applies 'copylocal' to all dependents</summary>
		[Property("copylocal", Required = false)]
		public CopyLocalElement CopyLocal { get; set; } = new CopyLocalElement();

		protected string ExplicitBuildType
		{
			get
			{
				if (!String.IsNullOrEmpty(BuildTypeProp.Value) && Project != null)
				{
					BuildTypeProp.Value = Project.ExpandProperties(BuildTypeProp.Value);
					_buildtype = StringUtil.Trim(BuildTypeProp.Value);
					BuildTypeProp.Value = String.Empty;
					return _buildtype;
				}
				else if (!String.IsNullOrEmpty(_buildtype))
				{
					if (!String.IsNullOrEmpty(BuildTypeProp.Value) && (StringUtil.Trim(BuildTypeProp.Value) != _buildtype))
					{
						throw new BuildException($"[{ModuleName}] Defining buildtype twice is not allowed: both '{_buildtype}' and '{StringUtil.Trim(BuildTypeProp.Value)}' were defined.", Location);
					}
					return _buildtype;
				}
				return String.Empty;
			}
		}

		protected bool OverrideBuildType
		{
			get
			{
				if (BuildTypeProp != null && Project != null)
				{
					return BuildTypeProp.AttrOverrideBuildTypeValue;
				}
				else
				{
					return false;
				}
			}
		}

		public virtual bool GenerateBuildLayoutByDefault { get; } = false;

		/// <summary></summary>        
		[Property("buildtype", Required = false)]
		public BuildTypePropertyElement BuildTypeProp { get; } = new BuildTypePropertyElement();

		protected string PackageName
		{
			get { return Project.Properties["package.name"]; }
		}

		/// <summary>Sets the dependencies for a project</summary>
		[BuildElement("dependencies", Required = false)]
		public DependenciesPropertyElement Dependencies { get; } = new DependenciesPropertyElement();
		protected override void InitializeAttributes(XmlNode elementNode)
		{
			// Package package = Package.GetPackage(Project, Project.Properties["package.name"]);

			Group = BuildGroupBaseTask.GetGroupName(Project);

			var initializer = NAnt.Core.Reflection.ElementInitHelper.GetElementInitializer(this);

			long foundRequiredAttributes = 0;
			long foundUnknownAttributes = 0;
			long foundRequiredElements = 0;
			long foundUnknownElements = 0;

			for (int i = 0; i < elementNode.Attributes.Count; i++)
			{
				XmlAttribute attr = elementNode.Attributes[i];

				initializer.InitAttribute(this, attr.Name, attr.Value, ref foundRequiredAttributes, ref foundUnknownAttributes);
			}

			initializer.CheckRequiredAttributes(elementNode, foundRequiredAttributes);
			initializer.CheckUnknownAttributes(elementNode, foundUnknownAttributes);

			NantScript.InitializeFromCode(Project, elementNode);

			// Used to warning people if they are not using Structured XML
			SetModuleProperty("structured-xml-used", "true");

			if (!IsPartial)
			{
				NantScript.Execute(NantScript.Order.before);

				// Execute partial before build steps:

				//Add property to use in partial modules processing
				Project.Properties.AddLocal("module.name", ModuleName);
				Project.Properties.AddLocal("module.group", Group);

				foreach (string partialBaseModule in StringUtil.ToArray(PartialModuleName))
				{
					PartialModuleTask partialModule = PartialModuleTask.GetPartialModules(Project).FirstOrDefault(mt => mt.ModuleName == partialBaseModule);
					if (partialModule != null)
					{
						var partialScript = new NantScript();
						partialScript.InitializeFromCode(Project, partialModule.Code);
						partialScript.Execute(NantScript.Order.before);
					}
				}
			}

			if (initializer.HasNestedElements)
			{
				foreach (XmlNode childNode in elementNode.ChildNodes)
				{
					if (childNode.Name == "do" || childNode.Name == "echo")
					{
						continue;
					}
					initializer.InitNestedElement(this, childNode, true, ref foundRequiredElements, ref foundUnknownElements);

				}

				initializer.CheckRequiredNestedElements(elementNode, foundRequiredElements);
				CheckUnknownNestedElements(initializer, elementNode, foundUnknownElements);
			}
			
		}

		protected override void ExecuteTask()
		{
			base.ExecuteTask();

			InitModule();

			//Add property to use in scripts
			Project.Properties.AddLocal("module.name", ModuleName);
			Project.Properties.AddLocal("module.group", Group);
				 
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
			Project.Properties.Remove("module.group");
		}


		protected bool IsPartial
		{
			get { return this is PartialModuleTask.PartialModule || this is PartialModuleTask.DotNetPartialModule; }
		}

		protected bool HasPartial { get; private set; } = false;

		protected virtual void SetupBuildLayoutData()
		{
			bool createBuildLayoutForThisModule = BuildLayout.Enable ?? (GenerateBuildLayoutByDefault && Project.Properties.GetBooleanPropertyOrDefault("framework.create-build-layout-for-default-types", false));
			if (!createBuildLayoutForThisModule)
			{
				return;
			}

			// settings this enables build layout creation during buildgraph generation
			SetModuleProperty("generate-buildlayout", "true");

			// create an option set that will appear in the tags element of the build layout
			OptionSet defaultLayoutOptions = Project.NamedOptionSets["config-build-layout-tags-common"];
			StructuredOptionSet buildLayoutTags = defaultLayoutOptions != null ? new StructuredOptionSet(defaultLayoutOptions) : new StructuredOptionSet();

			// default per module tags
			string buildType = BuildLayout.BuildType;
			if (buildType == null)
			{
				buildType = Project.Properties["eaconfig.default-buildlayout-type"];
				if (buildType == null)
				{
					throw new BuildException("Module 'buildlayout' task doesn't specify 'build-type' attribute. This must be specified or global property 'eaconfig.default-buildlayout-type' must be set to provide a global default.", Location, stackTrace: BuildException.StackTraceType.XmlOnly);
				}
			}
			OptionSet buildOptionSet = OptionSetUtil.GetOptionSetOrFail(Project, _buildTypeInfo.Name);
			buildLayoutTags.Options["BuildType"] = buildType;
			buildLayoutTags.Options["CodeCoverage"] = buildOptionSet.GetOptionOrDefault("codecoverage", Project.GetPropertyOrDefault("eaconfig.codecoverage", "false"));
			buildLayoutTags.Options["IsFBConfiguration"] = Project.Properties.GetPropertyOrDefault("config-use-FBConfig", Project.Properties.GetPropertyOrDefault("config-isFrostbite", "false"));
			buildLayoutTags.Options["Dll"] = Project.Properties.GetPropertyOrDefault("Dll", "false"); // whether configuration is a dll configuration, not whether this specific module is a dll
			buildLayoutTags.Options["Fastlink"] = buildOptionSet.GetOptionOrDefault("debugfastlink", Project.GetPropertyOrDefault("eaconfig.debugfastlink", "off"));
			buildLayoutTags.Options["ModuleBaseDirectory"] = Project.BaseDirectory;
			buildLayoutTags.Options["ModulePackageDirectory"] = Project.GetPropertyOrFail("package.dir");
			buildLayoutTags.Options["PackageName"] = PackageName;
			buildLayoutTags.Options["PackageVersion"] = Project.Properties[PackageProperties.PackageVersionPropertyName];
			buildLayoutTags.Options["UseTAPEventsForTests"] = Project.Properties.GetPropertyOrDefault("use-tap-events-for-tests", "true");

			// append user tags at the end, this allows it to overrwrite default tags
			buildLayoutTags.Append(BuildLayout.Tags);
			SetModuleOptionSet("build-layout-tags", buildLayoutTags);

			// create an optionset that will appear in the entry point element in the build layout (skipped for platforms where it is not defined)
			OptionSet entryPointOptions = OptionSetUtil.GetOptionSet(Project, "config-build-layout-entrypoint-common");
			if (entryPointOptions != null)
			{
				StructuredOptionSet buildLayoutEntry = new StructuredOptionSet(entryPointOptions);
				SetModuleOptionSet("build-layout-entrypoint", buildLayoutEntry);
			}
			else
			{
				DoOnce.Execute
				(
					Project, "buildlayout-entrypoint-optionset-missing", () =>
					{
						Project.Log.Warning.WriteLine("Could not generate buildlayout entry point information because of missing optionset 'config-build-layout-entrypoint-common'. " +
								"Your config packages may be out of date or entry point generation may not yet be supported on platform '{0}'.", Project.GetPropertyOrFail("config-system"));
					},
					DoOnce.DoOnceContexts.global
				);
			}
		}

		protected virtual void SetBuildType()
		{
			string existingBuildType =  GetModuleProperty("buildtype");

			string finalBuildType = String.Empty;

			if (!String.IsNullOrEmpty(existingBuildType))
			{
				string explicitBuildType = ExplicitBuildType;
				if (!String.IsNullOrEmpty(explicitBuildType))
				{
					if (explicitBuildType != existingBuildType && !OverrideBuildType)
					{
						Log.Warning.WriteLine($"[{ModuleName}] Conflicting build types defined '{existingBuildType}' and '{explicitBuildType}'. Will use buildtype='{explicitBuildType}'.");
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
					throw new BuildException("INTERNAL ERROR: BuildTypeInfo only available after buildtype property is set.", Location);
				}
				return _buildTypeInfo; 

			} 
		}

		private BuildType _buildTypeInfo;


		protected void ProcessPartialModules()
		{
			if (!String.IsNullOrEmpty(PartialModuleName))
			{
				//Add property to use in partial modules processing
				Project.Properties.AddLocal("module.name", ModuleName);
				Project.Properties.AddLocal("module.group", Group);

				foreach (string partialBaseModule in StringUtil.ToArray(PartialModuleName))
				{
					foreach (PartialModuleTask partialModule in PartialModuleTask.GetPartialModules(Project))
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
							HasPartial = true;
						}
					}
				}
				if (!HasPartial)
				{
					throw new BuildException(string.Format("{0}No definition of partial module '{1}' found", LogPrefix, PartialModuleName));
				}
			}
		}

		protected string TrimWhiteSpace(string value)
		{
			if (value != null)
			{
				return value.TrimWhiteSpace();
			}
			return null;
		}

		protected string TrimWhiteSpace(ConditionalPropertyElement value)
		{
			if (value != null)
			{
				return value.Value.TrimWhiteSpace();
			}
			return null;
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

		protected void SetNonEmptyProperty(string name, string value, bool append)
		{
			if (!String.IsNullOrEmpty(value))
			{
				SetProperty(name, value, append);
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
					if (fileSet.Includes.Count > 0 || fileSet.Excludes.Count > 0)
					{
						// Prepend items with custom optionsets:
						combined.Includes.PrependRange(fileSet.Includes.Where(fi => !String.IsNullOrEmpty(fi.OptionSet)));
						combined.Includes.AddRange(fileSet.Includes.Where(fi => String.IsNullOrEmpty(fi.OptionSet)));
						combined.Excludes.AddRange(fileSet.Excludes);
					}
					if (fileSet.BaseDirectory != null)
					{
						combined.BaseDirectory = fileSet.BaseDirectory;
					}
				}
				return combined;
			}
			return fileSet;
		}

		private StructuredOptionSet AppendPartialOptionSet(StructuredOptionSet optionSet, string fromoptionSetName)
		{
			if (optionSet == null)
			{
				return optionSet;
			}
			
			if (optionSet.AppendBase)
			{
				OptionSet fromoptionSet = OptionSetUtil.GetOptionSet(Project, fromoptionSetName);
				if (fromoptionSet != null)
				{
					StructuredOptionSet mergedOptionSet = new StructuredOptionSet(fromoptionSet);
					mergedOptionSet.Append(optionSet);
					optionSet = mergedOptionSet;
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
				SetModuleProperty(propertyName + "." + targetName + ".nantbuildonly", target.NantBuildOnly.ToString().ToLowerInvariant(), false);
			}
		}
		
		protected void SetupDependencies()
		{
			// Use
			SetModuleDependencies(0, "interfacedependencies", Dependencies.UseDependencies.Interface_1_Link_0 + Environment.NewLine + Dependencies.InterfaceDependencies.Interface_1_Link_0);
			SetModuleDependencies(1, "usedependencies", Dependencies.UseDependencies.Interface_1_Link_1 + Environment.NewLine + Dependencies.UseDependencies.Interface_1_Link_1); // TODO: dvaliant 2021-01-16, this looks like a bug - it's appending the same value to itself. second was likely mean to be .InterfaceDependencies. I'm afraid to fix this given how long it's been here and it's a very rare use case
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
				SetModuleProperty("copylocal.dependencies", copylocalitems, append: true);
			}

			if (Dependencies.SkipRuntimeDependency == true) 
			{
				SetModuleProperty("skip-runtime-dependency", "true");
			}

			var copylocalVal = CopyLocal.Value.ToArray().LastOrDefault();
			if (!String.IsNullOrEmpty(copylocalVal))
			{
				CopyLocal.Value = ConvertUtil.ToEnum<CopyLocalType>(copylocalVal).ToString();
				SetModuleProperty("copylocal", CopyLocal, append: false);
			}

			if (!CopyLocal.UseHardLinkUseDefault)
			{
				ConditionalPropertyElement useHardLink = new ConditionalPropertyElement(append: false);
				useHardLink.Value = CopyLocal.UseHardLinkIfPossible ? "true" : "false";
				SetModuleProperty("copylocal_usehardlinkifpossible", useHardLink);
			}

		}

		protected void SetModuleDependencies(int mappingInd, string dependencytype, string value)
		{
			if (!String.IsNullOrWhiteSpace(value))
			{
				var processedpackages = new HashSet<string>(); // To avoid duplicate entries

				var moduleDependenciesName = Mappings.ModuleDependencyPropertyMapping[mappingInd].Key;
				var moduleConstraintsName = Mappings.ModuleDependencyConstraintsPropertyMapping[mappingInd].Key;

				// Store packageName since it is looking up in multiple dictionaries and take locks
				string packageName = PackageName;

				DependencyDeclaration dependencyDeclaration = new DependencyDeclaration(value, Location, Name);
				foreach (DependencyDeclaration.Package package in dependencyDeclaration.Packages)
				{
					if (package.Name != packageName) // package cannot depend upon itself
					{
						SetModuleProperty(dependencytype, package.Name, true);
					}
					foreach (DependencyDeclaration.Group group in package.Groups)
					{
						foreach (string module in group.Modules)
						{
							if (package.Name == packageName) // if package is package for this module then this is intra-package reference
							{
								SetModuleProperty(group.Type.ToString() + moduleDependenciesName, module, true);
							}
							else // else this is referencing specific module in another package
							{
								SetModuleProperty(package.Name + "." + group.Type.ToString() + moduleConstraintsName, module, true);
							}
						}
					}
				}
			}
		}

		protected override void InitializeParentTask(XmlNode taskNode, Element parent)
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
						sb.AppendLine("         OptionSet=\"" + item.OptionSet + "\"");
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

	/// <summary>
	/// Execute arbitrary XML script. Useful in partial modules.
	/// </summary>
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

		/// <summary>Script can be executed 'before' structured XML data are processed or 'after'.</summary>
		[TaskAttribute("order", Required = true)]
		public Order ExecutionOrder { get; set; } = Order.before;

		/// <summary>Add all the child option elements.</summary>
		/// <param name="elementNode">XML node to initialize from.</param>
		protected override void InitializeElement(XmlNode elementNode)
		{
			if (!_initialized)
			{
				var script = new ScriptEntry();
				script.Order = ExecutionOrder;
				script.XmlNode = elementNode;

				_scripts.Add(script);

				ExecutionOrder = Order.before;
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
					SectionTask task = new SectionTask
					{
						Project = Project,
						Parent = null
					};
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
					ExecScript task = new ExecScript
					{
						Project = Project
					};
					task.Initialize(script.XmlNode);

					task.Execute();
				}
			}
		}

		class ExecScript : TaskContainer
		{
			/// <summary>Script can be executed 'before' structured XML data are processed or 'after'.</summary>
			[TaskAttribute("order", Required = false)]
			public Order Dummy { set {} }
		}

	}


	/// <summary>
	/// Module's global copylocal setting.
	/// </summary>
	[TaskName("copylocal")]
	public class CopyLocalElement : ConditionalPropertyElement
	{
		bool m_useHardLinkIfPossible;

		public CopyLocalElement()
			: base(append: false)
		{
			m_useHardLinkIfPossible = false;
			UseHardLinkUseDefault = true;
		}

		/// <summary>Specifies that the copylocal task will use hard link if possible (default is false)</summary>        
		[TaskAttribute("use-hardlink-if-possible", Required = false)]
		public bool UseHardLinkIfPossible
		{
			get { return m_useHardLinkIfPossible; }
			set { m_useHardLinkIfPossible = value; UseHardLinkUseDefault = false;  }
		}

		public bool UseHardLinkUseDefault { get; private set; }
	}

}
