// -- FILE ------------------------------------------------------------------
// name       : RtfElement.cs
// project    : RTF Framelet
// created    : Leon Poyyayil - 2008.05.19
// language   : c#
// environment: .NET 2.0
// copyright  : (c) 2004-2013 by Jani Giannoudis, Switzerland
// --------------------------------------------------------------------------
//using System;
//using Itenso.Sys;

#ifndef __RTFELEMENT_H
#define __RTFELEMENT_H

#include <string>
#include <ObjectBase.h>
#include <RtfElementVisitor.h>

namespace RTF2HTML
{
    namespace RtfElementKind
    {
        enum RtfElementKind
        {
            Tag,
            Group,
            Text
        }; // enum RtfElementKind
    }

    // ------------------------------------------------------------------------
    class RtfElement : public ObjectBase
    {
        public:

            RtfElement(QObject* parent = 0);
            RtfElement (RtfElementKind::RtfElementKind kind, QObject* parent = 0);

            virtual ~RtfElement();

            // ----------------------------------------------------------------------
            //protected RtfElement( RtfElementKind kind )
            //{
            //    this.kind = kind;
            //} // RtfElement

            // ----------------------------------------------------------------------
            virtual RtfElementKind::RtfElementKind Kind () {
                return mKind;
            }

            // ----------------------------------------------------------------------
            virtual void Visit( RtfElementVisitor* visitor );

            virtual bool Equals (RTF2HTML::ObjectBase* obj);

            virtual QString& ToString();
            virtual QString& GetType();

            virtual bool IsEqual (RTF2HTML::ObjectBase* obj );

            // ----------------------------------------------------------------------
            //protected abstract void DoVisit( IRtfElementVisitor visitor );
            virtual void DoVisit( RtfElementVisitor* visitor );
        private:
            // ----------------------------------------------------------------------
            // members
            RtfElementKind::RtfElementKind mKind;
    }; // class RtfElement

} // namespace Itenso.Rtf.Model
#endif //__RTFELEMENT_H
// -- EOF -------------------------------------------------------------------
