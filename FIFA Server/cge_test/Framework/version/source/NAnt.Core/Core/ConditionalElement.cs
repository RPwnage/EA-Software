// NAnt - A .NET build tool
// Copyright (C) 2003-2015 Electronic Arts Inc.
// Copyright (C) 2001-2002 Gerry Shaw
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
// Electronic Arts (Frostbite.Team.CM@ea.com)
// Kosta Arvanitis (karvanitis@ea.com)

using System.Xml;
using NAnt.Core.Reflection;
using NAnt.Core.Attributes;

namespace NAnt.Core
{
	/// <summary>Models a Conditional NAnt XML element in the build file.</summary>
	public abstract class ConditionalElement : Element
	{
		/// <summary>If true then the target will be executed; otherwise skipped. Default is "true".</summary>
		[TaskAttribute("if")]
		public virtual bool IfDefined { get; set; } = true;

		/// <summary>Opposite of if.  If false then the target will be executed; otherwise skipped. Default is "false".</summary>
		[TaskAttribute("unless")]
		public virtual bool UnlessDefined { get; set; } = false;

		// overriding initializeElement so that it does not try to warn about multiple initializations when 'if' is used
		protected override void InitializeElement(XmlNode elementNode) { }

		public override void Initialize(XmlNode elementNode)
		{
			base.Initialize(elementNode); // calling base initialize method so the 'if' condition is evaluated

			if (IfDefined && !UnlessDefined)
			{
				WarnOnMultipleInitialization(elementNode);
			}
		}

		protected bool OptimizedConditionalElementInit(string name, string value)
		{
			bool ret = true;
			switch (name)
			{
				case "if":
					IfDefined = ElementInitHelper.InitBoolAttribute(this, name, value);
					break;
				case "unless":
					UnlessDefined = ElementInitHelper.InitBoolAttribute(this, name, value);
					break;
				default:
					ret = false;
					break;
			}
			return ret;
		}
	}
}