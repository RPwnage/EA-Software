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
using System.Linq;
using System.Text;
using System.IO;

namespace NAnt.Core.Writers
{
	public class TextWriterWithIndent : CachedWriter, ITextWriter
	{
        public virtual int Indent
        {
            get { return m_ident; }
            set
            {
                if (m_ident >= 0)
                {
                    m_ident = value;
                    SetPadding();
                }
            }
        }

        public virtual int IndentSize
        {
            get { return m_identSize; }
            set
            {
                if (m_identSize > 0)
                {
                    m_identSize = value;
                    SetPadding();
                }
            }
        }

        public virtual Encoding Encoding
        {
            get { return m_writer.Encoding; }
        }

		private bool m_pad = false;
		private int m_ident = 0;
		private int m_identSize = 4;
		private string m_padding = String.Empty;

		private readonly TextWriter m_writer;
		private readonly string m_eol;

		public TextWriterWithIndent(bool writeBOM = false, string EOL = null)
		{
			m_writer = new StreamWriter(Data, new UTF8Encoding(writeBOM), 1024, leaveOpen: true);
			m_eol = EOL ?? Environment.NewLine;
			SetPadding();
		}

        public virtual void Write(string arg)
		{
            Pad();
			m_writer.Write(arg);
		}

        public virtual void Write(string format, params object[] args)
		{
            Pad();
            m_writer.Write(format, args);
		}

		public virtual void WriteLine()
		{
            m_writer.Write(m_eol);
            m_pad = true;
		}

		public virtual void WriteLine(string arg)
		{
            Pad();
            m_writer.Write(arg + m_eol);
            m_pad = true;
        }

		public virtual void WriteLine(string format, params object[] args)
		{
            Pad();
            m_writer.Write(format + m_eol, args);
            m_pad = true;
        }

		public virtual void WriteNonEmpty(string arg)
		{
			if (!String.IsNullOrEmpty(arg))
			{
				Write(arg);
			}
		}

		public virtual void WriteNonEmpty(string format, params object[] args)
		{
			if (args != null && args.Any(a=> a != null &&( ((a is String) && !String.IsNullOrEmpty(a as String)) || ! (a is String) )))
			{
				Write(format, args);
			}
		}

		public virtual void WriteNonEmptyLine(string arg)
		{
			if (!String.IsNullOrEmpty(arg))
			{
				WriteLine(arg);
			}
		}

		public virtual void WriteNonEmptyLine(string format, params object[] args)
		{
			if (args != null && args.Any(a => a != null && (((a is String) && !String.IsNullOrEmpty(a as String)) || !(a is String))))
			{
				WriteLine(format, args);
			}
		}

		protected override void Dispose(bool disposing)
		{
			m_writer.Dispose();
			base.Dispose(disposing);
		}

		private void SetPadding()
		{
			m_padding = String.Empty.PadLeft(m_identSize * m_ident);
		}

        private void Pad()
        {
            if (m_pad)
            {
                m_pad = false;
                m_writer.Write(m_padding);
            }
        }
    }
}