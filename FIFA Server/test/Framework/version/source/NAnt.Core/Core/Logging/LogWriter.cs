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

namespace NAnt.Core.Logging
{
	internal sealed class LogWriter : ILog
	{
		private readonly ILogListener[] m_listeners;
		private readonly Log m_log;
		private readonly string m_prefix;

		internal LogWriter(Log log, ILogListener[] listeners, string prefix = "")
		{
			m_log = log;
			m_listeners = listeners;
			m_prefix = prefix;
		}

		public void WriteLine()
		{
			foreach (ILogListener listener in m_listeners)
			{
				listener.WriteLine(String.Empty);
			}
		}

		public void WriteLine(string value)
		{
			foreach (string line in value.Split(new string[] { Environment.NewLine }, StringSplitOptions.None))
			{
				string text =
					m_log.Id +
					m_log.Padding +
					m_prefix +
					line;

				foreach (ILogListener listener in m_listeners)
				{
					listener.WriteLine(text);
				}
			}
		}

		public void WriteLine(string format, params object[] args)
		{
			WriteLine(String.Format(format, args));
		}

		public void Write(string value)
		{
			string[] lines = value.Split(new string[] { Environment.NewLine }, StringSplitOptions.None);
			foreach (string line in lines.Take(lines.Length - 1))
			{
				string text =
					m_log.Id +
					m_log.Padding +
					m_prefix +
					line;

				foreach (ILogListener listener in m_listeners)
				{
					listener.WriteLine(text);
				}
			}
			foreach (ILogListener listener in m_listeners)
			{
				listener.Write(lines.Last());
			}
		}

		public void Write(string format, params object[] args)
		{
			Write(String.Format(format, args));
		}
	}
}
