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
using System.Collections.Generic;
using System.IO;

using Microsoft.VisualStudio.TestTools.UnitTesting;

using NAnt.Core.Logging;
using NAnt.Core.PackageCore;
using NAnt.Core.Tasks;
using NAnt.Tests.Util;

namespace NAnt.Core.Tests
{
	// Tests for misc parts of NAnt.Core functionality to boost code coverage
	[TestClass]
	public class NAntTaskTests : NAntTestClassBase
	{
		[TestMethod]
		public void GlobalPropertyPropagationTest()
		{
			Project topLevelProject = new Project(Log.Silent);
			InitializePackageMapMasterConfig
			(
				topLevelProject,
				"<project>",
				"	<globalproperties>",
				"		testglobalproperty", // global property with no value
				"	</globalproperties>",
				"	<config package='eaconfig'/>",
				"</project>"
			);

			// property should still be valueless
			Assert.AreEqual(topLevelProject.Properties["testglobalproperty"], null);

			topLevelProject.Properties["testglobalproperty"] = "top_level_value"; // should be writable in top level
			Assert.IsFalse(topLevelProject.Properties.IsPropertyReadOnly("testglobalproperty"));

			// create a sub project via nant task
			string buildFile = Path.Combine(Environment.CurrentDirectory, "nested.build");
			SeedXmlDocumentCache
			(
				buildFile,
				"<project/>"
			);
			NAntTask nantTask = new NAntTask()
			{
				Project = topLevelProject,
				BuildFileName = buildFile,
			};
			nantTask.DoInitialize();
			nantTask.Execute();

			nantTask.NestedProject.Properties["testglobalproperty"] = "nested_level_value"; // this shouldn't change the property
			Assert.IsTrue(nantTask.NestedProject.Properties.IsPropertyReadOnly("testglobalproperty"));
			Assert.AreEqual("top_level_value", nantTask.NestedProject.Properties["testglobalproperty"]);
		}

		[TestMethod]
		public void ConditionalGlobalPropertyPropagation()
		{
			Project topLevelProject = new Project(Log.Silent);
			InitializePackageMapMasterConfig
			(
				topLevelProject,
				"<project>",
				"	<globalproperties>",
				"		<if condition='${test-condition} == true'>",
				"			test-property=test-value",
				"		</if>",
				"	</globalproperties>",
				"	<config package='eaconfig'/>",
				"</project>"
			);
			topLevelProject.Properties["test-condition"] = "false";

			Assert.AreEqual(null, topLevelProject.Properties["test-property"]); // this should have no value at top level, because masterconfig condition is false

			string emptyBuildFile = Path.Combine(Environment.CurrentDirectory, "empty.build");
			SeedXmlDocumentCache
			(
				emptyBuildFile,
				"<project/>"
			);

			{
				NAntTask nantTaskNonGlobalNoProp = new NAntTask()
				{
					Project = topLevelProject,
					BuildFileName = emptyBuildFile
				};
				nantTaskNonGlobalNoProp.DoInitialize();
				nantTaskNonGlobalNoProp.Execute();

				// nothing change from parent context so this should have no value
				Assert.AreEqual(null, nantTaskNonGlobalNoProp.NestedProject.Properties["test-property"]);
			}

			{
				NAntTask nantTaskNonGlobalPropArg = new NAntTask()
				{
					Project = topLevelProject,
					BuildFileName = emptyBuildFile
				};
				nantTaskNonGlobalPropArg.TaskProperties["test-condition"] = "true";
				nantTaskNonGlobalPropArg.DoInitialize();
				nantTaskNonGlobalPropArg.Execute();

				// the test conditiion property isn't global so the child gets to use it's own value when evaluating the masterconfig and gets test-value
				Assert.AreEqual("test-value", nantTaskNonGlobalPropArg.NestedProject.Properties["test-property"]);
			}

			{
				// this time set test-condition in top level before calling
				topLevelProject.Properties["test-condition"] = "true";
				NAntTask nantTaskNonGlobalParentProp = new NAntTask()
				{
					Project = topLevelProject,
					BuildFileName = emptyBuildFile
				};
				nantTaskNonGlobalParentProp.DoInitialize();
				nantTaskNonGlobalParentProp.Execute();

				// so this is the slightly wierd case - logically test-property isn't global so it shouldn't be passed down
				// or have have an initial value from masterconfig however initialization of global properties in nant happens
				// in *parent* context if there is no evaluated value in child. This is legacy behaviour and risky to change so
				// we validate it here
				Assert.AreEqual("test-value", nantTaskNonGlobalParentProp.NestedProject.Properties["test-property"]); 
			}

			// run the same two tests again but this time test property is global with a starting valie
			Project.GlobalProperties.Add("test-property", "global-test-value");
			topLevelProject.Properties["test-property"] = "global-test-value";
			topLevelProject.Properties.Remove("test-condition");

			{
				NAntTask nantTaskOGlobalPropArg = new NAntTask()
				{
					Project = topLevelProject,
					BuildFileName = emptyBuildFile
				};
				nantTaskOGlobalPropArg.TaskProperties["test-condition"] = "true";
				nantTaskOGlobalPropArg.DoInitialize();
				nantTaskOGlobalPropArg.Execute();

				// so this gets inherit from parent now it is global so it has parent value
				Assert.AreEqual("global-test-value", nantTaskOGlobalPropArg.NestedProject.Properties["test-property"]);
			}

			{
				// this time set test-condition in top level before calling
				topLevelProject.Properties["test-condition"] = "true";
				NAntTask nantTaskGlobalParentProp = new NAntTask()
				{
					Project = topLevelProject,
					BuildFileName = emptyBuildFile
				};
				nantTaskGlobalParentProp.DoInitialize();
				nantTaskGlobalParentProp.Execute();

				Assert.AreEqual("global-test-value", nantTaskGlobalParentProp.NestedProject.Properties["test-property"]); // this still inherits parent value
			}
		}

		[TestMethod]
		public void NAntTaskParameterPropagationTest()
		{
			Project topLevelProject = new Project(Log.Silent);
			InitializePackageMapMasterConfig
			(
				topLevelProject,
				"<project>",
				"	<globalproperties>",
				"		implicit-global-propagate",
				"		explicit-global-propagate",
				"	</globalproperties>",
				"	<config package='eaconfig'/>",
				"</project>"
			);

			topLevelProject.Properties["implicit-global-propagate"] = "implicit-global-propagate";

			// nant task doesn't offer C# api for adding properties
			// so we have to give it an xml stub to set properties
			string buildFileOne = Path.Combine(Environment.CurrentDirectory, "nestedOne.build");
			SeedXmlDocumentCache
			(
				buildFileOne,
				"<project/>"
			);
			NAntTask nantTaskOne = new NAntTask()
			{
				Project = topLevelProject,
				BuildFileName = buildFileOne
			};
			nantTaskOne.TaskProperties["propagate"] = "propagate";
			nantTaskOne.TaskProperties["explicit-global-propagate"] = "explicit-global-propagate";
			nantTaskOne.DoInitialize();
			nantTaskOne.Execute();

			// test proeprty was set in nested project
			Assert.AreEqual("propagate", nantTaskOne.NestedProject.Properties["propagate"]); // explicitly propagated
			Assert.AreEqual("explicit-global-propagate", nantTaskOne.NestedProject.Properties["explicit-global-propagate"]); // this is a global property and this is propagated
			Assert.AreEqual("implicit-global-propagate", nantTaskOne.NestedProject.Properties["implicit-global-propagate"]); // this is a global property and this is propagated
			
			// call nant task from nant task project
			string buildFileTwo = Path.Combine(Environment.CurrentDirectory, "nestedTwo.build");
			SeedXmlDocumentCache
			(
				buildFileTwo,
				"<project/>"
			);
			NAntTask nantTaskTwoGlobalPropagate = new NAntTask()
			{
				Project = nantTaskOne.NestedProject,
				BuildFileName = buildFileTwo,
				GlobalPropertiesAction = NAntTask.GlobalPropertiesActionTypes.propagate
			};
			nantTaskTwoGlobalPropagate.DoInitialize();
			nantTaskTwoGlobalPropagate.Execute();

			Assert.AreEqual(null, nantTaskTwoGlobalPropagate.NestedProject.Properties["propagate"]); // non global property is NOT propagated - this is not new "initialized" build
			Assert.AreEqual("explicit-global-propagate", nantTaskTwoGlobalPropagate.NestedProject.Properties["explicit-global-propagate"]); // this is a global property and this is propgated
			Assert.AreEqual("implicit-global-propagate", nantTaskTwoGlobalPropagate.NestedProject.Properties["implicit-global-propagate"]); // this is a global property and this is propgated

			NAntTask nantTaskTwoGlobalInitialize = new NAntTask()
			{
				Project = nantTaskOne.NestedProject,
				BuildFileName = buildFileTwo,
				GlobalPropertiesAction = NAntTask.GlobalPropertiesActionTypes.initialize
			};
			nantTaskTwoGlobalInitialize.DoInitialize();
			nantTaskTwoGlobalInitialize.Execute();

			// test property was propagated to doubly nested project even though isn't wasn't
			// explicitly set for the second level
			Assert.AreEqual("propagate", nantTaskTwoGlobalInitialize.NestedProject.Properties["propagate"]); // this is propagated *BECAUSE* this new "initialized" build, and it was explicitly propgated at higher level nant task
			Assert.AreEqual("explicit-global-propagate", nantTaskTwoGlobalInitialize.NestedProject.Properties["explicit-global-propagate"]); // this is global, but it was also explicitly propgated and thus should be set
			Assert.AreEqual(null, nantTaskTwoGlobalInitialize.NestedProject.Properties["implicit-global-propagate"]); // this is a global property that was never propagated and so it unset in new build
		}

		[TestMethod]
		public void NAntTaskPropertyOptionsetPrecedence()
		{
			Project topLevelProject = new Project(Log.Silent);
			InitializePackageMapMasterConfig
			(
				topLevelProject,
				"<project>",
				"	<globalproperties>",
				"		testPropThree",
				"	</globalproperties>",
				"	<config package='eaconfig'/>",
				"</project>"
			);

			topLevelProject.Properties["testPropOne"] = "toplevel-value";
			topLevelProject.Properties["testPropTwo"] = "toplevel-value";
			topLevelProject.Properties["testPropThree"] = "toplevel-value";
			topLevelProject.NamedOptionSets["testOptions"] = new OptionSet();
			topLevelProject.NamedOptionSets["testOptions"].Options["testPropOne"] = "optionset-value";
			topLevelProject.NamedOptionSets["testOptions"].Options["testPropTwo"] = "optionset-value";

			string buildFile = Path.Combine(Environment.CurrentDirectory, "nested.build");
			SeedXmlDocumentCache
			(
				buildFile,
				"<project/>"
			);
			NAntTask nantTask = new NAntTask()
			{
				Project = topLevelProject,
				OptionSetName = "testOptions",
				BuildFileName = buildFile
			};
			nantTask.TaskProperties["testPropOne"] = "property-value";
			nantTask.DoInitialize();
			nantTask.Execute();

			Assert.AreEqual("property-value", nantTask.NestedProject.Properties["testPropOne"]); // this should have the value from the task property, not the option set and definitely not the parent project
			Assert.AreEqual("optionset-value", nantTask.NestedProject.Properties["testPropTwo"]); // this should be from optionset
			Assert.AreEqual("toplevel-value", nantTask.NestedProject.Properties["testPropThree"]); // this should be inherted from top level because it is global
		}
	}
}