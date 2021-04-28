using System.Collections.Generic;
using System.Collections.ObjectModel;

namespace NAnt.LangTools
{
	// TODO: this doesn't really do anything anymore other than wrap a condition have some init properties - maybe it can be replaced / removed?
	public class Feature
	{
		internal readonly Condition Condition;
		internal readonly ReadOnlyDictionary<string, Property.Node> Properties;

		/*internal  - just internal for hacky demo! revert!*/ public Feature(string name, Dictionary<string, Dictionary<string, string>> valuesToProperties)
		{
			Condition = new Condition(name, valuesToProperties.Keys);

			Dictionary<string, Property.Node> properties = new Dictionary<string, Property.Node>();
			foreach (KeyValuePair<string, Dictionary<string, string>> valueToDict in valuesToProperties)
			{
				foreach (KeyValuePair<string, string> propertyNameToValue in valueToDict.Value)
				{
					Property.Conditional newValue = new Property.Conditional(Condition, new Dictionary<string, Property.Node>() { { valueToDict.Key, new Property.String(propertyNameToValue.Value) } });
					if (!properties.TryGetValue(propertyNameToValue.Key, out Property.Node property))
					{
						properties[propertyNameToValue.Key] = newValue;
					}
					else
					{
						properties[propertyNameToValue.Key] = property.Update(newValue);
					}
				}
			}

			Properties = new ReadOnlyDictionary<string, Property.Node>(properties);
		}
	}
}