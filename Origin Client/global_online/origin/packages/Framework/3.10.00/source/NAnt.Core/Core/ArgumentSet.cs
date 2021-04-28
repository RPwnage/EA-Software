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
// Kosta Arvanitis (karvanitis@ea.com)
using System;
using System.Xml;
using System.Text;
using System.Collections.Generic;

using NAnt.Core.Attributes;

namespace NAnt.Core
{
    public class ArgumentSet : ConditionalElement
    {
        List<string> _args = new List<string>();

        public List<string> Arguments
        {
            get { return _args; }
        }

        [XmlElement("arg", "NAnt.Core.ArgumentElement", Description="Argument value. Can provide text or file or both" )]
        protected override void InitializeElement(XmlNode elementNode)
        {
            // initialize the _args collection
            foreach (XmlNode node in elementNode)
            {
                if (node.Name.Equals("arg"))
                {
                    // TODO: decide if we should enforce arg elements not being able
                    // to accept a file and value attribute on the same element.
                    // Ideally this would be done via schema and since it doesn't
                    // really hurt for now I'll leave it in.
                    ArgumentElement arg = new ArgumentElement();
                    arg.Project = Project;
                    arg.Initialize(node);

                    if (arg.IfDefined && !arg.UnlessDefined)
                    {
                        if (arg.Value != null)
                        {
                            _args.Add(Project.ExpandProperties(arg.Value));
                        }

                        if (arg.File != null)
                        {
                            _args.Add(Project.GetFullPath(Project.ExpandProperties(arg.File)));
                        }
                    }
                }

                // KA: We can't check for non-arg elements here due to the restrictions we face with supporting 
                // backwards compatability. If we were to insert the error check here we could not contain arg 
                // elements outside of any argumentset, which is how it used to be done.
            }
        }

        public override string ToString()
        {
            StringBuilder sb = new StringBuilder();

            foreach (string arg in Arguments)
            {
                sb.Append(arg);
                sb.Append(" ");
            }

            return sb.ToString().Trim();
        }
    }

    public class ArgumentElement : ConditionalElement
    {
        string _value = null;
        string _file = null;

        /// <summary>Text value</summary>
        [TaskAttribute("value", Required = false, ExpandProperties = false)]
        public string Value
        {
            set { _value = value; }
            get { return _value; }
        }

        /// <summary>File Path. Relative file path is combined with current project base path.</summary>
        [TaskAttribute("file", Required = false)]
        public string File
        {
            set { _file = value; }
            get { return _file; }
        }

        protected override void InitializeElement(XmlNode node)
        {
            // sanity check
            if (Value == null && File == null)
            {
                throw new BuildException("Invalid arg element.", Location);
            }
        }
    }
}
