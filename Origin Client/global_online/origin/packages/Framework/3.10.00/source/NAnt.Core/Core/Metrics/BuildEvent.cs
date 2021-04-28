using System;
using System.Collections.Generic;
using System.Text;
using NAnt.Core;

namespace NAnt.Core.Metrics
{
    public interface IBuildEventData
    {
    }

    public class BuildEvent
    {
        public enum ActionState { Started, Finished };

        public readonly ActionState State;
        public readonly IBuildEventData EventData;
        public readonly DateTime Time;

        public BuildEvent(BuildEvent.ActionState state, IBuildEventData eventData)
        {
            Time = DateTime.Now;
            State = state;
            EventData = eventData;
        }
    }
}
