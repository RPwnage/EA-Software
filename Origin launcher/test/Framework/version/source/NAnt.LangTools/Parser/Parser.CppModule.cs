using System;
using System.Collections.Generic;
using System.Xml.Linq;

namespace NAnt.LangTools
{
	public partial class Parser
	{
		internal sealed class CppModule : ModuleBase
		{
			internal sealed class Configuration
			{
				internal readonly ConditionalPropertyElement Defines = new ConditionalPropertyElement();
				internal readonly ConditionalPropertyElement WarningSuppression = new ConditionalPropertyElement();

				internal void InitOrUpdateFromElement(Parser parser, XElement configElement)
				{
					parser.ProcessSubElements
					(
						configElement,
						new Dictionary<string, TaskHandler>
						{
							{ "defines", Defines.InitOrUpdateFromElement },
							{ "warningsuppression", WarningSuppression.InitOrUpdateFromElement }
						}
					);
				}
			}

			internal Property.Node BuildType { get; private set; } = null;

			internal readonly ConditionalPropertyElement IncludeDirs = new ConditionalPropertyElement();
			internal readonly Configuration Config = new Configuration();

			internal CppModule(string defaultBuildType = null)
			{
				BuildType = defaultBuildType;
			}

			internal void InitOrUpdateFromElement(Parser parser, XElement moduleElement)
			{
				parser.ProcessSubElements(moduleElement, GetInitOrUpdateTasks());
			}

			internal static void ModuleTaskHandler(Parser parser, XElement moduleElement) => BaseModuleTaskHandler(parser, moduleElement);
			internal static void LibraryTaskHandler(Parser parser, XElement libraryElement) => BaseModuleTaskHandler(parser, libraryElement, "Library");
			internal static void ProgramTaskHandler(Parser parser, XElement programElement) => BaseModuleTaskHandler(parser, programElement, "Program");

			protected override Dictionary<string, TaskHandler> GetInitOrUpdateTasks()
			{
				return CombineTaskHandlers
				(
					base.GetInitOrUpdateTasks(),
					new Dictionary<string, TaskHandler>
					{
						{ "buildtype", (innerParser, element) => BuildType = BuildTypeModuleElementHandler(innerParser, element, BuildType) },
						{ "bulkbuild", (innerParser, element) => { /* todo: bulk build */ } },
						{ "config", Config.InitOrUpdateFromElement },
						{ "copylocal", (innerParser, element) => { /* copy local */ } },
						{ "includedirs", IncludeDirs.InitOrUpdateFromElement },
						{ "sourcefiles", (innerParser, element) => { /* BIG TODO: FILESET SUPPORT */ } },
						{ "visualstudio", (innerParser, element) => { /* todo: visualstudio generation info */ } },
					}
				);
			}

			private static void BaseModuleTaskHandler(Parser parser, XElement moduleElement, string defaultBuildType = null)
			{
				ValidateAttributes(moduleElement, "if", "unless", "name"/*, buildtype*/); // todo: buildtype from attribute

				parser.HandleIfUnless
				(
					moduleElement, () =>
					{
						parser.ResolveAttributes
						(
							moduleElement, "name",
							(moduleName) =>
							{
								Property.Node groupNode = parser.GetPropertyOrNull(BuildGroupPrivateProperty) ?? "runtime"; // it's very unlikely this is conditional, but do a resolve just in case
								parser.ResolvePropertyNode
								(
									groupNode,
									group =>
									{
										CppModule module = new CppModule(defaultBuildType);
										module.InitOrUpdateFromElement(parser, moduleElement);

										// update modules list
										string modulesPropertyName = $"{group}.buildmodules";
										Property.Node modulesProperty = parser.GetPropertyOrNull(modulesPropertyName);
										modulesProperty = modulesProperty?.AppendLine(moduleName) ?? moduleName;
										parser.SetProperty(modulesPropertyName, modulesProperty, local: false);

										// record module information
										{
											// build type TODO: buildtype cannot also be set from property BEFORE or by attribute
											if (module.BuildType == null)
											{
												throw new FormatException(); // todo no build type
											}
											SetModuleProperty(parser, moduleName, group, "buildtype", module.BuildType);

											// include dirs
											if (module.IncludeDirs.Value != null)
											{
												SetModulePropertyFromConditionalPropertyElement(parser, moduleName, group, "includedirs", PathCombineMultiLineProperty(parser.PackageDirToken, module.IncludeDirs.Value), module.IncludeDirs.Append);
											}
										}
									}
								);
							}
						);
					}
				);
			}

			private static void SetModuleProperty(Parser parser, string module, string group, string name, Property.Node value)
			{
				string propertyName = $"{group}.{module}.{name}";
				parser.SetProperty(propertyName, value, local: false);
			}

			private static void SetModulePropertyFromConditionalPropertyElement(Parser parser, string module, string group, string name, ConditionalPropertyElement element)
			{
				SetModulePropertyFromConditionalPropertyElement(parser, module, group, name, element.Value, element.Append);
			}

			private static void SetModulePropertyFromConditionalPropertyElement(Parser parser, string module, string group, string name, Property.Node value, Property.Node append)
			{
				string propertyName = $"{group}.{module}.{name}";
				ConditionValueSet appendConditions = parser.ConvertBooleanToConditionSet(append);
				Property.Node existing = parser.GetPropertyOrNull(propertyName);

				if (appendConditions != ConditionValueSet.True || existing == null)
				{
					parser.SetProperty(propertyName, value, local: false);
				}
				if (appendConditions != ConditionValueSet.False && existing != null)
				{
					Property.Node appendedNode = existing.Append(value).AttachConditions(appendConditions);
					parser.SetProperty(propertyName, appendedNode, local: false);
				}
			}

			private static Property.Node BuildTypeModuleElementHandler(Parser parser, XElement buildTypeElement, Property.Node existingBuildType) // todo: these are getting painful to maintain - need a more generic xml parsing framework for complex elements
			{
				ValidateAttributes(buildTypeElement, "if", "unless", "name"/*, override*/);

				Property.Node newNode = null;
				parser.HandleIfUnless
				(
					buildTypeElement, () =>
					{
						parser.ProcessSubElements(buildTypeElement, NoTasks);
						newNode = AttributeOrContentsNode(parser, buildTypeElement, attributeName: "name");
					}
				);

				if (newNode != null)
				{
					existingBuildType = existingBuildType?.Update(newNode) ?? newNode;
				}
				return existingBuildType;
			}
		}
	}
}