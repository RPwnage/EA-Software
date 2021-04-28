using System.Xml.Linq;

namespace NAnt.LangTools
{
	public sealed partial class Parser
	{
		internal sealed class ConditionalPropertyElement
		{
			internal Property.Node Append { get; private set; } // note - this isn't used internally for appending multiple value, but externally for determining how the value should be used
			internal Property.Node Value { get; private set; } = null;

			internal ConditionalPropertyElement(bool defaultAppend = true)
				: base()
			{
				Append = defaultAppend.ToString();
			}

			internal void InitOrUpdateFromElement(Parser parser, XElement conditionalPropertyElement)
			{
				ValidateAttributes(conditionalPropertyElement, "if", "unless", "append", "value");

				Property.Node newValueNode = null;
				Property.Node newAppendNode = null;
				parser.HandleIfUnless
				(
					conditionalPropertyElement, () =>
					{
						parser.ProcessSubElements(conditionalPropertyElement, AllowNestedDo);
						newValueNode = AttributeOrContentsNode(parser, conditionalPropertyElement, allowDoElement: true);

						string appendAttrString = conditionalPropertyElement.Attribute("append")?.Value;
						if (appendAttrString != null)
						{
							ScriptEvaluation.Node scriptNode = ScriptEvaluation.Evaluate(appendAttrString);
							newAppendNode = parser.ExpandByCurrentConditions(parser.ResolveScriptEvaluation(scriptNode));
						}
					}
				);

				if (newValueNode != null)
				{
					Value = Value?.Append(newValueNode) ?? newValueNode;
				};
				if (newAppendNode != null)
				{
					Append = Append?.Update(newAppendNode) ?? newAppendNode;
				}
			}
		}
	}
}
