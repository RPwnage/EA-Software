using System;
using System.Collections;
using System.Collections.Generic;
using System.Collections.Concurrent;
using System.Diagnostics;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Linq;

namespace NAnt.Core
{
    #region Blocking GetOrAdd / AddOrUpdate
    public static class ConcurrentDictionaryBlockingExtensions
    {
        public static TValue GetOrAddBlocking<TKey, TValue>(this ConcurrentDictionary<TKey, Lazy<TValue>> dictionary, TKey key, Func<TKey, TValue> valueFactory)
        {
            return dictionary.GetOrAdd(key, new Lazy<TValue>(() => valueFactory(key))).Value;
        }

        public static TValue AddOrUpdateBlocking<TKey, TValue>(this ConcurrentDictionary<TKey, Lazy<TValue>> dictionary, TKey key, Func<TKey, TValue> addValueFactory, Func<TKey, TValue, TValue> updateValueFactory)
        {
            return dictionary.AddOrUpdate(key, new Lazy<TValue>(() => addValueFactory(key)), (k, oldValue) => new Lazy<TValue>(() => updateValueFactory(k, oldValue.Value))).Value;
        }

        public static TValue AddOrUpdateBlocking<TKey, TValue>(this ConcurrentDictionary<TKey, Lazy<TValue>> dictionary, TKey key, TValue addValue, Func<TKey, TValue, TValue> updateValueFactory)
        {
            return dictionary.AddOrUpdate(key, new Lazy<TValue>(() => addValue, false), (k, oldValue) => new Lazy<TValue>(() => updateValueFactory(k, oldValue.Value))).Value;
        }

        public static bool TryRemoveBlocking<TKey, TValue>(this ConcurrentDictionary<TKey, Lazy<TValue>> dictionary, TKey key)
        {
            Lazy<TValue> val;
            return dictionary.TryRemove(key, out val);
        }
    }
    #endregion

    [DebuggerDisplay("Count = {Count}")]
    [ComVisible(false)]
    public class BlockingConcurrentDictionary<TKey, TValue> : IDictionary<TKey, TValue>, IEnumerable<KeyValuePair<TKey, TValue>>  //, ICollection<KeyValuePair<TKey, TValue>>, IEnumerable<KeyValuePair<TKey, TValue>>, IDictionary, ICollection, IEnumerable
    {
        private ConcurrentDictionary<TKey, Lazy<TValue>> _impl;

        private IEnumerable<KeyValuePair<TKey, Lazy<TValue>>> ToLazy(IEnumerable<KeyValuePair<TKey, TValue>> collection)
        {
            return collection.Select(pair => new KeyValuePair<TKey, Lazy<TValue>>(pair.Key, new Lazy<TValue>(() => pair.Value, false)));
        }

        public BlockingConcurrentDictionary()
        {
            _impl = new ConcurrentDictionary<TKey, Lazy<TValue>>();
        }

        public BlockingConcurrentDictionary(IEnumerable<KeyValuePair<TKey, TValue>> collection)
        {
            _impl = new ConcurrentDictionary<TKey, Lazy<TValue>>(ToLazy(collection));
        }

        public BlockingConcurrentDictionary(IEqualityComparer<TKey> comparer)
        {
            _impl = new ConcurrentDictionary<TKey, Lazy<TValue>>(comparer);
        }

        public BlockingConcurrentDictionary(IEnumerable<KeyValuePair<TKey, TValue>> collection, IEqualityComparer<TKey> comparer)
        {
            _impl = new ConcurrentDictionary<TKey, Lazy<TValue>>(comparer);
        }

        public BlockingConcurrentDictionary(int concurrencyLevel, int capacity)
        {
            _impl = new ConcurrentDictionary<TKey, Lazy<TValue>>(concurrencyLevel, capacity);
        }

        public BlockingConcurrentDictionary(int concurrencyLevel, IEnumerable<KeyValuePair<TKey, TValue>> collection, IEqualityComparer<TKey> comparer)
        {
            _impl = new ConcurrentDictionary<TKey, Lazy<TValue>>(concurrencyLevel, ToLazy(collection), comparer);
        }

        public BlockingConcurrentDictionary(int concurrencyLevel, int capacity, IEqualityComparer<TKey> comparer)
        {
            _impl = new ConcurrentDictionary<TKey, Lazy<TValue>>(concurrencyLevel, capacity, comparer);
        }

        public int Count
        {
            get
            {
                return _impl.Count;
            }
        }

        public bool IsEmpty
        {
            get
            {
                return _impl.IsEmpty;
            }
        }

        public void Clear()
        {
            _impl.Clear();
        }

        public bool ContainsKey(TKey key)
        {
            return _impl.ContainsKey(key);
        }

        public IEnumerator<KeyValuePair<TKey, TValue>> GetEnumerator()
        {
            foreach (var e in _impl)
            {
                yield return new KeyValuePair<TKey, TValue>(e.Key, e.Value.Value);
            }
        }

        IEnumerator IEnumerable.GetEnumerator()
        {
            return GetEnumerator();
        }

        public ICollection<TKey> Keys
        {
            get
            {
                return _impl.Keys;
            }
        }

        public ICollection<TValue> Values
        {
            get
            {
                return _impl.Values.Select(v => v.Value).ToList();
            }
        }

        public TValue this[TKey key]
        {
            get
            {
                return _impl[key].Value;
            }
            set
            {
                _impl[key] = new Lazy<TValue>(() => value, false);
            }
        }

        public TValue AddOrUpdate(TKey key, Func<TKey, TValue> addValueFactory, Func<TKey, TValue, TValue> updateValueFactory)
        {
            return _impl.AddOrUpdateBlocking(key, addValueFactory, updateValueFactory);
        }

        public TValue AddOrUpdate(TKey key, TValue addValue, Func<TKey, TValue, TValue> updateValueFactory)
        {
            return _impl.AddOrUpdateBlocking(key, addValue, updateValueFactory);
        }


        public TValue GetOrAdd(TKey key, Func<TKey, TValue> valueFactory)
        {
            return _impl.GetOrAddBlocking(key, valueFactory);
        }

        public TValue GetOrAdd(TKey key, TValue value)
        {
            return _impl.GetOrAdd(key, new Lazy<TValue>(() => value, false)).Value;
        }

        public KeyValuePair<TKey, TValue>[] ToArray()
        {
            return _impl.Select(e => new KeyValuePair<TKey, TValue>(e.Key, e.Value.Value)).ToArray();
        }

        public bool TryAdd(TKey key, TValue value)
        {
            return _impl.TryAdd(key, new Lazy<TValue>(() => value, false));
        }

        public bool TryGetValue(TKey key, out TValue value)
        {
            Lazy<TValue> lazyValue;
            if (_impl.TryGetValue(key, out lazyValue))
            {
                value = lazyValue.Value;
                return true;
            }
            value = default(TValue);
            return false;
        }

        public bool TryRemove(TKey key, out TValue value)
        {
            Lazy<TValue> lazyValue;
            if (_impl.TryRemove(key, out lazyValue))
            {
                value = lazyValue.Value;
                return true;
            }
            value = default(TValue);
            return false;
        }

        private bool TryUpdate(TKey key, TValue newValue, TValue comparisonValue)
        {
            return _impl.TryUpdate(key, new Lazy<TValue>(() => newValue, false), new Lazy<TValue>(() => comparisonValue, false));
        }

        public bool IsReadOnly { get { return false; } }

        public void Add(TKey key, TValue value)
        {
            while (!TryAdd(key, value)) ;
        }

        public bool Remove(TKey key)
        {
            Lazy<TValue> lazy;
            return _impl.TryRemove(key, out lazy);
        }

        void ICollection<KeyValuePair<TKey, TValue>>.Add(KeyValuePair<TKey, TValue> pair)
        {
            Add(pair.Key, pair.Value);
        }

        bool ICollection<KeyValuePair<TKey, TValue>>.Contains(KeyValuePair<TKey, TValue> pair)
        {
            return ContainsKey(pair.Key);
        }
        bool ICollection<KeyValuePair<TKey, TValue>>.Remove(KeyValuePair<TKey, TValue> pair)
        {
            return Remove(pair.Key);
        }

        public bool IsFixedSize
        {
            get
            {
                return false;
            }
        }

        public void CopyTo(KeyValuePair<TKey, TValue>[] array, int startIndex)
        {
            _impl.Select(e => new KeyValuePair<TKey, TValue>(e.Key, e.Value.Value)).ToArray().CopyTo(array, startIndex);
        }

        void CopyTo(KeyValuePair<TKey, TValue>[] array, int startIndex, int num)
        {
            _impl.Select(e => new KeyValuePair<TKey, TValue>(e.Key, e.Value.Value)).ToList().CopyTo(0, array, startIndex, num);
        }
    }
}