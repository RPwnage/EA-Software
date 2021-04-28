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
using System.Management;
using System.Security.Principal;

namespace NAnt.Perforce.Processes
{
	internal class WindowsProcessWatcher : ProcessWatcher, IDisposable
	{
		private ManagementEventWatcher ProcessStartTracer;
		private bool Disposed;

		internal WindowsProcessWatcher()
		{
			ManagementScope scope = new ManagementScope("root\\CIMV2");
			scope.Options.EnablePrivileges = true;

			WqlEventQuery query = new WqlEventQuery();
			query.QueryString = "SELECT * FROM Win32_ProcessTrace";

			ProcessStartTracer = new ManagementEventWatcher(scope, query);
			ProcessStartTracer.EventArrived += new EventArrivedEventHandler(OnEvents);
			ProcessStartTracer.Start();

			// runtime callable wrapper can be killed before we try to explicitly stop watcher
			// supressing finalize lets it terminate via either means
			GC.SuppressFinalize(ProcessStartTracer);
		}

		public void Dispose()
		{
			Dispose(true);
			GC.SuppressFinalize(this);
		}

		protected virtual void Dispose(bool disposing)
		{
			if (Disposed)
			{
				return;
			}

			if (disposing)
			{
				ProcessStartTracer.Stop();
				ProcessStartTracer.Dispose();
			}

			Disposed = true;
		}
		
		internal static bool CanRun()
		{
			return new WindowsPrincipal(WindowsIdentity.GetCurrent()).IsInRole(WindowsBuiltInRole.Administrator);
		}

		private static void OnEvents(object sender, EventArrivedEventArgs e)
		{
			int processID = Int32.Parse(e.NewEvent["ProcessID"].ToString());
			int parentProcessID = Int32.Parse(e.NewEvent["ParentProcessID"].ToString());

			switch (e.NewEvent.ClassPath.ClassName)
			{
				case "Win32_ProcessStartTrace":
					ProcessWatcher.AddParent(processID, parentProcessID);
					break;
			}
		}
	}
}
