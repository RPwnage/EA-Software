// NAnt - A .NET build tool
// Copyright (C) 2001-2002 Gerry Shaw
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

using System;

namespace NAnt.Core.Util
{

    public static class NumericalStringComparator
    {
        static public int Compare(string x, string y)
        {
            return Compare(x, y, false);
        }

        static public int Compare(string x, string y, bool ignoreCase)
        {
            if (x == null)
            {
                throw new ArgumentNullException("x");
            }
            if (y == null)
            {
                throw new ArgumentNullException("y");
            }

            if (ignoreCase)
            {
                x = x.ToLower();
                y = y.ToLower();
            }

            int xIndex = 0;
            int yIndex = 0;
            int xLength = x.Length;
            int yLength = y.Length;
            Int64 diff = 0;
            while (diff == 0 && xIndex < xLength && yIndex < yLength)
            {
                diff = ignoreCase ? x[xIndex].CompareTo(y[yIndex]) : x[xIndex].CompareTo(y[yIndex]);
                if ((diff != 0 && (Char.IsDigit(x[xIndex]) || Char.IsDigit(y[yIndex]))) || (diff == 0 && Char.IsDigit(x[xIndex]) && Char.IsDigit(y[yIndex])))
                {
                    Int64 xNumber = -1;
                    if (Char.IsDigit(x[xIndex]))
                    {
                        int startingIndex = xIndex;
                        for (; xIndex < xLength && Char.IsDigit(x[xIndex]); ++xIndex) ;
                        xNumber = Convert.ToInt64(x.Substring(startingIndex, xIndex - startingIndex));
                    }

                    Int64 yNumber = -1;
                    if (Char.IsDigit(y[yIndex]))
                    {
                        int startingIndex = yIndex;
                        for (; yIndex < yLength && Char.IsDigit(y[yIndex]); ++yIndex) ;
                        yNumber = Convert.ToInt64(y.Substring(startingIndex, yIndex - startingIndex));
                    }
                    diff = xNumber - yNumber;
                }
                else
                {
                    ++xIndex;
                    ++yIndex;
                }
            }

            int comparison;
            if (diff == 0)
            {
                if (xIndex == xLength && yIndex < yLength)
                    comparison = -1;
                else if (xIndex < xLength && yIndex == yLength)
                    comparison = 1;
                else
                    comparison = 0;
            }
            else if (diff < 0)
                comparison = -1;
            else
                comparison = 1;

            return comparison;
        }
    }
}
