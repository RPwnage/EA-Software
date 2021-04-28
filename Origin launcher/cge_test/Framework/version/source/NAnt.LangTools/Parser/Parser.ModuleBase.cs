using System;
using System.Collections.Generic;
using System.Xml.Linq;

namespace NAnt.LangTools
{
	public partial class Parser
	{
		internal abstract class ModuleBase
		{
			internal sealed class DependenciesElement
			{
				internal Property.Node AutoDependencies = null;
				internal Property.Node AutoInterfaceDependencies = null;
				internal Property.Node AutoLinkDependencies = null;
				internal Property.Node BuildDependencies = null;
				internal Property.Node BuildInterfaceDependencies = null;
				internal Property.Node BuildOnlyDependencies = null;
				internal Property.Node InterfaceDependencies = null;
				internal Property.Node LinkDependencies = null;
				internal Property.Node UseDependencies = null;
				internal Property.Node UseLinkDependencies = null;

				internal void InitOrUpdateFromElement(Parser parser, XElement dependenciesElement)
				{
					ValidateAttributes(dependenciesElement);

					Property.Node discard = null; // TODO we waste time computing this and not using it - could probably be more efficient
					parser.HandleIfUnless
					(
						dependenciesElement, () =>
						{
							parser.ProcessSubElements
							(
								dependenciesElement,
								new Dictionary<string, TaskHandler>
								{
									{
										"auto", (innerParser, element) => ModuleDependencyElementHandler
										(
											innerParser, element,
											interfaceLink: ref AutoDependencies,
											interfaceNoLink: ref AutoInterfaceDependencies,
											noInterfaceLink: ref AutoLinkDependencies,
											noInterfaceNoLink: ref discard
										)
									},
									{
										"use", (innerParser, element) => ModuleDependencyElementHandler
										(
											innerParser, element,
											interfaceLink: ref UseDependencies,
											interfaceNoLink: ref InterfaceDependencies,
											noInterfaceLink: ref UseLinkDependencies,
											noInterfaceNoLink: ref discard
										)
									},
									{
										"build", (innerParser, element) => ModuleDependencyElementHandler
										(
											innerParser, element,
											interfaceLink: ref BuildDependencies,
											interfaceNoLink: ref BuildInterfaceDependencies,
											noInterfaceLink: ref LinkDependencies,
											noInterfaceNoLink: ref BuildOnlyDependencies
										)
									},
									{
										"interface", (innerParser, element) => ModuleDependencyElementHandler
										(
											innerParser, element,
											interfaceLink: ref discard, // if this looks wrong it's because it is - but it's also wrong in Framework so bug is replicated here
											interfaceNoLink: ref InterfaceDependencies,
											noInterfaceLink: ref UseLinkDependencies,
											noInterfaceNoLink: ref discard,
											linkDefault: false
										)
									},
									{
										"link", (innerParser, element) => ModuleDependencyElementHandler
										(
											innerParser, element,
											interfaceLink: ref BuildDependencies,
											interfaceNoLink: ref BuildInterfaceDependencies,
											noInterfaceLink: ref LinkDependencies,
											noInterfaceNoLink: ref BuildOnlyDependencies,
											interfaceDefault: false
										)
									}
								}
							);
						}
					);
				}

				private static void ModuleDependencyElementHandler(Parser parser, XElement moduleDependencyElement, ref Property.Node interfaceLink, ref Property.Node interfaceNoLink, ref Property.Node noInterfaceLink, ref Property.Node noInterfaceNoLink, bool interfaceDefault = true, bool linkDefault = true)
				{
					ValidateAttributes(moduleDependencyElement, "if", "unless"/*, "copylocal", "internal"*/, "interface", "link", "value");

					// we can't capture ref parameters in lambas so copy refs here and reassign when we're done
					Property.Node interfaceLinkLocal = interfaceLink;
					Property.Node interfaceNoLinkLocal = interfaceNoLink;
					Property.Node noInterfaceLinkLocal = noInterfaceLink;
					Property.Node noInterfaceNoLinkLocal = noInterfaceNoLink;

					parser.HandleIfUnless
					(
						moduleDependencyElement, () =>
						{
							parser.ProcessSubElements(moduleDependencyElement, AllowNestedDo);
							parser.ResolveAttributes
							(
								moduleDependencyElement, "interface", "link",
								(interfaceAttr, linkAttr) =>
								{
									bool isInterface = false;
									if (interfaceAttr is null)
									{
										isInterface = interfaceDefault;
									}
									else
									{
										if (!Boolean.TryParse(interfaceAttr, out isInterface))
										{
											throw new FormatException(); // todo
										}
									}

									bool isLink = false;
									if (linkAttr is null)
									{
										isLink = linkDefault;
									}
									else
									{
										if (!Boolean.TryParse(linkAttr, out isLink))
										{
											throw new FormatException(); // todo
										}
									}

									Property.Node newValue = AttributeOrContentsNode(parser, moduleDependencyElement, allowDoElement: true);
									if (isLink && isInterface)
									{
										interfaceLinkLocal = interfaceLinkLocal != null ? interfaceLinkLocal.AppendLine(newValue) : newValue;
									}
									else if (isInterface)
									{
										interfaceNoLinkLocal = interfaceNoLinkLocal != null ? interfaceNoLinkLocal.AppendLine(newValue) : newValue;
									}
									else if (isLink)
									{
										noInterfaceLinkLocal = noInterfaceLinkLocal != null ? noInterfaceLinkLocal.AppendLine(newValue) : newValue;
									}
									else
									{
										noInterfaceNoLinkLocal = noInterfaceNoLinkLocal != null ? noInterfaceNoLinkLocal.AppendLine(newValue) : newValue;
									}
								}
							);
						}
					);

					interfaceLink = interfaceLinkLocal;
					interfaceNoLink = interfaceNoLinkLocal;
					noInterfaceLink = noInterfaceLinkLocal;
					noInterfaceNoLink = noInterfaceNoLinkLocal;
				}
			}

			internal readonly DependenciesElement Dependencies = new DependenciesElement();

			protected virtual Dictionary<string, TaskHandler> GetInitOrUpdateTasks()
			{
				return new Dictionary<string, TaskHandler>
				{
					{ "dependencies", Dependencies.InitOrUpdateFromElement },
				};
			}
		}
	}
}