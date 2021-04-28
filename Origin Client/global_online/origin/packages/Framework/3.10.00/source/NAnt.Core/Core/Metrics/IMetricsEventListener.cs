using System;
using System.Collections.Generic;
using System.Text;
using NAnt.Core;
using NAnt.Core.Events;

namespace NAnt.Core.Metrics
{
    internal interface IMetricsEventListener
    {
        void BuildStarted(object sender, ProjectEventArgs args);

        void BuildFinished(object sender, ProjectEventArgs args);

        void TargetStarted(object sender, TargetEventArgs args);

        void TargetFinished(object sender, TargetEventArgs args);

        void TaskStarted(object sender, TaskEventArgs args);

        void TaskFinished(object sender, TaskEventArgs args);

        void WaitToFinish();
    }
}
