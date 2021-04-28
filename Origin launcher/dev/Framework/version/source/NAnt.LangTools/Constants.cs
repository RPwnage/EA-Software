using System.Collections.Generic;
using System.Collections.ObjectModel;

namespace NAnt.LangTools
{
	public static class Constants
	{
		public readonly static IEnumerable<string> Platforms = new string[]
		{
			"android",
			"capilano",
			"iphone",
			"kettle",
			"nx",
			"osx",
			"pc",
			"pc64",
			"ps5",
			"stadia",
			"unix",
			"unix64",
			"xbsx"
		};

		public readonly static IEnumerable<string> Compilers = new string[]
		{
			"vc",
			"clang"
		};

		public readonly static IEnumerable<string> BooleanOptions = new string[]
		{
			"true",
			"false"
		};

		public readonly static Condition PlatformCondition = new Condition("Platform", Platforms);
		public readonly static Condition SharedBuildCondition = new Condition("Shared Builds", BooleanOptions);
		public readonly static Condition CompilerCondition = new Condition("Compiler", Compilers);

		internal readonly static char[] DefaultDelimiters = {
			'\x0009', // tab
			'\x000a', // newline
			'\x000d', // carriage return
			'\x0020'  // space
		};

		internal readonly static char[] LinesDelimiters = {
			'\x000a', // newline
			'\x000d'  // carriage return
		};

		internal readonly static string Newline = "\n"; // using "\n" universally in an attempt to ignore platform specific newline concerns
	}
}
