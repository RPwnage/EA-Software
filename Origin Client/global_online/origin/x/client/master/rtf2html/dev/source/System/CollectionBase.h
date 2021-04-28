// make base class equivalent to .NET's ReadOnlyCollectionBase
// use this as the base class for all collection items

#ifndef __COLLECTIONBASE_H
#define __COLLECTIONBASE_H

#include "ObjectBase.h"

namespace RTF2HTML
{
    // ------------------------------------------------------------------------
    class CollectionBase: public ObjectBase
    {
        public:
            CollectionBase();
            virtual ~CollectionBase() {};
    };
} // namespace Itenso.Rtf.Model
#endif //__COLLECTIONBASE_H
// -- EOF -------------------------------------------------------------------
