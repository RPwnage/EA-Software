using System;
using System.Collections;

namespace NAnt.Core.PackageCore
{
    public enum PackageColumn
    {
        Name = 0,   // string
        Size,       // int
        UseCount,   // int
        LastUsed,   // date
        Status,     // string
        License,    // string
        Contact,    // string
        Summary,    // string
    }

    public enum PackageSortOrder
    {
        None = 0,
        Ascending = 1,
        Descending = 2,
    }

    public class PackageSorter : IComparer
    {
        public PackageSorter(PackageColumn column, PackageSortOrder sortOrder)
        {
            Column = column;
            SortOrder = sortOrder;
        }

        public PackageColumn Column;
        public PackageSortOrder SortOrder;

        public int Compare(object a, object b)
        {
            if (SortOrder == PackageSortOrder.Ascending)
                return Compare((Release)a, (Release)b);
            else if (SortOrder == PackageSortOrder.Descending)
                return Compare((Release)b, (Release)a);
            else
                return 0;
        }

        private int Compare(Release a, Release b)
        {
            switch (Column)
            {
                case PackageColumn.Name:
                    return String.Compare(a.FullName, b.FullName, true);
                case PackageColumn.Size:
                    return a.GetSize().CompareTo(b.GetSize());
                case PackageColumn.Status:
                    return a.Status.CompareTo(b.Status);
                case PackageColumn.License:
                    return String.Compare(a.License, b.License, true);
                case PackageColumn.Contact:
                    return String.Compare(a.ContactName, b.ContactName, true);
                case PackageColumn.Summary:
                    return String.Compare(a.Summary, b.Summary, true);
                default:
                    return 0;
            }
        }
    }
}
