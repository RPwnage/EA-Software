using System;
using System.IO;
using System.Reflection;

namespace EA
{
	// purpose of this class is to be an object that upon creation registers a special assembly resolve handler, it is to designed to be instantiated from MSBuild's
	// default app domain inside of the short lived app domains created by EAPresentationBuildTasks (see TaskHelper.cs, MarkupCompilePassX.cs, etc)
	[Serializable]
	public class AssemblyResolver : MarshalByRefObject
	{
		public AssemblyResolver()
		{
			AppDomain.CurrentDomain.AssemblyResolve += CurrentDomain_AssemblyResolve;
		}

		Assembly CurrentDomain_AssemblyResolve(object sender, ResolveEventArgs args)
		{
			string shortAssemblyName = args.Name.Substring(0, args.Name.IndexOf(','));
			if (shortAssemblyName == "EAPresentationBuildTasks")
			{
				// we know the presentation tasks dll will also be located alongside this dll
				string eaBuildTaskAssemblyPath = Path.ChangeExtension(Path.Combine(Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location), shortAssemblyName), ".dll");
				if (File.Exists(eaBuildTaskAssemblyPath))
				{
					return Assembly.LoadFrom(eaBuildTaskAssemblyPath);
				}
			}
			return null; // fall back to default handlers, which will give familiar errors if dll cannot be located
		}
	}
}
