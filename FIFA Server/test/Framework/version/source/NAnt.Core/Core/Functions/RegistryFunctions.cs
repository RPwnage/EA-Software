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
// Kosta Arvanitis (karvanitis@ea.com)
// Electronic Arts (Frostbite.Team.CM@ea.com)

using System;

using Microsoft.Win32;

using NAnt.Core;
using NAnt.Core.Attributes;
using NAnt.Core.Util;

namespace NAnt.Win32Tasks.Functions
{
	/// <summary>Collection of windows registry manipulation routines.</summary>
	[FunctionClass("Registry Functions")]
	public class RegistryFunctions : FunctionClassBase
	{
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
		/// <include file='Examples/Registry/RegistryGetValue.example' path='example'/>        
		[Function()]
		public static string RegistryGetValue(Project project, RegistryHive hive, string key, string value)
		{
			string val = RegistryUtil.GetRegistryValue(hive, key, value);

			if (null == val)
			{
				string msg = String.Format("Registry path not found.\nkey='{0}'\nvalue='{1}'\nhive='{2}'", key, value, hive.ToString());
				throw new BuildException(msg);
			}

			return val;
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
		/// <include file='Examples/Registry/RegistryGetValueDefault.example' path='example'/>
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
		/// <include file='Examples/Registry/RegistryKeyExists.example' path='example'/>        
		[Function()]
		public static string RegistryKeyExists(Project project, RegistryHive hive, string key)
		{
			using (var registryKey = RegistryKey.OpenBaseKey(hive, RegistryView.Registry32).OpenSubKey(key, false))
			{
				if (registryKey != null)
				{
					return Boolean.TrueString;
				}
			}

			using (var registryKey = RegistryKey.OpenBaseKey(hive, RegistryView.Registry64).OpenSubKey(key, false))
			{
				if (registryKey != null)
				{
					return Boolean.TrueString;
				}
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
		/// <include file='Examples/Registry/RegistryValueExists.example' path='example'/>        
		[Function()]
		public static string RegistryValueExists(Project project, RegistryHive hive, string key, string value)
		{
			using (var k = RegistryKey.OpenBaseKey(hive, RegistryView.Registry64).OpenSubKey(key))
			{
				if (k != null && k.GetValue(value) != null)
				{
					return Boolean.TrueString;
				}
			}

			using (var k = RegistryKey.OpenBaseKey(hive, RegistryView.Registry32).OpenSubKey(key))
			{
				if (k != null && k.GetValue(value) != null)
				{
					return Boolean.TrueString;
				}
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
		/// <include file='Examples/Registry/RegistryValueExistsDefault.example' path='example'/>
		[Function()]
		public static string RegistryValueExists(Project project, RegistryHive hive, string key)
		{
			return RegistryValueExists(project, hive, key, String.Empty);
		}

		// Although this function is not used by NAnt, it is here because the EABuildHelpers package
		// does use it.
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
			}

			string msg = String.Format("Registry not found for {0}.", hive.ToString());
			throw new BuildException(msg);
		}

	}
}
