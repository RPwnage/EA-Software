using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.IO;
using System.Linq;
using System.Reflection;

using Microsoft.VisualStudio.TestTools.UnitTesting;

namespace NAnt.LangTools.FunctionalTests
{
	[TestClass]
	public class FunctionalTests
	{
		[TestMethod]
		public void EABase_2_09_09()
		{
			Model model = FunctionalTest("EABase", "2.09.09");
		}

		[TestMethod] 
		public void EASTL_3_16_01()
		{
			Feature featureEATechWarningsAsErrors = SinglePropertyFeature("EATECH_WARNINGSASERRORS", "0", "1");
			Feature featureEASTLTestBuildExtras = SinglePropertyFeature("EASTLTest.BuildExtras", "0", "1");

			Feature featureEASTLAllocatorCopyEnabled = SinglePropertyFeature("EASTL.EASTL_ALLOCATOR_COPY_ENABLED", "0", "1");
			Feature featureEASTLAllocatorExplicitEnabled = SinglePropertyFeature("EASTL.EASTL_ALLOCATOR_EXPLICIT_ENABLED", "0", "1");
			Feature featureEASTLAllocatorMinAlignment = SinglePropertyFeature("EASTL.EASTL_ALLOCATOR_MIN_ALIGNMENT", "0", "1");
			Feature featureEASTLAssert = SinglePropertyFeature("EASTL.EASTL_ASSERT", "0", "1");
			Feature featureEASTLAssertEnabled = SinglePropertyFeature("EASTL.EASTL_ASSERT_ENABLED", "0", "1");
			Feature featureEASTLCoreAllocatorEnabled = SinglePropertyFeature("EASTL.EASTL_CORE_ALLOCATOR_ENABLED", "0", "1");
			Feature featureEASTLDebug = SinglePropertyFeature("EASTL.EASTL_DEBUG", "0", "1");
			Feature featureEASTLDebugBreak = SinglePropertyFeature("EASTL.EASTL_DEBUG_BREAK", "0", "1");
			Feature featureEASTLDebugBreakOverride = SinglePropertyFeature("EASTL.EASTL_DEBUG_BREAK_OVERRIDE", "0", "1");
			Feature featureEASTLDefaultNamePrefix = SinglePropertyFeature("EASTL.EASTL_DEFAULT_NAME_PREFIX", "0", "1");
			Feature featureEASTLDefaultSortFunction = SinglePropertyFeature("EASTL.EASTL_DEFAULT_SORT_FUNCTION", "0", "1");
			Feature featureEASTLDefaultStableSortFunction = SinglePropertyFeature("EASTL.EASTL_DEFAULT_STABLE_SORT_FUNCTION", "0", "1");
			Feature featureEASTLDevAssert = SinglePropertyFeature("EASTL.EASTL_DEV_ASSERT", "0", "1");
			Feature featureEASTLDevAssertEnabled = SinglePropertyFeature("EASTL.EASTL_DEV_ASSERT_ENABLED", "0", "1");
			Feature featureEASTLDevDebug = SinglePropertyFeature("EASTL.EASTL_DEV_DEBUG", "0", "1");
			Feature featureEASTL_EASTDCVsnprintf = SinglePropertyFeature("EASTL.EASTL_EASTDC_VSNPRINTF", "0", "1");
			Feature featureEASTLExceptionsEnabled = SinglePropertyFeature("EASTL.EASTL_EXCEPTIONS_ENABLED", "0", "1");
			Feature featureEASTLFixedSizeTrackingEnabled = SinglePropertyFeature("EASTL.EASTL_FIXED_SIZE_TRACKING_ENABLED", "0", "1");
			Feature featureEASTLListSizeCache = SinglePropertyFeature("EASTL.EASTL_LIST_SIZE_CACHE", "0", "1");
			Feature featureEASTLMaxStackUsage = SinglePropertyFeature("EASTL.EASTL_MAX_STACK_USAGE", "0", "1");
			Feature featureEASTLMinMaxEnabled = SinglePropertyFeature("EASTL.EASTL_MINMAX_ENABLED", "0", "1");
			Feature featureEASTLNameEnabled = SinglePropertyFeature("EASTL.EASTL_NAME_ENABLED", "0", "1");
			Feature featureEASTLOperatorEqualsOtherEnabled = SinglePropertyFeature("EASTL.EASTL_OPERATOR_EQUALS_OTHER_ENABLED", "0", "1");
			Feature featureEASTLResetEnabled = SinglePropertyFeature("EASTL.EASTL_RESET_ENABLED", "0", "1");
			Feature featureEASTLSizeT32bit = SinglePropertyFeature("EASTL.EASTL_SIZE_T_32BIT", "0", "1");
			Feature featureEASTLSListSizeCache = SinglePropertyFeature("EASTL.EASTL_SLIST_SIZE_CACHE", "0", "1");
			Feature featureEASTLStdIteratorCategoryEnabled = SinglePropertyFeature("EASTL.EASTL_STD_ITERATOR_CATEGORY_ENABLED", "0", "1");
			Feature featureEASTLUserConfigHeader = SinglePropertyFeature("EASTL.EASTL_USER_CONFIG_HEADER", "0", "1");
			Feature featureEASTLValidateCompareEnabled = SinglePropertyFeature("EASTL.EASTL_VALIDATE_COMPARE_ENABLED", "0", "1");
			Feature featureEASTLValidateIntrusiveList = SinglePropertyFeature("EASTL.EASTL_VALIDATE_INTRUSIVE_LIST", "0", "1");
			Feature featureEASTLValidationEnabled = SinglePropertyFeature("EASTL.EASTL_VALIDATION_ENABLED", "0", "1");

			Model model = FunctionalTest
			(
				"EASTL", "3.16.01",

				// control flow features
				featureEATechWarningsAsErrors,
				featureEASTLTestBuildExtras ,

				// defines control features
				featureEASTLAllocatorCopyEnabled,
				featureEASTLAllocatorExplicitEnabled,
				featureEASTLAllocatorMinAlignment,
				featureEASTLAssert,
				featureEASTLAssertEnabled,
				featureEASTLCoreAllocatorEnabled,
				featureEASTLDebug,
				featureEASTLDebugBreak,
				featureEASTLDebugBreakOverride,
				featureEASTLDefaultNamePrefix,
				featureEASTLDefaultSortFunction,
				featureEASTLDefaultStableSortFunction,
				featureEASTLDevAssert,
				featureEASTLDevAssertEnabled,
				featureEASTLDevDebug,
				featureEASTL_EASTDCVsnprintf,
				featureEASTLExceptionsEnabled,
				featureEASTLFixedSizeTrackingEnabled,
				featureEASTLListSizeCache,
				featureEASTLMaxStackUsage,
				featureEASTLMinMaxEnabled,
				featureEASTLNameEnabled,
				featureEASTLOperatorEqualsOtherEnabled,
				featureEASTLResetEnabled,
				featureEASTLSizeT32bit,
				featureEASTLSListSizeCache,
				featureEASTLStdIteratorCategoryEnabled,
				featureEASTLUserConfigHeader,
				featureEASTLValidateCompareEnabled,
				featureEASTLValidateIntrusiveList,
				featureEASTLValidationEnabled
			);

			// EASTL
			Assert.IsTrue(model.RuntimeModules.ContainsKey("EASTL"));
			Model.Module eastl = model.RuntimeModules["EASTL"];
			Assert.AreEqual(eastl.Conditions, ConditionValueSet.True);

			// TODO public build type

			Assert.AreEqual(2, eastl.PrivateBuildType.Count());
			VerifyEntry(eastl.PrivateBuildType, "LibraryExtra", Constants.SharedBuildCondition, "false");
			VerifyEntry(eastl.PrivateBuildType, "DynamicLibrary", Constants.SharedBuildCondition, "true");

			// todo: need to tease out *base* buildtype for this to be useful

			Assert.AreEqual(1, eastl.PublicIncludeDirs.Count());
			VerifyEntry(eastl.PublicIncludeDirs, "%PackageRootPlaceHolder[EASTL]%/include");

			Assert.AreEqual(62, eastl.PublicDefines.Count());
			VerifyEntry(eastl.PublicDefines, "EASTL_ALLOCATOR_COPY_ENABLED=0", featureEASTLAllocatorCopyEnabled.Condition, "0");
			VerifyEntry(eastl.PublicDefines, "EASTL_ALLOCATOR_COPY_ENABLED=1", featureEASTLAllocatorCopyEnabled.Condition, "1");
			VerifyEntry(eastl.PublicDefines, "EASTL_ALLOCATOR_EXPLICIT_ENABLED=0", featureEASTLAllocatorExplicitEnabled.Condition, "0");
			VerifyEntry(eastl.PublicDefines, "EASTL_ALLOCATOR_EXPLICIT_ENABLED=1", featureEASTLAllocatorExplicitEnabled.Condition, "1");
			VerifyEntry(eastl.PublicDefines, "EASTL_ALLOCATOR_MIN_ALIGNMENT=0", featureEASTLAllocatorMinAlignment.Condition, "0");
			VerifyEntry(eastl.PublicDefines, "EASTL_ALLOCATOR_MIN_ALIGNMENT=1", featureEASTLAllocatorMinAlignment.Condition, "1");
			VerifyEntry(eastl.PublicDefines, "EASTL_ASSERT=0", featureEASTLAssert.Condition, "0");
			VerifyEntry(eastl.PublicDefines, "EASTL_ASSERT=1", featureEASTLAssert.Condition, "1");
			VerifyEntry(eastl.PublicDefines, "EASTL_ASSERT_ENABLED=0", featureEASTLAssertEnabled.Condition, "0");
			VerifyEntry(eastl.PublicDefines, "EASTL_ASSERT_ENABLED=1", featureEASTLAssertEnabled.Condition, "1");
			VerifyEntry(eastl.PublicDefines, "EASTL_CORE_ALLOCATOR_ENABLED=0", featureEASTLCoreAllocatorEnabled.Condition, "0");
			VerifyEntry(eastl.PublicDefines, "EASTL_CORE_ALLOCATOR_ENABLED=1", featureEASTLCoreAllocatorEnabled.Condition, "1");
			VerifyEntry(eastl.PublicDefines, "EASTL_DEBUG=0", featureEASTLDebug.Condition, "0");
			VerifyEntry(eastl.PublicDefines, "EASTL_DEBUG=1", featureEASTLDebug.Condition, "1");
			VerifyEntry(eastl.PublicDefines, "EASTL_DEBUG_BREAK=0", featureEASTLDebugBreak.Condition, "0");
			VerifyEntry(eastl.PublicDefines, "EASTL_DEBUG_BREAK=1", featureEASTLDebugBreak.Condition, "1");
			VerifyEntry(eastl.PublicDefines, "EASTL_DEBUG_BREAK_OVERRIDE=0", featureEASTLDebugBreakOverride.Condition, "0");
			VerifyEntry(eastl.PublicDefines, "EASTL_DEBUG_BREAK_OVERRIDE=1", featureEASTLDebugBreakOverride.Condition, "1");
			VerifyEntry(eastl.PublicDefines, "EASTL_DEFAULT_NAME_PREFIX=0", featureEASTLDefaultNamePrefix.Condition, "0");
			VerifyEntry(eastl.PublicDefines, "EASTL_DEFAULT_NAME_PREFIX=1", featureEASTLDefaultNamePrefix.Condition, "1");
			VerifyEntry(eastl.PublicDefines, "EASTL_DEFAULT_SORT_FUNCTION=0", featureEASTLDefaultSortFunction.Condition, "0");
			VerifyEntry(eastl.PublicDefines, "EASTL_DEFAULT_SORT_FUNCTION=1", featureEASTLDefaultSortFunction.Condition, "1");
			VerifyEntry(eastl.PublicDefines, "EASTL_DEFAULT_STABLE_SORT_FUNCTION=0", featureEASTLDefaultStableSortFunction.Condition, "0");
			VerifyEntry(eastl.PublicDefines, "EASTL_DEFAULT_STABLE_SORT_FUNCTION=1", featureEASTLDefaultStableSortFunction.Condition, "1");
			VerifyEntry(eastl.PublicDefines, "EASTL_DEV_ASSERT=0", featureEASTLDevAssert.Condition, "0");
			VerifyEntry(eastl.PublicDefines, "EASTL_DEV_ASSERT=1", featureEASTLDevAssert.Condition, "1");
			VerifyEntry(eastl.PublicDefines, "EASTL_DEV_ASSERT_ENABLED=0", featureEASTLDevAssertEnabled.Condition, "0");
			VerifyEntry(eastl.PublicDefines, "EASTL_DEV_ASSERT_ENABLED=1", featureEASTLDevAssertEnabled.Condition, "1");
			VerifyEntry(eastl.PublicDefines, "EASTL_DEV_DEBUG=0", featureEASTLDevDebug.Condition, "0");
			VerifyEntry(eastl.PublicDefines, "EASTL_DEV_DEBUG=1", featureEASTLDevDebug.Condition, "1");
			VerifyEntry(eastl.PublicDefines, "EASTL_EASTDC_VSNPRINTF=0", featureEASTL_EASTDCVsnprintf.Condition, "0");
			VerifyEntry(eastl.PublicDefines, "EASTL_EASTDC_VSNPRINTF=1", featureEASTL_EASTDCVsnprintf.Condition, "1");
			VerifyEntry(eastl.PublicDefines, "EASTL_EXCEPTIONS_ENABLED=0", featureEASTLExceptionsEnabled.Condition, "0");
			VerifyEntry(eastl.PublicDefines, "EASTL_EXCEPTIONS_ENABLED=1", featureEASTLExceptionsEnabled.Condition, "1");
			VerifyEntry(eastl.PublicDefines, "EASTL_FIXED_SIZE_TRACKING_ENABLED=0", featureEASTLFixedSizeTrackingEnabled.Condition, "0");
			VerifyEntry(eastl.PublicDefines, "EASTL_FIXED_SIZE_TRACKING_ENABLED=1", featureEASTLFixedSizeTrackingEnabled.Condition, "1");
			VerifyEntry(eastl.PublicDefines, "EASTL_LIST_SIZE_CACHE=0", featureEASTLListSizeCache.Condition, "0");
			VerifyEntry(eastl.PublicDefines, "EASTL_LIST_SIZE_CACHE=1", featureEASTLListSizeCache.Condition, "1");
			VerifyEntry(eastl.PublicDefines, "EASTL_MAX_STACK_USAGE=0", featureEASTLMaxStackUsage.Condition, "0");
			VerifyEntry(eastl.PublicDefines, "EASTL_MAX_STACK_USAGE=1", featureEASTLMaxStackUsage.Condition, "1");
			VerifyEntry(eastl.PublicDefines, "EASTL_MINMAX_ENABLED=0", featureEASTLMinMaxEnabled.Condition, "0");
			VerifyEntry(eastl.PublicDefines, "EASTL_MINMAX_ENABLED=1", featureEASTLMinMaxEnabled.Condition, "1");
			VerifyEntry(eastl.PublicDefines, "EASTL_NAME_ENABLED=0", featureEASTLNameEnabled.Condition, "0");
			VerifyEntry(eastl.PublicDefines, "EASTL_NAME_ENABLED=1", featureEASTLNameEnabled.Condition, "1");
			VerifyEntry(eastl.PublicDefines, "EASTL_OPERATOR_EQUALS_OTHER_ENABLED=0", featureEASTLOperatorEqualsOtherEnabled.Condition, "0");
			VerifyEntry(eastl.PublicDefines, "EASTL_OPERATOR_EQUALS_OTHER_ENABLED=1", featureEASTLOperatorEqualsOtherEnabled.Condition, "1");
			VerifyEntry(eastl.PublicDefines, "EASTL_RESET_ENABLED=0", featureEASTLResetEnabled.Condition, "0");
			VerifyEntry(eastl.PublicDefines, "EASTL_RESET_ENABLED=1", featureEASTLResetEnabled.Condition, "1");
			VerifyEntry(eastl.PublicDefines, "EASTL_SIZE_T_32BIT=0", featureEASTLSizeT32bit.Condition, "0");
			VerifyEntry(eastl.PublicDefines, "EASTL_SIZE_T_32BIT=1", featureEASTLSizeT32bit.Condition, "1");
			VerifyEntry(eastl.PublicDefines, "EASTL_SLIST_SIZE_CACHE=0", featureEASTLSListSizeCache.Condition, "0");
			VerifyEntry(eastl.PublicDefines, "EASTL_SLIST_SIZE_CACHE=1", featureEASTLSListSizeCache.Condition, "1");
			VerifyEntry(eastl.PublicDefines, "EASTL_STD_ITERATOR_CATEGORY_ENABLED=0", featureEASTLStdIteratorCategoryEnabled.Condition, "0");
			VerifyEntry(eastl.PublicDefines, "EASTL_STD_ITERATOR_CATEGORY_ENABLED=1", featureEASTLStdIteratorCategoryEnabled.Condition, "1");
			VerifyEntry(eastl.PublicDefines, "EASTL_USER_CONFIG_HEADER=0", featureEASTLUserConfigHeader.Condition, "0");
			VerifyEntry(eastl.PublicDefines, "EASTL_USER_CONFIG_HEADER=1", featureEASTLUserConfigHeader.Condition, "1");
			VerifyEntry(eastl.PublicDefines, "EASTL_VALIDATE_COMPARE_ENABLED=0", featureEASTLValidateCompareEnabled.Condition, "0");
			VerifyEntry(eastl.PublicDefines, "EASTL_VALIDATE_COMPARE_ENABLED=1", featureEASTLValidateCompareEnabled.Condition, "1");
			VerifyEntry(eastl.PublicDefines, "EASTL_VALIDATE_INTRUSIVE_LIST=0", featureEASTLValidateIntrusiveList.Condition, "0");
			VerifyEntry(eastl.PublicDefines, "EASTL_VALIDATE_INTRUSIVE_LIST=1", featureEASTLValidateIntrusiveList.Condition, "1");
			VerifyEntry(eastl.PublicDefines, "EASTL_VALIDATION_ENABLED=0", featureEASTLValidationEnabled.Condition, "0");
			VerifyEntry(eastl.PublicDefines, "EASTL_VALIDATION_ENABLED=1", featureEASTLValidationEnabled.Condition, "1");

			// Assert.AreEqual(1, eastl.PrivateIncludeDirs.Count()); // TODO doesn't detect default include dir yet

			// EASTLTest
			Assert.IsTrue(model.TestModules.ContainsKey("EASTLTest"));
			Model.Module eastlTest = model.TestModules["EASTLTest"];
			Assert.AreEqual(eastlTest.Conditions, ConditionValueSet.True);
		}

		[TestMethod] 
		public void EAUser_1_07_03()
		{
			Model model = FunctionalTest("EAUser", "1.07.03");
		}

		[TestMethod]
		public void EAThread_1_33_00()
		{
			Model model = FunctionalTest("EAThread", "1.33.00");
		}

		private static Model FunctionalTest(string name, string version, params Feature[] features)
		{
			string buildFilePath = Path.Combine(Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location), $"{name}-{version}", $"{name}.build");
			return Model.CreateFromPackageFile(name, buildFilePath, features);
		}

		private static Feature SinglePropertyFeature(string propertyName, params string[] values)
		{
			return new Feature(propertyName, values.ToDictionary(value => value, value => new Dictionary<string, string> { { propertyName, value } }));
		}

		private static void VerifyEntry(ReadOnlyCollection<Model.ConditionalEntry> entries, string value) => VerifyEntry(entries, value, ConditionValueSet.True);
		private static void VerifyEntry(ReadOnlyCollection<Model.ConditionalEntry> entries, string value, Condition condition, string conditionValue) => VerifyEntry(entries, value, ConditionValueSet.True.And(condition, conditionValue));
		private static void VerifyEntry(ReadOnlyDictionary<string, ConditionValueSet> entries, string value) => VerifyEntry(entries, value, ConditionValueSet.True);
		private static void VerifyEntry(ReadOnlyDictionary<string, ConditionValueSet> entries, string value, Condition condition, string conditionValue) => VerifyEntry(entries, value, ConditionValueSet.True.And(condition, conditionValue));

		private static void VerifyEntry(ReadOnlyDictionary<string, ConditionValueSet> entries, string value, ConditionValueSet conditionValueSet)
		{
			if (!entries.TryGetValue(value, out ConditionValueSet entryConditions))
			{
				Assert.Fail($"No matching entry for value '{value}'.");
			}
			else if (entryConditions != conditionValueSet)
			{
				Assert.Fail($"Entry '{value}' value does not have expected conditions ({conditionValueSet}), instead has '{entryConditions}'.");
			}
		}

		private static void VerifyEntry(ReadOnlyCollection<Model.ConditionalEntry> entries, string value, ConditionValueSet conditionValueSet)
		{
			Model.ConditionalEntry match = entries.SingleOrDefault(entry => entry.Value == value && entry.Conditions == conditionValueSet);
			if (match == null)
			{
				Assert.Fail($"No matching entry for value '{value}' with conditions '{conditionValueSet}'.");
			}
		}
	}
}