using System;
using NAnt.Core.Attributes;

namespace NAnt.Core.Tasks
{

    /// <summary>Executes nested tasks in parallel. Only immediate nested tasks are executed in parallel.
    /// <para>
    ///   <b>NOTE.</b> Make sure there are no race conditions, properties with same names, etc each group.
    /// </para>
    /// <para>
    ///   <b>NOTE.</b> local properties are local to each thread. Normal nant properties are shared between threads.
    /// </para>
    /// </summary>
    /// <include file='Examples\Parallel.Do\Simple.example' path='example'/>
    /// <include file='Examples\Parallel.Do\Groups.example' path='example'/>
    [TaskName("parallel.do")]
    public class ParallelDoTask : ParallelTaskContainer
    {
    }
}
