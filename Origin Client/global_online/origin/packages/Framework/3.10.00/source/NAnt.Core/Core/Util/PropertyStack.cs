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
// File Maintainers:
// Gerry Shaw (gerry_shaw@yahoo.com)

using System;
using System.Collections.Generic;

namespace NAnt.Core.Util
{
    /// <summary>
    /// Use a stack for defining properties so that properties can be defined recusively within build tasks.
    /// This class works by storing previous values of properties and restoring them when they are removed.
    /// </summary>
    /// <code>
    ///		new stack('myprop')
    ///		stack.Add('val')			// myprop == val1
    ///		
    ///			stack.Add('val2')		// myprop == val2
    ///			stack.Remove			// myprop == val1
    ///
    ///		stack.Remove				// myprop = undef
    ///</code>
    internal class PropertyStack
    {
        Stack<string> _stack = new Stack<string>();
        string _name = null;

        /// <summary>
        /// Construct a PropertyStack
        /// </summary>
        /// <param name="name">The name of the property.</param>
        public PropertyStack(string name)
        {
            _name = name;
        }

        /// <summary>
        /// Adds a project property.  
        /// If one already exists it will be stored to be restored later when the new 
        /// one is removed.
        /// </summary>
        public void Push(Project project, string value)
        {
            Property prop;
            if (project.Properties.LocalContext.TryGetValue(_name, out prop))
            {
                string p = (prop.Deferred) ? project.ExpandProperties(prop.Value) : prop.Value;
                _stack.Push(p);
                prop.Value = value;                
            }
            else
            {
                prop = new Property(_name, value);
            }
            project.Properties.LocalContext[_name] = prop;
        }

        /// <summary>
        /// Removes a project property.  
        /// If one previously existed it will be restored.
        /// </summary>
        public string Pop(Project project)
        {
            string p = "";
            project.Properties.LocalContext.Remove(_name);

            if (_stack.Count != 0)
            {
                p = _stack.Pop();
                project.Properties.LocalContext.Add(_name, new Property(_name, p));
            }

            return p;
        }

        public void Clear(Project project)
        {
            project.Properties.LocalContext.Remove(_name);
            _stack.Clear();
        }
    }
}
