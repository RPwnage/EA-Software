using Microsoft.VisualStudio.TestTools.UnitTesting;

namespace NAnt.FunctionalTests2
{
	// Test function names listed here correspond to the directory name of the test. There *are* mechanisms to have
	// tests driven by data but the Visual Studio test explorer cannot enumerate them so opting instead to list them
	// here explicitly.

	[TestClass]
	public class LinkingTests : FunctionalTestBase
	{
		[TestMethod] public void StadiaTransitiveSo() => RunFunctionalTest();
	}
}
