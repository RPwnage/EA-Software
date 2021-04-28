using Microsoft.VisualStudio.TestTools.UnitTesting;

namespace NAnt.Core.Tests
{
	[TestClass]
	public class ExpressionTests
	{
		[TestMethod]
		public void WhiteSpaceTest()
		{
			Assert.IsTrue(Expression.Evaluate(@"   
true 
		AND     

				true
    "));

			Assert.IsTrue(Expression.Evaluate(@"

	  '
                    floating-string       
          '    ==          '
                    floating-string       
          '         "));
		}
	}
}
