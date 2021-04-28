using NAnt.Core.Attributes;

namespace EA.Eaconfig.Structured
{
	[TaskName("JavaLibrary", NestedElementsCheck = true, StrictAttributeCheck = true)]
	public class JavaLibrary : JavaModuleBaseTask
	{
		public JavaLibrary() : base("JavaLibrary")
		{
		}
	}
}