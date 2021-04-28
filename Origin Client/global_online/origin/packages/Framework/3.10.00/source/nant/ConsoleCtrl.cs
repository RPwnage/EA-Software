using System;
using System.Threading;
using System.Runtime.InteropServices;

namespace NAnt.Console
{
    /// <remarks>
    /// This class uses the .Net Runtime Interop Services to make a call to the 
    /// win32 API SetConsoleCtrlHandler function. 
    /// 
    /// We need this class so that we can get callbacks when the main console process terminates.
    /// However, if a user decides to kill the process manually (ie from the task manager) we
    /// will not get an event and the process will be abrubtly terminated and results undefined.
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
    public class ConsoleCtrl : IDisposable
    {
        /// <summary>The event that occurred. From wincom.h</summary>
        public enum ConsoleEvent
        {
            CTRL_C = 0,
            CTRL_BREAK = 1,
            CTRL_CLOSE = 2,
            CTRL_LOGOFF = 5,
            CTRL_SHUTDOWN = 6
        }

        /// <summary>Handler to be called when a console event occurs.</summary>
        public delegate void ControlEventHandler(ConsoleEvent consoleEvent);

        /// <summary>Event fired when a console event occurs</summary>
        public event ControlEventHandler ControlEvent;

        ControlEventHandler eventHandler;

        /// <summary>Create a new instance.</summary>
        public ConsoleCtrl()
        {
            // save this to a private var so the GC doesn't collect it...
            eventHandler = new ControlEventHandler(Handler);
            SetConsoleCtrlHandler(eventHandler, true);
        }

        ~ConsoleCtrl()
        {
            Dispose(false);
        }

        public void Dispose()
        {
            Dispose(true);
        }

        void Dispose(bool disposing)
        {
            if (disposing)
            {
                GC.SuppressFinalize(this);
            }
            if (eventHandler != null)
            {
                SetConsoleCtrlHandler(eventHandler, false);
                eventHandler = null;
            }
        }

        private void Handler(ConsoleEvent consoleEvent)
        {
            if (ControlEvent != null)
                ControlEvent(consoleEvent);
        }

        [DllImport("kernel32.dll")]
        static extern bool SetConsoleCtrlHandler(ControlEventHandler e, bool add);
    }
}