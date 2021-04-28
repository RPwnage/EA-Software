using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;

namespace NAnt.Core.Writers
{
    public class MakeWriter : CachedWriter, IMakeWriter
    {
        public MakeWriter(bool writeBOM = false)
        {
            _writer = new StreamWriter(Internal, new UTF8Encoding(writeBOM));
            
        }
        virtual public void Write(string arg)
        {
            _writer.Write(arg);
        }

        virtual public void Write(string format, params object[] args)
        {
            _writer.Write(format, args);
        }

        virtual public void WriteLine()
        {
            _writer.WriteLine();
        }

        virtual public void WriteLine(string arg)
        {
            _writer.WriteLine(arg);
        }

        virtual public void WriteLine(string format, params object[] args)
        {
            _writer.WriteLine(format, args);
        }

        virtual public void WriteTab(string arg)
        {
            _writer.Write(MakeFormat.TAB);
            _writer.Write(arg);
        }

        virtual public void WriteTab(string format, params object[] args)
        {
            _writer.Write(MakeFormat.TAB);
            _writer.Write(format, args);
        }


        virtual public void WriteTabLine(string arg)
        {
            _writer.Write(MakeFormat.TAB);
            _writer.WriteLine(arg);
        }

        virtual public void WriteTabLine(string format, params object[] args)
        {
            _writer.Write(MakeFormat.TAB);
            _writer.WriteLine(format, args);
        }

        virtual public void WriteComment(string arg)
        {
            _writer.Write(MakeFormat.COMMENT);
            _writer.WriteLine(arg);
        }

        virtual public void WriteComment(string format, params object[] args)
        {
            _writer.Write(MakeFormat.COMMENT);
            _writer.Write(MakeFormat.SPACE);
            _writer.WriteLine(format, args);
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

        protected override void DisposeResources()
        {
            _writer.Close();
        }

        virtual public Encoding Encoding 
        { 
            get { return _writer.Encoding; }
        }

        private TextWriter _writer;
    }
}
