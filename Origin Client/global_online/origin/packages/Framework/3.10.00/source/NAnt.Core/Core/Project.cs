using System;
using System.IO;
using System.Xml;
using System.Text;
using System.Linq;
using System.Reflection;
using System.Threading;
using System.Collections;
using System.Collections.Generic;
using System.Collections.Concurrent;

using NAnt.Core.Events;
using NAnt.Core.Util;
using NAnt.Core.Reflection;
using NAnt.Core.Logging;
using NAnt.Shared.Properties;
using NAnt.Core.PackageCore;

namespace NAnt.Core
{
    public sealed class Project : IDisposable
    {
        // xml element and attribute names that are not defined in metadata
        private const string ROOT_XML = "project";
        private const string PROJECT_NAME_ATTRIBUTE = "name";
        private const string PROJECT_DEFAULT_ATTRIBUTE = "default";
        private const string PROJECT_BASEDIR_ATTRIBUTE = "basedir";
        private const string TARGET_XML = "target";
        private const string TARGET_DEPENDS_ATTRIBUTE = "depends";

        public const string NANT_PROPERTY_VERSION = "nant.version";
        public const string NANT_PROPERTY_VERBOSE = "nant.verbose";
        public const string NANT_PROPERTY_LOCATION = "nant.location";
        public const string NANT_PROPERTY_PROJECT_NAME = "nant.project.name";
        public const string NANT_PROPERTY_PROJECT_BUILDFILE = "nant.project.buildfile";
        public const string NANT_PROPERTY_PROJECT_MASTERCONFIGFILE = "nant.project.masterconfigfile";
        public const string NANT_PROPERTY_PROJECT_BUILDROOT = "nant.project.buildroot";        
        public const string NANT_PROPERTY_PROJECT_TEMPROOT = "nant.project.temproot";
        public const string NANT_PROPERTY_PROJECT_BASEDIR = "nant.project.basedir";
        public const string NANT_PROPERTY_PROJECT_DEFAULT = "nant.project.default";
        public const string NANT_PROPERTY_PROJECT_PACKAGEROOTS = "nant.project.packageroots";
        public const string NANT_PROPERTY_PROJECT_MASTERTARGET = "mastertarget.name";
        public const string NANT_PROPERTY_PROJECT_CMDTARGETS = "cmdtargets.name";
        public const string NANT_OPTIONSET_PROJECT_CMDPROPERTIES = "nant.commadline.properties";
        public const string NANT_PROPERTY_COPY = "nant.copy";
        public const string NANT_PROPERTY_MKDIR = "nant.mkdir";
        public const string NANT_PROPERTY_COPY_32 = "nant.copy_32";
        public const string NANT_PROPERTY_MKDIR_32 = "nant.mkdir_32";


        public const string NANT_PROPERTY_ONSUCCESS = "nant.onsuccess";
        public const string NANT_PROPERTY_ONFAILURE = "nant.onfailure";
        public const string NANT_PROPERTY_CONTINUEONFAIL = "nant.continueonfail";
        public const string NANT_PROPERTY_PROBING = "nant.probing";
        public const string NANT_PROPERTY_QUICKRUN = "nant.quickrun";
        public const string NANT_PROPERTY_ENDLINE = "nant.endline";
        public const string NANT_PROPERTY_IS_SIGNED = "nant.issigned";
        public const string NANT_PROPERTY_TRANSITIVE = "nant.transitive";
        public const string NANT_PROPERTY_FRAMEWORK3 = "nant.framework3";
        public const string NANT_PROPERTY_WARN_LEVEL = "nant.warnlevel";
        public const string NANT_PROPERTY_LOG_LEVEL = "nant.loglevel";
        public const string NANT_PROPERTY_ONDEMAND = "nant.ondemand";

#if FRAMEWORK_PARALLEL_TRANSITION
        public const bool DEFAULT_IS_TRANSITIVE_VALUE = false;
#else
        public const bool DEFAULT_IS_TRANSITIVE_VALUE = true;
#endif

        public enum TargetStyleType
        {
            Build,
            Clean,
            Use,
        }

        public class GlobalPropertyBag : IEnumerable<GlobalPropertyDef>
        {
            private ConcurrentBag<GlobalPropertyDef> _globalProperties = new ConcurrentBag<GlobalPropertyDef>();
            private HashSet<string> _conditions = new HashSet<string>();

            public IEnumerable<GlobalPropertyDef> EvaluateExceptions(Project project)
            {
                if (project != null)
                {
                    var collection = new List<GlobalPropertyDef>();
                    var evaluated_conditions = new Dictionary<string, bool>();
                    foreach(var c in _conditions)
                    {
                        var evaluator = new ConditionEvaluator(project);
                        try
                        {
                            var expanded_condition = StringParser.ExpandString(c, new StringParser.PropertyEvaluator(evaluator.EvaluateProperty), new StringParser.FunctionEvaluator(project.Properties.EvaluateFunction));
                            if (evaluator.Result)
                            {
                                evaluated_conditions[c] = Expression.Evaluate(expanded_condition);
                            }
                            else
                            {
                                evaluated_conditions[c] = false;
                            }
                        }
                        catch (Exception e)
                        {
                            string msg = String.Format("Failed to evaluate condition for global properies: '{0}'", c);

                            throw new BuildException(msg, e);
                        }
                    }

                    foreach (var gdef in _globalProperties)
                    {
                        if (String.IsNullOrEmpty(gdef.Condition) || evaluated_conditions[gdef.Condition])
                        {
                            int ind = collection.FindIndex(d => d.Name == gdef.Name);
                            if (ind == -1)
                            {
                                collection.Add(gdef);
                            }
                            else
                            {
                                collection[ind] = gdef;
                            }
                        }
                    }
                    return collection;
                    
                }
                return _globalProperties;
            }

            public void Add(string name, bool isReadonly = true, string initialValue = null, string condition = null)
            {

                Project.GlobalPropertyDef oldVal = _globalProperties.FirstOrDefault((item) => item.Name == name && item.Condition == condition);
                if (oldVal != null)
                {
                    oldVal.SetInitialValue(initialValue);
                }
                else
                {
                    _globalProperties.Add(new GlobalPropertyDef(name, true, initialValue, condition));
                    if(!String.IsNullOrEmpty(condition))
                    {
                        _conditions.Add(condition);
                    }
                }
            }

            public void Add(GlobalPropertyDef newVal)
            {
                Project.GlobalPropertyDef oldVal = _globalProperties.FirstOrDefault((item) => item.Name == newVal.Name && item.Condition == newVal.Condition);
                if (oldVal != null)
                {
                    oldVal.SetInitialValue(newVal.InitialValue);
                }
                else
                {
                    _globalProperties.Add(newVal);
                    if (!String.IsNullOrEmpty(newVal.Condition))
                    {
                        _conditions.Add(newVal.Condition);
                    }
                }
            }

            public IEnumerator<GlobalPropertyDef> GetEnumerator()
            {
                foreach (var gdef in _globalProperties)
                {
                    if (!String.IsNullOrEmpty(gdef.Condition))
                    {
                        throw new BuildException("I presence of global properties exceptions (conditions) use method EvaluateExceptions(Project) to enumerate global properties");
                    }

                    yield return gdef;
                }
            }

            IEnumerator IEnumerable.GetEnumerator()
            {
                return GetEnumerator();
            }


            internal class ConditionEvaluator
            {
                internal ConditionEvaluator(Project project)
                {
                    _project = project;
                    _propertyNotFound = false;
                }
                internal string EvaluateProperty(string name)
                {
                    var val = _project.Properties[name];
                    if (val == null)
                    {
                        _propertyNotFound = true;
                        val = String.Empty;
                    }
                    return val;
                }

                internal bool Result
                {
                    get { return (_propertyNotFound == false); }
                }

                private readonly Project _project;
                private bool _propertyNotFound;
            }

        }

        public class GlobalPropertyDef
        {
            public GlobalPropertyDef(string name, bool isReadonly = true, string initialValue = null, string condition = null)
            {
                Name = name;
                IsReadonly = isReadonly;
                _initialValue = initialValue;
                Condition = condition;
            }

            public readonly string Name;
            public readonly bool IsReadonly;
            public string InitialValue
            {
                get { return _initialValue; }
            }
            private string _initialValue;

            public void SetInitialValue(string newvalue)
            {
                _initialValue = newvalue;
            }

            public readonly string Condition;
        }

        public readonly Object SyncRoot;

        public static readonly string NantLocation;

        public static readonly string NantProcessWorkingDirectory;

        public readonly Log Log;

        public static bool EnableCoreDump;

        public static bool NoParallelNant = false;

        /// <summary>The collection of File paths structures pointing to files already included in the project.</summary>
        public readonly PathStringCollection IncludedFiles;

        public readonly FunctionFactory FuncFactory;

        public readonly TaskFactory TaskFactory;

        /// <summary>Global properties.</summary>
        public static readonly GlobalPropertyBag GlobalProperties;

        // TODO:  Temporary reroute until we update Dippybird to the new Manifest interface.        
        public string GetGlobalProperties()
        {
            var text = new StringBuilder();
            foreach (var def in PackageMap.Instance.MasterConfig.GlobalProperties)
            {
                if (String.IsNullOrEmpty(def.Condition))
                {
                    var val = def.Value;
                    if (-1 != def.Value.IndexOfAny(new char[] { ' ', '\n', '\r', '\t' }))
                    {
                        val = val.Quote();
                    }
                    text.AppendFormat("{0}={1}", def.Name, val);
                    text.AppendLine();
                }
                else
                {
                    text.AppendFormat("<if condition=\"{0}\">", def.Condition);
                    text.AppendLine();
                    var val = def.Value;
                    if (-1 != def.Value.IndexOfAny(new char[] { ' ', '\n', '\r', '\t' }))
                    {
                        val = val.Quote();
                    }
                    text.AppendFormat("{0}={1}", def.Name, val);
                    text.AppendLine("</if>");
                }
            }
            return text.ToString();
        }

        /// <summary>Global NamedObjects. Id's of objects that are propagated to dependents</summary>
        public static readonly ISet<Guid> GlobalNamedObjects;


        /// <summary>Global NamedObjects. Id's of objects that are propagated to dependents</summary>
        private static readonly ConcurrentDictionary<string, Object> StaticNamedObjectContainer;


        /// <summary>Returns the active build file XmlDocument.</summary>
        public readonly XmlDocument BuildFileDocument;

        /// <summary>
        /// The URI form of the current Document
        /// </summary>
        public readonly Uri BuildFileURI;

        /// <summary>
        /// If the build document is not file backed then null will be returned.
        /// </summary>
        public string BuildFileLocalName
        {
            get
            {
                if (BuildFileURI != null && BuildFileURI.IsAbsoluteUri && BuildFileURI.IsFile)
                {
                    return BuildFileURI.LocalPath;
                }
                else
                {
                    return null;
                }
            }
        }

        /// <summary>The name of the project.</summary>
        public readonly string ProjectName;

        /// <summary>When true tasks should output more build log messages.</summary>
        public bool Verbose = false;

        /// <remarks>
        ///   <para>Used only if BuildTargets collection is empty.</para>
        /// </remarks>
        public readonly string DefaultTargetName;

        /// <summary> The NAnt Properties.</summary>
        ///
        /// <remarks>
        ///   <para>This is the collection of Properties that are defined by the system and property task statements.</para>
        ///   <para>These properties can be used in expansion.</para>
        /// </remarks>
        public readonly PropertyDictionary Properties;

        public readonly bool PropagateLocalProperties;

        /// <summary>Collection of named objects used by this project.</summary>

        public readonly ObjectDictionary NamedObjects;

        /// <summary>The named option sets in this project.</summary>
        public readonly OptionSetDictionary NamedOptionSets;

        /// <summary>The named file sets in this project.</summary>
        public FileSetDictionary NamedFileSets;

        /// <summary>The user defined tasks included in this this project.</summary>
        public readonly ConcurrentDictionary<string, XmlNode> UserTasks;

        /// <summary>The targets defined in the this project.</summary>
        public readonly TargetCollection Targets;

        /// <summary>The list of target names to built.</summary>
        /// <remarks>
        ///   <para>Targets are built in the order they appear in the collection.  If the collection is empty the default target will be built.</para>
        /// </remarks>
        public readonly List<string> BuildTargetNames;

        public TargetStyleType TargetStyle
        {
            get { return _currentTargetStyle; }
            set { _currentTargetStyle = value; }
        } private TargetStyleType _currentTargetStyle;

        /// <summary>The prefix used when sending messages to the log.</summary>
        public string LogPrefix
        {
            get
            {
                const string prefix = "[nant] ";
                return prefix.PadLeft(Log.IndentSize*3);
            }
        }

        public bool GlobalSuccess
        {
            get { return _globalSuccess; }
            set { _globalSuccess = value; }
        }

        /// <summary>The last error to be thrown.</summary>
        public BuildException LastError
        {
            get { return _lastError; }
            set { _lastError = value; }
        }

        /// <summary>When true all echo instructions will display their location.</summary>
        public bool TraceEcho;

        /// <summary>True if the project is nested. Default is false.</summary>
        public readonly bool Nested;

        public readonly Project ParentProject; // need to keep this to propagate package properties. See comment in nant task

        /// <summary>The Base Directory used for relative file references.</summary>
        public string BaseDirectory
        {
            get
            {
                return _projectBaseDirectory;
            }
        } private string _projectBaseDirectory; // normalized base directory

        /// <summary>Current script file.</summary>
        public string CurrentScriptFile
        {
            get
            {
                return (_projectCurrentScriptFile ?? BuildFileLocalName) ?? String.Empty;
            }
        } private string _projectCurrentScriptFile;


        // For compatibilitity with Framework 2.05 and older
        public readonly LocationMap LocationMap = new LocationMap();

        // project events
        public event ProjectEventHandler BuildStarted;
        public event ProjectEventHandler BuildFinished;
        public event TargetEventHandler TargetStarted;
        public event TargetEventHandler TargetFinished;
        public event TaskEventHandler TaskStarted;
        public event TaskEventHandler TaskFinished;

        public event TaskEventHandler UserTaskStarted;
        public event TaskEventHandler UserTaskFinished;

        /// <summary>Signals that a build has started. This event is fired before any targets have started.</summary>
        public void OnBuildStarted(ProjectEventArgs e)
        {
            if (BuildStarted != null)
                BuildStarted(this, e);
        }

        /// <summary>Signals that the last target has finished. This event will still be fired if an error occurred during the build.</summary>
        public void OnBuildFinished(ProjectEventArgs e)
        {
            if (BuildFinished != null)
                BuildFinished(this, e);
        }

        /// <summary>Signals that a target has started.</summary>
        public void OnTargetStarted(TargetEventArgs e)
        {
            if (TargetStarted != null)
                TargetStarted(this, e);
        }

        /// <summary>Signals that a target has finished. This event will still be fired if an error occurred during the build.</summary>
        public void OnTargetFinished(TargetEventArgs e)
        {
            if (TargetFinished != null)
                TargetFinished(this, e);
        }

        /// <summary>Signals that a task has started.</summary>
        public void OnTaskStarted(TaskEventArgs e)
        {
            if (TaskStarted != null)
                TaskStarted(this, e);
        }

        /// <summary>Signals that a user task has started.</summary>
        public void OnUserTaskStarted(TaskEventArgs e)
        {
            if (UserTaskStarted != null)
                UserTaskStarted(this, e);
        }

        /// <summary>Signals that a user task has finished. This event will still be fired if an error occurred during the build.</summary>
        public void OnUserTaskFinished(TaskEventArgs e)
        {
            if (UserTaskFinished != null)
                UserTaskFinished(this, e);
        }

        /// <summary>Signals that a task has finished. This event will still be fired if an error occurred during the build.</summary>
        public void OnTaskFinished(TaskEventArgs e)
        {
            if (TaskFinished != null)
                TaskFinished(this, e);
        }

        /// <summary>Signals that a task has been found and has been added to NAnt's list of known tasks.</summary>
        private void OnTaskDiscovered(object sender, TaskBuilderEventArgs e)
        {
            if (e.OldBuilder != null && e.OldBuilder.AssemblyFileName != e.TaskBuilder.AssemblyFileName)
            {
                Log.Debug.WriteLine(LogPrefix + "task {0} from '{1}' replaced previous definition from'{2}'", e.TaskBuilder.TaskName, e.TaskBuilder.AssemblyFileName, e.OldBuilder.AssemblyFileName);
            }

            //IM:
            // On Windows 8 this was causing rare code hangs in enterUpgradableLock in  Properties.Add(). Wasn't deadlock, attaching debugger resulted in code resuming execution.
            // Commanting out this code and implemented check by returning old builder in args. Properties below are not used by nant, but I'm not sure whether external projects are using them;

            //// add a true property for each task (use in build to test for task existence).
            //// add a property for each task with the assembly location.
            //if (Properties["nant.tasks." + e.TaskBuilder.TaskName] == null)
            //{
            //    Properties.AddReadOnly("nant.tasks." + e.TaskBuilder.TaskName, "true");
            //    Properties.AddReadOnly("nant.tasks." + e.TaskBuilder.TaskName + ".location", e.TaskBuilder.AssemblyFileName);
            //}
            //else if (Properties["nant.tasks." + e.TaskBuilder.TaskName + ".location"] != e.TaskBuilder.AssemblyFileName)
            //{
            //    Log.Debug.WriteLine(LogPrefix + "task {0} from '{1}' replaced previous definition from'{2}'", e.TaskBuilder.TaskName, e.TaskBuilder.AssemblyFileName, Properties["nant.tasks." + e.TaskBuilder.TaskName + ".location"]);
            //}
        }

        public static T GetTaskStaticObject<T>(string name) where T : new()
        {
            return (T)Project.StaticNamedObjectContainer.GetOrAdd(name, n => new T());

        }

        /// <summary>
        /// Run this once to intialize global static cache.
        /// </summary>
        public static void GlobalInit()
        {

#if NANT_CONCURRENT_DISABLE
            // Under mono most parallel execution is slower than consequtive. Untill this is resolved use consequtive execution 
            bool parallel = (PlatformUtil.IsMonoRuntime == false);
#else
            bool parallel = false;
#endif

            if (parallel)
            {
                System.Threading.Tasks.Parallel.Invoke(
                    () => TaskFactory.GlobalInit(),
                    () => FunctionFactory.GlobalInit()
                );
            }
            else
            {

                TaskFactory.GlobalInit();
                FunctionFactory.GlobalInit();
            }
        }

        static Project()
        {
            NantLocation = Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location);

            NantProcessWorkingDirectory = Environment.CurrentDirectory;

            GlobalProperties = new GlobalPropertyBag();

            // Following global properties are writable:
            GlobalProperties.Add( FrameworkProperties.BulkBuildPropertyName, isReadonly:true);
            GlobalProperties.Add( FrameworkProperties.BulkBuildExcludedPropertyName, isReadonly:true);
            GlobalProperties.Add( FrameworkProperties.AutoBuildUseDependencyPropertyName, isReadonly:false, initialValue:"true" );
            GlobalProperties.Add( "eaconfig.externalbuild", isReadonly:true);
            GlobalProperties.Add( "eaconfig.nantbuild", isReadonly:true);
            GlobalProperties.Add(NANT_PROPERTY_PROJECT_CMDTARGETS,  isReadonly:true);
            GlobalProperties.Add(NANT_PROPERTY_TRANSITIVE, true, DEFAULT_IS_TRANSITIVE_VALUE.ToString().ToLowerInvariant());


            GlobalNamedObjects = new HashSet<Guid>();

            StaticNamedObjectContainer = new ConcurrentDictionary<string, object>();

        }

        /// <summary>
        /// Constructs a new Project with the given source.
        /// </summary>
        /// <param name="source">
        /// <param name="logIdentLevel"></param>
        /// <param name="loglevel"></param>
        /// <param name="parentProject"></param>
        /// <param name="usestderr"></param>
        /// <para> The Source should be the full path to the build file.</para>
        /// <para> This can be of any form that XmlDocument.Load(string url) accepts.</para>
        /// </param>
        /// <remarks><para>If the source is a uri of form 'file:///path' then use the path part.</para></remarks>
        public Project(string source, Log.LogLevel loglevel, int logIdentLevel, Project parentProject = null, bool usestderr = false)
            : this(LineInfoDocument.Load(source), loglevel, logIdentLevel, parentProject, usestderr)
        {
        }

        /// <summary>
        /// Constructs a new Project with the given build file document.
        /// </summary>
        /// <param name="doc">A XmlDocument class containing a valid NAnt build file.</param>
        public Project(XmlDocument doc, Log.LogLevel loglevel, int logIdentLevel, Project parentProject = null, bool usestderr=false)
        {
            SyncRoot = new Object();

            bool useStdErr = parentProject == null ? usestderr : parentProject.Log.UseStdErr;

            Log = new Log(loglevel, logIdentLevel, usestderr: useStdErr);
            Log.Listeners.AddRange(LoggerFactory.Instance.CreateLogger(Log.DefaultLoggerTypes));

            // Initialize properties
            Properties = new PropertyDictionary(this);
            PropagateLocalProperties = false;

            _globalSuccess = true;
            _lastError = null;
            TraceEcho = false;

            Nested = parentProject != null;
            ParentProject = parentProject;

            _baseDirectoryStack = new ConcurrentStack<string>();
            FuncFactory = parentProject != null ? new FunctionFactory(parentProject.FuncFactory) : new FunctionFactory();
            TaskFactory = new TaskFactory();
            IncludedFiles = new PathStringCollection(Log);
            Targets = new TargetCollection(Log);
            BuildTargetNames = new List<string>();
            NamedObjects = new ObjectDictionary();
            NamedOptionSets = new OptionSetDictionary();
            NamedFileSets = new FileSetDictionary();
            UserTasks = new ConcurrentDictionary<string, XmlNode>();

            BuildFileDocument = doc;

            _currentTargetStyle = TargetStyleType.Use;

            BuildFileURI = String.IsNullOrEmpty(doc.BaseURI) ? null : UriFactory.CreateUri(doc.BaseURI);

            // check to make sure that the root element in named correctly
            if (!BuildFileDocument.DocumentElement.Name.Equals(Project.ROOT_XML))
            {
                string msg = String.Format("Root Element must be named '{0}' in '{1}'.", Project.ROOT_XML, doc.BaseURI);
                throw new BuildException(msg);
            }

            // set project properties from project file
            if (BuildFileDocument.DocumentElement.HasAttribute(Project.PROJECT_NAME_ATTRIBUTE))
            {
                ProjectName = BuildFileDocument.DocumentElement.GetAttribute(Project.PROJECT_NAME_ATTRIBUTE);
            }
            if (BuildFileDocument.DocumentElement.HasAttribute(Project.PROJECT_DEFAULT_ATTRIBUTE))
            {
                DefaultTargetName = BuildFileDocument.DocumentElement.GetAttribute(Project.PROJECT_DEFAULT_ATTRIBUTE);
            }

            // Set a reasonable default base directory (the BaseDirectory property can change as tasks included
            // from different build files are included.
            PushBaseDirectory(GetBuildFileDirectory(doc));

            // add nant.* properties
            Assembly assembly = Assembly.GetExecutingAssembly();
            Properties.AddReadOnly(Project.NANT_PROPERTY_FRAMEWORK3, "true");
            Properties.AddReadOnly(Project.NANT_PROPERTY_VERSION, assembly.GetName().Version.ToString());
            Properties.AddReadOnly(Project.NANT_PROPERTY_LOCATION, NantLocation);
            Properties.AddReadOnly(Project.NANT_PROPERTY_COPY, (PlatformUtil.IsMonoRuntime ? "mono " : String.Empty) + Path.Combine(NantLocation, "copy.exe"));
            Properties.AddReadOnly(Project.NANT_PROPERTY_MKDIR, (PlatformUtil.IsMonoRuntime ? "mono " : String.Empty) + Path.Combine(NantLocation, "mkdir.exe"));
            if (PlatformUtil.IsMonoRuntime)
            {
                Properties.AddReadOnly(Project.NANT_PROPERTY_COPY_32, "mono " + Path.Combine(NantLocation, "copy.exe"));
                Properties.AddReadOnly(Project.NANT_PROPERTY_MKDIR_32, "mono " + Path.Combine(NantLocation, "mkdir.exe"));
            }
            else
            {
                Properties.AddReadOnly(Project.NANT_PROPERTY_COPY_32,  Path.Combine(NantLocation, "copy_32.exe"));
                Properties.AddReadOnly(Project.NANT_PROPERTY_MKDIR_32, Path.Combine(NantLocation, "mkdir_32.exe"));
            }

            Properties.AddReadOnly(Project.NANT_PROPERTY_PROJECT_NAME, ProjectName);
            Properties.AddReadOnly(Project.NANT_PROPERTY_PROJECT_DEFAULT, DefaultTargetName);
            Properties.AddReadOnly(Project.NANT_PROPERTY_WARN_LEVEL, Enum.GetName(typeof(Log.WarnLevel), Log.WarningLevel).ToLowerInvariant());
            Properties.AddReadOnly(Project.NANT_PROPERTY_LOG_LEVEL, Enum.GetName(typeof(Log.LogLevel), Log.Level).ToLowerInvariant());

            if (BuildFileURI != null)
            {
                Properties.AddReadOnly(NANT_PROPERTY_PROJECT_BUILDFILE, BuildFileURI.LocalPath);
            }
            Properties.Add(Project.NANT_PROPERTY_CONTINUEONFAIL, "false");
            //Properties.Add(Project.NANT_PROPERTY_PROBING, ProbeOnly.ToString());
            Properties.AddReadOnly(NANT_PROPERTY_QUICKRUN, "false");
            Properties.AddReadOnly(NANT_PROPERTY_ENDLINE, Environment.NewLine);
            byte[] pubkey = assembly.GetName().GetPublicKey();
            bool isSigned = pubkey.Length > 0;
            Properties.AddReadOnly(NANT_PROPERTY_IS_SIGNED, (isSigned ? "true" : "false"));

#if FRAMEWORK_PARALLEL_TRANSITION
            Properties.AddReadOnly("framework.parallel-transition", "true");
#endif

/*
 todo:
            // fake events for already discovered tasks
            foreach (TaskBuilder builder in TaskFactory.Builders.Values)
            {
                OnTaskDiscovered(this, new TaskBuilderEventArgs(builder));
            }
*/
            // register to recieve events from TaskFactory to learn about new tasks as they get discovered
            TaskFactory.TaskDiscovered += new TaskBuilderEventHandler(OnTaskDiscovered);

            // scan project directory (and "tasks" subdirectory to discover new tasks)
            TaskFactory.ScanDirectories(BaseDirectory, Path.Combine(BaseDirectory, "tasks"));

        }

        // Internal "reference" constructor to be used to add a local property context on the stack.
        // Created object references all properties of the parent project, properties that are value types
        // are set to the parent project in Dispose method.

        public Project(Project parent, bool propagateLocalProperties = false)
        {
            Log = parent.Log;
            SyncRoot = parent.SyncRoot;

            // Initialize properties
            Properties = new PropertyDictionary(this, parent.Properties, parent.PropagateLocalProperties);
            PropagateLocalProperties = propagateLocalProperties;
            ParentProject = parent;  // need to keep this to propagate package properties. See comment in nant task

            _lastError = parent._lastError;
            //_baseDirectoryStack = parent._baseDirectoryStack;
            _baseDirectoryStack = new ConcurrentStack<string>(parent._baseDirectoryStack.Reverse());
            FuncFactory = parent.FuncFactory;
            TaskFactory = parent.TaskFactory;
            IncludedFiles = parent.IncludedFiles;
            Targets = parent.Targets;
            BuildTargetNames = parent.BuildTargetNames;
            NamedObjects = parent.NamedObjects;
            NamedOptionSets = parent.NamedOptionSets;
            NamedFileSets = parent.NamedFileSets;
            UserTasks = parent.UserTasks;

            BuildFileDocument = parent.BuildFileDocument;

            BuildFileURI = parent.BuildFileURI;

            ProjectName = parent.ProjectName;

            DefaultTargetName = parent.DefaultTargetName;

            Nested = parent.Nested;

            // project events
            BuildStarted = parent.BuildStarted;
            BuildFinished = parent.BuildFinished;
            TargetStarted = parent.TargetStarted;
            TargetFinished = parent.TargetFinished;
            TaskStarted = parent.TaskStarted;
            TaskFinished = parent.TaskFinished;

            UserTaskStarted = parent.UserTaskStarted;
            UserTaskFinished = parent.UserTaskFinished;


            // Value types to restore:
            _globalSuccess = parent._globalSuccess;
            _currentTargetStyle = parent._currentTargetStyle;
            TraceEcho = parent.TraceEcho;
            Verbose = parent.Verbose;
            _projectBaseDirectory = parent._projectBaseDirectory;

            _parentPropertyContext = parent;
        }

        /// <summary>
        /// Does Execute() and wraps in error handling and time stamping.
        /// </summary>
        /// <returns>Indication of success</returns>
        public bool Run()
        {
            bool success = true;
            DateTime startTime = DateTime.Now;
            try
            {
                // initialize runtime properties
                RunTimeInit();

                // TODO: move more of this reporting goop into the DefaultLogger
                OnBuildStarted(new ProjectEventArgs(this, ProjectEventArgs.BuildStatus.Started));

                Log.Debug.WriteLine(LogPrefix + NAntUtil.NantVersionString);
                Log.Debug.WriteLine(LogPrefix + BuildFileURI);

                bool ok = false;
                try
                {
                    Execute();
                    ok = true;
                }
                catch (ContextCarryingException e)
                {
                    e.PushNAntStackFrame((string.Empty.Equals(ProjectName)) ? "project" : ProjectName, NAntStackScopeType.Project, new Location(this.BuildFileLocalName, 0, 0));
                    throw e;
                }
                finally
                {
                    ExecuteFinalTarget(ok);
                }

                TimeSpan timeSpan = DateTime.Now.Subtract(startTime);

                if (_globalSuccess)
                {
                    // don't show this message if we are doing a nested build as it's confusing
                    if (this.Nested == false)
                    {
                        Log.Status.WriteLine(LogPrefix + "BUILD SUCCEEDED ({0:D2}:{1:D2}:{2:D2})", timeSpan.Hours, timeSpan.Minutes, timeSpan.Seconds);
                    }
                }
                else
                {
                    Log.Status.WriteLine(LogPrefix + "BUILD FAILED ({0:D2}:{1:D2}:{2:D2})", timeSpan.Hours, timeSpan.Minutes, timeSpan.Seconds);
                }

                success = _globalSuccess;
                return success;

            }
            catch (BuildException e)
            {
                TimeSpan timeSpan = DateTime.Now.Subtract(startTime);
                var errormessage = new BufferedLog(Log);

                errormessage.Status.WriteLine(LogPrefix + "BUILD FAILED ({0:D2}:{1:D2}:{2:D2})" + Environment.NewLine, timeSpan.Hours, timeSpan.Minutes, timeSpan.Seconds);

                errormessage.IndentLevel += 1;

                Exception current = e;
                string offset = "";
                while (current != null)
                {
                    if (!(current is BuildExceptionStackTraceSaver))
                    {
                        if (current is BuildException && ((BuildException)current).Location != Location.UnknownLocation)
                        {
                            errormessage.Error.WriteLine("{0}error at {1}", offset, ((BuildException)current).Location.ToString());
                        }
                        foreach (var line in current.Message.LinesToArray())
                        {
                            errormessage.Error.WriteLine(offset + line);
                        }

                        offset += "  ";
                    }
                    current = current.InnerException;
                    
                }
                errormessage.Status.WriteLine();
                errormessage.Flash();

                success = false;
                return false;

            }
            catch (Exception e)
            {
                TimeSpan timeSpan = DateTime.Now.Subtract(startTime);
                var errormessage = new BufferedLog(Log);

                errormessage.Status.WriteLine(LogPrefix + "BUILD FAILED ({0:D2}:{1:D2}:{2:D2})" + Environment.NewLine, timeSpan.Hours, timeSpan.Minutes, timeSpan.Seconds);

                errormessage.IndentLevel += 2;

                while (e != null && e is ExceptionStackTraceSaver)
                {
                    e = e.InnerException;
                }
                if (e != null)
                {
                    var cce = ContextCarryingException.GetLastContextCarryingExceptions(e);
                    if (cce != null)
                    {
                        errormessage.Error.WriteLine("error at {0} {1}  {2}", cce.SourceTaskType, cce.SourceTask.Quote(), Properties[PackageProperties.ConfigNameProperty]??String.Empty);
                    }
                    else
                    {
                        errormessage.Error.WriteLine("INTERNAL ERROR");
                    }

                    Exception current = e;
                    string offset = "  ";
                    while (current != null)
                    {
                        if (!(current is ContextCarryingException || current is BuildExceptionStackTraceSaver || current is ExceptionStackTraceSaver))
                        {
                            foreach (var line in current.Message.LinesToArray())
                            {
                                errormessage.Error.WriteLine(offset + line);
                            }
                            offset += "  ";
                        }
                        current = current.InnerException;
                        
                    }

                    errormessage.Status.WriteLine();
                    errormessage.IndentLevel += 1;
                    foreach (var line in e.StackTrace.LinesToArray())
                    {
                        errormessage.Status.WriteLine(line);
                    }
                }

                errormessage.Status.WriteLine();
                errormessage.Flash();

                CoreDump(e, BaseDirectory + "\\CoreDump.xml");

                success = false;
                return false;
            }
            finally
            {

                //Log.WriteLineIf(ShowTime, LogPrefix + "TIME IS NOW {0} {1}.", DateTime.Now.ToShortDateString(), DateTime.Now.ToShortTimeString());
                OnBuildFinished(new ProjectEventArgs(this, success ? ProjectEventArgs.BuildStatus.Success : ProjectEventArgs.BuildStatus.Failed));
            }
        }


        /// <summary>Executes the default target.</summary>
        /// <remarks>
        ///     <para>No top level error handling is done. Any BuildExceptions will make it out of this method.</para>
        /// </remarks>
        public void Execute()
        {
            // For nested project, it's parent may have already set package.frameworkversion. A nested project
            // usually doesn't have Manifest.xml, and this may result in using wrong framework version for 2.x.
            if (!Properties.Contains("package.frameworkversion"))
            {
                string frameworkVersion = ProjectFrameworkVersion();
                // Store the framework version for this project in a property, for easy access in tasks which
                // don't have visibility onto the PackageCore assembly, ie. <nant> and <nant2>
                Properties.AddReadOnly("package.frameworkversion", frameworkVersion);
            }

            Properties.UpdateReadOnly(Project.NANT_PROPERTY_PROJECT_CMDTARGETS, ((BuildTargetNames.Count > 0) ? BuildTargetNames : (DefaultTargetName ?? String.Empty).ToArray()).ToString(" "));
            
            // Initialize the list of Targets, and execute any global tasks.
            IncludeBuildFileDocument(BuildFileDocument);


            if (BuildTargetNames.Count == 0 && DefaultTargetName != null)
            {
                BuildTargetNames.Add(DefaultTargetName);
            }

            foreach (string targetName in BuildTargetNames)
            {
                Properties.UpdateReadOnly(NANT_PROPERTY_PROJECT_MASTERTARGET, targetName);

                Execute(targetName);
            }
        }

        /// <summary>Executes a specific target, and only that target.</summary>
        /// <param name="targetName">target name to execute.</param>
        /// <param name="failIfTargetDoesNotExist">Fail build if target does not exist</param>
        /// <remarks>
        ///   <para>Only the target is executed. No global tasks are executed.</para>
        /// </remarks>
        public bool Execute(string targetName, bool force = false, bool failIfTargetDoesNotExist= true)
        {
            if (!String.IsNullOrEmpty(targetName))
            {
                Target target;
                if (Targets.TryGetValue(targetName, out target))
                {
                    if (force)
                    {
                        target.Copy().Execute(this);
                    }
                    else
                    {
                        target.Execute(this);
                    }
                    return true;

                }
                else if(failIfTargetDoesNotExist)
                {
                    throw new BuildException(String.Format("Unknown target '{0}'.", targetName));
                }
            }
            else if(failIfTargetDoesNotExist)
            {
                throw new BuildException("Project.Execute(targetname): targetname parameter is null or empty");
            }
            return false;
        }


        public void IncludeBuildFileDocument(XmlDocument doc)
        {
            IncludeBuildFileDocument(doc, null);
        }

        /// <summary>This method is only meant to be used by the <see cref="Project"/> class and <see cref="NAnt.Core.Tasks.IncludeTask"/>.</summary>
        /// <param name="doc">Build file xml document.</param>
        /// <param name="location">Should only be used if XmlDocument is not sourced from a file. Otherwise null.</param>
        public void IncludeBuildFileDocument(XmlDocument doc, Location location)
        {
            string filename = _projectCurrentScriptFile = UriFactory.GetLocalPath(doc.BaseURI);

            IncludedFiles.TryAdd(BuildFileLocalName, filename);

            // get the build file directory to use as the BaseDirectory for this build file
            string buildFileDirectory = GetBuildFileDirectory(doc);
            // push new base directory for this build file
            PushBaseDirectory(buildFileDirectory);
            try
            {
                // initialize targets and global tasks
                foreach (XmlNode childNode in doc.DocumentElement.ChildNodes)
                {
                    if (childNode.NodeType == XmlNodeType.Element && childNode.NamespaceURI.Equals(doc.DocumentElement.NamespaceURI))
                    {
                        if (childNode.Name.Equals(TARGET_XML))
                        {
                            // create new target and add tasks to it
                            Target target = TargetFactory.CreateTarget(this, buildFileDirectory, filename, childNode, location);

                            // Refresh these members in case they changed...
                            target.Project = this;
                            target.Parent = this;
                            target.BaseDirectory = buildFileDirectory;
                            if (location != null)
                                target.Location = location;

                            Targets.Add(target, ignoreduplicate:false);
                        }
                        else
                        {
                            // global tasks
                            Task task = CreateTask(childNode);
                            task.Parent = this;
                            task.Execute();
                        }
                    }
                }
            }
            finally
            {
                // restore base directory
                PopBaseDirectory();
            }

        }

        public void PushBaseDirectory(string dir)
        {
            if (dir == null)
            {
                throw new ArgumentNullException("dir", "BaseDirectory cannot be null.");
            }

            if (dir != _projectBaseDirectory)
            {
                // project's current base directory is stored as a property
                Properties[NANT_PROPERTY_PROJECT_BASEDIR] = dir;
                _projectBaseDirectory = dir;
            }

            _baseDirectoryStack.Push(dir);
        }

        public void PopBaseDirectory()
        {
            string basedir;
            _baseDirectoryStack.TryPop(out basedir);
            // project's current base directory is stored as a property
            _baseDirectoryStack.TryPeek(out basedir);
            if (_projectBaseDirectory != basedir)
            {
                Properties[NANT_PROPERTY_PROJECT_BASEDIR] = basedir;
                _projectBaseDirectory = basedir;
            }
        }

        /// <summary>
        /// Expands a string from known properties
        /// </summary>
        /// <param name="input">The string with replacement tokens</param>
        /// <returns>The expanded and replaced string</returns>
        public string ExpandProperties(string input)
        {
            return Properties.ExpandProperties(input);
        }

        public string GetPropertyValue(string propertyName, bool expand = true)
        {
            string result = Properties[propertyName];
            if (expand && result != null)
            {
                result = ExpandProperties(result);
            }
            return result;
        }


        /// <summary>
        /// Creates a new Task from the given XmlNode
        /// </summary>
        /// <param name="taskNode">The task definition.</param>
        /// <returns>The new Task instance</returns>
        public Task CreateTask(XmlNode taskNode)
        {
            return CreateTask(taskNode, null);
        }

        /// <summary>
        /// Creates a new Task from the given XmlNode within a Target
        /// </summary>
        /// <param name="taskNode">The task definition.</param>
        /// <param name="target">The owner Target</param>
        /// <returns>The new Task instance</returns>
        public Task CreateTask(XmlNode taskNode, Target target)
        {
            Task task = TaskFactory.CreateTask(taskNode, this);
            task.Parent = target;
            task.Initialize(taskNode);
            return task;
        }

        /// <summary>Combine with project's <see cref="BaseDirectory"/> to form a full path to file or directory.</summary>
        /// <returns>
        ///   <para>A rooted path.</para>
        /// </returns>
        /// <param name="path">The relative or absolute path.</param>
        public string GetFullPath(string path)
        {
            if (path == null)
            {
                return BaseDirectory;
            }

            // expand any properties in path
            path = ExpandProperties(path);

            if (!Path.IsPathRooted(path))
            {
                path = Path.Combine(BaseDirectory, path);
            }

            path = PathNormalizer.Normalize(path);

            return path;
        }

        private void ExecuteFinalTarget(bool success)
        {
            string endTargetName = Properties[success ? NANT_PROPERTY_ONSUCCESS : NANT_PROPERTY_ONFAILURE];

            if (!String.IsNullOrEmpty(endTargetName))
            {
                Properties.UpdateReadOnly(NANT_PROPERTY_PROJECT_MASTERTARGET, endTargetName);

                Execute(endTargetName);
            }
        }

        /// <summary>
        /// Initializations which are only available at run time.
        /// </summary>
        private void RunTimeInit()
        {
            // add nant.* which are only available at run time (not construction time)
            Properties.Add(Project.NANT_PROPERTY_VERBOSE, Verbose.ToString());
        }

        private string GetBuildFileDirectory(XmlDocument doc)
        {
            string dir;
            if (String.IsNullOrEmpty(doc.BaseURI))
            {
                // if the project file doesn't exist on disk use the last known directory as the base directory
                if (!_baseDirectoryStack.TryPeek(out dir))
                {
                    // if no project file has ever existed use the current directory
                    dir = Environment.CurrentDirectory;
                }
            }
            else
            {
                Uri uri = UriFactory.CreateUri(doc.BaseURI);
                if (uri.LocalPath != null)
                {
                    dir = Path.GetDirectoryName(uri.LocalPath);
                }
                else
                {
                    dir = Environment.CurrentDirectory;
                }
            }
            dir = PathNormalizer.Normalize(dir);

            if (!Path.IsPathRooted(dir))
            {
                throw new ArgumentException("dir", "BaseDirectory must contain a rooted path.");
            }

            return dir;
        }

        /// <summary>
        /// Checks Framework version for a project. Usually, checking a package's framework version could be done
        /// by PackageTask, but because a project may declare other tasks before or even omit the package tag,
        /// doing so in PackageTask is insufficient.
        /// </summary>
        /// <returns>"2" if Manifest.xml indicates frameworkVersion 2; "1" otherwise</returns>
        private string ProjectFrameworkVersion()
        {
            string frameworkVersion = null;
            // A private package could be declared in an .example file. It may be a Framework 1.x, or 2.x. For
            // latter, it should have Manifest.xml. Example of 2.x: EA.FrameworkTasks\Examples\Profiling
            // For public packages, they are 2.x only if they have Manifest.xml <frameworkVersion> of 2.
            //
            // Building NAnt does unit test on NAnt, by running NAnt.build using nant.exe in
            // ${build.dir}/${config}/NAnt. But since the folder containing NAnt.build doesn't have Manifest.xml,
            // this fools the running NAnt to think it's a 1.x package. So PackageTask still needs to check framework
            // version in addition to this.
            //
            // Note: Now that folder source\NAnt has Manifest.xml, checking Framework version here may be sufficient,
            // and PackageTask may not need to do it.
            var release = PackageMap.Instance.Releases.FindByDirectory(BaseDirectory);
            var manifest = release != null ? release.Manifest : Manifest.Load(BaseDirectory, Log);

            switch (manifest.FrameworkVersion)
            { 
                case FrameworkVersions.Framework2:
                    frameworkVersion = "2";
                    Log.Debug.WriteLine(LogPrefix + BaseDirectory + ": Framework 2");
                    break;
                case FrameworkVersions.Framework1:
                default:
                    frameworkVersion = "1";
                    Log.Debug.WriteLine(LogPrefix + BaseDirectory + ": Framework 1");
                    break;
            }

            return frameworkVersion;
        }

        private void CoreDump(Exception e, string File)
        {
            if (EnableCoreDump)
            {
                try
                {
                    using (XmlTextWriter textWriter = new XmlTextWriter(File, null))
                    {
                        var ce = e as ContextCarryingException;

                        textWriter.Formatting = Formatting.Indented;
                        textWriter.Indentation = 4;
                        textWriter.IndentChar = ' ';
                        textWriter.WriteStartDocument();
                        textWriter.WriteStartElement("coredump");
                        textWriter.WriteStartElement("exception");
                        if (ce != null)
                        {
                            textWriter.WriteAttributeString("atTask", ce.SourceTask);
                        }
                        Exception inner = e.InnerException;
                        while (inner != null)
                        {
                            if (string.Empty != inner.Message)
                            {
                                textWriter.WriteStartElement("message");
                                textWriter.WriteString(inner.Message);
                                textWriter.WriteEndElement();
                            }
                            inner = inner.InnerException;
                        }
                        textWriter.WriteEndElement();

                        textWriter.WriteStartElement("callstack");
                        if (e != null)
                        {
                            if (ce != null)
                            {
                                foreach (var stackFrame in ce.GetCallstack())
                                {
                                    textWriter.WriteStartElement("stackframe");
                                    if (stackFrame.Location != Location.UnknownLocation)
                                        textWriter.WriteAttributeString("file", stackFrame.Location.FileName);
                                    if (stackFrame.Location.LineNumber > 0)
                                        textWriter.WriteAttributeString("line", stackFrame.Location.LineNumber.ToString());
                                    textWriter.WriteAttributeString("scope", stackFrame.MethodName);
                                    textWriter.WriteEndElement();
                                }
                            }
                            else
                            {
                                foreach (var stackFrame in (new System.Diagnostics.StackTrace(e, true).GetFrames()) ?? new System.Diagnostics.StackFrame[0])
                                {
                                    textWriter.WriteStartElement("stackframe");
                                    textWriter.WriteAttributeString("file", stackFrame.GetFileName());
                                    textWriter.WriteAttributeString("line", stackFrame.GetFileLineNumber().ToString());
                                    textWriter.WriteAttributeString("scope", stackFrame.GetMethod().Name);
                                    textWriter.WriteEndElement();
                                }
                            }
                        }
                        textWriter.WriteEndElement();

                        textWriter.WriteStartElement("properties");
                        //Global context
                        textWriter.WriteStartElement("scope");
                        textWriter.WriteAttributeString("name", "global");
                        textWriter.WriteNAntProperies(Properties);
                        textWriter.WriteEndElement();

                        textWriter.WriteEndElement();
                        textWriter.WriteStartElement("filesets");
                        textWriter.WriteNAntFileSets(NamedFileSets);
                        textWriter.WriteEndElement();
                        textWriter.WriteStartElement("optionsets");
                        textWriter.WriteNAntOptionSets(NamedOptionSets);
                        textWriter.WriteEndElement();
                        textWriter.WriteEndElement();
                        textWriter.Close();
                    }
                }
                catch (Exception)
                {
                    // Don't let the core dump throw its own exception as this hide the original/real issue
                    // The core dump, for example can throw an exception when it tries to write all filesets in the function WriteNAntFileSets.
                    // Of course, some filesets have explicit patterns with files that don't exist
                    // This triggers an exception in pattern.cs about "Cannot find explicitly included file"
                }
            }
        }

        // Clone constructor. Make distinct signature by adding parameter
        private Project(Project other, string clone)
        {
            SyncRoot = new Object();
            Log = new Log(other.Log.Level, other.Log.IndentLevel, usestderr: other.Log.UseStdErr);
            Log.Listeners.AddRange(other.Log.Listeners);

            // Initialize properties
            Properties = new PropertyDictionary(this, other);
            PropagateLocalProperties = other.PropagateLocalProperties;
            ParentProject = other.ParentProject;  // need to keep this to propagate package properties. See comment in nant task

            _globalSuccess = other.GlobalSuccess;
            _lastError = other.LastError;
            TraceEcho = other.TraceEcho;

            _baseDirectoryStack = new ConcurrentStack<string>(other._baseDirectoryStack.Reverse());
            _projectBaseDirectory = other._projectBaseDirectory;
            FuncFactory = other.FuncFactory;
            TaskFactory = other.TaskFactory;
            IncludedFiles = new PathStringCollection(other.IncludedFiles, Log);
            Targets = new TargetCollection(Log, other.Targets);
            BuildTargetNames = new List<string>(other.BuildTargetNames);
            NamedObjects = new ObjectDictionary(other.NamedObjects as ConcurrentDictionary<Guid, object>);
            NamedOptionSets = new OptionSetDictionary(other.NamedOptionSets); 
            NamedFileSets = new FileSetDictionary(other.NamedFileSets);
            UserTasks = new ConcurrentDictionary<string, XmlNode>(other.UserTasks);

            BuildFileDocument = other.BuildFileDocument;

            _currentTargetStyle = other._currentTargetStyle;

            BuildFileURI = other.BuildFileURI;

            ProjectName = other.ProjectName;

            DefaultTargetName = other.DefaultTargetName;

            Nested = other.Nested;

            // project events
            BuildStarted = other.BuildStarted;
            BuildFinished = other.BuildFinished;
            TargetStarted = other.TargetStarted;
            TargetFinished = other.TargetFinished;
            TaskStarted = other.TaskStarted;
            TaskFinished = other.TaskFinished;

            UserTaskStarted = other.UserTaskStarted;
            UserTaskFinished = other.UserTaskFinished;


            // Value types to restore:
            Verbose = other.Verbose;
            
        }

        public Project Clone()
        {
            return new Project(this, "clone");
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
                if(_parentPropertyContext != null)
                {
                    lock (_parentPropertyContext.SyncRoot)
                    {
                        _parentPropertyContext._globalSuccess = _globalSuccess;
                        if (_parentPropertyContext._projectBaseDirectory != _projectBaseDirectory)
                        {
                            _parentPropertyContext.Log.Error.WriteLine("Looks like internal error: base directory for a proxy Project() in Dispose() differs from the 'real' project."); 
                            _parentPropertyContext._projectBaseDirectory = _projectBaseDirectory;

                        }
                        
                    }
                }
                else  if (disposing)
                {
                   // Dispose managed resources
                }
            }
            _disposed = true;
        }
        private bool _disposed = false;

        private Project _parentPropertyContext = null;

        private bool _globalSuccess = true;
        private BuildException _lastError = null;
        private ConcurrentStack<string> _baseDirectoryStack;

    }

    /// <summary>Preserves compatibily with 2.05.00 version and older</summary>
    public class LocationMap
    {
        public Location GetLocation(XmlNode node)
        {
            return Location.GetLocationFromNode(node);
        }
    }

}
