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
using System.Linq;

namespace EA.Eaconfig
{
	public class BuildType
	{
		public enum Types
		{
			Generic = 0,

			Dynamic = 1 << 1,
			Library = 1 << 2,
			Program = 1 << 3,

			CSharp = 1 << 4,
			Managed = 1 << 5,
			MakeStyle = 1 << 7,

			Java = 1 << 9,
			Apk = 1 << 10,
			Aar = 1 << 11,
		}

		public readonly String Name;
		public readonly String BaseName;

		private readonly Types m_type;

		public bool IsCLR { get { return IsCSharp || IsManaged; } }
		public bool IsCSharp { get { return (m_type & Types.CSharp) == Types.CSharp; } }
		public bool IsDynamic { get { return (m_type & Types.Dynamic) == Types.Dynamic; } }
		public bool IsJava { get { return (m_type & Types.Java) == Types.Java; } }
		public bool IsLibrary { get { return (m_type & Types.Library) == Types.Library; } }
		public bool IsMakeStyle { get { return (m_type & Types.MakeStyle) == Types.MakeStyle; } }
		public bool IsManaged { get { return (m_type & Types.Managed) == Types.Managed; } }
		public bool IsProgram { get { return (m_type & Types.Program) == Types.Program; } }
		public bool IsApk { get { return (m_type & Types.Apk) == Types.Apk; } }
		public bool IsAar { get { return (m_type & Types.Aar) == Types.Aar; } }

		private static string[] s_dynamicTypes = new string[] { "DynamicLibrary", "ManagedCppAssembly" };
		private static string[] s_libraryTypes = new string[] { "Library", "StdLibrary" };
		private static string[] s_programTypes = new string[] { "Program", "StdProgram", "WindowsProgram", "ManagedCppProgram", "ManagedCppWindowsProgram" };
		private static string[] s_javaTypes = new string[] { "AndroidApk", "JavaLibrary", "AndroidAar" };


		public BuildType(string name, string basename, bool isMakestyle=false)
		{
			if (name == "Program")
			{
				name = "StdProgram";
			}
			else if (name == "Library")
			{
				name = "StdLibrary";
			}

			Name = name;
			BaseName = basename;

			if (BaseName.Contains("CSharp"))
			{
				m_type = Types.CSharp;
			}
			else if (BaseName.Contains("Managed"))
			{
				m_type = Types.Managed;
			}
			else if (s_javaTypes.Contains(basename))
			{
				m_type = Types.Java;
			}
			else
			{
				m_type = Types.Generic;
			}

			if (BaseName == "AndroidApk")
			{
				m_type |= Types.Apk;
			}

			if (BaseName == "AndroidAar")
			{
				m_type |= Types.Aar;
			}

			if (isMakestyle)
			{
				m_type |= Types.MakeStyle;
			}

			if (s_dynamicTypes.Contains(BaseName))
			{
				m_type |= Types.Dynamic;
			}
			else if (s_libraryTypes.Contains(BaseName))
			{
				m_type |= Types.Library;
			}
			else if (s_programTypes.Contains(BaseName))
			{
				m_type |= Types.Program;
			}
		}
	}
}


