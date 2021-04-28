// NAnt - A .NET build tool
// Copyright (C) 2001 Gerry Shaw
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
using System;
using System.Reflection;

namespace NAnt.Core.Attributes
{

    /// <summary>Indicates that field should be able to be converted into a Int32 within the given range.</summary>
    [AttributeUsage(AttributeTargets.Field | AttributeTargets.Property, Inherited = true)]
    public sealed class Int32ValidatorAttribute : ValidatorAttribute
    {

        int _minValue = Int32.MinValue;
        int _maxValue = Int32.MaxValue;

        public Int32ValidatorAttribute()
        {
        }

        public Int32ValidatorAttribute(int minValue, int maxValue)
        {
            MinValue = minValue;
            MaxValue = maxValue;
        }

        public int MinValue
        {
            get { return _minValue; }
            set { _minValue = value; }
        }

        public int MaxValue
        {
            get { return _maxValue; }
            set { _maxValue = value; }
        }

        public override void Validate(ref object value)
        {
            if (value.GetType() != typeof(System.Int32))
            {
                string msg = String.Format("Value is not of type {0}", typeof(System.Int32).ToString());
                throw new Exception(msg);
            }
            Int32 intValue = Convert.ToInt32(value);
            if (intValue < MinValue || intValue > MaxValue)
            {
                string msg = String.Format("Cannot resolve '{0}' to integer between '{1}' and '{2}'.", intValue.ToString(), MinValue, MaxValue);
                throw new Exception(msg);
            }
        }
    }
}