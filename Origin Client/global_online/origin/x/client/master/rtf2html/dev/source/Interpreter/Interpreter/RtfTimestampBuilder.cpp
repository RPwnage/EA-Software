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

#include "RtfTimestampBuilder.h"
#include "RtfTag.h"
#include "RtfSpec.h"

namespace RTF2HTML
{
    // ----------------------------------------------------------------------
    RtfTimestampBuilder::RtfTimestampBuilder(QObject* parent)
        : RtfElementVisitorBase ( RtfElementVisitorOrder::BreadthFirst, parent )
    {
        Reset();
    } // RtfTimestampBuilder

    // ----------------------------------------------------------------------
    void RtfTimestampBuilder::Reset()
    {
        year = 1970;
        month = 1;
        day = 1;
        hour = 0;
        minutes = 0;
        seconds = 0;
    } // Reset

    // ----------------------------------------------------------------------
    QDateTime* RtfTimestampBuilder::CreateTimestamp()
    {
        QDate date(year, month, day);
        QTime time(hour, minutes, seconds);
        QDateTime* dt = new QDateTime(date, time);
        return dt;
    } // CreateTimestamp

    // ----------------------------------------------------------------------
    void RtfTimestampBuilder::DoVisitTag( RtfTag* tag )
    {
        QString tagName = tag->Name();
        int value = tag->ValueAsNumber();

        if (tagName == RtfSpec::TagInfoYear)
        {
            year = value;
        }
        else if (tagName == RtfSpec::TagInfoMonth)
        {
            month = value;
        }
        else if (tagName == RtfSpec::TagInfoDay)
        {
            day = value;
        }
        else if (tagName == RtfSpec::TagInfoHour)
        {
            hour = value;
        }
        else if (tagName == RtfSpec::TagInfoMinute)
        {
            minutes = value;
        }
        else if (tagName == RtfSpec::TagInfoSecond)
        {
            seconds = value;
        }
    } // DoVisitTag

} // namespace Itenso.Rtf.Interpreter
// -- EOF -------------------------------------------------------------------
