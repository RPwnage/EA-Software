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
using System.Diagnostics;
using System.Runtime.InteropServices;
using System.ComponentModel;

namespace NAnt.Perforce.Processes
{
	internal class WinProcess : ChildProcess
	{
		[StructLayout(LayoutKind.Sequential)]
		internal struct ProcessBasicInformation
		{
			// These members must match PROCESS_BASIC_INFORMATION
			internal IntPtr Reserved1;
			internal IntPtr PebBaseAddress;
			internal IntPtr Reserved2_0;
			internal IntPtr Reserved2_1;
			internal IntPtr UniqueProcessId;
			internal IntPtr InheritedFromUniqueProcessId;
		}

		[DllImport("ntdll.dll")]
		private static extern int NtQueryInformationProcess(IntPtr processHandle, int processInformationClass, ref ProcessBasicInformation processInformation, int processInformationLength, out int returnLength);

		internal WinProcess(Process process)
			: base(process)
		{
		}

		protected override ChildProcess PlatformGetParent()
		{
			ProcessBasicInformation pbi = new ProcessBasicInformation();
			int returnLength = 0;
			try
			{
				int status = NtQueryInformationProcess(_Process.Handle, 0, ref pbi, Marshal.SizeOf(pbi), out returnLength);
				if (status != 0)
				{
					throw new Win32Exception(status);
				}
				else
				{
					Process parentProcess = null;
					try
					{
						parentProcess = Process.GetProcessById(pbi.InheritedFromUniqueProcessId.ToInt32());
						return ChildProcess.CreatePlatformChildProcess(parentProcess);
					}
					catch (ArgumentException)
					{
						// we can't retrieve this process id
					}
					return null;
				}
			}
			catch (Win32Exception)
			{
				return null;// secure process
			}
			catch (InvalidOperationException)
			{
				return null;// exited process
			}
		}
	}
}