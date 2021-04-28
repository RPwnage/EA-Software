using System;

using NAnt.Core;
using NAnt.Core.Writers;

namespace NAnt.Core.Events
{

    public class CachedWriterEventArgs : EventArgs
    {

        public CachedWriterEventArgs(bool isUpdatingFile, string filename)
        {
            IsUpdatingFile = isUpdatingFile;
            FileName = filename;
        }

        public readonly bool IsUpdatingFile;
        public readonly string FileName;
    }

    
    public delegate void CachedWriterEventHandler(object sender, CachedWriterEventArgs e);
}
