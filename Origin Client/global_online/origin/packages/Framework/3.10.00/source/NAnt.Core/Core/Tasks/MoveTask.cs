// NAnt - A .NET build tool
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

using System;
using System.IO;

using NAnt.Core.Attributes;
using NAnt.Core.Logging;

namespace NAnt.Core.Tasks {

    /// <summary>Moves a file or file set to a new location.</summary>
    /// <remarks>
    ///   <para>Files are only moved if the source file is newer than the destination file, or if the destination file does not exist.  This applies to files matched by a file set as well as files specified individually.</para>
    ///   <note>You can explicitly overwrite files with the overwrite attribute.</note>
    ///   <para>File sets are used to select groups of files to move. To use a file set, the todir attribute must be set.</para>
    /// </remarks>
    /// <include file='Examples/Move/MoveFile.example' path='example'/>
    /// <include file='Examples/Move/MoveFileSet.example' path='example'/>
    [TaskName("move")]
    public class MoveTask : CopyTask {

        /// <summary>
        /// Actually does the file (and possibly empty directory) moves.
        /// </summary>
        protected override void DoFileOperations() {
            if (FileCopyMap.Count > 0) {

                // loop thru our file list
                foreach (string sourcePath in FileCopyMap.Keys) {
                    string destinationPath = (string)FileCopyMap[sourcePath];
                    if (sourcePath == destinationPath) {
                        Log.Status.WriteLine(LogPrefix + "Skipping self-move of {0}" + sourcePath);
                        continue;
                    }

                    try {
                        // check if directory exists
                        if (Directory.Exists(sourcePath)) {
                            Log.Status.WriteLine(LogPrefix + "Moving directory {0} to {1}", sourcePath, destinationPath);
                            Directory.Move(sourcePath, destinationPath);
                        }
                        else {

                            DirectoryInfo todir = new DirectoryInfo(destinationPath);
                            if (!todir.Exists) {
                                Directory.CreateDirectory( Path.GetDirectoryName(destinationPath) );
                            }

                            Log.Status.WriteLine(LogPrefix + "Moving {0} to {1}", sourcePath, destinationPath);
                            
                            FileAttributes oldAttrib = FileAttributes.Normal;
                            if (MaintainAttributes) {
                                if(File.Exists(destinationPath)) {
                                    oldAttrib = File.GetAttributes(destinationPath);
                                }
                                else if (File.Exists(sourcePath)) {
                                    oldAttrib = File.GetAttributes(sourcePath);
                                }
                            }
                            
                            if (File.Exists(destinationPath)) {
                                if (Clobber) {
                                    File.SetAttributes(destinationPath, File.GetAttributes(destinationPath) & ~FileAttributes.ReadOnly & ~FileAttributes.Hidden);
                                }
                                File.Delete(destinationPath);
							}
		
                            File.Move(sourcePath, destinationPath);

                            if (MaintainAttributes && File.Exists(destinationPath)) {
                                File.SetAttributes(destinationPath, oldAttrib);
                            }
                        }

                    } catch (Exception e) {
                        string msg = String.Format("Failed to move {0} to {1}\n{2}", sourcePath, destinationPath, e.Message);
                        throw new BuildException(msg, Location);
                    }
                }
            }
        }
    }
}
