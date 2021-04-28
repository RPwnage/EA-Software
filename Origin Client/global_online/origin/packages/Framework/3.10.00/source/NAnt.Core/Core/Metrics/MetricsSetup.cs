#define USE_NEW_METRICS

using System;
using System.Collections.Generic;
using System.Text;
using System.Diagnostics;
using NAnt.Core;
using NAnt.Core.Events;

namespace NAnt.Core.Metrics
{
    public class MetricsSetup
    {
        public static bool SkipMetrics = false;

        public static void SetProject(Project project)
        {
            if (SkipMetrics) return;

            if (_metricsProcessor == null)
            {
                _metricsProcessor = MetricsProcessorFactory.Create(project.Verbose);
                // _metricsProcessor = new PipedMetricsProcessor();
                if (_metricsProcessor != null)
                {
                    _eventQueue = new BuildEventQueue();
                    _metricsListener = new MetricsEventListener(_eventQueue, _metricsProcessor);

                    _metricsThread = new MetricsThread(_eventQueue, _metricsProcessor);

                    _metricsThread.Start();
                }
            }

            if (_metricsProcessor != null && project != null)
            {
                project.BuildStarted += new ProjectEventHandler(_metricsListener.BuildStarted);
                project.BuildFinished += new ProjectEventHandler(_metricsListener.BuildFinished);
                project.TargetStarted += new TargetEventHandler(_metricsListener.TargetStarted);
                project.TargetFinished += new TargetEventHandler(_metricsListener.TargetFinished);
                project.TaskStarted += new TaskEventHandler(_metricsListener.TaskStarted);
                project.TaskFinished += new TaskEventHandler(_metricsListener.TaskFinished);
            }
        }

        private static IMetricsProcessor _metricsProcessor;
        private static IMetricsEventListener _metricsListener;
        private static MetricsThread _metricsThread;
        private static BuildEventQueue _eventQueue;
    }
}
