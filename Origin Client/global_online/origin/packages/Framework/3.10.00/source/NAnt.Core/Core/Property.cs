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
using System.Collections.Concurrent;
using System.Text.RegularExpressions;

namespace NAnt.Core
{

    public class Property
    {
        public static readonly string PrefixSeperator = ".";

        public  readonly string Name;
        string _value;
        bool _readOnly;
        bool _deferred;

        public Property(string name, string value) :
             this(name, value, false, false)
        {
        }

        public Property(string name, string value, bool readOnly)
            : this(name, value, readOnly, false)
        {
        }

        public Property(string name, string value, bool readOnly, bool deferred)
        {           
            Name = name;
            _value = value;
            _readOnly = readOnly;
            _deferred = deferred;
        }

        internal Property(Property other)
        {
            Name = other.Name;
            _value = other._value;
            _readOnly = other._readOnly;
            _deferred = other._deferred;
        }


        /// <summary>Validate property name.</summary>
        /// <remarks>
        ///   <para>If the property name is invalid a BuildException will be thrown.</para>
        ///   <para>A valid property name contains any word character, '-' character, or '.' character.</para>
        /// </remarks>
        /// <param name="name">The name of the property to check.</param>
        public static bool VerifyPropertyName(string name)
        {
            for (int i = 0; i < name.Length; i++)
            {
                var ch = name[i];
                if (!(Char.IsLetterOrDigit(ch) || ch == '_' || ch == '.' || ch == '-' || ch == '+' || ch == '/'))
                    return false;
            }
            return true;
        }

        public string Value
        {
            get { return _value; }
            set
            {
                if (ReadOnly)
                {
                    string msg = String.Format("Property '{0}' is readonly and cannot be modified.", Name);
                    throw new BuildException(msg);
                }
                _value = value;
            }
        }

        public bool Deferred
        {
            get { return _deferred; }
            set { _deferred = value; }
        }

        public string Prefix
        {
            get { return GetPrefix(Name); }
        }

        /// <summary>Get the prefix of the specified property name.</summary>
        /// <example>package.Framework.dir => package.Framework</example>
        public static string GetPrefix(string name)
        {
            int index = name.LastIndexOf('.');
            if (index >= 0)
            {
                return name.Substring(0, index);
            }
            return name;
        }

        public string Suffix
        {
            get { return GetSuffix(Name); }
        }

        /// <summary>Get the suffix of the specified property name.</summary>
        /// <example>package.Framework.dir => dir</example>
        public static string GetSuffix(string name)
        {
            int index = name.LastIndexOf('.');
            if (index >= 0)
            {
                return name.Substring(index + 1, name.Length - index - 1);
            }
            return name;
        }

        public static string Combine(string prefix, string name)
        {
            return (prefix + PrefixSeperator + name);
        }

        public bool ReadOnly
        {
            get { return _readOnly; }
            set { _readOnly = value; }
        }
    }
}
