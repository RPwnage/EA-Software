// Copyright (C) 2001-2003 Electronic Arts
//
// Kosta Arvanitis (karvanitis@ea.com)
using System;
using System.Xml;

using Microsoft.Win32;

using NAnt.Core;
using NAnt.Core.Attributes;
using NAnt.Core.Logging;
using System.Runtime.InteropServices;

namespace NAnt.Win32Tasks.Functions
{
    class RegistryFunctions64
    {
        private static readonly RegSam[] VIEWS_TO_TEST = new RegSam[] { RegSam.KEY_WOW64_64KEY, RegSam.KEY_WOW64_32KEY };

        [DllImport("advapi32.dll", EntryPoint = "RegOpenKeyExW")]
        static extern long RegOpenKeyEx(HkeyRoot hKey, [MarshalAs(UnmanagedType.LPWStr)]string subkey, uint reserved, RegSam sam, out IntPtr phkResult);

        [DllImport("advapi32.dll", EntryPoint = "RegQueryValueExW")]
        static extern long RegQueryValueEx(IntPtr hKey, [MarshalAs(UnmanagedType.LPWStr)]string valueName, int reserved, ref int lpType, [MarshalAs(UnmanagedType.VBByRefStr)] ref string lpData, ref int lpcbData);

        [DllImport("advapi32.dll", EntryPoint = "RegCloseKey")]
        static extern uint RegCloseKey(IntPtr hKey);

        public static bool SubKey64Exist(RegistryHive hive, string key)
        {
            bool keyExists = false;
            IntPtr resultHkey = IntPtr.Zero;
            try
            {
                foreach (RegSam view in VIEWS_TO_TEST)
                {
                    long lResult = RegOpenKeyEx(GetHiveKey(hive), key, 0, view | RegSam.KEY_QUERY_VALUE, out resultHkey);

                    keyExists = (0 == lResult);

                    if (keyExists)
                    {
                        break;
                    }
                }
            }
            finally
            {
                if (IntPtr.Zero != resultHkey)
                {
                    RegCloseKey(resultHkey);
                }
            }

            return keyExists;
        }

        public static string Get6432Value(RegistryHive hive, string key, string value)
        {
            IntPtr resultHkey = IntPtr.Zero;
            string result = null;
            try
            {
                foreach (RegSam view in VIEWS_TO_TEST)
                {
                    long lResult = RegOpenKeyEx(GetHiveKey(hive), key, 0, view | RegSam.KEY_QUERY_VALUE, out resultHkey);

                    if (0 == lResult)
                    {
                        int type = 0;
                        int len = 0;
                        string pvData = null;
                        lResult = RegQueryValueEx(resultHkey, value, 0, ref type, ref pvData, ref len);

                        if (len > 0)
                        {
                            pvData = new string(' ', len);

                            lResult = RegQueryValueEx(resultHkey, value, 0, ref type, ref pvData, ref len);

                            if (0 == lResult)
                            {
                                byte[] bytes = new byte[len];
                                for (int i = 0; i < len; i++)
                                {
                                    bytes[i] = (byte)pvData[i];
                                }

                                switch (type)
                                {
                                    case (int)RegType.REG_SZ:
                                    case (int)RegType.REG_MULTI_SZ:
                                        {
                                            System.Text.StringBuilder sb = new System.Text.StringBuilder(len / 2);
                                            for (int i = 0; i < len; i += 2)
                                            {
                                                if (0 == bytes[i] && 0 == bytes[i + 1])
                                                    continue;
                                                sb.Append(System.BitConverter.ToChar(bytes, i));
                                            }
                                            result = sb.ToString();
                                            break;
                                        }
                                    case (int)RegType.REG_DWORD:
                                        result = System.BitConverter.ToInt32(bytes, 0).ToString();
                                        break;
                                    case (int)RegType.REG_QWORD:
                                        result = System.BitConverter.ToInt64(bytes, 0).ToString();
                                        break;
                                    default:
                                        result = System.BitConverter.ToString(bytes);
                                        break;
                                }
                            }
                        }
                    }
                    if (result != null)
                    {
                        break;
                    }
                }
            }
            finally
            {
                if (IntPtr.Zero != resultHkey)
                {
                    RegCloseKey(resultHkey);
                }
            }

            return result;
        }

        private static HkeyRoot GetHiveKey(RegistryHive hive)
        {
            switch (hive)
            {
                case RegistryHive.LocalMachine:
                    return HkeyRoot.HKEY_LOCAL_MACHINE;
                case RegistryHive.Users:
                    return HkeyRoot.HKEY_USERS;
                case RegistryHive.CurrentUser:
                    return HkeyRoot.HKEY_CURRENT_USER;
                case RegistryHive.ClassesRoot:
                    return HkeyRoot.HKEY_CLASSES_ROOT;
                default:
                    {
                        string msg = String.Format("Registry not found for {0}.", hive.ToString());
                        throw new BuildException(msg);
                    }
            }
        }


        enum RegSam : uint
        {
            KEY_WOW64_32KEY = 0x00200,
            KEY_WOW64_64KEY = 0x00100,
            KEY_READ = 0x20019,
            KEY_QUERY_VALUE = 0x0001
        }

        enum HkeyRoot : uint
        {
            HKEY_CLASSES_ROOT = 0x80000000,
            HKEY_CURRENT_USER = 0x80000001,
            HKEY_LOCAL_MACHINE = 0x80000002,
            HKEY_USERS = 0x80000003
        }

        enum RegType : int
        {
            REG_SZ = 1,
            REG_MULTI_SZ = 7,
            REG_DWORD = 4,
            REG_QWORD = 11
        }

    }
    /// <summary>
    /// Collection of windows registry manipulation routines. 
    /// </summary>
    [FunctionClass("Registry Functions")]
    public class RegistryFunctions : FunctionClassBase
    {
        /// <summary>
        /// Retrieves registry key object. If object is not found in the regular location it tries to 
        /// to test Wow6432Node subtree (the latter is for 64 bit systems).
        /// </summary>
        /// <param name="hive">
        /// The top-level node in the windows registry. Possible values are: LocalMachine, Users, 
        /// CurrentUser, and ClassesRoot.
        /// </param>
        /// <param name="key">The name of the windows registry key.</param>
        /// <returns>The specified key or null.</returns>

        private static RegistryKey GetRegistryKey6432(RegistryHive hive, string key)
        {
            RegistryKey registryKey = GetHiveKey(hive).OpenSubKey(key, false);
            if (registryKey == null)
            {
                // If we are on Win64 we might need to check this
                int ind = key.IndexOf('\\');
                if (ind > 0 && ind < key.Length - 1)
                {
                    key = key.Substring(0, ind) + "\\Wow6432Node" + key.Substring(ind);
                }
                registryKey = GetHiveKey(hive).OpenSubKey(key, false);
            }

            return registryKey;
        }

        /// <summary>
        /// Get the specified value of the specified key in the windows registry.
        /// </summary>
        /// <param name="project"></param>
        /// <param name="hive">
        /// The top-level node in the windows registry. Possible values are: LocalMachine, Users, 
        /// CurrentUser, and ClassesRoot.
        /// </param>
        /// <param name="key">The name of the windows registry key.</param>
        /// <param name="value">The name of the windows registry key value.</param>
        /// <returns>The specified value of the specified key in the windows registry.</returns>
        /// <include file='Examples\Registry\RegistryGetValue.example' path='example'/>        
        [Function()]
        public static string RegistryGetValue(Project project, RegistryHive hive, string key, string value)
        {
            RegistryKey registryKey = GetHiveKey(hive).OpenSubKey(key, false);
            if (registryKey == null)
            {
                if (false == RegistryFunctions64.SubKey64Exist(hive, key))
                {
                    string msg = String.Format("Registry path not found.\nkey='{0}'\nvalue='{1}'\nhive='{2}'", key, value, hive.ToString());
                    throw new BuildException(msg);
                }
            }

            object registryKeyValue = null != registryKey ? registryKey.GetValue(value) : null;
            if (registryKeyValue == null)
            {
                registryKeyValue = RegistryFunctions64.Get6432Value(hive, key, value);
            }

            if (registryKeyValue == null)
            {

                string msg = String.Format("Registry value not found.\nkey='{0}'\nname={1}\nhive='{2}'", key, value, hive.ToString());
                throw new BuildException(msg);
            }

            return registryKeyValue.ToString();
        }

        /// <summary>
        /// Get the default value of the specified key in the windows registry.
        /// </summary>
        /// <param name="project"></param>
        /// <param name="hive">
        /// The top-level node in the windows registry. Possible values are: LocalMachine, Users, 
        /// CurrentUser, and ClassesRoot.
        /// </param>
        /// <param name="key">The name of the windows registry key.</param>
        /// <returns>The default value of the specified key in the windows registry.</returns>
        /// <include file='Examples\Registry\RegistryGetValueDefault.example' path='example'/>        
        [Function()]
        public static string RegistryGetValue(Project project, RegistryHive hive, string key)
        {
            return RegistryGetValue(project, hive, key, String.Empty);
        }

        /// <summary>
        /// Checks that the specified key exists in the windows registry.
        /// </summary>
        /// <param name="project"></param>
        /// <param name="hive">
        /// The top-level node in the windows registry. Possible values are: LocalMachine, Users, 
        /// CurrentUser, and ClassesRoot.
        /// </param>
        /// <param name="key">The name of the windows registry key.</param>
        /// <returns>True if the specified registry key exists, otherwise false.</returns>
        /// <include file='Examples\Registry\RegistryKeyExists.example' path='example'/>        
        [Function()]
        public static string RegistryKeyExists(Project project, RegistryHive hive, string key)
        {
            RegistryKey registryKey = GetHiveKey(hive).OpenSubKey(key, false);
            if (registryKey != null)
            {
                return Boolean.TrueString;
            }
            if (RegistryFunctions64.SubKey64Exist(hive, key))
            {
                return Boolean.TrueString;
            }

            return Boolean.FalseString;
        }

        /// <summary>
        /// Checks that the specified value of the specified key exists in the windows registry.
        /// </summary>
        /// <param name="project"></param>
        /// <param name="hive">
        /// The top-level node in the windows registry. Possible values are: LocalMachine, Users, 
        /// CurrentUser, and ClassesRoot.
        /// </param>
        /// <param name="key">The name of the windows registry key.</param>
        /// <param name="value">The name of the windows registry key value.</param>
        /// <returns>True if the value exists, otherwise false.</returns>
        /// <include file='Examples\Registry\RegistryValueExists.example' path='example'/>        
        [Function()]
        public static string RegistryValueExists(Project project, RegistryHive hive, string key, string value)
        {
            RegistryKey registryKey = GetHiveKey(hive).OpenSubKey(key, false);
            if (registryKey != null)
            {
                object registryKeyValue = registryKey.GetValue(value);
                if (registryKeyValue != null)
                {
                    return Boolean.TrueString;
                }
            }

            if (null != RegistryFunctions64.Get6432Value(hive, key, value))
            {
                return Boolean.TrueString;
            }

            return Boolean.FalseString;
        }

        /// <summary>
        /// Checks that the default value of the specified key exists in the windows registry.
        /// </summary>
        /// <param name="project"></param>
        /// <param name="hive">
        /// The top-level node in the windows registry. Possible values are: LocalMachine, Users, 
        /// CurrentUser, and ClassesRoot.
        /// </param>
        /// <param name="key">The name of the windows registry key.</param>
        /// <returns>True if the default value exists, otherwise false.</returns>
        /// <include file='Examples\Registry\RegistryValueExistsDefault.example' path='example'/>        
        [Function()]
        public static string RegistryValueExists(Project project, RegistryHive hive, string key)
        {
            return RegistryValueExists(project, hive, key, String.Empty);
        }

        protected static RegistryKey GetHiveKey(RegistryHive hive)
        {
            switch (hive)
            {
                case RegistryHive.LocalMachine:
                    return Registry.LocalMachine;
                case RegistryHive.Users:
                    return Registry.Users;
                case RegistryHive.CurrentUser:
                    return Registry.CurrentUser;
                case RegistryHive.ClassesRoot:
                    return Registry.ClassesRoot;
                default:
                    {
                        string msg = String.Format("Registry not found for {0}.", hive.ToString());
                        throw new BuildException(msg);
                    }
            }
        }

    }
}