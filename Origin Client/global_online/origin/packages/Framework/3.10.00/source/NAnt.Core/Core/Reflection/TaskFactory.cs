using System;
using System.Linq;
using System.Collections.Concurrent;
using System.Reflection;
using System.IO;
using System.Xml;
using System.Threading;
using System.Threading.Tasks;
using System.Diagnostics;


using NAnt.Core.Util;
using NAnt.Core.Threading;
using NAnt.Core.Attributes;
using NAnt.Core.Events;

namespace NAnt.Core.Reflection
{
    public sealed class TaskFactory
    {
        internal event TaskBuilderEventHandler TaskDiscovered;

        static TaskFactory()
        {
            TASK_ASSEMBLY_PATTERNS = new string[] { "Tasks.dll", "Tests.dll", "config.dll" };
        }

        internal static void GlobalInit()
        {
            _globalTaskBuilders = new TaskFactory();

            // initialize builtin NAnt.Core tasks
            _globalTaskBuilders.AddTasks(Assembly.GetExecutingAssembly());

            // scan the directory where NAnt.Core.dll lives (this assembly) and a tasks subfolder
            string nantDir = Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location);
            _globalTaskBuilders.ScanDirectories(nantDir, Path.Combine(nantDir, "tasks"));
        }

        internal TaskFactory()
        {
            if (_globalTaskBuilders != null)
            {
                _taskBuilders = new ConcurrentDictionary<string, TaskBuilder>(_globalTaskBuilders._taskBuilders);
            }
            else
            {
                _taskBuilders = new ConcurrentDictionary<string, TaskBuilder>();
            }
        }

        /// <summary>Scans the directorylist for any task assemblies and adds them to list of known tasks.</summary>
        /// <param name="paths">The full path to the directory to scan.</param>
        internal void ScanDirectories(params string[] paths)
        {
#if NANT_CONCURRENT
            // Under mono most parallel execution is slower than consequtive. Untill this is resolved use consequtive execution 
            bool parallel = (PlatformUtil.IsMonoRuntime == false);
#else
                bool parallel = false;
#endif
            if (parallel)
            {
                Parallel.ForEach(paths, path =>
                {
                    // Don't do anything if we don't have a valid directory path
                    if (!String.IsNullOrWhiteSpace(path) && Directory.Exists(path))
                    {
                        var assemblies = ReflectionUtil.GetAssemblyFiles(path, TASK_ASSEMBLY_PATTERNS);
                        Parallel.ForEach(assemblies, assemblyFile =>
                        {
                            AddTasks(AssemblyLoader.Get(assemblyFile), assemblyFile);
                        });
                    }
                });
            }
            else
            {
                foreach (var path in paths)
                {
                    // Don't do anything if we don't have a valid directory path
                    if (!String.IsNullOrWhiteSpace(path) && Directory.Exists(path))
                    {
                        var assemblies = ReflectionUtil.GetAssemblyFiles(path, TASK_ASSEMBLY_PATTERNS);

                        foreach (var assemblyFile in assemblies)
                        {
                            AddTasks(AssemblyLoader.Get(assemblyFile), assemblyFile);
                        }
                    }
                }
            }
        }

        internal int AddTasks(Assembly assembly, string assemblyFile = "unknown assembly location")
        {
            return AddTasks(assembly, false, false, assemblyFile);
        }

        internal int AddTasks(Assembly assembly, bool overrideExistingTask, bool failOnError, string assemblyFile)
        {
            int taskCount = 0;
#if NANT_CONCURRENT
            bool parallel = (PlatformUtil.IsMonoRuntime == false);
#else  
            bool parallel = false;
#endif
            try
            {
                if (parallel)
                {
                    Parallel.ForEach(assembly.GetTypes(), type =>
                    {
                        if (type.IsSubclassOf(typeof(Task)) && !type.IsAbstract)
                        {
                            // check if the class has defined a TaskName attribute
                            TaskNameAttribute attribute = Attribute.GetCustomAttribute(type, typeof(TaskNameAttribute)) as TaskNameAttribute;
                            if (attribute != null)
                            {
                                TaskBuilder taskBuilder = new TaskBuilder(type, attribute.Name);
                                TaskBuilder oldBuilder;

                                if (AddBuilder(taskBuilder, (overrideExistingTask || attribute.Override), failOnError, out oldBuilder))
                                {
                                    OnTaskDiscovered(new TaskBuilderEventArgs(oldBuilder, taskBuilder));
                                    Interlocked.Increment(ref taskCount);
                                }
                            }
                        }
                    });

                }
                else
                {
                    foreach (Type type in assembly.GetTypes())
                    {
                        if (type.IsSubclassOf(typeof(Task)) && !type.IsAbstract)
                        {
                            // check if the class has defined a TaskName attribute
                            TaskNameAttribute attribute = Attribute.GetCustomAttribute(type, typeof(TaskNameAttribute)) as TaskNameAttribute;
                            if (attribute != null)
                            {
                                TaskBuilder taskBuilder = new TaskBuilder(type, attribute.Name);
                                TaskBuilder oldBuilder;

                                if (AddBuilder(taskBuilder, (overrideExistingTask || attribute.Override), failOnError, out oldBuilder))
                                {
                                    OnTaskDiscovered(new TaskBuilderEventArgs(oldBuilder, taskBuilder));
                                    taskCount++;
                                }
                            }
                        }
                    }
                }
            }
            catch (Exception e)
            {
                var assemblyLocation = String.IsNullOrEmpty(assembly.Location) ? assemblyFile : assembly.Location;
                Exception ex = ThreadUtil.ProcessAggregateException(e);
                string msg = String.Format("Could not load tasks from '{0}'. {1}", assemblyLocation, ex.Message);
                if (ex is ReflectionTypeLoadException)
                {
                    msg = String.Format("Could not load tasks from '{0}'. Reasons:", assemblyLocation);
                    foreach (var reason in (ex as ReflectionTypeLoadException).LoaderExceptions.Select(le => le.Message).OrderedDistinct())
                    {
                        msg += Environment.NewLine + "\t" + reason;
                    }

                    ex = null; // We retrieved messages from inner exption, no need to add it.
                }

                throw new BuildException(msg, ex);
            }
            return taskCount;
        }

        private bool AddBuilder(TaskBuilder builder, bool overrideExistingBuilder, bool failOnError, out TaskBuilder oldbuilder)
        {
            bool ret = false;
            oldbuilder = null;
            if (overrideExistingBuilder)
            {
                //_taskBuilders[builder.TaskName] = builder;
                TaskBuilder tmp = null;
                _taskBuilders.AddOrUpdate(builder.TaskName, builder, (key, ob) => { tmp = ob; return builder; });
                oldbuilder = tmp;
                ret = true;
            }
            else
            {
                if (_taskBuilders.TryAdd(builder.TaskName, builder))
                {
                    ret = true;
                }
                else if (failOnError)
                {
                    throw new BuildException("Task '" + builder.TaskName + "' already exists!");
                }
            }

            return ret;
        }

        /// <summary> Creates a new Task instance for the given xml and project.</summary>
        /// <param name="taskNode">The XML to initialize the task with.</param>
        /// <param name="project">The Project that the Task belongs to.</param>
        /// <returns>The Task instance.</returns>
        public Task CreateTask(XmlNode taskNode, Project project)
        {
            TaskBuilder builder;
            if (_taskBuilders.TryGetValue(taskNode.Name, out builder))
            {
                Task task = builder.CreateTask();
                task.Project = project;
                return task;
            }
            throw new BuildException(String.Format("Unknown task <{0}>", taskNode.Name), Location.GetLocationFromNode(taskNode));
        }

        /// <summary> Creates a new Task instance for the given task name and project.</summary>
        /// <param name="taskName">The task name to initialize the task with.</param>
        /// <param name="project">The Project that the Task belongs to.</param>
        /// <returns>The Task instance, or null if not found.</returns>
        public Task CreateTask(string taskName, Project project)
        {
            TaskBuilder builder;
            if (_taskBuilders.TryGetValue(taskName, out builder))
            {
                Task task = builder.CreateTask();
                task.Project = project;
                return task;
            }
            return null;
        }

        /// <summary>Signals that a task has been found and has been added to NAnt's list of known tasks.</summary>
        private void OnTaskDiscovered(TaskBuilderEventArgs e)
        {
            if (TaskDiscovered != null)
            {
                TaskDiscovered(this, e);
            }
        }

        private readonly ConcurrentDictionary<string, TaskBuilder> _taskBuilders;

        private static TaskFactory _globalTaskBuilders;

        private static readonly string[] TASK_ASSEMBLY_PATTERNS;


    }
}
