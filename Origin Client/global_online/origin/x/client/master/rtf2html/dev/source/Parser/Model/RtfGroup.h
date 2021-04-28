// -- FILE ------------------------------------------------------------------
// name       : RtfGroup.cs
// project    : RTF Framelet
// created    : Leon Poyyayil - 2008.05.19
// language   : c#
// environment: .NET 2.0
// copyright  : (c) 2004-2013 by Jani Giannoudis, Switzerland
// --------------------------------------------------------------------------
//using System;
//using System.Text;
//using Itenso.Sys;

#ifndef __RTFGROUP_H
#define __RTFGROUP_H
#include <string.h>
#include "rtfElement.h"
#include "RtfElementCollection.h"

namespace RTF2HTML
{

    // ------------------------------------------------------------------------
    class RtfGroup : public RtfElement
    {
        public:

            // ----------------------------------------------------------------------
            RtfGroup(QObject* parent = 0);
            virtual ~RtfGroup();

            // ----------------------------------------------------------------------
            RtfElementCollection& Contents ();

            // ----------------------------------------------------------------------
            RtfElementCollection& WritableContents ();

            // ----------------------------------------------------------------------
            QString& Destination ();
            // ----------------------------------------------------------------------
            bool IsExtensionDestination ();

            // ----------------------------------------------------------------------
            RtfGroup* SelectChildGroupWithDestination( QString& destination );

            // ----------------------------------------------------------------------
            virtual QString& ToString();

            // ----------------------------------------------------------------------
            virtual void DoVisit( RtfElementVisitor* visitor );

            virtual bool IsEqual (ObjectBase* obj);

        private:
            // ----------------------------------------------------------------------
            // members
            RtfElementCollection contents;

            QString grpStr;
            QString emptyStr;

    }; // class RtfGroup
} // namespace Itenso.Rtf.Model
#endif //__RTFGROUP_H
// -- EOF -------------------------------------------------------------------
