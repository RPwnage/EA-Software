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

namespace NAnt.Perforce
{
	public abstract class P4TimeSpec
	{
		internal abstract string GetSpecString();
	}

	public class P4ChangelistSpec : P4TimeSpec
	{
		protected readonly uint _Changelist;

		public P4ChangelistSpec(uint changelist)
		{
			_Changelist = changelist;
		}

		internal override string GetSpecString()
		{
			return String.Format("@{0}", _Changelist);
		}
	}

	public class P4ClientOrLabelSpec : P4TimeSpec
	{
		protected readonly string _ClientOrLabel;

		public P4ClientOrLabelSpec(string clientOrLabel)
		{
			_ClientOrLabel = clientOrLabel;
		}

		internal override string GetSpecString()
		{
			return String.Format("@{0}", _ClientOrLabel);
		}
	}

	public class P4TimeStampSpec : P4TimeSpec
	{
		protected readonly DateTime _TimeStamp;

		public P4TimeStampSpec(DateTime timestamp)
		{
			_TimeStamp = timestamp;
		}

		internal override string GetSpecString()
		{
			return String.Format("@{0}", _TimeStamp.ToString("yyyy/MM/dd:hh:mm:ss"));
		}
	}

	public class P4PendingChangelistSpec : P4ChangelistSpec
	{
		public P4PendingChangelistSpec(uint changelist)
			: base(changelist)
		{
		}

		internal override string GetSpecString()
		{
			return String.Format("@={0}", _Changelist);
		}
	}

	public class P4RevisionSpec : P4TimeSpec
	{
		private readonly uint _Revision;

		public P4RevisionSpec(uint revision)
		{
			_Revision = revision;
		}

		internal override string GetSpecString()
		{
			return String.Format("#{0}", _Revision);
		}
	}

	public class P4HaveSpec : P4TimeSpec
	{
		internal override string GetSpecString()
		{
			return String.Format("#have");
		}
	}

	public class P4NowSpec : P4TimeSpec
	{
		internal override string GetSpecString()
		{
			return String.Format("@now");
		}
	}
}
