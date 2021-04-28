using System;
using System.Collections.Generic;

using Microsoft.VisualStudio.TestTools.UnitTesting;

namespace NAnt.LangTools.UnitTests
{
	[TestClass]
	public class ParserPropertyTests : ParserTestsBase
	{
		[TestMethod]
		public void SimpleProperty()
		{
			Parser parser = ParseSnippet
			(
				Script("<project>",
				"    <property name=\"testProp\" value=\"testVal\"/>",
				"</project>")
			);

			Assert.AreEqual("testVal", parser.Properties["testProp"].ToString());
		}

		[TestMethod]
		public void EvaluatedProperty()
		{
			Parser parser = ParseSnippet
			(
				Script("<project>",
				"    <property name=\"testPropOne\" value=\"testVal\" />",
				"    <property name=\"testPropTwo\" value=\"${testPropOne}\" />",
				"</project>")
			);

			Assert.AreEqual("testVal", parser.Properties["testPropOne"].ToString());
			Assert.AreEqual("testVal", parser.Properties["testPropTwo"].ToString());
		}

		[TestMethod]
		public void EvaluatedCompositeProperty()
		{
			Parser parser = ParseSnippet
			(
				Script("<project>",
				"    <property name=\"testPropOne\" value=\"testVal\" />",
				"    <property name=\"testPropTwo\" value=\"prefix-${testPropOne}\" />",
				"</project>")
			);

			Assert.AreEqual("testVal", parser.Properties["testPropOne"].ToString());
			Assert.AreEqual("prefix-testVal", parser.Properties["testPropTwo"].ToString());
		}

		[TestMethod]
		public void EvaluateBooleanRuleProperty()
		{
			Feature customFeature = new Feature
			(
				"Custom Feature",
				new Dictionary<string, Dictionary<string, string>>
				{
					{ "true", new Dictionary<string, string> { { "Custom", "true" } } },
					{ "false", new Dictionary<string, string> { { "Custom", "false" } } }
				}
			);

			Parser parser = ParseSnippet
			(
				Script("<project>",
				"    <property name=\"isCustomProp\" value=\"${Custom}\" />",
				"</project>"),
				customFeature
			);

			Property.Node property = parser.Properties["isCustomProp"];

			Assert.IsInstanceOfType(property, typeof(Property.Conditional));
			Property.Conditional conditional = property as Property.Conditional;
			Assert.AreEqual(2, conditional.SubNodes.Count);

			Assert.IsTrue(conditional.SubNodes.ContainsKey("true"));
			Assert.IsInstanceOfType(conditional.SubNodes["true"], typeof(Property.String));
			Assert.AreEqual("true", (conditional.SubNodes["true"] as Property.String).Value);

			Assert.IsTrue(conditional.SubNodes.ContainsKey("false"));
			Assert.IsInstanceOfType(conditional.SubNodes["false"], typeof(Property.String));
			Assert.AreEqual("false", (conditional.SubNodes["false"] as Property.String).Value);
		}

		[TestMethod]
		public void EvaluateBooleanRulePropertyCondition()
		{
			Feature customFeature = new Feature
			(
				"Custom Feature",
				new Dictionary<string, Dictionary<string, string>>
				{
					{ "true", new Dictionary<string, string> { { "Custom", "true" } } },
					{ "false", new Dictionary<string, string> { { "Custom", "false" } } }
				}
			);

			Parser parser = ParseSnippet
			(
				Script("<project>",
				"    <property name=\"isCustomProp\" value=\"true\" if=\"${Custom}\" />",
				"    <property name=\"isCustomProp\" value=\"false\" unless=\"${Custom}\" />",
				"</project>"),
				customFeature
			);

			Property.Node property = parser.Properties["isCustomProp"];

			Assert.IsInstanceOfType(property, typeof(Property.Conditional));
			Property.Conditional conditional = property as Property.Conditional;
			Assert.AreEqual(2, conditional.SubNodes.Count);

			Assert.IsTrue(conditional.SubNodes.ContainsKey("true"));
			Assert.IsInstanceOfType(conditional.SubNodes["true"], typeof(Property.String));
			Assert.AreEqual("true", (conditional.SubNodes["true"] as Property.String).Value);

			Assert.IsTrue(conditional.SubNodes.ContainsKey("false"));
			Assert.IsInstanceOfType(conditional.SubNodes["false"], typeof(Property.String));
			Assert.AreEqual("false", (conditional.SubNodes["false"] as Property.String).Value);
		}

		[TestMethod]
		public void EvaluateEnumRuleCondition()
		{
			Feature customFeature = new Feature
			(
				"CustomFeature",
				new Dictionary<string, Dictionary<string, string>>
				{
					{ "on", new Dictionary<string, string> { { "CustomProperty", "1" } } },
					{ "off", new Dictionary<string, string> { { "CustomProperty", "0" } } }
				}
			);

			Parser parser = ParseSnippet
			(
				Script("<project>",
				"    <property name=\"isCustomFeature\" value=\"true\" if=\"${CustomProperty} == 1\" />",
				"    <property name=\"isCustomFeature\" value=\"false\" if=\"${CustomProperty} == 0\" />",
				"</project>"),
				customFeature
			);

			Property.Node property = parser.Properties["isCustomFeature"];

			Assert.IsInstanceOfType(property, typeof(Property.Conditional));
			Property.Conditional conditional = property as Property.Conditional;
			Assert.AreEqual(2, conditional.SubNodes.Count);

			Assert.IsTrue(conditional.SubNodes.ContainsKey("on"));
			Assert.IsInstanceOfType(conditional.SubNodes["on"], typeof(Property.String));
			Assert.AreEqual("true", (conditional.SubNodes["on"] as Property.String).Value);

			Assert.IsTrue(conditional.SubNodes.ContainsKey("off"));
			Assert.IsInstanceOfType(conditional.SubNodes["off"], typeof(Property.String));
			Assert.AreEqual("false", (conditional.SubNodes["off"] as Property.String).Value);
		}

		[TestMethod]
		public void EvaluateMultipleRulePropertyContents()
		{
			Feature customFeatureOne = new Feature
			(
				"CustomFeatureOne",
				new Dictionary<string, Dictionary<string, string>>
				{
					{ "on", new Dictionary<string, string> { { "CustomPropertyOne", "1" } } },
					{ "off", new Dictionary<string, string> { { "CustomPropertyOne", "0" } } }
				}
			);

			Feature customFeatureTwo = new Feature
			(
				"CustomFeatureTwo",
				new Dictionary<string, Dictionary<string, string>>
				{
					{ "on", new Dictionary<string, string> { { "CustomPropertyTwo", "1" } } },
					{ "off", new Dictionary<string, string> { { "CustomPropertyTwo", "0" } } }
				}
			);

			Parser parser = ParseSnippet
			(
				Script("<project>",
				"	<property name=\"prop\">",
				"		${CustomPropertyOne}",
				"		${CustomPropertyTwo}",
				"	</property>",
				"</project>"),
				customFeatureOne,
				customFeatureTwo
			);

			Property.Node property = parser.Properties["prop"];

			Dictionary<string, ConditionValueSet> expansion = property.Expand();
			Assert.AreEqual(4, expansion.Count);

			// newlines seem to get normalized to \t always
			TestUtils.VerifyPropertyExpansionConditionSets
			(
				expansion, "\n\t\t0\n\t\t0\n\t",
				new Dictionary<Condition, string>() { { customFeatureOne.Condition, "off" }, { customFeatureTwo.Condition, "off" } }
			);
			TestUtils.VerifyPropertyExpansionConditionSets
			(
				expansion, "\n\t\t0\n\t\t1\n\t",
				new Dictionary<Condition, string>() { { customFeatureOne.Condition, "off" }, { customFeatureTwo.Condition, "on" } }
			);
			TestUtils.VerifyPropertyExpansionConditionSets
			(
				expansion, "\n\t\t1\n\t\t0\n\t",
				new Dictionary<Condition, string>() { { customFeatureOne.Condition, "on" }, { customFeatureTwo.Condition, "off" } }
			);
			TestUtils.VerifyPropertyExpansionConditionSets
			(
				expansion, "\n\t\t1\n\t\t1\n\t",
				new Dictionary<Condition, string>() { { customFeatureOne.Condition, "on" }, { customFeatureTwo.Condition, "on" } }
			);
		}

		[TestMethod]
		public void EvaluatedPropertyName()
		{
			Feature customFeature = new Feature
			(
				"CustomFeatureMode",
				new Dictionary<string, Dictionary<string, string>>
				{
					{ "alpha", new Dictionary<string, string> { { "CustomModeProperty", "alpha" } } },
					{ "beta", new Dictionary<string, string> { { "CustomModeProperty", "beta" } } }
				}
			);

			Parser parser = ParseSnippet
			(
				Script("<project>",
				"    <property name=\"${CustomModeProperty}-is-active\" value=\"true\"/>",
				"</project>"),
				customFeature
			);

			Property.Node alphaNode = parser.Properties["alpha-is-active"];
			Assert.IsInstanceOfType(alphaNode, typeof(Property.Conditional));
			Property.Conditional alphaConditional = alphaNode as Property.Conditional;
			Assert.AreEqual(1, alphaConditional.SubNodes.Count);
			Assert.IsTrue(alphaConditional.SubNodes.ContainsKey("alpha"));
			Assert.IsInstanceOfType(alphaConditional.SubNodes["alpha"], typeof(Property.String));
			Assert.AreEqual("true", (alphaConditional.SubNodes["alpha"] as Property.String).Value);

			Property.Node betaNode = parser.Properties["beta-is-active"];
			Assert.IsInstanceOfType(betaNode, typeof(Property.Conditional));
			Property.Conditional betaConditional = betaNode as Property.Conditional;
			Assert.AreEqual(1, betaConditional.SubNodes.Count);
			Assert.IsTrue(betaConditional.SubNodes.ContainsKey("beta"));
			Assert.IsInstanceOfType(betaConditional.SubNodes["beta"], typeof(Property.String));
			Assert.AreEqual("true", (betaConditional.SubNodes["beta"] as Property.String).Value);
		}

		[TestMethod]
		public void PropertyValueAggregation()
		{
			Parser parser = ParseSnippet
			(
				Script("<project>",
				"	<property name=\"aggregate\">",
				"		one",
				"	</property>",
				"	<property name=\"aggregate\">",
				"		${property.value}",
				"		two",
				"	</property>",
				"	<property name=\"aggregate\">",
				"		${property.value}",
				"		three",
				"	</property>",
				"</project>")
			);

			Property.Node aggregateNode = parser.Properties["aggregate"];
			Assert.IsInstanceOfType(aggregateNode, typeof(Property.String));
			string one = "\n\t\tone\n\t";
			string two = $"\n\t\t{one}\n\t\ttwo\n\t";
			string three = $"\n\t\t{two}\n\t\tthree\n\t";
			Assert.AreEqual(three, (aggregateNode as Property.String).Value);
		}

		[TestMethod]
		public void EvaluatedLoopAggregation()
		{
			Feature preambleFeature = new Feature
			(
				"Preamble",
				new Dictionary<string, Dictionary<string, string>>
				{
					{ "Alpha", new Dictionary<string, string> { { "preamble", "alpha" } } },
					{ "Beta", new Dictionary<string, string> { { "preamble", "beta" } } }
				}
			);

			Feature featureTwo = new Feature
			(
				"FeatureTwo",
				new Dictionary<string, Dictionary<string, string>>
				{
					{ "Epsilon", new Dictionary<string, string> { { "feature-two", "epsilon" } } },
					{ "Zeta", new Dictionary<string, string> { { "feature-two", "zeta" } } }
				}
			);

			Feature featureThree = new Feature
			(
				"FeatureThree",
				new Dictionary<string, Dictionary<string, string>>
				{
					{ "Eta", new Dictionary<string, string> { { "feature-three", "eta" } } },
					{ "Theta", new Dictionary<string, string> { { "feature-three", "theta" } } }
				}
			);

			Parser parser = ParseSnippet
			(
				Script("<project>",
				"	<property name=\"aggregate\">",
				"		initial",
				"		${preamble}",
				"	</property>",
				"",
				"	<property name=\"loop\">",
				"		one",
				"       two",
				"		three",
				"	</property>",
				"",
				"	<property name=\"feature-one\" value=\"gamma\"/>",
				"",
				"	<foreach item=\"String\" in=\"${loop}\" property=\"loop-i\">",
				"		<property name=\"aggregate\">",
				"			${property.value}",
				"			${feature-${loop-i}}",
				"		</property>",
				"	</foreach>",
				"</project>"),
				preambleFeature, featureTwo, featureThree
			);

			Property.Node aggregateNode = parser.Properties["aggregate"];

			Dictionary<string, ConditionValueSet> aggregateExpansion = aggregateNode.Expand();
			Assert.AreEqual(8, aggregateExpansion.Count);

			string template = "\n\t\t\t\n\t\t\t\n\t\t\t\n\t\tinitial\n\t\t{0}\n\t\n\t\t\tgamma\n\t\t\n\t\t\t{1}\n\t\t\n\t\t\t{2}\n\t\t";
			TestUtils.VerifyPropertyExpansionConditionSets
			(
				aggregateExpansion, String.Format(template, "alpha", "epsilon", "eta"),
				new Dictionary<Condition, string> { { preambleFeature.Condition, "Alpha" }, { featureTwo.Condition, "Epsilon" }, { featureThree.Condition, "Eta" } }
			);
			TestUtils.VerifyPropertyExpansionConditionSets
			(
				aggregateExpansion, String.Format(template, "alpha", "epsilon", "theta"),
				new Dictionary<Condition, string> { { preambleFeature.Condition, "Alpha" }, { featureTwo.Condition, "Epsilon" }, { featureThree.Condition, "Theta" } }
			);
			TestUtils.VerifyPropertyExpansionConditionSets
			(
				aggregateExpansion, String.Format(template, "alpha", "zeta", "eta"),
				new Dictionary<Condition, string> { { preambleFeature.Condition, "Alpha" }, { featureTwo.Condition, "Zeta" }, { featureThree.Condition, "Eta" } }
			);
			TestUtils.VerifyPropertyExpansionConditionSets
			(
				aggregateExpansion, String.Format(template, "alpha", "zeta", "theta"),
				new Dictionary<Condition, string> { { preambleFeature.Condition, "Alpha" }, { featureTwo.Condition, "Zeta" }, { featureThree.Condition, "Theta" } }
			);
			TestUtils.VerifyPropertyExpansionConditionSets
			(
				aggregateExpansion, String.Format(template, "beta", "epsilon", "eta"),
				new Dictionary<Condition, string> { { preambleFeature.Condition, "Beta" }, { featureTwo.Condition, "Epsilon" }, { featureThree.Condition, "Eta" } }
			);
			TestUtils.VerifyPropertyExpansionConditionSets
			(
				aggregateExpansion, String.Format(template, "beta", "epsilon", "theta"),
				new Dictionary<Condition, string> { { preambleFeature.Condition, "Beta" }, { featureTwo.Condition, "Epsilon" }, { featureThree.Condition, "Theta" } }
			);
			TestUtils.VerifyPropertyExpansionConditionSets
			(
				aggregateExpansion, String.Format(template, "beta", "zeta", "eta"),
				new Dictionary<Condition, string> { { preambleFeature.Condition, "Beta" }, { featureTwo.Condition, "Zeta" }, { featureThree.Condition, "Eta" } }
			);
			TestUtils.VerifyPropertyExpansionConditionSets
			(
				aggregateExpansion, String.Format(template, "beta", "zeta", "theta"),
				new Dictionary<Condition, string> { { preambleFeature.Condition, "Beta" }, { featureTwo.Condition, "Zeta" }, { featureThree.Condition, "Theta" } }
			);
		}

		[TestMethod]
		public void SimplePropertyFunctionEvaluation()
		{
			Parser parser = ParseSnippet
			(
				Script("<project>",
				"    <property name=\"greater\" value=\"@{StrCompareVersions('2', '1')}\"/>",
				"    <property name=\"same\" value=\"@{StrCompareVersions('1', '1')}\"/>",
				"    <property name=\"lesser\" value=\"@{StrCompareVersions('0', '1')}\"/>",
				"</project>")
			);

			Property.Node greaterNode = parser.Properties["greater"];
			Assert.IsInstanceOfType(greaterNode, typeof(Property.String));
			Assert.AreEqual("1", (greaterNode as Property.String).Value);

			Property.Node sameNode = parser.Properties["same"];
			Assert.IsInstanceOfType(sameNode, typeof(Property.String));
			Assert.AreEqual("0", (sameNode as Property.String).Value);

			Property.Node lesserNode = parser.Properties["lesser"];
			Assert.IsInstanceOfType(lesserNode, typeof(Property.String));
			Assert.AreEqual("-1", (lesserNode as Property.String).Value);
		}

		[TestMethod]
		public void ConditionalPropertyFunctionEvaluation()
		{
			Feature inverseFeature = new Feature
			(
				"Invert",
				new Dictionary<string, Dictionary<string, string>>
				{
					{ "off", new Dictionary<string, string> { { "high", "2" }, { "medium", "1" }, { "low", "0" } } },
					{ "on", new Dictionary<string, string> { { "high", "0" }, { "medium", "1" }, { "low", "2" } } },
				}
			);

			Feature flipFeature = new Feature
			(
				"Flip",
				new Dictionary<string, Dictionary<string, string>>
				{
					{ "on", new Dictionary<string, string> { { "flip", "0" } } },
					{ "off", new Dictionary<string, string> { { "flip", "2" } } },
				}
			);

			Parser parser = ParseSnippet
			(
				Script("<project>",
				"    <property name=\"greater\" value=\"@{StrCompareVersions(${high}, '1')}\"/>",
				"    <property name=\"same\" value=\"@{StrCompareVersions(${medium}, '1')}\"/>",
				"    <property name=\"lesser\" value=\"@{StrCompareVersions(${low}, '1')}\"/>",
				"	 <property name=\"flip\" value=\"@{StrCompareVersions(${high}, ${flip})}\"/>",
				"</project>"),
				inverseFeature, flipFeature
			);

			Property.Node greaterNode = parser.Properties["greater"];
			Assert.IsInstanceOfType(greaterNode, typeof(Property.Conditional));
			{
				Property.Conditional conditional = greaterNode as Property.Conditional;
				Assert.AreEqual(2, conditional.SubNodes.Count);

				Assert.IsTrue(conditional.SubNodes.ContainsKey("off"));
				Assert.IsInstanceOfType(conditional.SubNodes["off"], typeof(Property.String));
				Assert.AreEqual("1", (conditional.SubNodes["off"] as Property.String).Value);

				Assert.IsTrue(conditional.SubNodes.ContainsKey("on"));
				Assert.IsInstanceOfType(conditional.SubNodes["on"], typeof(Property.String));
				Assert.AreEqual("-1", (conditional.SubNodes["on"] as Property.String).Value);
			}

			Property.Node sameNode = parser.Properties["same"];
			Assert.IsInstanceOfType(sameNode, typeof(Property.String));
			Assert.AreEqual("0", (sameNode as Property.String).Value);

			Property.Node lesserNode = parser.Properties["lesser"];
			Assert.IsInstanceOfType(lesserNode, typeof(Property.Conditional));
			{
				Property.Conditional conditional = lesserNode as Property.Conditional;
				Assert.AreEqual(2, conditional.SubNodes.Count);

				Assert.IsTrue(conditional.SubNodes.ContainsKey("off"));
				Assert.IsInstanceOfType(conditional.SubNodes["off"], typeof(Property.String));
				Assert.AreEqual("-1", (conditional.SubNodes["off"] as Property.String).Value);

				Assert.IsTrue(conditional.SubNodes.ContainsKey("on"));
				Assert.IsInstanceOfType(conditional.SubNodes["on"], typeof(Property.String));
				Assert.AreEqual("1", (conditional.SubNodes["on"] as Property.String).Value);
			}

			Property.Node flipNode = parser.Properties["flip"];

			Dictionary<string, ConditionValueSet> flipExpansion = flipNode.Expand();
			Assert.AreEqual(3, flipExpansion.Count);

			TestUtils.VerifyPropertyExpansionConditionSets
			(
				flipExpansion, "2",
				new Dictionary<Condition, string>() { { inverseFeature.Condition, "off" }, { flipFeature.Condition, "on" } }
			);
			TestUtils.VerifyPropertyExpansionConditionSets
			(
				flipExpansion, "0",
				new Dictionary<Condition, string>() { { inverseFeature.Condition, "off" }, { flipFeature.Condition, "off" } },
				new Dictionary<Condition, string>() { { inverseFeature.Condition, "on" }, { flipFeature.Condition, "on" } }
			);
			TestUtils.VerifyPropertyExpansionConditionSets
			(
				flipExpansion, "-2",
				new Dictionary<Condition, string>() { { inverseFeature.Condition, "on" }, { flipFeature.Condition, "off" } }
			);
		}

		[TestMethod]
		public void SetNonLocalPropertyAfterLocalNoConditions()
		{
			Parser parser = ParseSnippet
			(
				Script("<project>",
				"	<property name=\"test\" local=\"true\">",
				"		alpha",
				"		beta",
				"	</property>",
				"	<property name=\"test\">",
				"		${property.value}",
				"		gamma",
				"		delta",
				"	</property>",
				"	<property name=\"record\" value=\"${test}\"/>",
				"</project>")
			);

			Assert.IsFalse(parser.Properties.ContainsKey("test"));
			Property.Node recordProperty = parser.Properties["record"];
			Assert.IsInstanceOfType(recordProperty, typeof(Property.String));
			Assert.AreEqual("\n\t\t\n\t\talpha\n\t\tbeta\n\t\n\t\tgamma\n\t\tdelta\n\t", (recordProperty as Property.String).Value);
		}

		[TestMethod]
		public void SetNonLocalPropertyAfterLocalConditional()
		{
			Feature conditionFeature = new Feature
			(
				"ConditionFeature",
				new Dictionary<string, Dictionary<string, string>>
				{
					{ "true", new Dictionary<string, string> { { "CustomProperty", "true" } } },
					{ "false", new Dictionary<string, string> { { "CustomProperty", "false" } } }
				}
			);

			Parser parser = ParseSnippet
			(
				Script("<project>",
				"	<property name=\"test\" local=\"true\" if=\"${CustomProperty}\">",
				"		alpha",
				"		beta",
				"	</property>",
				"	<property name=\"test\">",
				"		${property.value}",
				"		gamma",
				"		delta",
				"	</property>",
				"	<property name=\"record\" value=\"${test}\"/>",
				"</project>"),
				conditionFeature
			);

			Property.Node testProperty = parser.Properties["test"];
			Assert.IsInstanceOfType(testProperty, typeof(Property.String));
			Assert.AreEqual("\n\t\t\n\t\tgamma\n\t\tdelta\n\t", (testProperty as Property.String).Value);

			Property.Node recordProperty = parser.Properties["record"];
			Assert.IsInstanceOfType(recordProperty, typeof(Property.Composite));
			{
				Property.Composite composite = recordProperty as Property.Composite;
				Assert.AreEqual(3, composite.SubNodes.Length);

				Assert.IsInstanceOfType(composite.SubNodes[0], typeof(Property.String));
				Assert.AreEqual("\n\t\t", (composite.SubNodes[0] as Property.String).Value);

				Assert.IsInstanceOfType(composite.SubNodes[1], typeof(Property.Conditional));
				{
					Property.Conditional conditional = composite.SubNodes[1] as Property.Conditional;
					Assert.AreEqual(1, conditional.SubNodes.Count);

					Assert.IsTrue(conditional.SubNodes.ContainsKey("true"));
					Assert.IsInstanceOfType(conditional.SubNodes["true"], typeof(Property.String));
					Assert.AreEqual("\n\t\talpha\n\t\tbeta\n\t", (conditional.SubNodes["true"] as Property.String).Value);
				}

				Assert.IsInstanceOfType(composite.SubNodes[2], typeof(Property.String));
				Assert.AreEqual("\n\t\tgamma\n\t\tdelta\n\t", (composite.SubNodes[2] as Property.String).Value);
			}
		}
	}
}