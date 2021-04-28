using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Text.RegularExpressions;
using System.Diagnostics;

namespace NAnt.Core
{


    [Serializable]
    public class ContextCarryingException : Exception
    {
        private static readonly string[] AssembliesToIgnore = { "NAnt.Core" };
        private static readonly string[] LinesToIgnore = { "Task.Execute", "Target.Execute", "ForEachModule.ExecuteActionWithDependents" };

        /// List of Namespaces to be ommitted from the callstack for the sake of simplicity.
        private static readonly List<string> CallstackImplicitNamespaces = new List<string>()
        {
            "NAnt.Core",
            "System.Collections.Generic",
            "System.Collections",
            "System.Data",
            "System.IO",
            "System.Text.RegularExpressions",
            "System.Text",
            "System",
            "System.Threading",
            "System.Threading.Tasks",
            "EA.Eaconfig.Core"
        };

        private List<NAntStackFrame> _stackFrames;

        public ContextCarryingException(BuildException e, string taskName, NAntStackScopeType scopeType = NAntStackScopeType.Task)
            : base(String.Empty, e)
        {
            _stackFrames = new List<NAntStackFrame>() { new NAntStackFrame(NAntStackFrameType.NAnt, taskName, e.Location, scopeType) };
        }

        public ContextCarryingException(Exception e, string taskName, Location location, NAntStackScopeType scopeType = NAntStackScopeType.Task)
            : base(String.Empty, e)
        {
            _stackFrames = new List<NAntStackFrame>() { new NAntStackFrame(NAntStackFrameType.NAnt, taskName, location, scopeType) };
        }

        public string SourceTask
        {
            get
            {
                return _stackFrames.First().MethodName;
            }
        }

        public string SourceTaskType
        {
            get
            {
                var type = _stackFrames.First().ScopeType;
                if (type != NAntStackScopeType.CSharp)
                {
                    return type.ToString().ToLowerInvariant();
                }
                return "task";
            }
        }


        public void PushNAntStackFrame(string methodName, NAntStackScopeType scopeType, Location location)
        {
            _stackFrames.Add(new NAntStackFrame(NAntStackFrameType.NAnt, methodName, location, scopeType));
        }

        public NAntStackFrame PopNAntStackFrame()
        {
            var frame = _stackFrames.Last();
            _stackFrames.RemoveAt(_stackFrames.Count - 1);

            return frame;
        }

        public IEnumerable<NAntStackFrame> GetCallstack(Location location = null)
        {
            var type = BuildException.GetBaseStackTraceType(InnerException);

            IEnumerable<NAntStackFrame> csharpStackFrames;

            if(type == BuildException.StackTraceType.None)
            {
                return new List<NAntStackFrame>();
            }
            if (BuildException.StackTraceType.Full == type)
            {
                csharpStackFrames = GetExceptions(InnerException).Reverse()
                   .SelectMany(e => new StackTrace(e, true).GetFrames() ?? Enumerable.Empty<StackFrame>())
                   .Select(sf => new NAntStackFrame(NAntStackFrameType.CSharp, GetSimpleTypeName(sf.GetMethod().DeclaringType, CallstackImplicitNamespaces) + "." + sf.GetMethod().Name, new Location(sf.GetFileName(), sf.GetFileLineNumber(), sf.GetFileColumnNumber())))
                   .Where(nsf => !LinesToIgnore.Contains(nsf.MethodName));
            }
            else
            {
                csharpStackFrames = new List<NAntStackFrame>();
            }

            var nextCCException = GetNextContextCarryingExceptions();

            var innerCallStacks = nextCCException != null ? nextCCException.GetCallstack() : new List<NAntStackFrame>();

            return innerCallStacks.Concat(csharpStackFrames).Concat(_stackFrames);
        }

        private IEnumerable<Exception> GetExceptions(Exception e)
        {
            while (e != null && !(e is ContextCarryingException))
            {
                if(!(e is BuildExceptionStackTraceSaver || e is ExceptionStackTraceSaver))
                {
                    yield return e;
                }
                e = e.InnerException;
            }
        }

        private ContextCarryingException GetNextContextCarryingExceptions()
        {
            var e = InnerException;

            while (e != null && !(e is ContextCarryingException))
            {
                e = e.InnerException;
            }
            return (e as ContextCarryingException);
        }

        public static ContextCarryingException GetLastContextCarryingExceptions(Exception ex)
        {
            ContextCarryingException cce = null;
            var e = ex;
            while (e != null)
            {
                if (e is ContextCarryingException)
                {
                    cce = e as ContextCarryingException; 
                }
                e = e.InnerException;
            }
            return cce;
        }


        public override string StackTrace
        {
            get
            {
                StringBuilder sb = new StringBuilder();
                bool firstSystem = true;
                foreach (var stackFrame in GetCallstack())
                {
                    if(stackFrame.Type == NAntStackFrameType.NAnt && stackFrame.Location == Location.UnknownLocation)
                    {
                        continue;
                    }
                    if (stackFrame.Type == NAntStackFrameType.CSharp)
                    {

                        if (stackFrame.Location.FileName == null && stackFrame.Location.LineNumber == 0 && stackFrame.Location.ColumnNumber == 0)
                        {
                            if (!firstSystem)
                            {
                                continue;
                            }

                            firstSystem = false;
                        }
                        else if(-1 != stackFrame.MethodName.IndexOfAny(new char[]{'<', '>'}))
                        {
                            // Skip instantiated templates
                            continue;
                        }
                    }

                    var methodName = stackFrame.MethodName;
                    if (stackFrame.Type == NAntStackFrameType.NAnt)
                    {
                        methodName = methodName.Quote();
                    }
                    sb.AppendFormat("at {0} {1} {2}", stackFrame.ScopeType.ToString().ToLower().PadRight(8), methodName, stackFrame.Location.ToString().TrimEnd(new char[] { ':' }));
                    sb.AppendLine();
                }
                return sb.ToString();
            }
        }

        public string BaseMessage
        {
            get { return base.Message; }
        }


        public ContextCarryingException PropagateException(Task sourceTask)
        {
            return AssembliesToIgnore.Contains(sourceTask.GetType().Assembly.GetName().Name) ? null : new ContextCarryingException(this, sourceTask.Name, sourceTask.Location);
        }

        /// <summary>
        /// Returns a simplified name for a type.
        /// </summary>
        /// <param name="type">The type.</param>
        /// <param name="implicitNamespaces">NAnt project.</param>
        /// <returns>The formatted type name.</returns>
        private string GetSimpleTypeName(Type type, List<string> implicitNamespaces)
        {
            if (type.IsGenericParameter)
            {
                return string.Empty;
            }
            else
            {
                string nameSpace = type.Namespace;
                string name = type.Name;
                string parameters = string.Empty;
                if (type.IsGenericType)
                {
                    name = Regex.Replace(name, @"`\d+$", string.Empty);
                    parameters = string.Format("<{0}>", string.Join(", ", type.GetGenericArguments().Select(t => GetSimpleTypeName(t, implicitNamespaces)).ToArray()));
                }
                if (type.IsNested)
                {
                    nameSpace = string.Empty;
                    name = string.Format("{0}.{1}", GetSimpleTypeName(type.ReflectedType, implicitNamespaces), name);
                }
                // Remove simple namespaces from the name.
                if (implicitNamespaces.Contains(nameSpace))
                {
                    nameSpace = string.Empty;
                }
                return string.IsNullOrEmpty(nameSpace) ? string.Concat(name, parameters) : string.Concat(nameSpace, ".", name, parameters);
            }
        }

    }
}
