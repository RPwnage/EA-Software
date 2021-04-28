// Originally based on NAnt - A .NET build tool
// Copyright (C) 2003-2018 Electronic Arts Inc.
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

using System;
using System.Collections.Generic;


using EA.Make.MakeItems;

namespace EA.Make
{
	public class MakeFileBase 
	{
		public enum MakeProgram { VSIMAKE, GNUMAKE };

		internal MakeFileBase()
		{
		}

		public MakeEnvironment ENVIRONMENT
		{
			get
			{
				if (_environment == null)
				{
					_environment = new MakeEnvironment();
					Items.Add(_environment);
				}
				return _environment;
			}
		}
		private MakeEnvironment _environment;

		public string COMMENT
		{
			set
			{
				Items.Add(new MakeComment(value ?? String.Empty));
			}
		}

		public string LINE
		{
			set
			{
				Items.Add(new MakeLine(value ?? String.Empty));
			}
		}


		public string HEADER
		{
			set
			{
				Items.Add(new MakeComment(value ?? String.Empty, isHeader: true));
			}
		}

		public MakeUniqueStringCollection PHONY
		{
			get
			{
				if (_phony == null)
				{
					_phony = new MakeSpecialTarget(".PHONY");
					Items.Add(_phony);
				}
				return _phony.Prerequisites;
			}
			set
			{
				_phony.Prerequisites = value;
			}

		}
		private MakeSpecialTarget _phony;

		public MakeUniqueStringCollection SUFFIXES
		{
			get
			{
				if (_suffixes == null)
				{
					_suffixes = new MakeSpecialTarget(".SUFFIXES");
					Items.Add(_suffixes);
				}
				return _suffixes.Prerequisites;
			}
			set
			{
				_suffixes.Prerequisites = value;
			}
		}
		private MakeSpecialTarget _suffixes;

		public MakeVariableScalar Variable(string name, string value = null)
		{
			var variable = new MakeVariableScalar(name, value);
			Items.Add(variable);

			return variable;
		}

		public MakeVariableArray VariableMulti(string name, IEnumerable<string> values = null)
		{
			var variable = new MakeVariableArray(name, values);
			Items.Add(variable);

			return variable;
		}

		public MakeTarget Target(string name)
		{
			var variable = new MakeTarget(name);
			Items.Add(variable);
			return variable;
		}

		public MakeFragment Fragment(string header = null)
		{
				var fragment = new MakeFragment(header);
				Items.Add(fragment);
				return fragment;
		}

		public readonly List<IMakeItem> Items = new List<IMakeItem>();

	}
}
