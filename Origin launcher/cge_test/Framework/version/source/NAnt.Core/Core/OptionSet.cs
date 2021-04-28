// NAnt - A .NET build tool
// Copyright (C) 2003-2015 Electronic Arts Inc.
// Copyright (C) 2001-2003 Gerry Shaw
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
// 
// 
// File Maintainer:
// Electronic Arts (Frostbite.Team.CM@ea.com)
// Tomas Restrepo (tomasr@mvps.org)

using System;
using System.Xml;
using System.Collections.Generic;
using System.Collections.Concurrent;
using System.Linq;

using NAnt.Core.Attributes;
using NAnt.Core.Reflection;
using NAnt.Core.Logging;
using NAnt.Core.Threading;

namespace NAnt.Core
{

	/// <summary>
	/// Provides a dictionary of named option sets.  This is used by NAnt and the
	/// <see cref="NAnt.Core.Tasks.OptionSetTask"/> to create named option sets.
	/// </summary>
	public class OptionSetDictionary : ConcurrentDictionary<string, OptionSet>
	{
		private static HashSet<string> s_traceOptionSets = null;

		internal OptionSetDictionary()
			: base()
		{
		}

		internal OptionSetDictionary(ConcurrentDictionary<string, OptionSet> collection)
			: base(collection)
		{
		}

		internal OptionSetDictionary(OptionSetDictionary other, bool clone)
		{
			foreach (var os in other)
			{
				if (clone)
				{
					Add(os.Key, new OptionSet(os.Value, true));
				}
				else
				{
					this[os.Key] = os.Value;
				}
			}
		}

		public bool Contains(string name)
		{
			return ContainsKey(name);
		}

		public bool Add(string name, OptionSet set)
		{
			return TryAdd(name, set);
		}

		public bool Remove(string name)
		{
			OptionSet set;
			return TryRemove(name, out set);
		}

		public new OptionSet this[string name]
		{
			get
			{
				OptionSet os;
				if (!TryGetValue(name, out os))
				{
					os = null;
				}
				return os;
			}
			set
			{
				base[name] = value;
			}
		}

		public static void SetTraceOptionsets(IEnumerable<string> traceOptionSetNames)
		{
			if (traceOptionSetNames.Any())
			{
				s_traceOptionSets = new HashSet<string>(traceOptionSetNames.Select(x => x.ToLower()));
			}
			else
			{
				s_traceOptionSets = null;
			}
		}

		internal static bool IsTraceOptionSet(string optionSetName)
		{
			return s_traceOptionSets?.Contains(optionSetName) ?? false;
		}
	}

	/// <summary>
	/// Provides a dictionary of options.  An <see cref="OptionSet"/> contains
	/// a dictionary of named string options.
	/// </summary>
	public class OptionDictionary : Dictionary<string, string>
	{
		public OptionDictionary() : base()
		{
		}

		public OptionDictionary(IDictionary<string, string> other)
			: base(other)
		{
		}

		new public string this[string name]
		{
			get
			{
				if (!TryGetValue(name, out string val))
				{
					val = null;
				}
				return val;
			}
			set
			{
				base[name] = value;
			}
		}

		public bool Contains(string name)
		{
			return ContainsKey(name);
		}
	}

	/// <summary>Manages a set of options as a name/value collection.</summary>
	public class OptionSet : Element
	{
		private string localValue;

		public OptionSet()
		{
			Options = new OptionDictionary();
		}

		public OptionSet(OptionSet other)
		{
			if(other != null )
			{
				Options = new OptionDictionary(other.Options);
			} 
			else
			{
				Options = new OptionDictionary();
			}
		}

		public OptionSet(OptionSet other, bool clone)
		{
			if (other != null)
			{
				FromOptionSetName = other.FromOptionSetName;
				localValue = other.localValue;
				Options = new OptionDictionary(other.Options);
			}
			else
			{
				Options = new OptionDictionary();
			}
		}

		public bool ContentEquals(OptionSet other)
		{
			if ((Options == null && other.Options != null) || (Options != null && other.Options == null))
				return false;

			if ((FromOptionSetName == null && other.FromOptionSetName != null) || (FromOptionSetName != null && other.FromOptionSetName == null))
				return false;

			if ((localValue == null && other.localValue != null) || (localValue != null && other.localValue == null))
				return false;

			if (FromOptionSetName != null && !FromOptionSetName.Equals(other.FromOptionSetName))
				return false;

			if (localValue != null && !localValue.Equals(other.localValue))
				return false;

			if (Options != null && !this.Options.OrderBy(kvp => kvp.Key).SequenceEqual(other.Options.OrderBy(kvp => kvp.Key)))
				return false;

			return true;
		}

		/// <summary>The dictionary used to map option names to option values.</summary>
		public OptionDictionary Options { get; }

		/// <summary>The name of an option set to include.</summary>
		[TaskAttribute("fromoptionset", Required = false)]
		public string FromOptionSetName { get; set; }

		/// <summary>Optimization. Directly initialize</summary>
		[XmlElement("option", "NAnt.Core.OptionSet+Option", AllowMultiple = true)]
		public override void Initialize(XmlNode elementNode)
		{
			if (Project == null)
			{
				throw new InvalidOperationException("OptionSet Element has invalid (null) Project property.");
			}

			// Save position in build file for reporting useful error messages.
			if (!String.IsNullOrEmpty(elementNode.BaseURI))
			{
				try
				{
					Location = Location.GetLocationFromNode(elementNode);
				}
				catch (ArgumentException ae)
				{
					Log.Warning.WriteLine("Can't find node '{0}' location, file: '{1}'{2}{3}", elementNode.Name, elementNode.BaseURI, Environment.NewLine, ae.ToString());
				}

			}
			try
			{
				foreach (XmlAttribute attr in elementNode.Attributes)
				{
					switch (attr.Name)
					{
						case "fromoptionset":
							FromOptionSetName = attr.Value;
							break;
						default:
							string msg = String.Format("Unknown attribute '{0}'='{1}' in OptionSet element", attr.Name, attr.Value);
							throw new BuildException(msg, Location);
					}
				}

				SetOptions(elementNode, FromOptionSetName);

			}
			catch (Exception ex)
			{
				throw new ContextCarryingException(ex, Name, Location);
			}
		}


		/// <summary>Add all the child option elements.</summary>
		/// <param name="elementNode">XML node to initialize from.</param>
		protected override void InitializeElement(XmlNode elementNode)
		{
			SetOptions(elementNode, FromOptionSetName);
		}

		public void Append(OptionSet optionSet)
		{
			if(!Object.ReferenceEquals(this,optionSet))
			{
				foreach (var entry in optionSet.Options)
				{
					Options[entry.Key] = entry.Value;
				}
			}
		}

		internal void SetOptions(XmlNode elementNode, string fromOptionSetName)
		{
			if (fromOptionSetName != null)
			{
				OptionSet fromSet;
				if(!Project.NamedOptionSets.TryGetValue(fromOptionSetName, out fromSet))
				{
					string msg = String.Format("Unknown named option set '{0}'.", fromOptionSetName);
					throw new BuildException(msg, Location.GetLocationFromNode(elementNode));
				}
				Append(fromSet);
			}

			foreach (XmlNode node in elementNode)
			{
				if (node.NodeType == XmlNodeType.Element)
				{
					if (node.Name == "option")
					{
						ParseOption(node);
					}
					else
					{
						string msg = String.Format("Illegal element '{0}' inside optionset element.", node.Name);
						throw new BuildException(msg, Location.GetLocationFromNode(node));
					}
				}
			}
		}


		protected bool IsAcceptedOptionBodyNodeType( XmlNode node)
		{
			return (node.NodeType == XmlNodeType.Text ||
					node.NodeType == XmlNodeType.Comment ||
					node.NodeType == XmlNodeType.Whitespace ||
					node.NodeType == XmlNodeType.SignificantWhitespace ||
					node.NodeType == XmlNodeType.CDATA);
		}

		protected void ParseOption(XmlNode elementNode)
		{
			string name = null;
			string value = null;
			bool   ifDefined = true;
			bool   unlessDefined = false;

			try
			{
				foreach (XmlAttribute attr in elementNode.Attributes)
				{
					switch (attr.Name)
					{
						case "name": 
							name = attr.Value; 
							break;
						case "value":  
							value = attr.Value;
							break;
						case "if":
							ifDefined = ElementInitHelper.InitBoolAttribute(this, attr.Name, attr.Value);
							break;
						case "unless":
							unlessDefined = ElementInitHelper.InitBoolAttribute(this, attr.Name, attr.Value);
							break;
						default:
							string msg = String.Format("Unknown attribute '{0}'='{1}' in <option> element", attr.Name, attr.Value);
							throw new BuildException(msg, Location.GetLocationFromNode(elementNode));
					}
				}

				if(ifDefined && !unlessDefined)
				{
					if(name == null)
					{
						throw new BuildException("Missing required <option> attribute 'name'", Location.GetLocationFromNode(elementNode));
					}

					name = Project.ExpandProperties(name);

					localValue = Options[name];
					if (localValue == null)
						localValue = String.Empty;

					// Just defer for now so that everything just works
					if (value != null && elementNode.InnerText.Length != 0)
					{
						throw new BuildException("The option value can only appear in the value attribute or the element body.", Location.GetLocationFromNode(elementNode));
					}
					if (value == null)
					{
						if (elementNode.HasChildNodes)
						{
							System.Text.StringBuilder innerTextValue = new System.Text.StringBuilder();
							foreach (XmlNode childNode in elementNode.ChildNodes)
							{
								if (IsAcceptedOptionBodyNodeType(childNode))
								{
									if (childNode.NodeType != XmlNodeType.Comment && !String.IsNullOrEmpty(childNode.InnerText))
									{
										innerTextValue.Append(childNode.InnerText);
									}
								}
								// Special implementation for <do> block.
								else if (childNode.NodeType == XmlNodeType.Element && childNode.Name == "do")
								{
									bool do_if_defined = true;
									bool do_unless_defined = false;
									foreach (XmlAttribute childNodeAttr in childNode.Attributes)
									{
										switch (childNodeAttr.Name)
										{
											case "if":
												do_if_defined = ElementInitHelper.InitBoolAttribute(this, childNodeAttr.Name, childNodeAttr.Value);
												break;
											case "unless":
												do_unless_defined = ElementInitHelper.InitBoolAttribute(this, childNodeAttr.Name, childNodeAttr.Value);
												break;
											default:
												throw new BuildException(string.Format("Unknown <do> attribute '{0}'='{1}' in <option> element body!", childNodeAttr.Name, childNodeAttr.Value), Location.GetLocationFromNode(elementNode));
										}
									}
									if (do_if_defined && !do_unless_defined)
									{
										// Do some error check to make sure this child node contains text and comments only and nothing else.
										if (childNode.ChildNodes != null)
										{
											foreach (XmlNode nestedChild in childNode.ChildNodes)
											{
												if (!IsAcceptedOptionBodyNodeType(nestedChild))
												{
													throw new BuildException("The <do> element inside the <option> element body cannot contain anything other than text or comment block.", Location.GetLocationFromNode(elementNode));
												}
											}
										}
										innerTextValue.Append(childNode.InnerText);
									}
								}
								else
								{
									throw new BuildException("The option element body only support either text, <do> condition block or XML comment statement.  Cannot contain any other scripts.", Location.GetLocationFromNode(elementNode));
								}
							}
							value = innerTextValue.ToString();
						}
						else
						{
							value = elementNode.InnerText;
						}
					}
					value = StringParser.ExpandString
					(
						value,
						new StringParser.PropertyEvaluator(EvaluateParameter),
						new StringParser.FunctionEvaluator(Properties.EvaluateFunction),
						new StringParser.FindClosePropertyMatches(p => new string[] { })
					);

					Options[name] = value;

					localValue = null;
				}

			}
			catch (BuildException ex)
			{
				throw new ContextCarryingException(ex, Name, Location);
			}
		}

		private string EvaluateParameter(string name, Stack<string> evaluationStack)
		{
			if (name == "option.value")
			{
				return localValue;
			}

			return Properties.EvaluateParameter(name, evaluationStack);
		}

		// This class exists for documentation purposes only
		[ElementName("option", Mixed=true, StrictAttributeCheck=true)]
		private class Option : ConditionalElement
		{
			/// <summary>The name of the option.</summary>
			[TaskAttribute("name", Required=true)]
			public string _name { set {  } }

			/// <summary>The value of the option. Value can be set as option element text. 
			/// To get current value use ${option.value} expression.
			/// </summary>
			[TaskAttribute("value", Required=false)]
			public string _val  {  set { } }
		}

	}
}