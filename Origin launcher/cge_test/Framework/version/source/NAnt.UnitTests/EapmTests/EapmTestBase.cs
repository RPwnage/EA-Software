// NAnt - A .NET build tool
// Copyright (C) 2003-2015 Electronic Arts Inc.
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
using System.Globalization;
using System.Xml;

using Microsoft.VisualStudio.TestTools.UnitTesting;

using NAnt.Core.Util;
using NAnt.Tests.Util;

namespace EapmTests
{
	[TestClass]
	public class EapmTestBase : NAntTestClassWithDirectory
	{
		protected string RunEapm(params string[] args)
		{
			return RunEapm(0, args);
		}

		protected string RunEapm(int expectedReturnCode, params string[] args)
		{
			// If the machine has environment variable set, we need to change it so that it
			// doesn't interfere with test
			string oldFrameworkOnDemandRootEnvVar = Environment.GetEnvironmentVariable("FRAMEWORK_ONDEMAND_ROOT");
			if (oldFrameworkOnDemandRootEnvVar != null)
			{
				Environment.SetEnvironmentVariable("FRAMEWORK_ONDEMAND_ROOT", null);
			}
			try
			{
				TextWriter stdOutStream = Console.Out;
				using (StringWriter outputCapture = new StringWriter())
				{
					try
					{
						Console.SetOut(outputCapture);

						int returnCode = EA.PackageSystem.ConsolePackageManager.ConsolePackageManager.Main(args);
						Assert.AreEqual(expectedReturnCode, returnCode, Environment.NewLine + outputCapture.ToString());
					}
					finally
					{
						Console.SetOut(stdOutStream);
					}

					return outputCapture.ToString();
				}
			}
			finally
			{
				if (oldFrameworkOnDemandRootEnvVar != null)
				{
					Environment.SetEnvironmentVariable("FRAMEWORK_ONDEMAND_ROOT", oldFrameworkOnDemandRootEnvVar);
				}
			}
		}

		protected void CreateOndemandMetadata(string packageName, string packageVersion, string time)
		{
			File.WriteAllText(Path.Combine(m_testDir, "ondemand_metadata", packageName + "-" + packageVersion + ".metadata.xml"),
				string.Format("<package>" +
						"<name>{0}</name>" +
						"<version>{1}</version>" +
						"<directory>{2}</directory>" +
						"<referenced culture='{3}'>{4}</referenced>" +
					"</package>",
					packageName, packageVersion, Path.Combine(m_testDir, packageName, packageVersion), CultureInfo.CurrentCulture.Name, time));
		}

		protected XmlNode CreatePackageNode(XmlDocument doc, string name, string version)
		{
			XmlNode packageNode = doc.CreateElement("package");
			packageNode.SetAttribute("name", name);
			packageNode.SetAttribute("version", version);
			return packageNode;
		}

		protected void CreateMasterconfig(string directory, string name)
		{
			string masterconfigPath = Path.Combine(directory, name);

			if (File.Exists(masterconfigPath))
			{
				try
				{
					File.Delete(masterconfigPath);
				}
				catch (Exception) { } // not very important if we fail, but may help with robust if a corrupt masterconfig gets generated
			}

			XmlDocument doc = new XmlDocument();
			XmlNode rootNode = doc.CreateElement("project");

			XmlNode masterVersionsNode = doc.CreateElement("masterversions");
			masterVersionsNode.AppendChild(CreatePackageNode(doc, "mypackage", "test"));
			rootNode.AppendChild(masterVersionsNode);

			XmlNode packageRootsNode = doc.CreateElement("packageroots");
			XmlNode packageRootItemNode = doc.CreateElement("packageroot");
			packageRootItemNode.InnerText = directory;
			packageRootsNode.AppendChild(packageRootItemNode);
			rootNode.AppendChild(packageRootsNode);

			XmlNode ondemandNode = doc.CreateElement("ondemand");
			ondemandNode.InnerText = "false";
			rootNode.AppendChild(ondemandNode);

			XmlNode configNode = doc.CreateElement("config");
			configNode.SetAttribute("package", "eaconfig");

			rootNode.AppendChild(configNode);
			doc.AppendChild(rootNode);
			doc.Save(masterconfigPath);
		}

		protected static void ValidateInstall(string package, string version, string expectedDirectory, string eapmOutput)
		{
			string packageInstalledString = $"Package '{package}-{version}' installed";
			Assert.IsTrue(eapmOutput.Contains(packageInstalledString), $"Didn't see '{packageInstalledString}' in EAPM output:\n{eapmOutput}");
			Assert.IsTrue(Directory.Exists(expectedDirectory), "Package was not installed to expected location. EAPM output:\n" + eapmOutput);
		}
	}
}