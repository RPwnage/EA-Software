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

using System;
using System.Collections.Generic;
using System.Xml;

namespace EA.Eaconfig.Structured
{
	/// <summary></summary>
	[ElementName("StructuredFileSet", StrictAttributeCheck = true)]
	public class StructuredFileSet : FileSet
	{
		public StructuredFileSet() : base() {}

		public StructuredFileSet(StructuredFileSet source) : base(source)
		{
			AppendBase = source.AppendBase;
		}

		public StructuredFileSet(FileSet source)
			: base(source)
		{
		}

		public StructuredFileSet(FileSet source, bool append)
			: base(source)
		{
			AppendBase = append;
		}

		/// <summary>If true, the patterns specified by this task are added to the patterns contained in the fileset. If false, the fileset will only contains the patterns specified in this task. Default is "true".</summary>
		[TaskAttribute("append", Required = false)]
		public bool AppendBase { get; set; } = true;

		/// <summary></summary>
		[TaskAttribute("name", Required = false)]
		public string Suffix { get; set; } = String.Empty;

		[TaskAttribute("optionset", Required = false)] // For documentation
		public string DummyOptionset
		{
			set { }
		}

	}

	/// <summary>Sometimes it's useful to be able to access each fileset defined as a collection rather than
    /// appending from multiple definitions into a single fileset. Gives the facade of a fileset so 
    /// the XML declaration can be the same (minus properties which don't make sense for structured.</summary>
	[ElementName("StructuredFileSetCollection", StrictAttributeCheck = true)]
	public class StructuredFileSetCollection : Element
	{
		bool _ifDefined = true;
		bool _unlessDefined = false;

		public StructuredFileSetCollection() : base() { }

		public readonly List<StructuredFileSet> FileSets = new List<StructuredFileSet>();

		/// <summary>If true then the task will be executed; otherwise skipped. Default is "true".</summary>
		[TaskAttribute("if")]
		public virtual bool IfDefined
		{
			get { return _ifDefined; }
			set { _ifDefined = value; }
		}

		/// <summary>Opposite of if.  If false then the task will be executed; otherwise skipped. Default is "false".</summary>
		[TaskAttribute("unless")]
		public virtual bool UnlessDefined
		{
			get { return _unlessDefined; }
			set { _unlessDefined = value; }
		}

		// Define FileSet attributes as dummy attributes to avoid complaints about unknown attributes
		[TaskAttribute("basedir", Required = false)]
		public string BaseDir
		{
			set {}
		}

		// Define FileSet attributes as dummy attributes to avoid complaints about unknown attributes
		[TaskAttribute("fromfileset", Required = false)]
		public string FromFileset
		{
			set {}
		}

        // Define FileSet attributes as dummy attributes to avoid complaints about unknown attributes
        [TaskAttribute("failonmissing")]
        public bool FailOnMissingFile
        {
			set {}
        }

        // Define FileSet attributes as dummy attributes to avoid complaints about unknown attributes
		[TaskAttribute("name", Required=false)]
		public string Suffix
		{
			set {}
		}

        // Define FileSet attributes as dummy attributes to avoid complaints about unknown attributes
		[TaskAttribute("append", Required=false)]
		public bool AppendBase
		{
			set {}
		}

        [XmlElement("includes", "NAnt.Core.FileSet+IncludesElement", AllowMultiple = true)]
		[XmlElement("excludes", "NAnt.Core.FileSet+ExcludesElement", AllowMultiple = true)]
		[XmlElement("do", Container = XmlElementAttribute.ContainerType.ConditionalContainer, AllowMultiple = true)]
		[XmlElement("group", Container = XmlElementAttribute.ContainerType.ConditionalContainer, AllowMultiple = true)]
		public override void Initialize(XmlNode elementNode)
		{
			_ifDefined = true;
			_unlessDefined = false;
			base.Initialize(elementNode);
		}

		protected override void InitializeElement(XmlNode elementNode)
		{
			if (IfDefined && !UnlessDefined)
			{
				StructuredFileSet fs = new StructuredFileSet();
				fs.Project = Project;
				fs.Initialize(elementNode);

				for (int idx=0; idx<fs.Includes.Count; ++idx)
				{
					FileSetItem inc = fs.Includes[idx];
					if (!String.IsNullOrEmpty(inc.OptionSet) && 
						NAnt.Core.Functions.OptionSetFunctions.OptionSetGetValue(Project,inc.OptionSet,"preserve-basedir") == "on")
					{
						// We really just want to reset PreserveBaseDirValue to true, But "Pattern" is read only.  So we have to 
						// re-construct this FileSetItem object.  
						// We need this set to true so that fs.FileItems (ie the fs.Includes.GetMatchingItems() call) will pass down the
						// BaseDirectory information.
						if (inc.Pattern.PreserveBaseDirValue == false)
						{
							string incBaseDir = inc.Pattern.BaseDirectory;
							if (String.IsNullOrEmpty(incBaseDir))
							{
								if (!String.IsNullOrEmpty(fs.BaseDirectory))
								{
									incBaseDir = fs.BaseDirectory;
								}
								else
								{
									throw new BuildException(String.Format("The fileset '{0}' has 'include' element with optionset that turned on option 'preserve-basedir' but basedir information is not explicitly provided in either the 'fileset' or the 'include' element!", elementNode.Name), Location);
								}
							}

							NAnt.Core.Util.Pattern newPattern = NAnt.Core.Util.PatternFactory.Instance.CreatePattern(inc.Pattern.Value, incBaseDir, inc.AsIs);
							newPattern.PreserveBaseDirValue = true;
							newPattern.FailOnMissingFile = inc.Pattern.FailOnMissingFile;
							FileSetItem newInc = new FileSetItem(newPattern, inc.OptionSet, inc.AsIs, inc.Force, inc.Data);
							fs.Includes[idx] = newInc;
						}
					}
				}

				FileSets.Add(fs);
			}
		}
	}
}
