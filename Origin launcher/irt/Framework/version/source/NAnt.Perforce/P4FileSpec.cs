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
using System.Globalization;

namespace NAnt.Perforce
{
	// hierarchy:
	// 
	//  P4FileSpec - for commands that take any specification
	//      P4BaseLocation - for commands that don't allow range specification
	//          P4Location - for that only allow location specification (also pulls double duty as at head revision)
	//          P4LocationAtTime - all derived classes are just constructor helpers
	//              P4LocationAtChangelist
	//              P4LocationAtRevision - 0 allows for for null revision
	//              .. could add more helper classes here
	//      P4Range
	//          P4LocationAtTimeRange    
	//              P4LocationAtChangelistRange
	//              .. could add helper classes here
	//          P4TimeRange
	//              P4ChangelistRange
	//              .. could add helper classes here
	//      P4At

	public abstract class P4FileSpec
	{
		public static implicit operator P4FileSpec(string path)
		{
			return Parse(path, null);
		}

		public override string ToString()
		{
			return GetSpecString();
		}

		internal abstract string GetSpecString();

		public static P4FileSpec Parse(string locationString, string timeValue)
		{
			if (timeValue == null)
			{
				return Parse(locationString);
			}

			string timeValueLower = timeValue.Trim().ToLowerInvariant();

			if (timeValue.Length == 0)
			{
				return Parse(locationString);
			}

			if (timeValueLower.StartsWith("#") || timeValueLower.StartsWith("@"))
			{
				return Parse(locationString + timeValueLower);
			}
			else
			{
				if (timeValueLower == "head" || timeValueLower == "none" || timeValueLower == "have")
				{
					return Parse(locationString + "#" + timeValueLower);
				}
				else
				{
					return Parse(locationString + "@" + timeValueLower);
				}
			}
		}

		public static P4FileSpec Parse(string locationString)
		{
			int timeSpecificationIndex = Math.Max(locationString.IndexOf('#'), locationString.IndexOf('@'));
			if (timeSpecificationIndex > 0)
			{
				string location = locationString.Substring(0, timeSpecificationIndex);
				string time = locationString.Substring(timeSpecificationIndex);
				int timeRangeSpecifierIndex = location.IndexOf(',');
				if (timeRangeSpecifierIndex > 0)
				{
					throw new NotImplementedException("Revision range parsing not yet supported."); // TODO!!!!!!
				}
				else
				{
					string timeValue = time.Substring(1);
					if (time.StartsWith("#"))
					{
						uint revisionNo;
						if (timeValue == "have")
						{
							return new P4LocationAtHaveRevision(location);
						}
						else if (timeValue == "none")
						{
							return new P4LocationAtRevision(location, 0);
						}
						else if (timeValue == "head")
						{
							return new P4Location(location);
						}
						else if (UInt32.TryParse(timeValue, out revisionNo))
						{
							return new P4LocationAtRevision(location, revisionNo);
						}                
					}
					else if (time.StartsWith("@"))
					{
						// change / client / label / date spec
						uint clNo;
						if (UInt32.TryParse(timeValue, out clNo))
						{
							return new P4LocationAtChangeList(location, clNo);
						}

						DateTime datetime;
						// detect valid time strings based on format listed here: https://www.perforce.com/perforce/r16.1/manuals/cmdref/filespecs.html
						if (DateTime.TryParseExact(timeValue, "yyyy/M/d h:m:s", CultureInfo.InvariantCulture, DateTimeStyles.None, out datetime) ||
							DateTime.TryParseExact(timeValue, "yyyy/M/d:h:m:s", CultureInfo.InvariantCulture, DateTimeStyles.None, out datetime) ||
							DateTime.TryParseExact(timeValue, "yyyy/M/d", CultureInfo.InvariantCulture, DateTimeStyles.None, out datetime))
						{
							return new P4LocationAtTimeStamp(location, datetime);
						}
						else
						{
							return new P4LocationAtClientOrLabel(location, timeValue);
						}
					}
					throw new P4Exception(String.Format("Cannot parse '{0}'. Unknown time specifier format.", locationString));
				}
			}
			else
			{
				return new P4Location(locationString);
			}
		}
	}

	public abstract class P4BaseLocation : P4FileSpec
	{        
		public readonly string Path;

		internal P4BaseLocation(string path)
		{
			Path = path;
			// TODO we should validate the path here to make sure it doesn't contain revision specifiers
		}
	}

	public abstract class P4LocationAtTime : P4BaseLocation
	{
		private readonly P4TimeSpec _TimeSpec;

		internal P4LocationAtTime(string path, P4TimeSpec timeSpec)
			: base(path)
		{
			_TimeSpec = timeSpec;
		}

		internal override string GetSpecString()
		{
			return String.Format("{0}{1}", Path, _TimeSpec.GetSpecString());
		}
	}   
	
	public abstract class P4Range : P4FileSpec
	{
		protected readonly P4TimeSpec _FromTimeSpec;
		protected readonly P4TimeSpec _ToTimeSpec;

		internal P4Range(P4TimeSpec from, P4TimeSpec to)
		{
			_FromTimeSpec = from;
			_ToTimeSpec = to;
		}
	}

	public class P4TimeRange : P4Range
	{
		public P4TimeRange(P4TimeSpec from, P4TimeSpec to)
			: base(from, to)
		{
		}

		internal override string GetSpecString()
		{
			return String.Format("{0},{1}", _FromTimeSpec.GetSpecString(), _ToTimeSpec.GetSpecString());
		}
	}

	public class P4LocationAtTimeRange : P4Range
	{
		private readonly string _Path;

		public P4LocationAtTimeRange(string path, P4TimeSpec from, P4TimeSpec to)
			: base(from, to)
		{
			_Path = path;
		}

		internal override string GetSpecString()
		{
			return String.Format("{0}{1},{2}", _Path, _FromTimeSpec.GetSpecString(), _ToTimeSpec.GetSpecString());
		}
	}

	public class P4At : P4FileSpec
	{
		protected readonly P4TimeSpec _At;

		internal P4At(P4TimeSpec to)
		{
			_At = to;
		}

		internal override string GetSpecString()
		{
			return _At.GetSpecString();
		}
	}

	public class P4Location : P4BaseLocation
	{
		public P4Location(string path)
			: base(path)
		{
		}

		public static implicit operator P4Location(string path)
		{
			return Parse(path, null) as P4Location;
		}

		internal static P4Location DefaultLocation = new P4Location("//...");

		internal override string GetSpecString()
		{
			return Path;
		}
	}

	public class P4LocationAtHaveRevision : P4LocationAtTime
	{
		public P4LocationAtHaveRevision(string path)
			: base(path, new P4HaveSpec())
		{
		}
	}

	public class P4LocationAtChangeList : P4LocationAtTime
	{
		public readonly uint Changelist;

		public P4LocationAtChangeList(string path, uint changelist)
			: base(path, new P4ChangelistSpec(changelist))
		{
			Changelist = changelist;
		}
	}

	public class P4LocationAtClientOrLabel : P4LocationAtTime
	{
		protected readonly string ClientOrLabel;

		internal P4LocationAtClientOrLabel(string path, string clientOrLabel)
			: base(path, new P4ClientOrLabelSpec(clientOrLabel))
		{
			ClientOrLabel = clientOrLabel;
		}
	}

	public class P4LocationAtLabel : P4LocationAtClientOrLabel
	{
		public string Label { get { return ClientOrLabel; } }

		public P4LocationAtLabel(string path, string label)
			: base(path, label)
		{
		}
	}

	public class P4LocationAtClient : P4LocationAtClientOrLabel
	{
		public string Client { get { return ClientOrLabel; } }

		public P4LocationAtClient(string path, string client)
			: base(path, client)
		{
		}
	}

	public class P4LocationAtTimeStamp : P4LocationAtTime
	{
		public readonly DateTime TimeStamp;

		public P4LocationAtTimeStamp(string path, DateTime timestamp)
			: base(path, new P4TimeStampSpec(timestamp))
		{
			TimeStamp = timestamp;
		}
	} 

	public class P4LocationAtRevision : P4LocationAtTime
	{
		public P4LocationAtRevision(string path, uint revision)
			: base(path, new P4RevisionSpec(revision))
		{
		}
	}

	public class P4RevisiontAt : P4At
	{
		public P4RevisiontAt(uint revision)
			: base(new P4RevisionSpec(revision))
		{
		}
	}

	public class P4ChangelistRange : P4TimeRange
	{
		public P4ChangelistRange(uint minCL, uint maxCL)
			: base(new P4ChangelistSpec(minCL), new P4ChangelistSpec(maxCL))
		{
		}
	}

	public class P4ChangelistAt : P4At
	{
		public P4ChangelistAt(uint maxCL)
			: base(new P4ChangelistSpec(maxCL))
		{
		}
	}

	public class P4LocationAtChangelistRange : P4LocationAtTimeRange
	{
		public P4LocationAtChangelistRange(string path, uint minCL, uint maxCL)
			: base(path, new P4ChangelistSpec(minCL), new P4ChangelistSpec(maxCL))
		{
		}
	}
}
