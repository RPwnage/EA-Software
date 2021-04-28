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
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Text.RegularExpressions;
using System.Diagnostics;
using NAnt.Core.Threading;

namespace NAnt.Core
{


	[Serializable]
	public class ContextCarryingException : Exception
	{
		private readonly List<NAntStackFrame> _stackFrames = null;

		private static readonly string[] AssembliesToIgnore = { "NAnt.Core" };
		private static readonly string[] LinesToIgnore = { "Task.Execute", "Target.Execute", "ForEachModule.ExecuteActionWithDependents" };

		/// List of Namespaces to be omitted from the callstack for the sake of simplicity.
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

		public ContextCarryingException(Exception ex, string taskName, Location location, NAntStackScopeType scopeType = NAntStackScopeType.Task)
			: base(String.Empty, ex)
		{
			Exception threadAggregate = ThreadUtil.ProcessAggregateException(ex);
			BuildException be = threadAggregate as BuildException;
			if (be != null)
			{
				be.SetLocationIfMissing(location);
			}

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

			IEnumerable<NAntStackFrame> csharpStackFrames = null;

			if(type == BuildException.StackTraceType.None)
			{
				return new List<NAntStackFrame>();
			}
			if (BuildException.StackTraceType.Full == type)
			{
				try
				{
					csharpStackFrames = GetExceptions(InnerException)
						.Reverse()
						.SelectMany(e => new StackTrace(e, true).GetFrames() ?? Enumerable.Empty<StackFrame>())
						.Select(sf => sf == null 
							? new NAntStackFrame(NAntStackFrameType.CSharp, "(Unresolvable C# Frame)", Location.UnknownLocation)
							: new NAntStackFrame(NAntStackFrameType.CSharp, GetSimpleTypeName(sf.GetMethod().DeclaringType, CallstackImplicitNamespaces) + "." + sf.GetMethod().Name, new Location(sf.GetFileName(), sf.GetFileLineNumber(), sf.GetFileColumnNumber())))
						.Where(nsf => !LinesToIgnore.Contains(nsf.MethodName))
						// We need the ToList() below to force evaluation of the IEnumerable in case we get more exceptions.
						.ToList();
				}
				catch
				{
					// This GetCallstack() get called when higher level functions got exception and try to construct better exception
					// output.  So if for some reason we couldn't get call stack, just ignore the csharp stack frame.  We still want to
					// see the original higher level exception message.
				}
				if (csharpStackFrames == null)
				{
					csharpStackFrames = new List<NAntStackFrame>();
				}
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

		/// <summary>
		/// Returns a simplified name for a type.
		/// </summary>
		/// <param name="type">The type.</param>
		/// <param name="implicitNamespaces">NAnt project.</param>
		/// <returns>The formatted type name.</returns>
		private string GetSimpleTypeName(Type type, List<string> implicitNamespaces)
		{
			if (type==null || type.IsGenericParameter)
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
