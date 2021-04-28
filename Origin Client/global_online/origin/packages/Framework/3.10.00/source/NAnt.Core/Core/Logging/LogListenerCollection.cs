using System;
using System.Threading;
using System.Collections;
using System.Collections.Generic;
using System.Collections.Concurrent;

namespace NAnt.Core.Logging
{

    public class LogListenerCollection : ICollection<ILogListener>,  IEnumerable<ILogListener>
    {
        public LogListenerCollection()
        {
            _listeners = new List<ILogListener>();
        }

        public IEnumerator<ILogListener> GetEnumerator()
        {
                return _listeners.GetEnumerator();
        }

        public int Count 
        { 
            get
            {
                 return _listeners.Count;
            }
        }

        IEnumerator IEnumerable.GetEnumerator()
        {
            return _listeners.GetEnumerator();
        }

        public bool IsReadOnly
        {
            get { return false; }
        }

        public void Add(ILogListener item)
        {
            lock (Lock)
            {
                var newList = new List<ILogListener>(_listeners);

                _listeners = newList;
            }

        }

        public void Clear()
        {
            _listeners = new List<ILogListener>();
        }

        public bool Contains(ILogListener item)
        {
             return _listeners.Contains(item);
        }

        public void CopyTo(ILogListener[] array, int arrayIndex)
        {
               _listeners.CopyTo(array, arrayIndex);
        }

        public bool Remove(ILogListener item)
        {
            bool ret = false;
            lock (Lock)
            {
                var newList = new List<ILogListener>(_listeners);
                ret = newList.Remove(item);
                _listeners = newList;
            }
            return ret;
        }

        public void AddRange(IEnumerable<ILogListener> collection)
        {
            lock (Lock)
            {
                var newList = new List<ILogListener>(_listeners);
                newList.AddRange(collection);
                _listeners = newList;
            }
        }

        public void RemoveRange(IEnumerable<ILogListener> collection)
        {
            lock (Lock)
            {
                var newList = new List<ILogListener>(_listeners);
                foreach (ILogListener listener in collection)
                {
                    newList.Remove(listener);
                }

                _listeners = newList;
            }
        }

        public readonly Object Lock = new Object();

        private volatile List<ILogListener> _listeners;
    }

    public class LogListenersStack : IDisposable
    {
        // Use this constructor to remove all listeners and restore in Dispose()
        public LogListenersStack(bool clear) :
            this(clear, new List<ILogListener> ())
        {
        }

        public LogListenersStack(bool clear, ILogListener listener) :
            this(clear, new List<ILogListener>{listener}) 
        {
        }

        public LogListenersStack(bool clear, IEnumerable<ILogListener> listeners)
        {
            _listeners = listeners;
            _clear = clear;
            _registeredlogs = new ConcurrentBag<Log>();
        }

        public void RegisterLog(Log log)
        {
            _registeredlogs.Add(log);
            lock (log.LogListenerStacks)
            {
                log.LogListenerStacks.Add(this);
            }

            if (_clear)
            {
                
                lock(log.Listeners.Lock)
                {
                    _backup = new List<ILogListener>(log.Listeners);
                    log.Listeners.Clear();
                }
            }
            log.Listeners.AddRange(_listeners);
        }

        public void Dispose()
        {
            Dispose(true);
            GC.SuppressFinalize(this);
        }

        void Dispose(bool disposing)
        {
            if (!this._disposed)
            {
                if (disposing)
                {
                    foreach (var log in _registeredlogs)
                    {
                        log.Listeners.RemoveRange(_listeners);
                        if (_backup != null)
                        {
                            log.Listeners.AddRange(_backup);
                        }
                        lock (log.LogListenerStacks)
                        {
                            log.LogListenerStacks.Remove(this);
                        }
                    }
                }
            }
            _disposed = true;
        }
        private bool _disposed = false;

        bool _clear;
        List<ILogListener> _backup;
        IEnumerable<ILogListener> _listeners;
        ConcurrentBag<Log> _registeredlogs;

    }

}
