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

namespace NAnt.Perforce.Processes
{
	internal class OSXProcess : UnixProcess
	{
		internal OSXProcess(Process process)
			: base(process)
		{
		}

		internal static bool TryGetProcessParentId(int PID, out int PPID)
		{
			PPID = default(int);
			try
			{
				
				Process p = new Process();
				p.StartInfo.FileName = "ps";
				p.StartInfo.Arguments = String.Format("-o pid,ppid -p {0}", PID);
				p.StartInfo.UseShellExecute = false;
				p.StartInfo.RedirectStandardOutput = true;
				p.StartInfo.CreateNoWindow = true;
				p.Start();
				if (p.WaitForExit(1000))
				{
					string result = p.StandardOutput.ReadToEnd();
					if (0 == p.ExitCode)
					{
						string[] fragments = result.Split(new char[] { '\n', ' ' }, StringSplitOptions.RemoveEmptyEntries);
						if (fragments.Length > 3)
						{
							PPID = Int32.Parse(fragments[3]);
							return true;
						}
					}
				}
			}
			catch (Exception)
			{
			} 
			return false;
		}

		protected override ChildProcess PlatformGetParent()
		{
			int PPID;
			if (TryGetProcessParentId(PID, out PPID))
			{
				return ChildProcess.GetProcessById(PPID);
			}
			return null;
		}
	}
}