// Originally based on NAnt - A .NET build tool
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
// Gerry Shaw (gerry_shaw@yahoo.com)
// Ian MacLean (ian_maclean@another.com)
// Electronic Arts (Frostbite.Team.CM@ea.com)


using NAnt.Core.Attributes;
using NAnt.Core.Util;

namespace NAnt.Core.Tasks
{
	[TaskName("nant-coredump", NestedElementsCheck = true)]
	public class CoreDumpTask : Task
	{
		/// <summary>Write to the log file. Default is false</summary>
		[TaskAttribute("echo")]
		public bool Echo { 
			get { return (_echo); } set { _echo = value; } 
		} private bool _echo = false;

		/// <summary>Dump properties. Default - true.</summary>
		[TaskAttribute("dump-properties")]
		public bool DumpProperties 
		{ 
			get { return (_dumpProperties); } set { _dumpProperties = value; } 
		} private bool _dumpProperties = true;

		/// <summary>Dump fileset names. Default - true.</summary>
		[TaskAttribute("dump-filesets")]
		public bool DumpFilesets 
		{ 
			get { return (_dumpFilesets); } set { _dumpFilesets = value; } 
		} private bool _dumpFilesets = true;

		/// <summary>Dump optionset names</summary>
		[TaskAttribute("dump-optionsets")]
		public bool DumpOptionsets
		{
			get { return (_dumpOptionsets); }
			set { _dumpOptionsets = value; }
		} private bool _dumpOptionsets = true;

		/// <summary>Include content of filesets and optionsets. Default is false</summary>
		[TaskAttribute("expand-all")]
		public bool ExpandAll
		{
			get { return (_expandAll); }
			set { _expandAll = value; }
		} private bool _expandAll = false;


		/// <summary>Dump optionset names</summary>
		[TaskAttribute("tofile")]
		public string ToFile
		{
			get { return (_toFile); }
			set { _toFile = value; }
		} private string _toFile;

		/// <summary>format: text, xml. Default - text</summary>
		[TaskAttribute("format")]
		public CoreDump.Format Format
		{
			get { return (_format); }
			set { _format = value; }
		} private CoreDump.Format _format = CoreDump.Format.Text;



		protected override void ExecuteTask()
		{
			new CoreDump(Project, DumpProperties, DumpFilesets, DumpOptionsets, ExpandAll, Echo? Log : null).Write(ToFile, Format);
		}

	}
}
