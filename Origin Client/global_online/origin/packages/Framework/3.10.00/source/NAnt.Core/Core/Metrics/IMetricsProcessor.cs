using System;
using System.Collections.Generic;
using System.Text;
using NAnt.Core.Events;

namespace NAnt.Core.Metrics
{
    public enum Status { Continue, Done, Failed }

    public interface IMetricsProcessor
    {
        Status Init();

        Status ProcessBuildEvent(BuildEvent buildevent);

        BuildEvent CreateBuildEvent(BuildEvent.ActionState state, object sender, ProjectEventArgs args);

        BuildEvent CreateBuildEvent(BuildEvent.ActionState state, object sender, TargetEventArgs args);

        BuildEvent CreateBuildEvent(BuildEvent.ActionState state, object sender, TaskEventArgs args);

        void Stopping(int timeout);

        void Finish();
    }
}
