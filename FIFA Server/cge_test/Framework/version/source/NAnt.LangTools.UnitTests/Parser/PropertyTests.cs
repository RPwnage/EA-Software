using System.Collections.Generic;

using Microsoft.VisualStudio.TestTools.UnitTesting;

namespace NAnt.LangTools.UnitTests
{
	[TestClass]
	public class PropertyTests
	{
		[TestMethod]
		public void SetUnconditionalProperty()
		{
			Property.Map dict = new Property.Map();
			dict.SetProperty("test", "a");

			Property.Node testProperty = dict["test"];
			Assert.IsInstanceOfType(testProperty, typeof(Property.String));
			Assert.AreEqual("a", (testProperty as Property.String).Value);

			Dictionary<string, ConditionValueSet> expansion = testProperty.Expand();
			Assert.AreEqual(1, expansion.Count);

			TestUtils.VerifyPropertyExpansionConditionSets(expansion, "a");
		}

		[TestMethod]
		public void OverwriteUnconditionalProperty()
		{
			Property.Map dict = new Property.Map();
			dict.SetProperty("test", "a");
			dict.SetProperty("test", "b");

			Property.Node testProperty = dict["test"];
			Assert.IsInstanceOfType(testProperty, typeof(Property.String));
			Assert.AreEqual("b", (testProperty as Property.String).Value);

			Dictionary<string, ConditionValueSet> expansion = testProperty.Expand();
			Assert.AreEqual(1, expansion.Count);

			TestUtils.VerifyPropertyExpansionConditionSets(expansion, "b");
		}

		[TestMethod]
		public void AllAccountedForSimplify()
		{
			Condition featureCondition = TestUtils.CreateBooleanCondition("TestFeature");
			Property.Map dict = new Property.Map();
			dict.SetProperty("test", new Property.Conditional(featureCondition, new Dictionary<string, Property.Node> { { "true", "a" } }));
			dict.SetProperty("test", new Property.Conditional(featureCondition, new Dictionary<string, Property.Node> { { "false", "a" } }));

			Property.Node testProperty = dict["test"];
			Assert.IsInstanceOfType(testProperty, typeof(Property.String));
			Assert.AreEqual("a", (testProperty as Property.String).Value);

			Dictionary<string, ConditionValueSet> expansion = testProperty.Expand();
			Assert.AreEqual(1, expansion.Count);

			TestUtils.VerifyPropertyExpansionConditionSets(expansion, "a");
		}


		[TestMethod]
		public void OverwriteConditionalProperty()
		{
			Property.Map dict = new Property.Map();

			Condition featureCondition = TestUtils.CreateBooleanCondition("TestFeature");
			dict.SetProperty("test", new Property.Conditional(featureCondition, new Dictionary<string, Property.Node>
			{
				{ "true", "irelevant" }
			}));

			dict.SetProperty("test", new Property.Conditional(featureCondition, new Dictionary<string, Property.Node>
			{
				{ "true", "override" },
			}));

			Property.Node testProperty = dict["test"];
			Assert.IsInstanceOfType(testProperty, typeof(Property.Conditional));

			Property.Conditional conditional = testProperty as Property.Conditional;
			Assert.AreEqual(1, conditional.SubNodes.Count);

			Assert.IsTrue(conditional.SubNodes.ContainsKey("true"));
			Assert.IsInstanceOfType(conditional.SubNodes["true"], typeof(Property.String));
			Assert.AreEqual("override", (conditional.SubNodes["true"] as Property.String).Value);
		}

		[TestMethod]
		public void NestedAllAccountedForSimplify()
		{
			Property.Map dict = new Property.Map();

			Condition featureCondition = TestUtils.CreateBooleanCondition("TestFeature");
			dict.SetProperty("test", new Property.Conditional(featureCondition, new Dictionary<string, Property.Node>
			{
				{ "true", "irrelevant" },
				{ "false", "b" }
			}));
			dict.SetProperty("test", new Property.Conditional(featureCondition, new Dictionary<string, Property.Node>
			{
				{ "true", "a" }
			}));

			Property.Node testProperty = dict["test"];
			Assert.IsInstanceOfType(testProperty, typeof(Property.Conditional));

			Property.Conditional conditional = testProperty as Property.Conditional;
			Assert.AreEqual(2, conditional.SubNodes.Count);

			Assert.IsTrue(conditional.SubNodes.ContainsKey("true"));
			Assert.IsInstanceOfType(conditional.SubNodes["true"], typeof(Property.String));
			Assert.AreEqual("a", (conditional.SubNodes["true"] as Property.String).Value);

			Assert.IsTrue(conditional.SubNodes.ContainsKey("false"));
			Assert.IsInstanceOfType(conditional.SubNodes["false"], typeof(Property.String));
			Assert.AreEqual("b", (conditional.SubNodes["false"] as Property.String).Value);
		}

		[TestMethod]
		public void SetBooleanConditionalProperty()
		{
			Property.Map dict = new Property.Map();

			Condition featureCondition = TestUtils.CreateBooleanCondition("TestFeature");
			dict.SetProperty("test", new Property.Conditional(featureCondition, new Dictionary<string, Property.Node>()
			{
				{ "true", "a" }
			}));

			Property.Node testProperty = dict["test"];
			Assert.IsInstanceOfType(testProperty, typeof(Property.Conditional));

			Property.Conditional conditional = testProperty as Property.Conditional;
			Assert.AreEqual(1, conditional.SubNodes.Count);

			Assert.IsTrue(conditional.SubNodes.ContainsKey("true"));
			Assert.IsInstanceOfType(conditional.SubNodes["true"], typeof(Property.String));
			Assert.AreEqual("a", (conditional.SubNodes["true"] as Property.String).Value);

			Dictionary<string, ConditionValueSet> expansion = testProperty.Expand(throwOnMissing: false);
			Assert.AreEqual(1, expansion.Count);

			TestUtils.VerifyPropertyExpansionConditionSets(expansion, "a", new Dictionary<Condition, string> { { featureCondition, "true" } });
		}

		[TestMethod]
		public void UpdateUnconditionalPropertyWithConditionalValue()
		{
			Property.Map dict = new Property.Map();
			dict.SetProperty("test", "a");

			Condition featureCondition = TestUtils.CreateBooleanCondition("TestFeature");
			dict.SetProperty("test", new Property.Conditional(featureCondition, new Dictionary<string, Property.Node>
			{
				{ "true", "b" }
			}));

			Property.Node testProperty = dict["test"];
			Assert.IsInstanceOfType(testProperty, typeof(Property.Conditional));

			Property.Conditional conditional = testProperty as Property.Conditional;
			Assert.AreEqual(2, conditional.SubNodes.Count);

			Assert.IsTrue(conditional.SubNodes.ContainsKey("true"));
			Assert.IsInstanceOfType(conditional.SubNodes["true"], typeof(Property.String));
			Assert.AreEqual("b", (conditional.SubNodes["true"] as Property.String).Value);

			Assert.IsTrue(conditional.SubNodes.ContainsKey("false"));
			Assert.IsInstanceOfType(conditional.SubNodes["false"], typeof(Property.String));
			Assert.AreEqual("a", (conditional.SubNodes["false"] as Property.String).Value);

			Dictionary<string, ConditionValueSet> expansion = testProperty.Expand();
			Assert.AreEqual(2, expansion.Count);

			TestUtils.VerifyPropertyExpansionConditionSets(expansion, "b", new Dictionary<Condition, string> { { featureCondition, "true" } });
			TestUtils.VerifyPropertyExpansionConditionSets(expansion, "a", new Dictionary<Condition, string> { { featureCondition, "false" } });
		}

		[TestMethod]
		public void UpdateConditonalPropertyWithSameCondition()
		{
			Property.Map dict = new Property.Map();
			Condition featureCondition = TestUtils.CreateBooleanCondition("TestFeature");
			dict.SetProperty("test", new Property.Conditional(featureCondition, new Dictionary<string, Property.Node>
			{
				{ "true", "a" },
			}));

			dict.SetProperty("test", new Property.Conditional(featureCondition, new Dictionary<string, Property.Node>
			{
				{ "false", "b" },
			}));

			Property.Node testProperty = dict["test"];
			Property.Conditional conditional = testProperty as Property.Conditional;
			Assert.AreEqual(2, conditional.SubNodes.Count);

			Assert.IsTrue(conditional.SubNodes.ContainsKey("true"));
			Assert.IsInstanceOfType(conditional.SubNodes["true"], typeof(Property.String));
			Assert.AreEqual("a", (conditional.SubNodes["true"] as Property.String).Value);

			Assert.IsTrue(conditional.SubNodes.ContainsKey("false"));
			Assert.IsInstanceOfType(conditional.SubNodes["false"], typeof(Property.String));
			Assert.AreEqual("b", (conditional.SubNodes["false"] as Property.String).Value);

			Dictionary<string, ConditionValueSet> expansion = testProperty.Expand();
			Assert.AreEqual(2, expansion.Count);

			TestUtils.VerifyPropertyExpansionConditionSets(expansion, "a", new Dictionary<Condition, string> { { featureCondition, "true" } });
			TestUtils.VerifyPropertyExpansionConditionSets(expansion, "b", new Dictionary<Condition, string> { { featureCondition, "false" } });
		}

		[TestMethod]
		public void UpdateConditionalPropertyWithNewCondition()
		{
			Property.Map dict = new Property.Map();

			Condition firstFeatureCondition = TestUtils.CreateBooleanCondition("FirstFeature");
			dict.SetProperty("test", new Property.Conditional(firstFeatureCondition, new Dictionary<string, Property.Node>
			{
				{ "true", "a" },
				{ "false", "b" }
			}));

			Condition secondFeatureCondition = TestUtils.CreateBooleanCondition("SecondFeature");
			dict.SetProperty("test", new Property.Conditional(secondFeatureCondition, new Dictionary<string, Property.Node> 
			{ 
				{ "true", "c" } 
			}));

			Property.Node testProperty = dict["test"];
			Assert.IsInstanceOfType(testProperty, typeof(Property.Conditional));

			Property.Conditional conditional = testProperty as Property.Conditional;
			Assert.AreEqual(conditional.Condition, secondFeatureCondition);
			Assert.AreEqual(2, conditional.SubNodes.Count);

			Assert.IsTrue(conditional.SubNodes.ContainsKey("true"));
			Assert.IsInstanceOfType(conditional.SubNodes["true"], typeof(Property.String));
			Assert.AreEqual("c", (conditional.SubNodes["true"] as Property.String).Value);

			Assert.IsTrue(conditional.SubNodes.ContainsKey("false"));
			Assert.IsInstanceOfType(conditional.SubNodes["false"], typeof(Property.Conditional));
			{
				Property.Conditional nestedConditional = conditional.SubNodes["false"] as Property.Conditional;
				Assert.AreEqual(nestedConditional.Condition, firstFeatureCondition);
				Assert.AreEqual(2, nestedConditional.SubNodes.Count);

				Assert.IsTrue(nestedConditional.SubNodes.ContainsKey("true"));
				Assert.IsInstanceOfType(nestedConditional.SubNodes["true"], typeof(Property.String));
				Assert.AreEqual("a", (nestedConditional.SubNodes["true"] as Property.String).Value);

				Assert.IsTrue(nestedConditional.SubNodes.ContainsKey("false"));
				Assert.IsInstanceOfType(nestedConditional.SubNodes["false"], typeof(Property.String));
				Assert.AreEqual("b", (nestedConditional.SubNodes["false"] as Property.String).Value);
			}

			Dictionary<string, ConditionValueSet> expansion = testProperty.Expand();
			Assert.AreEqual(3, expansion.Count);

			TestUtils.VerifyPropertyExpansionConditionSets(expansion, "a", new Dictionary<Condition, string> { { firstFeatureCondition, "true" }, { secondFeatureCondition, "false" } });
			TestUtils.VerifyPropertyExpansionConditionSets(expansion, "b", new Dictionary<Condition, string> { { firstFeatureCondition, "false" }, { secondFeatureCondition, "false" } });
			TestUtils.VerifyPropertyExpansionConditionSets(expansion, "c", new Dictionary<Condition, string> { { secondFeatureCondition, "true" } });
		}

		[TestMethod]
		public void UpdateConditionalPropertyWithNestedSameCondition()
		{
			Property.Map dict = new Property.Map();

			Condition firstFeatureCondition = TestUtils.CreateBooleanCondition("FirstFeature");
			dict.SetProperty("test", new Property.Conditional(firstFeatureCondition, new Dictionary<string, Property.Node>
			{
				{ "true", "alpha" },
				{ "false", "beta" }
			}));

			Condition secondFeatureCondition = TestUtils.CreateBooleanCondition("SecondFeature");
			dict.SetProperty("test", new Property.Conditional(secondFeatureCondition, new Dictionary<string, Property.Node>
			{
				{ 
					"true", 
					new Property.Conditional(firstFeatureCondition, new Dictionary<string, Property.Node>
					{
						{ "true", "gamma" }
					})
				}
			}));
		}

		[TestMethod]
		public void UpdateConditionalWithImplicitSimplifcation()
		{
			Property.Map dict = new Property.Map();

			Condition firstFeatureCondition = TestUtils.CreateBooleanCondition("FirstFeature");
			Condition secondFeatureCondition = TestUtils.CreateBooleanCondition("SecondFeature");
			dict.SetProperty("test", new Property.Conditional(firstFeatureCondition, new Dictionary<string, Property.Node>
			{
				{ "true", "a" },
				{
					"false", new Property.Conditional(secondFeatureCondition, new Dictionary<string, Property.Node>
					{
						{ "true", "b" },
						{ "false", "c" }
					}
				)}
			}));

			dict.SetProperty("test", new Property.Conditional(secondFeatureCondition, new Dictionary<string, Property.Node>
			{
				{ "true", "b" },
			}));

			Property.Node testProperty = dict["test"];
			Property.Conditional conditional = testProperty as Property.Conditional;
			Assert.AreEqual(conditional.Condition, secondFeatureCondition);
			Assert.AreEqual(2, conditional.SubNodes.Count);

			Assert.IsTrue(conditional.SubNodes.ContainsKey("true"));
			Assert.IsInstanceOfType(conditional.SubNodes["true"], typeof(Property.String));
			Assert.AreEqual("b", (conditional.SubNodes["true"] as Property.String).Value);

			Assert.IsInstanceOfType(conditional.SubNodes["false"], typeof(Property.Conditional));
			{
				Property.Conditional nestedConditional = conditional.SubNodes["false"] as Property.Conditional;
				Assert.AreEqual(nestedConditional.Condition, firstFeatureCondition);
				Assert.AreEqual(2, nestedConditional.SubNodes.Count);

				Assert.IsTrue(nestedConditional.SubNodes.ContainsKey("true"));
				Assert.IsInstanceOfType(nestedConditional.SubNodes["true"], typeof(Property.String));
				Assert.AreEqual("a", (nestedConditional.SubNodes["true"] as Property.String).Value);

				Assert.IsTrue(nestedConditional.SubNodes.ContainsKey("false"));
				Assert.IsInstanceOfType(nestedConditional.SubNodes["false"], typeof(Property.String));
				Assert.AreEqual("c", (nestedConditional.SubNodes["false"] as Property.String).Value);
			}

			Dictionary<string, ConditionValueSet> expansion = testProperty.Expand();
			Assert.AreEqual(3, expansion.Count);

			TestUtils.VerifyPropertyExpansionConditionSets(expansion, "a", new Dictionary<Condition, string> { { firstFeatureCondition, "true" }, { secondFeatureCondition, "false" } });
			TestUtils.VerifyPropertyExpansionConditionSets(expansion, "b", new Dictionary<Condition, string> { { secondFeatureCondition, "true" } });
			TestUtils.VerifyPropertyExpansionConditionSets(expansion, "c", new Dictionary<Condition, string> { { firstFeatureCondition, "false" }, { secondFeatureCondition, "false" } });
		}

		[TestMethod]
		public void EmptySplit()
		{
			Assert.AreEqual(0, new Property.String("").SplitToArray().Length);
		}

		[TestMethod]
		public void SimpleSplit()
		{
			Property.Node[] splitNodes = new Property.String("\n\talpha \t\tbeta \n\t\n").SplitToArray();
			Assert.AreEqual(2, splitNodes.Length);

			Assert.IsInstanceOfType(splitNodes[0], typeof(Property.String));
			Assert.AreEqual("alpha", (splitNodes[0] as Property.String).Value);
			Assert.IsInstanceOfType(splitNodes[1], typeof(Property.String));
			Assert.AreEqual("beta", (splitNodes[1] as Property.String).Value);
		}

		[TestMethod]
		public void ConditionalSplit()
		{
			Condition condition = TestUtils.CreateBooleanCondition("Condition");
			Property.Conditional conditionalNode = new Property.Conditional
			(
				condition,
				new Dictionary<string, Property.Node>()
				{
					{ "true", "\n\talpha \t\tbeta \n\t\n" },
					{ "false", "gamma" }
				}
			);

			Property.Node[] splitNodes = conditionalNode.SplitToArray();
			Assert.AreEqual(2, splitNodes.Length);

			Assert.IsInstanceOfType(splitNodes[0], typeof(Property.Conditional));
			{
				Property.Conditional conditional = splitNodes[0] as Property.Conditional;
				Assert.AreEqual(2, conditional.SubNodes.Count);

				Assert.IsTrue(conditional.SubNodes.ContainsKey("true"));
				Assert.IsInstanceOfType(conditional.SubNodes["true"], typeof(Property.String));
				Assert.AreEqual("alpha", (conditional.SubNodes["true"] as Property.String).Value);

				Assert.IsTrue(conditional.SubNodes.ContainsKey("false"));
				Assert.IsInstanceOfType(conditional.SubNodes["false"], typeof(Property.String));
				Assert.AreEqual("gamma", (conditional.SubNodes["false"] as Property.String).Value);
			}

			Assert.IsInstanceOfType(splitNodes[1], typeof(Property.Conditional));
			{
				Property.Conditional conditional = splitNodes[1] as Property.Conditional;
				Assert.AreEqual(1, conditional.SubNodes.Count);

				Assert.IsTrue(conditional.SubNodes.ContainsKey("true"));
				Assert.IsInstanceOfType(conditional.SubNodes["true"], typeof(Property.String));
				Assert.AreEqual("beta", (conditional.SubNodes["true"] as Property.String).Value);
			}
		}

		[TestMethod]
		public void ConditionalSplitWithSimplifyingCollapse()
		{
			Condition condition = TestUtils.CreateBooleanCondition("Condition");
			Property.Conditional conditionalNode = new Property.Conditional
			(
				condition,
				new Dictionary<string, Property.Node>()
				{
					{ "true", "alpha\nbeta\ngamma" },
					{ "false", "alpha\ndelta\ngamma" },
				}
			);

			Property.Node[] splitNodes = conditionalNode.SplitToArray();
			Assert.AreEqual(3, splitNodes.Length);

			Assert.IsInstanceOfType(splitNodes[0], typeof(Property.String));
			Assert.AreEqual("alpha", (splitNodes[0] as Property.String).Value);

			Assert.IsInstanceOfType(splitNodes[1], typeof(Property.Conditional));
			{
				Property.Conditional conditional = splitNodes[1] as Property.Conditional;
				Assert.AreEqual(2, conditional.SubNodes.Count);

				Assert.IsTrue(conditional.SubNodes.ContainsKey("true"));
				Assert.IsInstanceOfType(conditional.SubNodes["true"], typeof(Property.String));
				Assert.AreEqual("beta", (conditional.SubNodes["true"] as Property.String).Value);

				Assert.IsTrue(conditional.SubNodes.ContainsKey("false"));
				Assert.IsInstanceOfType(conditional.SubNodes["false"], typeof(Property.String));
				Assert.AreEqual("delta", (conditional.SubNodes["false"] as Property.String).Value);
			}

			Assert.IsInstanceOfType(splitNodes[2], typeof(Property.String));
			Assert.AreEqual("gamma", (splitNodes[2] as Property.String).Value);
		}

		[TestMethod]
		public void CompositeSplitLeadingCollapse()
		{
			Condition condition = TestUtils.CreateBooleanCondition("Condition");
			Property.Node concatenated = Property.Concatenate
			(
				"alpha\n",
				new Property.Conditional
				(
					condition,
					new Dictionary<string, Property.Node>()
					{
						{ "false", "beta\n" },
						{ "true", "beta\ngamma\n" }
					}
				),
				"delta"
			);

			Assert.IsInstanceOfType(concatenated, typeof(Property.Composite));

			Property.Node[] splitNodes = concatenated.SplitToArray();
			Assert.AreEqual(4, splitNodes.Length);

			Assert.IsInstanceOfType(splitNodes[0], typeof(Property.String));
			Assert.AreEqual("alpha", (splitNodes[0] as Property.String).Value);

			Assert.IsInstanceOfType(splitNodes[1], typeof(Property.String));
			Assert.AreEqual("beta", (splitNodes[1] as Property.String).Value);

			Assert.IsInstanceOfType(splitNodes[2], typeof(Property.Conditional));
			{
				Property.Conditional conditional = splitNodes[2] as Property.Conditional;
				Assert.AreEqual(1, conditional.SubNodes.Count);

				Assert.IsTrue(conditional.SubNodes.ContainsKey("true"));
				Assert.IsInstanceOfType(conditional.SubNodes["true"], typeof(Property.String));
				Assert.AreEqual("gamma", (conditional.SubNodes["true"] as Property.String).Value);
			}

			Assert.IsInstanceOfType(splitNodes[3], typeof(Property.String));
			Assert.AreEqual("delta", (splitNodes[3] as Property.String).Value);
		}

		[TestMethod]
		public void CompositeSplitTrailingCollapse()
		{
			Condition condition = TestUtils.CreateBooleanCondition("Condition");
			Property.Node concatenated = Property.Concatenate
			(
				"alpha\n",
				new Property.Conditional
				(
					condition,
					new Dictionary<string, Property.Node>()
					{
						{ "false", "gamma\n" },
						{ "true", "beta\ngamma\n" }
					}
				),
				"delta"
			);

			Assert.IsInstanceOfType(concatenated, typeof(Property.Composite));

			Property.Node[] splitNodes = concatenated.SplitToArray();
			Assert.AreEqual(4, splitNodes.Length);

			Assert.IsInstanceOfType(splitNodes[0], typeof(Property.String));
			Assert.AreEqual("alpha", (splitNodes[0] as Property.String).Value);

			Assert.IsInstanceOfType(splitNodes[1], typeof(Property.String));
			Assert.AreEqual("gamma", (splitNodes[1] as Property.String).Value);

			Assert.IsInstanceOfType(splitNodes[2], typeof(Property.Conditional));
			{
				Property.Conditional conditional = splitNodes[2] as Property.Conditional;
				Assert.AreEqual(1, conditional.SubNodes.Count);

				Assert.IsTrue(conditional.SubNodes.ContainsKey("true"));
				Assert.IsInstanceOfType(conditional.SubNodes["true"], typeof(Property.String));
				Assert.AreEqual("beta", (conditional.SubNodes["true"] as Property.String).Value);
			}

			Assert.IsInstanceOfType(splitNodes[3], typeof(Property.String));
			Assert.AreEqual("delta", (splitNodes[3] as Property.String).Value);
		}

		[TestMethod]
		public void CompositeCrossConditionalBoundarySplit()
		{
			Condition condition = TestUtils.CreateBooleanCondition("Condition");
			Property.Node concatenated = Property.Concatenate
			(
				"alpha=alpha\nbeta=",
				new Property.Conditional
				(
					condition,
					new Dictionary<string, Property.Node>()
					{
						{ "true", "beta" },
						{ "false", "gamma" }
					}
				),
				"\ndelta=delta"
			);

			Assert.IsInstanceOfType(concatenated, typeof(Property.Composite));

			Property.Node[] splitNodes = concatenated.SplitToArray();
			Assert.AreEqual(3, splitNodes.Length);
		}
	}
}