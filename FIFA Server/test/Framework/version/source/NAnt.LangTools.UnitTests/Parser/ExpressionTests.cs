using System;

using Microsoft.VisualStudio.TestTools.UnitTesting;

namespace NAnt.LangTools.UnitTests
{
	[TestClass]
	public class ExpressionTests
	{
		[TestMethod]
		public void Empty()
		{
			Expression.Node node = Expression.Evaluate(String.Empty);
			Assert.IsInstanceOfType(node, typeof(Expression.String));
			Assert.IsTrue(ReferenceEquals(String.Empty, (node as Expression.String).Value));
		}

		[TestMethod]
		public void SimpleStringParse()
		{
			string simpleString = String.Format("a-simple-{0}", "string"); // format prevents reference to constant being used

			Expression.Node node = Expression.Evaluate(simpleString);
			Assert.IsInstanceOfType(node, typeof(Expression.String));
			Assert.IsTrue(ReferenceEquals(simpleString, (node as Expression.String).Value));
		}

		[TestMethod]
		public void LeadingAndTrailingWhiteSpace()
		{
			Expression.Node node = Expression.Evaluate("   leading-trailing        ");
			Assert.IsInstanceOfType(node, typeof(Expression.String));
			Assert.AreEqual("leading-trailing", (node as Expression.String).Value);
		}

		[TestMethod]
		public void AndSymbolOperator() => VerifySingleBinaryOperator
		(
			node: Expression.Evaluate("a&&b"),
			operation: Expression.Operation.And,
			firstOperand: "a",
			secondOperand: "b"
		);

		[TestMethod]
		public void AndWordOperator()=> VerifySingleBinaryOperator
		(
			node: Expression.Evaluate("a and b"),
			operation: Expression.Operation.And,
			firstOperand: "a",
			secondOperand: "b"
		);

		[TestMethod]
		public void OrSymbolOperator()=> VerifySingleBinaryOperator
		(
			node: Expression.Evaluate("a||b"),
			operation: Expression.Operation.Or,
			firstOperand: "a",
			secondOperand: "b"
		);

		[TestMethod]
		public void OrWordOperator()=> VerifySingleBinaryOperator
		(
			node: Expression.Evaluate("a OR b"),
			operation: Expression.Operation.Or,
			firstOperand: "a",
			secondOperand: "b"
		);

		[TestMethod]
		public void UnquotedUnquotedStringEquality() => VerifySingleBinaryOperator
		(
			node: Expression.Evaluate("alpha == beta"),
			operation: Expression.Operation.Equals,
			firstOperand: "alpha",
			secondOperand: "beta"
		);

		[TestMethod]
		public void UnquotedQuotedStringEquality() => VerifySingleBinaryOperator
		(
			node: Expression.Evaluate("alpha == 'beta'"),
			operation: Expression.Operation.Equals,
			firstOperand: "alpha",
			secondOperand: "beta"
		);

		[TestMethod]
		public void QuotedUnquotedStringEquality() => VerifySingleBinaryOperator
		(
			node: Expression.Evaluate("'alpha' == beta"),
			operation: Expression.Operation.Equals,
			firstOperand: "alpha",
			secondOperand: "beta"
		);

		[TestMethod]
		public void QuotedQuotedStringEquality() => VerifySingleBinaryOperator
		(
			node: Expression.Evaluate("'alpha' == 'beta'"),
			operation: Expression.Operation.Equals,
			firstOperand: "alpha",
			secondOperand: "beta"
		);

		[TestMethod]
		public void PrecedenceTest()
		{
			Expression.Node element = Expression.Evaluate("a && b && c || d");
			Assert.IsInstanceOfType(element, typeof(Expression.BinaryOperator));
			{
				Expression.BinaryOperator binaryOperator = element as Expression.BinaryOperator;
				Assert.AreEqual(Expression.Operation.Or, binaryOperator.Operation);

				Assert.IsInstanceOfType(binaryOperator.FirstOperand, typeof(Expression.BinaryOperator));
				{
					Expression.BinaryOperator firstNestedBinaryOperator = binaryOperator.FirstOperand as Expression.BinaryOperator;
					Assert.AreEqual(Expression.Operation.And, firstNestedBinaryOperator.Operation);

					Assert.IsInstanceOfType(firstNestedBinaryOperator.FirstOperand, typeof(Expression.String));
					Assert.AreEqual("a", (firstNestedBinaryOperator.FirstOperand as Expression.String).Value);

					Assert.IsInstanceOfType(firstNestedBinaryOperator.SecondOperand, typeof(Expression.BinaryOperator));
					{
						Expression.BinaryOperator secondNestedBinaryOperator = firstNestedBinaryOperator.SecondOperand as Expression.BinaryOperator;
						Assert.AreEqual(Expression.Operation.And, secondNestedBinaryOperator.Operation);

						Assert.IsInstanceOfType(secondNestedBinaryOperator.FirstOperand, typeof(Expression.String));
						Assert.AreEqual("b", (secondNestedBinaryOperator.FirstOperand as Expression.String).Value);

						Assert.IsInstanceOfType(secondNestedBinaryOperator.SecondOperand, typeof(Expression.String));
						Assert.AreEqual("c", (secondNestedBinaryOperator.SecondOperand as Expression.String).Value);
					}
				}

				Assert.IsInstanceOfType(binaryOperator.SecondOperand, typeof(Expression.String));
				Assert.AreEqual("d", (binaryOperator.SecondOperand as Expression.String).Value);
			}
		}

		// TODO more test
		// TODO invalid expression tests

		private static void VerifySingleBinaryOperator(Expression.Node node, Expression.Operation operation, string firstOperand, string secondOperand)
		{
			Assert.IsInstanceOfType(node, typeof(Expression.BinaryOperator));

			Expression.BinaryOperator binaryOperator = node as Expression.BinaryOperator;
			Assert.AreEqual(operation, binaryOperator.Operation);

			Assert.IsInstanceOfType(binaryOperator.FirstOperand, typeof(Expression.String));
			Assert.AreEqual(firstOperand, (binaryOperator.FirstOperand as Expression.String).Value);

			Assert.IsInstanceOfType(binaryOperator.SecondOperand, typeof(Expression.String));
			Assert.AreEqual(secondOperand, (binaryOperator.SecondOperand as Expression.String).Value);
		}
	}
}