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
using System.Collections.Generic;

using NAnt.Core;
using NAnt.Core.Writers;


namespace EA.Make.MakeItems
{
	// Syntax for a make file rule looks like this:
	//
	//    targets : normal-prerequisites | order-only-prerequisites
	//        recipes
	//
	// "targets" are usually filenames separated by spaces.  
	//     Note that if target name is not a file path, this target name should be listed in the .PHONY list and if this
	//     target name is used as prerequistes, they should be listed as order-only-prerequisites (see below).  Otherwise make will attempt to 
	//     test for presense of this "file" during incremental build.  Note that even though it is allowed to list multiple target name
	//     here, it is not recommended as each target name being listed will execute the exact same "recipes" so you will see exact same "identical"
	//     command line being executed multiple times (once for each target).
	// "normal prerequisites" are basically other file targets. So during build, make will test for up-to-date of that file 
	//     and trigger it's recipe if necessary. 
	// "order-only-prerequisites" on the other hand are usually just a target label (PHONY targets).  The name itself is not a file, so there
	//     will be no "file" up-to-date check of that target label and just execute that target (ie test that target's prerequisites and execute
	//     that target's receipes if necessary).
	// "receipes" are basically your command line build instructions
	//
	// Example:
	//
	// .PHONY: gen-header
	// 
	// build/sourcefile.obj: source/sourcefile.cpp | gen-header
	//    clang++ -c -I gendir -o build/sourcefile.obj source/sourcefile.cpp
	//
	// gen-header: source/gen-header.dat
	//    cp source/gen-header.dat gendir/sourcefile.h
	//

	public class MakeTarget : MakeItem
	{
		public string Target
		{
			get 
			{
				return Label;
			}
		}

		public MakeStringCollection Prerequisites
		{
			get 
			{
				if (_prerequisites == null)
				{
					_prerequisites = new MakeStringCollection();
				}
				return _prerequisites;
			}
			set
			{
				_prerequisites = value;
			}
		}
		private MakeStringCollection _prerequisites;

		public MakeStringCollection OrderOnlyPrerequisites
		{
			get
			{
				if (_order_only_prerequisites == null)
				{
					_order_only_prerequisites = new MakeStringCollection();
				}
				return _order_only_prerequisites;
			}
			set
			{
				_order_only_prerequisites = value;
			}

		}
		private MakeStringCollection _order_only_prerequisites;

		public List<string> Rules { get; } = new List<string>();

		public MakeTarget(string label) : base(label)
		{
		}

		public void AddRuleLine(string arg)
		{
			Rules.Add(arg);
		}

		public void AddRuleLine(string format, params object[] args)
		{
			Rules.Add(String.Format(format, args));
		}

		public void AddRuleCommentLine(string arg)
		{
			Rules.Add("@echo " + arg);
		}

		public void AddRuleCommentLine(string format, params object[] args)
		{
			Rules.Add(String.Format("@echo " + format, args));
		}

		public void WrapRulesWithCondition(string condition)
		{
			if (Rules != null)
			{
				string continuation = " \\";
				string commandSeparator = "";
				string extraIndent = "\t";	// For adding indentation for rules inside new if block!
				string ifStatementStart="";
				string ifStatementEnd = "";
				Dictionary<string, string> escapeBracketsList = new Dictionary<string, string>();
				if (NAnt.Core.Util.PlatformUtil.IsWindows)
				{
					commandSeparator = "&";
					ifStatementStart = String.Format("@if {0} ( {1}", condition, continuation);
					ifStatementEnd = ")";
					// Because Windows uses brackets to group commands, if the existing command already contains 
					// brackets, we need to escape them (only if it is not already in an if-block).
					escapeBracketsList.Add("(", "^(");
					escapeBracketsList.Add(")", "^)");
				}
				else
				{
					// Shell command for if statment on unix style machine is different from Windows
					commandSeparator = ";";
					ifStatementStart = String.Format("@if [ {0} ]; then {1}", condition.Replace("==", "="), continuation);
					ifStatementEnd = "fi";
				}

				// Under linux/osx bash shell, this is actually a command "terminator" not separator,
				// so we need to add this for every line!
				int cmdSeparatorCount = (NAnt.Core.Util.PlatformUtil.IsWindows) ? Rules.Count - 1 : Rules.Count;
				bool existingRuleInIfBlock = false;
				// Add indentation to current set of rules for the if block and do any necessary character escape,
				// add command separator/terminator symbol and add command continuation symbol for each existing rule.
				for (int ruleIdx = 0; ruleIdx < Rules.Count; ++ruleIdx)
				{
					string currentTrimmedRule = Rules[ruleIdx].Trim();

					// Under bash on linux/osx, we cannot have command starting with '@' inside if-block.
					// So strip them if they are there.
					if (currentTrimmedRule.StartsWith("@") && !NAnt.Core.Util.PlatformUtil.IsWindows)
					{
						int charIdx = Rules[ruleIdx].IndexOf("@");
						Rules[ruleIdx] = Rules[ruleIdx].Remove(charIdx, 1);
						currentTrimmedRule = Rules[ruleIdx].Trim();
					}
					Rules[ruleIdx] = extraIndent + Rules[ruleIdx];

					// If the rules contains any brackets, we need to escape them because the if block uses
					// brackets to group it.  However, if the rule itself is already an if block, we don't need to
					// do anything.
					if (!existingRuleInIfBlock)
					{
						existingRuleInIfBlock = currentTrimmedRule.StartsWith("if ") || currentTrimmedRule.StartsWith("@if ");
					}
					if (!existingRuleInIfBlock)
					{
						// Perform character escape for the if-brackets if necessary
						foreach (KeyValuePair<string, string> escapeItem in escapeBracketsList)
						{
							Rules[ruleIdx] = Rules[ruleIdx].Replace(escapeItem.Key, escapeItem.Value);
						}
					}
					else
					{
						if (currentTrimmedRule.EndsWith(ifStatementEnd))
						{
							existingRuleInIfBlock = false;
						}
					}

					// Add commandSeparator and continuation symbol for all existing rules in if block
					// (unless contunation symbol is already there).
					if (!currentTrimmedRule.EndsWith(continuation))
					{
						if (ruleIdx < cmdSeparatorCount)
						{
							// On windows, we only add this if we're not on the last line.
							// On osx/linux bash, we need this for everyline.
							Rules[ruleIdx] += commandSeparator;
						}
						Rules[ruleIdx] += continuation;
					}
				}

				// Now wrap current rules with if-block
				Rules.Insert(0, ifStatementStart);
				Rules.Add(ifStatementEnd);
			}
		}

		public override void Write(MakeWriter writer)
		{
			if (!String.IsNullOrEmpty(Target))
			{
				if (_order_only_prerequisites != null && _order_only_prerequisites.Count > 0)
				{
					writer.WriteLine("{0}: {1} | {2}", Target, _prerequisites.ToString(" "), _order_only_prerequisites.ToString(" "));
				}
				else
				{
					writer.WriteLine("{0}: {1}", Target ?? Label, _prerequisites.ToString(" "));
				}

				if (Rules != null)
				{
					foreach (var rule in Rules)
					{
						writer.WriteTabLine(rule);
					}
				}
				writer.WriteLine();
			}
		}

	}
}
