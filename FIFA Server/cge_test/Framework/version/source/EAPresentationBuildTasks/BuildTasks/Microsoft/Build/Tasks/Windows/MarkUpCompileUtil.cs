using System;
using System.IO;
using System.Linq;
using System.Security.Cryptography;
using System.Text;

namespace Microsoft.Build.Tasks.Windows
{
    internal static class MarkupCompileUtil
    {
        public static string GetSafeRelativePath(string file)
        {
            return Path.Combine
            (
                String.Concat(MD5.Create().ComputeHash(Encoding.Default.GetBytes(file)).Select(b => b.ToString("X2"))),
                Path.GetFileName(file)
            );
        }
    }
}
