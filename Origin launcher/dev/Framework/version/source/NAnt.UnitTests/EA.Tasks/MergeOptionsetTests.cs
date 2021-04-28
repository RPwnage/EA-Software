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
	[TestClass]
	public class MergeOptionsetTests
	{
		[TestMethod]
		public void MergeOptionsetTest()
		{
			Project project = new Project(Log.Silent);
			{
				OptionSet base_optionset = new OptionSet();
				base_optionset.Options.Add("s1", "source one");
				base_optionset.Options.Add("s2", "source two");
				base_optionset.Options.Add("shared", "shared_source");
				project.NamedOptionSets["base_optionset"] = base_optionset;

				OptionSet main_optionset = new OptionSet();
				main_optionset.Options.Add("o1", "orig one");
				main_optionset.Options.Add("shared", "shared_orig");
				project.NamedOptionSets["main_optionset"] = main_optionset;

				MergeOptionset.Execute(project, "main_optionset", "base_optionset");
				Assert.AreEqual(main_optionset.Options["o1"], "orig one");
				Assert.AreEqual(main_optionset.Options["shared"], "shared_orig shared_source");
				Assert.AreEqual(main_optionset.Options["s1"], "source one");
				Assert.AreEqual(main_optionset.Options["s2"], "source two");

				Assert.ThrowsException<ContextCarryingException>(() => MergeOptionset.Execute(project, "main_optionset", "I_DONT_EXIST"));
				Assert.ThrowsException<ContextCarryingException>(() => MergeOptionset.Execute(project, "I_DONT_EXIST", "base_optionset"));
				Assert.ThrowsException<ContextCarryingException>(() => MergeOptionset.Execute(project, "I_DONT_EXIST", "I_DONT_EXIST"));
			}
		}
	}
}