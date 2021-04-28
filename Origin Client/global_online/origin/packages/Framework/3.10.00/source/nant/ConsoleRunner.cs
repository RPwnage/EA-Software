using System;
using System.IO;
using System.Text;
using System.Linq;
using System.Reflection;
using System.Diagnostics;
using System.Threading;
using Concurrent = System.Threading.Tasks;
using System.Collections.Generic;
using System.Text.RegularExpressions;

using NAnt.Core;
using NAnt.Core.Logging;
using NAnt.Core.Util;
using NAnt.Core.PackageCore;
using NAnt.Core.Metrics;
using NAnt.Shared.Properties;

namespace NAnt.Console
{
    class ConsoleRunner : IDisposable
    {
#if PROFILE
        private Stopwatch sw = new Stopwatch();
#endif
        private ConsoleRunner()
        {
#if PROFILE
            sw.Start();
#endif
            // add the console event handler if we are on Windows
            if (PlatformUtil.IsWindows)
            {
                _consoleControl = new ConsoleCtrl();
                _consoleControl.ControlEvent += new ConsoleCtrl.ControlEventHandler(ConsoleEventHandler);
            }
        }

        private bool _disposed = false;
        private static ConsoleCtrl _consoleControl;


        private static int Main(string[] args)
        {
            PreloadKnownFrameworkAssemblies();
            using (ConsoleRunner runner = new ConsoleRunner())
            {
                int ret = runner.Start(args);
                return ret;
            }
        }

        private static void PreloadKnownFrameworkAssemblies()
        {
            // Note; the particular types that are used here are not very relevant, they are
            // just the first types that Intellisense showed in a particular namespace that lives
            // in a particular assembly. All we do is reference it, so we can get the actual name
            // of the assembly it lives in, and with that preload the assembly, so that we can avoid
            // needing the Assembly.LoadFrom context at a later point.
            NAnt.Core.Reflection.AssemblyLoader.PreloadAssembliesContaining(
                typeof(NAnt.Core.ArgumentElement),
                typeof(NAnt.DotNetTasks.AlTask),
                typeof(EA.CPlusPlusTasks.AsTask)
                );
        }

        public int Start(string[] args)
        {
#if DEBUG
            const bool DebugThrow = false;
#else
            const  bool DebugThrow = false;
#endif
            try
            {
                // add the console event handler if we are on Windows
                if (PlatformUtil.IsWindows)
                {
                    _consoleControl = new ConsoleCtrl();
                    _consoleControl.ControlEvent += new ConsoleCtrl.ControlEventHandler(ConsoleEventHandler);
                }

                //PROFILE("ConsoleRunner.Start");

                if (!ProcessCommandLine(args))
                {
                    return ReturnCode.Ok;
                }

                Project.EnableCoreDump = (bool)Option.EnableCoreDump.Value;

                Project.NoParallelNant = (bool)Option.NoParallelNant.Value;

                Log.WarningLevel = (Log.WarnLevel)Option.WarnLevel.Value;

                LoggerType loggertypes = 0;

                foreach(var loggertype in ((IEnumerable<string>)Option.Logger.Value))
                {
                    loggertypes |= ConvertUtil.ToEnum<LoggerType>(loggertype);
                }

                Log.DefaultLoggerTypes = (loggertypes == 0) ? LoggerType.Default : loggertypes;

                PrintProcessEnvironment((Log.LogLevel)Option.LogLevel.Value);

                //PROFILE("ConsoleRunner.ProcessCommandLine()");

                //PROFILE("ConsoleRunner.Log.Init()");

                
                Concurrent.Task task_ProjectGlobalInit = Concurrent.Task.Factory.StartNew(() => Project.GlobalInit());
                {
                    try
                    {
                        LoadPropertiesFile();
                    }
                    finally
                    {
                        WaitForTask(task_ProjectGlobalInit);
                    }

                    //PROFILE("Project.GlobalInit()");

                    using (Project project = new Project((string)Option.BuildFile.Value, (Log.LogLevel)Option.LogLevel.Value, (int)Option.LogIndentLevel.Value, usestderr: (bool)Option.UseStdErr.Value))
                    {
                        //PROFILE("ConsoleRunner new Project()");

                        InitProject(project);

                        //PROFILE("ConsoleRunner.InitProject()");

                        if (!project.Run())
                        {
                            return ReturnCode.Error;
                        }

                        //PROFILE("ConsoleRunner.Run");

                        if (Option.ExpandProjectTo.Value != null)
                        {
                            var expandProjectTo = (string)Option.ExpandProjectTo.Value;
                            if (expandProjectTo == String.Empty)
                                expandProjectTo = Path.GetFileName(project.BuildFileLocalName) + ".expanded";
                            File.WriteAllText(expandProjectTo, project.ToComparableString(), new UTF8Encoding(false));
                        }

                    }
                }
                PROFILE("ConsoleRunner.Execute");
            }
            catch (ApplicationException e)
            {
                ShowUserError(e);

                // throw an exception if we are in debug mode so that data can be inspected. On release it will work as it normally does                
                if (DebugThrow) throw;

                return ReturnCode.Error;

            }
            catch (Exception e)
            {
                ShowInternalError(e);

                // throw an exception if we are in debug mode so that data can be inspected. On release it will work as it normally does
                if (DebugThrow) throw;

                return ReturnCode.InternalError;
            }

            return ReturnCode.Ok;
        }

        private void LoadPropertiesFile()
        {
            // read properties from properties file(s)
            foreach (string file in (List<string>)Option.PropertiesFile.Value)
            {
                PropertiesFileLoader.LoadPropertiesFromFile(file, (Dictionary<string, string>)Option.DefineProperty.Value);
            }

            BuildFile.UseOverrides = (bool)Option.UseOverrides.Value;

            if (String.IsNullOrEmpty(Option.BuildFile.Value as String))
            {
                Option.BuildFile.Value = BuildFile.GetBuildFileName(Environment.CurrentDirectory, null, true, (bool)Option.FindBuildFile.Value);
            }
        }

        private static void ConsoleEventHandler(ConsoleCtrl.ConsoleEvent consoleEvent)
        {

            // allow nant to run as a service
            if (consoleEvent == ConsoleCtrl.ConsoleEvent.CTRL_LOGOFF)
            {
                return;
            }

            ProcessRunner.SafeKill(Process.GetCurrentProcess());
        }

        private void InitProject(Project project)
        {
                // Set initial values
                project.Verbose = (bool)Option.Verbose.Value;

                project.TraceEcho = (bool)Option.TraceEcho.Value;

                MetricsSetup.SkipMetrics = (bool)Option.NoMetrics.Value;

                // Check environment variable "FRAMEWORK_NOMETRICS":
                // FALSE - do metrics
                // empty string, any value not 'FALSE' - skip metrics
                var nometrics_env = Environment.GetEnvironmentVariable("FRAMEWORK_NOMETRICS");
                if(nometrics_env != null)
                {
                    MetricsSetup.SkipMetrics = true;
                    if(0 == String.Compare(nometrics_env, "false", true))
                    {
                        MetricsSetup.SkipMetrics = true;
                    }
                }

                // TEMP HACK ----------------------------------------------------------
                //IM: Launching FrameworkTelemetry process on Unix can cause process that invokes nant to wait for completion forever.
                // Temporarily disable metrics on non windows machines.
                if (!PlatformUtil.IsWindows)
                {
                    MetricsSetup.SkipMetrics = true;
                }
                // END TEMP HACK ----------------------------------------------------------
                MetricsSetup.SetProject(project);

                // set user supplied target names
                foreach (string targetName in (List<string>)Option.TargetNames)
                {
                    project.BuildTargetNames.Add(targetName);
                }


                OptionSet commandlineproperties = new OptionSet();
                commandlineproperties.Project = project;
                project.NamedOptionSets.Add(Project.NANT_OPTIONSET_PROJECT_CMDPROPERTIES, commandlineproperties);

                // Add input properties:
                foreach (KeyValuePair<string, string> prop in (Dictionary<string, string>)Option.DefineProperty.Value)
                {
                    project.Properties.AddReadOnly(prop.Key, prop.Value);

                    if (!commandlineproperties.Options.ContainsKey(prop.Key))
                    {
                        commandlineproperties.Options.Add(prop.Key, prop.Value);
                    }
                    else
                    {
                        if (commandlineproperties.Options[prop.Key] != prop.Value)
                        {
                            project.Log.Warning.WriteLine("Property '{0}' is defined more than once on the nant command line or properties file with different values: {0} and {1}, second value will be ignored.", prop.Key, commandlineproperties.Options[prop.Key], prop.Value);
                        }
                    }
                }
                foreach (KeyValuePair<string, string> prop in (Dictionary<string, string>)Option.DefineGlobalProperty.Value)
                {
                    Project.GlobalProperties.Add(prop.Key, true);

                    // Add property:
                    project.Properties.AddReadOnly(prop.Key, prop.Value);

                    if(!commandlineproperties.Options.ContainsKey(prop.Key))
                    {
                        commandlineproperties.Options.Add(prop.Key, prop.Value);
                    }
                    else
                    {
                        if (commandlineproperties.Options[prop.Key] != prop.Value)
                        {
                            project.Log.Warning.WriteLine("Property '{0}' is defined more than once on the nant command line or properties file with different values: {0} and {1}, second value will be ignored.", prop.Key, commandlineproperties.Options[prop.Key], prop.Value);
                        }
                    }
                }

            PackageMap.Init(Path.GetDirectoryName((string)Option.BuildFile.Value), (string)Option.MasterConfigFile.Value, (string)Option.BuildRoot.Value,
                            (bool)Option.ForceAutobuild.Value, (string)Option.ForceAutobuildPackage.Value, (string)Option.ForceAutobuildGroup.Value,
                            project);

            // If package.configs property is specified explicitly and config property is not specified, set config equal to first value in package.configs
            
            if (!project.Properties.Contains("config"))
            {
                var config = project.Properties[PackageProperties.PackageConfigsPropertyName].TrimWhiteSpace().ToArray().FirstOrDefault();
                if(!String.IsNullOrEmpty(config))
                {
                    project.Properties.AddReadOnly("config", config);
                    commandlineproperties.Options.Add("config", config);
                }
            }
        }


        // returns false if need to stop processing
        private static bool ProcessCommandLine(string[] args)
        {
            Option.ParseCommandLine(args);

            if ((bool)Option.Verbose.Value)
            {
                Option.LogLevel.Value = Log.LogLevel.Diagnostic;
            }

            if ((bool)Option.VersionHelp.Value)
            {
                ShowVersionHelp();
                return false;
            }
            else if ((bool)Option.ProgramHelp.Value)
            {
                ShowProgramHelp();
                return false;
            }
            else if ((bool)Option.ProjectHelp.Value)
            {
                // TODO
                //ShowProjectHelp();
                return false;
            }

            return true;
        }

        private static void ShowBanner()
        {
            // Get version information directly from assembly.  This takes more work but keeps the version 
            // numbers being displayed in sync with what the assembly is marked with.
            string version = Assembly.GetExecutingAssembly().GetName().Version.ToString();

            string title = Assembly.GetExecutingAssembly().GetName().Name;
            object[] titleAttribute = Assembly.GetExecutingAssembly().GetCustomAttributes(typeof(AssemblyTitleAttribute), true);
            if (titleAttribute.Length > 0)
                title = ((AssemblyTitleAttribute)titleAttribute[0]).Title;

            string copyright = "";
            object[] copyRightAttribute = Assembly.GetExecutingAssembly().GetCustomAttributes(typeof(AssemblyCopyrightAttribute), true);
            if (copyRightAttribute.Length > 0)
                copyright = ((AssemblyCopyrightAttribute)copyRightAttribute[0]).Copyright;

            string link = "";
            object[] companyAttribute = Assembly.GetExecutingAssembly().GetCustomAttributes(typeof(AssemblyCompanyAttribute), true);
            if (companyAttribute.Length > 0)
                link = ((AssemblyCompanyAttribute)companyAttribute[0]).Company;

            System.Console.WriteLine(String.Format("{0}-{1} {2}", title, version, copyright));
            System.Console.WriteLine(link);
            System.Console.WriteLine();
        }

        private static void ShowProgramHelp()
        {
            ShowBanner();

            const int optionPadding = 23;

            System.Console.WriteLine();
            System.Console.WriteLine("Usage: nant [options] [target [target2] ... ]");
            System.Console.WriteLine();
            System.Console.WriteLine("Options:");

            foreach (FieldInfo prop in typeof(Option).GetFields(BindingFlags.Static | BindingFlags.Public))
            {
                Option op = prop.GetValue(null) as Option;
                if (!String.IsNullOrEmpty(op.Description1))
                {
                    System.Console.WriteLine(op.Description1, (op.Name + op.Description0).PadRight(optionPadding));
                }
            }
            System.Console.WriteLine();
            System.Console.WriteLine("A file ending in .build will be used if no buildfile is specified.");
        }

        private static void ShowVersionHelp()
        {
            ShowBanner();
            ShowAssemblyVersions(Assembly.GetExecutingAssembly());
        }

        private static void ShowAssemblyVersions(Assembly assembly)
        {
            ShowAssembly(assembly.GetName());
            AssemblyName[] assemblies = assembly.GetReferencedAssemblies();
            Array.Sort<AssemblyName>(assemblies, new AssemblyNameComparer());
            foreach (AssemblyName assemblyName in assemblies)
            {
                ShowAssembly(assemblyName);
            }
        }

        private static void ShowAssembly(AssemblyName assemblyName)
        {
            string name = assemblyName.Name;
            string version = assemblyName.Version.ToString();

            // need to load assembly to get location
            Assembly assembly = Assembly.Load(assemblyName);
            string location;
            if (assembly.GlobalAssemblyCache)
            {
                location = "Global Assembly Cache";
            }
            else
            {
                location = Path.GetDirectoryName(assembly.Location);
            }

            const string assemblyFormatString = "{0,-15} {1,-12} {2}";
            System.Console.WriteLine(assemblyFormatString, name, version, location);
        }

        private static void ShowUserError(ApplicationException e)
        {
            Exception current = e;
            var offset = String.Empty;
            while (current != null)
            {
                if (current.Message.Length > 0)
                {
                    if (current is BuildException && ((BuildException)current).Location != Location.UnknownLocation)
                    {
                        System.Console.WriteLine("{0}error at {1} {2}", offset, ((BuildException)current).Location.ToString(), current.Message);
                    }
                    else
                    {
                        System.Console.WriteLine("{0}{1}", offset, current.Message);
                    }
                    offset += "  ";
                }
                current = current.InnerException;
            }
            System.Console.WriteLine("Try 'nant -help' for more information");
        }

        private static void ShowInternalError(Exception e)
        {
            System.Console.WriteLine("INTERNAL ERROR");
            System.Console.WriteLine(e.ToString());
            System.Console.WriteLine();
            System.Console.WriteLine("Please send bug reports to eatechcmsupport@ea.com.");
        }

        private static void PrintProcessEnvironment(Log.LogLevel loglevel)
        {
            if(loglevel >= Log.LogLevel.Diagnostic)
            {
                try
                {
                    System.Console.WriteLine("--------------------------------------------------------------------------------------------------------------");
                    System.Console.WriteLine("Environment:");
                    System.Console.WriteLine("--------------------------------------------------------------------------------------------------------------");
                    System.Console.WriteLine(Environment.CommandLine);
                    System.Console.WriteLine("");
                    System.Console.WriteLine("CurrentDirectory=\"{0}\"", Environment.CurrentDirectory);
                    System.Console.WriteLine("");
                    foreach (System.Collections.DictionaryEntry env in Environment.GetEnvironmentVariables())
                    {
                        System.Console.WriteLine("\t{0}={1}", env.Key, env.Value);
                    }
                    System.Console.WriteLine("");
                    System.Console.WriteLine("--------------------------------------------------------------------------------------------------------------");
                    System.Console.WriteLine("");
                }
                catch (Exception ex)
                {
                    System.Console.WriteLine("Unable to retrieve process environment: {0}", ex.Message);
                }
            }
        }

        private void PROFILE(string tag)
        {
#if PROFILE
            sw.Stop();
            System.Console.WriteLine("{0}: {1} millisec", tag, sw.ElapsedMilliseconds);
            sw.Restart();
#endif
        }

        private void WaitForTask(Concurrent.Task task)
        {
            Concurrent.Task.WaitAll(task);
            task.Dispose();
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
                    // Dispose managed resources.
                    if (_consoleControl != null)
                    {
                        _consoleControl.Dispose();
                    }
                }
            }
            _disposed = true;
        }

        private class AssemblyNameComparer : IComparer<AssemblyName>
        {
            public int Compare(AssemblyName x, AssemblyName y)
            {
                return String.Compare(x.Name, y.Name, true);
            }
        }

        private static class ReturnCode
        {
            public static readonly int Ok = 0;
            public static readonly int Error = 1;
            public static readonly int InternalError = 2;
        }

        private class Option : IComparable<Option>
        {
            internal readonly string Name;
            internal readonly string Description0;
            internal readonly string Description1;
            internal object Value;

            Option(object value, string name, string description) 
            { 
                Value = value; 
                Name = name; 
                Description0 = String.Empty; 
                Description1 = description; 
            }
            Option(object value, string name, string description0, string description1) 
            { 
                Value = value; 
                Name = name; 
                Description0 = description0; 
                Description1 = description1; 
            }

            public static readonly Option ProgramHelp = new Option(false,
                "-help",
                "  {0} print this message");
            public static readonly Option ProjectHelp = new Option(false,
                "-projecthelp",
                "  {0} print project help information");
            public static readonly Option VersionHelp = new Option(false,
                "-version",
                "  {0} print version information");
            public static readonly Option BuildFile = new Option(String.Empty,
                "-buildfile:",
                "<file>",
                "  {0} use given buildfile");
            public static readonly Option FindBuildFile = new Option(false,
                "-find",
                "  {0} search parent directories for buildfile");
            public static readonly Option DefineProperty = new Option(new Dictionary<string, string>(),
                "-D:",
                "<property>=<value>",
                "  {0} use value for given property");
            public static readonly Option DefineGlobalProperty = new Option(new Dictionary<string, string>(),
                "-G:",
                "<property>=<value>",
                "  {0} Define global property, use value for given property");

            public static readonly Option PropertiesFile = new Option(new List<string>(),
                "-propertiesfile:",
                "<file>",
                "  {0} use given properties file");
            public static readonly Option MasterConfigFile = new Option(String.Empty,
                "-masterconfigfile:",
                "<file>",
                "  {0} use specified master configuration file");
            public static readonly Option BuildRoot = new Option(String.Empty,
                "-buildroot:",
                "<path>",
                "  {0} use given build root");
            public static readonly Option ForceAutobuild = new Option(false,
                "-forceautobuild",
                "  {0} force autobuild for all packages, even for packages that have 'autobuildclean=false'");
            public static readonly Option ForceAutobuildPackage = new Option(String.Empty,
                "-forceautobuild:",
                "\"<package name> <package name>\"",
                "  {0} force autobuild for packages with given names, even for packages that have 'autobuildclean=false'");
            public static readonly Option ForceAutobuildGroup = new Option(String.Empty,
                "-forceautobuild:group:",
                "\"<grouptype name> <grouptype name>\"",
                "  {0} force autobuild for packages in the given grouptypes, even for groups that have 'autobuildclean=false'");
            public static readonly Option Verbose = new Option(false,
                "-verbose",
                "  {0} sets loglevel to 'Diagnostic'");

            public static readonly Option LogLevel = new Option(Log.LogLevel.Normal,
                "-loglevel:",
                EnumOptions<Log.LogLevel>(),
                "  {0} sets the log level. Default '" + Log.LogLevel.Normal.ToString() +"'.");

            public static readonly Option WarnLevel = new Option(Log.WarnLevel.Normal,
                "-warnlevel:",
                EnumOptions<Log.WarnLevel>(),
                "  {0} sets the warnings output level. Default '" + Log.WarnLevel.Normal.ToString() + "'.");

            public static readonly Option Logger = new Option(new List<string>(),
                "-logger:",
                "  {0} use specified logger class");
            public static readonly Option LogIndentLevel = new Option(0,
                "-indent:",
                "  {0} defined indentation value in log");
            public static readonly Option ShowTime = new Option(false,
                "-showtime",
                "  {0} log current time at the end of the build");
            public static readonly Option NoMetrics = new Option(false,
                "-nometrics", String.Empty);
            public static readonly Option UseOverrides = new Option(false,
                "-useoverrides", String.Empty);
            public static readonly Option ExpandProjectTo = new Option(null,
                "-expand-project-to:", String.Empty);
            public static readonly Option EnableCoreDump = new Option(false,
                "-enablecoredump",
                "  {0} enable project instance core dump. Core dump is disabled by default");
            public static readonly Option TraceEcho = new Option(false,
                "-trace-echo",
                "  {0} displays location of all echo instructions.");
            public static readonly Option NoParallelNant = new Option(false,
                "-noparallel",
                "  {0} Disables parallel build script loading and processing. Parallel processing is enabled by default.");

            public static readonly Option UseStdErr = new Option(false,
                "-usestderr",
                "  {0} Error records sent to console are written to stderr, otherwise to stdout.");

            public int CompareTo(Option other)
            {
                return other.Name.Length - Name.Length;
            }

            internal static readonly List<string> TargetNames = new List<string>();

            public static void ParseCommandLine(string[] args)
            {
                Option[] options = Array.ConvertAll<FieldInfo, Option>(
                                        typeof(Option).GetFields(BindingFlags.Static | BindingFlags.Public),
                                        field => field.GetValue(null) as Option);

                // Sort by option names, processing longer names first, important for options like forceautobuild and forceautobuild:*, etc.
                Array.Sort<Option>(options);

                foreach (string arg in args)
                {
                    bool found = false;
                    foreach (Option option in options)
                    {
                        if (found)
                        {
                            break;
                        }

                        if (arg.StartsWith(option.Name))
                        {
                            found = true;
                            if (option.Value is bool)
                            {
                                if (option.Name.EndsWith(":"))
                                {
                                    string value = String.Empty;

                                    if (arg.Length > option.Name.Length)
                                    {
                                        value = arg.Substring(option.Name.Length);
                                    }
                                    bool inputBoolean;
                                    if (Boolean.TryParse(value, out inputBoolean))
                                    {
                                        option.Value = inputBoolean;
                                    }
                                    else
                                    {
                                        String msg = String.Format("Invalid input for '{0}' parameter {1}. This is boolean parameter, valud values are '{0}true' or '{0}false'.", option.Name, arg);
                                        throw new ApplicationException(msg);
                                    }
                                }
                                else
                                {
                                    option.Value = true;
                                }
                            }
                            else if (option.Value is Dictionary<string, string>)
                            {
                                Dictionary<string, string> dictionary = (Dictionary<string, string>)option.Value;

                                Match match = Regex.Match(arg, option.Name + @"(\w+.*?)=(\w*.*)");
                                if (match.Success)
                                {
                                    string name = match.Groups[1].Value;
                                    string value = match.Groups[2].Value;

                                    if (dictionary.ContainsKey(name))
                                    {
                                        System.Console.WriteLine("WARNING: duplicate command line definition of the property '{0}' is ignored. '{1}{2}={3}' will be used.", arg, option.Name, name, dictionary[name]);
                                    }
                                    else
                                    {
                                        dictionary[name] = value;
                                    }
                                }
                                else
                                {
                                    String msg = String.Format("Invalid syntax for '{0}' parameter {1}", option.Name, arg);
                                    throw new ApplicationException(msg);
                                }
                            }
                            else if (option.Value is string)
                            {
                                if (!String.IsNullOrEmpty((string)option.Value))
                                {
                                    throw new ApplicationException(String.Format("Option '{0}' has already been specified.", option.Name));
                                }

                                if (arg.Length > option.Name.Length)
                                {
                                    option.Value = arg.Substring(option.Name.Length);
                                }

                                if (String.IsNullOrEmpty((string)option.Value))
                                {
                                    throw new ApplicationException(String.Format("Option '{0}' value is missing in the argument.", option.Name));
                                }
                            }
                            else if (option.Value is List<string>)
                            {
                                string value = null;
                                List<string> list = (List<string>)option.Value;

                                if (arg.Length > option.Name.Length)
                                {
                                    value = arg.Substring(option.Name.Length);
                                }

                                if (String.IsNullOrEmpty(value))
                                {
                                    throw new ApplicationException(String.Format("Option '{0}' value is missing in the argument.", option.Name));
                                }
                                if (list.Contains(value))
                                {
                                    System.Console.WriteLine("WARNING: duplicate command line argument {0}", arg);
                                }
                                else
                                {
                                    list.Add(value);
                                }
                            }
                            else if (option.Value is Enum)
                            {
                                string value = null;
                                if (arg.Length > option.Name.Length)
                                {
                                    value = arg.Substring(option.Name.Length);
                                }

                                if (String.IsNullOrEmpty(value))
                                {
                                    throw new ApplicationException(String.Format("Option '{0}' value is missing in the argument.", option.Name));
                                }

                                try
                                {
                                    option.Value = Enum.Parse(option.Value.GetType(), value, true);
                                }
                                catch (Exception ex)
                                {
                                    throw new ApplicationException(String.Format("Filed to convert Option '{0}' value '{1}' to Enum '{2}', valid values are: '{3}'.", option.Name, value, option.Value.GetType().Name, StringUtil.ArrayToString(Enum.GetValues(option.Value.GetType()), ", ")), ex);
                                }

                            }
                            else if (option.Value is int)
                            {
                                string value = null;
                                if (arg.Length > option.Name.Length)
                                {
                                    value = arg.Substring(option.Name.Length);
                                }

                                if (String.IsNullOrEmpty(value))
                                {
                                    throw new ApplicationException(String.Format("Option '{0}' value is missing in the argument.", option.Name));
                                }
                                int intval;
                                if (!Int32.TryParse(value, out intval))
                                {
                                    throw new ApplicationException(String.Format("Filed to convert Option '{0}' value '{1}' to integer.", option.Name, value));
                                }
                                option.Value = intval;
                            }
                            else
                            {
                                throw new Exception("INTERNAL ERROR: No processing done for parameter '" + option.Name + "'.");
                            }
                        }
                    }
                    if (!found)
                    {
                        if (arg.StartsWith("-"))
                        {
                            string msg = String.Format("Unknown command line option '{0}'.", arg);
                            throw new ApplicationException(msg);
                        }
                        // must be a target if not an option
                        TargetNames.Add(arg);
                    }
                }
            }

            private static string EnumOptions<T>()
            {
                return "[" + Enum.GetNames(typeof(T)).ToString("|") + "]";
            }
        }
    }
}
