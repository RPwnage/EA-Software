using System.Diagnostics;

namespace NAnt.LangTools
{
	public sealed partial class Model
	{
		public sealed class ConditionalEntry
		{
			public readonly string Value;
			public readonly ConditionValueSet Conditions; // TODO: useful internal representation but maybe limited for public API? can we do better?

			internal ConditionalEntry(string value, ConditionValueSet conditions)
			{
				Debug.Assert(conditions != ConditionValueSet.False);

				Value = value;
				Conditions = conditions;
			}

			public override bool Equals(object obj)
			{
				return obj is ConditionalEntry entry &&
					   Value == entry.Value &&
					   Conditions.Equals(entry.Conditions);
			}

			public override int GetHashCode()
			{
				int hashCode = -564868399;
				hashCode = hashCode * -1521134295 + Value.GetHashCode();
				hashCode = hashCode * -1521134295 + Conditions.GetHashCode();
				return hashCode;
			}

			public override string ToString()
			{
				string conditionStr = "";
				if (Conditions != ConditionValueSet.True)
				{
					conditionStr = $" ({Conditions.ToString()})";
				}
				return Value + conditionStr;
			}
		}
	}
}