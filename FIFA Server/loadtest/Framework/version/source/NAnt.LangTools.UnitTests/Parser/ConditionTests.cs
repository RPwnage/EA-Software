using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;

using Microsoft.VisualStudio.TestTools.UnitTesting;

namespace NAnt.LangTools.UnitTests
{
	[TestClass]
	public class ConditionTests
	{
		[TestMethod]
		public void OrReductionTest()
		{
			Condition featureOneCondition = TestUtils.CreateBooleanCondition("FeatureOne");
			ConditionValueSet trueSet = ConditionValueSet.True.And(featureOneCondition, "true");
			ConditionValueSet falseSet = ConditionValueSet.True.And(featureOneCondition, "false");

			Condition featureTwoCondition = TestUtils.CreateBooleanCondition("FeatureTwo");
			trueSet = trueSet.And(featureTwoCondition, "true");
			falseSet = falseSet.And(featureTwoCondition, "true");

			ConditionValueSet orSet = trueSet.Or(falseSet);
			Assert.AreEqual(1, orSet.Count());

			ReadOnlyDictionary<Condition, string> firstSet = orSet.First();
			Assert.AreEqual(1, firstSet.Count());

			KeyValuePair<Condition, string> firstPair = firstSet.First();
			Assert.AreEqual(featureTwoCondition, firstPair.Key);
			Assert.AreEqual("true", firstPair.Value);
		}

		[TestMethod]
		public void IndividualNegation()
		{
			Condition featureCondition = TestUtils.CreateBooleanCondition("Feature");
			ConditionValueSet set = ConditionValueSet.True;
			set = set.And(featureCondition, "true");
			set = set.And(featureCondition, "false");

			Assert.AreEqual(set, ConditionValueSet.False);
		}

		[TestMethod]
		public void GroupNegation()
		{
			Condition featureCondition = TestUtils.CreateBooleanCondition("Feature");
			ConditionValueSet trueSet = ConditionValueSet.True.And(featureCondition, "true");
			ConditionValueSet falseSet = ConditionValueSet.True.And(featureCondition, "false");

			Assert.AreEqual(ConditionValueSet.False, trueSet.And(falseSet));
		}

		[TestMethod]
		public void TrueWhen()
		{
			Condition featureOneCondition = TestUtils.CreateBooleanCondition("FeatureOne");
			ConditionValueSet one = ConditionValueSet.True.And(featureOneCondition, "true");

			Condition featureTwoCondition = TestUtils.CreateBooleanCondition("FeatureTwo");
			ConditionValueSet two = ConditionValueSet.True.And(featureTwoCondition, "true");
			ConditionValueSet notTwo = ConditionValueSet.True.And(featureTwoCondition, "false");

			Condition featureThreeCondition = TestUtils.CreateBooleanCondition("FeatureThree");
			ConditionValueSet three = ConditionValueSet.True.And(featureThreeCondition, "true");

			Condition featureFourCondition = TestUtils.CreateBooleanCondition("FeatureFour");
			ConditionValueSet four = ConditionValueSet.True.And(featureFourCondition, "true");

			ConditionValueSet oneAndTwo = one.And(two);
			ConditionValueSet oneOrTwo = one.Or(two);
			ConditionValueSet oneOrNotTwo = one.Or(notTwo);
			ConditionValueSet threeAndFour = three.And(four);
			ConditionValueSet oneAndTwoOrThreeAndFour = oneAndTwo.Or(threeAndFour);

			Assert.IsTrue(one.TrueWhen(oneOrTwo));
			Assert.IsTrue(one.TrueWhen(oneOrNotTwo));
			Assert.IsTrue(one.TrueWhen(oneAndTwo));
			Assert.IsTrue(oneOrTwo.TrueWhen(one));
			Assert.IsTrue(oneOrNotTwo.TrueWhen(one));
			Assert.IsFalse(oneAndTwo.TrueWhen(one));
			Assert.IsTrue(two.TrueWhen(oneOrTwo));
			Assert.IsTrue(oneOrTwo.TrueWhen(two));
			Assert.IsFalse(two.TrueWhen(oneOrNotTwo));
			Assert.IsFalse(oneOrNotTwo.TrueWhen(two));
			Assert.IsTrue(notTwo.TrueWhen(oneOrNotTwo));
			Assert.IsTrue(oneOrNotTwo.TrueWhen(notTwo));
			Assert.IsTrue(oneAndTwo.TrueWhen(oneAndTwoOrThreeAndFour));
			Assert.IsTrue(oneAndTwoOrThreeAndFour.TrueWhen(oneAndTwo));
			Assert.IsTrue(threeAndFour.TrueWhen(oneAndTwoOrThreeAndFour));
			Assert.IsTrue(oneAndTwoOrThreeAndFour.TrueWhen(threeAndFour));
		}
	}
}