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
using System.IO;

namespace NAnt.Core.Logging
{
	public class FileListener : ILogListener
	{
        public readonly string LogFilePath;

		private readonly StreamWriter m_writer = null;

		private bool m_closed = false;

		public FileListener(string logFileName, bool append = false)
		{
			try
			{
				LogFilePath = Path.GetFullPath(logFileName);

				// allow other programs to write to the log at the same time as 
				m_writer = new StreamWriter(File.Open(LogFilePath, append ? FileMode.Append : FileMode.Create, FileAccess.Write, FileShare.Write))
				{
					AutoFlush = true
				};
			}
			catch (Exception e)
			{
				throw new BuildException(String.Format("Failed to create buildlog file: '{0}'.", logFileName), innerException: e);
			}
		}

		public virtual void WriteLine(string arg)
		{
			lock (m_writer)
			{
				if (!m_closed)
				{
					m_writer.WriteLine(arg);
				}
			}
		}

		public virtual void Write(string arg)
		{
			lock (m_writer)
			{
				if (!m_closed)
				{
					m_writer.Write(arg);
				}
			}
		}

		public void Close()
		{
			m_closed = true;
			m_writer.Close();
		}
	}
}
