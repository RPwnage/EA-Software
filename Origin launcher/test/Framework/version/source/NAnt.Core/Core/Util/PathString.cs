// Originally based on NAnt - A .NET build tool
// Copyright (C) 2003-2015 Electronic Arts Inc.
// Copyright (C) 2001-2003 Gerry Shaw
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
// Ian MacLean (ian_maclean@another.com)
// Electronic Arts (Frostbite.Team.CM@ea.com)

using System;
using System.IO;
using System.Collections.Generic;

namespace NAnt.Core.Util
{
	public sealed class PathString : IEquatable<PathString>, IComparable<PathString>
	{
		public enum PathState { Undefined = 0, Normalized = 1, Directory = 2, File = 4, FullPath = 8 };
		public enum PathParam { NormalizeOnly = 0, GetFullPath = 1 };

		public static readonly PathStringFileNameEqualityComparer FileNameEqualityComparer = new PathStringFileNameEqualityComparer();

		public static string Quote(PathString p)
		{
			return p == null ? String.Empty : "\"" + p.Path + "\"";
		}


		public PathString(string path, PathState state = PathState.Undefined)
		{
			Path = path;
			State = state;
		}

		public readonly string Path;
		public readonly PathState State;

		public bool IsNormalized
		{
			get { return (State & PathState.Normalized) != 0; }
		}

		public bool IsFullPath
		{
			get { return (State & PathState.FullPath) != 0; }
		}

		public string NormalizedPath
		{
			get
			{
				return IsNormalized ? Path : PathNormalizer.Normalize(Path, !IsFullPath);
			}
		}

		public PathString Normalize(PathParam param = PathParam.GetFullPath, DirectorySeparatorOptions separatorOptions = DirectorySeparatorOptions.CurrentSystem)
		{
			string path = Path;
			if(IsNormalized)
			{
				if(param == PathParam.GetFullPath && ! IsFullPath)
				{
					return new PathString(PathNormalizer.Normalize(path, true, separatorOptions: separatorOptions), 
						State | PathState.Normalized | PathState.FullPath);
				}

				return this;
			}

			return new PathString(PathNormalizer.Normalize(path, param == PathParam.GetFullPath, separatorOptions: separatorOptions), 
				State | PathState.Normalized | (param == PathParam.GetFullPath ? PathState.FullPath : PathState.Undefined));
		}

		public static bool IsNullOrEmpty(PathString path)
		{
			return path == null || String.IsNullOrEmpty(path.Path);
		}

		public static PathString MakeNormalized(PathString path, PathParam param = PathParam.GetFullPath, DirectorySeparatorOptions separatorOptions = DirectorySeparatorOptions.CurrentSystem)
		{
			if (path != null)
			{                
				if (path.IsNormalized)
				{
					if (param == PathParam.GetFullPath && !path.IsFullPath)
					{
						return new PathString(PathNormalizer.Normalize(path.Path, true, separatorOptions: separatorOptions), 
							path.State | PathState.Normalized | PathState.FullPath);
					}

					return path;
				}

				return new PathString(PathNormalizer.Normalize(path.Path, param == PathParam.GetFullPath, separatorOptions: separatorOptions), 
					path.State | PathState.Normalized | (param == PathParam.GetFullPath ? PathState.FullPath : PathState.Undefined));
			}

			return new PathString(String.Empty, PathState.Normalized);

		}

		public static PathString MakeNormalized(string path, PathParam param = PathParam.GetFullPath, DirectorySeparatorOptions separatorOptions = DirectorySeparatorOptions.CurrentSystem)
		{
			PathState state = PathState.Normalized;

			path = PathNormalizer.Normalize(path.TrimQuotes(), param == PathParam.GetFullPath, separatorOptions: separatorOptions);

			if (!String.IsNullOrEmpty(path) && (param == PathParam.GetFullPath || System.IO.Path.IsPathRooted(path)))
			{
				state |= PathState.FullPath;
			}

			return new PathString(path, state);

		}

		public static PathString MakeCombinedAndNormalized(string baseDir, string path, PathParam param = PathParam.GetFullPath, DirectorySeparatorOptions separatorOptions = DirectorySeparatorOptions.CurrentSystem)
		{
			PathState state = PathState.Normalized;

			path = PathNormalizer.Normalize(path.TrimQuotes(), false, separatorOptions: separatorOptions);
			if (!System.IO.Path.IsPathRooted(path))
			{
				// Convert path relative to masterconfig.xml into absolute path.
				path = System.IO.Path.Combine(baseDir.TrimQuotes(), path);
			}

			path = PathNormalizer.Normalize(path, param == PathParam.GetFullPath, separatorOptions: separatorOptions);

			if (param == PathParam.GetFullPath || System.IO.Path.IsPathRooted(path))
			{
				state |= PathState.FullPath;
			}

			return new PathString(path, state);

		}

		public bool Equals(PathString other)
		{
			bool ret = false;

			if (Object.ReferenceEquals(this, other))
			{
				ret = true;
			}
			else if (Object.ReferenceEquals(other, null) || Object.ReferenceEquals(this, null))
			{
				ret = (Path == null);
			}
			else if (Path != null)
			{
				PathString x = IsNormalized ? this : MakeNormalized(Path, PathParam.NormalizeOnly);
				PathString y = other.IsNormalized ? other : MakeNormalized(other.Path, PathParam.NormalizeOnly);

				ret = x.Path.Equals(y.Path, PathUtil.PathStringComparison);
			}
			else
			{
				ret = other.Path == null;
			}

			return ret;
		}

		public override string ToString()
		{
			return Path;
		}

		public override bool Equals(object other)
		{
			if (other is PathString)
				return this.Equals((PathString)other);
			else
				return false;
		}

		public int CompareTo(PathString other)
		{
			int ret = 1;

			if (Object.ReferenceEquals(this, other))
			{
				ret = 0;
			}
			else if (!Object.ReferenceEquals(other, null) && Path != null)
			{
				PathString x = IsNormalized ? this : MakeNormalized(Path, PathParam.NormalizeOnly);
				PathString y = other.IsNormalized ? other : MakeNormalized(other.Path, PathParam.NormalizeOnly);

				ret = String.Compare(x.Path, y.Path, PathUtil.PathStringComparison);
			}
			else if (Object.ReferenceEquals(other, null) && Path != null)
			{
				ret = String.Compare(Path, String.Empty, PathUtil.PathStringComparison);

			}
			else
			{
				ret = String.Compare(Path, other.Path, PathUtil.PathStringComparison);
			}

			return ret;
		}


		public override int GetHashCode()
		{
			if(Path == null)
			{
				return String.Empty.GetHashCode();
			}

			var str = (IsNormalized ? this : MakeNormalized(Path, PathParam.NormalizeOnly)).Path;

			return (PathUtil.IsCaseSensitive ? str : str.ToLowerInvariant()).GetHashCode();            
		}

		public static bool operator == (PathString lhs, PathString rhs)
		{
			if (System.Object.ReferenceEquals(lhs, rhs))
			{
				return true;
			}

			if (((object)lhs == null) || ((object)rhs == null))
			{
				return false;
			}


			return lhs.Equals(rhs);
		}

		public static bool operator != (PathString lhs, PathString rhs)
		{
			return !(lhs == rhs);
		}

		public bool IsPathInDirectory(PathString dir)
		{
			PathString p = MakeNormalized(this);
			PathString d = MakeNormalized(dir);
			return p.Path.StartsWith(d.Path, PathUtil.PathStringComparison);
		}
	}

	public class PathStringFileNameEqualityComparer : IEqualityComparer<PathString>
	{
		public bool Equals(PathString x, PathString y)
		{
			return 0 == String.Compare(Path.GetFileName(x.Path), Path.GetFileName(y.Path), PathUtil.IsCaseSensitive ? StringComparison.Ordinal : StringComparison.OrdinalIgnoreCase);
		}

		public int GetHashCode(PathString obj)
		{
			return PathUtil.IsCaseSensitive ? Path.GetFileName(obj.Path).GetHashCode() : Path.GetFileName(obj.Path).ToLowerInvariant().GetHashCode();
		}
	}

	public class FileNameEqualityComparer : IEqualityComparer<string>
	{
		public static readonly FileNameEqualityComparer Instance = new FileNameEqualityComparer();

		public bool Equals(string x, string y)
		{
			return 0 == String.Compare(Path.GetFileName(x), Path.GetFileName(y), PathUtil.IsCaseSensitive ? StringComparison.Ordinal : StringComparison.OrdinalIgnoreCase);
		}

		public int GetHashCode(string obj)
		{
			return PathUtil.IsCaseSensitive ? Path.GetFileName(obj).GetHashCode() : Path.GetFileName(obj).ToLowerInvariant().GetHashCode();
		}
	}

	public static class PathStringExtensions
	{
		public static bool IsNullOrEmpty(this PathString path)
		{
			return (path == null || String.IsNullOrEmpty(path.Path)); 
		}
	}

}
