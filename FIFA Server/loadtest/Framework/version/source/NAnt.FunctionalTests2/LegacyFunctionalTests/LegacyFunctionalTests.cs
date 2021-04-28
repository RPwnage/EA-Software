using Microsoft.VisualStudio.TestTools.UnitTesting;

namespace NAnt.FunctionalTests2
{
	// Test function names listed here correspond to the directory name of the test. There *are* mechanisms to have
	// tests driven by data but the Visual Studio test explorer cannot enumerate them so opting instead to list them
	// here explicitly.

	[TestClass]
	public class LegacyFunctionalTests : FunctionalTestBase
	{
#pragma warning disable IDE1006 // Naming Styles
		[TestMethod] public void analyze() => RunFunctionalTest();
#pragma warning restore IDE1006 // Naming Styles
		[TestMethod] public void NuGetTest() => RunFunctionalTest();
	}
}
