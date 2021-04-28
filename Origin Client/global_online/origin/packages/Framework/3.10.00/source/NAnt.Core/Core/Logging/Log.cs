using System;
using System.Collections.Generic;
using System.Threading;

namespace NAnt.Core.Logging
{
    public class Log
    {
        public enum LogLevel
        { 
            Quiet,      // Error
            Minimal,    // Error, Warning
            Normal,     // Error, Warning, Status
            Detailed,   // Error, Warning, Status, Info
            Diagnostic  // Error, Warning, Status, Info, Debug
         };

        public enum WarnLevel
        {
            Quiet,          // No warnings
            Minimal,        // No most common warnings (package roots, duplicate packages)
            Normal,         // Warnings on conditions that can affect results
            Advise,         // Warnings on inconsistencies  in build scripts from which framework can auto recover. 
            Deprecation,    // Deprecation warnings
            Diagnostic      // Other warnings useful for diagnostic
        };

        public static WarnLevel WarningLevel = WarnLevel.Normal;

        public static LoggerType DefaultLoggerTypes = LoggerType.Console;

        public Log(LogLevel level, int identLevel = 0, string id = null, bool usestderr=false)
        {
            if (id == null)
            {
                int count = Interlocked.Increment(ref _log_instance_count);
                Id = (count > 0 ? count + "> " : String.Empty).PadRight(6);
            }
            else
            {
                Id = id;
            }

            _indentLevel = 0;
            _indentSize = 4;

            Listeners = new LogListenerCollection();

            if (identLevel >= 0)
            {
                _indentLevel = identLevel;
            }

            SetPadding();

            _level = level;

            UseStdErr = usestderr;

            Error = UseStdErr ? new ErrLogWriter(this, "[error] ") : new LogWriter(this, "[error] ");
            Warning = (level >= LogLevel.Minimal && WarningLevel > WarnLevel.Quiet) ? (ILog)new LogWriter(this, "[warning] ") : new NullLogWriter();
            Status = (level >= LogLevel.Normal) ? (ILog)new LogWriter(this, String.Empty) : new NullLogWriter();
            Info = (level >= LogLevel.Detailed) ? (ILog)new LogWriter(this, String.Empty) : new NullLogWriter();
            Debug = (level >= LogLevel.Diagnostic) ? (ILog)new LogWriter(this, "[debug] ") : new NullLogWriter();
        }


        //IMTODO: Remove these functions. They are for compatibility only
        #region deprecated

        public void WriteLine()
        {
            Status.WriteLine();
        }

        public void WriteLine(string arg)
        {
            Status.WriteLine(arg);
        }

        public void WriteLine(string format, params object[] args)
        {
            Status.WriteLine(format,args);
        }

        public void Write(string arg)
        {
            Status.Write(arg);
        }

        public void Write(string format, params object[] args)
        {
            Status.Write(format, args);
        }

        public void WriteLineIf(bool condition, string arg)
        {
            if(condition)
                Status.WriteLine(arg);
        }
        
        public void WriteIf(bool condition, string arg)
        {
            if (condition)
                Status.Write(arg);
        }
        

        public void WriteLineIf(bool condition, string format, params object[] args)
        {
            if (condition)
                Info.WriteLine(format, args);
        }
        #endregion
 
        public readonly string Id;

        public readonly bool UseStdErr;

        public readonly ILog Error;
        public readonly ILog Warning;
        public readonly ILog Status;
        public readonly ILog Info;
        public readonly ILog Debug;

        /// <summary>Gets or sets the number of spaces in an indent.  Default is four.</summary>
        public int IndentSize
        {
            get { return _indentSize; }
            set 
            {                 
                if (value > 0)
                {
                    _indentSize = value;
                    SetPadding();
                }

            }
        } 

        /// <summary>Gets or sets the indent level.  Default is zero.</summary>
        public int IndentLevel
        {
            get { return _indentLevel; }
            set 
            {
                if (value >= 0)
                {
                    _indentLevel = value;
                    SetPadding();
                }
            }
        } 

        public Log.LogLevel Level
        {
            get { return _level; }
        }

        public readonly LogListenerCollection Listeners;

        public string Padding
        {
            get { return _padding; }
        }

        private void SetPadding()
        {
            _padding = String.Empty.PadLeft(_indentSize * _indentLevel);
        }

        public void Append(Log other)
        {
            foreach (ILogListener listener in Listeners)
            {
                foreach (ILogListener externalListener in other.Listeners)
                {
                    string buffer = externalListener.GetBuffer();
                    if (buffer != null)
                    {
                        listener.Write(buffer);
                    }
                }
            }
        }

        private Log.LogLevel _level;
        private int _indentLevel;
        private int _indentSize;
        private string _padding;

        public HashSet<LogListenersStack> LogListenerStacks = new HashSet<LogListenersStack>();

        static private int _log_instance_count = -1;

    }
}
