// -- FILE ------------------------------------------------------------------
// name       : ReadOnlyBaseCollection.cs
// project    : RTF Framelet
// created    : Leon Poyyayil - 2008.05.20
// language   : c#
// environment: .NET 2.0
// copyright  : (c) 2004-2013 by Jani Giannoudis, Switzerland
// --------------------------------------------------------------------------
//using System.Collections;
//using Itenso.Sys;
//using Itenso.Sys.Collection;

#ifndef __READONLYBASECOLLECTION_H
#define __READONLYBASECOLLECTION_H

#include <vector>
#include <CollectionBase.h>

namespace RTF2HTML
{
    // ------------------------------------------------------------------------
    class ReadOnlyBaseCollection: public ObjectBase
    {
        public:
            ReadOnlyBaseCollection(QObject* parent = 0);
            virtual ~ReadOnlyBaseCollection();

            int Count();

            std::vector<RTF2HTML::ObjectBase*>& InnerList() {
                return mInnerList;
            }

            virtual bool Equals (ObjectBase* obj);
            virtual QString& ToString();

        private:
            virtual bool IsEqual (ObjectBase* obj);

            std::vector<RTF2HTML::ObjectBase*> mInnerList;
            QString toStr;
    }; // class ReadOnlyBaseCollection

} // namespace Itenso.Rtf.Model
#endif //__READONLYBASECOLLECTION_H
// -- EOF -------------------------------------------------------------------
