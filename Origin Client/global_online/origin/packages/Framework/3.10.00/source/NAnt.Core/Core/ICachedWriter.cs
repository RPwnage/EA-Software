using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;

using NAnt.Core.Events;

namespace NAnt.Core
{
    public interface ICachedWriter : IDisposable
    {

        string FileName { set; get; }

        MemoryStream Internal { get; }

        /// <summary>
        /// Appends content of another writer to this one.
        /// </summary>
        void Append(ICachedWriter other);

        /// <summary>
        /// Write memory content to the file. If file exist and content did not change leave file intact.
        /// </summary>
        void Flush();

        event CachedWriterEventHandler CacheFlushed;
    }
}
