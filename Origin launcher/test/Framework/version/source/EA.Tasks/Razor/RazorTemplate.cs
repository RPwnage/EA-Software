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
using System.IO;

using NAnt.Core;
using NAnt.Core.Logging;
using Model = EA.FrameworkTasks.Model;

namespace EA.Razor
{
	public class RazorTemplate<TModel> : IDisposable
	{
		private readonly StringWriter _writer;

		public RazorTemplate()
		{
			_writer = new StringWriter();
		}

		public string Result
		{
			get
			{
				_writer.Flush();
				return _writer.ToString();
			}
		}

		public virtual TModel Model
		{
			get;
			set;
		}

		public Model.IModule Module
		{
			get;
			set;
		}

		public Project Project
		{
			get;
			set;
		}

		public Log Log
		{
			get
			{
				return Project.Log;
			}
		}


		#region Razor Interface

		// This method is overridden by the System.WEb.Razor Engine with code that actually writes result.
		public virtual void Execute()
		{
			// implementation to deal with modern razor version
			ExecuteAsync().Wait();
		}

		public virtual System.Threading.Tasks.Task ExecuteAsync()
		{
			throw new NotImplementedException();
		}

		public virtual void WriteLiteral(object value)
		{
			_writer.Write(value);
		}

		public virtual void Write(object value)
		{
			_writer.Write(value);
		}

		// See https://github.com/aspnet/Razor/issues/177 & https://github.com/aspnet/Razor/pull/554 for a (poor) explanation about the refactoring of this API
		System.Collections.Generic.Stack<(string name, string suffix)> m_openAttribStack = new System.Collections.Generic.Stack<(string, string)>();

		public virtual void BeginWriteAttribute(string name, string prefix, int prefixOffset, string suffix, int suffixOffset, int attributeValuesCount)
		{
			m_openAttribStack.Push((name, suffix));

			WriteLiteral(prefix);
		}

		public void WriteAttributeValue(string prefix, int prefixOffset, object value, int valueOffset, int valueLength, bool isLiteral)
		{
			WriteLiteral(prefix + value);
		}

		public virtual void EndWriteAttribute()
		{
			var attrib = m_openAttribStack.Pop();
			WriteLiteral(attrib.suffix);
		}

		public virtual void WriteAttribute(string name, Tuple<string, int> startTag, Tuple<string, int> endTag, params object[] values)
		{
			if (values.Length == 0)
			{
				//Explicitly empty attribute in the template
				WriteLiteral(startTag.Item1);
				WriteLiteral(endTag.Item1);
			}
			else
			{
				bool hasValue = false;

				foreach (var value in values)
				{
					string str = null;
					var val = value as Tuple<Tuple<string, int>, Tuple<object, int>, bool>;
					if (val != null)
					{
						if (val.Item2 != null && val.Item2.Item1 != null)
						{
							str = val.Item2.Item1.ToString();
						}
					}
					else
					{
						var val1 = value as Tuple<Tuple<string, int>, Tuple<string, int>, bool>;
						if (val1 != null && val1.Item2 != null && val1.Item2.Item1 != null)
						{
							str = val1.Item2.Item1.ToString();
						}
					}

					if (!String.IsNullOrEmpty(str))
					{

						if (hasValue == false)
						{
							WriteLiteral(startTag.Item1);
							hasValue = true;
						}

						WriteLiteral(str);
					}
				}
				if (hasValue)
				{
					WriteLiteral(endTag.Item1);
				}
			}
		}



		#endregion

		#region Dispose interface implementation

		public void Dispose()
		{
			Dispose(true);
			GC.SuppressFinalize(this);
		}

		void Dispose(bool disposing)
		{
			if (!this._disposed)
			{
				if (disposing)
				{
					_writer.Dispose();
				}
			}
			_disposed = true;
		}
		private bool _disposed = false;

		#endregion

	}
}
