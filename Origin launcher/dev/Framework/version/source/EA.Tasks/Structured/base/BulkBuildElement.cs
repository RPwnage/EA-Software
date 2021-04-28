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
using NAnt.Core.Structured;

namespace EA.Eaconfig.Structured
{
	/// <summary>Bulkbuild input</summary>
	[ElementName("bulkbuild", StrictAttributeCheck = true, NestedElementsCheck = true)]
	public class BulkBuildElement : ConditionalElementContainer
	{
		public BulkBuildElement()
		{
			// All these attributes are optional.  We don't define the corresponding property in ModuleTask if these attributes
			// haven't been defined.
			_enable = null;
			_partial = null;
			MaxSize = -1;
			_splitByDirectories = null;
			MinSplitFiles = -1;
			DeviateMaxSizeAllowance = -1;
		}

		/// <summary>enable/disable bulkbuild generation.</summary>
		[TaskAttribute("enable")]
		public bool Enable
		{
			get { return _enable??false; }
			set { _enable = value; }
		} internal bool? _enable;

		/// <summary>If partial enabled, source files with custom build settings are excluded from bulkbuild generation and compiled separately.</summary>
		[TaskAttribute("partial")]
		public bool Partial
		{
			get { return _partial??false; }
			set { _partial = value; }
		} internal bool? _partial;

		/// <summary>If 'maxsize' is set, generated bulkbuild files will contain no more than maxsize entries. I.e. They are split in several if number of files exceeds maxsize.</summary>
		[TaskAttribute("maxsize")]
		public int MaxSize { get; set; }

		/// <summary>If 'SplitByDirectories' is set, generated bulkbuild files will be split according to directories (default is false).</summary>
		[TaskAttribute("SplitByDirectories")]
		public bool SplitByDirectories
		{
			get { return _splitByDirectories??false; }
			set { _splitByDirectories = value; }
		} internal bool? _splitByDirectories;

		/// <summary>Use 'MinSplitFiles' to specify minimum files when directory is split into a separate BulkBuild file (only used when SplitByDirectories is turned on).  That is, if current directory has less then the specified minimun files, this directory's files will be merged with the next directory to form a group and then do a split in this group.</summary>
		[TaskAttribute("MinSplitFiles")]
		public int MinSplitFiles { get; set; }

		/// <summary>Use 'DeviateMaxSizeAllowance' to specify a threadhold of number of files we're allowed to deviate from maxsize input.  This parameter
		/// only get used on incremental build where existing bulkbuild files have already been created from previous build.  Purpose of this parameter 
		/// is to allow your development process to not get slowed down by adding/removing files.  Default is set to 5.</summary>
		[TaskAttribute("DeviateMaxSizeAllowance")]
		public int DeviateMaxSizeAllowance { get; set; }

		/// <summary>Files that are not included in the bulkbuild</summary>
		[FileSet("loosefiles", Required = false)]
		public StructuredFileSet LooseFiles { get; } = new StructuredFileSet();

		/// <summary>Groups of sourcefiles to be used to generate bulkbuild files</summary>
		[FileSet("sourcefiles", Required = false)]
		public NamedStructuredFileSets SourceFiles { get; } = new NamedStructuredFileSets();

		/// <summary>Manual bulkbuild files</summary>
		[FileSet("manualsourcefiles", Required = false)]
		public StructuredFileSet ManualSourceFiles { get; } = new StructuredFileSet();
	}
}
