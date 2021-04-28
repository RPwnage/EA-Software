// NAnt - A .NET build tool
// Copyright (C) 2001-2002 Gerry Shaw
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

using NAnt.Core.Attributes;

namespace NAnt.DotNetTasks {

    /// <summary>Compiles Microsoft JScript.NET programs using jsc.</summary>
    // <include file='Examples/Jsc/Jsc.example' path='example'/>
    [TaskName("jsc")]
    public class JscTask : CompilerBase {

        public JscTask() : base("jsc") { }

        // add JScript.NET specific compiler options here (see CscTask)
        protected override void WriteOptions(TextWriter writer) {
            if (Debug) {
                writer.WriteLine("/debug");
                writer.WriteLine("/define:DEBUG;TRACE");
            }
        }

        protected override string GetExtension(){ return "js";}
    }
}
