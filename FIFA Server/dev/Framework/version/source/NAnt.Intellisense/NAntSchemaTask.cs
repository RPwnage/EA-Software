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
using System.Reflection;

using NAnt.Core.Attributes;
using NAnt.Core;
using NAnt.Core.Reflection;
using NAnt.Core.Logging;
using System.Collections.Generic;

namespace NAnt.Intellisense
{
	/// <summary>Generates an xml schema file (*.xsd) based on task source files. This file
	/// can be used by visual studio to provide intellisense for editing build files.</summary>
	[TaskName("nantschema")]
	public class NAntSchemaTask : Task
	{
		public NAntSchemaTask() : base("nantschema") { }

		/// <summary>The name of the output schema file.</summary>
		[TaskAttribute("outputfile", Required = true)]
		public String OutputFile { get; set; }

		/// <summary>The list of assemblies to load and search for tasks in</summary>
		[FileSet("fileSet")]
		public FileSet AssemblyFileSet { get; set; } = new FileSet();

		protected override void ExecuteTask()
		{
			List<string> assemblyFiles = new List<string>();
			if (AssemblyFileSet != null)
			{
				foreach (var fi in AssemblyFileSet.FileItems)
				{
					assemblyFiles.Add(fi.Path.Path);
				}
			}

			Install(OutputFile, assemblyFiles, this.Log);

			Project.Log.Minimal.WriteLine("Schema generated to {0}.", OutputFile);
		}

		public static void Install(string outputFile, List<string> assemblyFiles, Log log = null)
		{
			var schemaMetaData = CreateSchemaMetaData(assemblyFiles, log);

			var generator = new NantSchemaGenerator(schemaMetaData, log);

			generator.GenerateSchema("schemas/ea/framework3.xsd");

			generator.WriteSchema(outputFile);
		}

		private static SchemaMetadata CreateSchemaMetaData(List<string> assemblyFiles, Log log = null)
		{
			var schemaMetaData = new SchemaMetadata(log);

			foreach (var assemblyInfo in AssemblyLoader.GetAssemblyCacheInfo())
			{
				schemaMetaData.AddAssembly(assemblyInfo.Assembly, assemblyInfo.Path);
			}

			foreach (string file in assemblyFiles)
			{
				schemaMetaData.AddAssembly(Assembly.LoadFrom(file));
			}

			schemaMetaData.GenerateSchemaMetaData();

			return schemaMetaData;
		}
	}
}
