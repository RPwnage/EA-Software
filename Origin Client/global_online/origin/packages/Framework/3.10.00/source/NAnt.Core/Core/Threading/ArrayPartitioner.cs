using System;
using System.Collections.Generic;
using System.Collections.Concurrent;

namespace NAnt.Core.Threading
{
    /*
    class ArrayPartitioner<T> : OrderablePartitioner<T>
    {
        private ICollection<T> _array;

        public ArrayPartitioner(ICollection<T> array, bool keysOrderedInEachPartition = true, bool keysOrderedAcrossPartitions = true, bool keysNormalized=false) 
            : base(keysOrderedInEachPartition, keysOrderedAcrossPartitions, keysNormalized)
        {
            _array = array;
        }

        public override bool SupportsDynamicPartitions { get { return false; } }

        //
        // Summary:
        //     Partitions the underlying collection into the specified number of orderable
        //     partitions.
        //
        // Parameters:
        //   partitionCount:
        //     The number of partitions to create.
        //
        // Returns:
        //     A list containing partitionCount enumerators.
        public IList<IEnumerator<KeyValuePair<long, T>>> GetOrderablePartitions(int partitionCount)
        {
            //int patritionSize = _array.
            return null;

        }
    }
     */
}
