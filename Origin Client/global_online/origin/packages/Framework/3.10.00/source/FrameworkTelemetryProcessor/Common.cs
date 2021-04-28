using System;
using System.Collections.Generic;
using System.Text;

using System.Diagnostics;


namespace EATech.Common
{

    public static class Tools
    {

        #region " Write To Event Log "
        private static void _WriteToEventLog(String inSource, String inMessage, EventLogEntryType inEventLogEntryType)
        {
            try
            {
                if (!System.Diagnostics.EventLog.SourceExists(inSource))
                {
                    System.Diagnostics.EventLog.CreateEventSource(inSource, "Application");
                }
                System.Diagnostics.EventLog.WriteEntry(inSource, inMessage, inEventLogEntryType);
            }
            catch { } // Do nothing
        }
        public static void WriteToEventLog(String inSubSource, String inMessage)
        {
            _WriteToEventLog(inSubSource, inMessage, EventLogEntryType.Information);
        }
        public static void WriteToEventLog(String inSubSource, String inMessage, EventLogEntryType inEventLogEntryType)
        {
            _WriteToEventLog(inSubSource, inMessage, inEventLogEntryType);
        }
        public static void WriteToEventLog(String inSubSource, String inMessage, Exception inException)
        {
            _WriteToEventLog(String.Format("EXCEPTION{0}", inSubSource), String.Format("{0}\n\n{1}", String.IsNullOrEmpty(inMessage) ? "EXCEPTION" : inMessage, inException.ToString()), System.Diagnostics.EventLogEntryType.Error);
        }
        public static void WriteToEventLog(String inMessage, Exception inException)
        {
            _WriteToEventLog("EXCEPTION", String.Format("{0}\n\n{1}", inMessage, inException.ToString()), System.Diagnostics.EventLogEntryType.Error);
        }
        public static void WriteToEventLog(Exception inException)
        {
            _WriteToEventLog("EXCEPTION", inException.ToString(), System.Diagnostics.EventLogEntryType.Error);
        }
        [Conditional("DEBUG")]
        public static void DebugWriteToEventLog(String inSubSource, String inMessage, EventLogEntryType inEventType)
        {
            _WriteToEventLog(String.Format("DEBUG{0}", inSubSource), inMessage, inEventType);
        }
        [Conditional("TRACE")]
        public static void TraceWriteToEventLog(String inSubSource, String inMessage, EventLogEntryType inEventType)
        {
            _WriteToEventLog(String.Format("TRACE{0}", inSubSource), inMessage, inEventType);
        }
        #endregion
    }
}
