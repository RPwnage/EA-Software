// Originally based on NAnt - A .NET build tool
// Copyright (C) 2003-2015 Electronic Arts Inc.
// Copyright (C) 2001-2003 Gerry Shaw
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
// 
// As a special exception, the copyright holders of this software give you 
// permission to link the assemblies with independent modules to produce 
// new assemblies, regardless of the license terms of these independent 
// modules, and to copy and distribute the resulting assemblies under terms 
// of your choice, provided that you also meet, for each linked independent 
// module, the terms and conditions of the license of that module. An 
// independent module is a module which is not derived from or based 
// on these assemblies. If you modify this software, you may extend 
// this exception to your version of the software, but you are not 
// obligated to do so. If you do not wish to do so, delete this exception 
// statement from your version. 
// 
// 
// 
// Gerry Shaw (gerry_shaw@yahoo.com)
// Ian MacLean (ian_maclean@another.com)
// Electronic Arts (Frostbite.Team.CM@ea.com)

using System;
using System.Collections;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.IO;
using System.Linq;
using System.Net;
using System.Reflection;
using System.Text;
using System.Xml;

using NAnt.Core.Events;
using NAnt.Core.Logging;
using NAnt.Core.PackageCore;
using NAnt.Core.Reflection;
using NAnt.Core.Util;
using NAnt.Shared.Properties;

using Microsoft.Win32;

namespace NAnt.Core
{
	public sealed class Project : IDisposable
	{
		public class GlobalPropertyDef
		{
			public string InitialValue { get; private set; }

			public readonly string Condition;
			public readonly string Name;

			public GlobalPropertyDef(string name, string initialValue = null, string condition = null)
			{
				Name = name;
				InitialValue = initialValue;
				Condition = condition;
			}

			public void SetInitialValue(string newvalue)
			{
				InitialValue = newvalue;
			}

			public override string ToString()
			{
				return $"{Name}{(InitialValue != null ? $" = {InitialValue}" : String.Empty)}{(Condition != null ? $" (Condition: {Condition})" : String.Empty)}";
			}
		}

		public class GlobalPropertyBag
		{
			internal class ConditionEvaluator
			{
				internal bool Result
				{
					get { return (m_propertyNotFound == false); }
				}

				private readonly Project m_project;

				private bool m_propertyNotFound;

				internal ConditionEvaluator(Project project)
				{
					m_project = project;
					m_propertyNotFound = false;
				}

				internal string EvaluateProperty(string name, Stack<string> evaluationStack)
				{
					var val = m_project.Properties.EvaluateParameter(name, evaluationStack);
					if (val == null)
					{
						int defaultOperator = name.IndexOf("??");
						if (defaultOperator < 0)
						{
							m_propertyNotFound = true;
							val = String.Empty;
						}
						else if (defaultOperator > 0)
						{
							val = m_project.Properties.EvaluateParameter(name.Substring(0, defaultOperator).Trim(), evaluationStack);
							if (val == null)
							{
								val = (defaultOperator + 2 >= name.Length) ? String.Empty : name.Substring(defaultOperator + 2).Trim();
							}
						}
					}
					return val;
				}
			}

			private readonly List<GlobalPropertyDef> m_globalProperties = new List<GlobalPropertyDef>();
			private readonly HashSet<string> m_condiitions = new HashSet<string>();

			// DAVE-FUTURE-REFACTOR-TODO - doe this handle chained references? i.e if the value of one property is 
			// dependent on a another does it work or fail depending on resolve order?
			public IEnumerable<GlobalPropertyDef> EvaluateExceptions(Project project)
			{
				if (project != null)
				{
					var evaluated_conditions = new Dictionary<string, bool>();
					foreach (var c in m_condiitions)
					{
						var evaluator = new ConditionEvaluator(project);
						try
						{
							var expanded_condition = StringParser.ExpandString
							(
								c,
								new StringParser.PropertyEvaluator(evaluator.EvaluateProperty),
								new StringParser.FunctionEvaluator(project.Properties.EvaluateFunction),
								new StringParser.FindClosePropertyMatches(p => new string[] { })
							);

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
							string msg = String.Format("Failed to evaluate condition for global properties: '{0}'", c);

							throw new BuildException(msg, e);
						}
					}

					var collection = new Dictionary<string, GlobalPropertyDef>();

					foreach (var gdef in m_globalProperties)
					{
						if (String.IsNullOrEmpty(gdef.Condition) || evaluated_conditions[gdef.Condition])
						{
							collection[gdef.Name] = gdef;
						}
					}
					return collection.Values;

				}
				return m_globalProperties;
			}

			public void Add(string name, string initialValue = null, string condition = null)
			{
				GlobalPropertyDef oldVal = m_globalProperties.FirstOrDefault((item) => item.Name == name && item.Condition == condition);
				if (oldVal != null)
				{
					oldVal.SetInitialValue(initialValue);
				}
				else
				{
					m_globalProperties.Add(new GlobalPropertyDef(name, initialValue, condition));
					if (!String.IsNullOrEmpty(condition))
					{
						m_condiitions.Add(condition);
					}
				}
			}

			public void Add(GlobalPropertyDef newVal)
			{
				GlobalPropertyDef oldVal = m_globalProperties.FirstOrDefault((item) => item.Name == newVal.Name && item.Condition == newVal.Condition);
				if (oldVal != null)
				{
					oldVal.SetInitialValue(newVal.InitialValue);
				}
				else
				{
					m_globalProperties.Add(newVal);
					if (!String.IsNullOrEmpty(newVal.Condition))
					{
						m_condiitions.Add(newVal.Condition);
					}
				}
			}

			// for testing only
			internal void Clear()
			{
				m_condiitions.Clear();
				m_globalProperties.Clear();
			}
		}

		private class GlobalProjectState
		{
			internal bool GlobalSuccess = true;
			internal BuildException LastError = null;
			internal bool CancelBuildTargetExecution = false;
		}

		public enum TargetStyleType
		{
			Build,
			Clean,
			Use
		}

		public enum LocalProperitesPropagation
		{
			InheritableOnly,    // only local properties marked as inheritable are copied to child project
			InheritAll,         // all local properties are copied to child project
			Share               // local properties instance is shared, changes made in child are reflected in parent
		}

		public const string NANT_OPTIONSET_PROJECT_CMDPROPERTIES = "nant.commandline.properties";
		public const string NANT_OPTIONSET_PROJECT_CMDPROPERTIES_LEGACY = "nant.commadline.properties";
		public const string NANT_OPTIONSET_PROJECT_GLOBAL_CMDPROPERTIES = "nant.commandline.global.properties";

		public const string NANT_PROPERTY_CONTINUEONFAIL = "nant.continueonfail";
		public const string NANT_PROPERTY_ENV_PATH_SEPARATOR = "nant.env.path.separator";
		public const string NANT_PROPERTY_LINK_DEPENDENCY_IN_CHAIN = "nant.has-link-in-dependency-chain";
		public const string NANT_PROPERTY_LOG_LEVEL = "nant.loglevel";
		public const string NANT_PROPERTY_LOG_LEVEL_NUM = "nant.loglevelnum";
		public const string NANT_PROPERTY_ONDEMAND = "nant.ondemand";
		public const string NANT_PROPERTY_PACKAGESERVERONDEMAND = "nant.packageserverondemand";
		public const string NANT_PROPERTY_ONFAILURE = "nant.onfailure";
		public const string NANT_PROPERTY_ONSUCCESS = "nant.onsuccess";
		public const string NANT_PROPERTY_PROBING = "nant.probing";
		public const string NANT_PROPERTY_PROJECT_BASEDIR = "nant.project.basedir";
		public const string NANT_PROPERTY_PROJECT_BUILDFILE = "nant.project.buildfile";
		public const string NANT_PROPERTY_PROJECT_BUILDROOT = "nant.project.buildroot";
		public const string NANT_PROPERTY_PROJECT_CMDTARGETS = "cmdtargets.name";
		public const string NANT_PROPERTY_PROJECT_DEFAULT = "nant.project.default";
		public const string NANT_PROPERTY_PROJECT_ISTOPLEVEL = "nant.project.istoplevel";
		public const string NANT_PROPERTY_PROJECT_MASTERCONFIGFILE = "nant.project.masterconfigfile";
		public const string NANT_PROPERTY_PROJECT_MASTERTARGET = "mastertarget.name";
		public const string NANT_PROPERTY_PROJECT_NAME = "nant.project.name";
		public const string NANT_PROPERTY_PROJECT_ONDEMAND_ROOT = "nant.project.ondemandroot";
		public const string NANT_PROPERTY_PROJECT_PACKAGEROOTS = "nant.project.packageroots";
		public const string NANT_PROPERTY_PROJECT_TEMPROOT = "nant.project.temproot";
		public const string NANT_PROPERTY_QUICKRUN = "nant.quickrun";
		public const string NANT_PROPERTY_SUPPORT_CHANNEL = "nant.supportchannel";
		public const string NANT_PROPERTY_TRANSITIVE = "nant.transitive";
		public const string NANT_PROPERTY_VERSION = "nant.version";
		public const string NANT_PROPERTY_WARN_LEVEL = "nant.warnlevel";
		public const string NANT_PROPERTY_WARN_LEVEL_NUM = "nant.warnlevelnum";
		public const string NANT_PROPERTY_USE_NEW_CSPROJ = "nant.use-new-csproj";
		public const string NANT_PROPERTY_USE_NUGET_RESOLUTION = "nant.use-nuget-resolution";
		public const string NANT_PROPERTY_NET_CORE_SUPPORT = "nant.net-core-support";

		public const string NANT_PROPERTY_COPY = "nant.copy";
		public const string NANT_PROPERTY_MKDIR = "nant.mkdir";
		public const string NANT_PROPERTY_RMDIR = "nant.rmdir";

		[Obsolete("The BuildStarted event is deprecated and setting is has no effect.")]
		public event ProjectEventHandler BuildStarted { add { DeprecatedEventWarning("Setting BuildStarted"); } remove { DeprecatedEventWarning("Removing BuildStarted"); } }
		[Obsolete("The BuildFinished event is deprecated and setting is has no effect.")]
		public event ProjectEventHandler BuildFinished { add { DeprecatedEventWarning("Setting BuildFinished"); } remove { DeprecatedEventWarning("Removing BuildFinished"); } }
		[Obsolete("The TargetStarted event is deprecated and setting is has no effect.")]
		public event TargetEventHandler TargetStarted { add { DeprecatedEventWarning("Setting TargetStarted"); } remove { DeprecatedEventWarning("Removing TargetStarted"); } }
		[Obsolete("The TargetFinished event is deprecated and setting is has no effect.")]
		public event TargetEventHandler TargetFinished { add { DeprecatedEventWarning("Setting TargetFinished"); } remove { DeprecatedEventWarning("Removing TargetFinished"); } }
		[Obsolete("The TaskStarted event is deprecated and setting is has no effect.")]
		public event TaskEventHandler TaskStarted { add { DeprecatedEventWarning("Setting TaskStarted"); } remove { DeprecatedEventWarning("Removing TaskStarted"); } }
		[Obsolete("The TaskFinished event is deprecated and setting is has no effect.")]
		public event TaskEventHandler TaskFinished { add { DeprecatedEventWarning("Setting TaskFinished"); } remove { DeprecatedEventWarning("Removing TaskFinished"); } }
		[Obsolete("The UserTaskStarted event is deprecated and setting is has no effect.")]
		public event TaskEventHandler UserTaskStarted { add { DeprecatedEventWarning("Setting UserTaskStarted"); } remove { DeprecatedEventWarning("Removing UserTaskStarted"); } }
		[Obsolete("The UserTaskFinished event is deprecated and setting is has no effect.")]
		public event TaskEventHandler UserTaskFinished { add { DeprecatedEventWarning("Setting UserTaskFinished"); } remove { DeprecatedEventWarning("Removing UserTaskFinished"); } }

		public bool Nested { get { return ParentProject != null; } }
		public bool Verbose { get { return Log.Level >= Log.LogLevel.Detailed; } }

		[Obsolete("BuildMessage class is obsolete. Use methods from the Log class instead.")]
		public BuildMessage BuildMessage
		{
			get
			{
				return new BuildMessage(Log);
			}
		}

		internal bool CancelBuildTargetExecution { get { return m_projectState.CancelBuildTargetExecution; } set { m_projectState.CancelBuildTargetExecution = value; } }
		internal bool GlobalSuccess { get { return m_projectState.GlobalSuccess; } set { m_projectState.GlobalSuccess = value; } }
		internal BuildException LastError { get { return m_projectState.LastError; } set { m_projectState.LastError = value; } }

		public TargetStyleType TargetStyle // within Framework code TargetStyle could be readonly, but outside tasks sometimes manipulate this
		{
			get { return m_targetStyle; }
			set
			{
				Log.ThrowDeprecation
				(
					Log.DeprecationId.ModifyingTargetStyle, Log.DeprecateLevel.Normal,
					DeprecationUtil.GetCallSiteKey(),
					"{0} Setting TargetStyle is considered deprecated.", DeprecationUtil.GetCallSiteLocation()
				);
				m_targetStyle = value;
			}
		}

		public bool UseNugetResolution => Properties.GetBooleanPropertyOrFail(NANT_PROPERTY_USE_NUGET_RESOLUTION);
		public bool UseNewCsProjFormat => Properties.GetBooleanPropertyOrFail(NANT_PROPERTY_USE_NEW_CSPROJ);
		public bool NetCoreSupport => Properties.GetBooleanPropertyOrFail(NANT_PROPERTY_NET_CORE_SUPPORT);

		public readonly string ProjectName; // TODO do we really need a project name?

		public readonly Log Log;
		public readonly string LogPrefix; // DAVE-FUTURE-REFACTOR-TODO this could be static, but we'd need to go update every usage to not use instance reference

		public readonly IncludedFilesCollection IncludedFiles;
		public readonly FunctionFactory FuncFactory;
		public readonly TaskFactory TaskFactory;

		public readonly ConcurrentDictionary<string, XmlNode> UserTasks;
		public readonly Dictionary<string, string> CommandLineProperties;
		public readonly FileSetDictionary NamedFileSets;
		public readonly List<object> Partials;
		public readonly ObjectDictionary NamedObjects;
		public readonly OptionSetDictionary NamedOptionSets;
		public readonly PropertyDictionary Properties;
		public readonly TargetCollection Targets;

		public readonly Object PackageInitLock; // Used for safety around checking for multiple includes of packages at different versions - possible with masterconfig
												// exceptions
		public readonly Object ScriptInitLock; // DAVE-FUTURE-REFACTOR-TODO: this lock currently protects us from multiple initialize.xmls running in parallel,
											   // which, for the most part, protects from parallel script execution. Parallel script execution is a Bad Thing
											   // because users may be using non-local properties for control flow with name collisions between two scripts
											   // meaning the executions interfere with each other
											   // what this should REALLY be is a lock an all nant script execution, which is *mostly* doable, if a little bit of
											   // work to track down all the various ways nant script can be executed...
											   // ...however one thing that can screw is here is if custom C# tasks call parallel tasks which try to execute scripts
											   // - these will deadlock because the custom C# task is itself being called from nant script and thus holds the lock
											   // the parallel tasks need to include their own scripts (FBConfig unfortunately has tasks like this)


		public readonly Object BuildSyncRoot;

		public readonly string BuildFileLocalName; // path build file was loaded from if applicable
		public readonly string BaseDirectory;
		public readonly string CurrentScriptFile; // script file for this context if applicable

		public readonly ReadOnlyCollection<string> BuildTargetNames; // The list of target names to be built. Targets are built in the order they appear in the collection.

		public readonly Project ParentProject = null; // need to keep this to propagate package properties. See comment in NAnt task

		public static bool EnableCoreDump = false;
#if NANT_CONCURRENT
		public static bool NoParallelNant = false;
#else
		public static bool NoParallelNant = true;
#endif
		public static bool TraceEcho = false; // When true all echo tasks will display their location.
		public static bool SanitizeFbConfigs = false;

		public static readonly string NantLocation;
		public static readonly string NantDrive;
		public static readonly string NantVersion;

		public static readonly string NantProcessWorkingDirectory = Char.ToUpper(Environment.CurrentDirectory[0]) + Environment.CurrentDirectory.Substring(1); // we don't intended to manipulate this, so store it's initial value for speed
		public static readonly string TempPath = Path.GetTempPath(); // This call is quite expensive and we don't expect it to change during the life time of the process
		public static readonly ReadOnlyDictionary<string, string> EnvironmentVariables; // much faster to store our own map of environment variables

		public static Dictionary<string, string> InitialCommandLineProperties = new Dictionary<string, string>(); // initial properties specified on command line - used in nant task propagation // TODO technically should be readonly
		public static readonly GlobalPropertyBag GlobalProperties = new GlobalPropertyBag();
		public static readonly ISet<Guid> GlobalNamedObjects = new HashSet<Guid>(); // Id's of objects that are propagated to dependents

		private GlobalProjectState m_projectState; // propagate to subproject context so that certain state information is shared
		private TargetStyleType m_targetStyle = TargetStyleType.Use;

		private static readonly Dictionary<string, string> s_envKeys;

		// so this horribleness exists to deal with the ability to convert <target> style build steps
		// into build steps that invoke nant in generated build files (such as vs projects , see
		// NantInvocationProperties.cs for more info)
		// those need to propagate certain pieces of information on the command line to preserve
		// context however making them readonly as is default disables certain abilities like
		// resetting eaconfig.build.group
		private static readonly string[] s_internalControlProperties = new string[]
		{
			"build.buildtype",
			"build.buildtype.base",
			"build.module",
			"build.outputname",
			"config",
			"eaconfig.build.group",
			"groupname",
			"package.configs"
		};

		static Project()
		{
			// scan up to find a manifest file, if we don't just use assembly location,
			// this is mainly used for unit tests
			string nantLocation = PathUtil.GetExecutingAssemblyLocation();
			DirectoryInfo fwDirectory = new DirectoryInfo(nantLocation);
			while (true)
			{
				fwDirectory = fwDirectory.Parent;
				if (fwDirectory == null)
				{
					break;
				}
				if (File.Exists(Path.Combine(fwDirectory.FullName, "Manifest.xml")))
				{
					nantLocation = Path.Combine(fwDirectory.FullName, "bin");
					break;
				}
			}

			NantLocation = nantLocation;
			NantLocation = Char.ToUpper(NantLocation[0]) + NantLocation.Substring(1);

			NantDrive = PathUtil.GetAssemblyRootDir(NantLocation);
			NantDrive = Char.ToUpper(NantDrive[0]) + NantDrive.Substring(1);

			NantVersion = NAntUtil.NantVersionString;

			AddDefaultGlobalProperties();

			// capture environment variables
			{
				// Note that Environment.GetEnvironmentVariables() can return duplicate entries (a collection from
				// machine space, user space, and launch process inheritance).  If we encounter duplicate, retrieve
				// the value again using Environment.GetEnvironmentVariable(var) to get the "normal" default value.
				IDictionary envVars = Environment.GetEnvironmentVariables();

				// using loops here in case we have a double entry / mismatched case entry
				s_envKeys = new Dictionary<string, string>(comparer: PlatformUtil.IsWindows ? StringComparer.OrdinalIgnoreCase : StringComparer.Ordinal);
				foreach (DictionaryEntry entry in envVars.Cast<DictionaryEntry>())
				{
					s_envKeys[(string)entry.Key] = (string)entry.Key;
				}
				Dictionary<string, string> environmentVariables = new Dictionary<string, string>(comparer: PlatformUtil.IsWindows ? StringComparer.OrdinalIgnoreCase : StringComparer.Ordinal);
				foreach (DictionaryEntry entry in envVars.Cast<DictionaryEntry>())
				{
					if (environmentVariables.ContainsKey((string)entry.Key))
						// If we have duplicate, use Environment.GetEnvironmentVariable() and retrieve the value again.
						// The return from that function is the proper value that we should use.
						environmentVariables[(string)entry.Key] = Environment.GetEnvironmentVariable((string)entry.Key);
					else
						environmentVariables[(string)entry.Key] = (string)entry.Value;
				};

				// always make sure PATH is set to at least empty string
				if (!environmentVariables.ContainsKey("PATH"))
				{
					environmentVariables.Add("PATH", String.Empty);
					s_envKeys.Add("PATH", "PATH");
				}

				// if system root is missing or empty string, try and replace it with registry value - else ensure it is at least set to empty string
				if (PlatformUtil.IsWindows)
				{
					if (environmentVariables.TryGetValue("SYSTEMROOT", out string systemRoot))
					{
						if (String.IsNullOrEmpty(systemRoot))
						{
							string preserveCasing = s_envKeys["SYSTEMROOT"];
							environmentVariables[preserveCasing] = RegistryUtil.GetRegistryValue(RegistryHive.LocalMachine, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", "PathName") ?? String.Empty;
						}
					}
					else
					{
						s_envKeys["SYSTEMROOT"] = "SYSTEMROOT";
						environmentVariables["SYSTEMROOT"] = RegistryUtil.GetRegistryValue(RegistryHive.LocalMachine, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", "PathName") ?? String.Empty;
					}
				}

				EnvironmentVariables = new ReadOnlyDictionary<string, string>(environmentVariables);
			}
		}

		// Constructs a project without any file context - useful for unit tests or applications that need a project
		// context without a build file
		public Project(Log log, Dictionary<string, string> commandLineProperties = null)
			: this
			(
				log: log,
				functionFactory: new FunctionFactory(),
				topLevel: false,
				commandLineProperties: commandLineProperties
			)
		{
			BuildTargetNames = new ReadOnlyCollection<string>(new List<string>());
			Properties.UpdateReadOnly(NANT_PROPERTY_PROJECT_CMDTARGETS, String.Empty);
		}

		// Constructs a new project with the given build file path.
		public Project(Log log, string buildFile, IEnumerable<string> buildTargets = null, Project parent = null, bool topLevel = true, Dictionary<string, string> commandLineProperties = null)
			: this
			(
				log: log,
				functionFactory: parent != null ? new FunctionFactory(parent.FuncFactory) : new FunctionFactory(),
				topLevel: topLevel,
				commandLineProperties: commandLineProperties
			)
		{
			ParentProject = parent;

			buildFile = Path.GetFullPath(buildFile);
			buildFile = Char.ToUpper(buildFile[0]) + buildFile.Substring(1);

			BaseDirectory = Path.GetDirectoryName(buildFile);

			BuildFileLocalName = buildFile;
			CurrentScriptFile = buildFile;

			Properties.AddReadOnly(NANT_PROPERTY_PROJECT_BASEDIR, BaseDirectory, local: true, inheritable: true);
			Properties.AddReadOnly(NANT_PROPERTY_PROJECT_BUILDFILE, BuildFileLocalName);

			// check to make sure that the root element in named correctly
			LineInfoDocument buildFileDocument = LineInfoDocument.Load(buildFile);
			if (!buildFileDocument.DocumentElement.Name.Equals("project"))
			{
				throw new BuildException($"Root element must be named 'project' in '{BuildFileLocalName}'.");
			}

			// set project fields from project file
			string defaultTargetName = null;
			if (buildFileDocument.DocumentElement.HasAttribute("name"))
			{
				ProjectName = buildFileDocument.DocumentElement.GetAttribute("name");
				Properties.AddReadOnly(NANT_PROPERTY_PROJECT_NAME, ProjectName);
			}
			if (buildFileDocument.DocumentElement.HasAttribute("default"))
			{
				defaultTargetName = buildFileDocument.DocumentElement.GetAttribute("default");
				Properties.AddReadOnly(NANT_PROPERTY_PROJECT_DEFAULT, defaultTargetName);
			}

			// determine build targets
			List<string> finalBuildTargets = new List<string>();
			if (buildTargets != null && buildTargets.Any())
			{
				finalBuildTargets.AddRange(buildTargets);
			}
			else if (defaultTargetName != null)
			{
				finalBuildTargets.Add(defaultTargetName);
			}
			BuildTargetNames = new ReadOnlyCollection<string>(finalBuildTargets);
			Properties.UpdateReadOnly(NANT_PROPERTY_PROJECT_CMDTARGETS, String.Join(" ", BuildTargetNames));

			// finally init configuration properties for selected config (if we have a ${config})
			ConfigPackageLoader.InitializeInitialConfigurationProperties(this);
		}

		// constructor for include scope projects - used when including a file in another project
		public Project(Project parent, Log log, string includeFile, TargetStyleType forceStyle)
			: this
			(
				parent: parent,
				scriptFile: includeFile,
				baseDirectory: Path.GetDirectoryName(includeFile),
				log: log
			)
		{
			m_targetStyle = forceStyle;
		}

		// constructor for custom logging scope
		public Project(Project parent, Log log)
			: this
			(
				parent: parent,
				localPropagation: LocalProperitesPropagation.Share, // we're only changing logging, local properties should be shared with parent
				log: log
			)
		{
		}

		// constructor for target scope
		public Project(Project parent, Target target, TargetStyleType styleType)
			: this
			(
				parent: parent,
				baseDirectory: target.BaseDirectory // we want paths executed in the target to resolve relative to the target's base dir, not the calling projects
			)
		{
			m_targetStyle = styleType;
		}

		// constructor for parallel scope or task scope, used when we want to set some local properties which are different for each (parallel) execution in a single project
		public Project(Project parent, object scriptInitLock = null)
			: this
			(
				parent: parent,
				localPropagation: LocalProperitesPropagation.InheritAll, // all local properties are available in parallel block but each parallel execution maintains it's own set of local
				scriptInitLock: scriptInitLock
			)
		{
		}

		// base constructor for new projects
		private Project(Log log, FunctionFactory functionFactory, bool topLevel, Dictionary<string, string> commandLineProperties = null)
		{
			Log = log ?? throw new ArgumentNullException(nameof(log), "Project cannot be initialized without a Log instance.");
			LogPrefix = $"[nant {NantVersion}] ";

			IncludedFiles = new IncludedFilesCollection();

			TaskFactory = new TaskFactory();
			FuncFactory = functionFactory;

			NamedFileSets = new FileSetDictionary();
			NamedObjects = new ObjectDictionary();
			NamedOptionSets = new OptionSetDictionary();

			// setup default properties for new projects
			{
				Properties = new PropertyDictionary(this);

				// add nant.* properties
				Assembly assembly = Assembly.GetExecutingAssembly();

				Properties.AddReadOnly(NANT_PROPERTY_CONTINUEONFAIL, "false");
				Properties.AddReadOnly(NANT_PROPERTY_VERSION, NantVersion);

				Properties.AddReadOnly("nant.drive", NantDrive);
				Properties.AddReadOnly("nant.endline", Environment.NewLine);
				Properties.AddReadOnly("nant.framework3", "true");
				Properties.AddReadOnly("nant.generate-dependency-only-supported", "true");
				Properties.AddReadOnly("nant.issigned", (assembly.GetName().GetPublicKey().Length > 0 ? "true" : "false"));
				Properties.AddReadOnly("nant.location", NantLocation);
				Properties.AddReadOnly("nant.platform_host", PlatformUtil.Platform.ToLowerInvariant());
				Properties.AddReadOnly("nant.quickrun", "false");
				Properties.AddReadOnly("nant.supportchannel", CMProperties.CMSupportChannel);
				Properties.AddReadOnly("nant.verbose", (Log.Level >= Log.LogLevel.Detailed).ToString().ToLowerInvariant());
				Properties.AddReadOnly("nant.supports-sndbs-rewrite-rules", "true");

				Properties.AddReadOnly("framework-new-config-system", "true"); // some packages still assume this is false if unset for FW7 compatibility so force set true here

				string prepandMono = String.Empty;
				if (PlatformUtil.IsMonoRuntime)
				{
					if (PlatformUtil.IsOSX)
					{
						// On OSX, the user PATH environment seems to got lost when running under Xcode's shell script run phase
						// and we need to explicitly provide full path to mono which is installed to a standard location
						prepandMono = "/Library/Frameworks/Mono.framework/Versions/Current/Commands/mono ";
					}
					else
					{
						prepandMono = "mono ";
					}
				}

				// add nant.* tools properties - 32 properties are legacy, tools are compiled to anycpu code
				Properties.AddReadOnly(NANT_PROPERTY_COPY, prepandMono + Path.Combine(NantLocation, "copy.exe"));
				Properties.AddReadOnly(NANT_PROPERTY_COPY + "_32", prepandMono + Path.Combine(NantLocation, "copy.exe"));
				Properties.AddReadOnly(NANT_PROPERTY_MKDIR, prepandMono + Path.Combine(NantLocation, "mkdir.exe"));
				Properties.AddReadOnly(NANT_PROPERTY_MKDIR + "_32", prepandMono + Path.Combine(NantLocation, "mkdir.exe"));
				Properties.AddReadOnly(NANT_PROPERTY_RMDIR, prepandMono + Path.Combine(NantLocation, "rmdir.exe"));
				Properties.AddReadOnly(NANT_PROPERTY_RMDIR + "_32", prepandMono + Path.Combine(NantLocation, "rmdir.exe"));

				// add system info properties
				Properties.AddReadOnly("sys.clr.version", Environment.Version.ToString());
				Properties.AddReadOnly("sys.os", Environment.OSVersion.ToString());
				Properties.AddReadOnly("sys.os.folder.system", Environment.GetFolderPath(Environment.SpecialFolder.System));
				Properties.AddReadOnly("sys.os.folder.temp", TempPath);
				Properties.AddReadOnly("sys.os.platform", Environment.OSVersion.Platform.ToString());
				Properties.AddReadOnly("sys.os.version", Environment.OSVersion.Version.ToString());

				// Add the current local ip address(es), in case people need to use them when calling external applications, e.g. xbapp.exe
				IPAddress[] addresses = IPUtil.GetUserIPAddresses();
				Properties.AddReadOnly("sys.localnetworkaddress", addresses.Any() ? addresses.First().ToString() : "");
				Properties.AddReadOnly("sys.localnetworkaddresses", addresses.Any() ? String.Join(" ", addresses.Select(x => x.ToString())) : "");

				// dll property - make sure this is always defined so that packages can rely on it existing in any config
				// not readonly however so that it can be overidden by config
				Properties.Add("Dll", "false");

				// add eaconfig.* properties that Framework relies on being set  
				Properties.Add("eaconfig.runtime.groupname", "runtime");
				Properties.Add("eaconfig.runtime.outputfolder", String.Empty);
				Properties.Add("eaconfig.runtime.sourcedir", "source");
				Properties.Add("eaconfig.runtime.usepackagelibs", "false");

				Properties.Add("eaconfig.tool.groupname", "tool");
				Properties.Add("eaconfig.tool.outputfolder", "/tool");
				Properties.Add("eaconfig.tool.sourcedir", "tools");
				Properties.Add("eaconfig.tool.usepackagelibs", "false");

				Properties.Add("eaconfig.test.groupname", "test");
				Properties.Add("eaconfig.test.outputfolder", "/test");
				Properties.Add("eaconfig.test.sourcedir", "tests");
				Properties.Add("eaconfig.test.usepackagelibs", "true");

				Properties.Add("eaconfig.example.groupname", "example");
				Properties.Add("eaconfig.example.outputfolder", "/example");
				Properties.Add("eaconfig.example.sourcedir", "examples");
				Properties.Add("eaconfig.example.usepackagelibs", "true");
				
				if (PlatformUtil.IsWindows)
				{
					Properties.AddReadOnly(NANT_PROPERTY_ENV_PATH_SEPARATOR, ";");
				}
				else if (PlatformUtil.IsOSX || PlatformUtil.IsUnix)
				{
					Properties.AddReadOnly(NANT_PROPERTY_ENV_PATH_SEPARATOR, ":");
				}
				else
				{
					Properties.AddReadOnly(NANT_PROPERTY_ENV_PATH_SEPARATOR, ";");
				}

				Properties.AddReadOnly(NANT_PROPERTY_WARN_LEVEL, Enum.GetName(typeof(Log.WarnLevel), Log.WarningLevel).ToLowerInvariant());
				Properties.AddReadOnly(NANT_PROPERTY_LOG_LEVEL, Enum.GetName(typeof(Log.LogLevel), Log.Level).ToLowerInvariant());
				Properties.AddReadOnly(NANT_PROPERTY_WARN_LEVEL_NUM, ((int)Log.WarningLevel).ToString());
				Properties.AddReadOnly(NANT_PROPERTY_LOG_LEVEL_NUM, ((int)Log.Level).ToString());
				Properties.AddReadOnly(NANT_PROPERTY_PROJECT_ISTOPLEVEL, topLevel.ToString());

				// add all environment variables as properties
				// create property versions of the environment variables
				foreach (KeyValuePair<string, string> kvp in EnvironmentVariables)
				{
					if (Property.VerifyPropertyName(kvp.Key))
					{
						Properties[$"sys.env.{kvp.Key}"] = kvp.Value;
					}
					else if (Log.DebugEnabled)
					{
						Log.Debug.WriteLine(LogPrefix + "Cannot create property for '{0}'.", kvp.Key);
					}
				}

				// Most processes on Windows will often require SystemRoot, TMP abd ProgramData env variables to properties
				// automatically propagated to exec and build related processes
				if (PlatformUtil.IsWindows)
				{
					Properties.Add("build.env.TMP", EnvironmentVariables.GetValueOrDefault("TMP", String.Empty));
					Properties.Add("exec.env.TMP", EnvironmentVariables.GetValueOrDefault("TMP", String.Empty));

					Properties.Add("build.env.TEMP", EnvironmentVariables.GetValueOrDefault("TEMP", String.Empty));
					Properties.Add("exec.env.TEMP", EnvironmentVariables.GetValueOrDefault("TEMP", String.Empty));

					// use original casing to propagate SystemRoot and SystenDrive (systemroot is guaranteed to be set)
					Properties.Add("build.env." + s_envKeys["SystemRoot"], EnvironmentVariables["SystemRoot"]);
					Properties.Add("exec.env." + s_envKeys["SystemRoot"], EnvironmentVariables["SystemRoot"]);
					Properties.Add("build.env." + s_envKeys["SystemDrive"], EnvironmentVariables["SystemDrive"]);
					Properties.Add("exec.env." + s_envKeys["SystemDrive"], EnvironmentVariables["SystemDrive"]);

					// us original casing to propagate ProgramData (programdata is NOT guaranteed to be set)
					string originalProgamDataCasing = s_envKeys.GetValueOrDefault("ProgramData", "ProgramData");
					string programDataValue = EnvironmentVariables.GetValueOrDefault("ProgramData", String.Empty);
					Properties.Add("build.env." + originalProgamDataCasing, programDataValue);
					Properties.Add("exec.env." + originalProgamDataCasing, programDataValue);

					Properties.Add("build.env.PROCESSOR_ARCHITECTURE", EnvironmentVariables.GetValueOrDefault("PROCESSOR_ARCHITECTURE", String.Empty));
					Properties.Add("exec.env.PROCESSOR_ARCHITECTURE", EnvironmentVariables.GetValueOrDefault("PROCESSOR_ARCHITECTURE", String.Empty));

					// WINDIR is required so managed C++ code can be compiled with the /unsafe
					Properties.Add("build.env.WINDIR", EnvironmentVariables.GetValueOrDefault("WINDIR", String.Empty));
					Properties.Add("exec.env.WINDIR", EnvironmentVariables.GetValueOrDefault("WINDIR", String.Empty));
				}

				// initialize and properties that were passed on command line
				{
					// local and global command line names are legacy - they actually refer to the same optionset
					OptionSet commandLinePropertiesOptionset = new OptionSet
					{
						Project = this
					};
					CommandLineProperties = commandLineProperties != null ? // outside of Framework / in tests it is common to create projects without any initial properties so this is allowed to be null
						new Dictionary<string, string>(commandLineProperties) : 
						new Dictionary<string, string>();

					foreach (KeyValuePair<string, string> prop in CommandLineProperties)
					{
						Properties.Add(prop.Key, prop.Value, readOnly: !IsControlProperty(prop.Key)); // control properties are never readonly
						commandLinePropertiesOptionset.Options.Add(prop.Key, prop.Value);
					}
					NamedOptionSets.Add(NANT_OPTIONSET_PROJECT_CMDPROPERTIES, commandLinePropertiesOptionset);
					NamedOptionSets.Add(NANT_OPTIONSET_PROJECT_GLOBAL_CMDPROPERTIES, commandLinePropertiesOptionset);
				}

				if (PackageMap.IsInitialized)
				{
					// properties from masterconfig, will be empty if package map 
					foreach (GlobalPropertyDef propdef in GlobalProperties.EvaluateExceptions(this))
					{
						if (propdef.InitialValue != null && !Properties.Contains(propdef.Name)) // don't overwrite properties already set (such as from command line)
						{
							Properties.Add(propdef.Name, propdef.InitialValue); // masterconfig properties are not initially set as read-only even if they have an initial value
						}
					}
					PostPackageMapInit();
				}
			}

			Partials = new List<object>();
			Targets = new TargetCollection(Log);
			UserTasks = new ConcurrentDictionary<string, XmlNode>();

			PackageInitLock = new Object();
			ScriptInitLock = new Object();
			BuildSyncRoot = new Object();

			m_projectState = new GlobalProjectState();
		}

		// base construct for "sub project contexts" these are scoped project which share most fields with their
		// parent but may have their own local properties, base directory, log, style, etc
		private Project(Project parent, LocalProperitesPropagation localPropagation = LocalProperitesPropagation.InheritableOnly, string scriptFile = null, string baseDirectory = null, Log log = null, Object scriptInitLock = null)
		{
			ProjectName = parent.ProjectName;
			ParentProject = parent;  // need to keep this to propagate package properties. See comment in NAnt task

			Log = log ?? parent.Log;
			LogPrefix = $"[nant {NantVersion}]";

			IncludedFiles = parent.IncludedFiles;

			FuncFactory = parent.FuncFactory;
			TaskFactory = parent.TaskFactory;

			// initialize properties
			if (localPropagation == LocalProperitesPropagation.Share)
			{
				Properties = parent.Properties;
			}
			else
			{
				Properties = new PropertyDictionary(this, parent.Properties);
				foreach (Property localProperty in parent.Properties.GetLocalProperties())
				{
					if (localProperty.Inheritable || localPropagation == LocalProperitesPropagation.InheritAll)
					{
						Properties.Add(localProperty.Key, localProperty.Value, localProperty.ReadOnly, localProperty.Deferred, local: true, inheritable: localProperty.Inheritable);
					}
				}
			}

			// if log is being overriden update warning suppression legacy properties
			if (Log != parent.Log)
			{
				Log.ApplyLegacyProjectSuppressionProperties(this);
			}

			NamedObjects = parent.NamedObjects;
			Partials = parent.Partials;
			NamedOptionSets = parent.NamedOptionSets;
			NamedFileSets = parent.NamedFileSets;
			UserTasks = parent.UserTasks;
			Targets = parent.Targets;
			CommandLineProperties = parent.CommandLineProperties;

			PackageInitLock = parent.PackageInitLock;
			ScriptInitLock = scriptInitLock ?? parent.ScriptInitLock;
			BuildSyncRoot = parent.BuildSyncRoot;

			CurrentScriptFile = scriptFile ?? parent.CurrentScriptFile;
			BuildFileLocalName = parent.BuildFileLocalName;
			BaseDirectory = baseDirectory ?? parent.BaseDirectory;
			if (baseDirectory != parent.BaseDirectory)
			{
				// if base directory was changed update this project's copy (it's local inheritable property)
				Properties.UpdateReadOnly(NANT_PROPERTY_PROJECT_BASEDIR, BaseDirectory, local: true, inheritable: true);
			}

			BuildTargetNames = parent.BuildTargetNames;

			m_projectState = parent.m_projectState;
			m_targetStyle = parent.m_targetStyle;
		}

		// Clone constructor. Make distinct signature by adding parameter
		private Project(Project other, Project clone)
		{
			ScriptInitLock = new Object();
			PackageInitLock = new Object();
			BuildSyncRoot = other.BuildSyncRoot;
			Log = other.Log;
			LogPrefix = other.LogPrefix;

			// Initialize properties
			Properties = new PropertyDictionary(this, other);

			ParentProject = other.ParentProject;  // need to keep this to propagate package properties. See comment in NAnt task

			BaseDirectory = other.BaseDirectory;
			BuildTargetNames = other.BuildTargetNames;
			FuncFactory = other.FuncFactory;
			TaskFactory = other.TaskFactory;

			IncludedFiles = new IncludedFilesCollection(other.IncludedFiles);
			Targets = new TargetCollection(Log, other.Targets);
			NamedObjects = new ObjectDictionary(other.NamedObjects as ConcurrentDictionary<Guid, object>);
			Partials = new List<object>(other.Partials);
			NamedOptionSets = new OptionSetDictionary(other.NamedOptionSets);
			NamedFileSets = new FileSetDictionary(other.NamedFileSets);
			UserTasks = new ConcurrentDictionary<string, XmlNode>(other.UserTasks);
			CommandLineProperties = new Dictionary<string, string>(other.CommandLineProperties);

			CurrentScriptFile = other.CurrentScriptFile;

			BuildFileLocalName = other.BuildFileLocalName;

			ProjectName = other.ProjectName;

			m_projectState = other.m_projectState;
			m_targetStyle = other.m_targetStyle;
		}

		public Project Clone()
		{
			// TODO this is only used by nant build to run multiple build in a single (but cloned) project in parallel, if we can kill nant build then we can kill project cloning and
			// a whole bunch of code for cleanly cloning project internals
			return new Project(this, this);
		}

		public bool Run()
		{
			DateTime startTime = DateTime.Now;
#if DEBUG
			if (!Nested)
			{
				Log.Minimal.WriteLine(LogPrefix + "NOTE: This is a DEBUG build of NAnt and may be slower than an optimized build!");
			}
#endif
			try
			{
				Log.Debug.WriteLine(LogPrefix + BuildFileLocalName);

				bool ok = false;
				try
				{
					RunScript();

					if (!BuildTargetNames.Any())
					{
						Log.Minimal.WriteLine(LogPrefix + "No targets specified.");
					}
					else
					{
						foreach (string targetName in BuildTargetNames)
						{
							if (CancelBuildTargetExecution)
							{
								break;
							}
							Properties.UpdateReadOnly(NANT_PROPERTY_PROJECT_MASTERTARGET, targetName);
							Execute(targetName);
						}
					}
					ok = true;
				}
				catch (ContextCarryingException e)
				{
					e.PushNAntStackFrame(string.Empty.Equals(ProjectName) ? "project" : ProjectName, NAntStackScopeType.Project, new Location(BuildFileLocalName, 0, 0));
					throw e; // reset stack frame
				}
				finally
				{
					string endTargetName = Properties[ok ? NANT_PROPERTY_ONSUCCESS : NANT_PROPERTY_ONFAILURE];
					if (!String.IsNullOrEmpty(endTargetName))
					{
						Properties.UpdateReadOnly(NANT_PROPERTY_PROJECT_MASTERTARGET, endTargetName);
						Execute(endTargetName);
					}
				}

				if (!Nested)
				{
					TimeSpan timeSpan = DateTime.Now.Subtract(startTime);
					if (GlobalSuccess)
					{
						Log.Minimal.WriteLine(LogPrefix + "BUILD SUCCEEDED ({0:D2}:{1:D2}:{2:D2})", timeSpan.Hours, timeSpan.Minutes, timeSpan.Seconds);
					}
					else
					{
						Log.Error.WriteLine(LogPrefix + "BUILD FAILED ({0:D2}:{1:D2}:{2:D2})", timeSpan.Hours, timeSpan.Minutes, timeSpan.Seconds);
					}
#if DEBUG
					Log.Minimal.WriteLine(LogPrefix + "NOTE: This is a DEBUG build of NAnt and above time may be slower than an optimized build!");
#endif
				}

				return GlobalSuccess;
			}
			catch (BuildException e)
			{
				TimeSpan timeSpan = DateTime.Now.Subtract(startTime);
				using (BufferedLog errorMessage = new BufferedLog(Log))
				{
					errorMessage.Error.WriteLine();
					errorMessage.IndentLevel += 1;

					Exception current = e;
					string offset = "";
					while (current != null)
					{
						if (!(current is BuildExceptionStackTraceSaver))
						{
							if (current is BuildException && ((BuildException)current).Location != Location.UnknownLocation)
							{
								errorMessage.Error.WriteLine("{0}error at {1}", offset, ((BuildException)current).Location.ToString());
							}
							foreach (string line in current.Message.LinesToArray())
							{
								errorMessage.Error.WriteLine(offset + line);
							}
							offset += "  ";
						}
						current = current.InnerException;
					}
					errorMessage.IndentLevel -= 1;

					errorMessage.Error.WriteLine();
					errorMessage.Error.WriteLine(LogPrefix + "BUILD FAILED ({0:D2}:{1:D2}:{2:D2})" + Environment.NewLine, timeSpan.Hours, timeSpan.Minutes, timeSpan.Seconds);
				}

				return false;
			}
			catch (Exception e)
			{
				TimeSpan timeSpan = DateTime.Now.Subtract(startTime);
				using (BufferedLog errorMessage = new BufferedLog(Log))
				{
					errorMessage.Error.WriteLine();
					errorMessage.IndentLevel += 2;
					while (e != null && e is ExceptionStackTraceSaver)
					{
						e = e.InnerException;
					}
					if (e != null)
					{
						var cce = ContextCarryingException.GetLastContextCarryingExceptions(e);
						if (cce != null)
						{
							errorMessage.Error.WriteLine("error at {0} {1}  {2}", cce.SourceTaskType, cce.SourceTask.Quote(), Properties[PackageProperties.ConfigNameProperty] ?? String.Empty);
						}
						else
						{
							errorMessage.Error.WriteLine("INTERNAL ERROR");
						}

						Exception current = e;
						string offset = "  ";
						while (current != null)
						{
							if (!(current is ContextCarryingException || current is BuildExceptionStackTraceSaver || current is ExceptionStackTraceSaver))
							{
								foreach (var line in current.Message.LinesToArray())
								{
									errorMessage.Error.WriteLine(offset + line);
								}
								offset += "  ";
							}
							current = current.InnerException;
						}

						IList<string> stackTraceLines = e.StackTrace.LinesToArray();
						if (stackTraceLines.Any())
						{
							errorMessage.Error.WriteLine();
							errorMessage.IndentLevel += 1;
							foreach (string line in stackTraceLines)
							{
								errorMessage.Error.WriteLine(line);
							}
							errorMessage.IndentLevel -= 1;
						}
					}

					errorMessage.IndentLevel -= 2;

					if (!Nested)
					{
						errorMessage.Error.WriteLine(LogPrefix + "BUILD FAILED ({0:D2}:{1:D2}:{2:D2})" + Environment.NewLine, timeSpan.Hours, timeSpan.Minutes, timeSpan.Seconds);
					}
				}

				CoreDump(e, BaseDirectory + "\\CoreDump.xml"); // TODO, should probably ditch CoreDump

				return false;
			}
		}

		[Obsolete("Project objects do not need to be disposed.")]
		public void Dispose()
		{
			Log.ThrowDeprecation(Log.DeprecationId.ProjectDisposal, Log.DeprecateLevel.Normal, DeprecationUtil.GetCallSiteKey(), "{0} Project objects do not need to be disposed.", DeprecationUtil.GetCallSiteLocation());
		}

		[Obsolete]
		public void OnBuildStarted(ProjectEventArgs e) { DeprecatedEventWarning("Calling OnBuildStarted"); }
		public void OnBuildFinished(ProjectEventArgs e) { DeprecatedEventWarning("Calling OnBuildFinished"); }
		public void OnTargetFinished(TargetEventArgs e) { DeprecatedEventWarning("Calling OnTargetFinished"); }
		public void OnTargetStarted(TargetEventArgs e) { DeprecatedEventWarning("Calling OnTargetStarted"); }
		public void OnTaskFinished(TaskEventArgs e) { DeprecatedEventWarning("Calling OnTaskFinished"); }
		public void OnTaskStarted(TaskEventArgs e) { DeprecatedEventWarning("Calling OnTaskStarted"); }
		public void OnUserTaskFinished(TaskEventArgs e) { DeprecatedEventWarning("Calling OnUserTaskFinished"); }
		public void OnUserTaskStarted(TaskEventArgs e) { DeprecatedEventWarning("Calling OnUserTaskStarted"); }

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

		/// <summary>Executes a specific target, and only that target.</summary>
		/// <param name="targetName">target name to execute.</param>
		/// <param name="force"></param>
		/// <param name="failIfTargetDoesNotExist">Fail build if target does not exist</param>
		/// <remarks>
		///   <para>Only the target is executed. No global tasks are executed.</para>
		/// </remarks>
		public bool Execute(string targetName, bool force = false, bool failIfTargetDoesNotExist = true)
		{
			if (!String.IsNullOrEmpty(targetName))
			{
				if (Targets.TryGetValue(targetName, out Target target))
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
				else if (failIfTargetDoesNotExist)
				{
					IEnumerable<string> closestTargetMatches = Targets.FindClosestMatches(targetName);

					StringBuilder errmsg = new StringBuilder(String.Format("Unknown target '{0}'.", targetName));
					if (closestTargetMatches.Any())
					{
						errmsg.AppendLine();
						errmsg.Append("Closest target name matches:");
						errmsg.AppendLine(string.Join("", closestTargetMatches.Select(m => Environment.NewLine + "\t" + m)));
					}
					throw new BuildException(errmsg.ToString());
				}
			}
			else if (failIfTargetDoesNotExist)
			{
				throw new BuildException("Project.Execute(targetname): targetname parameter is null or empty");
			}
			return false;
		}

		public void IncludeBuildFileDocument(string fileName, Location location)
		{
			IncludeBuildFileDocument(fileName, TargetStyle, null, location);
		}

		// this overload is specifically used when including from dependent task - allows target style to be overridden,
		// and warnings to be set based on the package we are including initialize.xml from rather than ourselves
		public void IncludeBuildFileDocument(string fileName, TargetStyleType forceStyle, string packageName, Location location)
		{
			if (IncludedFiles.Contains(fileName))
			{
				return;
			}

			if (Log.DebugEnabled)
			{
				Log.Debug.WriteLine(LogPrefix + fileName);
			}

			// creating a new log in order to reset the warning / deprecation state
			Log nestedIncludeLog = new Log(Log.Level, Log.WarningLevel, Log.WarningAsErrorLevel, Log.DeprecationLevel, Log.IndentLevel, Log.Id, Log.Listeners, Log.ErrorListeners, Log.DeveloperWarningEnabled);
			Project fileLocalContextProject = new Project(this, nestedIncludeLog, fileName, forceStyle);
			{
				// use package specific warning settings for package we are including file from, not the one we are currently
				// including *into*
				if (packageName != null)
				{
					fileLocalContextProject.Log.ApplyLegacyPackageSuppressionProperties(fileLocalContextProject, packageName);
					fileLocalContextProject.Log.ApplyPackageSuppressions(fileLocalContextProject, packageName);
				}

				fileLocalContextProject.RunScript(CurrentScriptFile);
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

		public string GetPropertyValue(PropertyKey key, bool expand = true)
		{
			string result = Properties[key];
			if (expand && result != null)
			{
				result = ExpandProperties(result);
			}
			return result;
		}

		public Task CreateTask(XmlNode taskNode, object parent = null)
		{
			Task task = TaskFactory.CreateTask(taskNode, this);
			if (!(parent == null || parent is Target || parent is Task || parent is Project))
			{
				// ideally we'd do this check at compile time by providing some overloads but that breaks backwards compatbility
				// with existing calls .CreateTask(node, null) because they become ambiguous. Deprecating this function and creating
				// a new function with a new name would solve this but is more churn that this validation is probably worth

				// note about accepting project: this is only done in the Project class and no code I've discovered actually uses .Parent
				// as if it's project - it simply exists here because it does no harm to keep accepting this
				// note about accepting null: it never really makes sense to accept null (if we're accepting Project for top level task)
				// but third party taskdefs do this and again, deprecating is probably more churn than it's worth
				throw new BuildException($"Valid parent types for tasks are {nameof(Target)}, {nameof(Task)} or {nameof(Project)}. Passed type was '{parent.GetType().ToString()}'.");
			}
			task.Parent = parent;
			task.XmlNode = taskNode;
			task.Initialize(taskNode);
			return task;
		}

		// return a path relative to the project's base directory
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

		public void RecomputeGlobalPropertyExceptions()
		{
			// set global properties from masterconfig based on current project (for conditionals)
			foreach (GlobalPropertyDef propDef in GlobalProperties.EvaluateExceptions(this))
			{
				if (!String.IsNullOrEmpty(propDef.Condition) && propDef.InitialValue != null && !CommandLineProperties.ContainsKey(propDef.Name))
				{
					string oldVal = Properties[propDef.Name];
					if (oldVal == null)
					{
						Properties.Add(propDef.Name, propDef.InitialValue, readOnly: false, deferred: false, local: false);
						Log.Info.WriteLine($"{LogPrefix}Added new writable global property '{propDef.Name}' with value '{propDef.InitialValue}' because '{propDef.Condition}' exception evaluation result changed after loading configuration package.");
					}
					else if (!String.Equals(Properties[propDef.Name], propDef.InitialValue, StringComparison.Ordinal))
					{
						if (Properties.IsPropertyReadOnly(propDef.Name))
						{
							Properties.UpdateReadOnly(propDef.Name, propDef.InitialValue);
							Log.Info.WriteLine(LogPrefix + "Updated readonly global property from '{0}' to '{1}' because '{2}' exception evaluation result changed after loading configuration package.", oldVal, propDef.InitialValue, propDef.Condition);
						}
						else
						{
							Properties[propDef.Name] = propDef.InitialValue;
							Log.Info.WriteLine(LogPrefix + "Updated writable global property from '{0}' to '{1}' because '{2}' exception evaluation result changed after loading configuration package.", oldVal, propDef.InitialValue, propDef.Condition);
						}
					}
				}
			}
		}

		public static bool SanitizeConfigString(string configString, out string remappedString)
		{
			remappedString = configString.Replace("-dev-", "-");
			if (RemapFrostbiteConfigs(remappedString, out string remappedConfigs))
			{
				remappedString = remappedConfigs;
			}
			return remappedString != configString;
		}

		public static void SanitizeConfigProperties(Log log, ref Dictionary<string, string> properties)
		{
			string unsanitizedConfigs = "";
			string sanitizedConfigs = "";
			string unsanitizedFbConfigs = "";
			string sanitizedFbConfigs = "";
			if (properties.TryGetValue("package.configs", out string packageDotConfigsValue))
			{
				if (packageDotConfigsValue.Contains("-dev-"))
				{
					unsanitizedConfigs += packageDotConfigsValue;
					packageDotConfigsValue = packageDotConfigsValue.Replace("-dev-", "-");
					sanitizedConfigs += packageDotConfigsValue;
					properties["package.configs"] = packageDotConfigsValue;
				}

				if (RemapFrostbiteConfigs(properties["package.configs"], out string remappedConfigs))
				{
					unsanitizedFbConfigs += properties["package.configs"];
					sanitizedFbConfigs += remappedConfigs;
					properties["package.configs"] = remappedConfigs;
				}
			}

			if (properties.TryGetValue("config", out string configValue))
			{
				if (configValue.Contains("-dev-"))
				{
					unsanitizedConfigs = unsanitizedConfigs + " " + configValue;
					configValue = configValue.Replace("-dev-", "-");
					sanitizedConfigs = sanitizedConfigs + " " + configValue;
					properties["config"] = configValue;
				}

				if (RemapFrostbiteConfigs(properties["config"], out string remappedConfigs))
				{
					unsanitizedFbConfigs = unsanitizedFbConfigs + " " + properties["config"];
					sanitizedFbConfigs = sanitizedFbConfigs + " " + remappedConfigs;
					properties["config"] = remappedConfigs;
				}
			}

			if (sanitizedConfigs.Length > 0)
			{
				log.Warning.WriteLine($"Some or all config(s) passed in via one or all of the 'package.configs' or 'config' properties or the 'config' parameter contain '-dev-' ({unsanitizedConfigs.Trim()}).\nFramework no longer supports '-dev-' in config names, please update your build.");
				log.Warning.WriteLine($"Automatically trying to use config(s): '{sanitizedConfigs.Trim()}'.");
			}

			if (sanitizedFbConfigs.Length > 0)
			{
				log.Warning.WriteLine($"Some or all config(s) passed in via one or all of the 'package.configs' or 'config' properties or the 'config' parameter contain a frostbite specific configuration ({unsanitizedFbConfigs.Trim()}).\nFramework no longer supports the frostbite specific configurations since the are now identical to the eaconfig versions, please update your build command line(s) to use the eaconfig versions of the configurations.");
				log.Warning.WriteLine($"Automatically trying to use config(s): '{sanitizedFbConfigs.Trim()}'.  If this fails you might have a custom config extension where you need to rename your config xml files (ex win64-vc-dll-opt.xml -> pc64-vc-dll-opt.xml)");
			}
		}

		// Function that a maps Frostbite -> eaconfig config names
		// Returns true if the newConfigs list can be used, if false the values in newConfigs should not be used.
		public static bool RemapFrostbiteConfigs(string configs, out string newConfigs)
		{
			if (!SanitizeFbConfigs)
			{
				newConfigs = null;
				return false;
			}
			
			newConfigs = configs.Replace("win64-", "pc64-")
									   .Replace("win32-", "pc-")
									   .Replace("xb1-", "capilano-")
									   .Replace("ps4-", "kettle-")
									   .Replace("linux64-", "unix64-");
									   
			if (newConfigs != configs)
			{
				return true;
			}

			return false;
		}

		public static bool IsControlProperty(string key)
		{
			if (PropertyKey.CaseSensitivity == PropertyKey.CaseSensitivityLevel.Insensitive || PropertyKey.CaseSensitivity == PropertyKey.CaseSensitivityLevel.InsensitiveWithTracking)
			{
				return s_internalControlProperties.Contains(key, StringComparer.OrdinalIgnoreCase);
			}
			return s_internalControlProperties.Contains(key);
		}

		// this should only be called after package map has been initalize - which in effect means it should be called in two
		// places:
		// * in Project constructor for all projects created after PackageMap init
		// * after PackageMap init for the global init project used to initialize PackageMap
		internal void PostPackageMapInit()
		{
			Properties.AddReadOnly(NANT_PROPERTY_ONDEMAND, PackageMap.Instance.OnDemand.ToString());

			if (!String.IsNullOrEmpty(PackageMap.Instance.MasterConfigFilePath))
			{
				Properties.AddReadOnly(NANT_PROPERTY_PROJECT_MASTERCONFIGFILE, PackageMap.Instance.MasterConfigFilePath);
			}
			if (!String.IsNullOrEmpty(PackageMap.Instance.BuildRoot))
			{
				Properties.AddReadOnly(NANT_PROPERTY_PROJECT_BUILDROOT, PackageMap.Instance.BuildRoot);
				Properties.AddReadOnly(NANT_PROPERTY_PROJECT_TEMPROOT, Path.Combine(PackageMap.Instance.BuildRoot, "framework_tmp"));
			}
			if (PackageMap.Instance.PackageRoots.HasOnDemandRoot)
			{
				Properties.AddReadOnly(NANT_PROPERTY_PROJECT_ONDEMAND_ROOT, PackageMap.Instance.PackageRoots.OnDemandRoot.FullName);
			}

			// set NANT_PROPERTY_PROJECT_PACKAGEROOTS, a multi-line property expected by eaconfig's package-external 
			// target to exclude package root folders. Example: tlapt_rw4 has a package root "contrib"
			// in its folder, and we don't want to package contrib.
			StringBuilder rootPropertyBuilder = new StringBuilder();
			foreach (PackageRootList.Root root in PackageMap.Instance.PackageRoots)
			{
				rootPropertyBuilder.AppendLine(root.Dir.FullName);
			}
			Properties.AddReadOnly(NANT_PROPERTY_PROJECT_PACKAGEROOTS, rootPropertyBuilder.ToString());

			// temporary toggle for new nuget, new csproj and dot net core, all will be removed and mandated true in future
			bool dotNetCoreSupport = Properties.GetBooleanPropertyOrDefault(NANT_PROPERTY_NET_CORE_SUPPORT, false, failOnNonBoolValue: true);
			bool useNugetResolve;
			{
				if (Properties.TryGetBooleanProperty(NANT_PROPERTY_USE_NUGET_RESOLUTION, out bool useNugetResolvePropertyValue, failOnNonBoolValue: true))
				{
					useNugetResolve = useNugetResolvePropertyValue;
					if (!useNugetResolve && dotNetCoreSupport)
					{
						throw new BuildException($"Nuget resolution cannot be disabled (property {NANT_PROPERTY_USE_NUGET_RESOLUTION}=false) if .NET Core support is enabled. Disable .NET Core support with property {NANT_PROPERTY_NET_CORE_SUPPORT}=false.");
					}
				}
				else
				{
					useNugetResolve = dotNetCoreSupport;
				}
			}
			bool useNewCsProj;
			{
				if (Properties.TryGetBooleanProperty(NANT_PROPERTY_USE_NEW_CSPROJ, out bool useNewCsProjPropertyValue, failOnNonBoolValue: true))
				{
					useNewCsProj = useNewCsProjPropertyValue;
					if (!useNewCsProj && dotNetCoreSupport)
					{
						throw new BuildException($"New .csproj format cannot be disabled (property {NANT_PROPERTY_USE_NEW_CSPROJ}=false) if .NET Core support is enabled. Disable .NET Core support with property {NANT_PROPERTY_NET_CORE_SUPPORT}=false.");
					}
				}
				else
				{
					useNewCsProj = dotNetCoreSupport;
				}
			}
			Properties.UpdateReadOnly(NANT_PROPERTY_NET_CORE_SUPPORT, dotNetCoreSupport.ToString());
			Properties.UpdateReadOnly(NANT_PROPERTY_USE_NUGET_RESOLUTION, useNugetResolve.ToString());
			Properties.UpdateReadOnly(NANT_PROPERTY_USE_NEW_CSPROJ, useNewCsProj.ToString());

			Log.ApplyLegacyProjectSuppressionProperties(this); // apply legacy properties to warning suppression now properties are all set up
		}

		// for testing only
		internal static void ResetGlobalProperties()
		{
			GlobalProperties.Clear();
			AddDefaultGlobalProperties();
		}

		// loads the current build file or include script into the project
		private void RunScript(string includedFrom = null)
		{
			if (includedFrom != null)
			{
				IncludedFiles.AddInclude(includedFrom, CurrentScriptFile);
			}
			else
			{
				IncludedFiles.AddInclude(CurrentScriptFile);
			}

			XmlDocument doc = LineInfoDocument.Load(CurrentScriptFile);
			foreach (XmlNode childNode in doc.DocumentElement.ChildNodes)
			{
				if (childNode.NodeType == XmlNodeType.Element && childNode.NamespaceURI.Equals(doc.DocumentElement.NamespaceURI))
				{
					// global tasks
					Task task = CreateTask(childNode, this);
					task.Execute();
				}
			}
		}

		private void DeprecatedEventWarning(string eventDescription)
		{
			Log.ThrowDeprecation(Log.DeprecationId.ProjectEvents, Log.DeprecateLevel.Minimal, DeprecationUtil.GetCallSiteKey(frameOffset: 1), 
				"{0}: {1} is deprecated and will be removed in a future Framework version. Event will have no effect.", 
				DeprecationUtil.GetCallSiteLocation(frameOffset: 1), eventDescription);
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

						new CoreDump(this).Write(textWriter);

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

		internal static void AddDefaultGlobalProperties()
		{
			GlobalProperties.Add("eaconfig.nantbuild");
			GlobalProperties.Add("eaconfig-extra-config-dir");
			GlobalProperties.Add("nant.postbuildcopylocal");
			GlobalProperties.Add("nant.warningsaserrors");
			GlobalProperties.Add("nant.warningsuppression.all");
			GlobalProperties.Add("ondemand.referencetracking");
			GlobalProperties.Add(PackageMap.UseTopLevelVersionProperty);
			GlobalProperties.Add(FrameworkProperties.AutoBuildUseDependencyPropertyName, initialValue: "true");
			GlobalProperties.Add(FrameworkProperties.BulkBuildExcludedPropertyName);
			GlobalProperties.Add(FrameworkProperties.BulkBuildPropertyName);
			GlobalProperties.Add(FrameworkProperties.CredStoreFilePropertyName);
			GlobalProperties.Add(NANT_PROPERTY_PROJECT_CMDTARGETS);
			GlobalProperties.Add(NANT_PROPERTY_TRANSITIVE, initialValue: true.ToString().ToLowerInvariant());
			GlobalProperties.Add(NANT_PROPERTY_NET_CORE_SUPPORT);
			GlobalProperties.Add(NANT_PROPERTY_USE_NUGET_RESOLUTION);
			GlobalProperties.Add(NANT_PROPERTY_USE_NEW_CSPROJ);
		}
	}
}