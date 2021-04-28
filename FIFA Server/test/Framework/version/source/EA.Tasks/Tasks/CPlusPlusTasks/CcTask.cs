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

	/// <summary>A generic C/C++ compiler task.</summary>
	/// <remarks>
	/// <para>The <c>cc</c> task requires the following property to be set:</para>
	/// <list type='table'>
	///     <listheader>
	///         <term>Property</term>
	///         <description>Description</description>
	///     </listheader>
	///     <item>
	///         <term>${cc}</term>
	///         <description>The absolute pathname of the compiler executable.</description>
	///     </item>
	///     <item>
	///         <term>${cc.template.includedir}</term>
	///         <description>The syntax template to transform <c>${cc.includes}</c> into compiler flags.</description>
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
	///         <term>${cc.defines}</term>
	///         <description>The defines for the compilation.</description>
	///     </item>
	///		<item>      
	///			<term>${cc.forcelowercasefilepaths}</term>      
	///			<description>If <c>true</c> file paths (folder and file names) will be 
	///			forced to lower case (useful in avoiding redundant NAnt builds 
	///			between VS.net and command line environments in the case of capitalized
	///			folder names when file name case is unimportant).  Default is false.</description>
	///		</item>
	///     <item>
	///         <term>${cc.includedirs}</term>
	///         <description>The include directories for the compilation.</description>
	///     </item>
	///     <item>
	///         <term>${cc.nodep}</term>
	///         <description>If <c>true</c> dependency files will not be created.</description>
	///     </item>
	///     <item>
	///         <term>${cc.nocompile}</term>
	///         <description>If <c>true</c> object files not be created.</description>
	///     </item>
	///     <item>
	///         <term>${cc.objfile.extension}</term>
	///         <description>The file extension for object files.  Default is ".obj".</description>
	///     </item>
	///     <item>
	///         <term>${cc.options}</term>
	///         <description>The options flags for the compilation.  All the files from the <c>source</c> fileset will be compiled with the same options.  If you want different compile options, you must invoke the <c>cc</c> task with those options.</description>
	///     </item>
	///     <item>
	///         <term>${cc.parallelcompiler}</term>
	///         <description>If true, compilation will occur on parallel threads if possible
	///         .  Default is false.</description>
	///     </item>
	///     <item>
	///         <term>${cc.template.define}</term>
	///         <description>The syntax template to transform <c>${cc.defines}</c> into compiler flags.</description>
	///     </item>
	///     <item>
	///         <term>${cc.template.includedir}</term>
	///         <description>The syntax template to transform <c>${cc.includedirs}</c> into compiler flags.</description>
	///     </item>
	///     <item>
	///         <term>${cc.template.usingdir}</term>
	///         <description>The syntax template to transform <c>${cc.usingdirs}</c> into compiler flags.</description>
	///     </item>
	///     <item>
	///         <term>${cc.template.commandline}</term>
	///         <description>The template to use when creating the command line.  Default is %defines% %includedirs% %options%.</description>
	///     </item>
	///     <item>
	///         <term>${cc.template.sourcefile}</term>
	///         <description>The syntax template to transform a <c>source file</c> into compiler source file.</description>
	///     </item>
	///     <item>
	///         <term>${cc.template.responsefile}</term>
	///         <description>The syntax template to transform the <c>responsefile</c> into a response file flag. Default is @"%responsefile%".</description>
	///     </item>
	///     <item>
	///         <term>${cc.threadcount}</term>
	///         <description>The number of threads to use when compiling. Default is 1 per cpu.</description>
	///     </item>
	///     <item>
	///         <term>${cc.useresponsefile}</term>
	///         <description>If <c>true</c> a response file, containing the entire commandline, will be passed as the only argument to the compiler. Default is false.</description>
	///     </item>
	///     <item>
	///         <term>${cc.userelativepaths}</term>
	///         <description>If <c>True</c> the working directory of the compiler will be set to the base directory of the <c>sources</c> fileset. All source and output files will then be made relative to this path. Default is <c>false</c>.</description>
	///     </item>
	///     <item>
	///         <term>${cc.usingdirs}</term>
	///         <description>The using directories for managed C++, such as VC++. Compiler that support using directive should read this property.</description>
	///     </item>
	///     <item>      <term>${cc.template.responsefile.commandline}</term><description>The template to use when creating the command line for response file. Default value is ${cc.template.responsefile.commandline}</description></item>
	///     <item>      <term>${cc.responsefile.separator}</term><description>Separator used in response files. Default is " ".</description></item>
	///     <item>      <term>${cc.template.responsefile.objectfile}</term><description>The syntax template to transform the <c>objects</c> file set into compiler flags in response file. Default ${cc.template.objectfile}</description></item>
	/// </list>
	/// 
	/// <para>The following terms can be used in the above properties:</para>
	/// <list type='table'>
	///     <listheader><term>Term</term><description>Description</description></listheader>
	///     <item>      <term>%define%</term>     <description>Used by the <c>${cc.template.define}</c> property to represent the current value of the <c>${cc.defines}</c> property during template processing.</description></item>
	///     <item>      <term>%includedir%</term> <description>Used by the <c>${cc.template.includedir}</c> property to represent the individual value of the <c>${cc.includedirs}</c> property.</description></item>
	///     <item>      <term>%usingdir%</term> <description>Used by the <c>${cc.template.usingdir}</c> property to represent the individual value of the <c>${cc.usingdirs}</c> property.</description></item>
	///     <item>      <term>%responsefile%</term> <description>Used by the <c>${cc..template.responsefile}</c> property to represent the filename of the response file.</description></item>
	///     <item>      <term>%objectfile%</term> <description>Used by the <c>${cc.options}</c> property to represent the object file name.  Object file names are formed by appending <c>".obj"</c> to the source file name.</description></item>
	///     <item>      <term>%outputdir%</term>  <description>Used by the <c>${cc.options}</c> property to represent the actual values of the   <c>%outputdir%</c> attribute.</description></item>
	///     <item>      <term>%outputname%</term> <description>Used by the <c>${cc.options}</c> property to represent the actual value of the   <c>%outputname%</c> attribute.</description></item>
	///     <item>      <term>%sourcefile%</term> <description>Used by the <c>${cc.options}</c> property to represent the individual value of the <c>sources</c> fileset.</description></item>
	/// </list>
	/// <para>This task can use a named option set for specifying options for unique components.  For more information see the Fundamentals->Option Sets topic in help file.</para>
	/// </remarks>
	/// <include file="Examples/Shared/HelloWorld.example" path="example"/>
	/// <include file="Examples/Shared/VisualCppSetup.example" path="example"/>
	[TaskName("cc")]
	public class CcTask : CompilerBase {

		public CcTask() : base("cc") { }

		/// <summary>The set of files to compile.</summary>
		[FileSet("sources")]
		public FileSet CcSources {
			get { return Sources; }
		}

		/// <summary>Force using assemblies for managed builds.</summary>
		[Property("forceusing-assemblies")]
		public FileSet CcForceUsingAssemblies
		{
			get { return ForceUsingAssemblies; }
		}

		/// <summary>Custom include directories for this set files.  Use complete paths and place 
		/// each directory on it's own line.</summary>
		[Property("includedirs")]
		public PropertyElement CcIncludeDirectories {
			get { return IncludeDirectories; }
		}

		/// <summary>Custom using directories for this set files.  Use complete paths and place 
		/// each directory on it's own line. Only applied to managed C++ compiler, such as VC++.</summary>
		[Property("usingdirs")]
		public PropertyElement CcUsingDirectories
		{
			get { return UsingDirectories; }
		}

		/// <summary>Custom program options for these files.  Options get appended to the end of 
		/// the options specified by the <c>${cc.options}</c> property.</summary>
		[Property("options")]
		public PropertyElement CcOptions {
			get { return Options; }
		}

		/// <summary>Custom #defines for these files.  Defines get appended to the end of the
		/// defines specified in the <c>${cc.defines}</c> property.  Place each define on it's own line.</summary>
		[Property("defines")]
		public PropertyElement CcDefines {
			get { return Defines; }
		}
	}
}
