using System;
using System.Xml;

using NAnt.Core;
using NAnt.Core.Attributes;
using NAnt.Core.Structured;

namespace EA.Eaconfig.Structured
{
	public abstract class JavaModuleBaseTask : ModuleBaseTask
	{
		[ElementName("dependencies", NestedElementsCheck = true, StrictAttributeCheck = true, Mixed = true)]
		public class JavaDependencies : ConditionalPropertyElement
		{
			protected override void InitializeElement(XmlNode node) // DAVE-FUTURE-REFACTOR-TODO: copy pasta from DependencyDefinitionPropertyElement  - should see if we can simplify both as this seems to do prretty standard stuff, shouldn't this just be conditionalpropertyelement?
			{
				if (IfDefined && !UnlessDefined)
				{
					string newValue = null; // build up the total string value to be added by this element

					// if <element value="text"/> is set take this value
					if (AttrValue != null)
					{
						newValue = AttrValue;
					}

					// handle inner elements
					foreach (XmlNode child in node)
					{
						string newFragment = null; // fragment of new value added by text, cdata or <do> element
						switch (child.NodeType)
						{
							case XmlNodeType.Comment:
								continue; // skip comments
							case XmlNodeType.Text:
							case XmlNodeType.CDATA:
								if (AttrValue != null)
								{
									throw new BuildException($"Element value can not be defined in both '{AttrValue}' attribute and as element text.", Location);
								}

								newFragment = child.InnerText;
								break;
							default:
								if (child.Name == "do")
								{
									// gross hack, ConditionalElement gets direct access to internal value so cache current value then clear it....
									string currentValue = Value;
									Value = null;

									// ...initialize child and store what it set our value to...
									ConditionElement condition = new ConditionElement(Project, this);
									condition.Initialize(child);
									newFragment = Value;

									// ...then set it back to old value
									Value = currentValue;
								}
								else
								{
									// do is the only allow nested element
									throw new BuildException($"XML tag <{child.Name}> is not allowed inside property values. Use <do> element for conditions.", Location);
								}
								break;
						}

						// append new fragment to new value 
						if (newFragment != null)
						{
							newValue = (newValue == null) ? newFragment : newValue + Environment.NewLine + newFragment;
						}
					}

					// append new value to existing value, copy local value and internal value as appropriate
					if (newValue != null)
					{
						Value = (Value == null) ? newValue : Value + Environment.NewLine + newValue;
					}

					// perform expansion to get final value
					if (!String.IsNullOrEmpty(Value))
					{
						Value = Project.ExpandProperties(Value);
					}
				}
			}
		}

		/// <summary>Sets the directory to use for Gradle to build this Android module. A build.gradle should be located in this directory. If omitted
		/// then the directory of the .build file will be used.</summary>
		[TaskAttribute("gradle-directory", Required = false)]
		public string GradleDirectory
		{
			get { return m_gradleFilePath; }
			set { m_gradleFilePath = Project.GetFullPath(value); }
		}

		/// <summary>Project name for this module in Gradle context If omitted the module name will be used.</summary>
		[TaskAttribute("gradle-project", Required = false)]
		public string GradleProject { get; set; } = null;

		[TaskAttribute("package-transitive-native-dependencies", Required = false)]
		public bool PackageTransitiveNativeDependencies { get; set; } = true;

		/// <summary>Set the dependencies for this java module. Note that unlikely other modules, specific dependency types do not need to be defined.</summary>
		[BuildElement("dependencies", Override = true)]
		public new JavaDependencies Dependencies { get; set; } = new JavaDependencies();

		/// <summary>Java files to display in Visual Studio for debugging purposes (not used for build).</summary>
		[FileSet("javasourcefiles", Required = false)]
		public StructuredFileSet JavaSourceFiles { get; set; } = new StructuredFileSet();

		/// <summary>Extra values that should be added to the gradle.properties file when building this project.</summary>
		[Property("gradle-properties")]
		public ConditionalPropertyElement GradleProperties { get; } = new ConditionalPropertyElement();

		private string m_gradleFilePath;

		protected JavaModuleBaseTask(string buildtype) : base(buildtype)
		{
		}

		protected override void ExecuteTask()
		{
			// GRADLE-TODO: should probably throw an error if config-system isn't android
			base.ExecuteTask();
		}

		protected override void InitModule() 
		{
		}

		protected override void SetupModule()
		{
			string gradleDir = GradleDirectory ?? Project.BaseDirectory;

			SetModuleProperty("gradle-directory", gradleDir);
			SetModuleProperty("gradle-project", GradleProject ?? ModuleName);
			SetModuleProperty("android-package-transitive-native-dependencies", PackageTransitiveNativeDependencies.ToString());

			SetModuleProperty("gradle-properties", GradleProperties, GradleProperties.Append);

			SetModuleFileset("javasourcefiles", JavaSourceFiles);

			// dependencies are always build dependencies for java modules - java modules don't need an initialize.xml entry sp
			// we go straight to the build file
			SetModuleDependencies(6, "buildonlydependencies", Dependencies.Value);
		}

		protected override void FinalizeModule()
		{
		}
	}
}
