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
// File Maintainer
// Gerry Shaw (gerry_shaw@yahoo.com)

using System;
using System.IO;

using NAnt.Core.Attributes;

namespace NAnt.Core.Tasks {

    /// <summary>Allows wrapping of a group of tasks to be executed based on a conditional.</summary>
    /// <remarks>
    /// <para>Use the do task in conjunction with the if and unless attributes to execute a series
    /// of tasks based on the condition.</para>
    /// <para>Another use for the task is when you want to execute a task only if a property is
    /// defined.  Normally nant will try to expand the property at the same time performing
    /// the if/unless condition check.</para>
    /// </remarks>
    /// <include file='Examples\Do\Simple.example' path='example'/>
    /// <include file='Examples\Do\Property.example' path='example'/>
    [TaskName("do")]
    public class DoTask : TaskContainer{
    }
}
