// Copyright (C) Electronic Arts Canada Inc. 2002.  All rights reserved.

using System;
using NAnt.Core;

namespace EA.CPlusPlusTasks {

	public interface IExternalBuildTask
	{
		/// <summary>Executed by the build target.</summary>
		void ExecuteBuild(BuildTask buildTask, OptionSet optionSet);
	}
}
