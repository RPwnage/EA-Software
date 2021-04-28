using Microsoft.VisualStudio.TestTools.UnitTesting;

namespace NAnt.FunctionalTests2
{
	// Test function names listed here correspond to the directory name of the test. There *are* mechanisms to have
	// tests driven by data but the Visual Studio test explorer cannot enumerate them so opting instead to list them
	// here explicitly.

	[TestClass]
	public class AndroidTests : FunctionalTestBase
	{
		[TestMethod] public void AndroidApk() => RunFunctionalTest();
		// [TestMethod] public void AndroidApkWithJavaDependency() => RunFunctionalTest(); // TODO
		// [TestMethod] public void AndroidApkWithNativeDependency() => RunFunctionalTest(); // TODO
		[TestMethod] public void AndroidApkWithFireBaseClassPath() => RunFunctionalTest(); // TODO
		[TestMethod] public void AndroidApkWithGradleProperties() => RunFunctionalTest(); // TODO
		// [TestMethod] public void ApkFromProgramModule() => RunFunctionalTest(); // TODO
		[TestMethod] public void ProgramPrebuiltTransitiveNative() => RunFunctionalTest();

		[TestMethod] public void AarLibrary() => RunFunctionalTest();
		[TestMethod] public void AarLibraryWithJavaDependency() => RunFunctionalTest();
		[TestMethod] public void AarLibraryWithAarDependency() => RunFunctionalTest();
		[TestMethod] public void AarLibraryWithNativeDependency() => RunFunctionalTest();
		[TestMethod] public void AarLibraryLooseManifest() => RunFunctionalTest();
	}
}