// Copyright (C) Electronic Arts Canada Inc. 2002.  All rights reserved.

using System;
using NAnt.Core;

namespace EA.CPlusPlusTasks {

	/// <summary>A class from which all external build tasks will derive.</summary>
	/// <remarks>
	/// This class is deprecated, please inherit from Task and implement IExternalBuildTask instead.
	/// </remarks>
    public abstract class ExternalBuildTask : Task
    {
        /// <summary>Initializes the task.</summary>
        protected override void InitializeTask(System.Xml.XmlNode taskNode) 
        {
            // default implmentation is blank
        }

        /// <summary>Execute the task.</summary>
        protected override void ExecuteTask() 
        {
            // default implmentation is blank
        }

        /// <summary>Executed by the build target.</summary>
        public abstract void ExecuteBuild(BuildTask buildTask, OptionSet optionSet);
    }
}
