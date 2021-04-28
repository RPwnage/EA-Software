// Originally based on NAnt - A .NET build tool
// Copyright (C) 2003-2015 Electronic Arts Inc.
// Copyright (C) 2001-2003 Gerry Shaw
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
// 
// As a special exception, the copyright holders of this software give you 
// permission to link the assemblies with independent modules to produce 
// new assemblies, regardless of the license terms of these independent 
// modules, and to copy and distribute the resulting assemblies under terms 
// of your choice, provided that you also meet, for each linked independent 
// module, the terms and conditions of the license of that module. An 
// independent module is a module which is not derived from or based 
// on these assemblies. If you modify this software, you may extend 
// this exception to your version of the software, but you are not 
// obligated to do so. If you do not wish to do so, delete this exception 
// statement from your version. 
// 
// 
// 
// Gerry Shaw (gerry_shaw@yahoo.com)
// Ian MacLean (ian_maclean@another.com)
// Electronic Arts (Frostbite.Team.CM@ea.com)

using System;
using System.IO;
using System.Text;
using System.Security.Cryptography;

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

			// MD5 is the only algorithm which generates 128bit (16B) hashes, just the size of a GUID
			using(var md5 = new MD5CryptoServiceProvider())
			{
				byte[] hash = md5.ComputeHash(buffer);
				if (md5.HashSize != GUID_BYTE_NUM * 8)
				{
					string msg = String.Format("MD5 hash should have exactly {0} bits ({1} bytes)", GUID_BYTE_NUM, GUID_BYTE_NUM * 8);
					throw new Exception(msg);
				}
				Guid guid = new Guid(hash);
				return guid;
			}
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

		public static string GetFileHashString(string path)
		{
			// Code below ignores line endings to eliminate difference between Windows/Unix but it is 10 times slower.
			// Don't use it for now.
			//var buffer = new StringBuilder();
			//using (StreamReader sr = new StreamReader(path))
			//{
			//    string line;
			//    while ((line = sr.ReadLine()) != null)
			//    {
			//        buffer.Append(line.Trim(' '));
			//    }
			//}
			//using (var md5 = new MD5CryptoServiceProvider())
			//{
			//    var hash = md5.ComputeHash(Encoding.UTF8.GetBytes(buffer.ToString()));
			//    return Convert.ToBase64String(hash).TrimEnd('=');
			//}
			
			using (var fs = new FileStream(path, FileMode.Open, FileAccess.Read, FileShare.ReadWrite))
			{
				using (var md5 = new MD5CryptoServiceProvider())
				{
					var hash = md5.ComputeHash(File.ReadAllBytes(path));
					return Convert.ToBase64String(hash).TrimEnd('=');
				}
			}
		}

		public static string GetHashString(string value)
		{
			using (var md5 = new MD5CryptoServiceProvider())
			{
				var hash = md5.ComputeHash(Encoding.UTF8.GetBytes(value));
				return Convert.ToBase64String(hash).TrimEnd('=');
			}
		}

		public static byte[] GetSha2Hash(string value)
		{
			using (var sha2 = System.Security.Cryptography.SHA256.Create())
			{
				sha2.ComputeHash(Encoding.UTF8.GetBytes(value));

				return sha2.Hash;
			}
		}

		public static string GetSha2HashString(string value)
		{
			byte[] digest = GetSha2Hash(value);
			return BytesToHex(digest, 0, digest.Length);
		}
	}
}
