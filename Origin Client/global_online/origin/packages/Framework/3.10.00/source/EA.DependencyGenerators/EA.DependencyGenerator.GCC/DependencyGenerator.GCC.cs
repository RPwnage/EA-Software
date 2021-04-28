using System;
using System.Collections.Specialized;
using System.IO;

namespace EA {
    namespace DependencyGenerator
    {
        public class DependencyGenerator : IDependencyGenerator
        {
            public DependencyGenerator()
            {
                _ignoreMissingIncludes = true; ;
                _suppressWarnings = true;
            }

            override public void Dispose()
            {
            }

            override public void Reset()
            {
            }

            override public void AddIncludePath(String includePath)
            {
            }

            override public void AddIncludePath(String includePath, bool systemInclude)
            {
            }

            override public void AddUsingPath(String usingPath)
            {
            }

            override public void AddDefine(String define)
            {
            }

            override public StringCollection ParseFile(String path)
            {
                StringCollection dependencies = new StringCollection();

                if (!File.Exists(_dependencyFilePath))
                {
                    OutputWarning("Input dependency file '" + _dependencyFilePath + "' does not exist");
                    throw new System.Exception("Missing input dependency file '" + _dependencyFilePath + "'");
                }

                using (StreamReader reader = new StreamReader(_dependencyFilePath))
                {
                    bool skip = true;
                    string oneLine;
                    while ((oneLine = reader.ReadLine()) != null)
                    {
						oneLine = oneLine.Trim(TRIM_CHARS); 
						oneLine = oneLine.TrimEnd(TRIM_END_CHARS);
                        if (!String.IsNullOrEmpty(oneLine))
                        {
							foreach(string depPath in GetPath(oneLine))
                            {
								if(!skip) //Skip obj : cpp
								{
                                dependencies.Add(depPath);
                            }
								else
								{
                                    if (!depPath.EndsWith(":", StringComparison.Ordinal))
										skip = false;
								}
                        }
                    }
                }
                }
                return dependencies;
            }

            override public bool SkipDuplicates
            {
                get { return _skipDuplicates; }
                set { _skipDuplicates = value; }
            }

            override public bool ShowDuplicates
            {
                get { return _showDuplicates; }
                set { _showDuplicates = value; }
            }

            override public bool IgnoreMissingIncludes
            {
                get { return _ignoreMissingIncludes; }
                set { _ignoreMissingIncludes = value; }
            }

            override public bool SuppressWarnings
            {
                get { return _suppressWarnings; }
                set { _suppressWarnings = value; }
            }

            override public string DependencyFilePath
            {
                get { return _dependencyFilePath; }
                set { _dependencyFilePath = value; }
            }

            private void OutputWarning(string warn)
            {
                if (!_suppressWarnings)
                {
                    Console.WriteLine(LOG_PREFIX + warn);
                }
            }

            private string[] GetPath(string line)
            {
				string[] path = line.Split(SPLIT_CHARS, StringSplitOptions.RemoveEmptyEntries);
                return path;
            }

            private bool _skipDuplicates;
            private bool _showDuplicates;
            private bool _ignoreMissingIncludes;
            private bool _suppressWarnings;

            private string _dependencyFilePath;

            private static readonly string LOG_PREFIX = "[DependencyGenerator.GCC] ";
            private static readonly char[] TRIM_CHARS = new char[] { '\n', '\r', '\t', ' ', '"' };
			private static readonly char[] TRIM_END_CHARS  = new char[] { '\\', ' ' };
			private static readonly char[] SPLIT_CHARS = new char[] {' ' };
        }
    }
}
