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
using System.Collections.Generic;
using System.Text;

namespace NAnt.Core.Logging
{
	

	public sealed class BufferedLog : Log, IDisposable
	{
		// records a logging event that we want to replay later
		private struct BufferLogEvent
		{
			internal bool Line;
			internal bool Error;
			internal string Text;
		}

		// a specialized log listener that record events on it's owning
		// BufferedLog so they can be replayed
		private class BufferedLogInternalListener : ILogListener
		{
			private readonly List<BufferLogEvent> m_eventsList;
			private readonly bool m_logError;

			internal BufferedLogInternalListener(List<BufferLogEvent> eventList, bool error)
			{
				m_eventsList = eventList;
				m_logError = error;
			}

			public void Write(string value)
			{
				lock (m_eventsList)
				{
					m_eventsList.Add(new BufferLogEvent() { Line = false, Error = m_logError, Text = value });
				}
			}

			public void WriteLine(string value)
			{
				lock (m_eventsList)
				{
					m_eventsList.Add(new BufferLogEvent() { Line = true, Error = m_logError, Text = value });
				}
			}
		}

		private readonly List<BufferLogEvent> m_eventsList; // list that records all logging events
		private readonly Log m_flushLog; // log that this buffer log will write to once flushed

		public BufferedLog(Log flushLog)
			: this(flushLog, new List<BufferLogEvent>())
		{
		}

		private BufferedLog(Log flushLog, List<BufferLogEvent> events)
			: base
			(
				flushLog,
				listeners: new ILogListener[] { new BufferedLogInternalListener(events, error: false) },
				errorListeners: new ILogListener[] { new BufferedLogInternalListener(events, error: true) }
			)
		{
			m_eventsList = events;
			m_flushLog = flushLog;
		}

		public void Flush()
		{
			lock (m_eventsList)
			{
				foreach (BufferLogEvent logEvent in m_eventsList)
				{
					if (logEvent.Error)
					{
						foreach (ILogListener listener in m_flushLog.ErrorListeners)
						{
							if (logEvent.Line)
							{
								listener.WriteLine(logEvent.Text);
							}
							else
							{
								listener.Write(logEvent.Text);
							}
						}
					}
					else
					{
						foreach (ILogListener listener in m_flushLog.Listeners)
						{
							if (logEvent.Line)
							{
								listener.WriteLine(logEvent.Text);
							}
							else
							{
								listener.Write(logEvent.Text);
							}
						}
					}
				}
				m_eventsList.Clear();
			}
		}

		public void Dispose()
		{
			Flush();
		}
	}
}