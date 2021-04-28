using NAnt.Core.Attributes;
using NAnt.Core.Structured;

namespace EA.Eaconfig.Structured
{
    public class BuildLayoutElement : ConditionalElementContainer
	{
		[TaskAttribute("enable")]
		public bool? Enable { get; set; } = null;

		[TaskAttribute("build-type")]
		public string BuildType { get; set; } = null;

		[BuildElement("tags")]
		public StructuredOptionSet Tags { get; } = new StructuredOptionSet();
	}
}