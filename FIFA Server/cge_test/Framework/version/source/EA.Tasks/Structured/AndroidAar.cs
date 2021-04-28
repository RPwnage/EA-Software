using NAnt.Core.Attributes;

namespace EA.Eaconfig.Structured
{
	[TaskName("AndroidAar", NestedElementsCheck = true, StrictAttributeCheck = true)]
	public class AndroidAar : JavaModuleBaseTask
	{
		public AndroidAar() : base("AndroidAar")
		{
		}
	}
}