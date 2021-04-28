
using System;
using System.Collections;
using System.Collections.Generic;
using System.Collections.Specialized;
using System.IO;
using System.Text;
using System.Xml;

using NAnt.Core.Attributes;
using NAnt.Core.Logging;
using NAnt.Core.Util;
using NAnt.Core.Tasks;

namespace EA.Eaconfig
{

    /// <summary>Executes a system command. Repeats execution if condition is met</summary>
    /// 
    [TaskName("repeatexecif")]
    public class RepeatExecifTask : ExecTask
    {
        public RepeatExecifTask(): base()
        {
        }

        /// <summary>List of patterns.</summary>
        [TaskAttribute("repeatpattern", Required=false)]
        public string RepeatPatterns 
        {
            get { return _repeatpattern; }
            set { _repeatpattern = value; }
        }

        /// <summary>List of patterns.</summary>
        [TaskAttribute("maxrepeatcount", Required=false)]
        public int MaxCount
        {
            get { return _maxCount; }
            set { _maxCount = value; }
        }

        [TaskAttribute("maxlinestoscan", Required = false)]
        public int MaxLines
        {
            get { return _maxLines; }
            set { _maxLines = value; }
        }


        protected override void ExecuteTask() 
        {
            bool repeat = false;
            Exception error = null;
            int count = 0;

            if (!String.IsNullOrEmpty(_repeatpattern))
            {
                ICollection<string>  temp_matches = StringUtil.ToArray(_repeatpattern, "\n");
                if (temp_matches != null && temp_matches.Count > 0)
                {
                    _matches = new List<string>();

                    foreach (string match in temp_matches)
                    {
                        if (!String.IsNullOrEmpty(match))
                        {
                            string clean_match = match.Trim(TRIM_CHARS_KEEP_Q);
                            if (!String.IsNullOrEmpty(clean_match))
                            {
                                _matches.Add(clean_match);
                            }
                        }
                    }
                }
            }

            do
            {
                error = null;
                repeat = false;
                _matchFound = false;
                _scannedLines = 0;
                _matchLine = String.Empty;
                count++;            
                
                try
                {
                    base.ExecuteTask();

                }
                catch (Exception ex)
                {
                    error = ex;
                    repeat = ComputeRepeat(count);

                    if (repeat)
                    {
                        Log.WriteLine("");
                        Log.WriteLine("------------------------------------------------------------------------------");
                        Log.WriteLine("----------");
                        Log.WriteLine("---------- Framework2");
                        Log.WriteLine("----------");
                        Log.WriteLine("---------- detected recoverable error: '{0}'", _matchLine);
                        Log.WriteLine("---------- restarting external command, restartcount = {0} [Max={1}] ...", count, MaxCount);
                        Log.WriteLine("----------");
                        Log.WriteLine("------------------------------------------------------------------------------");
                        Log.WriteLine("");
                    }                    
                }

                count++;
            }
            while (repeat);

            if (error != null)
            {
                throw error;
            }
        }

        protected bool ComputeRepeat(int count)
        {
            return (_matchFound && (count < MaxCount));
        }

        /// <summary>Callback for procrunner stdout</summary>
        public override void LogStdOut(OutputEventArgs outputEventArgs)
        {
            base.LogStdOut(outputEventArgs);

            if (_maxLines <  0 || (_maxLines >  -1 && _scannedLines < _maxLines))
            {
                if (_matches != null && !_matchFound && outputEventArgs != null && outputEventArgs.Line != null)
                {
                    foreach (string match in _matches)
                    {
                        if (outputEventArgs.Line.Contains(match))
                        {
                            _matchFound = true;
                            _matchLine = match;
                            break;
                        }
                    }
                }
            }
            _scannedLines++;
        }


        /// <summary>Callback for procrunner stderr</summary>
        public override void LogStdErr(OutputEventArgs outputEventArgs)
        {
            base.LogStdErr(outputEventArgs);

            if (_maxLines < 0 || (_maxLines > -1 && _scannedLines < _maxLines))
            {
                if (_matches != null && !_matchFound && outputEventArgs != null && outputEventArgs.Line != null)
                {
                    foreach (string match in _matches)
                    {
                        if (outputEventArgs.Line.Contains(match))
                        {
                            _matchFound = true;
                            _matchLine = match;
                            break;
                        }
                    }
                }
            }
            _scannedLines++;
        }

        private ICollection<string> _matches = null;
        private string _repeatpattern = String.Empty;
        private bool _matchFound;
        private int _maxCount = 2;
        private int _maxLines = -1;
        private int _scannedLines = 0;
        private string _matchLine;

        private static readonly char[] TRIM_CHARS_KEEP_Q = new char[] { '\n', '\r', '\t', ' ' };

    }
}
