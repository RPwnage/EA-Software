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
using System;
using NAnt.Core.Attributes;

namespace NAnt.Core.Functions
{
	/// <summary>
	/// Collection of file manipulation routines.
	/// </summary>
	[FunctionClass("File Functions")]
	public class FileFunctions : FunctionClassBase
	{
		/// <summary>
		/// Determines whether the specified file exists.
		/// </summary>
		/// <param name="project"></param>
		/// <param name="path">The file to check.</param>
		/// <returns>True or False.</returns>
		/// <include file='Examples/File/FileExists.example' path='example'/>        
		[Function()]
		public static string FileExists(Project project, string path)
		{
			path = project.GetFullPath(path);
			return System.IO.File.Exists(path).ToString();
		}

		/// <summary>
		/// Check the FileAttributes of the file on the fully qualified path against a set of FileAttributes.
		/// </summary>
		/// <param name="project"></param>
		/// <param name="path">The path to the file.</param>
		/// <param name="attributes">The set of attributes to check. Delimited by a space.</param>
		/// <remarks>
		/// &lt;dl&gt;
		///		&lt;dt&gt;List of available attributes.&lt;/dt&gt;
		///		&lt;dl&gt;
		///			&lt;li&gt; Archive &lt;/li&gt;
		///			&lt;li&gt; Compressed &lt;/li&gt;
		///			&lt;li&gt; Device &lt;/li&gt;
		///			&lt;li&gt; Directory &lt;/li&gt;
		///			&lt;li&gt; Encrypted &lt;/li&gt;
		///			&lt;li&gt; Hidden &lt;/li&gt;
		///			&lt;li&gt; Normal &lt;/li&gt;
		///			&lt;li&gt; NotContentIndexed &lt;/li&gt;
		///			&lt;li&gt; Offline &lt;/li&gt;
		///			&lt;li&gt; ReadOnly &lt;/li&gt;
		///			&lt;li&gt; ReparsePoint &lt;/li&gt;
		///			&lt;li&gt; SparseFile &lt;/li&gt;
		///			&lt;li&gt; System &lt;/li&gt;
		///			&lt;li&gt; Temporary &lt;/li&gt;
		///		&lt;/dl&gt;
		///	&lt;/dl&gt;
		/// </remarks>
		/// <returns>True if all specified attributes are set; otherwise false.</returns>
		/// <include file='Examples/File/FileCheckAttributes.example' path='example'/>        
		[Function()]
		public static string FileCheckAttributes(Project project, string path, string attributes)
		{
			path = project.GetFullPath(path);

			// get the file attributes, throws exception if not found
			System.IO.FileAttributes fileAttribute = System.IO.File.GetAttributes(path);

			// replace each space with a comma so that the following call recognizes it as valid
			attributes = attributes.Replace(" ", ", ");

			// convert the case insensitive input string to an enum , throws exception if cant convert
			System.IO.FileAttributes userAttribute = (System.IO.FileAttributes)Enum.Parse(fileAttribute.GetType(), attributes, true);

			// compare the user attributes against the file attributes and return 
			// true if the file's attributes contains all of the user's attributes.
			bool b = ((int)(fileAttribute & userAttribute) == (int)userAttribute);
			return b.ToString();
		}

		/// <summary>
		/// Gets the FileAttributes of the file on the fully qualified path.
		/// </summary>
		/// <param name="project"></param>
		/// <param name="path">The path to the file.</param>
		/// <remarks>
		/// &lt;dl&gt;
		///		&lt;dt&gt;List of available attributes.&lt;/dt&gt;
		///		&lt;dl&gt;
		///			&lt;li&gt; Archive &lt;/li&gt;
		///			&lt;li&gt; Compressed &lt;/li&gt;
		///			&lt;li&gt; Device &lt;/li&gt;
		///			&lt;li&gt; Directory &lt;/li&gt;
		///			&lt;li&gt; Encrypted &lt;/li&gt;
		///			&lt;li&gt; Hidden &lt;/li&gt;
		///			&lt;li&gt; Normal &lt;/li&gt;
		///			&lt;li&gt; NotContentIndexed &lt;/li&gt;
		///			&lt;li&gt; Offline &lt;/li&gt;
		///			&lt;li&gt; ReadOnly &lt;/li&gt;
		///			&lt;li&gt; ReparsePoint &lt;/li&gt;
		///			&lt;li&gt; SparseFile &lt;/li&gt;
		///			&lt;li&gt; System &lt;/li&gt;
		///			&lt;li&gt; Temporary &lt;/li&gt;
		///		&lt;/dl&gt;
		///	&lt;/dl&gt;
		/// </remarks>
		/// <returns>A set of FileAttributes separated by a space.</returns>
		/// <include file='Examples/File/FileGetAttributes.example' path='example'/>        
		[Function()]
		public static string FileGetAttributes(Project project, string path)
		{
			path = project.GetFullPath(path);

			// get the file attributes, throws exception if not found
			System.IO.FileAttributes fileAttribute = System.IO.File.GetAttributes(path);
			return fileAttribute.ToString();
		}

		/// <summary>
		/// Gets the date time stamp the specified file or directory was last accessed.
		/// </summary>
		/// <param name="project"></param>
		/// <param name="path">The path to the file or directory.</param>
		/// <returns>The date and time that the specified file or directory was last accessed.</returns>
		/// <include file='Examples/File/FileGetLastAccessTime.example' path='example'/>        
		[Function()]
		public static string FileGetLastAccessTime(Project project, string path)
		{
			path = project.GetFullPath(path);

			DateTime dt = System.IO.File.GetLastAccessTime(path);
			return dt.ToString();
		}

		/// <summary>
		/// Gets the date time stamp the specified file or directory was last written to.
		/// </summary>
		/// <param name="project"></param>
		/// <param name="path">The path to the file or directory.</param>
		/// <returns>The date and time that the specified file or directory was last written to.</returns>
		/// <include file='Examples/File/FileGetLastWriteTime.example' path='example'/>        
		[Function()]
		public static string FileGetLastWriteTime(Project project, string path)
		{
			path = project.GetFullPath(path);

			DateTime dt = System.IO.File.GetLastWriteTime(path);
			return dt.ToString();
		}

		/// <summary>
		/// Gets the creation date time stamp of the specified file or directory.
		/// </summary>
		/// <param name="project"></param>
		/// <param name="path">The path to the file or directory.</param>
		/// <returns>The date and time that the specified file or directory was created.</returns>
		/// <include file='Examples/File/FileGetCreationTime.example' path='example'/>        
		[Function()]
		public static string FileGetCreationTime(Project project, string path)
		{
			path = project.GetFullPath(path);

			DateTime dt = System.IO.File.GetCreationTime(path);
			return dt.ToString();
		}


		/// <summary>
		/// Get the version number for the specified file.
		/// </summary>
		/// <param name="project"></param>
		/// <param name="path">The path to the specified file.</param>
		/// <returns>The version number for the specified file.</returns>
		/// <include file='Examples/File/FileGetVersion.example' path='example'/>        
		[Function()]
		public static string FileGetVersion(Project project, string path)
		{
			path = project.GetFullPath(path);
			return System.Diagnostics.FileVersionInfo.GetVersionInfo(path).FileVersion;
		}
		
		/// <summary>
		/// Get the size of the specified file.
		/// </summary>
		/// <param name="project"></param>
		/// <param name="path">The path to the specified file.</param>
		/// <returns>Size of the specified file in bytes.</returns>
		/// <include file='Examples/File/FileGetSize.example' path='example'/>        
		[Function()]
		public static string FileGetSize(Project project, string path)
		{
			path = project.GetFullPath(path);
			System.IO.FileInfo fileInfo = new System.IO.FileInfo(path);

			return fileInfo.Length.ToString();
		}
		
	}
}