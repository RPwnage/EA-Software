// NAnt - A .NET build tool
// Copyright (C) 2003-2015 Electronic Arts Inc.
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
// Electronic Arts (Frostbite.Team.CM@ea.com)
// Gerry Shaw (gerry_shaw@yahoo.com)
using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using NAnt.Core.Logging;

namespace NAnt.Core
{

	public class TargetCollection : ConcurrentDictionary<string, Target>
	{
		private readonly Log Log;

		public TargetCollection(Log log)
			: base()
		{
			Log = log;
		}

		internal TargetCollection(Log log, TargetCollection other)
			: base(other)
		{
			Log = log;
		}


		public Target Find(string targetName)
		{
			Target target;
			if (!TryGetValue(targetName, out target))
			{
				target = null;
			}
			return target;
		}

		public bool Contains(string name)
		{
			return ContainsKey(name);
		}

		public IEnumerable<string> FindClosestMatches(string name)
		{
			return Util.StringUtil.FindClosestMatchesFromList(Keys, name);
		}

		public bool Add(Target target, bool ignoreduplicate = false)
		{
			if (target.Override)
			{
				AddOrUpdate(target.Name, target, (key, t) =>
				{
					if (t.AllowOverride)
					{
						Log.Info.WriteLine("[{0}]: overriding target '{1}' previously defined in: {2}", target.Location, t.Name, t.Location.ToString());
						target.BaseTarget = t;
					}
					else
					{
						string msg = String.Format("Duplicate target '{0}' already defined in: {1} with attribute allowoverride='false'", t.Name, t.Location.ToString());
						if (ignoreduplicate)
						{
							Log.Info.WriteLine("[{0}]: {1}", target.Location, msg);
						}
						else
						{
							throw new BuildException(msg, target.Location, stackTrace: BuildException.StackTraceType.XmlOnly);
						}
					}

					return target;
				});
			}
			else
			{
				Target t = GetOrAdd(target.Name, target);
				if (t != target)
				{
					string details = t.AllowOverride ?
						"Add attribute 'override=\"true\"` if you intended to override an existing target." :
						"Pre-existing target was defined with attribute allowoverride='false'.";			
					string msg = $"Duplicate target '{t.Name}' already defined in: {t.Location.ToString()}. {details}";
					if (ignoreduplicate)
					{
						Log.Info.WriteLine("[{0}]: {1}", target.Location, msg);
					}
					else
					{
						throw new BuildException(msg, target.Location, stackTrace: BuildException.StackTraceType.XmlOnly);
					}
				}
			}
			return true;
		}
	}
}
