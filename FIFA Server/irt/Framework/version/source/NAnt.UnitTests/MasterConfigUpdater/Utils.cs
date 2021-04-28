using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using NAnt.Core.PackageCore;
using NAnt.Tests.Util;

namespace NAnt.MasterConfigUpdaterTest
{
    internal static class Utils
    {
        internal static void VerifyChanged(string inputContents, Action<MasterConfigUpdater> updateFunction, string expectedContents, Dictionary<string, Tuple<string, string>> fragments = null)
        {
            using (MasterConfigUpdateTestCase testMasterconfig = new MasterConfigUpdateTestCase(inputContents, fragments))
            {
                testMasterconfig.VerifyXmlLoad();
                testMasterconfig.VerifyNantLoad();

                MasterConfigUpdater testUpdater = MasterConfigUpdater.Load(testMasterconfig.MasterConfigPath, testMasterconfig.Project);
                updateFunction(testUpdater);
                testUpdater.Save();

                string postUpdateContents = File.ReadAllText(testMasterconfig.MasterConfigPath);
				Assert.IsTrue(expectedContents == postUpdateContents, Diff(testMasterconfig.MasterConfigPath, expectedContents, postUpdateContents));

                foreach (KeyValuePair<string, Tuple<string, string>> fragmentPathToContents in testMasterconfig.Fragments)
                {
                    string expectedFragmentPostUpdateContents = fragmentPathToContents.Value.Item2;
                    string postFragmentUpdateContents = File.ReadAllText(fragmentPathToContents.Key);

					Assert.IsTrue(expectedFragmentPostUpdateContents == postFragmentUpdateContents, Diff(fragmentPathToContents.Key, expectedFragmentPostUpdateContents, postUpdateContents));
                }
            }
        }

        internal static void VerifyUnchanged(string contents, Dictionary<string, string> fragments = null)
        {
			VerifyChanged
			(
                inputContents: contents,
                updateFunction: updater => Assert.IsTrue(!updater.GetPathsToModifyOnSave().Any(), "No files should need to be modified!"),
                expectedContents: contents,
                fragments: (fragments ?? new Dictionary<string, string>()).ToDictionary((KeyValuePair<string, string> pair) => pair.Key, (KeyValuePair<string, string> pair) => new Tuple<string, string>(pair.Value, pair.Value))
            );
        }

        internal static Exception VerifyThrows<T>(string contents, Action<MasterConfigUpdater> updateFunction = null, Dictionary<string, string> fragments = null) where T : Exception
		{
            using (MasterConfigUpdateTestCase testMasterconfig = new MasterConfigUpdateTestCase(contents, (fragments ?? new Dictionary<string, string>()).ToDictionary(pair => pair.Key, pair => new Tuple<string, string>(pair.Value, pair.Value))))
            {
                return Assert.ThrowsException<T>(() =>
                {
					MasterConfigUpdater updater = MasterConfigUpdater.Load(testMasterconfig.MasterConfigPath, testMasterconfig.Project);
					updateFunction?.Invoke(updater);
				});
            }
        }

        internal static string Diff(string fileSource, string expectedPostUpdateContents, string postUpdateContents)
        {
            for (int i = 0; i < expectedPostUpdateContents.Length; ++i)
            {
                if (postUpdateContents.Length <= i)
                {
                    return fileSource + " did not match! Contents shorter than expected!";
                }

                if (expectedPostUpdateContents[i] != postUpdateContents[i])
                {
                    return fileSource + " output did not match expected! Difference at offset " + i + "!";
                }
            }

            if (expectedPostUpdateContents.Length < postUpdateContents.Length)
            {
                return fileSource + " output did not match! Contents longer than expected!";
            }

            return "Contents matched!";
        }
    }
}