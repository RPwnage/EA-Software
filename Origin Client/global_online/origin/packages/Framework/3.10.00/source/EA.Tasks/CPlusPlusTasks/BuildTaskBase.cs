// Copyright (C) Electronic Arts Canada Inc. 2002-2005.  All rights reserved.

using System;
using System.Diagnostics;
using System.Collections;
using System.Collections.Generic;
using System.Collections.Specialized;
using System.IO;
using System.Text.RegularExpressions;
using System.Reflection;

using NAnt.Core;
using NAnt.Core.Util;
using NAnt.Core.Logging;

namespace EA.CPlusPlusTasks 
{
	/// <summary>
	/// A class from which all build tasks will derive.
	/// </summary>
    public abstract class BuildTaskBase : Task
    {
        Dictionary<string, string>	_envVariables = new Dictionary<string, string>();
        bool				        _initialized = false;
		
        /// <summary>Global option set.</summary>
        protected string    _optionSetName;
        protected OptionSet	_optionSet  = new OptionSet();

        // Response file settings properties
        protected string UseResponseFileProperty              { get { return Name + ".useresponsefile";       } }
        protected string ResponseFileTemplateProperty         { get { return Name + ".template.responsefile"; } }
        protected string UseRelativePathsProperty             { get { return Name + ".userelativepaths"; } }
        protected string UseAltSepInResponseFileProperty      { get { return Name + ".usealtsepinresponsefile"; } }
        protected string WorkingdirTemplateProperty           { get { return Name + ".workingdir"; } }

        // template used in link.template.responsefile to represent response file name
        protected const string ResponseFileVariable           = "%responsefile%";
        protected const string EnvironmentVariablePropertyNamePrefix = "build.env";

        // default response file settings
        protected const string DefaultResponseFileTemplate    = "@\"%responsefile%\"";
        protected const string ResponseFileExtension          = ".rsp";
        protected const bool   DefaultUseResponseFile         = false;
        protected const bool   DefaultUseRelativePath         = false;
        protected const bool   DefaultUseAltSepInResponseFile = false;

		//ForceLowerCaseFilePaths support
		protected string ForceLowerCaseFilePathsProperty	{ get { return Name + ".forcelowercasefilepaths";} }
		protected const bool   DefaultForceLowerCaseFilePaths = false;

        public BuildTaskBase() : base() { }

        public BuildTaskBase(string name) : base(name) { }

        /// <summary>Force initialization of global option set.</summary>
        public abstract void InitializeFromOptionSet(OptionSet specialOptions);
		

        /// <summary>Sets a global option to specified value.</summary>
        public void SetOption(string propertyName, string propertyValue) {
            _optionSet.Options[propertyName] = propertyValue;
        }
			
        /// <summary>Returns the specified option value from the global set of options.</summary>
        public string GetOption(string propertyName) {
            return _optionSet.Options[propertyName];
        }

        /// <summary>
        /// Returns the specified option value from the fileset items specified option set.
        /// If the given fileset item does not contain a optionset name the global property
        /// is returned.
        /// </summary>
        public string GetOption(FileItem fileItem, string propertyName) 
        {
            string propertyValue = null;			
            if (fileItem != null && fileItem.OptionSetName != null) {
                OptionSet optionSet;
                if(!Project.NamedOptionSets.TryGetValue(fileItem.OptionSetName, out optionSet))
                {                    
                    string msg = String.Format("Unknown option set '{0}'.", fileItem.OptionSetName);
                    throw new BuildException(msg, Location);
                }
                propertyValue = optionSet.Options[propertyName];
            }

            if (propertyValue == null) {
                propertyValue = _optionSet.Options[propertyName];
            }

            return propertyValue;
        }

		public void AddProperty(string propertyName) {
			string propertyValue = Project.Properties[propertyName];
			if (propertyValue != null) {
				_optionSet.Options[propertyName] = propertyValue;
			}
		}

        public abstract string OptionSetName { 
            get; // force overrides
            set; 
        }

        ///<summary>Initializes task and ensures the supplied attributes are valid.  Hack so that 
        ///the build task can initialize this task.</summary>
        ///<param name="taskNode">Xml node used to define this task instance.</param>
        protected override void InitializeTask(System.Xml.XmlNode taskNode) {
            OptionSet specialOptions = null;
            if (OptionSetName != null) 
            {
                Project.NamedOptionSets.TryGetValue(OptionSetName, out specialOptions);
            }
            if (specialOptions == null) {
                specialOptions = new OptionSet();
            }
            InitializeFromOptionSet(specialOptions);
        }

        public abstract string WorkingDirectory {
            get; // force overrides
        }

        protected virtual string GetWorkingDirFromTemplate(string program, string defaultVal)
        {
            return defaultVal;
        }

		public Process CreateProcess(string program, string arguments) {
			// caching
			lock (_envVariables)
			{
				if (_initialized == false) 
				{
					_initialized = true;
					InitializeProcessElements();
				}
			}
			
            string workingDirectory;
            if (UseRelativePaths){
                try
                {

                    workingDirectory = GetWorkingDirFromTemplate(Path.GetDirectoryName(program), WorkingDirectory);
                }
                catch
                {
                    workingDirectory = Project.BaseDirectory;
                }

            }
            else {
                try {
                    workingDirectory = Path.GetDirectoryName(program);

                    workingDirectory = GetWorkingDirFromTemplate(workingDirectory, WorkingDirectory);
                } 
                catch {
                    workingDirectory = Project.BaseDirectory;
                }

                workingDirectory = Project.GetFullPath(workingDirectory);
            }

			Process p = new Process();
			p.StartInfo.FileName  = program;
			p.StartInfo.Arguments = arguments;
			p.StartInfo.WorkingDirectory = workingDirectory;
            p.StartInfo.EnvironmentVariables.Clear();

            Dictionary<string, string> envMap = new Dictionary<string, string>();
            // Only set environment variables explicitly stated to prevent
            // side effects unique to a person's machine.
            lock (_envVariables)
            {
                foreach (KeyValuePair<string, string> entry in _envVariables)
                {
                    envMap[entry.Key] = entry.Value;
                }
            }

            // make sure the directory of the program being executed in the path (Cygwin tools/gcc require this to work)
            DirectoryInfo info = new DirectoryInfo(workingDirectory);
            string path = info.FullName + ";";
            if(envMap.ContainsKey("PATH")) path += envMap["PATH"];
            envMap["PATH"] = path;

            ProcessUtil.SetEnvironmentVariables(envMap, p.StartInfo, true, Log);

			// NOTE: There appears to be a bug in Cygwin when spawning gcc.  If you do not
			// redirect standard input it will fail with something to do with a signal.
			// Here are some references that I used to solve the bug
			// http://sources.redhat.com/ml/cygwin/2001-11/msg01651.html
			// http://groups.google.com/groups?hl=en&lr=&ie=UTF-8&threadm=a28d10%24ohr%241%40zappa.falch.net&rnum=3&prev=/groups%3Fsourceid%3Dnavclient%26q%3Dgcc%2B%2522Win32%2Berror%2B6%2522
			// http://www.google.com/search?sourceid=navclient&q=gcc+Win32+error+6
			p.StartInfo.RedirectStandardInput = true;

			p.StartInfo.RedirectStandardOutput = true;
			p.StartInfo.RedirectStandardError = true;
			p.StartInfo.UseShellExecute = false;
			p.StartInfo.CreateNoWindow = true;

			return p;
		}

        void InitializeProcessElements() {
			// side effects unique to a person's machine.
			_envVariables.Clear();

            foreach (Property property in Project.Properties)
            {				
				if (property.Prefix == EnvironmentVariablePropertyNamePrefix) {
					_envVariables[property.Suffix] = Project.ExpandProperties(property.Value.Trim());
				}
			}
		}

        protected bool UseResponseFile {
            get {
                try {
                    string option = GetOption(UseResponseFileProperty);
                    if (option == null) {
                        return DefaultUseResponseFile;
                    }
                    return Boolean.Parse(option);
                } catch {
                    string msg = String.Format("Could not parse boolean property '{0}'.", UseResponseFileProperty);
                    throw new BuildException(msg, Location);
                }
            }
        }

        protected bool UseAltSepInResponseFile
        {
            get
            {
                try
                {
                    string option = GetOption(UseAltSepInResponseFileProperty);
                    if (option == null)
                    {
                        return DefaultUseAltSepInResponseFile;
                    }
                    return Boolean.Parse(option);
                }
                catch
                {
                    string msg = String.Format("Could not parse boolean property '{0}'.", UseAltSepInResponseFileProperty);
                    throw new BuildException(msg, Location);
                }
            }
        }

        protected bool UseRelativePaths {
            get { 
                try {
                    string option = GetOption(UseRelativePathsProperty);
                    if (option == null) {
                        return DefaultUseRelativePath;
                    }
                    return Boolean.Parse(option);
                } catch {
                    string msg = String.Format("Could not parse boolean property '{0}'.", UseRelativePathsProperty);
                    throw new BuildException(msg, Location);
                }
            }
        }

		protected bool ForceLowerCaseFilePaths 
		{
			get 
			{
				try 
				{
					string option = GetOption(ForceLowerCaseFilePathsProperty);
					if (option == null) 
					{
						return DefaultForceLowerCaseFilePaths;
					}
					return Boolean.Parse(option);
				} 
				catch 
				{
					string msg = String.Format("Could not parse boolean property '{0}'.", ForceLowerCaseFilePathsProperty);
					throw new BuildException(msg, Location);
				}
			}
		}

        protected string GetResponseFileCommandLine_one(string outputName, string outputDir, string commandLine, BufferedLog bufferedLogger)
        {
            try
            {
                // create the filename for the response file
                string fileName = outputName + ResponseFileExtension;
                fileName = Path.Combine(outputDir, fileName);
                fileName = PathNormalizer.Normalize(fileName);

                // create file and write out commandline to the response file
                if (File.Exists(fileName)) {
                    File.Delete(fileName);
                }
                StreamWriter stream = File.CreateText(fileName);
                stream.Write(commandLine);
                stream.Close();

                string msg = String.Format(LogPrefix + "{0}[\n{1}\n]", fileName, commandLine.Trim());
                if (bufferedLogger != null)
                {
                    
                    bufferedLogger.Info.WriteLine(msg);
                }
                else
                {
                    Log.Info.WriteLine(msg);
                }

                // get the response file template and format the response file for 
                // command line
                string template = GetOption(ResponseFileTemplateProperty);
                if (template == null) 
                {
                    template = DefaultResponseFileTemplate;
                }
                template = template.Replace(ResponseFileVariable, fileName);

                return template;
            }
            catch(System.Exception e)
            {
                throw new BuildException("Failed to generate response file.", e);
            }
        }

        protected string GetResponseFileCommandLine(string outputName, string outputDir, string commandLine, BufferedLog bufferedLogger)
        {
            if (commandLine != null && UseAltSepInResponseFile)
            {
                commandLine = commandLine.Replace('\\', '/');
            }
            // Try to detect if we in PC config (assume that PC config uses Visual C.
            if (_optionSet.Options.Contains("cc.defines"))
            {
                string defines = _optionSet.Options["cc.defines"];
                if (!(!String.IsNullOrEmpty(defines) && (defines.IndexOf("WIN32") > -1 || defines.IndexOf("_XBOX") > -1)))
                {
                    return GetResponseFileCommandLine_one(outputName, outputDir, commandLine, bufferedLogger);
                }
            }
            else
            {
                return GetResponseFileCommandLine_one(outputName, outputDir, commandLine, bufferedLogger);
            }
            // VC compiler does not accept command lines longer than:
            const int maxLen = 131070;

            System.Text.StringBuilder outputCmdLine = new System.Text.StringBuilder();

            int startIndex = maxLen-1;
            int splitPos = -1;
            int begin = 0;
            StringCollection cmdArray = new StringCollection();

            while (startIndex <= commandLine.Length - 1)
            {
                //need to split command line and generate several rsp files:
                splitPos = commandLine.LastIndexOf('\n', startIndex, maxLen);
                if (splitPos == -1)
                {
                    
                    // Try to use space that is not inside quoted strings
                    Regex re = new System.Text.RegularExpressions.Regex(@"""[^""]*"" | [^\s]+", RegexOptions.IgnorePatternWhitespace|RegexOptions.RightToLeft);
                    Match theMatch = re.Match(commandLine, startIndex-maxLen, maxLen);
                    if (theMatch.Success)
                    {
                        // Reposition split index from match (whic is the next char after space to the space position
                        splitPos = theMatch.Index-1;
                    }
                }

                if (splitPos != -1)
                {                    
                    string partial = commandLine.Substring(begin, splitPos - begin+1);
                    cmdArray.Add(partial);
                    startIndex = splitPos+maxLen;
                    begin = splitPos + 1;
                }
                else
                {
                    // Can not split, leave this block as is. cl will likely fail
                    startIndex = startIndex + maxLen;                    
                }
            }
            if (begin < commandLine.Length)
            {
                if (begin == 0)
                {
                    cmdArray.Add(commandLine);
                }
                else
                {
                    cmdArray.Add(commandLine.Substring(begin));
                }
            }

            for(int i = 0; i < cmdArray.Count; i++)
            {
                string outputNameWithPostfix = outputName;
                if (i > 0)
                {
                    outputNameWithPostfix += "_" + i;
                }
                string outCmd = GetResponseFileCommandLine_one(outputNameWithPostfix, outputDir, cmdArray[i], bufferedLogger);

                outputCmdLine.Append(outCmd);
                outputCmdLine.Append(" ");
            }

            return outputCmdLine.ToString();
        }

		protected void AddDelimitedProperties(StringCollection values, string property) {
			if (property != null) {
				foreach (string line in property.Split(new char[] {';', '\n'})) {
					string trimmed = line.TrimWhiteSpace();
					if (trimmed.Length > 0) {
						values.Add(trimmed);
					}
				}
			}
		}

	}
}