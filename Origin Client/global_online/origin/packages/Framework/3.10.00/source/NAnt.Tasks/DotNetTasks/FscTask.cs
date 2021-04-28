// NAnt - A .NET build tool
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
// Mike Krueger (mike@icsharpcode.net)

using System;
using System.IO;
using System.Collections.Specialized;
//using System.Xml;

using NAnt.Core.Attributes;
using NAnt.Core;
using NAnt.Core.Util;
using NAnt.Core.Logging;

namespace NAnt.DotNetTasks
{

    /// <summary>Compiles C# programs using csc, Microsoft's C# compiler.
    /// <para>
    /// Command line switches are passed using this syntax: <b>&lt;arg value=""/></b>.
    /// See examples below to see how it can be used outside OR inside the <b>&lt;args></b> tags.
    /// </para>
    /// </summary>
    /// <include file="Examples/Csc/HelloWorld.example" path="example"/>
    /// <include file="Examples/Csc/HelloArgs.example" path="example"/>
    /// <include file="Examples/Csc/Resources.example" path="example"/>
    /// <include file="Examples/Csc/WindowsForms.xml" path="example"/>
    [TaskName("fsc")]
    public class FscTask : CompilerBase
    {

        public FscTask() : base("fsc") { }

        string _doc;
        StringWriter mWriter;

        public bool useDebugProperty = true;

        /// <summary>The name of the XML documentation file to generate.  This attribute 
        /// corresponds to the <c>/doc:</c> flag.  By default, documentation is not 
        /// generated.</summary>
        [TaskAttribute("doc")]
        public string Doc
        {
            get { return _doc; }
            set { _doc = value; }
        }

        /// <summary>A set of command line arguments.</summary>
        /// note this method exists solely for auto doc generation (nant.documenter)
        [ArgumentSet("args", Initialize = true)]
        public ArgumentSet ArgSet
        {
            get { return _argSet; }
        }

        /// <summary>Write an option using the default output format.</summary>
        protected override void WriteOption(TextWriter writer, string name)
        {
            writer.Write("--{0} ", name);
        }

        /// <summary>Write an option and its value using the default output format.</summary>
        protected override void WriteOption(TextWriter writer, string name, string arg)
        {
            // Always quote arguments ( )
            writer.Write("--{0}:{1} ", name, arg);
        }
        protected void WriteOptionOn(TextWriter writer, string name)
        {
            writer.Write("--{0}+ ", name);
        }
        protected void WriteOptionOff(TextWriter writer, string name)
        {
            writer.Write("--{0}- ", name);
        }

        protected override void ExecuteTask()
        {
            if (NeedsCompiling())
            {
                // create temp response file to hold compiler options
                mWriter = new StringWriter();

                try
                {
                    if (References.BaseDirectory == null)
                    {
                        References.BaseDirectory = BaseDirectory;
                    }
                    if (Modules.BaseDirectory == null)
                    {
                        Modules.BaseDirectory = BaseDirectory;
                    }
                    if (Sources.BaseDirectory == null)
                    {
                        Sources.BaseDirectory = BaseDirectory;
                    }

                    Log.Status.WriteLine(LogPrefix + Path.GetFileName(GetOutputPath()));

                    // specific compiler options
                    WriteOptions(mWriter);

                    WriteOption(mWriter, "target", OutputTarget);
                    if (Define != null && Define != string.Empty)
                    {
                        WriteOption(mWriter, "define", Define);
                    }

                    WriteOption(mWriter, "out", GetOutputPath());
                    //if (Win32Icon != null && Win32Icon != string.Empty) {
                    //    WriteOption(mWriter, "win32icon", Win32Icon);                        
                    //}

                    foreach (FileItem fileItem in References.FileItems)
                    {
                        WriteOption(mWriter, "reference", fileItem.FileName);
                    }
                    //foreach (FileItem fileItem in Modules.FileItems) {
                    //    WriteOption(mWriter, "addmodule", fileItem.FileName);
                    //}

                    // compile resources
                    if (Resources.ResxFiles.FileItems.Count > 0)
                    {
                        CompileResxResources(Resources.ResxFiles);
                    }

                    // Resx args
                    foreach (FileItem fileItem in Resources.ResxFiles.FileItems)
                    {
                        string prefix = GetFormNamespace(fileItem.FileName); // try and get it from matching form
                        string className = GetFormClassName(fileItem.FileName);
                        if (prefix == null)
                        {
                            prefix = Resources.Prefix;
                        }
                        string tmpResourcePath = Path.ChangeExtension(fileItem.FileName, "resources");
                        string manifestResourceName = Path.GetFileName(tmpResourcePath);
                        if (prefix != "")
                        {
                            string actualFileName = Path.GetFileNameWithoutExtension(fileItem.FileName);
                            if (className != null)
                                manifestResourceName = manifestResourceName.Replace(actualFileName, prefix + "." + className);
                            else
                                manifestResourceName = manifestResourceName.Replace(actualFileName, prefix + "." + actualFileName);
                        }
                        string resourceoption = tmpResourcePath + "," + manifestResourceName;
                        WriteOption(mWriter, "resource", resourceoption);
                    }

                    // other resources
                    foreach (FileItem fileItem in Resources.NonResxFiles.FileItems)
                    {
                        string baseDir = Path.GetFullPath(Resources.BaseDirectory);
                        string manifestResourceName = fileItem.FileName.Replace(baseDir, null);
                        manifestResourceName = manifestResourceName.TrimStart(Path.DirectorySeparatorChar);
                        manifestResourceName = manifestResourceName.Replace(Path.DirectorySeparatorChar, '.');
                        if (Resources.Prefix != "")
                        {
                            manifestResourceName = Resources.Prefix + "." + manifestResourceName;
                        }
                        string resourceoption = fileItem.FileName + "," + manifestResourceName;
                        WriteOption(mWriter, "resource", resourceoption);
                    }

                    foreach (FileItem fileItem in Sources.FileItems)
                    {
                        mWriter.Write("\"" + fileItem.FileName + "\" ");
                    }

                    // Make sure to close the response file otherwise contents
                    // will not be written to EXecuteTask() will fail.
                    mWriter.Close();

                    // display response file contents
                    Log.Info.WriteLine(LogPrefix + this.ProgramFileName + " " + this.ProgramArguments);

                    // call base class to do the work
                    base.ExecuteTask();

                    // clean up generated resources.
                    if (_resgenTask != null)
                    {
                        _resgenTask.RemoveOutputs();
                    }

                }
                finally
                {
                    // make sure we delete response file even if an exception is thrown
                    mWriter.Close(); // make sure stream is closed or file cannot be deleted
                }
            }
        }
        public override string ProgramArguments
        {
            get { return mWriter.ToString(); }
        }

        public override string ProgramFileName
        {
            get
            {
                if (Compiler == null)
                {
                    // Instead of relying on the .NET compilers to be in the user's path point
                    // to the compiler directly since it lives in the .NET Framework's runtime directory
                    if (PlatformUtil.IsWindows)
                    {
                        return Path.Combine(Path.GetDirectoryName(System.Runtime.InteropServices.RuntimeEnvironment.GetRuntimeDirectory()), Name);
                    }

                    if (PlatformUtil.IsUnix || PlatformUtil.IsOSX)
                    {
                        string runtimeVersion = System.Runtime.InteropServices.RuntimeEnvironment.GetSystemVersion();
                        switch (runtimeVersion)
                        {
                            case "v1.1.4322":
                                return "/usr/bin/mcs";
                            case "v2.0.50727":
                            default:
                                return "/usr/bin/gmcs";
                        }
                    }
                }
                return Compiler;
            }
        }

        protected override void WriteOptions(TextWriter writer)
        {
            if (_doc != null)
            {
                WriteOption(writer, "doc", _doc);
            }

            if (useDebugProperty)
            {
                if (Debug)
                {
                    WriteOptionOn(writer, "debug");
                }
                else
                {
                    WriteOptionOn(writer, "optimize");
                }
            }
        }

        protected override string GetExtension()
        {
            return "fs";
        }
    }
}
