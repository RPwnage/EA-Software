// NAnt - A .NET build tool
// Copyright (C) 2001-2002 Gerry Shaw
//
// Kosta Arvanitis (karvanitis@ea.com)

using System;
using System.IO;
using System.Xml;

using NAnt.Core.Attributes;
using NAnt.Core.Logging;

namespace NAnt.Core.Tasks 
{
    /// <summary>Evaluate a block of code.</summary>
    /// <remarks>
    /// The eval task will evaluate a specified block of code and optionally store the result in a property. If 
    /// no property name is specified the result ignored. This task is useful for running functions which do not 
    /// require output.
    /// </remarks>
    /// <include file='Examples\Eval\Eval.example' path='example'/>
	/// <include file='Examples\Eval\EvalProperty.example' path='example'/>
	/// <include file='Examples\Eval\EvalExpression.example' path='example'/>
    [TaskName("eval")]
    public class EvalTask : Task {

		public enum EvalType {
			Property,
			Function,
			Expression,
		}

		string _propertyName = null;
        string _code = null;
		EvalType _evalType;

        /// <summary>The code to evaluate.</summary>
        [TaskAttribute("code", Required=true, ExpandProperties=false)]
        public string Code {
            get { return _code; }
            set { _code = value; }
        }

		/// <summary>The type of code to evaluate. Valid values are <c>Property</c>, <c>Function</c> and <c>Expression</c>.</summary>
		[TaskAttribute("type", Required=true)]
		public EvalType Type {
			get { return _evalType; }
			set { _evalType = value; }
		}

		/// <summary>The name of the property to place the result into. If none is specifed result is ignored.</summary>
		[TaskAttribute("property", Required=false)]
		public string PropertyName {
			get { return _propertyName; }
			set { _propertyName = value; }
		}

#if BPROFILE
		/// <summary>Return additional info identifying the task to the BProfiler.</summary>
		public override string BProfileAdditionalInfo 
		{
			get { return Code.Substring(0, Math.Min(20, Code.Length)); }
		}
#endif

        protected override void ExecuteTask() {

			string val;
			
			try {
				switch(Type) {
					case EvalType.Expression:
						val = Project.ExpandProperties(Code);
						val = Expression.Evaluate(val).ToString();
						break;
					case EvalType.Function:
					case EvalType.Property:
						val = Project.ExpandProperties(Code);
						break;
					default:
						// we should never get here
						val = "";
						break;
				}
				if (PropertyName != null) {
					Project.Properties.Add(PropertyName, val);
				}
			}
			catch(System.Exception e) {
				throw new BuildException("Failed to evaluate code.", e);
			}
        }
    }
}
