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
using System.Runtime.InteropServices;

namespace NAnt.Console
{
	/// <remarks>
	/// This class uses the .Net Runtime Interop Services to make a call to the 
	/// win32 API SetConsoleCtrlHandler function. 
	/// 
	/// We need this class so that we can get callbacks when the main console process terminates.
	/// However, if a user decides to kill the process manually (ie from the task manager) we
	/// will not get an event and the process will be abruptly terminated and results undefined.
	/// </remarks>
	/// <summary>
	/// Class to catch console control events (ie CTRL-C) in C#. 
	/// Calls SetConsoleCtrlHandler() in Win32 API
	/// </summary>
	/// <example>
	///		public static void MyHandler(ConsoleCtrl.ConsoleEvent consoleEvent) { ... } 
	///
	///		ConsoleCtrl cc = new ConsoleCtrl();
	///		cc.ControlEvent += new ConsoleCtrl.ControlEventHandler(MyHandler);
	///</example>
	internal sealed class ConsoleCtrl : IDisposable
	{
		/// <summary>The event that occurred. From wincom.h</summary>
		internal enum ConsoleEvent
		{
			CTRL_C = 0,
			CTRL_BREAK = 1,
			CTRL_CLOSE = 2,
			CTRL_LOGOFF = 5,
			CTRL_SHUTDOWN = 6
		}

		/// <summary>Handler to be called when a console event occurs.</summary>
		internal delegate void ControlEventHandler(ConsoleEvent consoleEvent);

		/// <summary>Event fired when a console event occurs</summary>
		internal event ControlEventHandler ControlEvent;

		private ControlEventHandler m_eventHnadler;

		/// <summary>Create a new instance.</summary>
		public ConsoleCtrl()
		{
			// save this to a private var so the GC doesn't collect it...
			m_eventHnadler = new ControlEventHandler(Handler);
			SetConsoleCtrlHandler(m_eventHnadler, true); // adds
		}

		~ConsoleCtrl()
		{
			Dispose(false);
		}

		public void Dispose()
		{
			Dispose(true);
		}

		private void Dispose(bool disposing)
		{
			if (disposing)
			{
				GC.SuppressFinalize(this);
			}
			if (m_eventHnadler != null)
			{
				SetConsoleCtrlHandler(m_eventHnadler, false); // removes
				m_eventHnadler = null;
			}
		}

		private void Handler(ConsoleEvent consoleEvent)
		{
			ControlEvent(consoleEvent);
		}

		[DllImport("kernel32.dll")]
		private static extern bool SetConsoleCtrlHandler(ControlEventHandler e, bool add);
	}
}