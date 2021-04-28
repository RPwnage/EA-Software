using NAnt.Core;
using NAnt.Core.Attributes;
using NAnt.Core.Logging;

using System;
using System.Collections;
using System.Collections.Generic;
using System.Text;
using System.IO;

namespace EA.Eaconfig
{
    public abstract class GeneratePlatformOptions : GenerateOptions
    {
        public GeneratePlatformOptions() : base()
        {
        }

        public void Init(Project project, OptionSet configOptionSet, string configsetName)
        {
            Project = project;
            ConfigOptionSet = configOptionSet;
            ConfigSetName = configsetName;
            InternalInit();
        }

        protected override void GeneratePlatformSpecificOptions()
        {
            SetPlatformOptions();
        }

        protected abstract void SetPlatformOptions();
    }
}
