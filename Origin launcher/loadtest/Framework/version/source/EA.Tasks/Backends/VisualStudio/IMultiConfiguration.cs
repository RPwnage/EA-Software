using System;
using System.Collections.Generic;
using EA.FrameworkTasks.Model;

namespace EA.Eaconfig.Backends
{
	public interface IMultiConfiguration
	{
		IDictionary<Configuration, Configuration> ConfigurationMap { get; set; }

		List<Configuration> AllConfigurations { get; }
	}
}
