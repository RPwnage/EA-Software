// Originally based on NAnt - A .NET build tool
// Copyright (C) 2003-2018 Electronic Arts Inc.
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
// Electronic Arts (Frostbite.Team.CM@ea.com)

using System;
using System.IO;
using System.Security.Cryptography;
using System.Text;

namespace NAnt.Core.Util
{
	public class AesEncryptionUtil
	{
		// This is still far from perfect. Ideally this should use a separate
		// keyfile, interactive password, or some sort of registered + 
		// revokable certificate.
		//const string PASSWORD = "1dF7xUoCDNxIUwwabTOEu20+NdQJI3/uQvn8E9p0MCo=";
		const int SALT_LENGTH = 16;
		const int IV_LENGTH = 16;
		const int HASH_LENGTH = 32;

        public static string GetRandomKey(int size)
        {
            char[] chars =
                "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890".ToCharArray();
            byte[] data = new byte[size];
            using (RNGCryptoServiceProvider crypto = new RNGCryptoServiceProvider())
            {
                crypto.GetBytes(data);
            }
            StringBuilder result = new StringBuilder(size);
            foreach (byte b in data)
            {
                result.Append(chars[b % (chars.Length)]);
            }
            return result.ToString();
        }

        // Encrypted buffer:
        // [16 byte salt][32 byte IV][cipher (hash + text)]
        public static byte[] EncryptBuffer(byte[] plainText, string encryptKey)
		{
			byte[] plainTextHash = null;
			byte[] salt = new byte[SALT_LENGTH];
			byte[] iv = new byte[IV_LENGTH];
			byte[] key = null;

			// Salt and IV are initialized using the system secure RNG
			// to prevent the creation of duplicate ciphertext when
			// called with same inputs.
			using (RNGCryptoServiceProvider rng = new RNGCryptoServiceProvider())
			{
				rng.GetBytes(salt);
				rng.GetBytes(iv);
			}

			using (Rfc2898DeriveBytes generator = new Rfc2898DeriveBytes(encryptKey, salt))
			{
				key = generator.GetBytes(32);
			}

			using (SHA256 hashContext = SHA256.Create())
			{
				plainTextHash = hashContext.ComputeHash(plainText);
			}

			using (Aes aes = Aes.Create())
			{
				aes.Key = key;
				aes.IV = iv;

				using (MemoryStream memoryStream = new MemoryStream())
				{
					memoryStream.Write(salt, 0, salt.Length);
					memoryStream.Write(iv, 0, iv.Length);

					// Hash of plaintext is stored as a prefix to the plaintext
					// but also encrypted.
					using (CryptoStream cryptoStream = new CryptoStream(memoryStream, aes.CreateEncryptor(), CryptoStreamMode.Write))
					{
						cryptoStream.Write(plainTextHash, 0, plainTextHash.Length);
						cryptoStream.Write(plainText, 0, plainText.Length);
					}

					return memoryStream.ToArray();
				}
			}
		}

		public static byte[] DecryptBuffer(byte[] cipherText, string encryptKey)
		{
			byte[] salt = new byte[SALT_LENGTH];
			byte[] iv = new byte[IV_LENGTH];
			byte[] key = null;
			int offset = 0;

			Array.Copy(cipherText, offset, salt, 0, SALT_LENGTH);	offset += SALT_LENGTH;
			Array.Copy(cipherText, offset, iv, 0, IV_LENGTH);		offset += IV_LENGTH;

			using (Rfc2898DeriveBytes generator = new Rfc2898DeriveBytes(encryptKey, salt))
			{
				key = generator.GetBytes(32);
			}

			using (Aes aes = Aes.Create())
			{
				aes.Key = key;
				aes.IV = iv;

				using (MemoryStream memoryStream = new MemoryStream())
				{
					using (CryptoStream cryptoStream = new CryptoStream(memoryStream, aes.CreateDecryptor(), CryptoStreamMode.Write))
					{
						cryptoStream.Write(cipherText, offset, cipherText.Length - offset);
					}

					byte[] plainTextAndHash = memoryStream.ToArray();
					byte[] generatedHash = new byte[HASH_LENGTH];
					byte[] calculatedHash = new byte[HASH_LENGTH];

					Array.Copy(plainTextAndHash, 0, generatedHash, 0, HASH_LENGTH);

					byte[] plainText = new byte[plainTextAndHash.Length - HASH_LENGTH];
					Array.Copy(plainTextAndHash, HASH_LENGTH, plainText, 0, plainTextAndHash.Length - HASH_LENGTH);

					using (SHA256 hashContext = SHA256.Create())
					{
						calculatedHash = hashContext.ComputeHash(plainText);
					}

					// Comparison of hashes is performed in reverse so that the
					// loop executes in constant time, regardless of inputs.
					bool hashesEqual = true;
					for (int i = HASH_LENGTH - 1; i >= 0; --i)
					{
						hashesEqual = hashesEqual && (generatedHash[i] == calculatedHash[i]);
					}

					return hashesEqual ? plainText : null;
				}
			}
		}
	}
}
