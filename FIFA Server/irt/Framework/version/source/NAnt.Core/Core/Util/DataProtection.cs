using System;
using System.Security.Cryptography;

namespace NAnt.Core.Util
{
    public class DataProtection
    {
        // Create byte array for additional entropy when using Protect method.
        static byte[] s_aditionalEntropy = { 56, 12, 156, 140, 35 };

        public static byte[] EncryptBuffer(byte[] data)
        {
            try
            {
                // Encrypt the data using DataProtectionScope.CurrentUser. The result can be decrypted
                //  only by the same current user.
                return ProtectedData.Protect(data, s_aditionalEntropy, DataProtectionScope.CurrentUser);
            }
            catch (CryptographicException e)
            {
                Console.WriteLine("Data was not encrypted. An error occurred.");
                Console.WriteLine(e.ToString());
                return null;
            }
        }

        public static byte[] DecryptBuffer(byte[] data)
        {
            try
            {
                //Decrypt the data using DataProtectionScope.CurrentUser.
                return ProtectedData.Unprotect(data, s_aditionalEntropy, DataProtectionScope.CurrentUser);
            }
            catch (CryptographicException e)
            {
                Console.WriteLine("Data was not decrypted. An error occurred. " +
                    "(Tip: One situation where this can occur is if you are running under the system account " +
                    "but the credstore was generated under a different user.)");
                Console.WriteLine(e.ToString());
                return null;
            }
        }
    }
}
