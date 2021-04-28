using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace NAnt.Core.Util
{

    public class BitMask
    {
        public const uint None = 0;

        public BitMask(uint type = 0)
        {
            _types = type;
        }

        public void SetType(uint type)
        {
            _types |= type;
        }

        public void ClearType(uint type)
        {
            _types &= ~type;
        }

        public bool IsKindOf(uint t)
        {
            return (_types & t) != 0;
        }

        public uint Type
        {
            get
            {
                return _types;
            }
        }

        private uint _types;
    }

}
