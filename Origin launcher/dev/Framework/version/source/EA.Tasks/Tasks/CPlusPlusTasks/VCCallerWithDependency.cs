// Copyright (C) 2019 Electronic Arts Inc.
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
// Electronic Arts (Frostbite.Team.CM@ea.com)

using System.Collections.Generic;
using System.Text;
using System.IO;
using System.Diagnostics;

namespace EA.CPlusPlusTasks
{
    // A wrapper to call Visual Studio's C++ compiler or assembler to automatically generate a .d dependency file
    // like the clang compiler's -MMD options.

    public class VCCallerWithDependency
    {
        public enum ProgramType
        {
            COMPILER = 0,
            ASSEMBLER  = 1
        };

        public VCCallerWithDependency(ProgramType programType, string srcFile, string objFile, string depFile, bool verbose)
        {
            mProgramType = programType;
            mSrcFile = srcFile;
            mObjFile = objFile;
            mDepFile = depFile;
            mVerbose = verbose;
        }

        internal ProgramType mProgramType;
        internal bool mVerbose;
        internal string mSrcFile;
        internal string mObjFile;
        internal string mDepFile;
        internal StringBuilder mDependencyContent = new StringBuilder();
        internal HashSet<string> mDependencyAlreadyAdded = new HashSet<string>();

        internal void OutputDataReceivedEventHandler(object sender, DataReceivedEventArgs e)
        {
            if (e == null || e.Data == null)
                return;

            if (e.Data.StartsWith("Note: including file:"))
            {
                string includeLine = e.Data.Replace("Note: including file:", "").Trim().Replace("\\","/");
                if (includeLine.Contains(" "))
                {
                    includeLine = "\"" + includeLine + "\"";
                }
                if (!mDependencyAlreadyAdded.Contains(includeLine))
                {
                    mDependencyAlreadyAdded.Add(includeLine);
                    mDependencyContent.Append(" \\" + System.Environment.NewLine + "  " + includeLine);
                }
            }
            else
            {
                System.Console.WriteLine(e.Data);
            }
        }

        internal void ErrorDataReceivedEventHandler(object sender, DataReceivedEventArgs e)
        {
            if (e == null || e.Data == null)
                return;

            System.Console.Error.WriteLine(e.Data);
        }

        public static int Execute(ProgramType ptype, string clExe, List<string> clArguments, string srcFile, string objFile, string depFile, bool verbose)
        {
            VCCallerWithDependency vcCaller = new VCCallerWithDependency(ptype,srcFile,objFile,depFile,verbose);
            return vcCaller.Execute(clExe, clArguments);
        }

        public int Execute(string clExe, List<string> clArguments)
        {
            // Note C# argument input from Main(string[] args) will loose the quotes.  We need to rebuild a proper command line.
            StringBuilder cmdline = new StringBuilder();
            foreach (string arg in clArguments)
            {
                // Rebuild command line and re-escape the quotes if necessary.
                if (arg.Contains(" "))
                {
                    if (arg.Contains("\""))
                    {
                        int insertFirstQuotePos = 0;
                        int firstQuoteIdx = arg.IndexOf('\"');
                        if (firstQuoteIdx > 0 && (arg[firstQuoteIdx - 1] == ':' || arg[firstQuoteIdx - 1] == '='))
                        {
                            // If the first quote started right after a ':' or '=' symbol, we'll preserve that syntax.
                            insertFirstQuotePos = firstQuoteIdx;
                        }
                        string newArg = arg.Replace("\\\"", "\\\\\"");
                        newArg = newArg.Replace("\"", "\\\"");
                        newArg = newArg.Insert(insertFirstQuotePos, "\"") + "\"";
                        cmdline.Append(" " + newArg);
                    }
                    else
                    {
                        cmdline.Append(" \"" + arg + "\"");
                    }
                }
                else
                {
                    cmdline.Append(" " + arg);
                }
            }

            return Execute(clExe, cmdline.ToString().Trim());
        }

        public int Execute(string clExe, string commandline)
        {
            // Make sure Visual Studio compiler's /showIncludes is on command line.  If not, add it.
            // There are no special switches for assembler. So for assembler, we'll just list the source file only.
            string newCommandLine = commandline;
            if (mProgramType == ProgramType.COMPILER && !commandline.Contains("/showIncludes") && !commandline.Contains("-showIncludes"))
            {
                newCommandLine = "/showIncludes " + commandline;
            }

            if (mVerbose)
            {
                System.Console.WriteLine("\"{0}\" {1}", clExe, newCommandLine);
            }

            string dependencyFileName = string.IsNullOrEmpty(mDepFile) ? Path.ChangeExtension(mObjFile, ".d") : mDepFile;

            mDependencyContent.Clear();
            mDependencyContent.Append(mObjFile + ":");
            mDependencyContent.Append(" \\" + System.Environment.NewLine + "  " + mSrcFile.Replace("\\","/"));

            // Now setup and execute the process
            Process proc = new Process();
            proc.StartInfo.FileName = clExe;
            proc.StartInfo.Arguments = newCommandLine;
            proc.StartInfo.RedirectStandardOutput = true;
            proc.StartInfo.RedirectStandardError = true;
            proc.StartInfo.RedirectStandardInput = true;
            proc.StartInfo.UseShellExecute = false;
            proc.StartInfo.CreateNoWindow = true;

            proc.OutputDataReceived += new DataReceivedEventHandler(OutputDataReceivedEventHandler);
            proc.ErrorDataReceived += new DataReceivedEventHandler(ErrorDataReceivedEventHandler);

            proc.Start();
            proc.BeginOutputReadLine();
            proc.BeginErrorReadLine();
            proc.WaitForExit();

            // Now create the dependency file
            File.WriteAllText(dependencyFileName, mDependencyContent.ToString());

            return proc.ExitCode;
        }

    }
}
