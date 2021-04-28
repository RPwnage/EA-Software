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
using System.Text;
using System.Text.RegularExpressions;

namespace NAnt.Perforce
{
	public class P4Version
	{
		public readonly uint MajorVersion;
		public readonly uint MinorVersion;
		public readonly uint Revision;
		public readonly string ReleaseType; // release descriptor i.e BETA, emtpy string for official releases

		// format:
		// Rev. APP/PLATFORM_PROCESSOR/MAJOR_VERSION.MINOR_VERSION.RELEASETYPE/REVISION (DATE)
		private readonly static Regex _Regex = new Regex
		(
			@"(?:Rev\.)?" + // Rev. 
			@"[a-zA-Z0-9_]+\/" + // APP/ i.e P4/
			@"[a-zA-Z0-9_]+\/" + // PLATFORM_PROCESSOR i.e NTX64/
			@"([0-9]+)\.([0-9]+)(\.[a-zA-Z0-9_-]+)?\/" +  // MAJOR_VERSION.MINOR_VERSION.RELEASETYPE i.e 2015.1/ or sometimes 2015.1.BETA/, 2015.2.PREP-TEST_ONLY/
			@"([0-9]+) " + // REVISION i.e 995427
			@"\(.*\)" // (DATE) i.e (2015/01/29)
		);

		internal P4Version(string versionString)
		{
			Match match = _Regex.Match(versionString);
			if (match.Success)
			{
				MajorVersion = UInt32.Parse(match.Groups[1].Value);
				MinorVersion = UInt32.Parse(match.Groups[2].Value);
				ReleaseType = match.Groups[3].Value;
				Revision = UInt32.Parse(match.Groups[4].Value);
			}
			else
			{
				throw new UnknownVersionException(versionString);
			}
		}

		public bool AtLeast(uint majorVersion, uint minorVersion, uint? revision = null)
		{
			if (MajorVersion > majorVersion)
			{
				return true;
			}
			else if (MajorVersion == majorVersion)
			{
				if (MinorVersion > minorVersion)
				{
					return true;
				}
				else if (MinorVersion == minorVersion)
				{
					if (revision.HasValue)
					{
						return Revision >= revision;
					}
					else
					{
						return true;
					}
				}
			}
			return false;
		}

		public override string ToString()
		{
			StringBuilder versionBuilder = new StringBuilder();
			versionBuilder.Append(MajorVersion);
			versionBuilder.Append('.');
			versionBuilder.Append(MinorVersion);
			if (ReleaseType != String.Empty)
			{
				versionBuilder.Append('.');
				versionBuilder.Append(ReleaseType);
			}
			versionBuilder.Append('/');
			versionBuilder.Append(Revision);
			return versionBuilder.ToString();
		}
	}
}