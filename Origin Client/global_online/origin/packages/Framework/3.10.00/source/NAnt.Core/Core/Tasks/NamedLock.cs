using System;
using System.IO;
using System.Collections.Concurrent;

using NAnt.Core.Attributes;

namespace NAnt.Core.Tasks
{

    /// <summary>Allows wrapping of a group of tasks to be executed in a mutually exclusive way based on 'name' attribute.</summary>
    /// <remarks>
    /// <para>This task acts a a named mutex for a group of nested tasks.
    /// <para>Make sure that name name value does not collite with namedlock names that can be defined in other packages.</para>
    /// </remarks>
    /// <include file='Examples\NamedLock\Simple.example' path='example'/>

    [TaskName("namedlock")]
    public class NamedLock : TaskContainer
    {
        private static ConcurrentDictionary<string, object> _SyncRoots = new ConcurrentDictionary<string, object>();
        private string _lock_name = String.Empty;

        /// <summary>Unique name for the lock. All &lt;namedlock&gt; sections with same name are mutually exclusive
        /// Make sure that key string does not collide with names of &lt;namedlock&gt; that may be defined in other packages. 
        /// Using "package.[package name]." prefix is a good way to ensure unique values.
        /// </summary>
        [TaskAttribute("name", Required = true)]
        public string LockName { get { return _lock_name; } set { _lock_name = value; } }

        protected override void ExecuteTask()
        {
            lock (_SyncRoots.GetOrAdd(LockName, new object()))
            {
                base.ExecuteTask();
            }
        }

        public static void Execute(Project project, string name, Action action)
        {
            lock (_SyncRoots.GetOrAdd(name, new object()))
            {
                action();
            }
        }
    }
}
