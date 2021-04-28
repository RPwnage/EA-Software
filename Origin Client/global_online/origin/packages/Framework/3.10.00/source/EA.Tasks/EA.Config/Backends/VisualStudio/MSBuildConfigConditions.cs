using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

using NAnt.Core;
using NAnt.Core.Util;
using NAnt.Core.Attributes;
using NAnt.Core.Functions;
using NAnt.Core.Logging;
using NAnt.Core.Reflection;
using EA.FrameworkTasks.Model;

using EA.Eaconfig;
using EA.Eaconfig.Core;
using EA.Eaconfig.Build;
using EA.Eaconfig.Modules;


namespace EA.Eaconfig.Backends.VisualStudio
{
    internal class MSBuildConfigConditions
    {
        internal class Condition
        {
            internal enum Type { None, Positive, Negative };

            internal Condition(Type type, IEnumerable<Configuration> configs = null)
            {
                ConditionType = type;
                Configurations = configs;

            }

            internal readonly Type ConditionType;
            internal readonly IEnumerable<Configuration> Configurations;

        }

        internal static Condition GetCondition(IEnumerable<Configuration> configs, IEnumerable<Configuration> allconfigs)
        {
            if (configs.Count() == allconfigs.Count())
            {
                return new Condition(Condition.Type.None);
            }

            if (configs.Count() > allconfigs.Count() / 2)
            {
                return new Condition(Condition.Type.Negative, allconfigs.Except(configs));
            }
         
            return new Condition(Condition.Type.Positive, configs);
        }

        internal static string FormatCondition(string macro, Condition condition, Func<Configuration, string> configformatter)
        {
            if (condition.ConditionType == Condition.Type.None)
            {
                return string.Empty;
            }

            StringBuilder sb = new StringBuilder();

            if (condition.ConditionType == Condition.Type.Negative)
            {
                sb.Append("!(");
            }

            foreach (var config in condition.Configurations)
            {
                if (sb.Length > 3)
                {
                    sb.Append("Or");
                }
                sb.AppendFormat(" '{0}' == '{1}' ", macro, configformatter(config));
            }

            if (condition.ConditionType == Condition.Type.Negative)
            {
                sb.Append(")");
            }

            return sb.ToString();


        }
    }
}
