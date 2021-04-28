using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Text.RegularExpressions;
using System.Xml;
using System.Xml.Linq;

namespace NAnt.LangTools
{
	public partial class Parser
	{
		internal delegate void TaskHandler(Parser parser, XElement taskElement);

		internal static readonly Dictionary<string, TaskHandler> NoTasks = new Dictionary<string, TaskHandler>();
		internal static readonly Dictionary<string, TaskHandler> AllowNestedDo = new Dictionary<string, TaskHandler>
		{
			{ "do", (parser, element) => { } }, // defer handling to AttributeOrContentsNode
		};
		internal static readonly Dictionary<string, TaskHandler> RootTasks = new Dictionary<string, TaskHandler>
		{
			// low level nant tasks
			{ "do", DoTaskHandler },
			{ "fileset", (parser, element) => { /*BIG TODO: filesets!*/ } },
			{ "foreach", ForEachTaskHandler },
			{ "optionset", OptionSetTaskHandler },
			{ "property", PropertyTaskHandler },

			// ea tasks
			{ "BuildType", BuildTypeTaskHandler },
			{ "package", PackageTaskHandler },

			// initialize.xml public data tasks // TODO: move these to class, or at least public module
			{ "publicdata", PublicDataTaskHandler },			
			{ "module", PublicModuleTaskHandler },

			// .build structured tasks
			{ "Library", CppModule.LibraryTaskHandler },
			{ "Module", CppModule.ModuleTaskHandler },
			{ "Program", CppModule.ProgramTaskHandler },

			// grouping
			{ "examples", ExampleGroupTaskHandler },
			{ "runtime", RuntimeGroupTaskHandler },
			{ "tests", TestGroupTaskHandler },
			{ "tools", ToolGroupTaskHandler },

			// deprecated
			{ "spu", (parser, element) => { /* todo */ } }
		};

		private void ResolveAttributes(XElement taskElement, string attribute, Action<string> onResolve)
		{
			string attributeString = taskElement.Attribute(attribute)?.Value;
			if (attributeString == null)
			{
				onResolve(null);
				return;
			}

			ScriptEvaluation.Node scriptNode = ScriptEvaluation.Evaluate(attributeString);
			Property.Node node = ResolveScriptEvaluation(scriptNode);
			ResolvePropertyNode(node, onResolve);
		}

		private void ResolveAttributes(XElement taskElement, string attribute1, string attribute2, Action<string, string> onResolve) =>
			ResolveAttributes(taskElement, attribute1, (attribute1Value) =>
				ResolveAttributes(taskElement, attribute2, (attribute2Value) =>
					onResolve(attribute1Value, attribute2Value)));

		private void ResolveAttributes(XElement taskElement, string attribute1, string attribute2, string attribute3, Action<string, string, string> onResolve) =>
			ResolveAttributes(taskElement, attribute1, attribute2, (attribute1Value, attribute2Value) =>
				ResolveAttributes(taskElement, attribute3, (attribute3Value) =>
					onResolve(attribute1Value, attribute2Value, attribute3Value)));

		private void ResolveAttributes(XElement taskElement, string attribute1, string attribute2, string attribute3, string attribute4, Action<string, string, string, string> onResolve) =>
			ResolveAttributes(taskElement, attribute1, attribute2, attribute3, (attribute1Value, attribute2Value, attribute3Value) =>
				ResolveAttributes(taskElement, attribute4, (attribute4Value) =>
					onResolve(attribute1Value, attribute2Value, attribute3Value, attribute4Value)));

		private void HandleIfUnless(XElement taskElement, Action onTrue)
		{
			ConditionValueSet ifConditionSet = null;
			string ifString = taskElement.Attribute("if")?.Value;
			if (ifString != null)
			{
				ifConditionSet = ConvertBooleanToConditionSet(ifString);
			}

			ConditionValueSet unlessConditionSet = null;
			string unlessString = taskElement.Attribute("unless")?.Value;
			if (unlessString != null)
			{
				unlessConditionSet = ConvertBooleanToConditionSet(unlessString, invert: true);
			}

			ConditionValueSet finalSet = (ifConditionSet != null && unlessConditionSet != null) ?
				ifConditionSet.And(unlessConditionSet) :
				ifConditionSet ?? unlessConditionSet ?? ConditionValueSet.True;

			m_conditionStack.Push(finalSet);
			if (finalSet != ConditionValueSet.False)
			{
				onTrue();
			}
			m_conditionStack.Pop();
		}

		private ConditionValueSet ConvertBooleanToConditionSet(Property.Node boolProperty, bool invert = false)
		{
			ConditionValueSet conditionSet = ConditionValueSet.False;
			Dictionary<string, ConditionValueSet> expandedBool = boolProperty.Expand(m_conditionStack.Peek());

			foreach (KeyValuePair<string, ConditionValueSet> stringToConditions in expandedBool)
			{
				Expression.Node combinedNode = Expression.Evaluate(stringToConditions.Key);
				combinedNode = Expression.Reduce(combinedNode);
				if (combinedNode == (invert ? Expression.False : Expression.True))
				{
					conditionSet = conditionSet.Or(stringToConditions.Value);
				}
				else if (combinedNode != (invert ? Expression.True : Expression.False))
				{
					throw new FormatException(); // TODO
				}
			}

			return conditionSet;
		}

		private ConditionValueSet ConvertBooleanToConditionSet(string boolString, bool invert = false)
		{
			ScriptEvaluation.Node booleanEvaluation = ScriptEvaluation.Evaluate(boolString);
			Property.Node boolProperty = ResolveScriptEvaluation(booleanEvaluation);
			return ConvertBooleanToConditionSet(boolProperty, invert);
		}

		private void ProcessSubElements(XElement element, Dictionary<string, TaskHandler> taskHandlers, bool allowUnrecognisedTasks = false)
		{
			foreach (XElement subElement in element.Elements())
			{
				if (!taskHandlers.TryGetValue(subElement.Name.LocalName, out TaskHandler handler))
				{
					if (!allowUnrecognisedTasks)
					{
						throw new KeyNotFoundException($"Unhandled task: {subElement.Name.LocalName}."); // to line no etc
					}

					// BIG TODO: log these
#if DEBUG
					throw new NotImplementedException($"Possible unimplemented task: {subElement.Name.LocalName}.");
#endif
				}
				else
				{
					handler(this, subElement);
				}
			}
		}

		private void IncludeFile(string nantScriptFile)
		{
			// TODO - should check whether what are are including is inside package or not
			// TODO - tokenize the basedir before we can set it reliably

			// set new base dir
			//Property.Node previousBaseDir = GetPropertyOrNull("nant.project.basedir"); // todo: make constant
			//SetProperty("nant.project.basedir", Path.GetDirectoryName(nantScriptFile), false);
			{
				// set new local property context
				m_localProperties.Push(new Property.Map());
				{
					ProcessSubElements(XDocument.Load(nantScriptFile, LoadOptions.PreserveWhitespace | LoadOptions.SetBaseUri | LoadOptions.SetLineInfo).Root, RootTasks, allowUnrecognisedTasks: true);
				}
				m_localProperties.Pop();
			}
			/*if (previousBaseDir != null)
			{
				SetProperty("nant.project.basedir", previousBaseDir, false);
			}*/
		}

		private static void ValidateAttributes(XElement taskElement, params string[] validNames)
		{
			IEnumerable<string> invalidNames = taskElement.Attributes().Select(attr => attr.Name.LocalName).Except(validNames);
			if (invalidNames.Any())
			{
				throw new NotImplementedException($"Unhandled attributes: {String.Join(", ", invalidNames)}.");
			}
		}

		private static void DoTaskHandler(Parser parser, XElement taskElement)
		{
			ValidateAttributes(taskElement, "if", "unless");

			parser.HandleIfUnless(taskElement, () => parser.ProcessSubElements(taskElement, RootTasks, allowUnrecognisedTasks: true));
		}

		private static void PropertyTaskHandler(Parser parser, XElement taskElement)
		{
			ValidateAttributes(taskElement, "name", "value", "local", "if", "unless");

			parser.HandleIfUnless
			(
				taskElement, () =>
				{
					parser.ProcessSubElements(taskElement, NoTasks);
					parser.ResolveAttributes
					(
						taskElement, "name", "local",
						(name, local) =>
						{
							bool isLocal = false;
							if (local != null && !Boolean.TryParse(local, out isLocal))
							{
								throw new FormatException(); // todo
							}

							Property.Node node = AttributeOrContentsNode
							(
								parser, taskElement,
								resolveNode: propertyName =>
								{
									if (propertyName == "property.value")
									{
										return parser.GetPropertyOrNull(name) ?? String.Empty;
									}
									return null; // null falls back to default implementation
								},
								automaticallyExpand: false
							);
							parser.SetProperty(name, node, isLocal);
						}
					);
				}
			);
		}

		private static void ForEachTaskHandler(Parser parser, XElement taskElement)
		{
			ValidateAttributes(taskElement, "if", "unless", "item", /*delimiter,*/ "in", "property", "local"); // todo

			parser.HandleIfUnless
			(
				taskElement, () => parser.ResolveAttributes
				(
					taskElement, "item", "in", "property", "local",
					(item, inAttr, property, local) =>
					{
						bool isLocal = false;
						if (local != null && !bool.TryParse(local, out isLocal))
						{
							throw new FormatException(); // todo
						}

						switch (item)
						{
							case "String":
								Property.Node existingValue = parser.GetPropertyOrNull(property); // todo - if it has a value already error on local (see nant implementation)
								string[] inSplit = inAttr.Split(Regex.Unescape(new string(Constants.DefaultDelimiters)).ToCharArray(), StringSplitOptions.RemoveEmptyEntries); // todo, this madness will make sense once I support delim
								foreach (string inElem in inSplit) 
								{
									string trimmedElem = inElem.Trim(); // from nant's foreach
									if (trimmedElem.Length > 0)
									{
										parser.SetProperty(property, trimmedElem, isLocal);
										parser.ProcessSubElements(taskElement, RootTasks, allowUnrecognisedTasks: true);
									}
								}
								if (existingValue is null)
								{
									parser.RemoveProperty(property);
								}
								else
								{
									parser.SetProperty(property, existingValue, local: false);
								}
								break;
							default:
								throw new NotImplementedException(); // todo	
						}
					}
				)
			);
		}

		private static void OptionSetTaskHandler(Parser parser, XElement taskElement)
		{
			ValidateAttributes(taskElement, "name", "fromoptionset", /*"append",*/ "if", "unless");

			parser.HandleIfUnless
			(
				taskElement, () => parser.ResolveAttributes
				(
					taskElement, "name", "fromoptionset",
					(name, fromOptionSet) =>
					{
						bool append = true; // todo

						CreateOptionSetFromElement(parser, taskElement, name, fromOptionSet, append);
					}
				)
			);
		}

		private static void PackageTaskHandler(Parser parser, XElement taskElement)
		{
			ValidateAttributes(taskElement, "name", "targetversion", "initializeself");

			parser.ProcessSubElements(taskElement, NoTasks);

			string basePath = Path.GetDirectoryName(new Uri(taskElement.Document.BaseUri).AbsolutePath);
			string packageDir = parser.PackageDirToken;

			parser.SetProperty("package.name", parser.PackageName, local: false);
			parser.SetProperty("package.dir", packageDir, local: false);
			parser.SetProperty($"package.{parser.PackageName}.dir", packageDir, local: false);

			// TODO: using Framework names as our parsers names for now - but there is no reason to maintain this if we want cleaner names abstract names
			// TODO: we could potentially limit compiler by platforms
			parser.SetProperty
			(
				"config-system",
				new Property.Conditional(Constants.PlatformCondition, Constants.Platforms.ToDictionary<string, string, Property.Node>(p => p, p => p)), 
				local: false
			);
			parser.SetProperty
			(
				"config-compiler",
				new Property.Conditional(Constants.CompilerCondition, Constants.Compilers.ToDictionary<string, string, Property.Node>(p => p, p => p)),
				local: false
			);
			parser.SetProperty
			(
				"Dll",
				new Property.Conditional(Constants.SharedBuildCondition, Constants.BooleanOptions.ToDictionary<string, string, Property.Node>(p => p, p => p)),
				local: false
			);

			OptionSet libraryOptions = parser.GetOptionSetOrNull("config-options-library") ?? new OptionSet();
			// todo: set some placeholder token defaults?
			parser.SetOptionSet("config-options-library", libraryOptions);

			OptionSet programOptions = parser.GetOptionSetOrNull("config-options-program") ?? new OptionSet();
			// todo: set some placeholder token defaults?
			parser.SetOptionSet("config-options-program", programOptions);

			// use the <include> task to include the "Initialize" script if it exists
			string initScriptPath = Path.Combine(basePath, Path.Combine("scripts", "Initialize.xml"));
			if (!File.Exists(initScriptPath))
			{
				// Some packages may use lower case, which is important on Linux
				initScriptPath = Path.Combine(basePath, Path.Combine("scripts", "initialize.xml"));

				if (!File.Exists(initScriptPath))
				{
					initScriptPath = Path.Combine(basePath, "Initialize.xml");
					if (!File.Exists(initScriptPath))
					{
						// Some packages may use lower case, which is important on Linux
						initScriptPath = Path.Combine(basePath, "initialize.xml");
						if (!File.Exists(initScriptPath))
						{
							return;
						}
					}
				}
			}

			parser.IncludeFile(initScriptPath);
		}

		private static void BuildTypeTaskHandler(Parser parser, XElement taskElement)
		{
			ValidateAttributes(taskElement, "name", "from", "if", "unless");

			// for the purposes of our parser buildtype is almost the same as an optionset declaration
			// since we have no interest in resolving from platform agnostic options to specific options.
			// We want to keep things as abstract as possible
			parser.HandleIfUnless
			(
				taskElement, () => parser.ResolveAttributes
				(
					taskElement, "name", "from",
					(name, from) => CreateOptionSetFromElement(parser, taskElement, name, from)
				)
			);
		}

		private static void RuntimeGroupTaskHandler(Parser parser, XElement runtimeElement) => GroupTaskHandler(parser, runtimeElement, "runtime");
		private static void TestGroupTaskHandler(Parser parser, XElement testElement) => GroupTaskHandler(parser, testElement, "test");
		private static void ToolGroupTaskHandler(Parser parser, XElement toolElement) => GroupTaskHandler(parser, toolElement, "tool");
		private static void ExampleGroupTaskHandler(Parser parser, XElement exampleElement) => GroupTaskHandler(parser, exampleElement, "example");

		private static void GroupTaskHandler(Parser parser, XElement groupElement, string group)
		{
			ValidateAttributes(groupElement, "if", "unless");

			parser.HandleIfUnless
			(
				groupElement, () =>
				{
					Property.Node existingGroup = parser.GetPropertyOrNull(BuildGroupPrivateProperty);
					parser.SetProperty(BuildGroupPrivateProperty, group, local: false);
					parser.ProcessSubElements(groupElement, RootTasks, allowUnrecognisedTasks: true);

					if (existingGroup is null)
					{
						parser.RemoveProperty(BuildGroupPrivateProperty);
					}
					else
					{
						parser.SetProperty(BuildGroupPrivateProperty, existingGroup, local: false);
					}
				}
			);
		}

		private static void PublicDataTaskHandler(Parser parser, XElement publicDataElement)
		{
			ValidateAttributes(publicDataElement, "if", "unless", "packagename"/*, "libnames", "add-defaults", "configbindir"*/);

			parser.HandleIfUnless
			(
				publicDataElement, () => parser.ResolveAttributes
				(
					publicDataElement, "packagename",
					(packageName) =>
					{
						// TODO: probably some properties we need to set here, but honestly this doesn't do much other than wrap
						// any arbitrary task
						// TODO: validate package name?
						parser.ProcessSubElements(publicDataElement, RootTasks, allowUnrecognisedTasks: true);
					}
				)
			);
		}

		private static void PublicModuleTaskHandler(Parser parser, XElement moduleElement)
		{
			ValidateAttributes(moduleElement, "name", "buildtype");

			parser.HandleIfUnless
			(
				// TODO: need to check we are in a <publicdata> - nant fails for <module> once if / unless are handled if outside <publicdata>
				moduleElement, () => parser.ResolveAttributes
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
								ConditionalPropertyElement includeDirs = new ConditionalPropertyElement();
								ConditionalPropertyElement defines = new ConditionalPropertyElement(); ;

								// TODO: convert to a class
								parser.ProcessSubElements
								(
									moduleElement,
									new Dictionary<string, TaskHandler>
									{
										{ "includedirs", includeDirs.InitOrUpdateFromElement },
										{ "defines", defines.InitOrUpdateFromElement }
									}
								);

								// TODO: includedirs has defaults if add default is used
								if (includeDirs.Value != null)
								{
									Property.Node joinedIncludeDirs = PathCombineMultiLineProperty(parser.PackageDirToken, includeDirs.Value);
									parser.SetProperty($"package.{parser.PackageName}.{moduleName}.includedirs", joinedIncludeDirs, local: false);
								}
								if (defines.Value != null)
								{
									parser.SetProperty($"package.{parser.PackageName}.{moduleName}.defines", defines.Value, local: false);
								}

								// update public build modules list, following framework's implementation so even first value gets prepended with newline
								string buildModulesPropertyName = $"package.{parser.PackageName}.{group}.buildmodules";
								Property.Node oldValue =  parser.GetPropertyOrNull(buildModulesPropertyName) ?? "";
								parser.SetProperty(buildModulesPropertyName, oldValue + Constants.Newline + moduleName, local: false);						
							}
						);
					}
				)
			);
		}

		private static void CreateOptionSetFromElement(Parser parser, XElement taskElement, string optionSetName, string fromOptionSetName, bool append = true)
		{
			OptionSet newSet = append ?
				(parser.GetOptionSetOrNull(optionSetName) ?? new OptionSet()) :
				new OptionSet();

			if (fromOptionSetName != null)
			{
				OptionSet fromSet = parser.GetOptionSetOrNull(fromOptionSetName);
				if (fromSet == null)
				{
					throw new KeyNotFoundException($"Cannot create new optionset from '{fromOptionSetName}'. No optionset exists with this name.");
				}
				newSet = newSet.Update(fromSet);
			}

			parser.ProcessSubElements
			(
				taskElement,
				new Dictionary<string, TaskHandler>()
				{
					{ "option", (innerParser, element) => SetOptionSetOptionFromElement(innerParser, element, newSet) }
				}
			);

			parser.SetOptionSet(optionSetName, newSet);
		}

		private static void SetOptionSetOptionFromElement(Parser parser, XElement optionElement, OptionSet optionSet)
		{
			ValidateAttributes(optionElement, "name", "value", "if", "unless");

			parser.HandleIfUnless
			(
				optionElement, () =>
				{
					parser.ProcessSubElements(optionElement, NoTasks);
					parser.ResolveAttributes
					(
						optionElement, "name",
						(name) =>
						{
							Property.Node existingOption = optionSet.GetOptionOrNullWithConditions(name, parser.m_conditionStack.Peek()) ?? String.Empty;
							Property.Node optionValueNode = AttributeOrContentsNode
							(
								parser, optionElement,
								resolveNode: propertyName =>
								{
									if (propertyName == "option.value")
									{
										return existingOption;
									}
									return null; // null falls back to default implementation
								},
								automaticallyExpand: false
							);
							optionSet.SetOption(name, optionValueNode, parser.m_conditionStack.Peek());
						}
					);
				}
			);
		}

		private static Property.Node AttributeOrContentsNode(Parser parser, XElement element, string attributeName = "value", Func<string, Property.Node> resolveNode = null, bool allowDoElement = false, bool automaticallyExpand = true)
		{
			string valueAttrString = element.Attribute(attributeName)?.Value;

			Property.Node node = null;
			foreach (XNode child in element.Nodes())
			{
				if (child is XText childText) // also handles CDATA
				{
					if (valueAttrString != null)
					{
						throw new FormatException(); // TODO
					}

					ScriptEvaluation.Node scriptNode = ScriptEvaluation.Evaluate(childText.Value);
					Property.Node newNode = parser.ResolveScriptEvaluation(scriptNode, resolveNode);
					node = node != null ? node.AppendLine(newNode) : newNode;
				}
				else if (child is XElement childElement)
				{
					if (!allowDoElement)
					{
						throw new FormatException(); // todo
					}

					if (childElement.Name.LocalName != "do")
					{
						throw new FormatException(); // todo
					}

					if (valueAttrString != null)
					{
						throw new FormatException(); // TODO
					}

					Property.Node newNode = NestedDoContentsHandler(parser, childElement, resolveNode);
					if (newNode != null)
					{
						node = node != null ? node.Append(newNode) : newNode;
					}
				}
				else if (!(child is XComment))
				{
					throw new NotImplementedException(); // todo: good paranoia exception
				}
			}
			if (node is null && valueAttrString != null)
			{
				ScriptEvaluation.Node scriptNode = ScriptEvaluation.Evaluate(valueAttrString);
				node = parser.ResolveScriptEvaluation(scriptNode, resolveNode);
			}
			return automaticallyExpand ? parser.ExpandByCurrentConditions(node) : node; // it's always safe to expand but not optimial, err on the side of expanding by default and allow disabling as optimization
		}

		private static Property.Node NestedDoContentsHandler(Parser parser, XElement doElement, Func<string, Property.Node> resolveNode = null)
		{
			ValidateAttributes(doElement, "if", "unless");
			parser.ProcessSubElements(doElement, AllowNestedDo);

			Property.Node node = null;
			parser.HandleIfUnless
			(
				doElement, () =>
				{
					foreach (XNode child in doElement.Nodes())
					{
						if (child is XText childText) // also handles CDATA
						{
							ScriptEvaluation.Node scriptNode = ScriptEvaluation.Evaluate(childText.Value);
							Property.Node newNode = parser.ResolveScriptEvaluation(scriptNode, resolveNode);
							node = node != null ? node.Append(newNode) : newNode;
						}
						else if (child is XElement childElement)
						{
							if (childElement.Name.LocalName != "do")
							{
								throw new FormatException(); // todo
							}

							Property.Node newNode = NestedDoContentsHandler(parser, childElement, resolveNode);
							if (newNode != null)
							{
								node = node != null ? node.Append(newNode) : newNode;
							}
						}
						else if (!(child is XComment))
						{
							throw new NotImplementedException(); // todo: good paranoia exception
						}
					}
					if (node != null)
					{
						node = parser.ExpandByCurrentConditions(node);
					}
				}
			);
			if (node is null)
			{
				return null;
			}
			return new Property.String(string.Empty).Update(node); // update an empty string to get empty string for the conditions that don't apply
		}

		private static Property.Node PathCombineMultiLineProperty(string rootDir, Property.Node pathsLineProperty)
		{
			IEnumerable<Property.Node> splitIncludes = pathsLineProperty
				.SplitLinesToArray()
				.Select
				(
					splitInclude => splitInclude.Transform
					(
						// TODO: this needs to be much smarter. We can't use platform specific functions (i.e Path.Combine) since
						// we want this to be platform agnostic. Also do we really want to store the the actual package root
						// here? - a "token" that the consumer of this code can decide what they want to replace with might be 
						// better. That said...consumer can also do this themselves if we give them package root? Or perhaps we
						// do it for them BUT when crunching data rather than during parsing? need mo' thonk
						value =>
						{
							value = value.Trim();
							if (value.Length == 0)
							{
								return null; // nulls are skipped in output aggregation
							}
							bool windowsRooted = Char.IsLetter(value.First()) && value.ElementAtOrDefault(1) == ':'; // TODO we need to worry about UNC or \\?\?
							bool unixRooted = value.First() == '/'; // todo - do windows only package start with this?
							if (!windowsRooted && !unixRooted && !value.StartsWith(rootDir, StringComparison.OrdinalIgnoreCase)) // ignore case always, for windows support
							{
								value = PathNormalize(rootDir + '/' + value);
							}
							return value;
						}
					)
				)
				.Where(splitInclude => splitInclude != null);

			Property.Node joinedIncludeDirs = Property.Join(Constants.Newline, splitIncludes);
			return joinedIncludeDirs;
		}

		private static string PathNormalize(string path)
		{
			// TODO: many more cases to deal with
			path = path.Replace('\\', '/');
			while (path.Contains("//"))
			{
				path = path.Replace("//", "/");
			}
			return path;
		}
	}
}