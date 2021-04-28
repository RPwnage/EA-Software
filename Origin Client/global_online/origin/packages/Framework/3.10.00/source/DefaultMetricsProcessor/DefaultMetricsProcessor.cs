using System;
using System.IO;
using System.IO.Pipes;
using System.Collections.Generic;
using System.Text;
using System.Diagnostics;
using System.Reflection;
using System.Security.Principal;
using NAnt.Core;
using NAnt.Core.Util;
using NAnt.Core.Events;
using NAnt.Core.Metrics;
using EA.FrameworkTasks;
using NAnt.Core.PackageCore;

namespace EA.MetricsProcessor
{
    public class DefaultMetricsProcessor : IMetricsProcessor
    {
        public virtual Status Init()
        {
            bool isTelemetryRynning = false;

            if (PlatformUtil.IsMonoRuntime)
            {
                foreach (var p in Process.GetProcessesByName("mono"))
                {
                    foreach (ProcessModule mod in p.Modules)
                    {
                        if (mod.FileName.Contains("FrameworkTelemetryProcessor.exe"))
                        {
                            isTelemetryRynning = true;
                            break;
                        }
                    }
                    if (isTelemetryRynning)
                    {
                        break;
                    }
                }
            }
            else
            {
                isTelemetryRynning = Process.GetProcessesByName("FrameworkTelemetryProcessor").Length > 0;
            }

            if (!isTelemetryRynning)
            {
                // Start the telemetry tracking process.
                ProcessStartInfo startInfo = new ProcessStartInfo();
                startInfo.CreateNoWindow = true;
                startInfo.UseShellExecute = true;
                string tmpTelemetryProcessorPath = Path.Combine(
                    Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),
                    "temp",
                    "FrameworkTelemetryProcessor.exe");

                if (PlatformUtil.IsMonoRuntime)
                {
                    startInfo.FileName = "mono";
                    startInfo.Arguments = tmpTelemetryProcessorPath;
                }
                else
                {
                    startInfo.FileName = tmpTelemetryProcessorPath;
                }
                startInfo.WindowStyle = ProcessWindowStyle.Hidden;
                startInfo.WorkingDirectory = Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location);

                // Need to reset readonly attribute, otherwise copy may fail
                if (File.Exists(tmpTelemetryProcessorPath))
                {
                    File.SetAttributes(tmpTelemetryProcessorPath, File.GetAttributes(tmpTelemetryProcessorPath) & ~FileAttributes.ReadOnly & ~FileAttributes.Hidden);
                }

                // copy over the existing file if it exists
                File.Copy(Path.Combine(startInfo.WorkingDirectory, "FrameworkTelemetryProcessor.exe"), tmpTelemetryProcessorPath, true);

                Process.Start(startInfo);
            }

            eventStack = new Stack<BuildEvent>();
            targetFilter = new Dictionary<string, TargetEventData.TargetType>();
            targetNamesStack = new Stack<TargetStackData>();
            metricsData = new MetricsData();

            pipeClient = new NamedPipeClientStream(".", "FrameworkTelemetryPipe",
                        PipeDirection.Out, PipeOptions.None);
            pipeClient.Connect();

            detailsLevel = 3;   // log everything.  This will be set
            // correctly in the server process that actually queries the database
            // detailsLevel = dataStore.GetNTLoginDetailsLevel(GetAccountName());
            if (detailsLevel < 0)
            {
                return Status.Done;
            }

            return Status.Continue;
        }

        public virtual void Stopping(int timeout)
        {
            lock (this)
            {
                stoppingTime = DateTime.Now;
                stoppingTimeout = timeout;

                pipeClient.Close();
            }

            // Do I need to pass this to the TelemetryProcessing process?
            // dataStore.SetStopping(stoppingTimeout, stoppingTime);
        }

        private void CheckSkipDbOperationDueToTimeout()
        {
            if (!skipTasks || !skipTargets)
            {
                bool proceed = false;

                lock (this)
                {
                    if (stoppingTimeout > 0)
                    {
                        proceed = true;
                    }
                }
                if (proceed)
                {
                    int timeAfterStopping = 0;

                    lock (this)
                    {
                        TimeSpan stopSpan = (DateTime.Now - stoppingTime);
                        timeAfterStopping = (int)stopSpan.Milliseconds;
                    }
                    if (!skipTasks)
                    {
                        skipTasks = timeAfterStopping > SKIP_TASK_TIMEOUT_MS;
#if DEBUG
                        if (skipTasks)
                        {
                            Console.WriteLine("Timeout SKIP_TASKS. timeAfterStopping={0} ms", timeAfterStopping);
                        }
#endif
                    }
                    if (!skipTargets)
                    {
                        skipTargets = timeAfterStopping > SKIP_TARGETS_TIMEOUT_MS;
#if DEBUG
                        if (skipTargets)
                        {
                            Console.WriteLine("Timeout SKIP_TARGETS. timeAfterStopping={0} ms", timeAfterStopping);
                        }
#endif
                    }
                }
            }
        }

        public virtual void Finish()
        {
            // Do I need to pass this to the TelemetryProcessing process?
            /*
            try
            {
                dataStore.Cleanup();
            }
            catch (Exception) { }
             */
        }

        public virtual BuildEvent CreateBuildEvent(BuildEvent.ActionState state, object sender, ProjectEventArgs args)
        {
            if (args.Project.Nested)
            {
                return null;
            }

            if (args.Project.BuildTargetNames.Count == 0 && String.IsNullOrEmpty(args.Project.DefaultTargetName))
            {
                return null;
            }
            if (targetFilter == null)
            {
                return null;
            }

            // Fill target filter
            targetFilter["incredibuild"] = TargetEventData.TargetType.Undefined;
            targetFilter["sndbs"] = TargetEventData.TargetType.Undefined;
            targetFilter["visualstudio"] = TargetEventData.TargetType.Undefined;
            targetFilter["eaconfig-run-caller"] = TargetEventData.TargetType.Undefined;


            if (args.Project.BuildTargetNames.Count == 0)
            {
                targetFilter[args.Project.DefaultTargetName] = TargetEventData.TargetType.Top;
            }
            else
            {
                foreach (string topTarget in args.Project.BuildTargetNames)
                {
                    targetFilter[topTarget] = TargetEventData.TargetType.Top;
                }
            }
            targetFilter["eaconfig-build-caller"] = TargetEventData.TargetType.BuildCaller;

            BuildEventData eventData = new BuildEventData();
            eventData.Status = args.Status;
            eventData.BuildFileName = args.Project.BuildFileLocalName;
            if (args.Status != ProjectEventArgs.BuildStatus.Started)
            {
                eventData.MasterconfigFile = PackageMap.Instance.MasterConfigFilePath;
            }
            else
            {
                eventData.MasterconfigFile = String.Empty;
            }

            return new BuildEvent(state, eventData);
        }

        public virtual BuildEvent CreateBuildEvent(BuildEvent.ActionState state, object sender, TargetEventArgs args)
        {
            TargetEventData.TargetType targetType;
            if (targetFilter!= null && targetFilter.TryGetValue(args.Target.Name, out targetType))
            {
                TargetEventData eventData = new TargetEventData();

                eventData.Type = targetType;
                eventData.Config = args.Target.Project.Properties["config"];
                eventData.ConfigSystem = args.Target.Project.Properties["config-system"];
                eventData.ConfigCompiler = args.Target.Project.Properties["config-compiler"];
                eventData.BuildGroup = "";
                if (targetType == TargetEventData.TargetType.BuildCaller)
                {
                    eventData.TargetName = args.Target.Project.Properties["eaconfig.build.target"];
                    eventData.BuildGroup = args.Target.Project.Properties["eaconfig.build.group"];
                    if (String.IsNullOrEmpty(eventData.BuildGroup))
                    {
                        eventData.BuildGroup = "runtime";
                    }
                }
                if (String.IsNullOrEmpty(eventData.TargetName))
                {
                    eventData.TargetName = args.Target.Name;
                }
                if (eventData.TargetName == "incredibuild")
                {
                    string usexge = args.Target.Project.Properties["incredibuild.usexge"];
                    if ((eventData.ConfigSystem != null && eventData.ConfigSystem == "ps3") || (usexge != null && usexge.Equals("true", StringComparison.OrdinalIgnoreCase)))
                    {
                        eventData.TargetName = "incredibuild-xge";
                    }
                    else
                    {
                        eventData.TargetName = "incredibuild-console";
                    }
                }
                eventData.Result = args.Target.TargetSuccess ? ResultType.Sucess : ResultType.Failure;

                return new BuildEvent(state, eventData);
            }
            return null;
        }

        public virtual BuildEvent CreateBuildEvent(BuildEvent.ActionState state, object sender, TaskEventArgs args)
        {
            if (detailsLevel < 1)
            {
                return null;
            }
            if (args.Task.Name == "dependent")
            {
                TaskEventData eventData = new TaskEventData();
                DependentTask dtask = args.Task as DependentTask;
                if (dtask != null)
                {
                    eventData.TaskName = args.Task.Name;
                    eventData.PackageName = dtask.PackageName;
                    eventData.PackageVersion = dtask.PackageVersion;
                    eventData.IsAlreadyBuilt = dtask.IsAlreadyBuilt;

                    if (state == BuildEvent.ActionState.Finished)
                    {
                        if (string.IsNullOrEmpty(dtask.PackageVersion))
                        {
                            return null;
                        }
                        if (dtask.Project != null)
                        {
                            eventData.TargetName = dtask.Project.DefaultTargetName;
                        }
                        else
                        {
                            eventData.TargetName = String.Empty;
                        }
                        eventData.BuildGroup = args.Task.Project.Properties["eaconfig.build.group"];
                        if (String.IsNullOrEmpty(eventData.BuildGroup))
                        {
                            eventData.BuildGroup = "runtime";
                        }
                        eventData.Config = args.Task.Project.Properties["config"];
                        eventData.ConfigSystem = args.Task.Project.Properties["config-system"];
                        eventData.ConfigCompiler = args.Task.Project.Properties["config-compiler"];

                        eventData.IsUse = (dtask.Project.TargetStyle == Project.TargetStyleType.Use);
                        eventData.IsBuild = (dtask.Project.TargetStyle == Project.TargetStyleType.Build);

                        Release info = PackageMap.Instance.Releases.FindByNameAndVersion(eventData.PackageName, eventData.PackageVersion);
                        if (info != null)
                        {
                            eventData.Autobuild = true;// info.AutoBuildClean();
                            eventData.IsFramework2 = true;// info.FrameworkVersion == "2";
                        }

                        eventData.Result = args.Task.TaskSuccess ? ResultType.Sucess : ResultType.Failure;
                    }

                    return new BuildEvent(state, eventData);
                }

            }
            else if (args.Task.Name == "package")
            {
                TaskEventData eventData = new TaskEventData();
                PackageTask ptask = args.Task as PackageTask;
                if (ptask != null)
                {
                    eventData.TaskName = args.Task.Name;
                    eventData.PackageName = ptask.PackageName;
                    eventData.PackageVersion = ptask.PackageVersion;
                    eventData.IsAlreadyBuilt = false;

                    if (state == BuildEvent.ActionState.Finished)
                    {
                        if (string.IsNullOrEmpty(ptask.PackageVersion))
                        {
                            return null;
                        }

                        if (ptask.Project != null)
                        {
                            eventData.TargetName = ptask.Project.DefaultTargetName;
                        }
                        else
                        {
                            eventData.TargetName = String.Empty;
                        }
                        eventData.BuildGroup = args.Task.Project.Properties["eaconfig.build.group"];
                        if (String.IsNullOrEmpty(eventData.BuildGroup))
                        {
                            eventData.BuildGroup = "runtime";
                        }
                        eventData.Config = args.Task.Project.Properties["config"];
                        eventData.ConfigSystem = args.Task.Project.Properties["config-system"];
                        eventData.ConfigCompiler = args.Task.Project.Properties["config-compiler"];

                        eventData.IsUse = false;
                        eventData.IsBuild = true;

                        Release info = PackageMap.Instance.Releases.FindByNameAndVersion(eventData.PackageName, eventData.PackageVersion);
                        if (info != null)
                        {
                            eventData.Autobuild = info.AutoBuildClean(ptask.Project);
                            eventData.IsFramework2 = true;// info.FrameworkVersion == "2";
                        }

                        eventData.Result = args.Task.TaskSuccess ? ResultType.Sucess : ResultType.Failure;
                    }
                    return new BuildEvent(state, eventData);
                }

            }
            return null;
        }

        public virtual Status ProcessBuildEvent(BuildEvent buildevent)
        {
            if (IsNeedToStop())
            {
                return Status.Failed;
            }

            Status status = Status.Continue;
            switch (buildevent.State)
            {
                case BuildEvent.ActionState.Started:
                    ProcessStartEvent(buildevent);
                    break;
                case BuildEvent.ActionState.Finished:
                    status = ProcessFinishEvent(buildevent);
                    break;
                default:
#if DEBUG
                    Debug.Fail("Unknown BuildEvent.ActionState");
#endif
                    break;
            }

            return status;
        }

        private void ProcessStartEvent(BuildEvent buildevent)
        {
            eventStack.Push(buildevent);

            if (buildevent.EventData is TaskEventData)
            {
                /*
                TaskEventData data = buildevent.EventData as TaskEventData;
#if DEBUG
                Debug.Assert(data != null, "ProcessStartTaskEvent");
#endif
                string packageName = data.PackageName == null ? MetricsData.UNDEFINED : data.PackageName;
                string packageVersion = data.PackageVersion == null ? MetricsData.UNDEFINED : data.PackageVersion;

                // NAnt.Core.Logging.Log.WriteLine("Telemetry Event Start: TaskEvent - packageName = {0}, packageVersion = {1}",
                //    packageName, packageVersion);
                */
            }
            else if (buildevent.EventData is TargetEventData)
            {
                TargetEventData targetData = buildevent.EventData as TargetEventData;

                targetEventCount++;

                if (targetData.Type == TargetEventData.TargetType.BuildCaller ||
                    targetData.Type == TargetEventData.TargetType.Undefined ||
                    (targetData.Type == TargetEventData.TargetType.Top && targetEventCount == 1))
                {
                    int? parentTargetId = null;
                    if (targetNamesStack.Count > 0)
                    {
                        parentTargetId = targetNamesStack.Peek().TargetId;
                    }
                    int configId = GetConfigKey(targetData.Config, targetData.ConfigSystem, targetData.ConfigCompiler);

                    int targetId;

                    string targetKey = MetricsData.TargetKey(targetData.TargetName, parentTargetId, configId,
                                            targetData.BuildGroup == null ? MetricsData.UNDEFINED : targetData.BuildGroup);

                    if (!metricsData.TargetIDCache.TryGetValue(targetKey, out targetId))
                    {
                        string s = string.Format("START TARGET&targetName={0}&parentTargetId={1}&configId={2}&BuildGroup={3}",
                            targetData.TargetName,
                            parentTargetId,
                            configId,
                            targetData.BuildGroup == string.Empty ? MetricsData.UNDEFINED : targetData.BuildGroup);
                        SendString(s);

                        targetId = s.GetHashCode();

                        metricsData.TargetIDCache[targetKey] = targetId;
                    }

                    targetNamesStack.Push(new TargetStackData(targetData.TargetName, targetId));
                }

                // NAnt.Core.Logging.Log.WriteLine("Telemetry Event Start: TargetEvent - targetName = {0}",
                //    targetData.TargetName);
            }
            else if (buildevent.EventData is BuildEventData)
            {
                buildEventCount++;
                BuildEventData buildData = buildevent.EventData as BuildEventData;

                if (buildEventCount == 1)
                {
                    StringBuilder commandLine = new StringBuilder();

                    string[] args = Environment.GetCommandLineArgs();
                    for (int i = 1; i < args.Length; i++)
                    {
                        if (i > 1)
                            commandLine.Append(' ');
                        commandLine.Append(args[i]);
                    }

                    string machineName = String.Empty;
                    if (Environment.OSVersion.Platform == PlatformID.Unix ||
                        Environment.OSVersion.Platform == PlatformID.MacOSX ||
                        (int)Environment.OSVersion.Platform == 128)
                    {
                        machineName = Environment.MachineName + "." + Environment.UserDomainName;
                    }
                    else
                    {
                        machineName = System.Net.Dns.GetHostEntry("127.0.0.1").HostName;
                        if (String.IsNullOrEmpty(machineName))
                        {
                            machineName = Environment.MachineName;
                        }
                    }

                    string s = string.Format(
                        "START BUILD&machineName={0}&WorkingSet={1}&nantVersion={2}&NTLogIn={3}&IsServiceAccount={4}&BuildFileName={5}&BuildCmdLine={6}",
                        machineName,
                        (int)(Environment.WorkingSet / (1024 * 8)),
                        Assembly.GetEntryAssembly().GetName().Version.ToString(),
                        GetAccountName(),
                        System.Security.Principal.WindowsIdentity.GetCurrent().IsSystem,
                        buildData.BuildFileName,
                        commandLine.ToString());

                    SendString(s);
                }
                // NAnt.Core.Logging.Log.WriteLine("Telemetry Event Start: BuildEvent - buildFileName = {0}, masterconfigfile = {1}",
                //    buildData.BuildFileName, buildData.MasterconfigFile);
            }
            else
            {
#if DEBUG
                Debug.Fail(String.Format("ProcessStartEvent: unknown BuildEvent.EventArgs type={0}", buildevent.EventData.GetType().Name));
#endif
            }
        }

        private Status ProcessFinishEvent(BuildEvent buildevent)
        {
            Status status = Status.Continue;

            BuildEvent buildeventStart = GetMatchingStartEvent(buildevent);

            CheckSkipDbOperationDueToTimeout();

            if (buildevent.EventData is TaskEventData)
            {
                // NAnt.Core.Logging.Log.WriteLine("Telemetry Event Finish: TaskEvent");
                ProcessFinishTaskEvent(buildevent, buildeventStart);
            }
            else if (buildevent.EventData is TargetEventData)
            {
                // NAnt.Core.Logging.Log.WriteLine("Telemetry Event Finish: TargetEvent");
                ProcessFinishTargetEvent(buildevent, buildeventStart);
            }
            else if (buildevent.EventData is BuildEventData)
            {
                // NAnt.Core.Logging.Log.WriteLine("Telemetry Event Finish: BuildEvent");

                ProcessFinishBuildEvent(buildevent, buildeventStart);
                status = buildEventCount == 0 ? Status.Done : Status.Continue;
            }
            else
            {
#if DEBUG
                Debug.Fail(String.Format("StoreEvent: unknown BuildEvent.EventArgs type={0}", buildevent.EventData.GetType().Name));
#endif
            }
            return status;
        }

        private void ProcessFinishBuildEvent(BuildEvent buildevent, BuildEvent buildeventStart)
        {
            buildEventCount--;

            BuildEventData data = buildevent.EventData as BuildEventData;
#if DEBUG
            Debug.Assert(data != null, "ProcessFinishBuildEvent");
#endif
            if (buildEventCount == 0 && buildeventStart != null)
            {
                TimeSpan buildTime = buildevent.Time - buildeventStart.Time;

                string s = string.Format("FINISH BUILD&timeInMS={0}&status={1}",
                    buildTime.TotalMilliseconds, data.Status);

                SendString(s);
            }
        }

        private void ProcessFinishTargetEvent(BuildEvent buildevent, BuildEvent buildeventStart)
        {
            TargetEventData targetData = buildevent.EventData as TargetEventData;
#if DEBUG
            Debug.Assert(targetData != null, "ProcessFinishTargetEvent: invalid event data");
#endif

            targetEventCount--;

            if ((targetData.Type == TargetEventData.TargetType.Top && targetEventCount > 0) || buildeventStart == null)
            {
                return;
            }
#if DEBUG
            Debug.Assert(targetNamesStack.Count > 0, "Target Stack is invalid");
#endif
            TargetStackData targetStackData = targetNamesStack.Pop();

            TimeSpan buildTime = buildevent.Time - buildeventStart.Time;
            int result = (int)targetData.Result;

            if (!skipTargets)
            {
                string s = string.Format("UPDATETARGETINFO&targetID={0}&Result={1}&BuildTimeMillisec={2}",
                    targetStackData.TargetId, result, (int)buildTime.TotalMilliseconds);
                SendString(s);
            }
        }

        private void ProcessFinishTaskEvent(BuildEvent buildevent, BuildEvent buildeventStart)
        {
            TaskEventData data = buildevent.EventData as TaskEventData;
#if DEBUG
            Debug.Assert(data != null, "ProcessFinishTaskEvent");
#endif
            if (buildeventStart != null && !skipTasks)
            {
                // If package task started inside dependent task - skip it

                if (data.TaskName == "package" && eventStack.Count > 0)
                {
                    TaskEventData parentData = eventStack.Peek().EventData as TaskEventData;

                    if (parentData != null && parentData.TaskName == "dependent" && parentData.PackageName == data.PackageName)
                    {
                        return;
                    }
                }

                string packageName = data.PackageName == null ? MetricsData.UNDEFINED : data.PackageName;
                string packageVersion = data.PackageVersion == null ? MetricsData.UNDEFINED : data.PackageVersion;

                int frameworkVersion = data.IsFramework2 ? 2 : 1;

                if (detailsLevel < 2)
                {
                    string key = MetricsData.PackageVersionCache.Key(packageName, packageVersion);
                    MetricsData.PackageVersionCache cachedPackageVersion;

                    if (!metricsData.PackageVersions.TryGetValue(key, out cachedPackageVersion))
                    {
                        int packageVersionId;

                        // dataStore.InsertUpdatePackageInfoBase_GetPackageVersionID(packageName, packageVersion, frameworkVersion, out packageVersionId);
                        string s = string.Format("PACKAGE&PackageName={0}&PackageVersion={1}&FrameworkVersion={2}",
                            packageName, packageVersion, frameworkVersion);
                        packageVersionId = s.GetHashCode();
                        SendString(s);

                        cachedPackageVersion = new MetricsData.PackageVersionCache(packageName, packageVersion, packageVersionId);
                        metricsData.PackageVersions[key] = cachedPackageVersion;
                        cachedPackageVersion.IsUseStored = true;
                    }
                    else
                    {
                        if (cachedPackageVersion.IsUseStored)
                        {
                            return;
                        }

//                        dataStore.InsertUpdatePackageInfoBase(cachedPackageVersion.DbVersionKey, frameworkVersion);
                        cachedPackageVersion.IsUseStored = true;

                        string s = string.Format("PACKAGEUPDATE&PackageVersionID={0}&FrameworkVersion={1}",
                            cachedPackageVersion.DbVersionKey,
                            frameworkVersion);
                        SendString(s);
                    }

                }
                else if (detailsLevel < 3)
                {
                    if (!data.IsBuild)
                    {
                        string key = MetricsData.PackageVersionCache.Key(packageName, packageVersion);
                        MetricsData.PackageVersionCache cachedPackageVersion;

                        if (!metricsData.PackageVersions.TryGetValue(key, out cachedPackageVersion))
                        {
                            int packageVersionId;

//                            dataStore.InsertUpdatePackageInfoUseDepend_GetPackageVersionID(packageName, packageVersion, frameworkVersion, out packageVersionId);
                            string s = string.Format("PACKAGE&PackageName={0}&PackageVersion={1}&FrameworkVersion={2}",
                                packageName, packageVersion, frameworkVersion);
                            packageVersionId = s.GetHashCode();
                            SendString(s);
                            cachedPackageVersion = new MetricsData.PackageVersionCache(packageName, packageVersion, packageVersionId);
                            metricsData.PackageVersions[key] = cachedPackageVersion;
                            cachedPackageVersion.IsUseStored = true;
                        }
                        else
                        {
                            // Use dependencies
                            if (cachedPackageVersion.IsUseStored)
                            {
                                return;
                            }

                            string s = string.Format("PACKAGEUPDATE&PackageVersionID={0}&FrameworkVersionDbVersionKey={1}",
                                cachedPackageVersion.DbVersionKey, frameworkVersion);
                            SendString(s);
                            // dataStore.InsertUpdatePackageInfoUseDepend(cachedPackageVersion.DbVersionKey, frameworkVersion);
                            cachedPackageVersion.IsUseStored = true;
                        }
                    }
                    else if (!data.IsAlreadyBuilt)
                    {
                        // Store build dependency:
                        int? targetId = null;
                        if (targetNamesStack.Count > 0)
                        {
                            int id = targetNamesStack.Peek().TargetId;
                            if (id >= 0)
                            {
                                targetId = id;
                            }
                        }

                        int configID = GetConfigKey(data.Config, data.ConfigSystem, data.ConfigCompiler);

                        string buildGroup = data.BuildGroup == null ? MetricsData.UNDEFINED : data.BuildGroup;
                        int result = (int)data.Result;

                        TimeSpan buildTime = buildevent.Time - buildeventStart.Time;

                        string key = MetricsData.PackageVersionCache.Key(packageName, packageVersion);
                        MetricsData.PackageVersionCache cachedPackageVersion;

                        if (!metricsData.PackageVersions.TryGetValue(key, out cachedPackageVersion))
                        {
                            int packageVersionId;
//                            int packageId = dataStore.InsertUpdatePackageInfoBuildDepend_GetPackageVersionID(packageName, packageVersion, configID, buildGroup,
//                                                                       targetId, data.Autobuild, frameworkVersion, result,
//                                                                       (int)buildTime.TotalMilliseconds, out packageVersionId);
                            string s = string.Format("PACKAGE&PackageName={0}&PackageVersion={1}&ConfigID={2}&BuildGroup={3}&buildTargetID={4}&IsAutobuild={5}&FrameworkVersion={6}&Result={7}&BuildTimeMillisec={8}",
                                packageName, packageVersion, configID, buildGroup,
                                targetId, data.Autobuild, frameworkVersion, result,
                                (int)buildTime.TotalMilliseconds);
                            packageVersionId = s.GetHashCode();
                            SendString(s);

                            cachedPackageVersion = new MetricsData.PackageVersionCache(packageName, packageVersion, packageVersionId);
                            metricsData.PackageVersions[key] = cachedPackageVersion;
                        }
                        else
                        {
                            // int packageId = dataStore.InsertUpdatePackageInfoBuildDepend(cachedPackageVersion.DbVersionKey, configID, buildGroup,
                            //                                            targetId, data.Autobuild, frameworkVersion, result,
                            //                                            (int)buildTime.TotalMilliseconds);
                            string s = string.Format("PACKAGEINFOBUILDDEPEND&PackageVersionID={0}&ConfigID={1}&BuildGroup={2}&buildTargetID={3}&IsAutobuild={4}&FrameworkVersion={5}&Result={6}&BuildTimeMillisec={7}",
                                cachedPackageVersion.DbVersionKey, configID, buildGroup,
                                targetId, data.Autobuild, frameworkVersion, result,
                                (int)buildTime.TotalMilliseconds);
                            SendString(s);
                        }
                    }
                }
                else
                {
                    int? targetId = null;
                    if (targetNamesStack.Count > 0)
                    {
                        int id = targetNamesStack.Peek().TargetId;
                        if (id >= 0)
                        {
                            targetId = id;
                        }
                    }

                    int configID = GetConfigKey(data.Config, data.ConfigSystem, data.ConfigCompiler);
                    string buildGroup = data.BuildGroup == null ? MetricsData.UNDEFINED : data.BuildGroup;
                    int result = (int)data.Result;

                    TimeSpan buildTime = buildevent.Time - buildeventStart.Time;

                    string key = MetricsData.PackageVersionCache.Key(packageName, packageVersion);
                    MetricsData.PackageVersionCache cachedPackageVersion;

                    if (!metricsData.PackageVersions.TryGetValue(key, out cachedPackageVersion))
                    {
                        int packageVersionId;
                        string s = string.Format("PACKAGE&PackageName={0}&PackageVersion={1}&ConfigID={2}&BuildGroup={3}&buildTargetID={4}&IsBuildDependency={5}&IsAutobuild={6}&FrameworkVersion={7}&Result={8}&BuildTimeMillisec={9}",
                           packageName, packageVersion, configID, buildGroup,
                           targetId, data.IsBuild, data.Autobuild, frameworkVersion, result,
                           (int)buildTime.TotalMilliseconds);
                        SendString(s);
                        packageVersionId = s.GetHashCode();

                        cachedPackageVersion = new MetricsData.PackageVersionCache(packageName, packageVersion, packageVersionId);
                        metricsData.PackageVersions[key] = cachedPackageVersion;
                    }
                    else
                    {
                        string s = string.Format("PACKAGEUPDATE&PackageVersionId={0}&ConfigID={1}&BuildGroup={2}&buildTargetID={3}&IsBuildDependency={4}&IsAutobuild={5}&FrameworkVersion={6}&Result={7}&BuildTimeMillisec={8}",
                           cachedPackageVersion.DbVersionKey, configID, buildGroup,
                           targetId, data.IsBuild, data.Autobuild, frameworkVersion, result,
                           (int)buildTime.TotalMilliseconds);
                        SendString(s);
                    }
                }

            }
        }

        private BuildEvent GetMatchingStartEvent(BuildEvent buildeventEnd)
        {
            BuildEvent buildeventStart = null;

            if (eventStack.Count > 0)
            {
                buildeventStart = eventStack.Pop();
#if DEBUG
                if (buildeventStart.EventData.GetType() != buildeventEnd.EventData.GetType())
                {
                    Debug.Assert(buildeventStart.EventData.GetType() != buildeventEnd.EventData.GetType(), "Internal error in event stack, event data types do not match");
                }
#endif
            }

            return buildeventStart;
        }

        private int GetConfigKey(string configName, string configSystem, string configCompiler)
        {
            if (!String.IsNullOrEmpty(configName))
            {
                if (String.IsNullOrEmpty(configSystem))
                {
                    // Try to derive from config name:
                    string[] components = configName.Split(new char[] { '-' });
                    if (components.Length > 2)
                    {
                        configSystem = components[0];
                        configCompiler = components[1];
                    }
                }
            }

            configName = String.IsNullOrEmpty(configName) ? MetricsData.UNDEFINED : configName;
            configSystem = String.IsNullOrEmpty(configSystem) ? MetricsData.UNDEFINED : configSystem;
            configCompiler = String.IsNullOrEmpty(configCompiler) ? MetricsData.UNDEFINED : configCompiler;


            string key = MetricsData.ConfigCache.Key(configName, configSystem, configCompiler);

            MetricsData.ConfigCache cachedConfig;
            if (!metricsData.ConfigKeys.TryGetValue(key, out cachedConfig))
            {
                string s = string.Format("CONFIGID&configName={0}&configSystem={1}&configCompiler={2}",
                    configName, configSystem, configCompiler);
                int id = s.GetHashCode();
                SendString(s);

                cachedConfig = new MetricsData.ConfigCache(configName, configSystem, configCompiler, id);
                metricsData.ConfigKeys[key] = cachedConfig;
            }
            return cachedConfig.DbConfigKey;
        }

        private string GetAccountName()
        {
            WindowsIdentity identity = System.Security.Principal.WindowsIdentity.GetCurrent();
            string name = identity.Name;
            if (identity.IsSystem)
            {
                name = Environment.MachineName + "\\" + name;
                if (name.Length > 101)
                {
                    name = name.Substring(0, 99) + "~";
                }
            }
            return name;
        }


        private bool IsNeedToStop()
        {
            bool stop = false;
            if (stoppingTimeout > 0)
            {
                lock (this)
                {
                    if (stoppingTimeout > 0)
                    {
                        TimeSpan stopSpan = (DateTime.Now - stoppingTime);
                        int stopIn = ((int)stopSpan.Milliseconds - stoppingTimeout);
                        if (stopIn > -65)
                        {
                            stop = true;
                        }
                    }
                }
            }
            return stop;
        }


        /*
                private MetricsData.PackageVersionCache GetPackageVersionKey(string packageName, string packageVersion)
                {
                    string key = MetricsData.PackageVersionCache.Key(packageName, packageVersion);

                    MetricsData.PackageVersionCache cachedPackageVersion;
                    if (!metricsData.PackageVersions.TryGetValue(key, out cachedPackageVersion))
                    {
                        int outPackageID;
                        int id = dataStore.GetPackageAndVersionIDs(packageName, packageVersion, out outPackageID);
                        cachedPackageVersion = new MetricsData.PackageVersionCache(packageName, packageVersion, id, outPackageID);
                        metricsData.PackageVersions[key] = cachedPackageVersion;
                    }
                    return cachedPackageVersion;
                }
        */

        private Stack<BuildEvent> eventStack;
        private int buildEventCount = 0;
        private int targetEventCount = 0;

        private Dictionary<string, TargetEventData.TargetType> targetFilter;

        private Stack<TargetStackData> targetNamesStack;

        private MetricsData metricsData;
       //  private DataStore dataStore = null;

        private int detailsLevel = 0;

        private int stoppingTimeout = -1;
        private DateTime stoppingTime;
        private const int SKIP_TASK_TIMEOUT_MS = 500;
        private const int SKIP_TARGETS_TIMEOUT_MS = 1200;
        private bool skipTasks = false;
        private bool skipTargets = false;

        private NamedPipeClientStream pipeClient;

        internal class TargetStackData
        {
            internal TargetStackData(string name, int id)
            {
                TargetName = name;
                TargetId = id;
            }
            internal readonly string TargetName;
            internal readonly int TargetId;
        }

        public int SendString(string outString)
        {
            byte[] outBuffer = UnicodeEncoding.UTF8.GetBytes(outString);
            int len = outBuffer.Length;
            if (len > UInt16.MaxValue)
            {
                len = (int)UInt16.MaxValue;
            }
            pipeClient.WriteByte((byte)(len / 256));
            pipeClient.WriteByte((byte)(len & 255));
            pipeClient.Write(outBuffer, 0, len);
            pipeClient.Flush();

            return outBuffer.Length + 2;
        }
    }
}
