using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

using NAnt.Core;
using NAnt.Core.Util;

namespace EA.FrameworkTasks.Model
{
    public class BuildStep : BitMask
    {
        public const uint PreBuild = 1;
        public const uint PreLink = 2;
        public const uint PostBuild = 4;
        public const uint Build = 8;
        public const uint Clean = 16;
        public const uint ReBuild = 32;
        public const uint ExecuteAlways = 64;

        public BuildStep(string stepname, uint type, List<TargetCommand> targets = null, List<Command> commands = null)
            : base(type)
        {
            Name = stepname;
            TargetCommands = targets ?? new List<TargetCommand>();
            Commands = commands ?? new List<Command>();
        }

        public readonly string Name;
        public readonly IList<TargetCommand> TargetCommands;
        public readonly IList<Command>  Commands;
        public List<PathString> InputDependencies;
        public List<PathString> OutputDependencies;
        
        /// <summary>
        /// Name of the target a custom build step should run before (build, link, run).
        /// Supported by native NAnt and MSBuild.
        /// </summary>
        public string Before;

        /// <summary>
        /// Name of the target a custom build step should run after (build, run).
        /// Supported by native NAnt and MSBuild.
        /// </summary>
        public string After;
    }
}
