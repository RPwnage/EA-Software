// NAnt - A .NET build tool
// Copyright (C) 2001 Gerry Shaw
//
// Kosta Arvanitis (karvanitis@ea.com)

using System;
using System.IO;

using NAnt.Core;
using NAnt.Core.Logging;
using NAnt.Core.Util;
using NAnt.Core.Attributes;

using EA.SharpZipLib;

namespace NAnt.ZipTasks {
    
    /// <summary>
    /// Unzip the contents of a zip file to a specified directory.
    /// </summary>
    /// <include file='Examples/UnZip/UnZip.example' path='example'/>
    [TaskName("unzip")]
    public class UnZipTask : Task {

        string _zipfile = null;
		string _outDir = null;

        /// <summary>The full path to the zip file.</summary>
        [TaskAttribute("zipfile", Required=true)]
        public string ZipFileName { 
			get { return Project.GetFullPath(_zipfile); } 
			set {_zipfile = value; } 
		}
        
		/// <summary>The full path to the destination folder.</summary>
		[TaskAttribute("outdir", Required=true)]
		public string OutDir { 
			get { return _outDir; }
			set { _outDir = value; } 
		}

		protected void ZipEventCallback(object source, ZipEventArgs e) {
			Log.Debug.WriteLine(LogPrefix + "UnZipping {0}", e.ZipEntry.Name);
		}
		
		protected override void ExecuteTask() {
			try {
				Log.Status.WriteLine(LogPrefix + "Unzipping {0} to {1}", ZipFileName, OutDir);
				
				ZipLib zipLib = new ZipLib();
				zipLib.ZipEvent += new ZipEventHandler(ZipEventCallback);
				zipLib.UnzipFile(ZipFileName, OutDir);
				zipLib.ZipEvent -= new ZipEventHandler(ZipEventCallback);
			} 
			catch(System.Exception e) {
				string msg = String.Format("Error extracting zip file '{0}'", ZipFileName);
				throw new BuildException(msg, Location, e);
			}
        }
   }
}