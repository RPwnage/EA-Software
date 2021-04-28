// NAnt - A .NET build tool
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
// Kosta Arvanitis (karvanitis@ea.com)
using System;
using System.Collections;
using System.Collections.Specialized;
using System.Text;

using NAnt.Core;
using NAnt.Core.Attributes;
using NAnt.Core.Writers;

namespace NAnt.Core.Functions
{
    /// <summary>
    /// Collection of file set manipulation routines.
    /// </summary>
    [FunctionClass("FileSet Functions")]
    public class FileSetFunctions : FunctionClassBase
    {
        /// <summary>
        /// Check if the specified fileset is defined.
        /// </summary>
        /// <param name="project"></param>
        /// <param name="filesetName">The fileset name to check.</param>
        /// <returns>True or False.</returns>
        /// <include file='Examples\FileSet\FileSetExists.example' path='example'/>        
        [Function()]
        public static string FileSetExists(Project project, string filesetName)
        {
            return project.NamedFileSets.ContainsKey(filesetName).ToString();
        }

        /// <summary>
        /// Gets the number of files contained in the FileSet.
        /// </summary>
        /// <param name="project"></param>
        /// <param name="fileSetName">The name of the FileSet.</param>
        /// <returns>Number of file in the fileset.</returns>
        /// <include file='Examples\FileSet\FileSetCount.example' path='example'/>        
        [Function()]
        public static string FileSetCount(Project project, string fileSetName)
        {
            FileSet fileSet;
            if (!project.NamedFileSets.TryGetValue(fileSetName, out fileSet))
            {
                string msg = String.Format("Unknown named fileset '{0}'.", fileSetName);
                throw new BuildException(msg);
            }
            return fileSet.FileItems.Count.ToString();
        }

        /// <summary>
        /// Returns the specified filename at the zero-based index of the specified FileSet.
        /// </summary>
        /// <param name="project"></param>
        /// <param name="fileSetName">The name of the FileSet.</param>
        /// <param name="index">Zero-based index of the specified FileSet. Index must be non-negative and less than the size of the FileSet.</param>
        /// <returns>Filename at zero-based index of the FileSet. </returns>
        /// <include file='Examples\FileSet\FileSetGetItem.example' path='example'/>        
        [Function()]
        public static string FileSetGetItem(Project project, string fileSetName, int index)
        {
            FileSet fileSet;
            if (!project.NamedFileSets.TryGetValue(fileSetName, out fileSet))
            {
                string msg = String.Format("Unknown named fileset '{0}'.", fileSetName);
                throw new BuildException(msg);
            }

            if (index < 0 || index >= fileSet.FileItems.Count)
            {
                string msg = String.Format("Fileset '{0}' at index '{1}' is out of range.", fileSetName, index);
                throw new BuildException(msg);
            }

            return ((FileItem)fileSet.FileItems[index]).FileName;
        }


        /// <summary>
        /// Converts a FileSet to a string.
        /// </summary>
        /// <param name="project"></param>
        /// <param name="fileSetName">The name of the FileSet.</param>
        /// <param name="delimiter">The delimiter used to seperate each file.</param>
        /// <returns>A string of delimited files.</returns>
        /// <include file='Examples\FileSet\FileSetToString.example' path='example'/>        
        [Function()]
        public static string FileSetToString(Project project, string fileSetName, string delimiter)
        {
            FileSet fileSet;
            if (!project.NamedFileSets.TryGetValue(fileSetName, out fileSet))
            {
                string msg = String.Format("Unknown named fileset '{0}'.", fileSetName);
                throw new BuildException(msg);
            }

            StringBuilder files = new StringBuilder();

            foreach (FileItem fileItem in fileSet.FileItems)
            {
                files.Append(fileItem.FileName);
                files.Append(delimiter);
            }
            if (files.Length - delimiter.Length >= 0)
            {
                files.Remove(files.Length - delimiter.Length, delimiter.Length);
            }

            return files.ToString();
        }

        /// <summary>
        /// Undefine an existing fileset
        /// </summary>
        /// <param name="project"></param>
        /// <param name="filesetName">The fileset name to undefine.</param>
        /// <returns>True or False.</returns>
        /// <include file='Examples\FileSet\FileSetUndefine.example' path='example'/>        
        [Function()]
        public static string FileSetUndefine(Project project, string filesetName)
        {

            FileSet fileSet;
            return project.NamedFileSets.TryRemove(filesetName, out fileSet).ToString();
        }

        /// <summary>
        /// Get the base directory of a fileset
        /// </summary>
        /// <param name="project"></param>
        /// <param name="fileSetName"></param>
        /// <returns>Path to the base directory</returns>
        [Function()]
        public static string FileSetGetBaseDir(Project project, string fileSetName)
        {
            FileSet fileSet;
            if (!project.NamedFileSets.TryGetValue(fileSetName, out fileSet))
            {
                string msg = String.Format("Unknown named fileset '{0}'.", fileSetName);
                throw new BuildException(msg);
            }
            return fileSet.BaseDirectory;
        }

        /// <summary>
        /// Converts a FileSet to an XML string.
        /// </summary>
        /// <param name="project"></param>
        /// <param name="fileSetName">The name of the FileSet.</param>
        /// <returns>string containing XML desribing the fileset.</returns>
        [Function()]
        public static string FileSetDefinitionToXmlString(Project project, string fileSetName)
        {
            FileSet fileSet;
            if (!project.NamedFileSets.TryGetValue(fileSetName, out fileSet))
            {
                string msg = String.Format("Unknown named fileset '{0}'.", fileSetName);
                throw new BuildException(msg);
            }

            using(var writer = new XmlWriter(new XmlFormat(identChar: ' ', identation: 2, newLineOnAttributes: false, encoding: new UTF8Encoding(false) )))
            {
                writer.WriteStartElement("fileset");
                writer.WriteAttributeString("name", fileSetName);
                writer.WriteAttributeString("basedir", fileSet.BaseDirectory);
                writer.WriteAttributeString("failonmissing", fileSet.FailOnMissingFile);
                foreach (var include in fileSet.Includes)
                {
                    writer.WriteStartElement("include");
                    writer.WriteAttributeString("name", include.Pattern.BaseDirectory + "/" + include.Pattern.Value);
                    if(include.AsIs)
                        writer.WriteAttributeString("asis", include.AsIs);
                    if(include.Force)
                        writer.WriteAttributeString("force", include.Force);
                    writer.WriteNonEmptyAttributeString("optionset", include.OptionSet);
                    writer.WriteEndElement();
                }

                foreach (var exclude in fileSet.Excludes)
                {
                    writer.WriteStartElement("exclude");
                    writer.WriteAttributeString("name", exclude.Pattern.BaseDirectory + "/" + exclude.Pattern.Value);
                    writer.WriteEndElement();
                }
                writer.WriteEndElement();
                writer.Close();
                var bytes = writer.Internal.ToArray();
                return Encoding.UTF8.GetString(bytes, 0, bytes.Length);
            }
        }
    }
}