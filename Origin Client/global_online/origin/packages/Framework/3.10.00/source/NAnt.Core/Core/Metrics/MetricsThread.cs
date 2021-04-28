#if DEBUG
#define METRICS_LOG
#endif
using System;
using System.Collections.Generic;
using System.Text;
using System.Threading;
using System.Diagnostics;

namespace NAnt.Core.Metrics
{
    internal class MetricsThread
    {
        private readonly Thread _thread;
        private readonly BuildEventQueue _eventQueue;
        private readonly IMetricsProcessor _metricsProcessor;

        internal MetricsThread(BuildEventQueue eventQueue, IMetricsProcessor metricsProcessor)
        {
            _eventQueue = eventQueue;
            _metricsProcessor = metricsProcessor;

            _thread = new Thread(new ThreadStart(ThreadProc));
            _thread.IsBackground = true;
            _thread.Priority = ThreadPriority.Normal;
        }

        internal void Start()
        {
            _thread.Start();
        }

        internal void Finish(int timeout)
        {
#if METRICS_LOG
            DateTime start = DateTime.Now;
#endif
            bool ret = true;
            try
            {
                _metricsProcessor.Stopping(timeout);
                ret = _thread.Join(timeout);
            }
            catch (Exception)
            {
                ret = false;
            }
            if (!ret)
            {
                _metricsProcessor.Finish();
            }

#if METRICS_LOG
            if (!ret)
            {
                Console.WriteLine("    Metrics error: Finish timed out");
            }
            Console.WriteLine("\t- metrics status='{0}', overhead {1} milliseconds", ret ? "OK" : "timed out", (DateTime.Now - start).TotalMilliseconds);

#endif
        }

        void ThreadProc()
        {
            bool done = false;
            BuildEvent buildEvent = null;

            Status status = Status.Continue;

            try
            {
                if (_metricsProcessor == null)
                {
                    status = Status.Done;
                }
                else
                {
                    status = _metricsProcessor.Init();
                }
            }
            catch (Exception ex)
            {
                status = Status.Failed;

#if METRICS_LOG
                Console.WriteLine("    Metrics error: MetrixProcessor.Init() failed '{0}'", ex.Message);
#endif
                VoidEx(ex);
            }

            if (Status.Continue != status)
            {
                // Stop the queue and exit this thread. 
                _eventQueue.Stop();
                done = true;
            }

            while (!done)
            {
                lock (_eventQueue)
                {
                    bool timedOut = false;

                    while (!_eventQueue.Stopping && !timedOut && (_eventQueue.Count == 0))
                    {
                        if (!Monitor.Wait(_eventQueue, _eventQueue.threadExpireTimeout))
                        {
                            timedOut = true;
                        }
                    }

                    if (!_eventQueue.Stopping && !timedOut && (_eventQueue.Count > 0))
                    {
                        buildEvent = _eventQueue.Dequeue();
                        Debug.Assert(buildEvent != null);
                    }
                    else
                    {
                        done = true;
                    }
                }

                if (!done && (buildEvent != null))
                {
                    try
                    {
#if METRICS_LOG
                        //NAnt.Core.Logging.Log.Write("    [T] ");
#endif
                        status = _metricsProcessor.ProcessBuildEvent(buildEvent);
                    }
                    catch (Exception ex)
                    {
                        status = Status.Failed;
#if METRICS_LOG
                        Console.WriteLine("    Metrics error: build event failed {0}", ex.Message);
#endif
                        VoidEx(ex);
                    }

                    if (status != Status.Continue)
                    {
                        // Stop the queue and exit this thread. 
                        _eventQueue.Stop();
                        done = true;
                    }
                }
            }

            if (_metricsProcessor != null)
            {
                try
                {
                    _metricsProcessor.Finish();
                }
                catch (Exception ex)
                {
#if METRICS_LOG
                    Console.WriteLine("    Metrics error: Processor finish failed {0}", ex.Message);
#endif
                    VoidEx(ex);
                }
            }
        }

        // To get rid of compiler warnings
        private void VoidEx(Exception ex) { }
    }
}
