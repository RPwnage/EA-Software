using System;
using System.Collections.Generic;
using System.Text;
using System.Threading;
using NAnt.Core;

namespace NAnt.Core.Metrics
{
    internal sealed class BuildEventQueue : WaitHandle
    {
        const int DEFAULT_THREAD_EXPIRE_TIMEOUT = 60 * 60 * 1000;

        internal void Enqueue(BuildEvent buildEvent)
        {
            lock (this)
            {
                if (!_stopping && buildEvent != null)
                {
                    _eventQueue.Enqueue(buildEvent);
                    Monitor.Pulse(this);
                }
            }
        }

        internal BuildEvent Dequeue()
        {
            return _eventQueue.Dequeue();
        }

        internal void Stop()
        {
            lock (this)
            {
                _stopping = true;
                // Clean event queue to free up objects in the queue
                _eventQueue.Clear();

                Monitor.Pulse(this);
            }
        }

        internal int Count
        {
            get
            {
                return _eventQueue.Count;
            }
        }

        internal bool Stopping
        {
            get
            {
                return _stopping;
            }
        }

        internal int threadExpireTimeout = DEFAULT_THREAD_EXPIRE_TIMEOUT;

        private readonly Queue<BuildEvent> _eventQueue = new Queue<BuildEvent>();

        private bool _stopping = false;
    }
}
