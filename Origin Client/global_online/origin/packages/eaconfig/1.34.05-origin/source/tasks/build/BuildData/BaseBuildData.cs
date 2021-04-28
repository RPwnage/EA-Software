using NAnt.Core;
using NAnt.Core.Attributes;
using NAnt.Core.Functions;
using NAnt.Core.Logging;
using NAnt.Core.Tasks;

using System;
using System.Collections;
using System.Collections.Generic;
using System.Text;
using System.IO;

namespace EA.Eaconfig
{
    public class BaseBuildData 
    {
        public enum CopyLocalType { True, False, Slim };

        public BuildType     BuildType;
        public bool          BulkBuild = true;
        public CopyLocalType CopyLocal = CopyLocalType.False;

        public string        GroupOutputDir; // ${eaconfig.${eaconfig.build.group}.outputfolder}

        public readonly OptionSet BuildOptionSet = new OptionSet();

        public readonly Dictionary<string, string> BuildProperties = new Dictionary<string,string>();
        public readonly Dictionary<string, FileSet> BuildFileSets = new Dictionary<string, FileSet>();
    }
}

