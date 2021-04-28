// (c) 2002-2003 Electronic Arts Inc.
//
// Kosta Arvanitis (karvanitis@ea.com)

using System;

namespace EA.FrameworkTasks {

	public interface IDependentTask {

		string PackageName {
			get;
		}

		string PackageVersion {
			get;
		}
	}
}