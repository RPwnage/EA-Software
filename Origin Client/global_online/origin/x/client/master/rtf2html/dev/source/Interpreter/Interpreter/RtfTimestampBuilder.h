// -- FILE ------------------------------------------------------------------
// name       : RtfTimestampBuilder.cs
// project    : RTF Framelet
// created    : Leon Poyyayil - 2008.05.23
// language   : c#
// environment: .NET 2.0
// copyright  : (c) 2004-2013 by Jani Giannoudis, Switzerland
// --------------------------------------------------------------------------
//using System;
//using Itenso.Rtf.Support;

#ifndef __RTFTIMESTAMPBUILDER_H
#define __RTFTIMESTAMPBUILDER_H

#include <qdatetime.h>
#include "RtfElementVisitorBase.h"

namespace RTF2HTML
{

    // ------------------------------------------------------------------------
    class RtfTimestampBuilder : public RtfElementVisitorBase
    {
        public:

            // ----------------------------------------------------------------------
            RtfTimestampBuilder(QObject* parent = 0);

            // ----------------------------------------------------------------------
            void Reset();
            // ----------------------------------------------------------------------
            QDateTime* CreateTimestamp();

            // ----------------------------------------------------------------------
            virtual void DoVisitTag( RtfTag* tag );

        private:
            // ----------------------------------------------------------------------
            // members
            int year;
            int month;
            int day;
            int hour;
            int minutes;
            int seconds;

    }; // class RtfTimestampBuilder

} // namespace Itenso.Rtf.Interpreter
// -- EOF -------------------------------------------------------------------
#endif //__RTFTIMESTAMPBUILDER_H