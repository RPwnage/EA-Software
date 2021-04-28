using System;
using System.Collections.Concurrent;

namespace NAnt.Core
{
    /// <summary>
    /// Provides a dictionary of named option sets.  This is used by NAnt and the
    /// <see cref="NAnt.Core.Tasks.OptionSetTask"/> to create named option sets.
    /// </summary>
    public class ObjectDictionary : ConcurrentDictionary<Guid, object>
    {
        public bool Contains(Guid name)
        {
            return ContainsKey(name);
        }

        public bool Add(Guid name, object obj)
        {
            return TryAdd(name, obj);
        }

        public bool Remove(Guid name)
        {
            object obj;
            return TryRemove(name, out obj);
        }

        internal ObjectDictionary() : base()
        {
        }
        internal ObjectDictionary(ConcurrentDictionary<Guid, object> collection)
            : base(collection)
        {
        }
    }

}
