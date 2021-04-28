using System;

using Microsoft.VisualStudio.TestTools.UnitTesting;

using NAnt.Core.Util;

namespace NAnt.Core.Tests
{
	[TestClass]
	public class MacroMapTests
	{
		[TestMethod] public void NoReplaceTest()
		{
			string replaceString = "nochange";
			string returnString = MacroMap.Replace(replaceString, new MacroMap() { { "test", "replace" } });

			Assert.AreEqual(replaceString, returnString);

			Assert.IsTrue(ReferenceEquals(replaceString, returnString)); // if there was nothing to replace we should get the same reference back
		}

		[TestMethod] public void BasicTest() => Assert.AreEqual("replace", MacroMap.Replace("%test%", new MacroMap { { "test", "replace" } }));
		[TestMethod] public void LeadingTrailingTest() => Assert.AreEqual("leading replace trailing", MacroMap.Replace("leading %test% trailing", new MacroMap { { "test", "replace" } }));
		[TestMethod] public void EscapeTest() => Assert.AreEqual("%", MacroMap.Replace("%%", new MacroMap() { { "test", "replace" } }));

		[TestMethod] public void MultipleReplaceTest()
		{
			Assert.AreEqual
			(
				"onetwothreefourfive",
				MacroMap.Replace
				(
					"%1%two%3%four%5%",
					new MacroMap()
					{
						{ "1", "one" },
						{ "3", "three" },
						{ "5", "five" }
					}
				)
			);
		}

		[TestMethod] public void InvalidNullReplacement()
		{
			string ReturnNull()
			{
				return null;
			}

			ArgumentException argNullEx = Assert.ThrowsException<ArgumentException>(() => MacroMap.Replace("%null%", new MacroMap() { { "null", ReturnNull } }));
			Assert.AreEqual
			(
				"Error evaluating %null% at offset 0 in replacement string '%null%': replacment value was null.",
				argNullEx.Message
			);
		}

		[TestMethod] public void MultipleMapTest()
		{
			Assert.AreEqual
			(
				"firstOne secondTwo",
				MacroMap.Replace
				(
					"%one% %two%",
					new MacroMap
					(
						inherit: new MacroMap()
						{
							{ "one", "firstTwo" },
							{ "two", "secondTwo" }
						}
					)
					{
						{ "one", "firstOne" }
					}
				)
			);
		}

		[TestMethod] public void ReverseMultiMapTest()
		{
			Assert.AreEqual
			(
				"secondTwo firstOne",
				MacroMap.Replace
				(
					"%two% %one%",
					new MacroMap
					(
						inherit: new MacroMap()
						{
							{ "one", "firstTwo" },
							{ "two", "secondTwo" }
						}
					)
					{
						{ "one", "firstOne" }
					}
				)
			);
		}

		[TestMethod] public void LazyFunctionTest()
		{
			bool lazyCalled = false;

			string Lazy()
			{
				lazyCalled = true;
				return "lazyReplace";
			}

			MacroMap lazyMap = new MacroMap()
			{
				{ "lazy", Lazy },
				{ "test", "replace" }
			};

			Assert.AreEqual("replace", MacroMap.Replace("%test%", lazyMap));
			Assert.IsFalse(lazyCalled); // lazy token did not appear, so lazy should not be called

			Assert.AreEqual("lazyReplace", MacroMap.Replace("%lazy%", lazyMap));
		}

		[TestMethod]
		public void LazyLambdaTest()
		{
			bool lazyCalled = false;

			MacroMap lazyMap = new MacroMap()
			{
				{ "lazy", () => { lazyCalled = true; return "lazyReplace"; } },
				{ "test", "replace" }
			};

			Assert.AreEqual("replace", MacroMap.Replace("%test%", lazyMap));
			Assert.IsFalse(lazyCalled); // lazy token did not appear, so lazy should not be called

			Assert.AreEqual("lazyReplace", MacroMap.Replace("%lazy%", lazyMap));
		}

		[TestMethod] public void MixedEscapeTest() => Assert.AreEqual("%replace%", MacroMap.Replace("%%%test%%%", new MacroMap() { { "test", "replace" } }));

		[TestMethod]
		public void InvalidEndOfEscapeTest()
		{
			FormatException formatEx = Assert.ThrowsException<FormatException>(() => MacroMap.Replace("%", new MacroMap()));
			Assert.AreEqual
			(
				"Unexpected end in token replacement string '%' at offset 1. Expected a token escape (%%) or a token string (%token%).",
				formatEx.Message
			);
		}

		[TestMethod]
		public void InvalidEndOfTokenTest()
		{
			FormatException formatEx = Assert.ThrowsException<FormatException>(() => MacroMap.Replace("%token", new MacroMap()));
			Assert.AreEqual
			(
				"Unexpected end in token replacement string '%token' at offset 6. Expected a token escape (%%) or a token string (%token%).",
				formatEx.Message
			);
		}

		[TestMethod]
		public void InvalidEndOfTokenAfterEscaoeTest()
		{
			FormatException formatEx = Assert.ThrowsException<FormatException>(() => MacroMap.Replace("%%%token", new MacroMap()));
			Assert.AreEqual
			(
				"Unexpected end in token replacement string '%%%token' at offset 8. Expected a token escape (%%) or a token string (%token%).",
				formatEx.Message
			);
		}

		[TestMethod]
		public void UnresolvedTokenTest()
		{
			MacroMap replaceMap = new MacroMap()
			{
				{ "exists", "test" }
			};

			ArgumentException argumentEx = Assert.ThrowsException<ArgumentException>(() => MacroMap.Replace("this%exists%butthis%doesnt%", replaceMap));
			Assert.AreEqual
			(
				"Macro %doesnt% at offset 19 in replacement string 'this%exists%butthis%doesnt%' is not valid in this context.",
				argumentEx.Message
			);
		}

		[TestMethod]
		public void AllowUnresolvedTokenTest()
		{
			string replaceString = "this%token%notreplaced";
			string returnString = MacroMap.Replace(replaceString, new MacroMap() { { "test", "replace" } }, allowUnresolved: true);

			Assert.AreEqual(replaceString, returnString);

			Assert.IsTrue(ReferenceEquals(replaceString, returnString)); // if there was nothing to replace we should get the same reference back
		}

		[TestMethod]
		public void LazyExceptionTest()
		{
			MacroMap lazyExceptionMap = new MacroMap()
			{
				{ "lazyex", () => throw new InvalidOperationException("Token 'lazyex' cannot be evaluated at this time.") }
			};

			ArgumentException formatEx = Assert.ThrowsException<ArgumentException>(() => MacroMap.Replace("%lazyex%", lazyExceptionMap));
			Assert.AreEqual
			(
				"Error evaluating %lazyex% at offset 0 in replacement string '%lazyex%': Token 'lazyex' cannot be evaluated at this time.",
				formatEx.Message
			);
		}

		[TestMethod]
		public void UpdateMapTest()
		{
			MacroMap updateMap = new MacroMap()
			{
				{ "one", "1" },
				{ "two", () => "2" }
			};

			Assert.AreEqual("12", MacroMap.Replace("%one%%two%", updateMap));

			updateMap.Update("one", () => "one");
			updateMap.Update("two", "two");

			Assert.AreEqual("onetwo", MacroMap.Replace("%one%%two%", updateMap));
		}
	}
}