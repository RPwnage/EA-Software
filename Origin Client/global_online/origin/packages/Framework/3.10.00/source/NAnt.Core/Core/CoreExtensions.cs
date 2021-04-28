using System;
using System.Collections;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;
using System.Text.RegularExpressions;

using NAnt.Core.Util;

namespace NAnt.Core
{
    public static class CoreExtensions
    {
        public static bool IsNullOrEmpty<T>(this IEnumerable<T> array)
        {
            return (array == null || array.Count() == 0);
        }
    }
}