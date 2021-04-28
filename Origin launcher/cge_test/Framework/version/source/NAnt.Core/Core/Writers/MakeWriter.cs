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

using System.Text;
using System.IO;

namespace NAnt.Core.Writers
{
	public class MakeWriter : CachedWriter, IMakeWriter
	{
		public virtual Encoding Encoding
		{
			get { return m_writer.Encoding; }
		}

		private TextWriter m_writer;

		public MakeWriter(bool writeBOM = false)
		{
			m_writer = new StreamWriter(Data, new UTF8Encoding(writeBOM), 1024, leaveOpen: true);	
		}

		virtual public void Write(string arg)
		{
			m_writer.Write(arg);
		}

		virtual public void Write(string format, params object[] args)
		{
			m_writer.Write(format, args);
		}

		virtual public void WriteLine()
		{
			m_writer.WriteLine();
		}

		virtual public void WriteLine(string arg)
		{
			m_writer.WriteLine(arg);
		}

		virtual public void WriteLine(string format, params object[] args)
		{
			m_writer.WriteLine(format, args);
		}

		virtual public void WriteTab(string arg)
		{
			m_writer.Write(MakeFormat.TAB);
			m_writer.Write(arg);
		}

		virtual public void WriteTab(string format, params object[] args)
		{
			m_writer.Write(MakeFormat.TAB);
			m_writer.Write(format, args);
		}


		virtual public void WriteTabLine(string arg)
		{
			m_writer.Write(MakeFormat.TAB);
			m_writer.WriteLine(arg);
		}

		virtual public void WriteTabLine(string format, params object[] args)
		{
			m_writer.Write(MakeFormat.TAB);
			m_writer.WriteLine(format, args);
		}

		virtual public void WriteComment(string arg)
		{
			m_writer.Write(MakeFormat.COMMENT);
			m_writer.WriteLine(arg);
		}

		virtual public void WriteComment(string format, params object[] args)
		{
			m_writer.Write(MakeFormat.COMMENT);
			m_writer.Write(MakeFormat.SPACE);
			m_writer.WriteLine(format, args);
		}

		virtual public void WriteHeader(string arg)
		{
			WriteLine(MakeFormat.HEADER_SEP_LINE);
			WriteComment(arg);
			WriteLine(MakeFormat.HEADER_SEP_LINE);
			WriteLine();
		}

		virtual public void WriteHeader(string format, params object[] args)
		{
			WriteLine(MakeFormat.HEADER_SEP_LINE);
			WriteComment(format, args);
			WriteLine(MakeFormat.HEADER_SEP_LINE);
			WriteLine();
		}

		protected override void Dispose(bool disposing)
		{
			m_writer.Dispose();
			base.Dispose(disposing);
		}
	}
}
