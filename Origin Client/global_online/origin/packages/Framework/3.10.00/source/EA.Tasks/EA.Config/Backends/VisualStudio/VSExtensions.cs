using System;
using System.Collections.Generic;

using NAnt.Core;

namespace EA.Eaconfig.Backends.VisualStudio
{
    internal static class VSExtensions
    {
        internal static void AddNonEmpty(this IDictionary<string, string> dictionary, string name, string value)
        {
            if (dictionary != null)
            {
                value = value.TrimWhiteSpace();
                if (!String.IsNullOrEmpty(value))
                {
                    dictionary.Add(name, value);
                }
            }
        }

        internal static void AddIfTrue(this IDictionary<string, string> dictionary, string name, bool value)
        {
            if (dictionary != null)
            {
                if(value)
                {
                    dictionary.Add(name, value.ToString().ToLowerInvariant());
                }
            }
        }
    }
}
