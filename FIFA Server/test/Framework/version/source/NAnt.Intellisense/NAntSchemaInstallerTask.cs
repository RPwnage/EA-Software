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
using System.IO;
using System.Xml;
using NAnt.Core.Attributes;
using NAnt.Core;
using System.Diagnostics;
using System.Security.Principal;
using NAnt.Core.Logging;

namespace NAnt.Intellisense
{
	/// <summary>Copies XML schema file to the schema directory under the 
	/// visual studio install directory. Also, extends the visual studio 
	/// schema catalog to associate the new schema with the *.build extension.</summary>
	[TaskName("nantschemainstall")]
	public class NAntSchemaInstallerTask : Task
	{
		private const String _nantInstallFilename = "nant_intellisense.xml";

		public NAntSchemaInstallerTask()
		{
		}

		public static string GetSchemaDirectory(string environmentVariable, string visualstudioVersion, Log log)
		{
			String path = Environment.GetEnvironmentVariable(environmentVariable);
			if (path == null)
			{
				return null;
			}

			DirectoryInfo vsInstallDir = new DirectoryInfo(path);
			if (vsInstallDir.Parent != null && vsInstallDir.Parent.Parent != null)
			{
				vsInstallDir = vsInstallDir.Parent.Parent;
			}
			else
			{
				if (log != null)
				{
					log.Warning.WriteLine(@"Cannot find path '../../xml/Schemas' relative to '{0}' because the original path from the environment variable '{1}' is less than 2 levels deep.",
						path, environmentVariable);
				}
				else
				{
					Console.WriteLine(@"Warning: Cannot find path '../../xml/Schemas' relative to '{0}' because the original path from the environment variable '{1}' is less than 2 levels deep.",
						path, environmentVariable);
				}
				return null;
			}

			string schemaFolder = Path.Combine(vsInstallDir.FullName, "xml", "Schemas");
			if (!Directory.Exists(schemaFolder))
			{
				if (log != null)
				{
					log.Warning.WriteLine("The Environment variable {0} exists but the visual studio {1} schema directory could not be found: '{2}'", environmentVariable, visualstudioVersion, schemaFolder);
				}
				else
				{
					Console.WriteLine("The Environment variable {0} exists but the visual studio {1} schema directory could not be found: '{2}'", environmentVariable, visualstudioVersion, schemaFolder);
				}
				return null;
			}
			return schemaFolder;
		}

		// VS2017 no longer uses the environment variable or registry keys so find the location of the schema folder is different
		public static string FindVs2017SchemaDirectory(Log log = null)
		{
			// firstly see if the most likely default install location exists
			string defaultInstallDirectory = @"C:\Program Files (x86)\Microsoft Visual Studio\2017\Professional\Xml\Schemas";
			if (Directory.Exists(defaultInstallDirectory))
			{
				return defaultInstallDirectory;
			}

			// use vswhere to find the location of visual studio, for cases where they are using a preview version or not using professional, etc.
			string vsWherePath = @"C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe";
			if (File.Exists(vsWherePath))
			{
				string installPath = null;

				try
				{
					Process process = new Process();
					process.StartInfo.FileName = vsWherePath;
					process.StartInfo.RedirectStandardOutput = true;
					process.StartInfo.CreateNoWindow = true;
					process.StartInfo.UseShellExecute = false;
					process.OutputDataReceived += new DataReceivedEventHandler((sender, e) =>
					{

						if (!String.IsNullOrEmpty(e.Data) && e.Data.StartsWith("installationPath:"))
						{
							installPath = e.Data.Substring("installationPath: ".Length);
						}
					});
					process.Start();
					process.BeginOutputReadLine();
					process.WaitForExit();
					process.Close();
				}
				catch (Exception)
				{
					if (log != null)
					{
						log.Warning.WriteLine("Failed Running vswhere.exe to find the vs2017 install directory");
					}
					else
					{
						Console.WriteLine("Failed Running vswhere.exe to find the vs2017 install directory");
					}
				}

				if (!installPath.IsNullOrEmpty() && Directory.Exists(installPath))
				{
					string schemaDir = Path.Combine(installPath, "xml", "Schemas");
					if (Directory.Exists(schemaDir))
					{
						return installPath;
					}
				}
			}

			if (log != null)
			{
				log.Warning.WriteLine("Could not find path to VS2017 schema directory");
			}
			else
			{
				Console.WriteLine("Could not find path to VS2017 schema directory");
			}
			return null;
		}

		/// <summary>The install directory of visual studio 2012 (vs11) where the schema files will be copied.
		/// By default uses environment variable VS110COMNTOOLS.</summary>
		[TaskAttribute("vs11path", Required = false)]
		public String Vs11Path { get; set; }

		/// <summary>The install directory of visual studio 2013 (vs12) where the schema files will be copied.
		/// By default uses environment variable VS120COMNTOOLS.</summary>
		[TaskAttribute("vs12path", Required = false)]
		public String Vs12Path { get; set; }

		/// <summary>The install directory of visual studio 2015 (vs14) where the schema files will be copied.
		/// By default uses environment variable VS140COMNTOOLS.</summary>
		[TaskAttribute("vs14path", Required = false)]
		public String Vs14Path { get; set; }

		/// <summary>The install directory of visual studio 2017 (vs15) where the schema files will be copied.</summary>
		[TaskAttribute("vs15path", Required = false)]
		public String Vs15Path { get; set; }

		/// <summary>The name of the schema file. Should match the name of the output file of the nantschema task</summary>
		[TaskAttribute("inputfile", Required = true)]
		public String InputFile { get; set; }

		/// <summary>Creates a schema catalog file which is used to associate files with the 
		/// extension *.build with the XML schema file framework3.xsd</summary>
		private static void CreateXmlSchemaCatalogDocument(string schemaDirectory)
		{
			// only need to create this file once
			string catalogFilePath = Path.Combine(schemaDirectory, _nantInstallFilename);
			if (File.Exists(catalogFilePath)) return;

			System.Xml.XmlDocument document = new System.Xml.XmlDocument();

			XmlNode rootNode = document.CreateNode(XmlNodeType.Element,
				"SchemaCatalog", "http://schemas.microsoft.com/xsd/catalog");
			document.AppendChild(rootNode);

			XmlElement element = document.CreateElement("Association", rootNode.NamespaceURI);
			XmlAttribute extension = document.CreateAttribute("extension");
			extension.Value = "build";
			element.Attributes.Append(extension);

			XmlAttribute schema = document.CreateAttribute("schema");
			schema.Value = "%InstallRoot%/xml/schemas/framework3.xsd";
			element.Attributes.Append(schema);

			XmlAttribute xmlns = document.CreateAttribute("targetNamespace");
			xmlns.Value = "schemas/ea/framework3.xsd";
			element.Attributes.Append(xmlns);

			rootNode.AppendChild(element);

			document.Save(catalogFilePath);
		}

		/// <summary>Copies the schema file to all visual studio schema directories</summary>
		private static void CopySchemaFile(string vsSchemaPath, string inputFile)
		{
			FileInfo schemafile = new FileInfo(inputFile);
			var filename = Path.GetFileName(schemafile.FullName);

			string dstFilePath = Path.Combine(new DirectoryInfo(vsSchemaPath).FullName, filename);
			if (File.Exists(dstFilePath))
			{
				File.SetAttributes(dstFilePath, FileAttributes.Normal);
			}
			schemafile.CopyTo(dstFilePath, true);
		}

		public static void Install(string inputFile, string vs14Path = null, string vs15Path = null, Log log = null)
		{
			string[] visualstudioSchemaPaths = {
				vs14Path ?? GetSchemaDirectory("VS140COMNTOOLS", "2015", log),
				vs15Path ?? FindVs2017SchemaDirectory(log)
			};

			// The visual studio directory can only be modified with administrator privilege
			bool isElevated = false;
			try
			{
				using (WindowsIdentity identity = WindowsIdentity.GetCurrent())
				{
					WindowsPrincipal principal = new WindowsPrincipal(identity);
					isElevated = principal.IsInRole(WindowsBuiltInRole.Administrator);
				}
			}
			catch { }
			if (!isElevated)
			{
				if (log != null)
				{
					log.Warning.WriteLine("Detected as not running under administrator privileges, may not be able to modify Visual Studio directory.");
				}
				else
				{
					Console.WriteLine("Detected as not running under administrator privileges, may not be able to modify Visual Studio directory.");
				}
			}

			foreach (string vsSchemaPath in visualstudioSchemaPaths)
			{
				if (vsSchemaPath != null)
				{
					try
					{
						CreateXmlSchemaCatalogDocument(vsSchemaPath);

					}
					catch (Exception e)
					{
						if (log != null)
						{
							log.Warning.WriteLine("Filed to create schema catalog: " + e.Message);
							if (!isElevated) log.Warning.WriteLine("Administrator Priviledges were not detected, try running the command again with administrator priviledges.");
						}
						else
						{
							Console.WriteLine("Filed to create schema catalog: " + e.Message);
							if (!isElevated) Console.WriteLine("Administrator Priviledges were not detected, try running the command again with administrator priviledges.");
						}
					}

					try
					{
						CopySchemaFile(vsSchemaPath, inputFile);
					}
					catch (Exception e)
					{
						if (log != null)
						{
							log.Warning.WriteLine("Filed to create schema catalog: " + e.Message);
							if (!isElevated) log.Warning.WriteLine("Administrator Priviledges were not detected, try running the command again with administrator priviledges.");
						}
						else
						{
							Console.WriteLine("Filed to create schema catalog: " + e.Message);
							if (!isElevated) Console.WriteLine("Administrator Priviledges were not detected, try running the command again with administrator priviledges.");
						}
					}
				}
			}
		}

		protected override void ExecuteTask()
		{
			Install(InputFile, Vs14Path, Vs15Path, this.Log);
		}
	}
}
