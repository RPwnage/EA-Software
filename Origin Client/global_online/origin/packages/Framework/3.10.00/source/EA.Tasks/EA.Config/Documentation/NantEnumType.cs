using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace EA.Eaconfig
{
    public class NantEnumType
    {
        public NantEnumType(Type type)
        {
            Key = type.FullName.NantSafeSchemaKey();
            Names = type.GetEnumNames().Where(n => !SKIP_ENUM_NAMES.Contains(n)).ToList();
        }

        public NantEnumType(string key, IEnumerable<string> names)
        {
            Key = key.NantSafeSchemaKey();
            Names = new List<string>(names);
        }

        public readonly string Key;
        public readonly IEnumerable<string> Names;
        public readonly static HashSet<string> SKIP_ENUM_NAMES = new HashSet<string>(StringComparer.OrdinalIgnoreCase)
            {
                "None"
            };
    };
}
