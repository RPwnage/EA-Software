using System;

namespace NAnt.Core.Logging
{
    internal class ErrLogWriter : LogWriter
    {
        internal ErrLogWriter(Log log, string prefix = "") : base(log, prefix) { }

        public override void WriteLine()
        {
            foreach (ILogListener listener in _log.Listeners)
            {
                GetRealListener(listener).WriteLine(String.Empty);
            }
        }

        public override void WriteLine(string arg)
        {
            var txt = String.Format("{0}{1}{2}{3}", _log.Id, _log.Padding, _prefix, arg);
            foreach (ILogListener listener in _log.Listeners)
            {
                GetRealListener(listener).WriteLine(txt);
            }
        }

        private ILogListener GetRealListener(ILogListener listener)
        {
            if (typeof(ConsoleListener).IsAssignableFrom(listener.GetType()))
            {
                return stderr;
            }
            return listener;
        }

        private static readonly ConsoleStdErrListener stderr = new ConsoleStdErrListener();
    }

    internal class LogWriter : ILog
    {
        internal LogWriter(Log log, string prefix="")
        {
            _prefix = prefix;
            _log = log;
        }

        public virtual void WriteLine()
        {
            foreach (ILogListener listener in _log.Listeners)
            {
                listener.WriteLine(String.Empty);
            }
        }


        public virtual void WriteLine(string arg)
        {
            var txt = String.Format("{0}{1}{2}{3}", _log.Id, _log.Padding, _prefix, arg);
            foreach (ILogListener listener in _log.Listeners)
            {
                listener.WriteLine(txt);
            }
        }

        public virtual void WriteLine(string format, params object[] args)
        {
            WriteLine(String.Format(format,args));
        }

        public virtual void Write(string arg)
        {
            var txt = String.Format("{0}{1}{2}{3}", _log.Id, _log.Padding, _prefix, arg);

            foreach (ILogListener listener in _log.Listeners)
            {
                listener.Write(txt);
            }
        }

        public virtual void Write(string format, params object[] args)
        {
            Write(String.Format(format, args));
        }

        protected readonly Log _log;
        protected readonly string _prefix;
    }
}
