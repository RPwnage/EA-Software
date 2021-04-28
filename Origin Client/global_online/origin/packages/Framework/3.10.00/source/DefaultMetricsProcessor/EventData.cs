using System;
using System.Collections.Generic;
using System.Text;
using NAnt.Core;
using NAnt.Core.Events;
using NAnt.Core.Metrics;

namespace EA.MetricsProcessor
{
    public enum ResultType { Started=0, Sucess=1, Failure=2 };

    public class BuildEventData : IBuildEventData
    {
        public ProjectEventArgs.BuildStatus Status;
        public string BuildFileName;
        public string MasterconfigFile;
    }

    public class TargetEventData : IBuildEventData
    {
        public enum TargetType { Undefined, Top, BuildCaller };

        public TargetType Type;
        public string TargetName;
        public string Config;
        public string ConfigSystem;
        public string ConfigCompiler;
        public string BuildGroup;
        public ResultType Result;
    }

    public class TaskEventData : IBuildEventData
    {
        public string TargetName;
        public string TaskName;
        public string PackageName;
        public string PackageVersion;
        public string BuildGroup;
        public string Config;
        public string ConfigSystem;
        public string ConfigCompiler;
        public bool IsUse;
        public bool IsBuild;
        public bool Autobuild;
        public bool IsFramework2;
        public ResultType Result;
        public bool IsAlreadyBuilt = false;
    }

}
