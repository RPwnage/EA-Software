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
// Electronic Arts (Frostbite.Team.CM@ea.com)
// Kosta Arvanitis (karvanitis@ea.com)

using System;

using NAnt.Core;
using NAnt.Core.Logging;
using NAnt.Core.Attributes;

using EA.SharpZipLib;

namespace NAnt.ZipTasks
{

	/// <summary>
	/// Unzip the contents of a zip file to a specified directory.
	/// </summary>
	/// <include file='Examples/UnZip/UnZip.example' path='example'/>
	[TaskName("unzip")]
	public class UnZipTask : Task {

		string _zipfile = null;

		/// <summary>The full path to the zip file.</summary>
		[TaskAttribute("zipfile", Required=true)]
		public string ZipFileName { 
			get { return Project.GetFullPath(_zipfile); } 
			set {_zipfile = value; } 
		}

		/// <summary>The full path to the destination folder.</summary>
		[TaskAttribute("outdir", Required = true)]
		public string OutDir { get; set; } = null;

		/// <summary>
		/// If zip file contains entry with symlink, preserve the symlink if possible (default true).
		/// </summary>
		[TaskAttribute("preserve_symlink", Required = false)]
		public bool PreserveSymlink { get; set; } = false;

		protected void ZipEventCallback(object source, ZipEventArgs e) {
			Log.Debug.WriteLine(LogPrefix + "UnZipping {0}", e.ZipEntry.Name);
		}
		
		protected override void ExecuteTask() {
			try {
				Log.Status.WriteLine(LogPrefix + "Unzipping {0} to {1}", ZipFileName, OutDir);
				
				ZipLib zipLib = new ZipLib();
				zipLib.ZipEvent += new ZipEventHandler(ZipEventCallback);
				zipLib.UnzipFile(ZipFileName, OutDir, PreserveSymlink);
				zipLib.ZipEvent -= new ZipEventHandler(ZipEventCallback);
			} 
			catch(System.Exception e) {
				string msg = String.Format("Error extracting zip file '{0}'", ZipFileName);
				throw new BuildException(msg, Location, e);
			}
		}
   }
}
