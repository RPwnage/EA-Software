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

using Microsoft.VisualStudio.TestTools.UnitTesting;

using NAnt.Core;
using NAnt.Core.Logging;

using EA.Eaconfig;

namespace EA.Tasks.Tests
{
	// This is a test for the script init task
	[TestClass]
	public class ScriptInit
	{
		[TestMethod]
		public void ImplicitScriptInitTest()
		{
			Project project = new Project(Log.Silent);
			{
				project.Properties.Add("package.mypackage.dir", "C:/packages/mypackage/dev");
				project.Properties.Add("package.mypackage.version", "dev");
				project.Properties.Add("package.mypackage.builddir", "C:/packages/mypackage/dev/build");
				project.Properties.Add("config", "pc-vc-debug");
				project.Properties.Add("lib-prefix", "lib");
				project.Properties.Add("lib-suffix", ".lib");

				ScriptInitTask scriptInitTask = new ScriptInitTask(project, packageName: "mypackage");
				scriptInitTask.Execute();

				Assert.AreEqual(project.Properties["package.mypackage.includedirs"], "C:/packages/mypackage/dev/include");

				string defaultLibPath = "C:/packages/mypackage/dev/build/pc-vc-debug/lib/libmypackage.lib";
				Assert.AreEqual(project.Properties["package.mypackage.default.scriptinit.lib"], defaultLibPath);

				FileSet libs = project.NamedFileSets["package.mypackage.libs"];
				Assert.IsTrue(libs.Includes.Count == 1);
				Assert.AreEqual(libs.Includes[0].ToString(), defaultLibPath);
			}
		}

		[TestMethod]
		public void ExplicitScriptInitTest()
		{
			Project project = new Project(Log.Silent);
			{
				project.Properties.Add("package.mypackage.dir", "C:/packages/mypackage/dev");
				project.Properties.Add("package.mypackage.version", "dev");
				project.Properties.Add("package.mypackage.builddir", "C:/packages/mypackage/dev/build");
				project.Properties.Add("config", "pc-vc-debug");
				project.Properties.Add("lib-prefix", "lib");
				project.Properties.Add("lib-suffix", ".lib");

				ScriptInitTask scriptInitTask = new ScriptInitTask(project, packageName: "mypackage", includeDirs: "custom-include-dir", libs: "custom.lib");
				scriptInitTask.Execute();

				Assert.AreEqual(project.Properties["package.mypackage.includedirs"], "custom-include-dir");
				FileSet libs = project.NamedFileSets["package.mypackage.libs"];
				Assert.IsTrue(libs.Includes.Count == 1);
				Assert.AreEqual(libs.Includes[0].ToString(), "custom.lib");
			}
		}
	}
}