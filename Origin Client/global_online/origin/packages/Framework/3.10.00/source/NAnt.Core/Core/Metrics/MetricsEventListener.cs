using System;
using System.Collections.Generic;
using System.Text;
using NAnt.Core.Metrics;
using NAnt.Core.Events;

namespace NAnt.Core.Metrics
{
    internal sealed class MetricsEventListener : IMetricsEventListener
    {
        internal MetricsEventListener(BuildEventQueue queue, IMetricsProcessor processor)
        {
            eventQueue = queue;
            metricsProcessor = processor;
        }

        public void BuildStarted(object sender, ProjectEventArgs args)
        {
            try
            {
                eventQueue.Enqueue(
                    metricsProcessor.CreateBuildEvent(BuildEvent.ActionState.Started, sender, args));
            }
            catch (Exception ex)
            {
#if DEBUG
                Console.WriteLine("BuildStarted callback failed: {0}" + ex.Message);
                Console.WriteLine(ex.StackTrace);
#endif
                VoidEx(ex);
            }
        }

        public void BuildFinished(object sender, ProjectEventArgs args)
        {
            try
            {
                eventQueue.Enqueue(
                    metricsProcessor.CreateBuildEvent(BuildEvent.ActionState.Finished, sender, args));
            }
            catch (Exception ex)
            {
#if DEBUG
                Console.WriteLine("BuildFinished callback failed: {0}" + ex.Message);
                Console.WriteLine(ex.StackTrace);
#endif
                VoidEx(ex);
            }

        }

        public void TargetStarted(object sender, TargetEventArgs args)
        {
            try
            {
                eventQueue.Enqueue(
                    metricsProcessor.CreateBuildEvent(BuildEvent.ActionState.Started, sender, args));
            }
            catch (Exception ex)
            {
#if DEBUG
                Console.WriteLine("TargetStarted callback failed: {0}" + ex.Message);
                Console.WriteLine(ex.StackTrace);
#endif
                VoidEx(ex);
            }
        }

        public void TargetFinished(object sender, TargetEventArgs args)
        {
            try
            {
                eventQueue.Enqueue(
                    metricsProcessor.CreateBuildEvent(BuildEvent.ActionState.Finished, sender, args));
            }
            catch (Exception ex)
            {
#if DEBUG
                Console.WriteLine("TargetFinished callback failed: {0}" + ex.Message);
                Console.WriteLine(ex.StackTrace);
#endif
                VoidEx(ex);
            }
        }

        public void TaskStarted(object sender, TaskEventArgs args)
        {
            try
            {
                eventQueue.Enqueue(
                    metricsProcessor.CreateBuildEvent(BuildEvent.ActionState.Started, sender, args));
            }
            catch (Exception ex)
            {
#if DEBUG
                Console.WriteLine("TaskStarted callback failed: {0}" + ex.Message);
                Console.WriteLine(ex.StackTrace);
#endif
                VoidEx(ex);
            }
        }

        public void TaskFinished(object sender, TaskEventArgs args)
        {
            try
            {
                eventQueue.Enqueue(
                    metricsProcessor.CreateBuildEvent(BuildEvent.ActionState.Finished, sender, args));
            }
            catch (Exception ex)
            {
#if DEBUG
                Console.WriteLine("TaskFinished callback failed: {0}" + ex.Message);
                Console.WriteLine(ex.StackTrace);
#endif
                VoidEx(ex);
            }
        }

        public void WaitToFinish()
        {
        }

        // To get rid of compiler warnings
        private void VoidEx(Exception ex) {}

        private BuildEventQueue eventQueue;
        private IMetricsProcessor metricsProcessor;
    }
}
