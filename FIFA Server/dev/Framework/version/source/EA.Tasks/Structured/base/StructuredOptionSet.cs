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

using NAnt.Core;
using NAnt.Core.Attributes;
using NAnt.Core.Reflection;
using NAnt.Core.Threading;

using System;
using System.Xml;

namespace EA.Eaconfig.Structured
{
	/// <summary></summary>
	[ElementName("StructuredOptionSet", StrictAttributeCheck = true)]
	public class StructuredOptionSet : OptionSet
	{
		public StructuredOptionSet() : base() { }

		public StructuredOptionSet(StructuredOptionSet source)
		{
			AppendBase = source.AppendBase;
		}

		public StructuredOptionSet(OptionSet source)
		{
			base.Append(source);
		}

		public StructuredOptionSet(OptionSet source, bool append)
			:   this(source)
		{
			AppendBase = append;
		}

		[TaskAttribute("append", Required = false)]
		public bool AppendBase { get; set; } = true;

		/// <summary>Optimization. Directly initialize</summary>
		public override void Initialize(XmlNode elementNode)
		{
			if (Project == null)
			{
				throw new InvalidOperationException("OptionSet Element has invalid (null) Project property.");
			}

			// Save position in buildfile for reporting useful error messages.
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
						case "append":
							AppendBase = ElementInitHelper.InitBoolAttribute(this, attr.Name, attr.Value);
							break;

						default:
							string msg = String.Format("Unknown attribute '{0}'='{1}' in OptionSet element", attr.Name, attr.Value);
							throw new BuildException(msg, Location);
					}
				}

				InitializeElement(elementNode);

			}
			catch (Exception ex)
			{
				throw new ContextCarryingException(ex, Name, Location);
			}
		}

	}

}
