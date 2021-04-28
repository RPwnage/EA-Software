using NAnt.Core;

using System;
using System.Collections.Generic;
using System.Text;


namespace EA.Eaconfig
{
    public class PropertyUtil
    {

        public static bool PropertyExists(Project project, string name)
        {
            return project.Properties.Contains(name);
        }

        public static string SetPropertyIfMissing(Project project, string name, string defaultValue)
        {
            string val = project.Properties[name];

            if (val == null)
            {
                project.Properties[name] = val = defaultValue;
            }
            return val;
        }

        public static bool GetBooleanProperty(Project project, string name)
        {
            bool ret = false;
            string val = project.Properties[name];
            if (!String.IsNullOrEmpty(val))
            {
                if (val.Trim().Equals("true", StringComparison.OrdinalIgnoreCase))
                {
                    ret = true;
                }
            }
            return ret;
        }

        public static bool GetBooleanPropertyOrDefault(Project project, string name, bool defaultVal)
        {
            bool ret = defaultVal;
            string val = project.Properties[name];
            if (!String.IsNullOrEmpty(val))
            {
                if (val.Trim().Equals("true", StringComparison.OrdinalIgnoreCase))
                {
                    ret = true;
                }
                else if (val.Trim().Equals("false", StringComparison.OrdinalIgnoreCase))
                {
                    ret = false;
                }
            }
            return ret;
        }


        public static string GetPropertyOrDefault(Project project, string name, string defaultValue)
        {
            string val = project.Properties[name];

            if (val == null)
            {
                val = defaultValue;
            }
            return val;
        }

        public static bool IsPropertyEqual(Project project, string name, string value)
        {
            bool ret = false;
            string val = project.Properties[name];
            if (!String.IsNullOrEmpty(val))
            {
                if (val.Trim().Equals(value, StringComparison.Ordinal))
                {
                    ret = true;
                }
            }
            else
            {
                if (string.IsNullOrEmpty(value))
                {
                    ret = true;
                }
            }
            return ret;
        }

        public static string AddNonDuplicatePathToNormalizedPathListProp(string currentNormalizedPathProperty, string newInfo)
        {
            // Convert currentNormalizedPathProperty to StringBuilder and StringCollection
            StringBuilder newProperty = new StringBuilder();
            System.Collections.Specialized.StringCollection newPathList = new System.Collections.Specialized.StringCollection();
            if (!String.IsNullOrEmpty(currentNormalizedPathProperty))
            {
                newProperty = new StringBuilder(currentNormalizedPathProperty);
                newPathList.AddRange(currentNormalizedPathProperty.Split(new char[] { ';', '\n' }));
            }
            // Now Add the new info to the property
            if (!String.IsNullOrEmpty(newInfo))
            {
                foreach (string line in newInfo.Split(new char[] { ';', '\n' }))
                {
                    string trimmed = line.Trim();
                    if (trimmed.Length > 0)
                    {
                        string normalizedPath = NAnt.Core.Util.PathNormalizer.Normalize(trimmed);
                        bool foundPath = false;
                        // Check and see if this path is already added to the list.
                        // This is a linear search, but we can't re-sort this list in case build
                        // script include paths were setup with expectation of certain paths 
                        // taking precedence over another path.  We just append the list as is.
                        foreach (string pathInCollection in newPathList)
                        {
                            // We need to trim the path here as well because the end of the string
                            // might contain a \r character which also needs to be stripped out.
                            string trimmedPathInCollection = pathInCollection.Trim();
                            if (trimmedPathInCollection == normalizedPath)
                            {
                                foundPath = true;
                            }
                        }
                        if (!foundPath)
                        {
                            newProperty.AppendLine(normalizedPath);
                            newPathList.Add(normalizedPath);
                        }
                    }
                }
            }
            if (newProperty.Length > 0)
            {
                return newProperty.ToString();
            }
            else
            {
                return currentNormalizedPathProperty;
            }            
        }
    }
}


