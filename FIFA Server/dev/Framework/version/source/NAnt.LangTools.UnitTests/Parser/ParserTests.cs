using Microsoft.VisualStudio.TestTools.UnitTesting;

namespace NAnt.LangTools.UnitTests
{
	[TestClass]
	public class ParserTests : ParserTestsBase
	{
		[TestMethod]
		public void Empty()
		{
			ParseSnippet("<project/>");
		}
	}
}