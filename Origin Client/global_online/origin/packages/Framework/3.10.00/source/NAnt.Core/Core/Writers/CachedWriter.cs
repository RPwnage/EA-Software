using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;

using NAnt.Core.Events;

namespace NAnt.Core.Writers
{
    

    public abstract class CachedWriter : ICachedWriter
    {
        private bool disposed = false;
        private int _bufferSize = 1000;
        private string _fileName = String.Empty;
        protected MemoryStream _data;

        public event CachedWriterEventHandler CacheFlushed;

        public string FileName
        {
            set { _fileName = value; }
            get { return _fileName; }
        }

        public MemoryStream Internal
        {
            get
            {
                return _data;
            }
        }

        public CachedWriter()
        {
            _data = new MemoryStream(_bufferSize);
        }

        virtual public void Append(ICachedWriter other)
        {
            if (other != null)
            {
                byte[] buffer = other.Internal.ToArray();
                _data.Write(buffer, 0, buffer.Length);
            }
        }

        /// <summary>
        /// Write memory content to the file. If file exist and content did not change leave file intact.
        /// </summary>
        public void Flush()
        {
            if (!String.IsNullOrEmpty(_fileName))
            {
                bool isDifferent = true;
                // create the path to the file if necessary
                string filePath = Path.GetDirectoryName(_fileName);
                if (!String.IsNullOrEmpty(filePath) && !Directory.Exists(filePath))
                {
                    Directory.CreateDirectory(filePath);
                }

                byte[] data = _data.ToArray();

                // compare our contents to the existing file before we overwrite
                if (File.Exists(_fileName) && IsEqual(data, File.ReadAllBytes(_fileName)))
                {
                    isDifferent = false;
                }

                if (CacheFlushed != null)
                {
                    CacheFlushed(this, new CachedWriterEventArgs(isDifferent, _fileName));
                }

                //-- Only output if the contents are different
                if (isDifferent)
                {
                    File.WriteAllBytes(_fileName, data);
                }
            }
        }

        public void Dispose()
        {
            Dispose(true);
            // This object will be cleaned up by the Dispose method.
            // Therefore, you should call GC.SupressFinalize to
            // take this object off the finalization queue
            // and prevent finalization code for this object
            // from executing a second time.
            GC.SuppressFinalize(this);
        }

        private void Dispose(bool disposing)
        {
            // Check to see if Dispose has already been called.
            if (!this.disposed)
            {
                // If disposing equals true, dispose all managed
                // and unmanaged resources.
                if (disposing)
                {
                    DisposeResources();
                }
                // Call the appropriate methods to clean up
                // unmanaged resources here.
                // If disposing is false, only the following code is executed.
                Flush();


                // Note disposing has been done.
                disposed = true;
            }
        }

        protected abstract void DisposeResources();

        private static bool IsEqual(byte[] a, byte[] b)
        {
            int aBOM = 0;
            int bBOM = 0;
            if (a.Length > 3 && a[0] == BOM[0] && a[1] == BOM[1] && a[2] == BOM[2])
            {
                aBOM = 3;
            }
            if (b.Length > 3 && b[0] == BOM[0] && b[1] == BOM[1] && b[2] == BOM[2])
            {
                bBOM = 3;
            } 
 
            if (a.Length - aBOM != b.Length - bBOM)
                return false;

            while (aBOM < a.Length)
            {
                if (a[aBOM] != b[bBOM])
                    return false;
                aBOM++;
                bBOM++;
            }
            return true;
        }

        private readonly static byte[] BOM = new byte[] { 0xEF, 0xBB, 0xBF };
    }
}
