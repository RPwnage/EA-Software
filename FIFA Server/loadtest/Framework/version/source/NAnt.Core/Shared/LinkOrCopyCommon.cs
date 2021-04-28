// NAnt - A .NET build tool
// Copyright (C) 2003-2019 Electronic Arts Inc.
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


// IMPORTANT!  DO NOT reference any Framework modules in this class.  This file
// gets included directly by copy.exe, FrameworkCopyWithAttributes, and CopyTask.

using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Net;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading;

using Microsoft.Win32.SafeHandles;

using Mono.Unix.Native;

namespace NAnt.Shared
{
	public class FileWriteAccessMutexName
	{
		public static string GetMutexName(string path)
		{
			// Note that mutex name cannot use backslash (\) because it has special meaning!
			return "EA.Framework.FileWriteAccess-" + path.ToLower().Replace(":", "-").Replace("\\", "/").Replace("//", "/");
		}
	}

	public enum LinkOrCopyMethod
	{
		Symlink,	// link one location to another, see junction notes for limitations on windows
		Hardlink,	// only usable for files, creates a new file system entry that references (with ref count) the same file storage. Attributes that are part of the storage (such as modified time) are shared
		Junction,	// windows only, like symlibks but limited to absolute local paths and only directories - these limitations are what make them useful however, because thet don't require elevated permissions
		Copy        // nearly always a last resort if we considered linking, but always supported
	}

	public enum LinkOrCopyStatus
	{
		Failed,
		Symlinked,
		AlreadySymlinked,
		Hardlinked,
		AlreadyHardlinked,
		Junctioned,
		AlreadyJunctioned,
		Copied,
		AlreadyCopied
	}

	public enum OverwriteMode
	{
		NoOverwrite,		// don't overwrite existing file system object
		Overwrite,			// overwrite existing file system objects
		OverwriteReadOnly	// overwrite existing file system objects even if they are readonly
	}

	[Flags]
	public enum CopyFlags
	{
		None = 0,
		MaintainAttributes = 1 << 0, // when copying with overwrite, maintain target file attributes
		PreserveTimestamp = 1 << 1,  // destination file is given same timestamp as source file, if flags is not specified timestamp is set to time of copy
		SkipUnchanged = 1 << 2,      // when copying with overwrite, doesn't overwrite files with a) same length and b) behaviour based on PreserveTimestamp. If PreserveTimeStamp is set then doesn't overwrite file if timestamp is same as source, otherwise doesn't override if timestamp is newer than source
		Default = MaintainAttributes | PreserveTimestamp | SkipUnchanged
	}

	public static class LinkOrCopyCommon
	{
		internal static class WindowsCommon
		{
			[Flags]
			internal enum EFileAccess : uint
			{
				GenericRead = 0x80000000,
				GenericWrite = 0x40000000,
				GenericExecute = 0x20000000,
				GenericAll = 0x10000000
			}

			[Flags]
			internal enum EFileShare : uint
			{
				None = 0x00000000,
				Read = 0x00000001,
				Write = 0x00000002,
				Delete = 0x00000004
			}

			internal enum ECreationDisposition : uint
			{
				New = 1,
				CreateAlways = 2,
				OpenExisting = 3,
				OpenAlways = 4,
				TruncateExisting = 5
			}

			[Flags]
			internal enum EFileAttributes : uint
			{
				Readonly = 0x00000001,
				Hidden = 0x00000002,
				System = 0x00000004,
				Directory = 0x00000010,
				Archive = 0x00000020,
				Device = 0x00000040,
				Normal = 0x00000080,
				Temporary = 0x00000100,
				SparseFile = 0x00000200,
				ReparsePoint = 0x00000400,
				Compressed = 0x00000800,
				Offline = 0x00001000,
				NotContentIndexed = 0x00002000,
				Encrypted = 0x00004000,
				Write_Through = 0x80000000,
				Overlapped = 0x40000000,
				NoBuffering = 0x20000000,
				RandomAccess = 0x10000000,
				SequentialScan = 0x08000000,
				DeleteOnClose = 0x04000000,
				BackupSemantics = 0x02000000,
				PosixSemantics = 0x01000000,
				OpenReparsePoint = 0x00200000,
				OpenNoRecall = 0x00100000,
				FirstPipeInstance = 0x00080000
			}

			internal const int ERROR_NOT_A_REPARSE_POINT = 4390; // The file or directory is not a reparse point.
			internal const int ERROR_REPARSE_ATTRIBUTE_CONFLICT = 4391; // The reparse point attribute cannot be set because it conflicts with an existing attribute.
			internal const int ERROR_INVALID_REPARSE_DATA = 4392; // The data present in the reparse point buffer is invalid.
			internal const int ERROR_REPARSE_TAG_INVALID = 4393; // The tag present in the reparse point buffer is invalid.
			internal const int ERROR_REPARSE_TAG_MISMATCH = 4394; // There is a mismatch between the tag specified in the request and the tag present in the reparse point.
			internal const int FSCTL_SET_REPARSE_POINT = 0x000900A4; // Command to set the reparse point data block.
			internal const int FSCTL_GET_REPARSE_POINT = 0x000900A8; // Command to get the reparse point data block.
			internal const int FSCTL_DELETE_REPARSE_POINT = 0x000900AC; // Command to delete the reparse point data base.

			// This prefix indicates to NTFS that the path is to be treated as a non-interpreted
			// path in the virtual file system.
			internal const string NonInterpretedPathPrefix = @"\??\";

			// https://msdn.microsoft.com/en-us/library/windows/desktop/aa364571.aspx
			[DllImport("kernel32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
			internal static extern bool DeviceIoControl(IntPtr hDevice, uint dwIoControlCode, IntPtr InBuffer, int nInBufferSize, IntPtr OutBuffer, int nOutBufferSize, out int pBytesReturned, IntPtr lpOverlapped);

			[DllImport("kernel32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
			internal static extern SafeFileHandle CreateFile(string lpFileName, EFileAccess dwDesiredAccess, EFileShare dwShareMode, IntPtr lpSecurityAttributes, ECreationDisposition dwCreationDisposition, EFileAttributes dwFlagsAndAttributes, IntPtr hTemplateFile);

			internal static SafeFileHandle OpenWithReparseData(string reparsePoint, EFileAccess accessMode)
			{
				SafeFileHandle reparsePointHandle = CreateFile
				(
					reparsePoint,
					accessMode,
					EFileShare.Read | EFileShare.Write | EFileShare.Delete,
					IntPtr.Zero,
					ECreationDisposition.OpenExisting,
					EFileAttributes.BackupSemantics | EFileAttributes.OpenReparsePoint,
					IntPtr.Zero
				);

				if (Marshal.GetLastWin32Error() != 0)
				{
					Marshal.ThrowExceptionForHR(Marshal.GetHRForLastWin32Error());
				}

				return reparsePointHandle;
			}

			internal static bool GetReparseDataForPath<ReparseBufferType>(string path, out ReparseBufferType reparseDataBuffer)
			{
				int outBufferSize = Marshal.SizeOf(typeof(ReparseBufferType));
				IntPtr outBufferPtr = Marshal.AllocHGlobal(outBufferSize);
				try
				{
					bool result = false;
					using (SafeFileHandle handle = OpenWithReparseData(path, EFileAccess.GenericRead))
					{
						// We already opened with file share read/write/delete to avoid multi-thread issue.  However, we still 
						// want to close off the using scope immediately after calling DeviceIoControl when we no longer need 
						// the handle to let the system unlock the file handle.
						result = DeviceIoControl(handle.DangerousGetHandle(), FSCTL_GET_REPARSE_POINT,
							IntPtr.Zero, 0, outBufferPtr, outBufferSize, out int bytesReturned, IntPtr.Zero);
					}
					if (!result)
					{
						int error = Marshal.GetLastWin32Error();
						if (error == ERROR_NOT_A_REPARSE_POINT)
						{
							reparseDataBuffer = default;
							return false;
						}

						Marshal.ThrowExceptionForHR(Marshal.GetHRForLastWin32Error());
					}
					reparseDataBuffer = (ReparseBufferType)Marshal.PtrToStructure(outBufferPtr, typeof(ReparseBufferType));
					return true;
				}
				finally
				{
					Marshal.FreeHGlobal(outBufferPtr);
				}
			}

			internal static bool IsWindows()
			{
				return Environment.OSVersion.Platform == PlatformID.Win32NT ||
					Environment.OSVersion.Platform == PlatformID.Win32S ||
					Environment.OSVersion.Platform == PlatformID.Win32Windows ||
					Environment.OSVersion.Platform == PlatformID.WinCE;
			}

			internal static bool IsLocalPath(string path)
			{
				if ((!PathIsUNC(path) && PathIsNetworkPath(path)) || !IsLocalHost(new Uri(path).Host))
				{
					return false;
				}
				return true;
			}

			[DllImport("Shlwapi.dll", CharSet = CharSet.Unicode)]
			[return: MarshalAs(UnmanagedType.Bool)]
			private static extern bool PathIsNetworkPath(string pszPath);

			[DllImport("Shlwapi.dll", CharSet = CharSet.Unicode)]
			[return: MarshalAs(UnmanagedType.Bool)]
			private static extern bool PathIsUNC(string pszPath);

			private static bool IsLocalHost(string path)
			{
				IPAddress[] host = null;
				try
				{
					host = Dns.GetHostAddresses(path);
				}
				catch (Exception)
				{
					return false;
				}

				IPAddress[] local = Dns.GetHostAddresses(Dns.GetHostName());
				return host.Any(hostAddress => IPAddress.IsLoopback(hostAddress) || local.Contains(hostAddress));
			}
		}

		internal static class UnixCommon
		{
			internal static void WrappedSysCall(Func<int> syscall)
			{
				int result = syscall();
				if (result != 0)
				{
					string error = Stdlib.strerror((Errno)result);
					throw new Exception(error);
				}
			}
		}

		internal static class SymLink
		{
			[StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
			internal struct REPARSE_DATA_BUFFER // note, NOT the same as the junction reparse buffer - this has an additional member
			{
				public uint ReparseTag;
				public ushort ReparseDataLength;
				public ushort Reserved;

				public ushort SubstituteNameOffset;			// Offset, in bytes, of the substitute name string in the PathBuffer array.
				public ushort SubstituteNameLength;			// Length, in bytes, of the substitute name string. If this string is null-terminated, SubstituteNameLength does not include space for the null character.
				public ushort PrintNameOffset;				// Offset, in bytes, of the print name string in the PathBuffer array.
				public ushort PrintNameLength;				// Length, in bytes, of the print name string. If this string is null-terminated, PrintNameLength does not include space for the null character. 

				public uint Flags;							// 0 = absolute path, 1 = relative path

				// A buffer containing the unicode-encoded path string. The path string contains the substitute name string and print name string.
				[MarshalAs(UnmanagedType.ByValArray, SizeConst = 16 * 1024)]
				public byte[] PathBuffer;
			}

			internal const uint IO_REPARSE_TAG_SYMLINK = 0xA000000C; // Reparse point tag used to identify symbolic links
            internal const ulong SYMLINK_FLAG_RELATIVE = 1;

			internal const uint SYMBOLIC_LINK_FLAG_DIRECTORY = 0x1;
			internal const uint SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE = 0x2;

			internal static bool IsSupported()
			{
				return WindowsCommon.IsWindows(); // hanve't got this working on unix yet, creating symlinks works but detecting already linked has issues
			}

			internal static string GetInvalidReasonOrNull(string source, string dest)
			{
				return null;
			}

			internal static bool IsSymLink(string link)
			{
				return IsSupported() && GetTarget(link) != null;
			}

			internal static void Create(string target, string link)
			{
				if (WindowsCommon.IsWindows())
				{
					WindowsCreate(target, link);
				}
				else
				{
					UnixCreate(target, link);
				}
			}

			internal static bool IsAlreadyLinked(string target, string link)
			{
				bool caseInsensitiveFileSystem = Path.DirectorySeparatorChar == '\\'; // totally reliable way of detecting platform case sensitivity...
				return GetTarget(link)?.Equals(target, caseInsensitiveFileSystem ? StringComparison.OrdinalIgnoreCase : StringComparison.Ordinal) ?? false;
			}

			private static void WindowsCreate(string target, string link)
			{
				string linkDirectory = Path.GetDirectoryName(link);
				Directory.CreateDirectory(linkDirectory);

				// derive link type from target
				FileAttributes attributes = File.GetAttributes(target); // this work for directories too
				bool isDirectory = (attributes & FileAttributes.Directory) == FileAttributes.Directory;

				// default create with allow unprivileged create - this flag is dependent on machine settings (see ms docs)
				// but specifiying it even when it is unavailable is not an error, it will just be ignored
				// we always pass it to maximise chances that symlink is create vs other methods (we often fall back to junction
				// on windows) because 3rd party applications (e.g git) are more likely to play nice with symlinks than
				// junctions
				uint flags = SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE |
					(isDirectory ? SYMBOLIC_LINK_FLAG_DIRECTORY : 0);

				// ms docs says:
				// > If the function succeeds, the return value is nonzero. 
				// however, some automated testing how shown this function returning a non-zero yet still failing
				// we'll try and grab a good error if the function tells us it fails but we'll also double check
				// the link was created
				if (!WindowsSymlink(link, target, flags))
				{
					Marshal.ThrowExceptionForHR(Marshal.GetHRForLastWin32Error());
				}

				if (!IsAlreadyLinked(target, link))
				{
					throw new IOException($"Failed to create symbolic link from '{link}' to '{target}' but cause was not detected.");
				}
			}

			private static void UnixCreate(string target, string link)
			{
				if (Syscall.symlink(target, link) != 0)
				{
					throw new IOException("Syscall 'symlink' failed.");
				}
			}

			private static string GetTarget(string link)
			{
				if (WindowsCommon.IsWindows())
				{
					return WindowsGetTarget(link);
				}
				else
				{
					return UnixGetTarget(link);
				}
			}

			private static string UnixGetTarget(string link)
			{
				StringBuilder outPath = new StringBuilder(2048);
				UnixCommon.WrappedSysCall(() => Syscall.readlink(link, outPath));
				return outPath.ToString();
			}

			private static string WindowsGetTarget(string symlinkPath)
			{
				if (!File.Exists(symlinkPath) && !Directory.Exists(symlinkPath))
				{
					return null;
				}
				
				FileAttributes attributes = File.GetAttributes(symlinkPath); // this work for directories too
				if (!attributes.HasFlag(FileAttributes.ReparsePoint))
				{
					return null;
				}

				if (!WindowsCommon.GetReparseDataForPath(symlinkPath, out REPARSE_DATA_BUFFER outBuffer))
				{
					return null;
				}

				if (outBuffer.ReparseTag != IO_REPARSE_TAG_SYMLINK)
				{
					return null;
				}

				string substituteString = Encoding.Unicode.GetString(outBuffer.PathBuffer, outBuffer.SubstituteNameOffset, outBuffer.SubstituteNameLength);

				// The substituteString always start with the "\??\" Dos device namespace (if it is not a relative path).  See this link for more info
				// about this: http://stackoverflow.com/questions/14329653/does-the-substitutename-string-in-the-pathbuffer-of-a-reparse-data-buffer-stru
				if (outBuffer.Flags != SYMLINK_FLAG_RELATIVE && substituteString.StartsWith(WindowsCommon.NonInterpretedPathPrefix))
				{
					substituteString = substituteString.Substring(WindowsCommon.NonInterpretedPathPrefix.Length);
				}
				return substituteString;
			}

			// https://docs.microsoft.com/en-us/windows/desktop/api/winbase/nf-winbase-createsymboliclinka
			// return code seems to inconsistent across windows versions (https://stackoverflow.com/questions/33010440/createsymboliclink-on-windows-10)
			// - don't trust it and instead rely on checking if the symlink exists after calling
			[DllImport("kernel32.dll", CharSet = CharSet.Unicode, EntryPoint = "CreateSymbolicLink", SetLastError = true)]
			private static extern bool WindowsSymlink(string newFileName, string existingFileName, uint flags);
		}

		internal static class Junction
		{
			[StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
			internal struct REPARSE_DATA_BUFFER// note, NOT the same as the symlink reparse buffer - this has one less member
			{
				public uint ReparseTag;
				public ushort ReparseDataLength;
				public ushort Reserved;

				public ushort SubstituteNameOffset; // Offset, in bytes, of the substitute name string in the PathBuffer array.
				public ushort SubstituteNameLength; // Length, in bytes, of the substitute name string. If this string is null-terminated, SubstituteNameLength does not include space for the null character.
				public ushort PrintNameOffset;      // Offset, in bytes, of the print name string in the PathBuffer array.
				public ushort PrintNameLength;      // Length, in bytes, of the print name string. If this string is null-terminated, PrintNameLength does not include space for the null character. 

				// A buffer containing the unicode-encoded path string. The path string contains the substitute name string and print name string.
				[MarshalAs(UnmanagedType.ByValArray, SizeConst = 0x3ff0)]
				public byte[] PathBuffer;
			}

			internal const uint IO_REPARSE_TAG_MOUNT_POINT = 0xA0000003; // Reparse point tag used to identify mount points and junction points.

			internal static bool IsSupported()
			{
				// junction is a windows only concept
				return WindowsCommon.IsWindows();
			}

			internal static string GetInvalidReasonOrNull(string target, string link)
			{
				if (!Directory.Exists(target))
				{
					return $"Target '{target}' is not a directory. Junctions can only be used with directories.";
				}

				if (!WindowsCommon.IsLocalPath(target))
				{
					return $"Target '{target}' is not a local path. Junctions cannot refer to remote locations.";
				}
				return null;
			}

			internal static bool IsJunction(string link)
			{
				return IsSupported() && GetTarget(link) != null;
			}

			internal static bool IsAlreadyLinked(string target, string link)
			{
				string currentTarget = GetTarget(link);
				bool caseInsensitiveFileSystem = Path.DirectorySeparatorChar == '\\'; // totally reliable way of detecting platform case sensitivity...
				return currentTarget?.Equals(target, caseInsensitiveFileSystem ? StringComparison.OrdinalIgnoreCase : StringComparison.Ordinal) ?? false;
			}

			internal static void Create(string target, string link)
			{
				Directory.CreateDirectory(link);
				using (SafeFileHandle handle = WindowsCommon.OpenWithReparseData(link, WindowsCommon.EFileAccess.GenericWrite))
				{
					byte[] sourceDirBytes = Encoding.Unicode.GetBytes(WindowsCommon.NonInterpretedPathPrefix + target);

					REPARSE_DATA_BUFFER reparseDataBuffer = new REPARSE_DATA_BUFFER
					{
						ReparseTag = IO_REPARSE_TAG_MOUNT_POINT,
						ReparseDataLength = (ushort)(sourceDirBytes.Length + 12),
						SubstituteNameOffset = 0,
						SubstituteNameLength = (ushort)sourceDirBytes.Length,
						PrintNameOffset = (ushort)(sourceDirBytes.Length + 2),
						PrintNameLength = 0,
						PathBuffer = new byte[0x3ff0]
					};
					Array.Copy(sourceDirBytes, reparseDataBuffer.PathBuffer, sourceDirBytes.Length);

					int inBufferSize = Marshal.SizeOf(reparseDataBuffer);
					IntPtr inBuffer = Marshal.AllocHGlobal(inBufferSize);

					try
					{
						Marshal.StructureToPtr(reparseDataBuffer, inBuffer, false);

						bool result = WindowsCommon.DeviceIoControl(handle.DangerousGetHandle(), WindowsCommon.FSCTL_SET_REPARSE_POINT,
							inBuffer, sourceDirBytes.Length + 20, IntPtr.Zero, 0, out int bytesReturned, IntPtr.Zero);
						if (!result)
						{
							Marshal.ThrowExceptionForHR(Marshal.GetHRForLastWin32Error());
						}
					}
					finally
					{
						Marshal.FreeHGlobal(inBuffer);
					}
				}
			}

			private static string GetTarget(string junctionPath)
			{
				if (!File.Exists(junctionPath) && !Directory.Exists(junctionPath))
				{
					return null;
				}

				FileAttributes attributes = File.GetAttributes(junctionPath); // this work for directories too
				if (!attributes.HasFlag(FileAttributes.ReparsePoint))
				{
					return null;
				}

				if (!WindowsCommon.GetReparseDataForPath(junctionPath, out REPARSE_DATA_BUFFER outBuffer))
				{
					return null;
				}

				if (outBuffer.ReparseTag != IO_REPARSE_TAG_MOUNT_POINT)
				{
					return null;
				}

				string substituteString = Encoding.Unicode.GetString(outBuffer.PathBuffer, outBuffer.SubstituteNameOffset, outBuffer.SubstituteNameLength);
				if (substituteString.StartsWith(WindowsCommon.NonInterpretedPathPrefix))
				{
					substituteString = substituteString.Substring(WindowsCommon.NonInterpretedPathPrefix.Length);
				}
				return substituteString;
			}
		}

		internal static class HardLink
		{
			[StructLayout(LayoutKind.Sequential, Pack = 4)]
			private struct BY_HANDLE_FILE_INFORMATION
			{
				public uint FileAttributes;
				public System.Runtime.InteropServices.ComTypes.FILETIME CreationTime;
				public System.Runtime.InteropServices.ComTypes.FILETIME LastAccessTime;
				public System.Runtime.InteropServices.ComTypes.FILETIME LastWriteTime;
				public uint VolumeSerialNumber;
				public uint FileSizeHigh;
				public uint FileSizeLow;
				public uint NumberOfLinks;
				public uint FileIndexHigh;
				public uint FileIndexLow;
			}

			internal static bool IsSupported()
			{
				return true;
			}

            internal static string GetDriveLetter(string path)
            {
                if (String.IsNullOrEmpty(path))
                {
                    return null;
                }

                // Some versions of Runtime do not have MacOSX definition. Use integer value 6
                if (Environment.OSVersion.Platform == PlatformID.Unix ||
                   (int)Environment.OSVersion.Platform == 6)
                {
                    if (path[0] == Path.DirectorySeparatorChar)
                    {
                        int i = path.IndexOf(Path.DirectorySeparatorChar, 1);
                        if (i != -1)
                        {
                            return path.Substring(0, i);
                        }
                    }
                    return String.Empty;
                }

                int ind = path.IndexOf(':');

                if (ind < 0) // It must be a network path. return host name as a drive letter
                {
                    char[] PATH_SEPARATOR_CHARS_ALT = new char[] { Path.DirectorySeparatorChar, Path.AltDirectorySeparatorChar };

                    path = path.TrimStart(PATH_SEPARATOR_CHARS_ALT);
                    ind = path.IndexOfAny(PATH_SEPARATOR_CHARS_ALT);
                }
                if (ind < 0)
                {
                    return null;
                }
                return path.Substring(0, ind);
            }

            internal static string GetInvalidReasonOrNull(string target, string link)
			{
                string thisAssemblyFile = Assembly.GetExecutingAssembly().Location;
                var PathStringComparison = File.Exists(thisAssemblyFile.ToUpperInvariant()) && File.Exists(thisAssemblyFile.ToLowerInvariant()) ? StringComparison.OrdinalIgnoreCase : StringComparison.Ordinal;

                if (!GetDriveLetter(target).Equals(GetDriveLetter(link), PathStringComparison))
                {
                    return $"Target '{target}' and '{link}' or on different drives. Hardlinks can only be used on the same drive.";
                }

                if (!File.Exists(target))
				{
					return $"Target '{target}' is not a file. Hardlinks can only be used for files."; // TODO should hardlink mode, rathe than failing for directories, attempt to copy dir structure but link each file?
				}
				return null;
			}

			internal static bool IsAlreadyLinked(string target, string link)
			{
				if (WindowsCommon.IsWindows())
				{
					return WindowsIsAlreadyHardLinked(target, link);
				}
				else
				{
					return UnixIsAlreadyHardLinked(target, link);
				}
			}

			internal static void Create(string target, string link)
			{
				if (WindowsCommon.IsWindows())
				{
					WindowsCreate(target, link);
				}
				else // assume not windows == unix, not ideal but these are the only two situations we expect FW to run under
				{
					UnixCreate(target, link);
				}
			}

			private static bool WindowsIsAlreadyHardLinked(string target, string link)
			{
				using (SafeFileHandle hSrcFile = WindowsCommon.CreateFile(
					target,
					WindowsCommon.EFileAccess.GenericRead,
					WindowsCommon.EFileShare.Read | WindowsCommon.EFileShare.Write | WindowsCommon.EFileShare.Delete,
					IntPtr.Zero,
					WindowsCommon.ECreationDisposition.OpenExisting,
					WindowsCommon.EFileAttributes.BackupSemantics | WindowsCommon.EFileAttributes.OpenReparsePoint,
					IntPtr.Zero
				))
				using (SafeFileHandle hDestFile = WindowsCommon.CreateFile(
					link,
					WindowsCommon.EFileAccess.GenericRead,
					WindowsCommon.EFileShare.Read | WindowsCommon.EFileShare.Write | WindowsCommon.EFileShare.Delete,
					IntPtr.Zero,
					WindowsCommon.ECreationDisposition.OpenExisting,
					WindowsCommon.EFileAttributes.BackupSemantics | WindowsCommon.EFileAttributes.OpenReparsePoint,
					IntPtr.Zero
				))
				{
					if (
						GetFileInformationByHandle(hSrcFile, out BY_HANDLE_FILE_INFORMATION srcFileInfo) &&
						GetFileInformationByHandle(hDestFile, out BY_HANDLE_FILE_INFORMATION destFileInfo)
					)
					{
						return srcFileInfo.VolumeSerialNumber == destFileInfo.VolumeSerialNumber &&
							srcFileInfo.FileIndexHigh == destFileInfo.FileIndexHigh &&
							srcFileInfo.FileIndexLow == destFileInfo.FileIndexLow;
					}
					return false;
				}
			}

			private static bool UnixIsAlreadyHardLinked(string target, string link)
			{
				if (Syscall.stat(target, out Stat targetStat) == 0 && Syscall.stat(link, out Stat linkStat) == 0)
				{
					return targetStat.st_ino == linkStat.st_ino;
				}
				return false;
			}

			private static void WindowsCreate(string target, string link)
			{
				if (!WindowsHardLink(link, target, IntPtr.Zero))
				{
					Marshal.ThrowExceptionForHR(Marshal.GetHRForLastWin32Error());
				}
			}

			private static void UnixCreate(string target, string link)
			{
				if (Syscall.link(target, link) != 0)
				{
					throw new IOException("Syscall 'link' failed.");
				}
			}

			// https://docs.microsoft.com/en-us/windows/desktop/api/winbase/nf-winbase-createhardlinka
			// returns *true* (aka non-zero) on success or false otherwise (aka zero)
			[DllImport("kernel32.dll", CharSet = CharSet.Unicode, EntryPoint = "CreateHardLink", SetLastError = true)]
			private static extern bool WindowsHardLink(string newFileName, string exitingFileName, IntPtr securityAttributes);

			[DllImport("kernel32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
			private static extern bool GetFileInformationByHandle(SafeFileHandle hFile, out BY_HANDLE_FILE_INFORMATION lpFileInformation);
		}

		public static LinkOrCopyStatus LinkOrCopy(string target, string link, OverwriteMode overwriteMode = OverwriteMode.NoOverwrite, CopyFlags copyFlags = CopyFlags.Default, uint retries = 0, uint retryDelayMilliseconds = 0, params LinkOrCopyMethod[] methods)
		{
			LinkOrCopyStatus result = InternalLinkOrCopy(target, link, overwriteMode, copyFlags, retries, retryDelayMilliseconds, out Exception exception, methods);
			if (exception != null)
			{
				throw exception;
			}

			if (result == LinkOrCopyStatus.Failed)
			{
				// if linkType was Failed then an exception should been thrown above
				// This code is paranoia code in case we end up here
				throw new IOException($"Failed to create link from '{target}' to '{link}'. Failure cause could not be determined.");
			}
			return result;
		}

		public static LinkOrCopyStatus TryLinkOrCopy(string target, string link, OverwriteMode overwriteMode = OverwriteMode.NoOverwrite, CopyFlags copyFlags = CopyFlags.Default, uint retries = 0, uint retryDelayMilliseconds = 0, params LinkOrCopyMethod[] methods)
		{
			return InternalLinkOrCopy(target, link, overwriteMode, copyFlags, retries, retryDelayMilliseconds, out Exception exception, methods);
		}

		private static LinkOrCopyStatus InternalLinkOrCopy(string target, string link, OverwriteMode overwriteMode, CopyFlags copyFlags, uint retries, uint retryDelayMilliseconds, out Exception exception, params LinkOrCopyMethod[] methods)
		{
			// normalize source and dest
			target = Path.GetFullPath(new Uri(target).LocalPath).TrimEnd(Path.DirectorySeparatorChar, Path.AltDirectorySeparatorChar);
			link = Path.GetFullPath(new Uri(link).LocalPath).TrimEnd(Path.DirectorySeparatorChar, Path.AltDirectorySeparatorChar);
			bool caseInsensitiveFileSystem = Path.DirectorySeparatorChar == '\\'; // totally reliable way of detecting platform case sensitivity...
			if (target.Equals(link, caseInsensitiveFileSystem ? StringComparison.OrdinalIgnoreCase : StringComparison.Ordinal))
			{
				throw new ArgumentException("Source and destination refer to the same path.");
			}

			// make sure we have at least one real method, and remove duplciates
			IEnumerable<LinkOrCopyMethod> filteredMethods = methods.Distinct().Where(method => Enum.IsDefined(typeof(LinkOrCopyMethod), method));
			if (!filteredMethods.Any())
			{
				throw new ArgumentException("No valid methods for link or copy.", nameof(methods));
			}

			// check at least one method is supported on current platform
			filteredMethods = filteredMethods.Where
			(
				method =>
				{
					switch (method)
					{
						case LinkOrCopyMethod.Symlink:
							return SymLink.IsSupported();
						case LinkOrCopyMethod.Junction:
							return Junction.IsSupported();
						case LinkOrCopyMethod.Hardlink:
							return HardLink.IsSupported();
						default:
							return true; // copy always supported
					}
				}
			);
			if (!filteredMethods.Any())
			{
				throw new PlatformNotSupportedException("The selected link methods are not supported on the curent platform.");
			}

			// check source exists - quick check before validity tests
			if (!File.Exists(target) && !Directory.Exists(target))
			{
				throw new IOException($"Target '{target}' does not exist.");
			}

			// check source and destination are valid for requested methods, but don't fail if any method is invalid -
			// it's possible user might want this to fail if an invalid method is requested (such as hardlinking dirs)
			// but it's more likely they'd want it to fallback to another method if the path is invalid (for example
			// windows users might attempt hardlink for files that may be on different drives and have that fallback
			// to a copy gracefully)
			Lazy<Dictionary<LinkOrCopyMethod, string>> invalidReasons = new Lazy<Dictionary<LinkOrCopyMethod, string>>();
			List<LinkOrCopyMethod> validMethods = new List<LinkOrCopyMethod>();
			foreach (LinkOrCopyMethod method in filteredMethods)
			{
				switch (method)
				{
					case LinkOrCopyMethod.Symlink:
						string symLinkInvalidReason = SymLink.GetInvalidReasonOrNull(target, link);
						if (symLinkInvalidReason != null)
						{
							invalidReasons.Value.Add(LinkOrCopyMethod.Symlink, symLinkInvalidReason);
						}
						else
						{
							validMethods.Add(LinkOrCopyMethod.Symlink);
						}
						break;
					case LinkOrCopyMethod.Junction:
						string junctionInvalidReason = Junction.GetInvalidReasonOrNull(target, link);
						if (junctionInvalidReason != null)
						{
							invalidReasons.Value.Add(LinkOrCopyMethod.Junction, junctionInvalidReason);
						}
						else
						{
							validMethods.Add(LinkOrCopyMethod.Junction);
						}
						break;
					case LinkOrCopyMethod.Hardlink:
						string hardlinkInvalidReason = HardLink.GetInvalidReasonOrNull(target, link);
						if (hardlinkInvalidReason != null)
						{
							invalidReasons.Value.Add(LinkOrCopyMethod.Hardlink, hardlinkInvalidReason);
						}
						else
						{
							validMethods.Add(LinkOrCopyMethod.Hardlink);
						}
						break;
					case LinkOrCopyMethod.Copy:
						validMethods.Add(LinkOrCopyMethod.Copy);
						break;
				}
			}
			if (!validMethods.Any())
			{
				throw new ArgumentException
				(
					$"None of the requested link methods are valid for '{target}', '{link}'."
					+ String.Join(String.Empty, invalidReasons.Value.OrderBy(kvp => kvp.Key).Select(kvp => $"{Environment.NewLine}\t{kvp.Key.ToString()}: {kvp.Value}"))
				);
			}

			// check if files are already linked using one the specified methods
			exception = null;
			if (File.Exists(link) || Directory.Exists(link)) // quick check
			{
				foreach (LinkOrCopyMethod method in validMethods)
				{
					switch (method)
					{
						case LinkOrCopyMethod.Symlink:
							if (SymLink.IsAlreadyLinked(target, link))
							{
								return LinkOrCopyStatus.AlreadySymlinked;
							}
							break;
						case LinkOrCopyMethod.Junction:
							if (Junction.IsAlreadyLinked(target, link))
							{
								return LinkOrCopyStatus.AlreadyJunctioned;
							}
							break;
						case LinkOrCopyMethod.Hardlink:
							if (HardLink.IsAlreadyLinked(target, link))
							{
								return LinkOrCopyStatus.AlreadyHardlinked;
							}
							break;
						default:
							// copy implementation will do a file by file up-to-date-check if needed, note however that if other methods are attempted but fail first they will delete the target directory first
							// meaning a full copy will have to be done 
							/* TODO is this correct behaviour? some alternatives:
							 * Do an "is already copied" check - if all the target files already exist in the destination then maybe we should assume this is null / incremental build
							   that fell back to copy last time?
							 * we could move the target to a temp location rather than deleting when we try to create a link, then if link succeeds we delete temp location if it fails we move it back
							*/
							break;
					}
				}
			}

			// now we're actually trying to do filesystem, so apply the whole retry to this block
			// note about how the retry is applied: we try all methods that are allowed before retrying, rather than retring single methods until retry limit is exceed
			uint maxTries = 1 + retries;

			List<Exception> exceptions = null;
			for (uint tryCount = 0; tryCount < maxTries; ++tryCount)
			{
				if (tryCount >= retries) // this is the last attempt, so record why it failed
				{
					exceptions = new List<Exception>();
				}

				// try each method
				foreach (LinkOrCopyMethod method in validMethods)
				{
					switch (method)
					{
						case LinkOrCopyMethod.Symlink:
							try
							{
								DeleteExistingIfModeAllowsOrThrowException(target, link, overwriteMode);
								SymLink.Create(target, link);
								return LinkOrCopyStatus.Symlinked;
							}
							catch (Exception e)
							{
								exceptions?.Add(e);
							}
							break;
						case LinkOrCopyMethod.Junction:
							try
							{
								DeleteExistingIfModeAllowsOrThrowException(target, link, overwriteMode);
								Junction.Create(target, link);
								return LinkOrCopyStatus.Junctioned;
							}
							catch (Exception e)
							{
								exceptions?.Add(e);
							}
							break;
						case LinkOrCopyMethod.Hardlink:
							try
							{
								// Hard links behave differently. If original file is deleted, the link is deleted as well.
								DeleteExistingIfModeAllowsOrThrowException(target, link, overwriteMode);
								HardLink.Create(target, link);
								return LinkOrCopyStatus.Hardlinked;
							}
							catch (Exception e)
							{
								exceptions?.Add(e);
							}
							break;
						case LinkOrCopyMethod.Copy:
							try
							{
								return Copy(target, link, overwriteMode, copyFlags);
							}
							catch (Exception e)
							{
								exceptions?.Add(e);
							}
							break;
					}
				}

				// no method worked, if this isn't the last attempt, wait for the given retry delay
				if (tryCount < retries)
				{
					Thread.Sleep((int)Math.Min(retryDelayMilliseconds, Int32.MaxValue));
				}
			}

			if (exceptions.Count == 1)
			{
				exception = exceptions.Single();
			}
			else
			{
				exception = new AggregateException("Failed to create link or copy.", exceptions.ToArray());
			}
			return LinkOrCopyStatus.Failed;
		}

		private static void DeleteExistingIfModeAllowsOrThrowException(string target, string link, OverwriteMode overwriteMode)
		{
			// "originalLinkedTarget" could be null if:
			// 1. "link" hasn't been created yet.
			// 2. "link" is a hard link.
			// 3. "link" is a real path.
			bool fileLinkExists = File.Exists(link);
			bool directoryLinkExists = Directory.Exists(link);
			if (overwriteMode == OverwriteMode.NoOverwrite && 
				(fileLinkExists || directoryLinkExists))
			{
				// note, even this exception will trigger retries which is arguably incorrect, but it's possbile retry is waiting for target
				// to be delete so we'll allow it
				throw new IOException($"Destination '{link}' already exists and is not linked to '{target}' via any of the specified methods.");
			}

			if (fileLinkExists)
			{
				if (overwriteMode == OverwriteMode.OverwriteReadOnly)
				{
					File.SetAttributes(link, File.GetAttributes(link) & ~FileAttributes.ReadOnly);
				}
				File.Delete(link);
			}
			else if (directoryLinkExists)
			{
				bool isLinkOrJunction = Junction.IsJunction(link) || SymLink.IsSymLink(link);
				if (overwriteMode == OverwriteMode.OverwriteReadOnly)
				{
					File.SetAttributes(link, File.GetAttributes(link) & ~FileAttributes.ReadOnly);
					if (!isLinkOrJunction)
					{
						try
						{
							// remove read only for the directory and all contained directories and files
							// If the "link" it points to no longer exists, it can get DirectoryNotFoundException.  The try/catch is
							// wrapped in case we didn't property determine "originalLinkedTargetExists".  However, the input "link" may
							// actually be a real directory and not a link.  So we still need to try execute the following loop.
							foreach (string fileSystemEntry in Directory.GetFileSystemEntries(link, "*", SearchOption.AllDirectories))
							{
								File.SetAttributes(fileSystemEntry, File.GetAttributes(fileSystemEntry) & ~FileAttributes.ReadOnly);
							}
						}
						catch (Exception e) when (e is FileNotFoundException || e is DirectoryNotFoundException)
						{
							// paranoia safety, it's still possible file or directory was deleted by another process before
							// we managed to, but if our delete fails because the target is missing, then we're in the state we want
							// anyway
						}
					}
				}
				Directory.Delete(link, recursive: !isLinkOrJunction);
			}
		}

		private static LinkOrCopyStatus Copy(string source, string dest, OverwriteMode overwriteMode, CopyFlags copyFlags)
		{
			LinkOrCopyStatus copyStatus = LinkOrCopyStatus.AlreadyCopied;
			if (File.Exists(source))
			{
				if (CopyFile(source, dest, overwriteMode, copyFlags))
				{
					copyStatus = LinkOrCopyStatus.Copied;
				}
			}
			else if (Directory.Exists(source))
			{
				foreach (string file in Directory.GetFiles(source, "*", SearchOption.AllDirectories))
				{
					string relativeDest = file.Substring(source.Length).TrimStart(Path.DirectorySeparatorChar);
					if (CopyFile(file, Path.Combine(dest, relativeDest), overwriteMode, copyFlags))
					{
						copyStatus = LinkOrCopyStatus.Copied;
					}
				}
			}
			else
			{
				throw new IOException($"Couldn't find file system entry '{source}'. Source may have been modified during copy.");
			}
			return copyStatus;
		}

		private static bool CopyFile(string source, string dest, OverwriteMode overwriteMode, CopyFlags copyFlags)
		{
			FileInfo sourceInfo = new FileInfo(source);
			FileInfo destInfo = new FileInfo(dest);

			FileAttributes targetAttributes = FileAttributes.Normal;
			if ((copyFlags & CopyFlags.MaintainAttributes) == CopyFlags.MaintainAttributes)
			{
				if (destInfo.Exists)
				{
					targetAttributes = destInfo.Attributes;
				}
				else
				{
					targetAttributes = sourceInfo.Attributes;
				}
			}

			if (destInfo.Exists)
			{
				// check if copy is up to date
				if (
					(copyFlags & CopyFlags.SkipUnchanged) == CopyFlags.SkipUnchanged &&
					sourceInfo.Length == destInfo.Length
				)
				{
					if ((copyFlags & CopyFlags.PreserveTimestamp) == CopyFlags.PreserveTimestamp)
					{
						if (sourceInfo.LastWriteTime == destInfo.LastWriteTime)
						{
							return false;
						}
					}
					else if (sourceInfo.LastWriteTime <= destInfo.LastWriteTime)
					{
						return false;
					}
				}

				// if we get here we need to copy but destination exists
				if (overwriteMode == OverwriteMode.NoOverwrite)
				{
					throw new IOException($"Cannot copy to '{dest}', file already exists.");
				}
				else
				{
					if (overwriteMode == OverwriteMode.OverwriteReadOnly)
					{
						destInfo.Attributes = destInfo.Attributes & ~FileAttributes.ReadOnly & ~FileAttributes.Hidden;
					}

					destInfo.Delete();
				}
			}
			else if (!destInfo.Directory.Exists)
			{
				destInfo.Directory.Create();
			}

			sourceInfo.CopyTo(dest, overwrite: overwriteMode != OverwriteMode.NoOverwrite); // we should have deleted it if already exists, but use overwrite
			destInfo.Refresh(); // update after delete / copying over

			// preserve timestamp
			destInfo.Attributes = destInfo.Attributes & ~FileAttributes.ReadOnly & ~FileAttributes.Hidden; ; // need to make sure it is not readonly in order to modify timestamp
			if ((copyFlags & CopyFlags.PreserveTimestamp) == CopyFlags.PreserveTimestamp)
			{
				// The FileInfo.CopyTo() command actually preserve timestamp by default and this is 
				// probably not needed.  But we'll set it just in case.
				destInfo.CreationTime = sourceInfo.CreationTime;
				destInfo.LastWriteTime = sourceInfo.LastWriteTime;
				destInfo.LastAccessTime = sourceInfo.LastAccessTime;
			}
			else
			{
				destInfo.LastWriteTime = DateTime.Now;
			}

			// retore or set normal attributes 
			destInfo.Attributes = targetAttributes;

			return true;
		}
	}
}