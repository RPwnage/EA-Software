// NAnt - A .NET build tool
// Copyright (C) 2001-2003 Gerry Shaw
//
// Kosta Arvanitis (karvanitis@ea.com)

using System;
using System.Collections.Specialized;
using System.IO;
using System.Reflection;
using System.Xml;

using NAnt.Core;
using NAnt.Core.Attributes;
using NAnt.Core.Logging;
using NAnt.Core.Util;
using NAnt.Core.Reflection;

namespace NAnt.Core.Tasks
{

    /// <summary>Loads functions from a specified assembly.</summary>
    /// <remarks>
    /// <para>
    /// NAnt can only use .NET assemblies; other types of files which
    /// end in .dll won't work.
    /// </para>
    /// </remarks>
    /// <include file='Examples/FuncDef/FuncDef.example' path='example'/>
    [TaskName("funcdef")]
    public class FuncDefTask : Task
    {
        string	_assemblyFileName = null;
        bool _override = false;
        bool _failOnErrorFuncDef = false;

        /// <summary>File name of the assembly containing the NAnt functions.</summary>
        [TaskAttribute("assembly", Required = true)]
        public string AssemblyFileName
        {
            get { return _assemblyFileName; }
            set { _assemblyFileName = value; }
        }

        /// <summary>
        /// Override function(s) with the same name.
        /// Default is false. When override is 'false' &lt;funcdef&gt; will fail on duplicate function names.
        /// </summary>
        [TaskAttribute("override", Required = false)]
        public bool Override
        {
            get { return _override; }
            set { _override = value; }
        }

        [TaskAttribute("failonerror")]
        public override bool FailOnError
        {
            get { return _failOnErrorFuncDef; }
            set { _failOnErrorFuncDef = value; }
        }


#if BPROFILE
		/// <summary>Return additional info identifying the task to the BProfiler.</summary>
		public override string BProfileAdditionalInfo 
		{
			get { return AssemblyFileName; }
		}
#endif

        protected override void ExecuteTask()
        {
            string assemblyFileName = Project.GetFullPath(AssemblyFileName);
            try
            {
                int num = Project.FuncFactory.AddFunctions(AssemblyLoader.Get(assemblyFileName), Override, FailOnError);

                Log.Debug.WriteLine(LogPrefix + "Added {0} functions from {1}.", num, UriFactory.CreateUri(assemblyFileName));

            }
            catch (Exception e)
            {
                string msg = String.Format("Could not add functions from '{0}'.", assemblyFileName);
                throw new BuildException(msg, Location, e);
            }
        }
    }
}
