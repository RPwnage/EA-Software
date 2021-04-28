using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.IO;
using System.Linq;
using System.Reflection;

using Microsoft.VisualStudio.TestTools.UnitTesting;

namespace NAnt.LangTools.UnitTests
{
	internal static class TestUtils
	{
		private static readonly ReadOnlyCollection<string> s_boolVals = new ReadOnlyCollection<string>(new string[] { "true", "false" });

		internal static Condition CreateBooleanCondition(string conditionName)
		{
			return new Condition(conditionName, s_boolVals);
		}

		internal static void VerifyPropertyExpansionConditionSets(Dictionary<string, ConditionValueSet> expansion, string expandedValue, params Dictionary<Condition, string>[] verifyValues)
		{
			Assert.IsTrue(expansion.ContainsKey(expandedValue));
			if (!verifyValues.Any())
			{
				Assert.AreEqual(ConditionValueSet.True, expansion[expandedValue]);
			}
			else
			{
				Assert.IsTrue(expansion[expandedValue].Count() == verifyValues.Count());
				foreach (ReadOnlyDictionary<Condition, string> conditionSet in expansion[expandedValue])
				{
					Dictionary<Condition, string> match = verifyValues.FirstOrDefault(value => !conditionSet.Except(value).Any());
					Assert.IsTrue(match != null, "No matching condition set.");
					foreach (KeyValuePair<Condition, string> kvp in match)
					{
						Assert.IsTrue(conditionSet.ContainsKey(kvp.Key));
						Assert.AreEqual(kvp.Value, conditionSet[kvp.Key]);
					}
				}
			}
		}
	}

	public class ParserTestsBase
	{
		private string m_testDir;

		[TestInitialize]
		public void TestSetup()
		{
			m_testDir = CreateTestDirectory(GetType().Name);
		}

		[TestCleanup]
		public void TestCleanUp()
		{
			if (Directory.Exists(m_testDir))
			{
				Directory.Delete(m_testDir, recursive: true);
			}
		}

		protected Parser ParsePackage(string package, string buildScript, string initializeXml, params Feature[] features)
		{
			string initializePath = Path.Combine(m_testDir, "Initialize.xml");
			File.WriteAllText(initializePath, initializeXml);
			return ParsePackage(package, buildScript, features);
		}

		protected Parser ParsePackage(string package, string buildScript, params Feature[] features)
		{
			string buildPath = Path.Combine(m_testDir, $"{package}.build");
			File.WriteAllText(buildPath, buildScript);
			return Parser.Parse(package, buildPath, features);
		}

		protected Parser ParseSnippet(string contents, params Feature[] features) => ParsePackage("Test", contents, features);

		protected static string Script(params string[] contents)
		{
			return String.Join(Environment.NewLine, contents);
		}

		private static string CreateTestDirectory(string directoryName)
		{
			string testDir = Path.GetFullPath(Path.Combine
			(
				Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location),
				"../../../Local",
				directoryName
			));
			if (Directory.Exists(testDir))
			{
				Directory.Delete(testDir, recursive: true);
			}
			Directory.CreateDirectory(testDir);
			return testDir;
		}
	}
}