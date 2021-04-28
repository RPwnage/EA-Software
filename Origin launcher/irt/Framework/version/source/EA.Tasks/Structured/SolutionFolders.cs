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
using System.Linq;
using System.Text;
using System.Xml;

using NAnt.Core;
using NAnt.Core.Attributes;
using NAnt.Core.Structured;
using NAnt.Core.Util;

namespace EA.Eaconfig.Structured
{
	/// <summary>Define folders for Visual Studio Solutions </summary>
	[TaskName("SolutionFolders", NestedElementsCheck = true)]
	internal class SolutionFolders : ConditionalTaskContainer, IElementAppend
	{
	   internal  const bool DEFAULT_APPEND_VALUE = true;

		public SolutionFolders() : base()
		{
			Folders = new SolutionFolderCollection(this);
		}

		/// <summary>Append this definition to existing definition. Default: 'true'.</summary>
		[TaskAttribute("append")]
		public virtual bool Append
		{
			get { return _append; }
			set { _append = value; }
		} private bool _append = true;

		/// <summary>Folders.</summary>
		[BuildElement("folder", Required = false)]
		public SolutionFolderCollection Folders { get; set; }

		protected override void ExecuteTask()
		{
			base.ExecuteTask();

			SetFolderDefinitions();
		}

		private void SetFolderDefinitions()
		{
			var folderlist = new StringBuilder();
			foreach(var folder in Folders.FolderCollection.OrderBy(f => f.FolderName))
			{
				var path = new Stack<string>();

				TraverseTree(path, folder, folderlist);
			}

			var folders = folderlist.ToString().TrimWhiteSpace();

			SetProperty("solution.folders", folderlist.ToString(), Append);
		}

		private void TraverseTree(Stack<string> path, SolutionFolder folder, StringBuilder folderlist)
		{
			path.Push(folder.FolderName);

			SetEntry(path.Reverse(), folder, folderlist);

			foreach (var child in folder.Folders.FolderCollection.OrderBy(f => f.FolderName))
			{
				TraverseTree(path, child, folderlist);
			}

			path.Pop();
		}

		private void SetEntry(IEnumerable<string> path, SolutionFolder entry, StringBuilder folderlist)
		{
			// Set Folder Items
			var filesetName = String.Format("solution.folder.{0}", path.ToString("."));
			SetFileSet(filesetName, entry.Items, entry.Items.AppendBase);

			// Set Folder Projects
			var folderProjectsName = String.Format("solution.folder.packages.{0}", path.ToString("."));
			SetProperty(folderProjectsName, entry.Projects, entry.Projects.Append);

			if (entry.Items.Includes.Count > 0
				|| !String.IsNullOrEmpty(entry.Projects.Value.TrimWhiteSpace())
				|| entry.Folders.FolderCollection.Count > 0)
			{
				folderlist.AppendLine(path.ToString("/"));
			}
		}

		private void SetProperty(string name, ConditionalPropertyElement value, bool append)
		{
			if (value != null)
			{
				SetProperty(name, value.Value, append);
			}
		}

		private void SetProperty(string name, string value, bool append)
		{
			var sval = value.TrimWhiteSpace();

			if (!String.IsNullOrEmpty(sval))
			{
				sval = Project.ExpandProperties(sval);

				if (append)
				{
					string oldVal = Project.Properties[name];
					if (!String.IsNullOrEmpty(oldVal))
					{
						sval = oldVal + Environment.NewLine + sval;
					}
				}
				Project.Properties[name] = sval;
			}
		}

		private void SetFileSet(string name, FileSet fileSet, bool append)
		{
			if(fileSet != null && (fileSet.Includes.Count > 0 || fileSet.Excludes.Count > 0))
			{
				if(append)
				{ 
					var oldfs = FileSetUtil.GetFileSet(Project, name);
					if (oldfs != null)
					{
						oldfs.Include(fileSet, fileSet.BaseDirectory);
						return;
					}
				}
				Project.NamedFileSets[name] = fileSet;
			}
		}
	}

	[ElementName("folder", StrictAttributeCheck = true)]
	internal class SolutionFolder : ConditionalElementContainer, IElementAppend
	{
		internal SolutionFolder(bool append)
			: base()
		{
			_append = append;
			Items.AppendBase = append;
			Projects.Append = append;
			Folders = new SolutionFolderCollection(this);
		}

		/// <summary>Append this definition to existing definition. Default: inherits from 'SolutionFolders' element' or parent folder</summary>
		[TaskAttribute("append")]
		public virtual bool Append
		{
			get { return _append; }
			set { _append = value; }
		} private bool _append;

		/// <summary>The name of the folder.</summary>
		[TaskAttribute("name", Required = true)]
		public string FolderName { get; set; }

		/// <summary>Items (files) to put in this folder</summary>
		[FileSet("items", Required = false)]
		public StructuredFileSet Items { get; } = new StructuredFileSet();

		/// <summary>Packages and modules to include in the solution folders. Syntax is similar to dependency declarations.</summary>
		[Property("projects", Required = false)]
		public ConditionalPropertyElement Projects { get; set; } = new ConditionalPropertyElement(append: false);

		/// <summary>Subfolders.</summary>
		[BuildElement("folder", Required = false)]
		public SolutionFolderCollection Folders { get; set; }
	}

	public class SolutionFolderCollection : ConditionalElement
	{
		internal readonly List<SolutionFolder> FolderCollection = new List<SolutionFolder>();

		public override void Initialize(XmlNode elementNode)
		{
			IfDefined = true;
			UnlessDefined = false;
			base.Initialize(elementNode);
		}

		internal SolutionFolderCollection(IElementAppend parent)
			: base()
		{
			_parent = parent;
		}

		private IElementAppend _parent;

		/// <summary>Disabling warning about multiple folder elements, it is an incorrect warning in this case because it is parsed by the initialize element method rather than purely through reflection</summary>
		public override bool WarnOnMultiple
		{
			get { return false; }
		}

		/// <summary>Append this definition to existing definition. Default: inherits from 'SolutionFolders' element'</summary>
		[TaskAttribute("append")]
		public virtual bool Append
		{
			get { return _append??_parent.Append; }
			set { _append = value; }
		} private bool? _append = null;


		/// <summary>Items (files) to put in this folder</summary>
		[TaskAttribute("name", Required = true)]
		public string DummyName
		{
			get { return String.Empty; }
			set {}
		}

		[XmlElement("items", "NAnt.Core.FileSet", AllowMultiple = false, Description = "Items (files) to put in this folder.")]
		[XmlElement("projects", "EA.Eaconfig.Structured.ConditionalPropertyElement", AllowMultiple = false, Description="Packages and modules to include in the solution folders. Syntax is similar to dependency declarations.")]
		[XmlElement("folder", "EA.Eaconfig.Structured.SolutionFolderCollection", AllowMultiple = true, Description="Subfolders.")]
		protected override void InitializeElement(XmlNode elementNode)
		{
			if (IfDefined && !UnlessDefined)
			{
				SolutionFolder folder = new SolutionFolder(Append);
				folder.Project = Project;
				folder.Initialize(elementNode);
				FolderCollection.Add(folder);
			}
		}
	}

	internal interface IElementAppend
	{
		bool Append { get; }
	}
}
