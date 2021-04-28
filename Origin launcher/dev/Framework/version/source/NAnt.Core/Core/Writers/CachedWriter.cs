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

using Microsoft.IO;

using NAnt.Core.Events;

namespace NAnt.Core.Writers
{
	public abstract class CachedWriter : ICachedWriter
	{
		public event CachedWriterEventHandler CacheFlushed;
		public string FileName { set; get; } = String.Empty;
		public MemoryStream Internal => Data; // backwards compatbility

		public readonly MemoryStream Data;

		private bool m_flushed = false;
		private bool m_disposed = false;

		private static bool s_keepBackups;

		private readonly static byte[] s_byteOrderMark = new byte[] { 0xEF, 0xBB, 0xBF };
		private readonly static RecyclableMemoryStreamManager s_streamManager = new RecyclableMemoryStreamManager();

		public CachedWriter()
		{
			Data = s_streamManager.GetStream(null, 84 * 1024, true);
		}

		public void Dispose()
		{
			Dispose(true);
		}

		/// <summary>
		/// Write memory content to the file. If file exist and content did not change leave file intact.
		/// </summary>
		public void Flush()
		{
			if (!m_flushed)
			{
				if (!String.IsNullOrEmpty(FileName)) // filename doesn't have to to be set, sometimes we use these writers in memoory only
				{
					CachedWriterEventArgs.FlushState state = CachedWriterEventArgs.FlushState.Updating;
					// create the path to the file if necessary
					string filePath = Path.GetDirectoryName(FileName);
					if (!String.IsNullOrEmpty(filePath) && !Directory.Exists(filePath))
					{
						Directory.CreateDirectory(filePath);
					}

					byte[] data = Data.GetBuffer();
					int dataLength = (int)Data.Length; // when using recycable stream buffer is possibly larger than what we wrote - only want the stream length

					// compare our contents to the existing file before we overwrite
					var fileInfo = new FileInfo(FileName);
					if (fileInfo.Exists)
					{
						if (IsEqual(fileInfo, data, dataLength))
						{
							state = CachedWriterEventArgs.FlushState.Unchanged;
						}
					}
					else
					{
						state = CachedWriterEventArgs.FlushState.Creating;
					}

					CacheFlushed?.Invoke(this, new CachedWriterEventArgs(state, FileName));

					//-- Only output if the contents are different
					if (state != CachedWriterEventArgs.FlushState.Unchanged)
					{
						if (s_keepBackups && File.Exists(FileName))
						{
							string backupFileName = GetBackupFileName(FileName);

							try
							{
								File.Delete(backupFileName);
							}
							catch (Exception)
							{
							}

							try
							{
								File.Move(FileName, backupFileName);
							}
							catch (Exception)
							{
								Console.WriteLine("Unable to create backup file '{0}'", backupFileName);
							}
						}

						if (Data.Length == 0)
						{
							using (File.Create(FileName))
							{ }
						}
						else
						{
							using (FileStream fileStream = File.Create(FileName, dataLength, FileOptions.Asynchronous))
							{
								fileStream.Write(data, 0, dataLength);
							}
						}
					}
				}

				m_flushed = true;
			}
		}

		public static void SetBackupGeneratedFilesPolicy(bool keepBackups)
		{
			s_keepBackups = keepBackups;
		}

		public static string GetBackupFileName(string file)
		{
			return file + ".bak";
		}

		protected virtual void Dispose(bool disposing) // DAVE-FUTURE-REFACTOR-TODO, this requires a overriding in a very specific way due recycablememorystreams not supporting idempotent .Dispose()
													   // be good to move this logic to be enforce in base class
		{
			if (!m_disposed)
			{
				if (disposing)
				{
					Flush();
					Data.Dispose();
				}
				m_disposed = true;
			}
		}

		// TODO-Remove: This stub is here because icepick had a custom task that extended this class and overrided this method
		// Remove this at some point in the future after icepick as been updated.
		protected virtual void DisposeResources() {}

		private static int BomSize(byte[] arr, long arrLength)
		{
			return arrLength > 3 && arr[0] == s_byteOrderMark[0] && arr[1] == s_byteOrderMark[1] && arr[2] == s_byteOrderMark[2] ? 3 : 0;
		}

		private static bool IsEqual(byte[] lhs, long lhsLength, byte[] rhs, long rhsLength)
		{
			int aBOM = BomSize(lhs, lhsLength);
			int bBOM = BomSize(rhs, rhsLength);

			if (lhsLength - aBOM != rhsLength - bBOM)
				return false;

			// Files are almost always equal, so optimize for that
			bool isEquals = true;
			while (aBOM < lhsLength)
			{
				isEquals &= lhs[aBOM] == rhs[bBOM];
				aBOM++;
				bBOM++;
			}
			return isEquals;
		}

		private static bool IsEqual(FileInfo fileInfo, byte[] data, int dataLength)
		{
			// Code is complicated to avoid doing 8mb allocations and cause lots of gc-calls
			// Sizes over ~84k hits the large object allocator
			int bufSize = 84000;

			if (dataLength < bufSize + 3) // Need to handle BOM differences so therefore we add 3
			{
				byte[] fileBytes = File.ReadAllBytes(fileInfo.FullName);
				return IsEqual(data, dataLength, fileBytes, fileBytes.Length);
			}

			int offset = 0;
			bool isFirstCall = true;
			byte[] oldBuf = new byte[bufSize];

			using (var f = fileInfo.OpenRead())
			{
				// Adjust for bom on new data
				int newBomSize = BomSize(data, dataLength);
				offset = newBomSize;
				int dataLengthWithoutBom = dataLength - newBomSize;

				int totalReadWithoutBom = 0;

				while (true)
				{
					// Read as many bytes as we can
					int readMax = bufSize;
					int readCount = f.Read(oldBuf, 0, readMax);
					totalReadWithoutBom += readCount;
					int consumed = 0;
					bool mightBeMore = readCount == readMax;

					if (isFirstCall)
					{
						// Adjust for bom on oldData
						int oldBomSize = BomSize(oldBuf, readCount);
						totalReadWithoutBom -= oldBomSize;
						consumed += oldBomSize;
						isFirstCall = false;
					}

					// Check if we read more than what the data is, in that case we don't match
					if (dataLengthWithoutBom < totalReadWithoutBom)
						return false;

					bool isEqual = true;
					while (consumed != readCount)
					{
						isEqual &= data[offset] == oldBuf[consumed];
						offset++;
						consumed++;
					}

					if (!isEqual)
						return false;

					// Still potentially more to read
					if (mightBeMore)
						continue;

					// If we consumed all new data it means they are the same
					return offset == dataLength;
				}
			}
		}
	}
}