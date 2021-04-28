using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;

namespace NAnt.Core.Writers
{
    public class TextWriterWithIndent : CachedWriter, ITextWriter
    {
        public TextWriterWithIndent(bool writeBOM = false, string EOL = null)
        {
            _writer = new StreamWriter(Internal, new UTF8Encoding(writeBOM));

            _eol = EOL ?? Environment.NewLine;

            SetPadding();
        }

        virtual public int Indent 
        { 
            get { return _indent; }
            set
            {
                if (_indent >= 0)
                {
                    _indent = value;
                    SetPadding();
                }
            }
        }

        virtual public int IndentSize 
        {
            get { return _indentSize; }
            set
            {
                if (_indentSize > 0)
                {
                    _indentSize = value;
                    SetPadding();
                }
            }
        }

        virtual public void Write(string arg)
        {
            _writer.Write(_padding + arg);
        }

        virtual public void Write(string format, params object[] args)
        {
            _writer.Write(_padding+format, args);
        }

        virtual public void WriteLine()
        {
            _writer.Write(_eol);
        }

        virtual public void WriteLine(string arg)
        {
            _writer.Write(_padding+arg+_eol);
        }

        virtual public void WriteLine(string format, params object[] args)
        {
            _writer.Write(_padding+format+_eol, args);
        }

        virtual public void WriteNonEmpty(string arg)
        {
            if(!String.IsNullOrEmpty(arg))
            {
                Write(arg);
            }
        }

        virtual public void WriteNonEmpty(string format, params object[] args)
        {
            if(args != null && args.Any(a=> a != null &&( ((a is String) && !String.IsNullOrEmpty(a as String)) || ! (a is String) )))
            {
                Write(format, args);
            }
        }

        virtual public void WriteNonEmptyLine(string arg)
        {
            if(!String.IsNullOrEmpty(arg))
            {
                WriteLine(arg);
            }
        }

        virtual public void WriteNonEmptyLine(string format, params object[] args)
        {
            if (args != null && args.Any(a => a != null && (((a is String) && !String.IsNullOrEmpty(a as String)) || !(a is String))))
            {
                WriteLine(format, args);
            }
        }

        protected override void DisposeResources()
        {
            _writer.Close();
        }

        virtual public Encoding Encoding
        {
            get { return _writer.Encoding; }
        }

        private void SetPadding()
        {
            _padding = String.Empty.PadLeft(_indentSize * _indent);
        }

        private TextWriter _writer;
        private int _indent = 0;
        private int _indentSize = 4;
        private string _padding = String.Empty;
        private readonly string _eol;
    }
}
