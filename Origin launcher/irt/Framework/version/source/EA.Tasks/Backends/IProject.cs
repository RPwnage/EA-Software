using EA.FrameworkTasks.Model;
using NAnt.Core.Util;

namespace EA.Eaconfig.Backends
{
	public interface IProject : IMultiConfiguration
	{
		PathString OutputDir { get; }

		string ProjectFileName { get; }
		
		string Name { get; }

		string RelativeDir { get; }
	}
}
