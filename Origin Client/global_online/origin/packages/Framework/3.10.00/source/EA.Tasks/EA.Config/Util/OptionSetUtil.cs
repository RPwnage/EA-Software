using NAnt.Core;
using EA.FrameworkTasks;

using System;
using System.Collections.Generic;
using System.Text;
using System.Xml;

namespace EA.Eaconfig
{
    public class OptionSetUtil
    {
        public static bool OptionSetExists(Project project, string name)
        {
            if (String.IsNullOrEmpty(name))
            {
                return false;
            }
            return project.NamedOptionSets.Contains(name);
        }

        public static OptionSet GetOptionSet(Project project, string name)
        {
            if (String.IsNullOrEmpty(name))
            {
                return null;
            }

            OptionSet optionSet = project.NamedOptionSets[name];
            return optionSet;
        }

        public static OptionSet GetOptionSetOrFail(Project project, string name)
        {
            if (String.IsNullOrEmpty(name))
            {
                Error.Throw(project, "GetOptionSetOrFail", "Optionset name is null or empty.");
            }

            OptionSet optionSet = project.NamedOptionSets[name];
            if (optionSet == null)
            {
                Error.Throw(project, "GetOptionSetOrFail", "Optionset '{0}' does not exist.", name);
            }
            return optionSet;
        }

        public static void CreateOptionSetInProjectIfMissing(Project project, string name)
        {
            if (String.IsNullOrEmpty(name))
            {
                Error.Throw(project, "", "Can't create OptionSet set with empty name");
            }
            if (project.NamedOptionSets[name] == null)
            {
                project.NamedOptionSets[name] = new OptionSet(); 
            }
        }

        public static bool IsOptionContainValue(OptionSet optionset, string optionName, string optionValue)
        {
            bool ret = false;
            if (optionset != null && optionset.Options.Contains(optionName))
            {
                if (-1 != optionset.Options[optionName].IndexOf(optionValue))
                {
                    ret = true;
                }
            }
            return ret;
        }

        public static string GetOption(OptionSet set,  string optionName)
        {
            if (set == null || optionName == null)
            {
                return String.Empty;
            }

            return set.Options[optionName];
        }

        public static string GetOptionOrFail(Project project, OptionSet set, string optionName)
        {
            if (set == null)
            {
                Error.Throw(project, "GetOptionOrFail", "Optionset object is null.");
            }
            if (String.IsNullOrEmpty(optionName))
            {
                Error.Throw(project, "GetOptionOrFail", "Option name is null or empty.");
            }

            string value = set.Options[optionName];

            if (String.IsNullOrEmpty(value))
            {
                Error.Throw(project, "GetOptionOrFail", "Option '{0}' does not exist or has an empty value.", optionName);
            }

            return value;
        }


        public static string GetOptionOrDefault(OptionSet set, string optionName, string defaultVal)
        {            
            if (set == null || optionName == null)
            {
                return defaultVal;
            }

            string val =  set.Options[optionName];
            if (val == null)
            {
                val = defaultVal;
            }
            return val;
        }


        public static string GetOption(Project project, string optionsetName, string optionName)
        {
            return GetOptionSetOrFail(project, optionsetName).Options[optionName];
        }

        public static bool GetBooleanOption(OptionSet set, string optionName)
        {
            string option = set.Options[optionName];
            if (!String.IsNullOrEmpty(option) &&
                (option.Equals("true", StringComparison.OrdinalIgnoreCase) ||
                option.Equals("on", StringComparison.OrdinalIgnoreCase)))
            {
                return true;
            }
            return false;
        }


        public static bool GetBooleanOption(Project project, string optionsetName, string optionName)
        {
            string option = GetOptionSetOrFail(project, optionsetName).Options[optionName];
            if (!String.IsNullOrEmpty(option) &&
                (option.Equals("true", StringComparison.OrdinalIgnoreCase) ||
                option.Equals("on", StringComparison.OrdinalIgnoreCase)))
            {
                return true;
            }
            return false;
        }


        public static bool AppendOption(OptionSet optionset, string optionName, string optionValue)
        {
            bool ret = false;
            if (optionset.Options.Contains(optionName))
            {
                optionset.Options[optionName] += "\n" + optionValue;
                ret = true;
            }
            else
            {
                optionset.Options[optionName] = optionValue;
            }
            return ret;
        }

        public static void ReplaceOption(OptionSet optionSet, string optionName, string oldValue, string newValue)
        {
            if(newValue == null) 
                newValue = string.Empty;

            if (oldValue == newValue)
                return;

            string option = optionSet.Options[optionName];
            if (!String.IsNullOrEmpty(option))
            {
                if (!String.IsNullOrEmpty(newValue))
                {
                    if (-1 == option.IndexOf(newValue))
                    {
                        if (!String.IsNullOrEmpty(oldValue) && (-1 != option.IndexOf(oldValue)))
                        {
                            option = option.Replace(oldValue, newValue);
                        }
                        else if (!String.IsNullOrEmpty(newValue))
                        {
                            option += "\n" + newValue;
                        }
                    }
                    else if (!String.IsNullOrEmpty(oldValue))
                    {
                        if (oldValue != newValue)
                        {
                            option = option.Replace(oldValue, String.Empty);
                        }
                    }
                }
                else
                {
                    if (!String.IsNullOrEmpty(oldValue))
                    {
                        option = option.Replace(oldValue, String.Empty);
                    }
                }

                optionSet.Options[optionName] = option;
            }
            else if (!String.IsNullOrEmpty(newValue))
            {
                optionSet.Options[optionName] = newValue;
            }
        }

        public static bool ReplaceOnlyOption(OptionSet optionSet, string optionName, string oldValue, string newValue)
        {
            bool res = false;

            if (newValue == null)
                newValue = string.Empty;

            if (oldValue == newValue)
                return true;

            if (String.IsNullOrEmpty(oldValue))
                return false;

            string option = optionSet.Options[optionName];
            if (!String.IsNullOrEmpty(option))
            {
                res = -1 != option.IndexOf(oldValue);
                if (res)
                {
                    option = option.Replace(oldValue, newValue);
                    optionSet.Options[optionName] = option;
                }
            }
            return res;
        }
    }

}


