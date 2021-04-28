using System;

namespace NuGetDepender
{
	class Program
	{
		static void Main(string[] args)
		{
			Console.WriteLine(NuGet.Constants.NuGetVersion);
			Console.WriteLine(log4net.AssemblyInfo.Version);
			Console.WriteLine(Newtonsoft.Json.WriteState.Array);
			Console.WriteLine(System.Data.Entity.EntityState.Added);
		}
	}
}
