using NAnt.Core.Attributes;

namespace EA.Eaconfig.Structured
{
	[TaskName("AndroidApk", NestedElementsCheck = true, StrictAttributeCheck = true)]
	public class AndroidApk : JavaModuleBaseTask
	{
		public AndroidApk() : base("AndroidApk")
		{
		}
	}
}