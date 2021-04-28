// NAnt - A .NET build tool
// Copyright (C) 2001 Gerry Shaw
//
// Kosta Arvanitis (karvanitis@ea.com)
using System;
using System.IO;
using System.Collections.Specialized;

using ICSharpCode.SharpZipLib.Checksums;
using ICSharpCode.SharpZipLib.Zip;

using NAnt.Core.Util;

namespace EA.SharpZipLib
{
    public class ZipEventArgs : EventArgs
    {
        ZipEntry _zipEntry;
        int _entryIndex;
        int _entryCount;

        public ZipEventArgs(ZipEntry zipEntry, int entryIndex, int entryCount)
        {
            _zipEntry = zipEntry;
            _entryIndex = entryIndex;
            _entryCount = entryCount;
        }

        public ZipEntry ZipEntry
        {
            get { return _zipEntry; }
        }

        public int EntryIndex
        {
            get { return _entryIndex; }
        }

        public int EntryCount
        {
            get { return _entryCount; }
        }
    }

    public delegate void ZipEventHandler(object source, ZipEventArgs e);

    public class ZipLib
    {

        public event ZipEventHandler ZipEvent;
        Crc32 crc = new Crc32();

        private ZipFile _header_entries;

        public ZipLib()
        {
        }

        public static int GetHostID()
        {
            return System.Environment.OSVersion.Platform == PlatformID.Unix ? 3 : 0;
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
        /// Unzip a file.
        /// </summary>
        /// <param name="zipFileName">The full path to the zip file.</param>
        /// <param name="targetDir">The full path to the destination folder.</param>
        public void UnzipFile(string zipFileName, string targetDir)
        {
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

                    UnzipStream(File.OpenRead(zipFileName), targetDir, entryCount);
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

        /// <summary>
        /// Unzip an input stream.
        /// </summary>
        /// <param name="stream">The stream.</param>
        /// <param name="targetDir">The full path to the destination folder.</param>
        /// <param name="entryCount">The total number of entries.</param>
        public void UnzipStream(Stream stream, string targetDir, int entryCount)
        {
            int entryIndex = 0;

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
                    FileAttributes fileattrib = 0;

                    if (headerEntry.Name == zipEntry.Name)
                    {
                        fileattrib = (FileAttributes)headerEntry.ExternalFileAttributes;
                    }
                    else
                    {
                        foreach (ZipEntry zi in _header_entries)
                        {
                            if (zi.Name.Equals(zipEntry.Name))
                            {
                                fileattrib = (FileAttributes)zi.ExternalFileAttributes;
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

                        if (GetHostID() == zipEntry.HostSystem)
                        {
                            File.SetAttributes(outFile, fileattrib);
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
        }

        /// <summary>
        /// Create a zip file.
        /// </summary>
        /// <param name="fileNames">The collection of files to add to the zip file.</param>
        /// <param name="baseDirectory">The full path to the basedirectory from which each zipentry will be made relative to.</param>
        /// <param name="zipFileName">The full path to the zip file.</param>
        /// <param name="zipEntryDir">The base path to append to each zip entry, should be relative. May be null for none.</param>
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
                zOutstream.SetLevel(zipLevel);

                foreach (string file in fileNames)
                {
                    string fpath = file;

                    if (File.Exists(file) == false)
                    {
                        throw new FileNotFoundException();
                    }

                    if (Path.IsPathRooted(fpath) == true)
                    {
                        fpath = PathNormalizer.MakeRelative(file, baseDirectory, true);
                    }

                    // read source file
                    using (FileStream streamReader = File.OpenRead(file))
                    {
                        long fileSize = streamReader.Length;
                        byte[] buffer = new byte[fileSize];
                        streamReader.Read(buffer, 0, buffer.Length);
                        streamReader.Close();

                        // modify path for zip entry
                        if (zipEntryDir != null)
                        {
                            fpath = Path.Combine(zipEntryDir, fpath);
                        }
                        //Replace to unix-style dir separators resulting path.
                        fpath = fpath.Replace(@"\", "/");


                        // create ZIP entry
                        ZipEntry zipEntry = new ZipEntry(fpath);

                        if (zipLevel == 0)
                        {
                            zipEntry.Size = fileSize;

                            // calculate crc32 of current file
                            crc.Reset();
                            crc.Update(buffer);
                            zipEntry.Crc = crc.Value;
                        }

                        string srcPath = Path.GetFullPath(file);
                        var fInfo = new FileInfo(file);
                        zipEntry.Size = fInfo.Length;

                        if (useModTime)
                        {
                            // use the source file's last modified timestamp
                            zipEntry.DateTime = fInfo.LastWriteTime;
                        }
                        
                        zipEntry.ExternalFileAttributes = (int)fInfo.Attributes;
                        zipEntry.HostSystem = GetHostID();

                        // write file to ZIP
                        zOutstream.PutNextEntry(zipEntry);
                        zOutstream.Write(buffer, 0, buffer.Length);

                        // call the internal event handler
                        OnZipEvent(new ZipEventArgs(zipEntry, entryIndex++, entryCount));
                    }
                }

                zOutstream.Finish();
                zOutstream.Close();
            }
        }
    }
}
