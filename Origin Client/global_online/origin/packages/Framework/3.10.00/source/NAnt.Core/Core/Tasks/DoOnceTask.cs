using System;
using System.IO;
using System.Threading;
using System.Collections.Concurrent;

using NAnt.Core.Attributes;
using NAnt.Core.PackageCore;

namespace NAnt.Core.Tasks
{

    /// <summary>Container task. Executes nested elements once for a given unique key and a context.</summary>
    /// <remarks>
    /// do.once task is thread safe.
    /// This task acts within single nant process. It can not cross nant process boundary.
    /// </remarks>
    /// <example>
    ///    &lt;do.once key="package.TestPackage.echo_message" context="global"&gt;
    ///     &lt;do if="${config-system}==pc"/&gt;
    ///          &lt;Generate-PC-Code/&gt;
    ///     &lt;/do&gt;
    ///  &lt;/do.once&gt;
    /// </example>
    [TaskName("do.once")]
    public class DoOnce : TaskContainer
    {
        public enum DoOnceContexts {global, project};

        private class ExecuteCount
        {
            internal int count = 0;
        }

        private static readonly Guid ProjectExecuteStatusId = new Guid("F0858FEC-AC85-458E-9222-361F5DD49DDB");
        private static readonly Guid ProjectBlockingExecuteStatusId = new Guid("3BB689EA-CA01-40E9-9A5B-FFF86D7D0E06");
        private static readonly ConcurrentDictionary<string, ExecuteCount> _GlobalExecuteStatus = new ConcurrentDictionary<string, ExecuteCount>();
        private static readonly PackageAutoBuilCleanMap _GlobalExecuteBlockingStatus = new PackageAutoBuilCleanMap();

        private string _key = String.Empty;
        private DoOnceContexts _context = DoOnceContexts.global;
        private bool _isBlocking = false;

        /// <summary>Unique key string. When using global context make sure that key string does 
        /// not collide with keys that may be defined in other packages. 
        /// Using "package.[package name]." prefix is a good way to ensure unique values.
        /// </summary>
        [TaskAttribute("key", Required = true)]
        public string Key { get { return _key; } set { _key = value; } }

        /// <summary>
        /// Context for the key. Context can be either <b>global</b> or <b>project</b>. Default value is <b>global</b>.
        /// <list type="bullet">
        /// <item><b>global</b> context means that for each unique key task is executed once in nant process.</item>
        /// <item><b>project</b> context means that for each unique key task is executed once for each package.</item>
        /// </list>
        /// </summary>
        [TaskAttribute("context", Required = false)]
        public DoOnceContexts Context { get { return _context; } set { _context = value; } }

        /// <summary>
        /// Defines behavior when several instances of do.once task with the same key are invoked simultaneously.
        /// <list type="bullet">
        /// <item><b>blocking=false</b> (default) - One instance will execute nested elements, other instances will return immediately without waiting.</item>
        /// <item><b>blocking=false</b> - One instance will execute nested elements, other instances will wait for the first instance to complete and
        /// then return without executing nested elements.</item>
        /// </list>
        /// </summary>
        [TaskAttribute("blocking", Required = false)]
        public bool IsBlocking { get { return _isBlocking; } set { _isBlocking = value; } }


        protected override void ExecuteTask()
        {
            Execute(Project, Key, () => base.ExecuteTask(), Context, IsBlocking);
        }

        public static void Execute(string key, Action action)
        {
            Execute(null, key, action, DoOnceContexts.global);
        }

        public static void Execute(Project project, string key, Action action, DoOnceContexts context = DoOnceContexts.global, bool isblocking = false)
        {
            if (isblocking)
            {
                PackageAutoBuilCleanMap executeStatus = null;

                switch (context)
                {
                    case DoOnceContexts.global:
                        executeStatus = _GlobalExecuteBlockingStatus;
                        break;
                    case DoOnceContexts.project:
                        executeStatus = project.NamedObjects.GetOrAdd(ProjectBlockingExecuteStatusId, g => new PackageAutoBuilCleanMap()) as PackageAutoBuilCleanMap;
                        break;
                }

                using (PackageAutoBuilCleanMap.PackageAutoBuilCleanState actionstatus = executeStatus.StartBuild(key, "Do.Once"))
                {
                    if (!actionstatus.IsDone())
                    {
                        action();
                    }
                }
            }
            else
            {

                ConcurrentDictionary<string, ExecuteCount> executeStatus = null;
                switch (context)
                {
                    case DoOnceContexts.global:
                        executeStatus = _GlobalExecuteStatus;
                        break;
                    case DoOnceContexts.project:
                        executeStatus = project.NamedObjects.GetOrAdd(ProjectExecuteStatusId, g => new ConcurrentDictionary<string, ExecuteCount>()) as ConcurrentDictionary<string, ExecuteCount>;
                        break;
                }

                var execcount = executeStatus.GetOrAdd(key, new ExecuteCount());
                if (1 == Interlocked.Increment(ref execcount.count))
                {
                    action();
                }
            }

        }
    }
}
