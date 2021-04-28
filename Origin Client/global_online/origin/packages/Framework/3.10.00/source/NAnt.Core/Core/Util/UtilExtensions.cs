using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace NAnt.Core.Util
{
    public static class UtilExtensions
    {
        public static  void AddNonEmpty(this ICollection<string> target, string value)
        {
            if (target != null && !String.IsNullOrWhiteSpace(value))
            {
                target.Add(value);
            }
        }

        public static void AddNonEmpty(this IDictionary<string, string> target, string name, string value)
        {
            if (target != null && !String.IsNullOrWhiteSpace(value) && !String.IsNullOrWhiteSpace(name))
            {
                target.Add(name, value);
            }
        }

        #region OrderedDistinct

        public static IEnumerable<TSource> OrderedDistinct<TSource>(this IEnumerable<TSource> source)
        {
            if (source == null) throw new ArgumentNullException("source");

            return OrderedDistinct<TSource>(source, null);
        }

        public static IEnumerable<TSource> OrderedDistinct<TSource>(this IEnumerable<TSource> source, IEqualityComparer<TSource> comparer)
        {
            if (source == null) throw new ArgumentNullException("source");

            if (comparer == null)
                comparer = EqualityComparer<TSource>.Default;

            return CreateOrderedDistinctIterator(source, comparer);
        }

        static IEnumerable<TSource> CreateOrderedDistinctIterator<TSource>(IEnumerable<TSource> source, IEqualityComparer<TSource> comparer)
        {
            
            var processed = new HashSet<TSource>(comparer);
            foreach (var element in source)
            {
                if (processed.Add(element))
                {
                    yield return element;
                }
            }
        }

        #endregion



    }
}
