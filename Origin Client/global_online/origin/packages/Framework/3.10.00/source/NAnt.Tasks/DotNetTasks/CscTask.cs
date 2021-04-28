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
    [TaskName("csc")]
    public class CscTask : CompilerBase
    {
        public CscTask() : base("csc") { }

        string _doc;

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

        public bool useDebugProperty = true;

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

                        string comp = "dmcs";
                        switch (runtimeVersion)
                        {
                            case "v1.1.4322":
                                comp = "mcs";
                                break;
                            case "v2.0.50727":
                                comp = "gmcs";
                                break;
                            case "v4.0.30319":
                            default:
                                comp = "dmcs";
                                break;
                        }

                        if (File.Exists("/usr/bin/" + comp))
                            return "/usr/bin/" + comp;
                        else if (File.Exists("/opt/novell/" + comp))
                            return "/opt/novell/bin/" + comp;
                        else
                            return comp;
                    }
                }
                return Compiler;
            }
        }

        protected override void WriteOptions(TextWriter writer)
        {
            WriteOption(writer, "fullpaths");
            if (_doc != null)
            {
                WriteOption(writer, "doc", _doc);
            }

            if (useDebugProperty)
            {
                if (Debug)
                {
                    WriteOption(writer, "debug");
                    WriteOption(writer, "define", "DEBUG");
                    WriteOption(writer, "define", "TRACE");
                }
                else
                {
                    WriteOption(writer, "optimize");
                }
            }
        }

        protected override string GetExtension()
        {
            return "cs";
        }
    }
}
