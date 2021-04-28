using System;
using System.Diagnostics;
using NAnt.Core;

namespace NAnt.Core.Util
{
    public class Chrono
    {
        public Chrono()
        {
            _sw.Start();
        }

        public override string ToString()
        {
            _sw.Stop();
            var elapsed = _sw.Elapsed;
            if (elapsed.Hours == 0 && elapsed.Minutes == 0 && elapsed.Seconds == 0)
            {
                return String.Format("({0} millisec)", elapsed.Milliseconds);
            }
            return String.Format("({0:D2}:{1:D2}:{2:D2})", elapsed.Hours, elapsed.Minutes, elapsed.Seconds);
        }
        private Stopwatch _sw = new Stopwatch();
    }
}
