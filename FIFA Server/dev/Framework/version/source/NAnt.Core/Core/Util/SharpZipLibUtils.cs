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
// Electronic Arts (Frostbite.Team.CM@ea.com)
// Kosta Arvanitis (karvanitis@ea.com)

using System;
using System.Collections.Generic;
using System.Collections.Specialized;
using System.IO;
using System.Text;

using ICSharpCode.SharpZipLib.Checksum;
using ICSharpCode.SharpZipLib.Core;
using ICSharpCode.SharpZipLib.Zip;

using NAnt.Core.Util;

using Mono.Unix.Native;
using System.Security.AccessControl;

// This is a helper wrapper of the NuGet ICSharpCode.SharpZipLib packages API.
// It provides simple Zip/Unzip functions to abstract away details like input streams,
// and to handle special cases like symlinks and unix file permissions.
namespace EA.SharpZipLib
{
	public class ZipEventArgs : EventArgs
	{
		public ZipEventArgs(ZipEntry zipEntry, int entryIndex, int entryCount)
		{
			ZipEntry = zipEntry;
			EntryIndex = entryIndex;
			EntryCount = entryCount;
		}

		public ZipEntry ZipEntry { get; }

		public int EntryIndex { get; }

		public int EntryCount { get; }
	}

	public delegate void ZipEventHandler(object sender, ZipEventArgs e);

	public class ZipLib
	{
		public event ZipEventHandler ZipEvent;
		Crc32 crc = new Crc32();

		private ZipFile _header_entries;

		public ZipLib()
		{
		}

		static ZipLib()
		{
			ZipStrings.CodePage = Encoding.UTF8.CodePage;
		}

		public static int GetHostID()
		{
			return (int)(
				// Note that Mono doesn't actually return PlatformID.MacOSX for Environment.OSVersion.Platform.  On OSX, it will return Unix!
				Environment.OSVersion.Platform == PlatformID.Unix ? HostSystemID.Unix :
				//Environment.OSVersion.Platform == PlatformID.MacOSX ? HostSystemID.OSX :
				HostSystemID.Msdos
			);
		}

		private static bool IsUnixOrOsxHostID(int zipEntryHostId)
		{
			return (zipEntryHostId == (int)HostSystemID.Unix || zipEntryHostId == (int)HostSystemID.OSX);
		}

		private static FilePermissions ExtractUnixPermissions(uint externalFileAttributes)
		{
			// Referencing http://unix.stackexchange.com/questions/14705/the-zip-formats-external-file-attribute
			// the Unix file attributes are the highest two bytes in the attributes field. 
			return (FilePermissions)(externalFileAttributes >> 16);
		}

		private static int AddUnixPermissions(int zipEntryExternalFileAttribute, Mono.Unix.Native.FilePermissions unixPermissions)
		{
			uint newExtFileAttr = ((uint)zipEntryExternalFileAttribute & 0x00FF) | ((uint)unixPermissions << 16);
			return (int)newExtFileAttr;
		}


		protected void OnZipEvent(ZipEventArgs e)
		{
			if (ZipEvent != null)
			{
				// call external event handlers
				ZipEvent(this, e);
			}
		}

		/// <summary>
		/// Unzips a specific file from a zip file to the output target dir
		/// </summary>
		/// <param name="pathToZip">The full path to the zip file.</param>
		/// <param name="targetDir">The full path to the destination folder.</param>
		/// <param name="fileName">If file to unzip.</param>
		public void UnzipSpecificFile(string pathToZip, string targetDir, string fileName)
		{
			string adjustedFileName = PathUtil.ReplaceDirectorySeparators(fileName);
			using (Stream fsInput = File.OpenRead(pathToZip))
			using (ZipFile zf = new ZipFile(fsInput))
			{
				foreach (ZipEntry zipEntry in zf)
				{
					if (!zipEntry.IsFile)
					{
						// Ignore directories
						continue;
					}
					string adjustedZipName = PathUtil.ReplaceDirectorySeparators(zipEntry.Name);
					if (adjustedZipName.ToLowerInvariant() == adjustedFileName.ToLowerInvariant())
					{
						var buffer = new byte[4096];
						using (var zipStream = zf.GetInputStream(zipEntry))
						{
							using (Stream fsOutput = File.Create(Path.Combine(targetDir, adjustedZipName)))
							{
								StreamUtils.Copy(zipStream, fsOutput, buffer);
							}
						}
						return;
					}
				}
			}
		}

		/// <summary>
		/// Unzip a file.
		/// </summary>
		/// <param name="zipFileName">The full path to the zip file.</param>
		/// <param name="targetDir">The full path to the destination folder.</param>
		/// <param name="preserveSymlink">If zip entry contain symlink, preserve that symlink if possible.</param>
		public void UnzipFile(string zipFileName, string targetDir, bool preserveSymlink = true)
		{
			if (!File.Exists(zipFileName))
			{
				throw new FileNotFoundException(String.Format("Attempting unizp operation for file '{0}'. But this file does not exist.", zipFileName));
			}
			using (var stream = File.OpenRead(zipFileName))
			{
				using (ZipFile zipFile = new ZipFile(File.OpenRead(zipFileName)))
				{
					stream.Close();

					_header_entries = zipFile;
					if (zipFile.Count > Int32.MaxValue)
					{
						throw new ArgumentOutOfRangeException(String.Format("Number of entries={0} in zip file '{1}' exceeds maximal allowed limit {2}", zipFile.Count, zipFileName, Int32.MaxValue));
					}
					int entryCount = (int)zipFile.Count;

					UnzipStream(File.OpenRead(zipFileName), targetDir, entryCount, preserveSymlink);
				}
			}
		}

		/// <summary>
		/// Unzip an input stream.
		/// </summary>
		/// <param name="stream">The stream.</param>
		/// <param name="targetDir">The full path to the destination folder.</param>
		public void UnzipStream(Stream stream, string targetDir)
		{
			UnzipStream(stream, targetDir, 0);
		}

		delegate bool CreateSymLinkDelegate(string from, string to);
		static CreateSymLinkDelegate _CreateSymLinkImpl;

		static bool CreateSymbolicLinkMono(string from, string to)
		{
			if (Syscall.symlink(from, to) != 0)
			{
				throw new Exception(string.Format("Unable to create symbolic link from '{0}' to '{1}'; aborting.", from, to));
			}
			return true;
		}

		[System.Runtime.InteropServices.DllImport("kernel32.dll", CharSet = System.Runtime.InteropServices.CharSet.Unicode, SetLastError = true)]
		internal static extern bool CreateSymbolicLink(string newFileName, string exitingFileName, uint flags);

		static bool CreateSymbolicLinkWindows(string from, string to)
		{
			// Windows is a little simpler than Mono, only requiring P/Invoking
			// to the Win32 function CreateSymbolicLink();

			FileAttributes attributes = File.GetAttributes(from);
			uint flags = (attributes & FileAttributes.Directory) == FileAttributes.Directory ? 1u : 0u;

			return CreateSymbolicLink(to, from, flags);
		}

		static bool CreateSymLink(string from, string to)
		{
			if (_CreateSymLinkImpl == null)
			{
				if (Type.GetType("Mono.Runtime") != null)
				{
					_CreateSymLinkImpl = CreateSymbolicLinkMono;
				}
				else
				{
					_CreateSymLinkImpl = CreateSymbolicLinkWindows;
				}
			}

			return _CreateSymLinkImpl(from, to);
		}

		struct SymLink
		{
			public string From;
			public string To;

			public SymLink(string from, string to)
			{
				From = from;
				To = to;
			}
		}

		/// <summary>
		/// Unzip an input stream.
		/// </summary>
		/// <param name="stream">The stream.</param>
		/// <param name="targetDir">The full path to the destination folder.</param>
		/// <param name="entryCount">The total number of entries.</param>
		/// <param name="preserveSymlink">If zip entry contains symlink, preserve that symlink if possible.</param>
		public void UnzipStream(Stream stream, string targetDir, int entryCount, bool preserveSymlink = true)
		{
			int entryIndex = 0;
			List<SymLink> symlinks = new List<SymLink>();

			using (var zipInputStream = new ZipInputStream(stream))
			{
				ZipEntry zipEntry;
				while ((zipEntry = zipInputStream.GetNextEntry()) != null)
				{
					string directoryName = Path.GetDirectoryName(zipEntry.Name);
					string fileName = Path.GetFileName(zipEntry.Name);

					// sanity check (for some reason one of the zip entries is the directory, which has no filename)
					if (fileName == "")
					{
						continue;
					}

					ZipEntry headerEntry = _header_entries[entryIndex];
					uint fileattrib = 0;

					if (headerEntry.Name == zipEntry.Name)
					{
						fileattrib = (uint)headerEntry.ExternalFileAttributes;
					}
					else
					{
						foreach (ZipEntry zi in _header_entries)
						{
							if (zi.Name.Equals(zipEntry.Name))
							{
								fileattrib = (uint)zi.ExternalFileAttributes;
								break;
							}
						}
					}

					string outDir = Path.Combine(targetDir, directoryName);
					string outFile = Path.Combine(targetDir, zipEntry.Name);

					// create directory for entry
					Directory.CreateDirectory(outDir);

					// create file
					try
					{
						FilePermissions unixPermissions = ExtractUnixPermissions(fileattrib);

						if ((unixPermissions & FilePermissions.S_IFLNK) == FilePermissions.S_IFLNK)
						{
							// The link target is identified by the contents of
							// the file data for this zip entry.
							long bytes = zipInputStream.Length;
							byte[] linkSourceBytes = new byte[bytes];
							zipInputStream.Read(linkSourceBytes, 0, linkSourceBytes.Length);
							string relativeLinkSource = System.Text.ASCIIEncoding.ASCII.GetString(linkSourceBytes);
							string linkSource = PathString.MakeNormalized(Path.Combine(outDir, relativeLinkSource)).Path;
							string normalizedOutFile = PathString.MakeNormalized(outFile).Path;

							// Symbolic link creation is deferred to the end after all regular files have been extracted.
							// Test to see if any of the output is actual an input to previous symlink source.  If that is the case
							// insert this new symlink just before first usage so that this file will be created first.
							int prevSouceIsSymLinkIdx = symlinks.FindIndex(sl => sl.From == normalizedOutFile);
							if (prevSouceIsSymLinkIdx >= 0)
								symlinks.Insert(prevSouceIsSymLinkIdx, new SymLink(linkSource, normalizedOutFile));
							else
								symlinks.Add(new SymLink(linkSource, normalizedOutFile));
						}
						else
						{
							using (FileStream streamWriter = File.Create(outFile))
							{
								// data i/o
								int size = 2048;
								byte[] data = new byte[2048];
								while (true)
								{
									size = zipInputStream.Read(data, 0, data.Length);
									if (size > 0)
									{
										streamWriter.Write(data, 0, size);
									}
									else
									{
										break;
									}
								}
								streamWriter.Close();
							}

							// HostSystem seems to always return 0 (ie Msdos) even with zip files created on osx or unix machines directly.
							if (GetHostID() == zipEntry.HostSystem || zipEntry.HostSystem == (int)HostSystemID.Msdos)
							{
								File.SetAttributes(outFile, (FileAttributes)fileattrib);
							}

							if (PlatformUtil.IsMonoRuntime
								&& (PlatformUtil.IsOSX || PlatformUtil.IsUnix)
								// Commenting out HostSystem test because it seems to always return 0.
								// && IsUnixOrOsxHostID(zipEntry.HostSystem)
								// since we don't have hostsystem info, we test the unxipermission info
								// to make sure it has regular file attribute set.
								&& (unixPermissions & FilePermissions.S_IFREG) == FilePermissions.S_IFREG
							)
							{
								Syscall.chmod(outFile, (Mono.Unix.Native.FilePermissions)unixPermissions);
							}
						}

					}
					catch (Exception ex)
					{
						string msg = String.Format("Unzip failed to write uncompressed file '{0}'.\nDetails:  {1}", outFile, ex.Message);
						throw new Exception(msg, ex);
					}

					// NOTE the created date is right now, but the modified date is stored in the zip entry
					//File.SetLastWriteTime(outFile, zipEntry.DateTime);

					// call the internal event handler
					OnZipEvent(new ZipEventArgs(zipEntry, entryIndex++, entryCount));
				}
			}

			// Symbolic link creation is deferred to the end to ensure all 
			// regular files have first been unarchived.
			foreach (SymLink symlink in symlinks)
			{
				if (preserveSymlink)
				{
					CreateSymLink(symlink.From, symlink.To);
					if (!File.Exists(symlink.To))
					{
						// On Windows, CreateSymbolicLink() function can return true even if it failed due to insufficient privilege.
						// So test for file exists and if it fail, we just do a basic copy.
						File.Copy(symlink.From, symlink.To);
					}
				}
				else
				{
					File.Copy(symlink.From, symlink.To);
				}
			}
		}

		/// <summary>
		/// Create a zip file.
		/// </summary>
		/// <param name="fileNames">The collection of files to add to the zip file.</param>
		/// <param name="baseDirectory">The full path to the basedirectory from which each zipentry will be made relative to.</param>
		/// <param name="zipFileName">The full path to the zip file.</param>
		/// <param name="zipEntryDir">The base path to prepend to each zip entry, should be relative. May be null for none.</param>
		/// <param name="zipLevel">Compression level. May be 0 for default compression.</param>
		/// <param name="useModTime">Preserve last modified timestamp of each file.</param>
		public void ZipFile(StringCollection fileNames, string baseDirectory, string zipFileName, string zipEntryDir, int zipLevel, bool useModTime)
		{
			int entryIndex = 0;
			int entryCount = fileNames.Count;

			FileInfo zipFileInfo = new FileInfo(zipFileName);
			if (Directory.Exists(zipFileInfo.DirectoryName) == false)
			{
				Directory.CreateDirectory(zipFileInfo.DirectoryName);
			}

			using (ZipOutputStream zOutstream = new ZipOutputStream(File.Create(zipFileInfo.FullName)))
			{
				zOutstream.IsStreamOwner = true;
				zOutstream.SetLevel(zipLevel);

				foreach (string fileName in fileNames)
				{
					// check file ecists
					FileInfo fileInfo = new FileInfo(fileName);
					if (fileInfo.Exists == false)
					{
						throw new FileNotFoundException(String.Format("Cannot find zip file input: {0}", fileName));
					}

					// modify path for zip entry
					string pathInZipFile = fileName;
					if (Path.IsPathRooted(pathInZipFile) == true)
					{
						pathInZipFile = PathNormalizer.MakeRelative(pathInZipFile, baseDirectory, true);
					}
					if (zipEntryDir != null)
					{
						pathInZipFile = Path.Combine(zipEntryDir, pathInZipFile);
					}
					pathInZipFile = pathInZipFile.Replace(@"\", "/");
					pathInZipFile = ZipEntry.CleanName(pathInZipFile);

					// create ZIP entry
					ZipEntry zipEntry = new ZipEntry(pathInZipFile);
					if (useModTime)
					{
						// use the source file's last modified timestamp
						zipEntry.DateTime = fileInfo.LastWriteTime;
					}
					zipEntry.Size = fileInfo.Length;
					zipEntry.ExternalFileAttributes = (int)fileInfo.Attributes;
					if (PlatformUtil.IsMonoRuntime && (PlatformUtil.IsUnix || PlatformUtil.IsOSX))
					{
						Stat statObj;
						if (Syscall.stat(fileName, out statObj) != 0)
						{
							throw new Exception(string.Format("Unable to get permissions for file '{0}'.", fileName));
						}
						FilePermissions unixPermissions = statObj.st_mode;
						zipEntry.ExternalFileAttributes = AddUnixPermissions(zipEntry.ExternalFileAttributes, unixPermissions);
					}
					zipEntry.HostSystem = GetHostID();
					if (useModTime)
					{
						zipEntry.DateTime = fileInfo.LastWriteTime; // use the source file's last modified timestamp
					}

					zOutstream.PutNextEntry(zipEntry);

					// write file to stream
					using (FileStream streamReader = File.OpenRead(fileName))
					{
						StreamUtils.Copy(streamReader, zOutstream, new byte[4096]);
						zOutstream.CloseEntry();
					}

					// call the internal event handler
					OnZipEvent(new ZipEventArgs(zipEntry, entryIndex++, entryCount));
				}

				zOutstream.Finish();
			}
		}
	}
}
