// -- FILE ------------------------------------------------------------------
// name       : RtfDocumentInfoBuilder.cs
// project    : RTF Framelet
// created    : Leon Poyyayil - 2008.05.23
// language   : c#
// environment: .NET 2.0
// copyright  : (c) 2004-2013 by Jani Giannoudis, Switzerland
// --------------------------------------------------------------------------
//using System;
//using Itenso.Rtf.Model;
//using Itenso.Rtf.Support;

#ifndef __RTFDOCUMENTINFOBUILDER_H
#define __RTFDOCUMENTINFOBUILDER_H

#include "RtfElementVisitorBase.h"
#include "RtfDocumentInfo.h"
#include "RtfTextBuilder.h"
#include "RtfTimestampBuilder.h"

namespace RTF2HTML
{
    // ------------------------------------------------------------------------
    class RtfDocumentInfoBuilder : public RtfElementVisitorBase
    {
        public:
            // ----------------------------------------------------------------------
            RtfDocumentInfoBuilder( RtfDocumentInfo* info, QObject* parent = 0 );
            virtual ~RtfDocumentInfoBuilder ();

            // ----------------------------------------------------------------------
            void Reset();
            // ----------------------------------------------------------------------
            virtual void DoVisitGroup( RtfGroup* group );

            // ----------------------------------------------------------------------
            virtual void DoVisitTag( RtfTag* tag );

        private:
            // ----------------------------------------------------------------------
            QString& ExtractGroupText( RtfGroup* group );

            // ----------------------------------------------------------------------
            QDateTime* ExtractTimestamp( RtfGroup* group );

            // ----------------------------------------------------------------------
            // members
            RtfDocumentInfo* info;
            RtfTextBuilder* textBuilder;
            RtfTimestampBuilder* timestampBuilder;

    }; // class RtfDocumentInfoBuilder

} // namespace Itenso.Rtf.Interpreter
// -- EOF -------------------------------------------------------------------
#endif //__RTFDOCUMENTINFOBUILDER_H