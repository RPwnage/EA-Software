#define FRAMEWORK_PARALLEL_TRANSITION
// Copyright (c) 2007 Electronic Arts
using System;
using System.IO;
using System.Reflection;
using System.Diagnostics;
using System.Text;

namespace NAnt.Core.Util
{
    public static class Hash
    {
        /// <summary>
        /// Calculate a hash for the argument and return it as a 16 byte GUID-format string
        /// </summary>
        public static Guid MakeGUIDfromString(string inS)
        {
            const int GUID_BYTE_NUM = 16;

            byte[] buffer = Encoding.UTF8.GetBytes(inS.ToLower());

            // MD5 is the only algo which generates 128bit (16B) hashes, just the size of a GUID
            System.Security.Cryptography.MD5 md5 = new System.Security.Cryptography.MD5CryptoServiceProvider();
            byte[] hash = md5.ComputeHash(buffer);
            if (md5.HashSize != GUID_BYTE_NUM * 8)
            {
                string msg = String.Format("MD5 hash should have exactly {0} bits ({1} bytes)", GUID_BYTE_NUM, GUID_BYTE_NUM * 8);
                throw new Exception(msg);
            }

#if FRAMEWORK_PARALLEL_TRANSITION
            const char DASH_CHAR = '-';
            // Create a GUID-string representation of the hash code bytes
            // A GUID has the format: {00010203-0405-0607-0809-101112131415}
            System.Text.StringBuilder guidstr = new System.Text.StringBuilder(GUID_BYTE_NUM + 4);
            guidstr.Append('{');

            // Append each byte as a 2 character upper case hex string.
            guidstr.Append(BytesToHex(hash, 0, 4));
            guidstr.Append(DASH_CHAR);
            for (int i = 0; i < 3; i++)
            {
                guidstr.Append(BytesToHex(hash, 4 + i * 2, 2));
                guidstr.Append(DASH_CHAR);
            }
            guidstr.Append(BytesToHex(hash, 10, 6));
            guidstr.Append('}');
            Guid guid = new Guid(guidstr.ToString());
#else
            Guid guid = new Guid(hash);
#endif
            return guid;

        }


        /// <summary>
        /// Convert the bytes in a buffer to a hex representation
        /// </summary>
        public static string BytesToHex(byte[] buffer, int start, int len)
        {
            System.Text.StringBuilder sb = new System.Text.StringBuilder(len);
            for (int i = start; i < start + len; i++)
            {
                sb.AppendFormat("{0:X2}", buffer[i]);
            }
            return sb.ToString();
        }
    }
}
