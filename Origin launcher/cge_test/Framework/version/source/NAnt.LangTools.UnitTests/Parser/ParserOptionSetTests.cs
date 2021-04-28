using System.Linq;

using Microsoft.VisualStudio.TestTools.UnitTesting;

namespace NAnt.LangTools.UnitTests
{
	[TestClass]
	public class ParserOptionSetTests : ParserTestsBase
	{
		[TestMethod]
		public void EmptyOptionSet()
		{
			Parser parser = ParseSnippet
			(
				Script("<project>",
				"    <optionset name=\"empty-set\"/>",
				"</project>")
			);

			Assert.IsTrue(parser.OptionSets.ContainsKey("empty-set"));
			Assert.IsFalse(parser.OptionSets["empty-set"].OptionSet.Any());
		}

		[TestMethod]
		public void SingleOptionOptionSet()
		{
			Parser parser = ParseSnippet
			(
				Script("<project>",
				"    <optionset name=\"single-set\">",
				"		<option name=\"option\" value=\"value\"/>",
				"    </optionset>",
				"</project>")
			);

			Assert.IsTrue(parser.OptionSets.ContainsKey("single-set"));
			OptionSet.Map.Entry singleSet = parser.OptionSets["single-set"];
			Assert.IsTrue(singleSet.OptionSet.ContainsKey("option"));

			Property.Node value = singleSet.OptionSet["option"];
			Assert.IsInstanceOfType(value, typeof(Property.String));
			Assert.AreEqual("value", (value as Property.String).Value);
		}
	}
}