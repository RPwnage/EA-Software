// NAnt - A .NET build tool
// Copyright (C) 2003-2015 Electronic Arts Inc.
// Copyright (C) 2001 Gerry Shaw
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
// Mike Krueger (mike@icsharpcode.net)
// Gerry Shaw (gerry_shaw@yahoo.com)

using System;

using NAnt.Core;
using NAnt.Core.Logging;
using NAnt.Core.Attributes;

using EA.SharpZipLib;

namespace NAnt.ZipTasks
{

	/// <summary>
	/// Creates a zip file from a specified fileset.
	/// </summary>
	/// <remarks>
	///   <para>Uses <a href="http://www.icsharpcode.net/OpenSource/NZipLib/">NZipLib</a>, an open source Zip/GZip library written entirely in C#.</para>
	/// <para>Full zip functionality is not available; all you can do
	/// is to create a zip file from a fileset.</para>
	/// </remarks>
	/// <include file='Examples/Zip/Zip.example' path='example'/>
	[TaskName("zip")]
	public class ZipTask : Task {

		string _zipfile = null;
		string _zipEntryDir = null;

		/// <summary>The zip file to create. Use a qualified name to create the zip file in a 
		/// location which is different than the current working directory.</summary>
		[TaskAttribute("zipfile", Required=true)]
		public string ZipFileName { 
			get { return Project.GetFullPath(_zipfile); } set {_zipfile = value; } 
		}

		/// <summary>Desired level of compression Default is 6.</summary>
		/// <value>0 - 9 (0 - STORE only, 1-9 DEFLATE (1-lowest, 9-highest))</value>
		[TaskAttribute("ziplevel", Required = false), Int32Validator(0, 9)]
		public int ZipLevel { get; set; } = 6;

		/// <summary>Prepends directory to each zip file entry.</summary>
		[TaskAttribute("zipentrydir", Required=false)]
		public string ZipEntryDir { 
			get { 
				if (_zipEntryDir != null && _zipEntryDir.Length > 0)
				{
					// normalize the basedir to use forward slashes (infozip complains if zip files don't)
					_zipEntryDir = _zipEntryDir.Replace('\\', '/');

					if (_zipEntryDir[_zipEntryDir.Length-1] != '/') {
						_zipEntryDir = _zipEntryDir + '/';
					}
				}
				return _zipEntryDir; 
			} 
			set {_zipEntryDir = value; } 
		}

		/// <summary>The set of files to be included in the archive.</summary>
		[FileSet("fileset")]
		public FileSet ZipFileSet { get; } = new FileSet();

		/// <summary>Preserve last modified timestamp of each file.</summary>
		[TaskAttribute("usemodtime", Required = false)]
		public bool UseModTime { get; set; } = false;

		protected void ZipEventCallback(object source, ZipEventArgs e) {
			Log.Debug.WriteLine(LogPrefix + "Zipping {0}", e.ZipEntry.Name);
		}

		protected override void ExecuteTask() {
			try {
				int fileCount = ZipFileSet.FileItems.Count;
				Log.Status.WriteLine(LogPrefix + "Zipping {0} files to {1}", fileCount, ZipFileName);

				ZipLib zipLib = new ZipLib();
				zipLib.ZipEvent += new ZipEventHandler(ZipEventCallback);
				zipLib.ZipFile(ZipFileSet.FileItems.ToStringCollection(), 
					ZipFileSet.BaseDirectory, 
					ZipFileName, 
					ZipEntryDir, 
					ZipLevel,
					UseModTime);
				zipLib.ZipEvent -= new ZipEventHandler(ZipEventCallback);
			}
			catch(System.Exception e) {
				string msg = String.Format("Error creating zip file '{0}'", ZipFileName);
				throw new BuildException(msg, Location, e);
			}
		}
	}
}