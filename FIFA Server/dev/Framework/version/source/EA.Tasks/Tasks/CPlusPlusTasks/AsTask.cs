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

using NAnt.Core;
using NAnt.Core.Attributes;

namespace EA.CPlusPlusTasks
{

	/// <summary>A generic assembler compiler task.</summary>
	/// <remarks>
	/// <para>The <c>as</c> task requires the following property to be set:</para>
	/// <list type='table'>
	///     <listheader>
	///         <term>Property</term>
	///         <description>Description</description>
	///     </listheader>
	///     <item>
	///         <term>${as}</term>
	///         <description>The absolute pathname of the compiler executable.</description>
	///     </item>
	///     <item>
	///         <term>${as.template.includedir}</term>
	///         <description>The syntax template to transform <c>${as.includes}</c> into compiler flags.</description>
	///     </item>
	/// </list>
	/// 
	/// <para>The task will make use of the following properties:</para>
	/// <list type='table'>
	///     <listheader>
	///         <term>Property</term>
	///         <description>Description</description>
	///     </listheader>
	///     <item>
	///         <term>${as.defines}</term>
	///         <description>The defines for the compilation.</description>
	///     </item>
	///     <item>
	///         <term>${as.includedirs}</term>
	///         <description>The include directories for the compilation.</description>
	///     </item>
	///     <item>
	///         <term>${as.options}</term>
	///         <description>The options flags for the compilation.  All the files from the <c>source</c> file set will be compiled with the same options.  If you want different compile options, you must invoke the <c>as</c> task with those options.</description>
	///     </item>
	///		<item>
	///			<term>${as.template.commandline}</term>
	///			<description>The template to use when creating the command line.  Default is %defines% %includedirs% %options%.</description>
	///     </item>
	///     <item>
	///         <term>${as.template.define}</term>
	///         <description>The syntax template to transform <c>${as.defines}</c> into compiler flags.</description>
	///     </item>
	///     <item>
	///			<term>${as.template.sourcefile}</term>
	///         <description>The syntax template to transform a <c>source file</c> into compiler source file.</description>
	///     </item>
	///     <item>      
	///         <term>${as.template.responsefile}</term>
	///         <description>The syntax template to transform the <term>responsefile</term> into a response file flag. Default is @"%responsefile%".</description>
	///     </item>
	///     <item>
	///         <term>${as.nodep}</term>
	///         <description>If <c>true</c> dependency files will not be created.</description>
	///     </item>
	///     <item>
	///         <term>${as.nocompile}</term>
	///         <description>If <c>true</c> object files not be created.</description>
	///     </item>
	///     <item>
	///         <term>${as.useresponsefile}</term>
	///         <description>If <c>true</c> a response file, containing the entire commandline, will be passed as the only argument to the compiler. Default is false.</description>
	///     </item>
	///     <item>
	///         <term>${as.userelativepaths}</term>
	///         <description>If <c>True</c> the working directory of the assembler will be set to the base directory of the <c>asmsources</c> fileset. All source and output files will then be made relative to this path. Default is <c>false</c>.</description>
	///     </item>
	///     <item>
	///         <term>${as.threadcount}</term>
	///         <description>The number of threads to use when compiling. Default is 1 per cpu.</description>
	///     </item>
	///     <item>
	///         <term>${as.objfile.extension}</term>
	///         <description>The file extension for object files.  Default is ".obj".</description>
	///     </item>
	///		<item>      
	///			<term>${as.forcelowercasefilepaths}</term>      
	///			<description>If <c>true</c> file paths (folder and file names) will be 
	///			forced to lower case (useful in avoiding redundant NAnt builds 
	///			between VS.net and command line environments in the case of capitalized
	///			folder names when file name case is unimportant).  Default is false.</description>
	///		</item>
	/// <item>      <term>${as.template.responsefile.commandline}</term><description>The template to use when creating the command line for response file. Default value is ${as.template.responsefile.commandline}</description></item>
	/// <item>      <term>${as.responsefile.separator}</term><description>Separator used in response files. Default is " ".</description></item>
	/// <item>      <term>${as.template.responsefile.objectfile}</term><description>The syntax template to transform the <term>objects</term> file set into compiler flags in response file. Default ${as.template.objectfile}</description></item>
	/// </list>
	/// 
	/// <para>The task declares the following template items in order to help defining the above 
	/// properties:</para>
	/// <list type='table'>
	///     <listheader><term>Template item</term><description>Description</description></listheader>
	///     <item>      <term>%define%</term>     <description>Used by the <term>${as.template.define}</term> property to represent the current value of the <term>${as.defines}</term> property during template processing.</description></item>
	///     <item>      <term>%includedir%</term> <description>Used by the <term>${as.template.includedir}</term> property to represent the individual value of the <term>${as.includedirs}</term> property.</description></item>
	///     <item>      <term>%responsefile%</term> <description>Used by the <term>${cc..template.responsefile}</term> property to represent the filename of the response file.</description></item>
	///     <item>      <term>%objectfile%</term> <description>Used by the <term>${as.options}</term> property to represent the object file name.  Object file names are formed by appending <c>".obj"</c> to the source file name.</description></item>
	///     <item>      <term>%outputdir%</term>  <description>Used by the <term>${as.options}</term> property to represent the actual values of the   <c>%outputdir%</c> attribute.</description></item>
	///     <item>      <term>%outputname%</term> <description>Used by the <term>${as.options}</term> property to represent the actual value of the   <c>%outputname%</c> attribute.</description></item>
	///     <item>      <term>%sourcefile%</term> <description>Used by the <term>${as.options}</term> property to represent the individual value of the <term>sources</term> file set.</description></item>
	/// </list>
	/// <para>This task can use a named option set for specifying options for unique components.  For more information see the Fundamentals->Option Sets topic in help file.</para>
	/// </remarks>
	[TaskName("as")]
	public class AsTask : CompilerBase {

		public AsTask() : base("as") {
		}
		
		/// <summary>The set of files to compile.</summary>
		[FileSet("asmsources")]
		public FileSet AsSources { 
			get { return Sources; }
		}

		/// <summary>Custom include directories for this set files.  Use complete paths and place 
		/// each directory on it's own line.</summary>
		[Property("asmincludedirs")]
		public PropertyElement AsIncludeDirectories {
			get { return IncludeDirectories; }
		}

		/// <summary>Custom program options for these files.  Options get appended to the end of 
		/// the options specified by the <c>${cc.options}</c> property.</summary>
		[Property("asmoptions")]
		public PropertyElement AsOptions {
			get { return Options; }
		}

		/// <summary>Custom #defines for these files.  Defines get appended to the end of the
		/// defines specified in the <c>${cc.defines}</c> property.  Place each define on it's own line.</summary>
		[Property("asmdefines")]
		public PropertyElement AsDefines {
			get { return Defines; }
		}

	}
}
