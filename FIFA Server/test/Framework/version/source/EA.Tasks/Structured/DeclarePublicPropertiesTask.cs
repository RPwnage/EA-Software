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
using System.Collections.Concurrent;
using System.Linq;
using System.Xml;

using NAnt.Core;
using NAnt.Core.Attributes;


namespace EA.Eaconfig.Structured
{
	/// <summary>
	/// Public property declaration allows to describe properties thatcan affect/control package build
	/// </summary>
	[TaskName("declare-public-properties")]
	public class DeclarePublicPropertiesTask : Task
	{
		public static readonly ConcurrentDictionary<string, PropertyDeclarationEntry> data = new ConcurrentDictionary<string, PropertyDeclarationEntry>();

		public DeclarePublicPropertiesTask()
		{
			PropertyDeclaration = new PublicPropertyDeclaration(this);
		}

		/// <summary>
		/// Name of the package this property affects.
		/// </summary>
		[TaskAttribute("packagename", Required = false)]
		public string PackageName { set; get; }

		/// <summary>
		/// Public property declaration allows to describe properties
		/// </summary>
		[Property("property")]
		public PublicPropertyDeclaration PropertyDeclaration { get; }

		protected override void ExecuteTask()
		{
		}

		public class PropertyDeclarationEntry
		{
			public readonly string PropertyName;
			public readonly string PropertyDescription;
			public readonly IList<string> Packages;

			public PropertyDeclarationEntry(string name, string description, string package)
			{
				PropertyName = name;
				PropertyDescription = description;
				Packages = new List<string>();
				Packages.Add(package);
			}
		}

		public class PublicPropertyDeclaration : Element
		{
			private readonly DeclarePublicPropertiesTask _declaration;

			internal PublicPropertyDeclaration(DeclarePublicPropertiesTask declaration)
			{
				_declaration = declaration;
			}

			/// <summary>
			/// Name of the property.
			/// </summary>
			[TaskAttribute("name", Required = true)]
			public new string Name { set; get; }

			/// <summary>
			/// Name of the property.
			/// </summary>
			[TaskAttribute("description", Required = false)]
			public string Description { set; get; }

			public override void Initialize(XmlNode elementNode)
			{
				Description = null;
				Name = null;

				base.Initialize(elementNode);
			}

			protected override void InitializeElement(XmlNode elementNode)
			{
				if (Description != null && elementNode.InnerText.Length != 0)
				{
					throw new BuildException("The property declaration description can only appear in the 'description' value attribute or the element body but not both.", Location);
				}

				if (Description == null)
				{
					Description = elementNode.InnerText;
				}

				Description = Project.ExpandProperties(Description).TrimWhiteSpace();

				var package = GetPackageName();

				DeclarePublicPropertiesTask.data.AddOrUpdate(Name, 
					(n)=> 
					{
						return new PropertyDeclarationEntry(Name, Description, package);
					},
					(n,e)=> 
					{
						if (!String.Equals(e.PropertyDescription, Description, StringComparison.InvariantCultureIgnoreCase))
						{
							Log.Warning.WriteLine("{0} Two different descriptions for the same property '{1}'", Location.ToString(), Name);
						}
						else
						{
							e.Packages.Add(package);
						}

						return e;
					});
			}

			private string GetPackageName()
			{
				return _declaration.PackageName ?? Project.GetPropertyValue("package.name");
			}
		}
	}

	[TaskName("print-public-properties")]
	public class PrintPublicPropertiesTask : Task
	{
		protected override void ExecuteTask()
		{
			Log.Status.WriteLine();
			Log.Status.WriteLine("Public Property Declarations:");

			foreach(var pde in DeclarePublicPropertiesTask.data.Values.OrderBy(x=>x.PropertyName))
			{
				PrintPropertyDeclaration(pde);
			}
		}

		private void PrintPropertyDeclaration(DeclarePublicPropertiesTask.PropertyDeclarationEntry pde)
		{
			Log.Status.WriteLine();
			Log.Status.WriteLine("{0}   [{1}]", pde.PropertyName, pde.Packages.Distinct().OrderBy(s=>s).ToString(", "));
			Log.Status.WriteLine();
			foreach (var line in pde.PropertyDescription.LinesToArray())
			{
				Log.Status.WriteLine("      {0}", line);
			}
		}
	}
}

